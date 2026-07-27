/*
 * MeanDistanceQueryEngine.cpp - Computes distance statistics (median excluded) across single-genome graphs (SGGs).
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <mutex>
#include <utility>
#include <vector>

#include "common/io/FileReader.hpp"
#include "common/io/Log.hpp"
#include "common/utils/Exception.hpp"
#include "common/utils/memory.hpp"
#include "common/utils/ProgressLogger.hpp"
#include "common/utils/WorkerPool.hpp"
#include "unitig_distance/core/DistanceQueryBatch.hpp"
#include "unitig_distance/core/MeanDistanceQueryEngine.hpp"
#include "unitig_distance/sgg/SGG.hpp"
#include "unitig_distance/sgg/SGGEdges.hpp"
#include "unitig_distance/sgg/SGGEdgesFilenames.hpp"
#include "unitig_distance/sgg/sgg_types.hpp"

namespace PANGWES {
namespace {

// Provided for ProgressLogger to log distance-query-processing progress together with a value-based threshold.
constexpr std::uint64_t LOGGING_INTERVAL_TIME_MS = 10000;

// Shared static progress state for ProgressLogger; accessed by all SGG worker jobs.
std::uint64_t s_sgg_count{};
std::uint64_t s_n_sggs{};
ProgressLogger s_progress_logger{};
std::mutex s_progress_logger_mutex{};

// Callback for ProgressLogger.
void progress_logger_callback(std::uint64_t sgg_count, std::uint64_t elapsed_time_ms) {
    assert(s_n_sggs > 0 && "s_n_sggs not set");

    Log::out() << "Computed graph distances for " << Format::pretty_uint(sgg_count) << "/"
               << Format::pretty_uint(s_n_sggs) << " SGGs. Elapsed time: "
               << Format::duration_to_string(elapsed_time_ms) << "." << std::endl;
}

} // namespace

MeanDistanceQueryEngine::MeanDistanceQueryEngine(const UnitigWeights& unitig_weights,
                                                 const SGGEdgesFilenames& sgg_edges_filenames,
                                                 QueriesReader& queries_reader)
    : m_unitig_weights{unitig_weights},
      m_sgg_edges_filenames{sgg_edges_filenames},
      m_distance_query_batches{},
      m_distance_query_batch_mutexes{},
      m_n_queries_read{}
{
    // Compress queries into compact batches.
    m_distance_query_batches = DistanceQueryBatch::compute_distance_query_batches(queries_reader);

    if (m_distance_query_batches.empty()) {
        throw Exception(ErrorCode::EMPTY_DATA, "distance query batches");
    }

    Log::out() << "Computed " << Format::pretty_uint(m_distance_query_batches.size())
               << " distance query batches from " << Format::pretty_uint(queries_reader.n_queries_read())
               << " queries." << std::endl;

    if (WorkerPool::is_async()) {
        m_distance_query_batch_mutexes = std::vector<std::mutex>(m_distance_query_batches.size());
    }

    m_n_queries_read = queries_reader.n_queries_read();

    // Prepare static progress state for progress logger.
    s_sgg_count = 0;
    s_n_sggs = sgg_edges_filenames.size();
}

std::vector<Distance> MeanDistanceQueryEngine::compute_distances() {
    assert(!m_distance_query_batches.empty() && "distance query batches not computed");

    if (WorkerPool::stopped()) {
        throw Exception(ErrorCode::OPERATION_FOR_STOPPED_WORKER_POOL, "MeanDistanceQueryEngine::compute_distances()");
    }

    // Prepare the progress logger; log after every max(50, 5%) SGGs processed.
    const std::size_t logging_interval_sgg_count = std::min(s_n_sggs, std::max<std::size_t>(50, s_n_sggs / 20));
    s_progress_logger = ProgressLogger(progress_logger_callback, logging_interval_sgg_count, LOGGING_INTERVAL_TIME_MS);

    std::vector<Distance> distances(m_n_queries_read);

    Log::out() << "Allocated result storage for " << Format::pretty_uint(m_n_queries_read) << " queries." << std::endl;

    const bool is_async = WorkerPool::is_async();

    std::vector<JobT> jobs;
    jobs.reserve(m_sgg_edges_filenames.size());

    for (std::size_t filename_index = 0; filename_index < m_sgg_edges_filenames.size(); ++filename_index) {
        jobs.push_back(prepare_sgg_computation_job(distances, filename_index, is_async));
    }

    // Only log this in async mode as caller thread mode simply begins executing the jobs immediately.
    if (is_async) {
        Log::out() << "Posting " << Format::pretty_uint(jobs.size()) << " SGG distance computation jobs." << std::endl;
    }

    WorkerPool::post(jobs);
    WorkerPool::wait();

    s_progress_logger.log(s_sgg_count);

    return distances;
}

JobT MeanDistanceQueryEngine::prepare_sgg_computation_job(std::vector<Distance>& distances,
                                                          std::size_t sgg_edges_filename_index,
                                                          bool is_async) {
    return [this, &distances, sgg_edges_filename_index, is_async]()
    {
        const auto sgg_edges_filename = m_sgg_edges_filenames[sgg_edges_filename_index];
        const auto sgg = SGG(SGGEdges(Memory::make_unique<FileReader>(sgg_edges_filename), m_unitig_weights));

        for (std::size_t batch_index = 0; batch_index < m_distance_query_batches.size(); ++batch_index) {
            const auto& distance_query_batch = m_distance_query_batches[batch_index];
            const auto source_unitig_id = distance_query_batch.source_unitig_id();
            const auto& targets = distance_query_batch.targets();

            auto sgg_distances = sgg.compute_unitig_distances(source_unitig_id, targets);

            if (!sgg_distances.empty()) {
                if (sgg_distances.size() != targets.size()) {
                    throw Exception(ErrorCode::INVALID_STATE, "distances and targets must be aligned");
                }

                std::unique_lock<std::mutex> lock;

                if (is_async) {
                    assert(batch_index < m_distance_query_batch_mutexes.size() && "query batch mutex missing");
                    lock = std::unique_lock<std::mutex>(m_distance_query_batch_mutexes[batch_index]);
                }

                for (std::size_t i = 0; i < targets.size(); ++i) {
                    if (sgg_distances[i] != INF_DISTANCE) {
                        distances[targets[i].query_index].add_distance(sgg_distances[i]);
                    }
                }
            }
        }

        // Processing this SGG is done, signal progress logger to log if needed. 
        std::lock_guard<std::mutex> lock(s_progress_logger_mutex);
        s_progress_logger.log_if_due(++s_sgg_count);
    };
}

} // namespace PANGWES
