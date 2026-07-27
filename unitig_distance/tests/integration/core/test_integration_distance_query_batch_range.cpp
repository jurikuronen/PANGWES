/*
 * test_integration_distance_query_batch_range.cpp - Integration tests for core/DistanceQueryBatchRange.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <algorithm>
#include <cstddef>
#include <limits>
#include <vector>

#include "common/io/FileReader.hpp"
#include "common/test_harness/Test.hpp"
#include "common/utils/memory.hpp"
#include "unitig_distance/core/DistanceQueryBatch.hpp"
#include "unitig_distance/core/DistanceQueryBatchRange.hpp"
#include "unitig_distance/io/QueriesReader.hpp"

namespace PANGWES {
namespace {

constexpr auto queries_file = TEST_DATA_DIR "/test_efc_k31.queries";
constexpr auto expected_queries_size = 5000;

bool test_integration_distance_query_batch_ranges_for_all_maximum_n_queries() {
    auto queries_reader = QueriesReader(Memory::make_unique<FileReader>(queries_file));
    const auto distance_query_batches = DistanceQueryBatch::compute_distance_query_batches(queries_reader);

    std::size_t n_queries = 0;
    for (const auto& distance_query_batch : distance_query_batches) {
        n_queries += distance_query_batch.targets().size();
    }
    ASSERT_EQUAL(n_queries, expected_queries_size);

    const auto smallest_valid_maximum_n_queries = maximum_batch_n_queries(distance_query_batches);
    ASSERT_GREATER(smallest_valid_maximum_n_queries, 0);

    // Index `n_ranges` gives the smallest `maximum_n_queries_per_range` producing `n_ranges` ranges.
    std::vector<std::size_t> optimal_n_queries(distance_query_batches.size() + 1,
                                               std::numeric_limits<std::size_t>::max());

    /*
     * Test every valid `maximum_n_queries_per_range` value.
     *
     * Optimal (smallest) values to produce a specified number of ranges are recorded.
    */
    for (std::size_t maximum_n_queries_per_range = smallest_valid_maximum_n_queries;
         maximum_n_queries_per_range <= n_queries;
         ++maximum_n_queries_per_range)
    {
        const auto ranges = compute_distance_query_batch_ranges(distance_query_batches, maximum_n_queries_per_range);
        ASSERT_FALSE(ranges.empty());

        const auto n_ranges = ranges.size();
        optimal_n_queries[n_ranges] = std::min(optimal_n_queries[n_ranges], maximum_n_queries_per_range);

        const auto optimal_ranges = compute_optimal_distance_query_batch_ranges(distance_query_batches,
                                                                                maximum_n_queries_per_range);
        const auto expected_optimal_ranges = compute_distance_query_batch_ranges(distance_query_batches,
                                                                                 optimal_n_queries[n_ranges]);

        ASSERT_EQUAL(optimal_ranges.size(), ranges.size());
        ASSERT_EQUAL(optimal_ranges.size(), expected_optimal_ranges.size());

        for (std::size_t range_index = 0; range_index < optimal_ranges.size(); ++range_index) {
            const auto& optimal_range = optimal_ranges[range_index];
            const auto& expected_optimal_range = expected_optimal_ranges[range_index];

            ASSERT_EQUAL(optimal_range.begin_index_inclusive, expected_optimal_range.begin_index_inclusive);
            ASSERT_EQUAL(optimal_range.end_index_exclusive, expected_optimal_range.end_index_exclusive);
            ASSERT_EQUAL(optimal_range.n_queries, expected_optimal_range.n_queries);
        }
    }

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_integration_distance_query_batch_ranges_for_all_maximum_n_queries),
    };

    return Test::run_suite("test_integration_distance_query_batch_range", tests);
}
