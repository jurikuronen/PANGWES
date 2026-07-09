/*
 * test_integration_worker_pool.cpp - Integration tests for utils/WorkerPool.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <thread>
#include <vector>

#include "common/test_harness/Test.hpp"
#include "common/utils/Exception.hpp"
#include "common/utils/WorkerPool.hpp"

namespace PANGWES {
namespace {

constexpr std::size_t test_max_n_workers = 5;
constexpr std::size_t test_n_jobs = 20;
constexpr auto test_max_wait = std::chrono::seconds{1};

// Global test "mock job" funcs. Initialized in setup_test_jobs().
std::vector<JobT> test_job_funcs;

// Global storage about jobs that are allowed to finish. Initialized in setup_test_jobs();
std::vector<int> test_job_can_finish;

// Global storage about completed mock jobs. Initialized in setup_test_jobs().
std::vector<int> test_job_states;

// Global value recording the count of mock jobs that actually started running.
std::size_t test_jobs_started;

// Global Boolean that will be set to true when WorkerPool::stop() is called.
std::atomic<int> test_pool_stopped;

// Global synchronization primitives to control mock job execution.
std::condition_variable test_cv;
std::mutex test_mtx;

/*
 * Setups `n_jobs` mock jobs.
 *
 * In caller thread state, all jobs complete immediately.
 *
 * In async state, workers wait until given permission to finish the job. This is controlled by the test functions:
 * - finish_n_test_jobs()
 * - finish_all_test_jobs()
 * - test_stop_worker_pool()
 *
 * To avoid issues getting blocked on the calling thread, avoid using `WorkerPool::stop()` in these tests and prefer
 * the helper `test_stop_worker_pool()`.
*/
void setup_test_jobs(bool async_mode) {
    test_job_funcs.resize(test_n_jobs);
    test_job_can_finish.assign(test_n_jobs, 0);
    test_job_states.assign(test_n_jobs, 0);
    test_jobs_started = 0;
    test_pool_stopped = 0;

    for (std::size_t job = 0; job < test_n_jobs; ++job) {
        test_job_funcs[job] = [async_mode, job](){
            // Record that the job started running.
            {
                std::lock_guard<std::mutex> lock(test_mtx);
                ++test_jobs_started;
            }
            test_cv.notify_all();

            /*
             * Async state: workers wait until given permission to finish the job.
             * Caller thread state: test job completes immediately.
            */
            if (async_mode) {
                std::unique_lock<std::mutex> lock(test_mtx);

                // Wait until permission to proceed.
                test_cv.wait_for(lock, test_max_wait,
                                 [job](){ return test_pool_stopped != 0 || test_job_can_finish[job] != 0; });
            }

            // When not in asynchronous mode, finish immediately.
            test_job_states[job] = static_cast<int>(!async_mode || test_job_can_finish[job] != 0);
        };
    }
}

// Safely stops the pool by freeing up workers in the mock jobs.
void test_stop_worker_pool() {
    test_pool_stopped = 1;
    test_cv.notify_all();

    WorkerPool::stop();
}

// Waits for a given number of mock jobs to have started.
void test_wait_for_jobs_to_start(std::size_t n_jobs_to_start) {
    assert(n_jobs_to_start >= 0);

    std::unique_lock<std::mutex> lock(test_mtx);

    test_cv.wait_for(lock, test_max_wait, [n_jobs_to_start]() { return test_jobs_started >= n_jobs_to_start; });
}

// Waits a short time for any mock jobs to finish.
void test_wait_for_current_jobs_to_end() {
    std::this_thread::sleep_for(std::chrono::milliseconds{20});
}

// Signals the first n jobs to finish and notifies all workers.
void finish_n_test_jobs(std::size_t n) {
    std::lock_guard<std::mutex> lock(test_mtx);

    for (std::size_t i = 0; i < n; ++i) {
        test_job_can_finish[i] = 1;
    }
    test_cv.notify_all();
}

// Signals all jobs to finish and notifies all workers.
void finish_all_test_jobs() {
    finish_n_test_jobs(test_job_funcs.size());
}

// Counts the number of finished jobs.
int count_test_jobs_done() noexcept {
    int jobs_done = 0;

    for (const auto& job_state : test_job_states) {
        jobs_done += static_cast<int>(job_state != 0);
    }

    return jobs_done;
}

// Returns true if any test job was finished.
bool any_test_jobs_done() noexcept {
    return count_test_jobs_done() > 0;
}

// Returns true if all test jobs were finished.
bool all_test_jobs_done() noexcept {
    return count_test_jobs_done() == static_cast<int>(test_job_states.size());
}

// Finishes all mock jobs and stops the worker pool.
void test_cleanup() {
    finish_all_test_jobs();

    test_stop_worker_pool();
}

bool test_setup() {
    test_cleanup();

    return true;
}

bool test_teardown() {
    test_cleanup();

    return true;
}

bool check_start_and_stop(std::size_t n_workers) {
    WorkerPool::init_and_start(n_workers);

    ASSERT_FALSE(WorkerPool::stopped());
    ASSERT_EQUAL(WorkerPool::unfinished_jobs(), 0);

    test_stop_worker_pool();

    ASSERT_TRUE(WorkerPool::stopped());

    return true;
}

bool check_post_jobs(std::size_t n_workers) {
    WorkerPool::init_and_start(n_workers);

    WorkerPool::post(test_job_funcs);
    if (WorkerPool::is_async()) {
        ASSERT_EQUAL(WorkerPool::unfinished_jobs(), test_n_jobs);
    } else {
        ASSERT_EQUAL(WorkerPool::unfinished_jobs(), 0);
    }

    test_cleanup();

    return true;
}

bool check_post_jobs_and_wait(std::size_t n_workers, std::size_t unfinished_jobs_count_at_most_to_wait = 0) {
    WorkerPool::init_and_start(n_workers);

    WorkerPool::post(test_job_funcs);

    if (!WorkerPool::is_async()) {
        // Jobs done immediately by the calling thread.
        ASSERT_EQUAL(WorkerPool::unfinished_jobs(), 0);
    }

    // Normal waiting.
    if (unfinished_jobs_count_at_most_to_wait == 0) {
        finish_all_test_jobs();

        WorkerPool::wait();

        ASSERT_EQUAL(WorkerPool::unfinished_jobs(), 0);

        ASSERT_TRUE(all_test_jobs_done());
    // Waiting until some count of unfinished jobs.
    } else {
        // Jobs to finish for fulfilling `unfinished_jobs_count_at_most_to_wait` criterion.
        const auto jobs_to_finish = unfinished_jobs_count_at_most_to_wait <= test_n_jobs
                                    ? test_n_jobs - unfinished_jobs_count_at_most_to_wait
                                    : 0;
        finish_n_test_jobs(jobs_to_finish);

        WorkerPool::wait_until_unfinished_jobs_at_most(unfinished_jobs_count_at_most_to_wait);

        ASSERT_LESS_EQUAL(WorkerPool::unfinished_jobs(), unfinished_jobs_count_at_most_to_wait);

        ASSERT_EQUAL(count_test_jobs_done(), jobs_to_finish);
    }

    test_cleanup();

    return true;
}

bool check_post_more_jobs(std::size_t n_workers) {
    auto new_job_done = false;

    WorkerPool::init_and_start(n_workers);

    WorkerPool::post(test_job_funcs);
    WorkerPool::post({[&new_job_done](){ new_job_done = true; }});

    finish_all_test_jobs();

    WorkerPool::wait();

    ASSERT_TRUE(all_test_jobs_done());
    ASSERT_TRUE(new_job_done);
    ASSERT_EQUAL(WorkerPool::unfinished_jobs(), 0);

    test_cleanup();

    return true;
}

bool check_stop_prevents_adding_new_jobs(std::size_t n_workers) {
    WorkerPool::init_and_start(n_workers);

    test_stop_worker_pool();

    EXPECT_THROW(WorkerPool::post(test_job_funcs), ErrorCode::OPERATION_FOR_STOPPED_WORKER_POOL);

    finish_all_test_jobs();

    test_wait_for_current_jobs_to_end();

    ASSERT_EQUAL(WorkerPool::unfinished_jobs(), 0);
    ASSERT_FALSE(any_test_jobs_done());

    test_cleanup();

    return true;
}

bool check_stop_stops_new_processing_jobs_async(std::size_t n_workers) {
    WorkerPool::init_and_start(n_workers);

    WorkerPool::post(test_job_funcs);

    test_wait_for_jobs_to_start(n_workers);

    /*
     * In async mode, sets all mock jobs assigned to current workers as finished. This simulates the current jobs
     * completing when the worker pool is stopped. Here, `test_stop_worker_pool()` releases all workers and test job
     * states will be set accordingly to how many jobs we set as finished here.
    */
    finish_n_test_jobs(n_workers);

    test_stop_worker_pool();

    test_wait_for_current_jobs_to_end();

    const auto n_jobs_done = count_test_jobs_done();

    ASSERT_EQUAL(n_jobs_done, n_workers);

    // Now set all jobs as finished and confirm that the pool didn't allow the next jobs to proceed.
    finish_all_test_jobs();

    test_wait_for_current_jobs_to_end();

    const auto n_jobs_done_after = count_test_jobs_done();

    ASSERT_EQUAL(n_jobs_done, n_jobs_done_after);

    return true;
}

bool check_bad_job_stops_pool_and_defer_seeing_exception(std::size_t n_workers) {
    WorkerPool::init_and_start(n_workers);

    WorkerPool::post(test_job_funcs);
    WorkerPool::post({[](){ throw Exception{ErrorCode::INVALID_KMER_LENGTH}; }});

    // Wait for bad job to execute.
    finish_all_test_jobs();

    test_wait_for_current_jobs_to_end();

    ASSERT_TRUE(WorkerPool::stopped());
    ASSERT_EQUAL(WorkerPool::unfinished_jobs(), 0);

    return true;
}

bool test_integration_worker_pool_start_and_stop_caller_thread() {
    return check_start_and_stop(0);
}

bool test_integration_worker_pool_init_and_start_while_in_caller_thread_mode_is_noop() {
    WorkerPool::init_and_start(0);
    WorkerPool::init_and_start(test_max_n_workers);

    ASSERT_FALSE(WorkerPool::stopped());
    ASSERT_FALSE(WorkerPool::is_async());
    ASSERT_EQUAL(WorkerPool::n_workers(), 0);

    return true;
}

bool test_integration_worker_pool_start_and_stop_async() {
    for (std::size_t n_workers = 1; n_workers <= test_max_n_workers; ++n_workers) {
        if (!check_start_and_stop(n_workers)) {
            return false;
        }
    }

    return true;
}

bool test_integration_worker_pool_init_and_start_while_async_is_noop() {
    constexpr auto initial_n_workers = std::size_t{2};

    WorkerPool::init_and_start(initial_n_workers);
    WorkerPool::init_and_start(0);

    ASSERT_FALSE(WorkerPool::stopped());
    ASSERT_TRUE(WorkerPool::is_async());
    ASSERT_EQUAL(WorkerPool::n_workers(), initial_n_workers);

    WorkerPool::init_and_start(test_max_n_workers);

    ASSERT_FALSE(WorkerPool::stopped());
    ASSERT_TRUE(WorkerPool::is_async());
    ASSERT_EQUAL(WorkerPool::n_workers(), initial_n_workers);

    return true;
}

bool test_integration_worker_pool_post_jobs_caller_thread() {
    setup_test_jobs(false);

    return check_post_jobs(0);
}

bool test_integration_worker_pool_post_jobs_async() {
    for (std::size_t n_workers = 1; n_workers <= test_max_n_workers; ++n_workers) {
        setup_test_jobs(true);

        if (!check_post_jobs(n_workers)) {
            return false;
        }
    }

    return true;
}

bool test_integration_worker_pool_post_jobs_and_wait_caller_thread() {
    setup_test_jobs(false);

    return check_post_jobs_and_wait(0);
}

bool test_integration_worker_pool_post_jobs_and_wait_async() {
    for (std::size_t n_workers = 1; n_workers <= test_max_n_workers; ++n_workers) {
        setup_test_jobs(true);

        if (!check_post_jobs_and_wait(n_workers)) {
            return false;
        }
    }

    return true;
}

bool test_integration_worker_pool_post_jobs_and_wait_until_n_finished_async() {
    std::vector<std::size_t> unfinished_job_counts{1, 5, test_n_jobs - 1, test_n_jobs, test_n_jobs + 1};
    for (std::size_t n_workers = 1; n_workers <= test_max_n_workers; ++n_workers) {
        for (const auto unfinished_jobs_count_at_most_to_wait : unfinished_job_counts) {
            setup_test_jobs(true);

            if (!check_post_jobs_and_wait(n_workers, unfinished_jobs_count_at_most_to_wait)) {
                return false;
            }
        }
    }

    return true;
}

bool test_integration_worker_pool_post_more_jobs_caller_thread() {
    setup_test_jobs(false);

    return check_post_more_jobs(0);
}

bool test_integration_worker_pool_post_more_jobs_async() {
    for (std::size_t n_workers = 1; n_workers <= test_max_n_workers; ++n_workers) {
        setup_test_jobs(true);

        if (!check_post_more_jobs(n_workers)) {
            return false;
        }
    }

    return true;
}

bool test_integration_worker_pool_stop_prevents_adding_new_jobs_caller_thread() {
    setup_test_jobs(false);

    return check_stop_prevents_adding_new_jobs(0);
}

bool test_integration_worker_pool_stop_prevents_adding_new_jobs_async() {
    for (std::size_t n_workers = 1; n_workers <= test_max_n_workers; ++n_workers) {
        setup_test_jobs(true);

        if (!check_stop_prevents_adding_new_jobs(n_workers)) {
            return false;
        }
    }
    return true;
}

bool test_integration_worker_pool_stop_stops_processing_jobs_async() {
    for (std::size_t n_workers = 1; n_workers <= test_max_n_workers; ++n_workers) {
        setup_test_jobs(true);

        if (!check_stop_stops_new_processing_jobs_async(n_workers)) {
            return false;
        }
    }
    return true;
}

bool test_integration_worker_pool_throwing_job_stops_pool_rethrown_on_init_and_start() {
    for (std::size_t n_workers = 1; n_workers <= test_max_n_workers; ++n_workers) {
        setup_test_jobs(true);

        if (!check_bad_job_stops_pool_and_defer_seeing_exception(n_workers)) {
            return false;
        }

        // Calling init_and_start() rethrows the exception and also handles worker pool clean-up.
        EXPECT_THROW(WorkerPool::init_and_start(n_workers), ErrorCode::INVALID_KMER_LENGTH);

        // Now the pool should be properly stopped and the stored exception cleared, so we get the ordinary exception.
        EXPECT_THROW(WorkerPool::post(test_job_funcs), ErrorCode::OPERATION_FOR_STOPPED_WORKER_POOL);
    }
    return true;
}

bool test_integration_worker_pool_throwing_job_stops_pool_rethrown_on_post() {
    for (std::size_t n_workers = 1; n_workers <= test_max_n_workers; ++n_workers) {
        setup_test_jobs(true);

        if (!check_bad_job_stops_pool_and_defer_seeing_exception(n_workers)) {
            return false;
        }

        // Calling post() rethrows the exception and also handles worker pool clean-up.
        EXPECT_THROW(WorkerPool::post(test_job_funcs), ErrorCode::INVALID_KMER_LENGTH);

        // Now the pool should be properly stopped and the stored exception cleared, so we get the ordinary exception.
        EXPECT_THROW(WorkerPool::post(test_job_funcs), ErrorCode::OPERATION_FOR_STOPPED_WORKER_POOL);
    }
    return true;
}

bool test_integration_worker_pool_throwing_job_stops_pool_rethrown_on_wait() {
    for (std::size_t n_workers = 1; n_workers <= test_max_n_workers; ++n_workers) {
        setup_test_jobs(true);

        if (!check_bad_job_stops_pool_and_defer_seeing_exception(n_workers)) {
            return false;
        }

        // Calling wait() rethrows the exception and also handles worker pool clean-up.
        EXPECT_THROW(WorkerPool::wait(), ErrorCode::INVALID_KMER_LENGTH);

        // Now the pool should be properly stopped and the stored exception cleared, so we get the ordinary exception.
        EXPECT_THROW(WorkerPool::post(test_job_funcs), ErrorCode::OPERATION_FOR_STOPPED_WORKER_POOL);
    }
    return true;
}

bool test_integration_worker_pool_n_workers() {
    for (std::size_t n_workers = 1; n_workers <= test_max_n_workers; ++n_workers) {
        WorkerPool::init_and_start(n_workers);

        ASSERT_EQUAL(WorkerPool::n_workers(), n_workers);

        WorkerPool::stop();
    }

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        // Tests for worker pool behavior when jobs are executed directly on the caller's thread.
        TEST(test_integration_worker_pool_start_and_stop_caller_thread),
        TEST(test_integration_worker_pool_init_and_start_while_in_caller_thread_mode_is_noop),
        TEST(test_integration_worker_pool_post_jobs_caller_thread),
        TEST(test_integration_worker_pool_post_jobs_and_wait_caller_thread),
        TEST(test_integration_worker_pool_post_more_jobs_caller_thread),
        TEST(test_integration_worker_pool_stop_prevents_adding_new_jobs_caller_thread),
        // Tests for worker pool in async mode (workers running; jobs posted to the workers).
        TEST(test_integration_worker_pool_start_and_stop_async),
        TEST(test_integration_worker_pool_init_and_start_while_async_is_noop),
        TEST(test_integration_worker_pool_post_jobs_async),
        TEST(test_integration_worker_pool_post_jobs_and_wait_async),
        TEST(test_integration_worker_pool_post_jobs_and_wait_until_n_finished_async),
        TEST(test_integration_worker_pool_post_more_jobs_async),
        TEST(test_integration_worker_pool_stop_prevents_adding_new_jobs_async),
        TEST(test_integration_worker_pool_stop_stops_processing_jobs_async),
        // Other tests.
        TEST(test_integration_worker_pool_throwing_job_stops_pool_rethrown_on_init_and_start),
        TEST(test_integration_worker_pool_throwing_job_stops_pool_rethrown_on_post),
        TEST(test_integration_worker_pool_throwing_job_stops_pool_rethrown_on_wait),
        TEST(test_integration_worker_pool_n_workers),
    };

    return Test::run_suite("test_integration_worker_pool", tests, test_setup, test_teardown);
}
