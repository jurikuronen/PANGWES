/*
 * test_unit_distance_query_batch_range.cpp - Unit tests for core/DistanceQueryBatchRange.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <cstddef>
#include <vector>

#include "common/test_harness/Test.hpp"
#include "common/utils/Exception.hpp"
#include "common/utils/memory.hpp"
#include "mocks/MockFileReader.hpp"
#include "mocks/MockQueriesData.hpp"
#include "unitig_distance/core/DistanceQueryBatch.hpp"
#include "unitig_distance/core/DistanceQueryBatchRange.hpp"
#include "unitig_distance/io/QueriesReader.hpp"

namespace PANGWES {
namespace {

using queries_data_t = std::vector<Mocks::MockQueryData>;

std::vector<DistanceQueryBatch>
test_create_distance_query_batches(const queries_data_t& query_data)
{
    Mocks::MockIfStream::set_contents(Mocks::MockQueriesData(query_data).to_contents());

    auto queries_reader = QueriesReader(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file));

    return DistanceQueryBatch::compute_distance_query_batches(queries_reader);
}

bool check_batch_range_size(const DistanceQueryBatchRange& distance_query_batch_range,
                            std::size_t expected_begin_index_inclusive,
                            std::size_t expected_end_index_exclusive,
                            std::size_t expected_n_queries)
{
    const auto begin_index_inclusive = distance_query_batch_range.begin_index_inclusive;
    const auto end_index_exclusive = distance_query_batch_range.end_index_exclusive;
    const auto n_queries = distance_query_batch_range.n_queries;

    ASSERT_EQUAL(begin_index_inclusive, expected_begin_index_inclusive);
    ASSERT_EQUAL(end_index_exclusive, expected_end_index_exclusive);
    ASSERT_EQUAL(n_queries, expected_n_queries);

    return true;
}

bool test_unit_distance_query_batch_range_empty_batches() {
    const auto distance_query_batch_ranges = compute_distance_query_batch_ranges({}, 10);
    const auto optimal_ranges = compute_optimal_distance_query_batch_ranges({}, 10);

    ASSERT_TRUE(distance_query_batch_ranges.empty());
    ASSERT_TRUE(optimal_ranges.empty());

    return true;
}

bool test_unit_distance_query_batch_range_bad_maximum_n_queries() {
    EXPECT_THROW(compute_distance_query_batch_ranges({}, 0), ErrorCode::INVALID_ARGUMENT);
    EXPECT_THROW(compute_optimal_distance_query_batch_ranges({}, 0), ErrorCode::INVALID_ARGUMENT);

    return true;
}

bool test_unit_distance_query_batch_range_every_batch_of_maximum_size() {
    const auto distance_query_batches = test_create_distance_query_batches(queries_data_t{
          {0, 1},   {0, 2},   {0, 3}, {10, 11}, {10, 12}, {10, 13},
        {20, 21}, {20, 22}, {20, 23}, {30, 31}, {30, 32}, {30, 33},
    });
    const auto maximum_n_queries_per_range = 3;
    const auto distance_query_batch_ranges = compute_distance_query_batch_ranges(distance_query_batches,
                                                                                 maximum_n_queries_per_range);

    ASSERT_FALSE(distance_query_batch_ranges.empty());
    ASSERT_EQUAL(distance_query_batch_ranges.size(), 4);

    // Each range should contain a single batch of the exact maximum size.
    for (std::size_t i = 0; i < distance_query_batch_ranges.size(); ++i) {
        if (!check_batch_range_size(distance_query_batch_ranges[i], i, i + 1, maximum_n_queries_per_range)) {
            return false;
        }
    }

    return true;
}

bool test_unit_distance_query_batch_range_too_low_maximum_n_queries_throws() {
    const auto distance_query_batches = test_create_distance_query_batches(queries_data_t{
          {0, 1}, {0, 2}, {5, 6}, {5, 7}, {5, 8}, {10, 11}, {10, 12},
    });
    const auto maximum_n_queries_per_range = 2;

    EXPECT_THROW(compute_distance_query_batch_ranges(distance_query_batches, maximum_n_queries_per_range),
                 ErrorCode::INVALID_ARGUMENT);
    EXPECT_THROW(compute_optimal_distance_query_batch_ranges(distance_query_batches, maximum_n_queries_per_range),
                 ErrorCode::INVALID_ARGUMENT);

    return true;
}

bool test_unit_distance_query_batch_range_one_batch_is_smaller() {
    // The middle batch undershoots.
    const auto distance_query_batches = test_create_distance_query_batches(queries_data_t{
          {0, 1}, {0, 2}, {5, 6}, {10, 11}, {10, 12},
    });
    const auto maximum_n_queries_per_range = 2;
    const auto distance_query_batch_ranges = compute_distance_query_batch_ranges(distance_query_batches,
                                                                                 maximum_n_queries_per_range);

    ASSERT_FALSE(distance_query_batch_ranges.empty());
    // The distance query batches are sorted, so the smaller batch gets added last (as the last batch after the loop).
    ASSERT_EQUAL(distance_query_batch_ranges.size(), 3);

    // The first two ranges should contain a single batch of the exact maximum size.
    for (std::size_t i = 0; i < 2; ++i) {
        if (!check_batch_range_size(distance_query_batch_ranges[i], i, i + 1, maximum_n_queries_per_range)) {
            return false;
        }
    }

    // The last range should undershoot the maximum by 1.
    return check_batch_range_size(distance_query_batch_ranges.back(), 2, 3, maximum_n_queries_per_range - 1);
}

bool test_unit_distance_query_batch_range_exact_maximum_results_in_a_single_batch() {
    const auto distance_query_batches = test_create_distance_query_batches(queries_data_t{
        {0, 1}, {0, 2}, {10, 11}, {10, 12}, {10, 13},
    });
    const auto maximum_n_queries_per_range = 5;
    const auto distance_query_batch_ranges = compute_distance_query_batch_ranges(distance_query_batches,
                                                                                 maximum_n_queries_per_range);

    ASSERT_EQUAL(distance_query_batch_ranges.size(), 1);

    return check_batch_range_size(distance_query_batch_ranges.front(), 0, 2, maximum_n_queries_per_range);
}

bool test_unit_distance_query_batch_range_no_batch_exceeds_maximum_size() {
    const auto distance_query_batches = test_create_distance_query_batches(queries_data_t{
          {0, 1},   {0, 2}, {10, 11}, {10, 12}, {20, 21}, {20, 22},
        {30, 31}, {30, 32}, {40, 41}, {40, 42}, {50, 51}, {50, 52},
    });
    const auto maximum_n_queries_per_range = 5;
    const auto distance_query_batch_ranges = compute_distance_query_batch_ranges(distance_query_batches,
                                                                                 maximum_n_queries_per_range);

    ASSERT_FALSE(distance_query_batch_ranges.empty());
    // A third two-query batch would exceed the maximum, so each range should contain two batches.
    ASSERT_EQUAL(distance_query_batch_ranges.size(), 3);

    const auto expected_n_queries_per_range = 4;
    for (std::size_t i = 0; i < distance_query_batch_ranges.size(); ++i) {
        const auto expected_begin_index_inclusive = 2 * i;
        const auto expected_end_index_exclusive = 2 * (i + 1);

        if (!check_batch_range_size(distance_query_batch_ranges[i],
                                    expected_begin_index_inclusive,
                                    expected_end_index_exclusive,
                                    expected_n_queries_per_range))
        {
            return false;
        }
    }

    return true;
}

bool test_unit_distance_query_batch_range_maximum_larger_than_total_results_in_a_single_batch() {
    const auto distance_query_batches = test_create_distance_query_batches(queries_data_t{
          {0, 1}, {0, 2}, {0, 3}, {4, 5}, {4, 6}, {7, 8},
    });
    const auto maximum_n_queries_per_range = 100;
    const auto distance_query_batch_ranges = compute_distance_query_batch_ranges(distance_query_batches,
                                                                                 maximum_n_queries_per_range);

    ASSERT_FALSE(distance_query_batch_ranges.empty());
    // We should have one range with all three batches (total 6 queries).
    ASSERT_EQUAL(distance_query_batch_ranges.size(), 1);

    return check_batch_range_size(distance_query_batch_ranges.front(), 0, 3, 6);
}

bool test_unit_distance_query_batch_range_optimal_ranges_preserve_range_count() {
    const auto distance_query_batches = test_create_distance_query_batches(queries_data_t{
         {0, 1},  {0, 2}, {10, 11}, {10, 12}, {20, 21}, {20, 22}, {30, 31}, {30, 32},
    });
    const auto maximum_n_queries_per_range = 6;

    const auto maximum_n_queries_ranges = compute_distance_query_batch_ranges(distance_query_batches,
                                                                              maximum_n_queries_per_range);
    const auto optimal_ranges = compute_optimal_distance_query_batch_ranges(distance_query_batches,
                                                                            maximum_n_queries_per_range);

    ASSERT_EQUAL(maximum_n_queries_ranges.size(), 2);
    ASSERT_EQUAL(optimal_ranges.size(), maximum_n_queries_ranges.size());

    // The fixed limit produces two ranges: three 2-query batches followed by one 2-query batch.
    if (!check_batch_range_size(maximum_n_queries_ranges[0], 0, 3, 6) ||
        !check_batch_range_size(maximum_n_queries_ranges[1], 3, 4, 2))
    {
        return false;
    }

    // The optimal limit preserves two ranges while reducing the largest range: two 2-query batches per range.
    if (!check_batch_range_size(optimal_ranges[0], 0, 2, 4) || !check_batch_range_size(optimal_ranges[1], 2, 4, 4)) {
        return false;
    }

    return true;
}

bool test_unit_distance_query_batch_range_maximum_range_n_queries_empty_ranges() {
    const std::vector<DistanceQueryBatchRange> distance_query_batch_ranges;

    ASSERT_EQUAL(maximum_range_n_queries(distance_query_batch_ranges), 0);

    return true;
}

bool test_unit_distance_query_batch_range_maximum_range_n_queries() {
    const std::vector<DistanceQueryBatchRange> distance_query_batch_ranges{
        {0, 1, 5},
        {1, 3, 9},
        {3, 4, 7},
    };

    ASSERT_EQUAL(maximum_range_n_queries(distance_query_batch_ranges), 9);

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_unit_distance_query_batch_range_empty_batches),
        TEST(test_unit_distance_query_batch_range_bad_maximum_n_queries),
        TEST(test_unit_distance_query_batch_range_every_batch_of_maximum_size),
        TEST(test_unit_distance_query_batch_range_too_low_maximum_n_queries_throws),
        TEST(test_unit_distance_query_batch_range_one_batch_is_smaller),
        TEST(test_unit_distance_query_batch_range_exact_maximum_results_in_a_single_batch),
        TEST(test_unit_distance_query_batch_range_no_batch_exceeds_maximum_size),
        TEST(test_unit_distance_query_batch_range_maximum_larger_than_total_results_in_a_single_batch),
        TEST(test_unit_distance_query_batch_range_optimal_ranges_preserve_range_count),
        TEST(test_unit_distance_query_batch_range_maximum_range_n_queries_empty_ranges),
        TEST(test_unit_distance_query_batch_range_maximum_range_n_queries),
    };

    return Test::run_suite("test_unit_distance_query_batch_range", tests);
}
