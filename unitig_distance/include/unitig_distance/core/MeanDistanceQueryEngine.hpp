/*
 * MeanDistanceQueryEngine.hpp - Computes distance statistics (median excluded) across single-genome graphs (SGGs).
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>

#include "common/utils/WorkerPool.hpp"
#include "unitig_distance/core/Distance.hpp"
#include "unitig_distance/core/DistanceQueryBatch.hpp"
#include "unitig_distance/core/UnitigWeights.hpp"
#include "unitig_distance/io/QueriesReader.hpp"
#include "unitig_distance/sgg/SGGEdgesFilenames.hpp"

namespace PANGWES {

/*
 * Computes shortest-path distance statistics for query pairs across the SGGs constructed from the provided data.
 *
 * Does not calculate median shortest-path distances.
 *
 * The queries are first compacted into batches during construction. Calling compute_distances() then posts a
 * distance-computation job for each SGG, which processes every batch.
 *
 * Memory use scales mainly with the number of queries and worker threads, since each worker reserves memory for the SGG
 * it processes and the computed results are added directly to the main distances vector. The total memory requirement
 * is far lower than that of MedianDistanceQueryEngine, so running out of memory is not expected.
 * DistanceQueryMemoryBudget could estimate this memory use, but budgeting it is unnecessary.
*/
class MeanDistanceQueryEngine {
public:
    /*
     * Constructs a distance query engine to answer queries on SGGs constructed from the provided `unitig_weights` and
     * `sgg_edges_filenames`, which are stored by reference as members.
     *
     * Reads queries using `queries_reader` and compresses them into distance query batches, stored in
     * `m_distance_query_batches`.
     *
     * Throws if reading and processing the queries fails.
    */
    MeanDistanceQueryEngine(const UnitigWeights& unitig_weights,
                            const SGGEdgesFilenames& sgg_edges_filenames,
                            QueriesReader& queries_reader);

    MeanDistanceQueryEngine(const MeanDistanceQueryEngine&) = delete;
    MeanDistanceQueryEngine& operator=(const MeanDistanceQueryEngine&) = delete;
    MeanDistanceQueryEngine(MeanDistanceQueryEngine&&) = delete;
    MeanDistanceQueryEngine& operator=(MeanDistanceQueryEngine&&) = delete;

    // Computes and returns all distances using data stored by the constructor.
    std::vector<Distance> compute_distances();

private:
    const UnitigWeights& m_unitig_weights;
    const SGGEdgesFilenames& m_sgg_edges_filenames;
    std::vector<DistanceQueryBatch> m_distance_query_batches;
    std::vector<std::mutex> m_distance_query_batch_mutexes;
    std::size_t m_n_queries_read;

    // Returns a job that constructs one SGG and computes every distance query batch against it.
    JobT prepare_sgg_computation_job(std::vector<Distance>& distances,
                                     std::size_t sgg_edges_filename_index,
                                     bool is_async);
};

} // namespace PANGWES
