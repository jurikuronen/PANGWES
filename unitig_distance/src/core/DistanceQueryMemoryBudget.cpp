/*
 * DistanceQueryMemoryBudget.cpp - Class for calculating the memory budget for distance-query computations.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "common/io/FileReader.hpp"
#include "common/io/Log.hpp"
#include "common/utils/Exception.hpp"
#include "common/utils/memory.hpp"
#include "common/utils/Timer.hpp"
#include "common/utils/utils.hpp"
#include "common/utils/WorkerPool.hpp"
#include "unitig_distance/core/DistanceQueryMemoryBudget.hpp"
#include "unitig_distance/sgg/SGG.hpp"
#include "unitig_distance/sgg/SGGEdges.hpp"

namespace PANGWES {
namespace {

// Values used to estimate a good batch size of SGGs to estimate the maximum memory requirement.
constexpr std::size_t MIN_SGGS_TO_ESTIMATE = 50;
constexpr std::size_t SGG_ESTIMATION_PERCENT = 5;
constexpr std::size_t DYNAMIC_MEMORY_ALLOWANCE_PER_THREAD = Memory::MiB * 2;

// Margin taken from the memory limit to account for allocator overhead.
constexpr double MEMORY_MARGIN_PERCENT = 10;

// Structure that records SGG memory usage.
struct SGGMemoryUsage {
    std::size_t sgg_edges_reserved_bytes;
    std::size_t sgg_reserved_bytes;
    std::size_t sgg_static_bytes;
};

// Structure that summarizes SGG memory usage.
struct SGGMemoryUsageSummary {
    std::size_t construction_min_bytes;
    std::size_t construction_max_bytes;
    std::size_t computation_min_bytes;
    std::size_t computation_max_bytes;
};

// Shared static estimation state accessed by all SGG memory-estimation jobs.
std::vector<SGGMemoryUsage> s_sgg_memory_usage;
std::atomic<std::size_t> s_sggs_constructed{};
Timer s_sgg_memory_timer;

// Constructs an SGG and returns the memory usage information.
SGGMemoryUsage calculate_sgg_memory_usage(const UnitigWeights& unitig_weights, const std::string& sgg_edges_filename) {
    const auto sgg_edges = SGGEdges(Memory::make_unique<FileReader>(sgg_edges_filename), unitig_weights);
    const auto sgg = SGG(sgg_edges);

    return { sgg_edges.reserved_bytes(), sgg.reserved_bytes(), sgg.static_bytes() };
}

// Summarize all calculated SGG memory usage information.
SGGMemoryUsageSummary calculate_total_sgg_memory_usage() {
    assert(!s_sgg_memory_usage.empty() && "SGG memory usage has not been computed");

    // Get the maximum static bytes used by any of the SGG jobs.
    const auto sgg_static_max_bytes = std::max_element(s_sgg_memory_usage.begin(),
                                                       s_sgg_memory_usage.end(),
                                                       [](const SGGMemoryUsage& lhs, const SGGMemoryUsage& rhs)
    {
        return lhs.sgg_static_bytes < rhs.sgg_static_bytes;
    })->sgg_static_bytes;

    std::size_t construction_min_bytes = std::numeric_limits<std::size_t>::max();
    std::size_t construction_max_bytes = 0;
    std::size_t computation_min_bytes = std::numeric_limits<std::size_t>::max();
    std::size_t computation_max_bytes = 0;

    for (const auto& memory_usage : s_sgg_memory_usage) {
        const auto computation_bytes = Utils::safe_add(memory_usage.sgg_reserved_bytes,
                                                       sgg_static_max_bytes,
                                                       "SGG computation memory");
        const auto construction_bytes = Utils::safe_add(computation_bytes,
                                                        memory_usage.sgg_edges_reserved_bytes,
                                                        "SGG construction memory");

        construction_min_bytes = std::min(construction_min_bytes, construction_bytes);
        construction_max_bytes = std::max(construction_max_bytes, construction_bytes);
        computation_min_bytes = std::min(computation_min_bytes, computation_bytes);
        computation_max_bytes = std::max(computation_max_bytes, computation_bytes);
    }

    return {
        construction_min_bytes,
        construction_max_bytes,
        computation_min_bytes,
        computation_max_bytes
    };
}

} // namespace

void DistanceQueryMemoryBudget::estimate_sgg_memory_usage(const UnitigWeights& unitig_weights,
                                                          const SGGEdgesFilenames& sgg_edges_filenames)
{
    assert(s_sgg_memory_usage.empty() && "SGG memory estimation already running");

    if (sgg_edges_filenames.size() == 0) {
        throw Exception(ErrorCode::EMPTY_DATA, "SGG edges filenames");
    }

    s_sgg_memory_timer.mark();
    s_sggs_constructed = 0;

    const auto n_sggs_to_construct =
        std::min(sgg_edges_filenames.size(),
                 std::max(MIN_SGGS_TO_ESTIMATE,
                          sgg_edges_filenames.size() * SGG_ESTIMATION_PERCENT / 100));

    Log::out() << "Constructing " << Format::pretty_uint(n_sggs_to_construct) << " SGGs to estimate memory usage."
               << std::endl;

    std::vector<JobT> jobs;
    jobs.reserve(n_sggs_to_construct);

    s_sgg_memory_usage.resize(n_sggs_to_construct);

    const auto sorted_filename_indices = sgg_edges_filenames.indices_by_descending_file_size();

    for (std::size_t index = 0; index < n_sggs_to_construct; ++index) {
        const auto& sgg_edges_filename = sgg_edges_filenames[sorted_filename_indices[index]];

        jobs.push_back([&unitig_weights,
                        &sgg_edges_filename,
                        n_sggs_to_construct,
                        index]()
        {
            s_sgg_memory_usage[index] = calculate_sgg_memory_usage(unitig_weights, sgg_edges_filename);

            if (++s_sggs_constructed == n_sggs_to_construct) {
                Log::out() << "Constructed " << Format::pretty_uint(n_sggs_to_construct)
                           << " SGGs to estimate memory usage in "
                           << Format::duration_to_string(s_sgg_memory_timer.last_interval_ms(true)) << "." << std::endl;
            }
        });
    }

    WorkerPool::post(jobs);
}

std::size_t DistanceQueryMemoryBudget::maximum_distance_matrix_rows(std::size_t max_memory_usage_bytes,
                                                                    std::size_t base_memory_bytes,
                                                                    std::size_t matrix_columns,
                                                                    std::size_t max_matrix_rows)
{
    if (matrix_columns == 0 || max_matrix_rows == 0) {
        throw Exception(ErrorCode::INVALID_ARGUMENT, "distance matrix dimensions must be non-zero");
    }

    assert(!s_sgg_memory_usage.empty() && "SGG memory estimation not started");

    // Wait in case SGG memory usage calculation is still ongoing.
    WorkerPool::wait();

    // Don't use the full memory limit to account for allocator overhead.
    max_memory_usage_bytes =
        static_cast<std::size_t>(static_cast<double>(max_memory_usage_bytes) * (1.00 - 0.01 * MEMORY_MARGIN_PERCENT));
    Log::out() << "Applied a " << MEMORY_MARGIN_PERCENT << "% safety margin to the memory limit." << std::endl;

    const auto sgg_memory_usage = calculate_total_sgg_memory_usage();
    Memory::clear_and_release_reserved_memory(s_sgg_memory_usage);

    // CALLER_THREAD reports n_workers() == 0, but then the thread itself is working.
    const auto n_workers = std::max<std::size_t>(WorkerPool::n_workers(), 1);

    /*
     * Allowance for unaccounted dynamic memory.
     * Include median distance calculation buffers in this allowance, although the storage is static.
    */
    const auto dynamic_memory_allowance_per_thread = Utils::safe_multiply(DYNAMIC_MEMORY_ALLOWANCE_PER_THREAD,
                                                                          n_workers,
                                                                          "dynamic memory allowance per thread");
    const auto static_distance_calculation_buffers =
        Utils::safe_multiply(Utils::safe_multiply(n_workers, matrix_columns),
                             sizeof(std::uint64_t),
                             "median distance calculation buffers");
    const auto dynamic_memory_allowance_bytes = Utils::safe_add(dynamic_memory_allowance_per_thread,
                                                                static_distance_calculation_buffers,
                                                                "dynamic memory allowance");

    if (base_memory_bytes >= max_memory_usage_bytes ||
        dynamic_memory_allowance_bytes >= max_memory_usage_bytes - base_memory_bytes)
    {
        throw Exception(ErrorCode::INVALID_ARGUMENT,
                        "base memory usage (", base_memory_bytes, ") + dynamic memory allowance (",
                        dynamic_memory_allowance_bytes,
                        ") must be smaller than maximum memory usage ", max_memory_usage_bytes);
    }

    const auto available_memory_bytes = max_memory_usage_bytes - base_memory_bytes - dynamic_memory_allowance_bytes;

    Log::out() << "Estimated base memory usage: "
               << Format::pretty_uint(Memory::bytes_to_mebibytes(base_memory_bytes)) << " MiB." << std::endl;

    Log::out() << "Dynamic memory allowance: "
               << Format::pretty_uint(Memory::bytes_to_mebibytes(dynamic_memory_allowance_bytes)) << " MiB."
               << std::endl;

    Log::out() << "Remaining memory budget: "
               << Format::pretty_uint(Memory::bytes_to_mebibytes(available_memory_bytes)) << " MiB." << std::endl;

    const auto sgg_peak_memory_bytes = std::max(sgg_memory_usage.construction_max_bytes,
                                                sgg_memory_usage.computation_max_bytes);

    const auto total_sgg_memory_bytes = Utils::safe_multiply(sgg_peak_memory_bytes,
                                                             n_workers,
                                                             "total SGG memory use");

    if (total_sgg_memory_bytes >= available_memory_bytes) {
        throw Exception(ErrorCode::INVALID_ARGUMENT,
                        "remaining memory budget (", available_memory_bytes,
                        ") must exceed total SGG memory usage (", total_sgg_memory_bytes, ")");
    }

    const auto distance_matrix_memory_budget_bytes = available_memory_bytes - total_sgg_memory_bytes;

    Log::out() << "Estimated SGG construction memory usage: "
               << Format::pretty_uint(Memory::bytes_to_mebibytes(sgg_memory_usage.construction_min_bytes)) << "-"
               << Format::pretty_uint(Memory::bytes_to_mebibytes(sgg_memory_usage.construction_max_bytes)) << " MiB."
               << std::endl;

    Log::out() << "Estimated SGG computation memory usage: "
               << Format::pretty_uint(Memory::bytes_to_mebibytes(sgg_memory_usage.computation_min_bytes)) << "-"
               << Format::pretty_uint(Memory::bytes_to_mebibytes(sgg_memory_usage.computation_max_bytes)) << " MiB."
               << std::endl;

    Log::out() << "Distance matrix memory budget: "
               << Format::pretty_uint(Memory::bytes_to_mebibytes(distance_matrix_memory_budget_bytes)) << " MiB."
               << std::endl;

    const auto distance_matrix_row_bytes = Utils::safe_multiply(matrix_columns,
                                                                sizeof(std::uint64_t),
                                                                "even one distance matrix row");

    const auto maximum_rows = distance_matrix_memory_budget_bytes / distance_matrix_row_bytes;

    return std::min(maximum_rows, max_matrix_rows);
}

} // namespace PANGWES
