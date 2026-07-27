/*
 * WorkerPool.hpp - A global worker pool that enables posting jobs for worker threads.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <exception>
#include <functional>
#include <initializer_list>
#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>

#include "common/utils/Exception.hpp"
#include "common/type_traits/type_traits.hpp"

namespace PANGWES {

using JobT = std::function<void()>;

// Global (static) worker pool.
class WorkerPool {
private:
    enum class PoolState : std::uint8_t {
        // Not running, all threads joined, no longer accepting new jobs (default value).
        STOPPED,
        // Jobs executed on the calling thread.
        CALLER_THREAD,
        // Jobs executed on worker threads asynchronously.
        ASYNC,
    };

    using pool_state_t = typename Traits::underlying_type_t<PoolState>;

public:
    WorkerPool() = delete;

    // Initializes the pool and starts workers or enables CALLER_THREAD state. No-op when the pool is not STOPPED.
    static void init_and_start(std::size_t n_workers);

    // Stops the pool and joins workers. Safe to call multiple times.
    static void stop() noexcept;

    // Returns the number of worker threads used by the pool.
    static std::size_t n_workers() noexcept;

    // Returns the number of unfinished jobs. In CALLER_THREAD state this value is always 0.
    static std::size_t unfinished_jobs() noexcept;

    // Returns true if the pool is stopped.
    static bool stopped() noexcept;

    // Returns true if the pool is in async state.
    static bool is_async() noexcept;

    // Blocks the calling thread until all jobs are complete or the pool stops. Rethrows the first captured exception.
    static void wait();

    /*
     * Blocks until the number of unfinished jobs in the pool is at most `n`.
     *
     * Supports only one waiter waiting with this function at a time.
     *
     * Rethrows the first captured exception.
    */
    static void wait_until_unfinished_jobs_at_most(std::size_t n);

    /*
     * Posts jobs for execution.
     * - STOPPED: throws.
     * - CALLER_THREAD: job executed on the calling thread.
     * - ASYNC: enqueues jobs and wakes workers.
    */
    template <typename JobsContainer = std::initializer_list<JobT>>
    static void post(const JobsContainer& jobs) {
        static_assert(std::is_same<typename Traits::decay_t<typename JobsContainer::value_type>, JobT>::value,
                      "JobsContainer::value_type must be JobT");

        // Investigate the cause for the pool to be stopped to throw the correct exception type.
        const auto rethrow_stored_exception_or_throw_stopped_pool_exception = [](std::unique_lock<std::mutex>& lock) {
            const auto stored_exception = s_stored_exception;

            /*
             * If there's a stored exception, it means that some worker caught an exception and the pool was put into a
             * "soft stopped" state. Stop properly and rethrow the exception.
            */
            if (stored_exception) {
                s_stored_exception = std::exception_ptr{};

                // Must be unlocked before calling stop() to prevent a deadlock.
                lock.unlock();
                stop();

                std::rethrow_exception(stored_exception);
            }

            // Otherwise, we throw the ordinary exception for post() called on stopped worker pool.
            throw Exception(ErrorCode::OPERATION_FOR_STOPPED_WORKER_POOL, "post()");
        };

        if (stopped()) {
            std::unique_lock<std::mutex> lock(s_mutex);
            rethrow_stored_exception_or_throw_stopped_pool_exception(lock);
        }

        if (s_pool_state == static_cast<pool_state_t>(PoolState::CALLER_THREAD)) {
            // Execute jobs immediately on the calling thread.
            for (const auto& job : jobs) {
                job();
            }

            return;
        }

        std::unique_lock<std::mutex> lock(s_mutex);

        // Need to re-check here as previously we were not under the lock.
        if (stopped()) {
            rethrow_stored_exception_or_throw_stopped_pool_exception(lock);
        }

        // Post jobs to workers.
        s_unfinished_jobs += jobs.size();
        s_jobs.insert(s_jobs.end(), jobs.begin(), jobs.end());

        lock.unlock();

        s_mutex_condition.notify_all();
    }

private:
    static std::vector<std::thread> s_workers;
    static std::deque<JobT> s_jobs;
    static std::mutex s_mutex;
    static std::condition_variable s_mutex_condition;
    static std::condition_variable s_done_condition;
    static std::atomic<std::size_t> s_unfinished_jobs;
    static std::atomic<pool_state_t> s_pool_state;
    static std::exception_ptr s_stored_exception;
    static std::size_t s_unfinished_jobs_wait_limit;

    // Gets the next job from the jobs queue. Returns an empty job if STOPPED.
    static JobT get_next_job() noexcept;

    /*
     * Called by one of the workers when catching an exception. Stops the pool and unblocks waiters.
     *
     * Stores the first exception caught by any worker.
     *
     * In this state, calling post() or wait() joins workers and rethrows the exception.
    */
    static void trigger_stop(std::exception_ptr exception_ptr) noexcept;
};

} // namespace PANGWES
