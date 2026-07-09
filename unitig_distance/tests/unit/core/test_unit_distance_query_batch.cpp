/*
 * test_unit_distance_query_batch.cpp - Unit tests for core/DistanceQueryBatch.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <cstddef>
#include <map>
#include <utility>
#include <vector>

#include "common/test_harness/Test.hpp"
#include "common/utils/Exception.hpp"
#include "common/utils/memory.hpp"
#include "mocks/MockFileReader.hpp"
#include "mocks/MockQueriesData.hpp"
#include "unitig_distance/core/DistanceQueryBatch.hpp"
#include "unitig_distance/io/QueriesReader.hpp"

namespace PANGWES {
namespace {

// Preset test mock queries data for use in some tests.
const auto preset_mock_queries_data = Mocks::MockQueriesData(std::vector<Mocks::MockQueryData>{
    {0, 1}, {0, 2}, {0, 3}, {0, 4}, {0, 5},
    {1, 2}, {1, 3}, {1, 4}, {2, 3}, {2, 4},
    {3, 5}, {3, 6}, {3, 7}, {4, 5}, {4, 7},
});

// Fills the provided vector with distance query batches from preset mock queries data. Returns false on any failures.
bool fill_mock_batches_from_preset_data(std::vector<DistanceQueryBatch>& distance_query_batches) {
    // Reset's is_open()'s mocked state.
    Mocks::MockIfStream::set_contents(preset_mock_queries_data.to_contents());

    auto queries_reader = QueriesReader(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file));

    distance_query_batches = DistanceQueryBatch::compute_distance_query_batches(queries_reader);

    ASSERT_FALSE(distance_query_batches.empty());

    return true;
}

// Checks that valid distance query batches were constructed from `mock_queries_data`.
bool check_batches(const Mocks::MockQueriesData& mock_queries_data,
                 const std::vector<DistanceQueryBatch>& distance_query_batches,
                 const std::vector<std::size_t>& expected_targets_sizes) noexcept
{
    std::map<std::size_t, std::size_t> query_indices_seen;

    for (std::size_t batch_idx = 0; batch_idx < distance_query_batches.size(); ++batch_idx) {
        const auto& distance_query_batch = distance_query_batches[batch_idx];
        const auto source_unitig_id = distance_query_batch.source_unitig_id();
        const auto& targets = distance_query_batch.targets();

        ASSERT_EQUAL(targets.size(), expected_targets_sizes[batch_idx]);

        for (const auto& target : targets) {
            const auto target_unitig_id = target.target_unitig_id;
            const auto query_index = target.query_index;

            ASSERT_LESS(query_index, mock_queries_data.size());
            ASSERT_TRUE(target_unitig_id == mock_queries_data.unitig1_id(query_index) ||
                        target_unitig_id == mock_queries_data.unitig2_id(query_index));
            ASSERT_TRUE(source_unitig_id == mock_queries_data.unitig1_id(query_index) ||
                        source_unitig_id == mock_queries_data.unitig2_id(query_index));

            ++query_indices_seen[query_index];
        }
    }

    // Verify that all query indices were accounted for and each index appeared exactly once.
    ASSERT_EQUAL(query_indices_seen.size(), mock_queries_data.size());

    for (const auto& query_index_seen : query_indices_seen) {
        const auto count = query_index_seen.second;

        ASSERT_EQUAL(count, 1);
    }

    return true;
}

bool test_unit_distance_query_batch_constructor() {
    constexpr auto test_source_unitig_id = 3;
    constexpr auto test_target_unitig_id_1 = 4;
    constexpr auto test_query_index_1 = 5;
    constexpr auto test_target_unitig_id_2 = 6;
    constexpr auto test_query_index_2 = 7;

    std::vector<DistanceQueryTarget> targets{
        {test_target_unitig_id_1, test_query_index_1},
        {test_target_unitig_id_2, test_query_index_2},
    };

    const auto distance_query_batch = DistanceQueryBatch(test_source_unitig_id, std::move(targets));
    const auto& stored_targets = distance_query_batch.targets();

    ASSERT_EQUAL(distance_query_batch.source_unitig_id(), test_source_unitig_id);
    ASSERT_EQUAL(stored_targets.size(), 2);
    ASSERT_EQUAL(stored_targets[0].target_unitig_id, test_target_unitig_id_1);
    ASSERT_EQUAL(stored_targets[0].query_index, test_query_index_1);
    ASSERT_EQUAL(stored_targets[1].target_unitig_id, test_target_unitig_id_2);
    ASSERT_EQUAL(stored_targets[1].query_index, test_query_index_2);

    return true;
}

bool test_unit_compute_distance_query_batches() {
    std::vector<DistanceQueryBatch> distance_query_batches;

    if (!fill_mock_batches_from_preset_data(distance_query_batches)) {
        return false;
    }

    /*
     * Expected batching for the preset dataset:
     * - 1st batch from unitig ID 3, involving 6 queries.
     * - 2nd batch from unitig ID 4, involving 5 of the remaining queries.
     * - 3rd batch from unitig ID 0, involving 3 of the remaining queries.
     * - 4th batch from unitig ID 1 or 2, involving one and the final remaining query.
     *
     * Note that it's not defined which unitig ID should become the source in the last batch. Therefore we must make an
     * adjustment for this in the testing.
    */
    ASSERT_EQUAL(distance_query_batches.size(), 4);

    const std::vector<std::size_t> expected_targets_sizes{6, 5, 3, 1};
    const std::vector<std::size_t> expected_batch_sources{3, 4, 0, 2};

    // Check that correct sources were chosen in the batches.
    for (std::size_t batch_idx = 0; batch_idx < distance_query_batches.size(); ++batch_idx) {
        const auto source_unitig_id = distance_query_batches[batch_idx].source_unitig_id();

        // Custom check for the last batch; see the note above.
        if (batch_idx + 1 == distance_query_batches.size()) {
            ASSERT_TRUE(source_unitig_id == 1 || source_unitig_id == 2);
        } else {
            ASSERT_EQUAL(source_unitig_id, expected_batch_sources[batch_idx]);
        }
    }

    return check_batches(preset_mock_queries_data, distance_query_batches, expected_targets_sizes);
}

bool test_unit_compute_distance_query_batches_not_batchable() {
    const auto mock_queries_data_not_batchable = Mocks::MockQueriesData(std::vector<Mocks::MockQueryData>{
        {0, 1}, {2, 3}, {4, 5}, {6, 7}, {8, 9},
    });
    Mocks::MockIfStream::set_contents(mock_queries_data_not_batchable.to_contents());

    auto queries_reader = QueriesReader(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file));

    const auto distance_query_batches = DistanceQueryBatch::compute_distance_query_batches(queries_reader);

    ASSERT_FALSE(distance_query_batches.empty());
    // Check that we got no optimization in the batching for this data set.
    ASSERT_EQUAL(mock_queries_data_not_batchable.size(), distance_query_batches.size());

    const std::vector<std::size_t> expected_targets_sizes{1, 1, 1, 1, 1};

    return check_batches(mock_queries_data_not_batchable, distance_query_batches, expected_targets_sizes);
}

bool test_unit_compute_distance_query_batches_single_batch() {
    const auto mock_queries_data = Mocks::MockQueriesData(std::vector<Mocks::MockQueryData>{
        {5, 0}, {5, 0}, {3, 0}, {2, 0}, {1, 0},
    });
    Mocks::MockIfStream::set_contents(mock_queries_data.to_contents());

    auto queries_reader = QueriesReader(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file));

    const auto distance_query_batches = DistanceQueryBatch::compute_distance_query_batches(queries_reader);

    ASSERT_FALSE(distance_query_batches.empty());
    // Check that we got a single batch.
    ASSERT_EQUAL(distance_query_batches.size(), 1);

    return check_batches(mock_queries_data, distance_query_batches, std::vector<std::size_t>{mock_queries_data.size()});
}

bool test_unit_distance_query_batch_reserved_bytes() {
    std::vector<DistanceQueryBatch> distance_query_batches;

    if (!fill_mock_batches_from_preset_data(distance_query_batches)) {
        return false;
    }

    std::size_t expected_total_reserved_bytes = distance_query_batches.capacity() * sizeof(DistanceQueryBatch);

    for (const auto& distance_query_batch : distance_query_batches) {
        const auto& targets = distance_query_batch.targets();

        using target_type = Traits::element_type_t<decltype(targets)>;
        std::size_t expected_reserved_bytes = sizeof(distance_query_batch) +
                                              targets.capacity() * sizeof(target_type);

        // The object size is included in `reserved_bytes()`.
        ASSERT_EQUAL(distance_query_batch.reserved_bytes(), expected_reserved_bytes);

        // The outer calculation already included `sizeof(DistanceQueryBatch)`, so avoid double counting.
        expected_total_reserved_bytes += expected_reserved_bytes - sizeof(DistanceQueryBatch);
    }

    ASSERT_EQUAL(Memory::container_reserved_bytes(distance_query_batches), expected_total_reserved_bytes);

    return true;
}

bool test_unit_distance_query_batch_maximum_batch_n_queries_empty_batches() {
    const std::vector<DistanceQueryBatch> distance_query_batches;

    ASSERT_EQUAL(maximum_batch_n_queries(distance_query_batches), 0);

    return true;
}

bool test_unit_distance_query_batch_maximum_batch_n_queries() {
    std::vector<DistanceQueryBatch> distance_query_batches;

    if (!fill_mock_batches_from_preset_data(distance_query_batches)) {
        return false;
    }

    ASSERT_EQUAL(maximum_batch_n_queries(distance_query_batches), 6);

    return true;
}

bool test_unit_compute_distance_query_batches_throws_on_empty_queries_file() {
    std::vector<DistanceQueryBatch> distance_query_batches;

    Mocks::MockIfStream::set_contents({});

    auto queries_reader = QueriesReader(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file));

    EXPECT_THROW(DistanceQueryBatch::compute_distance_query_batches(queries_reader), ErrorCode::FILE_EMPTY);

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_unit_distance_query_batch_constructor),
        TEST(test_unit_compute_distance_query_batches),
        TEST(test_unit_compute_distance_query_batches_not_batchable),
        TEST(test_unit_compute_distance_query_batches_single_batch),
        TEST(test_unit_distance_query_batch_reserved_bytes),
        TEST(test_unit_distance_query_batch_maximum_batch_n_queries_empty_batches),
        TEST(test_unit_distance_query_batch_maximum_batch_n_queries),
        TEST(test_unit_compute_distance_query_batches_throws_on_empty_queries_file),
    };

    return Test::run_suite("test_unit_distance_query_batch", tests);
}
