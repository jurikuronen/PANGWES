/*
 * DistanceQueryBatchRange.hpp - Partitioning of distance query batches into contiguous index ranges.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cstddef>
#include <vector>

#include "unitig_distance/core/DistanceQueryBatch.hpp"

namespace PANGWES {

/*
 * Stores index ranges (begin-inclusive, end-exclusive) to a container of distance query batches.
 *
 * Includes the number of queries contained in the batches of the range.
*/
struct DistanceQueryBatchRange {
    std::size_t begin_index_inclusive;
    std::size_t end_index_exclusive;
    std::size_t n_queries;
};

// Returns the number of queries in the largest range, or zero if the ranges are empty.
std::size_t maximum_range_n_queries(const std::vector<DistanceQueryBatchRange>& distance_query_batch_ranges) noexcept;

/*
 * Computes distance query batch ranges where each range contains at most `maximum_n_queries_per_range` queries.
 *
 * Throws INVALID_ARGUMENT if the maximum is zero or a single distance query batch exceeds it.
*/
std::vector<DistanceQueryBatchRange>
compute_distance_query_batch_ranges(const std::vector<DistanceQueryBatch>& distance_query_batches,
                                    std::size_t maximum_n_queries_per_range);

/*
 * Binary searches for an optimal `n_queries_per_range` and returns a list of distance query batch ranges where:
 * - The number of ranges is the same as with `maximum_n_queries_per_range`.
 * - The largest range is as small as possible.
 *
 * This is to reduce the memory required to contain the results of the largest range.
 *
 * Throws INVALID_ARGUMENT if the maximum is zero or a single distance query batch exceeds it.
*/
std::vector<DistanceQueryBatchRange>
compute_optimal_distance_query_batch_ranges(const std::vector<DistanceQueryBatch>& distance_query_batches,
                                            std::size_t maximum_n_queries_per_range);

} // namespace PANGWES
