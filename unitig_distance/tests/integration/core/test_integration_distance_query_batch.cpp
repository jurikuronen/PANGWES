/*
 * test_integration_distance_query_batch.cpp - Integration tests for core/DistanceQueryBatch.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <unordered_map>

#include "common/io/FileReader.hpp"
#include "common/test_harness/Test.hpp"
#include "common/utils/memory.hpp"
#include "unitig_distance/core/DistanceQueryBatch.hpp"
#include "unitig_distance/io/QueriesReader.hpp"

namespace PANGWES {
namespace {

constexpr auto queries_file = TEST_DATA_DIR "/test_efc_k31.queries";
constexpr auto expected_queries_size = 5000;

// Regression guard for the batching result.
constexpr auto expected_query_batches_max_size = 560;

bool test_integration_distance_query_batch_from_queries() {
    auto queries_reader = QueriesReader(Memory::make_unique<FileReader>(queries_file));

    const auto distance_query_batches = DistanceQueryBatch::compute_distance_query_batches(queries_reader);

    ASSERT_EQUAL(distance_query_batches.size(), expected_query_batches_max_size);

    // Count queries for each unitig.
    std::unordered_map<std::size_t, std::size_t> query_counts_per_unitig_id;

    // Re-read the queries.
    queries_reader.start_reading();

    std::size_t n_queries = 0;
    for (QueriesReader::QueryData query_data; queries_reader.getquery(query_data); ) {
        ++query_counts_per_unitig_id[query_data.unitig1_id];
        ++query_counts_per_unitig_id[query_data.unitig2_id];
        ++n_queries;
    }

    queries_reader.stop_reading();

    ASSERT_EQUAL(n_queries, expected_queries_size);

    // For tracking how many times each query index appears in the batches.
    std::unordered_map<std::size_t, std::size_t> index_counts;

    // Go through the batches and decrement query counts while tracking seen query indices.
    for (const auto& batch : distance_query_batches) {
        const auto source_unitig_id = batch.source_unitig_id();
        const auto& targets = batch.targets();

        query_counts_per_unitig_id[source_unitig_id] -= targets.size();

        for (const auto& target : targets) {
            const auto target_unitig_id = target.target_unitig_id;
            const auto query_index = target.query_index;
            --query_counts_per_unitig_id[target_unitig_id];
            ++index_counts[query_index];
        }
    }

    // Verify that each query index was seen exactly once.
    ASSERT_EQUAL(index_counts.size(), n_queries);
    for (std::size_t i = 0; i < n_queries; ++i) {
        ASSERT_EQUAL(index_counts.count(i), 1);
    }

    // Verify that correct query counts per unitig ID were observed in the batches.
    for (const auto query_count_data : query_counts_per_unitig_id) {
        const auto count = query_count_data.second;

        /*
         * We first counted the queries for this unitig manually, and then subtracted for each occurrence while going
         * through the batches. If everything cancels out, the count should now be zero.
        */
        ASSERT_EQUAL(count, 0);
    }

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_integration_distance_query_batch_from_queries),
    };

    return Test::run_suite("test_integration_distance_query_batch", tests);
}
