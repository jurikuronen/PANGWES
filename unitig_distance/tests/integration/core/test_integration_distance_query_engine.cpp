/*
 * test_integration_distance_query_engine.cpp - Integration tests for core/DistanceQueryEngine.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "common/io/FileReader.hpp"
#include "common/test_harness/Test.hpp"
#include "common/utils/Exception.hpp"
#include "common/utils/memory.hpp"
#include "common/utils/WorkerPool.hpp"
#include "mocks/MockFileReader.hpp"
#include "mocks/MockQueriesData.hpp"
#include "test_integration_distance_data.hpp"
#include "test_integration_distance_query_common.hpp"
#include "unitig_distance/core/Distance.hpp"
#include "unitig_distance/core/DistanceQueryEngine.hpp"
#include "unitig_distance/io/QueriesReader.hpp"

namespace PANGWES {
namespace {

bool test_setup() {
    WorkerPool::stop();

    return true;
}

bool test_teardown() {
    WorkerPool::stop();

    return true;
}

// Compute distances for the unitig-pairs in the test_efc_k31.queries file.
std::vector<Distance> test_compute_distances(std::size_t n_workers, bool no_median_distance) {
    WorkerPool::stop();
    WorkerPool::init_and_start(n_workers);

    const auto unitig_weights = make_unitig_weights();
    const auto sgg_edges_filenames = make_sgg_edges_filenames();
    auto queries_reader = QueriesReader(Memory::make_unique<FileReader>(queries_filename));

    DistanceQueryEngine distance_query_engine(unitig_weights,
                                              sgg_edges_filenames,
                                              queries_reader,
                                              Memory::GiB,
                                              no_median_distance);

    return distance_query_engine.compute_distances();
}

/*
 * Compute distances for the unitig-pairs in test_integration_distance_data.hpp whose expected distances have been
 * calculated via brute-force.
*/
std::vector<Distance> test_compute_expected_distances(std::size_t n_workers, bool no_median_distance) {
    WorkerPool::stop();
    WorkerPool::init_and_start(n_workers);

    const auto unitig_weights = make_unitig_weights();
    const auto sgg_edges_filenames = make_sgg_edges_filenames();

    // Set query contents after make_sgg_edges_filenames() has consumed its mocked file contents.
    std::vector<Mocks::MockQueryData> queries;
    queries.reserve(DistanceTestData::expected_distance_data.size());
    for (const auto& test_data : DistanceTestData::expected_distance_data) {
        queries.emplace_back(test_data.unitig_from, test_data.unitig_to);
    }
    Mocks::MockIfStream::set_contents(Mocks::MockQueriesData(queries).to_contents());

    auto queries_reader = QueriesReader(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file));

    DistanceQueryEngine distance_query_engine(unitig_weights,
                                              sgg_edges_filenames,
                                              queries_reader,
                                              Memory::GiB,
                                              no_median_distance);

    return distance_query_engine.compute_distances();
}

// Verifies that single-threaded and async modes obtained the same results.
bool check_distance_statistics_match(const std::vector<Distance>& single_thread_distances,
                                     const std::vector<Distance>& async_distances)
{
    ASSERT_EQUAL(single_thread_distances.size(), async_distances.size());

    bool observed_multiple_distances = false;

    for (std::size_t query_index = 0; query_index < single_thread_distances.size(); ++query_index) {
        const auto& single_thread_distance = single_thread_distances[query_index];
        const auto& async_distance = async_distances[query_index];
        const auto count = single_thread_distance.count();

        ASSERT_EQUAL(async_distance.count(), count);

        if (count == 0) {
            continue;
        }

        ASSERT_EQUAL(single_thread_distance.mean_distance(), async_distance.mean_distance());
        ASSERT_EQUAL(single_thread_distance.min_distance(), async_distance.min_distance());
        ASSERT_EQUAL(single_thread_distance.max_distance(), async_distance.max_distance());
        ASSERT_EQUAL(single_thread_distance.has_median_distance(), async_distance.has_median_distance());

        if (single_thread_distance.has_median_distance()) {
            ASSERT_EQUAL(single_thread_distance.median_distance(), async_distance.median_distance());
        }

        if (count > 1) {
            observed_multiple_distances = true;
            ASSERT_EQUAL(single_thread_distance.sample_variance(), async_distance.sample_variance());
        }
    }

    ASSERT_TRUE(observed_multiple_distances);

    return true;
}

// Verifies that distances calculated by DistanceQueryEngine match those test_integration_distance_data.hpp.
bool check_distances_match_expected(const std::vector<Distance>& distances, bool median_included)
{
    ASSERT_EQUAL(distances.size(), DistanceTestData::expected_distance_data.size());

    for (std::size_t query_index = 0; query_index < distances.size(); ++query_index) {
        const auto& distance = distances[query_index];
        const auto& expected_distance = DistanceTestData::expected_distance_data[query_index];
        const auto expected_count = expected_distance.count();

        ASSERT_EQUAL(distance.count(), expected_count);

        if (expected_count == 0) {
            continue;
        }

        ASSERT_EQUAL(distance.mean_distance(), expected_distance.mean_distance());
        ASSERT_EQUAL(distance.min_distance(), expected_distance.min_distance());
        ASSERT_EQUAL(distance.max_distance(), expected_distance.max_distance());
        ASSERT_EQUAL(distance.has_median_distance(), median_included);

        if (median_included) {
            ASSERT_EQUAL(distance.median_distance(), expected_distance.median_distance());
        }

        if (expected_count > 1) {
            ASSERT_EQUAL(distance.sample_variance(), expected_distance.sample_variance());
        }
    }

    return true;
}

bool test_integration_distance_query_engine_mean_async_and_single_threaded_results_match() {
    const auto single_thread_distances = test_compute_distances(0, true);
    const auto async_distances = test_compute_distances(3, true);

    if (!check_distance_statistics_match(single_thread_distances, async_distances)) {
        return false;
    }

    // check_distance_statistics_match() checks has_median_distance() matches between the distances.
    for (const auto& distance : async_distances) {
        ASSERT_FALSE(distance.has_median_distance());
    }

    return true;
}

bool test_integration_distance_query_engine_median_async_and_single_threaded_results_match() {
    const auto single_thread_distances = test_compute_distances(0, false);
    const auto async_distances = test_compute_distances(3, false);

    return check_distance_statistics_match(single_thread_distances, async_distances);
}

bool test_integration_distance_query_engine_mean_results_match_expected() {
    const auto single_thread_distances = test_compute_expected_distances(0, true);
    const auto async_distances = test_compute_expected_distances(3, true);

    if (!check_distance_statistics_match(single_thread_distances, async_distances)) {
        return false;
    }

    if (!check_distances_match_expected(single_thread_distances, false)) {
        return false;
    }

    return check_distances_match_expected(async_distances, false);
}

bool test_integration_distance_query_engine_median_results_match_expected() {
    const auto single_thread_distances = test_compute_expected_distances(0, false);
    const auto async_distances = test_compute_expected_distances(3, false);

    if (!check_distance_statistics_match(single_thread_distances, async_distances)) {
        return false;
    }

    if (!check_distances_match_expected(single_thread_distances, true)) {
        return false;
    }

    return check_distances_match_expected(async_distances, true);
}

bool test_integration_distance_query_engine_constructor_empty_queries_throws() {
    WorkerPool::init_and_start(0);

    const auto unitig_weights = make_unitig_weights();
    const auto sgg_edges_filenames = make_sgg_edges_filenames();

    Mocks::MockIfStream::set_contents({});
    auto queries_reader = QueriesReader(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file));

    // Request the MedianDistanceQueryEngine, which posts jobs to the worker pool.
    EXPECT_THROW(DistanceQueryEngine(unitig_weights, sgg_edges_filenames, queries_reader, Memory::GiB, false),
                 ErrorCode::FILE_EMPTY);

    // Check that DistanceQueryMemoryBudget's constructor's fail-safe triggered and stopped the worker pool.
    ASSERT_TRUE(WorkerPool::stopped());

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_integration_distance_query_engine_mean_async_and_single_threaded_results_match),
        TEST(test_integration_distance_query_engine_median_async_and_single_threaded_results_match),
        TEST(test_integration_distance_query_engine_mean_results_match_expected),
        TEST(test_integration_distance_query_engine_median_results_match_expected),
        TEST(test_integration_distance_query_engine_constructor_empty_queries_throws),
    };

    return Test::run_suite("test_integration_distance_query_engine", tests, test_setup, test_teardown);
}
