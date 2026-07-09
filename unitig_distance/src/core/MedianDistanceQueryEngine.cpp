/*
 * MedianDistanceQueryEngine.cpp - Computes distance statistics (median included) across single-genome graphs (SGGs).
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "common/io/FileReader.hpp"
#include "common/io/Log.hpp"
#include "common/utils/Exception.hpp"
#include "common/utils/memory.hpp"
#include "common/utils/Timer.hpp"
#include "common/utils/WorkerPool.hpp"
#include "unitig_distance/core/DistanceQueryBatch.hpp"
#include "unitig_distance/core/DistanceQueryBatchRange.hpp"
#include "unitig_distance/core/DistanceQueryMemoryBudget.hpp"
#include "unitig_distance/core/MedianDistanceQueryEngine.hpp"
#include "unitig_distance/sgg/SGG.hpp"
#include "unitig_distance/sgg/SGGEdges.hpp"
#include "unitig_distance/sgg/sgg_types.hpp"

namespace PANGWES {
namespace {

// Median calculation buffer.
thread_local std::vector<std::uint64_t> s_median_distance_buffer;

} // namespace

MedianDistanceQueryEngine::MedianDistanceQueryEngine(const UnitigWeights& unitig_weights,
                                                     const SGGEdgesFilenames& sgg_edges_filenames,
                                                     QueriesReader& queries_reader,
                                                     std::size_t max_memory_usage_bytes)
    : m_unitig_weights{unitig_weights},
      m_sgg_edges_filenames{sgg_edges_filenames},
      m_distances{},
      m_distance_query_batches{},
      m_maximum_distance_matrix_rows{}
{
    try {
        DistanceQueryMemoryBudget::estimate_sgg_memory_usage(m_unitig_weights, m_sgg_edges_filenames);

        // Compress queries into compact batches.
        m_distance_query_batches = DistanceQueryBatch::compute_distance_query_batches(queries_reader);

        if (m_distance_query_batches.empty()) {
            throw Exception(ErrorCode::EMPTY_DATA, "distance query batches");
        }

        Log::out() << "Computed " << Format::pretty_uint(m_distance_query_batches.size())
                   << " distance query batches from " << Format::pretty_uint(queries_reader.n_queries_read())
                   << " queries." << std::endl;

        // Prepare distances vector to be able to hold distances for all the queries.
        m_distances.assign(queries_reader.n_queries_read(), Distance{});

        Log::out() << "Allocated result storage for " << Format::pretty_uint(queries_reader.n_queries_read())
                   << " queries." << std::endl;

        // This function waits for SGG memory estimation to finish.
        m_maximum_distance_matrix_rows =
            DistanceQueryMemoryBudget::maximum_distance_matrix_rows(max_memory_usage_bytes,
                                                                    compute_base_reserved_memory(),
                                                                    m_sgg_edges_filenames.size(),
                                                                    m_distances.size());
    } catch (...) {
        /*
         * Worker pool jobs (for estimating SGG memory usage) refer to unitig weights and sgg edges filenames that are
         * also being destroyed. Stop and join the workers to prevent use-after-free.
        */
        WorkerPool::stop();
        throw;
    }
}

std::vector<Distance> MedianDistanceQueryEngine::compute_distances() {
    assert(!m_distance_query_batches.empty() && "distance query batches not computed");
    assert(!m_distances.empty() && "distances vector not initialized");
    assert(m_maximum_distance_matrix_rows > 0 && "maximum distance rows not calculated");

    Timer timer;

    const bool is_async = WorkerPool::is_async();
    const auto n_sggs = m_sgg_edges_filenames.size();

    /*
     * Divide the distance query batches into ranges where each range has at most `m_maximum_distance_matrix_rows`
     * queries. The number of queries in the largest range is minimized while preserving the range count.
    */
    const auto distance_query_batch_ranges =
        compute_optimal_distance_query_batch_ranges(m_distance_query_batches, m_maximum_distance_matrix_rows);

    const auto matrix_rows = maximum_range_n_queries(distance_query_batch_ranges);

    Log::out() << "Grouped the query batches into " << Format::pretty_uint(distance_query_batch_ranges.size())
               << " ranges of at most " << Format::pretty_uint(matrix_rows)
               << " queries each." << std::endl;

    std::vector<std::uint64_t> distance_matrix(matrix_rows * n_sggs, INF_DISTANCE);

    Log::out() << "Allocated a (" << matrix_rows << " x " << n_sggs << ") distance matrix." << std::endl;

    for (std::size_t range_index = 0; range_index < distance_query_batch_ranges.size(); ++range_index) {
        const auto& query_range = distance_query_batch_ranges[range_index];

        const auto n_queries = query_range.n_queries;
        const auto begin_index_inclusive = query_range.begin_index_inclusive;
        const auto end_index_exclusive = query_range.end_index_exclusive;

        // Clear the distance matrix for this range.
        distance_matrix.assign(n_queries * n_sggs, INF_DISTANCE);

        // Prepare computation jobs.
        std::vector<JobT> compute_jobs;
        compute_jobs.reserve(n_sggs);

        for (std::size_t sgg_index = 0; sgg_index < n_sggs; ++sgg_index) {
            compute_jobs.push_back(prepare_sgg_computation_job(sgg_index, query_range, distance_matrix));
        }

        // Prepare post-processing jobs.
        std::vector<JobT> post_process_jobs;
        post_process_jobs.reserve(end_index_exclusive - begin_index_inclusive);

        std::size_t row_begin = 0;

        for (std::size_t batch_index = begin_index_inclusive; batch_index < end_index_exclusive; ++batch_index) {
            const auto& targets = m_distance_query_batches[batch_index].targets();

            post_process_jobs.push_back(prepare_process_distances_job(distance_matrix, batch_index, row_begin));

            row_begin += targets.size();
        }

        assert(row_begin == n_queries && "distance matrix rows are not aligned with queries");

        // Only log about posting jobs in async mode as caller thread mode simply begins executing the jobs immediately.
        if (is_async) {
            Log::out() << "Posting " << Format::pretty_uint(compute_jobs.size()) << " SGG distance computation jobs."
                       << std::endl;
        }

        // Post computation jobs to the worker pool.
        WorkerPool::post(compute_jobs);
        WorkerPool::wait();

        Log::out() << "Computed a total of " << Format::pretty_uint(n_queries * n_sggs)
                   << " SGG distances. Elapsed time: "
                   << Format::duration_to_string(timer.total_ms(true)) << "." << std::endl;

        if (is_async) {
            Log::out() << "Posting " << Format::pretty_uint(post_process_jobs.size())
                       << " SGG distance post-processing jobs." << std::endl;
        }
        // Post post-processing jobs to the worker pool.
        WorkerPool::post(post_process_jobs);
        WorkerPool::wait();

        Log::out() << "Computed results for " << Format::pretty_uint(n_queries)
                   << " distance queries (batch range "
                   << Format::pretty_uint(range_index + 1) << "/"
                   << Format::pretty_uint(distance_query_batch_ranges.size()) << "). Elapsed time: "
                   << Format::duration_to_string(timer.total_ms(true)) << "." << std::endl;
    }

    return std::move(m_distances);
}

std::size_t MedianDistanceQueryEngine::compute_base_reserved_memory() const noexcept {
    return sizeof(*this) +
           Memory::container_reserved_bytes(m_unitig_weights) +
           Memory::container_reserved_bytes(m_sgg_edges_filenames) +
           Memory::container_reserved_bytes(m_distances) +
           Memory::container_reserved_bytes(m_distance_query_batches);
}

JobT MedianDistanceQueryEngine::prepare_sgg_computation_job(std::size_t sgg_edges_filename_index,
                                                            const DistanceQueryBatchRange& query_range,
                                                            std::vector<std::uint64_t>& distance_matrix)
{
    return [this, sgg_edges_filename_index, &query_range, &distance_matrix]()
    {
        const auto sgg_edges_filename = m_sgg_edges_filenames[sgg_edges_filename_index];
        const auto sgg = SGG(SGGEdges(Memory::make_unique<FileReader>(sgg_edges_filename), m_unitig_weights));

        const auto column_start = sgg_edges_filename_index * query_range.n_queries;
        std::size_t row_index = 0;

        for (std::size_t batch_index = query_range.begin_index_inclusive;
             batch_index < query_range.end_index_exclusive;
             ++batch_index)
        {
            const auto& distance_query_batch = m_distance_query_batches[batch_index];
            const auto& targets = distance_query_batch.targets();
            auto sgg_distances = sgg.compute_unitig_distances(distance_query_batch.source_unitig_id(), targets);

            if (!sgg_distances.empty()) {
                if (sgg_distances.size() != targets.size()) {
                    throw Exception(ErrorCode::INVALID_STATE, "distances and targets must be aligned");
                }

                std::copy(sgg_distances.begin(),
                          sgg_distances.end(),
                          distance_matrix.data() + column_start + row_index);
            }

            row_index += targets.size();
        }
    };
}

JobT MedianDistanceQueryEngine::prepare_process_distances_job(const std::vector<std::uint64_t>& distance_matrix,
                                                              std::size_t batch_index,
                                                              std::size_t row_begin)
{
    return [this, &distance_matrix, batch_index, row_begin]()
    {
        const auto& targets = m_distance_query_batches[batch_index].targets();
        const auto n_sggs = m_sgg_edges_filenames.size();
        const auto matrix_rows = distance_matrix.size() / n_sggs;
        auto row_index = row_begin;

        for (const auto& target : targets) {
            Distance distance{};
            s_median_distance_buffer.clear();

            for (std::size_t sgg_index = 0; sgg_index < n_sggs; ++sgg_index) {
                const auto sgg_distance = distance_matrix[sgg_index * matrix_rows + row_index];

                if (sgg_distance != INF_DISTANCE) {
                    distance.add_distance(sgg_distance);
                    s_median_distance_buffer.push_back(sgg_distance);
                }
            }

            if (!s_median_distance_buffer.empty()) {
                const auto lower_median_index = (s_median_distance_buffer.size() - 1) / 2;
                auto* lower_median = s_median_distance_buffer.data() + lower_median_index;

                std::nth_element(s_median_distance_buffer.data(),
                                 lower_median,
                                 s_median_distance_buffer.data() + s_median_distance_buffer.size());

                distance.set_median_distance(*lower_median);
            }

            assert(target.query_index < m_distances.size() && "distance query index out of bounds");
            m_distances[target.query_index] = std::move(distance);
            ++row_index;
        }
    };
}

} // namespace PANGWES
