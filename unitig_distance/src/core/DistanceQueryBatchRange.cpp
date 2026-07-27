/*
 * DistanceQueryBatchRange.cpp - Partitioning of distance query batches into contiguous index ranges.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <algorithm>
#include <cstddef>

#include "common/utils/Exception.hpp"
#include "unitig_distance/core/DistanceQueryBatchRange.hpp"

namespace PANGWES {

std::size_t maximum_range_n_queries(const std::vector<DistanceQueryBatchRange>& distance_query_batch_ranges) noexcept {
    if (distance_query_batch_ranges.empty()) {
        return 0;
    }

    return std::max_element(distance_query_batch_ranges.begin(),
                            distance_query_batch_ranges.end(),
                            [](const DistanceQueryBatchRange& lhs, const DistanceQueryBatchRange& rhs)
    {
        return lhs.n_queries < rhs.n_queries;
    })->n_queries;
}

std::vector<DistanceQueryBatchRange>
compute_distance_query_batch_ranges(const std::vector<DistanceQueryBatch>& distance_query_batches,
                                    std::size_t maximum_n_queries_per_range)
{
    if (maximum_n_queries_per_range == 0) {
        throw Exception(ErrorCode::INVALID_ARGUMENT, "maximum n_queries per range must be non-zero");
    }

    std::vector<DistanceQueryBatchRange> ranges;

    std::size_t begin_index = 0;
    std::size_t current_index = 0;
    std::size_t current_n_queries = 0;

    for (const auto& distance_query_batch : distance_query_batches) {
        const auto batch_n_queries = distance_query_batch.targets().size();

        if (batch_n_queries > maximum_n_queries_per_range) {
            throw Exception(ErrorCode::INVALID_ARGUMENT,
                            "distance query batch size exceeds maximum allowed queries per range");
        }

        // Start a new range if adding this batch would exceed the maximum.
        if (current_n_queries > maximum_n_queries_per_range - batch_n_queries) {
            ranges.push_back({begin_index, current_index, current_n_queries});

            begin_index = current_index;
            current_n_queries = 0;
        }

        current_n_queries += batch_n_queries;
        ++current_index;
    }

    const auto batches_size = distance_query_batches.size();

    // Add the last range.
    if (begin_index != batches_size) {
        ranges.push_back({begin_index, batches_size, current_n_queries});
    }

    return ranges;
}

std::vector<DistanceQueryBatchRange>
compute_optimal_distance_query_batch_ranges(const std::vector<DistanceQueryBatch>& distance_query_batches,
                                            std::size_t maximum_n_queries_per_range)
{
    auto maximum_n_queries_ranges = compute_distance_query_batch_ranges(distance_query_batches,
                                                                        maximum_n_queries_per_range);

    if (maximum_n_queries_ranges.empty()) {
        return maximum_n_queries_ranges;
    }

    // Binary search for the smallest n_queries_per_range for which the n_ranges is optimal.
    const auto optimal_n_ranges = maximum_n_queries_ranges.size();
    std::size_t n_queries_low = maximum_batch_n_queries(distance_query_batches);
    std::size_t n_queries_high = maximum_n_queries_per_range;

    while (n_queries_low < n_queries_high) {
        const auto n_queries_per_range = n_queries_low + (n_queries_high - n_queries_low) / 2;
        const auto candidate_ranges = compute_distance_query_batch_ranges(distance_query_batches, n_queries_per_range);

        if (candidate_ranges.size() == optimal_n_ranges) {
            n_queries_high = n_queries_per_range;
        } else {
            n_queries_low = n_queries_per_range + 1;
        }
    }

    return compute_distance_query_batch_ranges(distance_query_batches, n_queries_low);
}

} // namespace PANGWES
