/*
 * test_integration_log.cpp - Integration tests for io/Log.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <algorithm>

#include "common/test_harness/Test.hpp"
#include "common/utils/WorkerPool.hpp"
#include "mocks/MockLog.hpp"

namespace PANGWES {
namespace {

using Mocks::MockLog;
using Mocks::MockOStream;

constexpr auto n_jobs = 1000;
constexpr auto n_threads = 8;

std::string get_mock_stream_contents_no_newlines() {
    auto contents = MockOStream::contents();

    // Remove '\r' and '\n' characters.
    contents.erase(std::remove(contents.begin(), contents.end(), '\r'), contents.end());
    contents.erase(std::remove(contents.begin(), contents.end(), '\n'), contents.end());

    return contents;
}

bool test_setup() {
    // Enable logging.
    MockLog::set_verbose(true);

    // Clean slate.
    MockOStream::clear();

    // Start worker pool.
    WorkerPool::stop();
    WorkerPool::init_and_start(n_threads);

    return true;
}

bool test_teardown() {
    WorkerPool::stop();

    return true;
}

bool check_logged_contents_equals(const std::string& expected_contents) {
    const auto contents_no_newlines = get_mock_stream_contents_no_newlines();

    ASSERT_FALSE(contents_no_newlines.empty());
    ASSERT_EQUAL(contents_no_newlines, expected_contents);

    return true;
}

bool check_logged_contents_contains_all(const std::vector<std::string>& expected_contents) {
    const auto contents_no_newlines = get_mock_stream_contents_no_newlines();

    ASSERT_FALSE(contents_no_newlines.empty());

    for (const auto& expected_content : expected_contents) {
        const auto pos = contents_no_newlines.find(expected_content);

        ASSERT_FALSE(pos == std::string::npos);
    }

    return true;
}

bool check_logged_contents_contains_none(const std::vector<std::string>& expected_not_contained) {
    const auto contents_no_newlines = get_mock_stream_contents_no_newlines();

    for (const auto& expected_not_to_contain : expected_not_contained) {
        const auto pos = contents_no_newlines.find(expected_not_to_contain);

        ASSERT_TRUE(pos == std::string::npos);
    }

    return true;
}

bool test_integration_log_multithreaded() {
    const std::string test_str = "test";

    std::string expected_contents_no_newlines{};

    for (auto job = 0; job < n_jobs; ++job) {
        expected_contents_no_newlines += test_str;

        WorkerPool::post({[test_str](){ MockLog::out_without_date_block() << test_str << std::endl; }});
    }

    WorkerPool::wait();

    return check_logged_contents_equals(expected_contents_no_newlines);
}

bool test_integration_log_multithreaded_post_all_jobs_at_once() {
    const std::string test_str = "test";

    std::string expected_contents_no_newlines;
    std::vector<JobT> jobs;

    jobs.reserve(n_jobs);

    for (auto job = 0; job < n_jobs; ++job) {
        expected_contents_no_newlines += test_str;

        jobs.push_back([test_str](){ MockLog::out_without_date_block() << test_str << std::endl; });
    }

    WorkerPool::post(jobs);

    WorkerPool::wait();

    return check_logged_contents_equals(expected_contents_no_newlines);
}

bool test_integration_log_chaining_multithreaded() {
    std::vector<std::string> expected_contents;

    expected_contents.reserve(n_jobs);

    for (auto job = 0; job < n_jobs; ++job) {
        expected_contents.push_back("test[" + std::to_string(job) + "]");

        WorkerPool::post({[job](){ MockLog::out_without_date_block() << "test[" << job << "]" << std::endl; }});
    }

    WorkerPool::wait();

    return check_logged_contents_contains_all(expected_contents);
}

bool test_integration_log_chaining_multithreaded_post_all_jobs_at_once() {
    std::vector<JobT> jobs;
    std::vector<std::string> expected_contents;

    jobs.reserve(n_jobs);
    expected_contents.reserve(n_jobs);

    for (auto job = 0; job < n_jobs; ++job) {
        expected_contents.push_back("test[" + std::to_string(job) + "]");

        jobs.push_back([job](){ MockLog::out_without_date_block() << "test[" << job << "]" << std::endl; });
    }

    WorkerPool::post(jobs);

    WorkerPool::wait();

    return check_logged_contents_contains_all(expected_contents);
}

bool test_integration_log_only_std_endl_flushes_multithreaded() {
    constexpr auto flush_job = 20;

    std::vector<std::string> expected_not_contained;
    const auto expected_contents = "test[" + std::to_string(flush_job) + "]";

    for (auto job = 0; job < n_jobs; ++job) {
        if (job > flush_job) {
            expected_not_contained.push_back("test[" + std::to_string(job) + "]");
        }

        WorkerPool::post({[job](){
            MockLog::out_without_date_block() << "test[" << job << "]";
            if (job == flush_job) {
                MockLog::out_without_date_block() << std::endl;
            }
        }});
    }

    WorkerPool::wait();

    return check_logged_contents_contains_all({expected_contents}) &&
           check_logged_contents_contains_none(expected_not_contained);
}

bool test_integration_log_only_std_endl_flushes_multithreaded_post_all_jobs_at_once() {
    constexpr auto flush_job = 20;

    std::vector<JobT> jobs;
    std::vector<std::string> expected_not_contained;
    const auto expected_contents = "test[" + std::to_string(flush_job) + "]";

    jobs.reserve(n_jobs);

    for (auto job = 0; job < n_jobs; ++job) {
        if (job > flush_job) {
            expected_not_contained.push_back("test[" + std::to_string(job) + "]");
        }

        jobs.push_back([job](){
            MockLog::out_without_date_block() << "test[" << job << "]";
            if (job == flush_job) {
                MockLog::out_without_date_block() << std::endl;
            }
        });
    }

    WorkerPool::post(jobs);

    WorkerPool::wait();

    return check_logged_contents_contains_all({expected_contents}) &&
           check_logged_contents_contains_none(expected_not_contained);
}

bool test_integration_log_verbose_false_multithreaded() {
    MockLog::set_verbose(false);

    for (auto job = 0; job < n_jobs; ++job) {
        WorkerPool::post({[](){ MockLog::out_without_date_block() << "test" << std::endl; }});
    }

    WorkerPool::wait();

    ASSERT_TRUE(MockOStream::contents().empty());

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_integration_log_multithreaded),
        TEST(test_integration_log_multithreaded_post_all_jobs_at_once),
        TEST(test_integration_log_chaining_multithreaded),
        TEST(test_integration_log_chaining_multithreaded_post_all_jobs_at_once),
        TEST(test_integration_log_only_std_endl_flushes_multithreaded),
        TEST(test_integration_log_only_std_endl_flushes_multithreaded_post_all_jobs_at_once),
        TEST(test_integration_log_verbose_false_multithreaded),
    };

    return Test::run_suite("test_integration_log", tests, test_setup, test_teardown);
}
