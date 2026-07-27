/*
 * MedianDistanceQueryEngine.hpp - Computes distance statistics (median included) across single-genome graphs (SGGs).
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "common/utils/WorkerPool.hpp"
#include "unitig_distance/core/Distance.hpp"
#include "unitig_distance/core/DistanceQueryBatch.hpp"
#include "unitig_distance/core/DistanceQueryBatchRange.hpp"
#include "unitig_distance/core/UnitigWeights.hpp"
#include "unitig_distance/io/QueriesReader.hpp"
#include "unitig_distance/sgg/SGGEdgesFilenames.hpp"

namespace PANGWES {

/*
 * Computes shortest-path distance statistics, including the median, for query pairs across SGGs constructed from the
 * provided data.
 *
 * The constructor reads the queries into compact batches and estimates SGG memory usage to calculate the memory budget
 * for the distance matrix required for median calculation.
 *
 * Memory use scales strictly with both the number of queries (n) and the number of SGGs (s) to construct the
 * (n x s) distance matrix.
 *
 * Calling compute_distances() further divides the distance query batches into ranges and prepares the distance matrix.
 * Afterwards, for one range at a time, a distance-computation job is posted for each SGG, which processes all batches
 * in the range. Once all jobs have finished, the current distances will be processed into the main distances vector.
 *
 * Note: the matrix will be stored in column-major order. This is efficient for the distance-computation jobs, but
 *       inefficient for the post-processing jobs (inserting the results into the main distances vector).
*/
class MedianDistanceQueryEngine {
public:
    /*
     * Constructs a distance query engine to answer queries on SGGs constructed from the provided `unitig_weights` and
     * `sgg_edges_filenames`, which are stored by reference as members.
     *
     * Reads queries using `queries_reader` and compresses them into distance query batches, stored in
     * `m_distance_query_batches`, and prepares `m_distances` for the count of read queries.
     *
     * Estimates SGG memory usage and calculates the maximum number of distance-matrix rows that fit in the memory
     * budget.
     *
     * Throws if reading and processing the queries fails, or estimating the memory budget fails.
    */
    MedianDistanceQueryEngine(const UnitigWeights& unitig_weights,
                              const SGGEdgesFilenames& sgg_edges_filenames,
                              QueriesReader& queries_reader,
                              std::size_t max_memory_usage_bytes);

    MedianDistanceQueryEngine(const MedianDistanceQueryEngine&) = delete;
    MedianDistanceQueryEngine& operator=(const MedianDistanceQueryEngine&) = delete;
    MedianDistanceQueryEngine(MedianDistanceQueryEngine&&) = delete;
    MedianDistanceQueryEngine& operator=(MedianDistanceQueryEngine&&) = delete;

    // Computes and returns all distances using data stored by the constructor.
    std::vector<Distance> compute_distances();

private:
    const UnitigWeights& m_unitig_weights;
    const SGGEdgesFilenames& m_sgg_edges_filenames;
    std::vector<Distance> m_distances;
    std::vector<DistanceQueryBatch> m_distance_query_batches;
    std::size_t m_maximum_distance_matrix_rows;

    // Computes the base reserved memory of data stored in MedianDistanceQueryEngine after construction.
    std::size_t compute_base_reserved_memory() const noexcept;

    /*
     * Returns a job that constructs one SGG (corresponds to a matrix column) and computes distances for the given query
     * range (corresponds to matrix rows).
    */
    JobT prepare_sgg_computation_job(std::size_t sgg_edges_filename_index,
                                     const DistanceQueryBatchRange& query_range,
                                     std::vector<std::uint64_t>& distance_matrix);

    // Returns a job that stores distance statistics from the distance matrix into the main `m_distances` vector.
    JobT prepare_process_distances_job(const std::vector<std::uint64_t>& distance_matrix,
                                       std::size_t batch_index,
                                       std::size_t row_begin);
};

} // namespace PANGWES
