/*
 * test_integration_concurrent_set.cpp - Integration tests for utils/ConcurrentSet.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <algorithm>
#include <atomic>
#include <cstddef>
#include <numeric>
#include <random>
#include <set>
#include <vector>

#include "common/test_harness/Test.hpp"
#include "common/utils/ConcurrentSet.hpp"
#include "common/utils/WorkerPool.hpp"

namespace PANGWES {
namespace {

constexpr auto test_n_workers = std::size_t{7};
constexpr auto test_n_keys = std::size_t{65536};

// Shared test keys used by the tests.
std::vector<std::size_t> test_keys = []() {
    std::vector<std::size_t> keys(test_n_keys);

    // Create unique keys.
    std::iota(keys.begin(), keys.end(), 12345);

    return keys;
}();

void shuffle_jobs(std::vector<JobT>& jobs) {
    std::mt19937 random_engine(12345);
    std::shuffle(jobs.begin(), jobs.end(), random_engine);
}

bool test_setup() {
    WorkerPool::stop();
    WorkerPool::init_and_start(test_n_workers);

    ASSERT_TRUE(WorkerPool::is_async());

    return true;
}

bool test_teardown() {
    WorkerPool::stop();

    return true;
}

bool test_integration_concurrent_set_inserting_many_unique_keys_multithreaded() {
    ConcurrentSet<std::size_t> set{};
    std::vector<JobT> jobs;

    // Spawn a job for inserting each key.
    for (std::size_t key_idx = 0; key_idx < test_n_keys; ++key_idx) {
        jobs.push_back([&set, key_idx]() {
            set.insert(test_keys[key_idx]);
        });
    }

    shuffle_jobs(jobs);
    WorkerPool::post(jobs);
    WorkerPool::wait();

    ASSERT_EQUAL(set.size(), test_keys.size());

    // Verify that all keys were inserted.
    for (const auto& key : test_keys) {
        ASSERT_TRUE(set.contains(key));
    }

    return true;
}

bool test_integration_concurrent_set_inserting_duplicate_keys_multithreaded() {
    ConcurrentSet<std::size_t> set{};
    std::atomic<int> inserted_duplicate_keys{};

    const auto half_of_keys = test_n_keys / 2;

    // First half of the keys in a single thread.
    for (std::size_t key_idx = 0; key_idx < half_of_keys; ++key_idx) {
        ASSERT_TRUE(set.insert(test_keys[key_idx]));
    }

    std::vector<JobT> jobs;

    // Next, spawn jobs for inserting the rest of the keys while also trying to insert duplicate keys.
    for (std::size_t key_idx = half_of_keys; key_idx < test_n_keys; ++key_idx) {
        // Job for inserting a new key.
        jobs.push_back([&set, key_idx]() {
            set.insert(test_keys[key_idx]);
        });

        // Job for inserting a duplicate key.
        const auto duplicate_key_idx = key_idx - half_of_keys;
        jobs.push_back([&set, &inserted_duplicate_keys, duplicate_key_idx]() {
            const bool insert_successful = set.insert(test_keys[duplicate_key_idx]);

            inserted_duplicate_keys.fetch_add(static_cast<int>(insert_successful));
        });
    }

    shuffle_jobs(jobs);
    WorkerPool::post(jobs);
    WorkerPool::wait();

    ASSERT_EQUAL(inserted_duplicate_keys.load(), 0);
    ASSERT_EQUAL(set.size(), test_keys.size());

    // Verify that both halves of the keys were inserted.
    for (const auto& key : test_keys) {
        ASSERT_TRUE(set.contains(key));
    }

    return true;
}

bool test_integration_concurrent_set_overlapping_insertion_jobs_multithreaded() {
    ConcurrentSet<std::size_t> set{};
    std::vector<JobT> jobs;

    // Spawn jobs that insert the same range of keys.
    for (std::size_t worker = 0; worker < test_n_workers; ++worker) {
        jobs.push_back([&set]() {
            for (std::size_t key_idx = 0; key_idx < test_n_keys; ++key_idx) {
                set.insert(test_keys[key_idx]);
            }
        });
    }

    WorkerPool::post(jobs);
    WorkerPool::wait();

    ASSERT_EQUAL(set.size(), test_keys.size());

    // Verify that all keys were inserted.
    for (const auto& key : test_keys) {
        ASSERT_TRUE(set.contains(key));
    }

    return true;
}

bool test_integration_concurrent_set_iterating_after_insertions_multithreaded() {
    ConcurrentSet<std::size_t> set{};
    std::vector<JobT> jobs;

    // Spawn a job for inserting each key.
    for (std::size_t key_idx = 0; key_idx < test_n_keys; ++key_idx) {
        jobs.push_back([&set, key_idx]() {
            set.insert(test_keys[key_idx]);
        });
    }

    shuffle_jobs(jobs);
    WorkerPool::post(jobs);
    WorkerPool::wait();

    ASSERT_EQUAL(set.size(), test_keys.size());

    std::set<std::size_t> keys_seen;

    for (const auto& key : set) {
        keys_seen.insert(key);
    }

    ASSERT_EQUAL(keys_seen.size(), test_n_keys);

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_integration_concurrent_set_inserting_many_unique_keys_multithreaded),
        TEST(test_integration_concurrent_set_inserting_duplicate_keys_multithreaded),
        TEST(test_integration_concurrent_set_overlapping_insertion_jobs_multithreaded),
        TEST(test_integration_concurrent_set_iterating_after_insertions_multithreaded),
    };

    return Test::run_suite("test_integration_concurrent_set", tests, test_setup, test_teardown);
}
