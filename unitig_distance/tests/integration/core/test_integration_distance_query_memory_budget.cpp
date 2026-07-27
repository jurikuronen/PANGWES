/*
 * test_integration_distance_query_memory_budget.cpp - Integration tests for core/DistanceQueryMemoryBudget.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <cstddef>

#include "common/test_harness/Test.hpp"
#include "common/utils/Exception.hpp"
#include "common/utils/memory.hpp"
#include "common/utils/WorkerPool.hpp"
#include "test_integration_distance_query_common.hpp"
#include "unitig_distance/core/DistanceQueryMemoryBudget.hpp"

namespace PANGWES {
namespace {

// Chosen so maximum_distance_matrix_rows() returns less than test_max_matrix_rows multithreaded cases.
constexpr auto test_n_workers = 3;
constexpr auto test_max_memory_usage_bytes = 88 * Memory::MiB;
constexpr auto test_base_memory_bytes = Memory::MiB * 64;
constexpr auto test_max_matrix_rows = 125000;

bool test_setup() {
    WorkerPool::stop();

    WorkerPool::init_and_start(test_n_workers);

    return true;
}

bool test_teardown() {
    WorkerPool::stop();

    return true;
}

bool test_integration_distance_query_memory_budget_happy_path() {

    const auto unitig_weights = make_unitig_weights();
    const auto sgg_edges_filenames = make_sgg_edges_filenames();

    DistanceQueryMemoryBudget::estimate_sgg_memory_usage(unitig_weights, sgg_edges_filenames);

    ASSERT_GREATER(WorkerPool::unfinished_jobs(), 0);
    WorkerPool::wait();

    // Check that calculating the maximum matrix rows waits for the asynchronous estimation jobs.
    const auto maximum_rows = DistanceQueryMemoryBudget::maximum_distance_matrix_rows(test_max_memory_usage_bytes,
                                                                                      test_base_memory_bytes,
                                                                                      n_sggs,
                                                                                      test_max_matrix_rows);

    ASSERT_EQUAL(WorkerPool::unfinished_jobs(), 0);

    ASSERT_GREATER(maximum_rows, 0);
    ASSERT_LESS(maximum_rows, test_max_matrix_rows);

    return true;
}

bool test_integration_distance_query_memory_budget_single_thread_fits_all_rows() {
    // Verify CALLER_THREAD + 1 worker are equal.
    for (std::size_t n_workers = 0; n_workers <= 1; ++n_workers) {
        WorkerPool::stop();
        WorkerPool::init_and_start(n_workers);

        const auto unitig_weights = make_unitig_weights();
        const auto sgg_edges_filenames = make_sgg_edges_filenames();

        DistanceQueryMemoryBudget::estimate_sgg_memory_usage(unitig_weights, sgg_edges_filenames);
        WorkerPool::wait();

        const auto maximum_rows = DistanceQueryMemoryBudget::maximum_distance_matrix_rows(test_max_memory_usage_bytes,
                                                                                          test_base_memory_bytes,
                                                                                          n_sggs,
                                                                                          test_max_matrix_rows);

        // The test sizes have been chosen such that single-thread can fit all the rows into memory.
        ASSERT_EQUAL(maximum_rows, test_max_matrix_rows);
    }

    return true;
}

bool test_integration_distance_query_memory_budget_max_memory_usage_too_low_throws() {
    const auto unitig_weights = make_unitig_weights();
    const auto sgg_edges_filenames = make_sgg_edges_filenames();

    DistanceQueryMemoryBudget::estimate_sgg_memory_usage(unitig_weights, sgg_edges_filenames);
    WorkerPool::wait();

    // The memory limit cannot accommodate the dynamic memory allowance.
    EXPECT_THROW(DistanceQueryMemoryBudget::maximum_distance_matrix_rows(Memory::MiB, 0, n_sggs, test_max_matrix_rows),
                 ErrorCode::INVALID_ARGUMENT);

    return true;
}

bool test_integration_distance_query_memory_budget_max_memory_usage_too_low_throws_2() {
    const auto unitig_weights = make_unitig_weights();
    const auto sgg_edges_filenames = make_sgg_edges_filenames();

    DistanceQueryMemoryBudget::estimate_sgg_memory_usage(unitig_weights, sgg_edges_filenames);
    WorkerPool::wait();

    // The memory limit hits the SGG memory check.
    EXPECT_THROW(DistanceQueryMemoryBudget::maximum_distance_matrix_rows(test_max_memory_usage_bytes,
                                                                         test_base_memory_bytes,
                                                                         n_sggs * 10000,
                                                                         test_max_matrix_rows),
                 ErrorCode::INVALID_ARGUMENT);

    return true;
}

bool test_integration_distance_query_memory_budget_zero_matrix_dimensions_throws() {
    const auto unitig_weights = make_unitig_weights();
    const auto sgg_edges_filenames = make_sgg_edges_filenames();

    DistanceQueryMemoryBudget::estimate_sgg_memory_usage(unitig_weights, sgg_edges_filenames);
    WorkerPool::wait();

    EXPECT_THROW(DistanceQueryMemoryBudget::maximum_distance_matrix_rows(test_max_memory_usage_bytes,
                                                                         test_base_memory_bytes,
                                                                         0,
                                                                         0),
                 ErrorCode::INVALID_ARGUMENT);

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_integration_distance_query_memory_budget_happy_path),
        TEST(test_integration_distance_query_memory_budget_single_thread_fits_all_rows),
        TEST(test_integration_distance_query_memory_budget_max_memory_usage_too_low_throws),
        TEST(test_integration_distance_query_memory_budget_max_memory_usage_too_low_throws_2),
        TEST(test_integration_distance_query_memory_budget_zero_matrix_dimensions_throws),
    };

    return Test::run_suite("test_integration_distance_query_memory_budget", tests, test_setup, test_teardown);
}
