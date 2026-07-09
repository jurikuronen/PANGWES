/*
 * WorkerPool.hpp - A global worker pool that enables posting jobs for worker threads.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <cassert>
#include <utility>

#include "common/io/Log.hpp"
#include "common/utils/WorkerPool.hpp"

namespace PANGWES {

// Initialize static member variables.
std::vector<std::thread> WorkerPool::s_workers{};
std::deque<JobT> WorkerPool::s_jobs{};
std::mutex WorkerPool::s_mutex{};
std::condition_variable WorkerPool::s_mutex_condition{};
std::condition_variable WorkerPool::s_done_condition{};
std::atomic<std::size_t> WorkerPool::s_unfinished_jobs{0};
std::atomic<WorkerPool::pool_state_t>
WorkerPool::s_pool_state{static_cast<pool_state_t>(WorkerPool::PoolState::STOPPED)};
std::exception_ptr WorkerPool::s_stored_exception{};
std::size_t WorkerPool::s_unfinished_jobs_wait_limit{0};

void WorkerPool::init_and_start(std::size_t n_workers) {
    if (!stopped()) {
        return;
    }

    /*
     * The pool might be in a "soft stopped" state because a worker caught an exception and having a stored exception
     * means that the user has not been informed of this yet. In that case, we must properly stop the pool and rethrow
     * the exception here.
    */
    if (s_stored_exception) {
        // Stop properly to join workers.
        stop();

        const auto stored_exception = std::move(s_stored_exception);

        s_stored_exception = std::exception_ptr{};
        std::rethrow_exception(stored_exception);
    }

    if (n_workers == 0) {
        s_pool_state = static_cast<pool_state_t>(PoolState::CALLER_THREAD);

        return;
    }

    s_pool_state = static_cast<pool_state_t>(PoolState::ASYNC);

    const auto worker_loop = []() {
        while (true) {
            // Returns an empty job if the pool was stopped.
            const auto job = get_next_job();

            if (!job) {
                return;
            }

            try {
                job();
            } catch(...) {
                trigger_stop(std::current_exception());
                return;
            }

            std::lock_guard<std::mutex> lock(s_mutex);

            // If pool was stopped while running, exit without modifying unfinished jobs.
            if (stopped()) {
                return;
            }

            assert(s_unfinished_jobs.load() > 0 && "finishing a job with job tracking already at 0 unfinished jobs");
            --s_unfinished_jobs;

            if (s_unfinished_jobs.load() <= s_unfinished_jobs_wait_limit) {
                s_done_condition.notify_all();
            }
        }
    };

    for (std::size_t i = 0; i < n_workers; ++i) {
        s_workers.emplace_back(worker_loop);
    }
}

void WorkerPool::stop() noexcept {
    std::unique_lock<std::mutex> lock(s_mutex);

    s_pool_state = static_cast<pool_state_t>(PoolState::STOPPED);
    s_mutex_condition.notify_all();

    lock.unlock();

    for (auto& worker : s_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    s_workers.clear();
    s_jobs.clear();
    s_unfinished_jobs = 0;
    s_done_condition.notify_all();
}

std::size_t WorkerPool::n_workers() noexcept {
    return s_workers.size();
}

std::size_t WorkerPool::unfinished_jobs() noexcept {
    return s_unfinished_jobs.load();
}

bool WorkerPool::stopped() noexcept {
    return s_pool_state.load() == static_cast<pool_state_t>(PoolState::STOPPED);
}

bool WorkerPool::is_async() noexcept {
    return s_pool_state.load() == static_cast<pool_state_t>(PoolState::ASYNC);
}

void WorkerPool::wait() {
    wait_until_unfinished_jobs_at_most(0);
}

void WorkerPool::wait_until_unfinished_jobs_at_most(std::size_t n) {
    std::unique_lock<std::mutex> lock(s_mutex);

    if (n > 0) {
        // Signal worker loop to notify at `s_unfinished_jobs <= s_unfinished_jobs_wait_limit`.
        s_unfinished_jobs_wait_limit = n;
    }

    s_done_condition.wait(lock, [n](){ return stopped() || s_unfinished_jobs.load() <= n; });

    if (n > 0) {
        // Reset wait limit to zero to avoid unnecessary notifies.
        s_unfinished_jobs_wait_limit = 0;
    }

    const auto stored_exception = s_stored_exception;
    s_stored_exception = std::exception_ptr{};

    // If there was an exception, the pool was put into a "soft stopped" state. Stop properly and rethrow the exception.
    if (stored_exception) {
        // Must be unlocked before calling stop() to prevent a deadlock.
        lock.unlock();
        stop();
        std::rethrow_exception(stored_exception);
    }
}

JobT WorkerPool::get_next_job() noexcept {
    std::unique_lock<std::mutex> lock(s_mutex);

    s_mutex_condition.wait(lock, [](){ return stopped() || !s_jobs.empty(); });

    if (stopped()) {
        return {};
    }

    const auto job = std::move(s_jobs.front());
    s_jobs.pop_front();

    return job;
}

void WorkerPool::trigger_stop(std::exception_ptr exception_ptr) noexcept {
    std::unique_lock<std::mutex> lock(s_mutex);

    if (!s_stored_exception) {
        s_stored_exception = std::move(exception_ptr);
        Log::out_without_date_block() << "FATAL ERROR: A worker thread caught an exception." << std::endl;
    }

    s_pool_state = static_cast<pool_state_t>(PoolState::STOPPED);

    // Safe because worker loop returns on stopped state before further modifying s_unfinished_jobs.
    s_unfinished_jobs = 0;
    s_jobs.clear();

    lock.unlock();

    s_mutex_condition.notify_all();
    s_done_condition.notify_all();
}

} // namespace PANGWES
