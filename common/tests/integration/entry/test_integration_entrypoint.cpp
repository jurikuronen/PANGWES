/*
 * test_integration_entrypoint.cpp - Integration tests for entry/entrypoint.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <atomic>
#include <cstddef>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "common/entry/entrypoint.hpp"
#include "common/test_harness/Test.hpp"
#include "common/utils/Exception.hpp"
#include "common/utils/WorkerPool.hpp"

namespace PANGWES {
namespace {

constexpr auto test_n_workers = 2;
constexpr auto test_exception = "test exception";
constexpr auto test_project_options_exception = "test project options exception";

struct EntrypointTestState {
    bool project_options_constructed        = false;
    bool project_options_constructor_throws = false;

    bool help_requested                     = false;
    bool version_requested                  = false;

    bool check_options_called               = false;
    bool check_options_result               = true;

    bool print_run_details_called           = false;

    bool run_function_called                = false;
    bool run_function_throws                = false;
    bool run_function_pool_started          = false;
    bool test_job_ran                       = false;
    std::size_t run_function_n_workers      = 0;
};

EntrypointTestState test_state;

class ScopedStreamCapture {
public:
    explicit ScopedStreamCapture(std::ostream& stream)
        : m_stream{stream},
          m_capture{},
          m_original_buffer{stream.rdbuf(m_capture.rdbuf())}
    { }

    ~ScopedStreamCapture() {
        m_stream.rdbuf(m_original_buffer);
    }

    ScopedStreamCapture(const ScopedStreamCapture&) = delete;
    ScopedStreamCapture& operator=(const ScopedStreamCapture&) = delete;

    std::string contents() const {
        return m_capture.str();
    }

private:
    std::ostream& m_stream;
    std::ostringstream m_capture;
    std::streambuf* const m_original_buffer;
};

class TestOptions {
public:
    TestOptions(int argc, char** argv) {
        (void)argc;
        (void)argv;

        test_state.project_options_constructed = true;

        if (test_state.project_options_constructor_throws) {
            throw std::runtime_error{test_project_options_exception};
        }
    }

    static bool help_requested() noexcept {
        return test_state.help_requested;
    }

    static bool version_requested() noexcept {
        return test_state.version_requested;
    }

    static bool verbose() noexcept {
        return false;
    }

    static std::size_t n_workers() noexcept {
        return test_n_workers;
    }

    static void print_run_details() noexcept {
        test_state.print_run_details_called = true;
    }
};

struct EntrypointResult {
    int exit_code;
    std::string cerr;
};

bool check_test_options(const TestOptions& options) noexcept {
    (void)options;

    test_state.check_options_called = true;

    return test_state.check_options_result;
}

void run_test_function(const TestOptions& options) {
    (void)options;

    test_state.run_function_called = true;
    test_state.run_function_pool_started = WorkerPool::is_async();
    test_state.run_function_n_workers = WorkerPool::n_workers();

    if (test_state.run_function_throws) {
        throw std::runtime_error{test_exception};
    }

    WorkerPool::post({[](){ test_state.test_job_ran = true; }});
    WorkerPool::wait();
}

EntrypointResult test_run_program() {
    ScopedStreamCapture out{std::cout};
    ScopedStreamCapture err{std::cerr};

    return {Entry::run_program<TestOptions>(1, nullptr, "", check_test_options, run_test_function), err.contents()};
}

bool test_setup() {
    WorkerPool::stop();

    test_state = EntrypointTestState{};

    return true;
}

bool test_teardown() {
    WorkerPool::stop();

    return true;
}

bool test_integration_entrypoint_help_requested_returns() {
    test_state.help_requested = true;

    const auto result = test_run_program();

    ASSERT_EQUAL(result.exit_code, 0);
    ASSERT_TRUE(result.cerr.empty());

    ASSERT_TRUE(test_state.project_options_constructed);
    ASSERT_FALSE(test_state.check_options_called);
    ASSERT_FALSE(test_state.print_run_details_called);
    ASSERT_FALSE(test_state.run_function_called);
    ASSERT_TRUE(WorkerPool::stopped());

    return true;
}

bool test_integration_entrypoint_version_requested_returns() {
    test_state.version_requested = true;

    const auto result = test_run_program();

    ASSERT_EQUAL(result.exit_code, 0);
    ASSERT_TRUE(result.cerr.empty());

    ASSERT_TRUE(test_state.project_options_constructed);
    ASSERT_FALSE(test_state.check_options_called);
    ASSERT_FALSE(test_state.print_run_details_called);
    ASSERT_FALSE(test_state.run_function_called);
    ASSERT_TRUE(WorkerPool::stopped());

    return true;
}

bool test_integration_entrypoint_check_options_failure_returns() {
    test_state.check_options_result = false;

    const auto result = test_run_program();

    ASSERT_EQUAL(result.exit_code, 1);
    ASSERT_TRUE(result.cerr.empty());

    ASSERT_TRUE(test_state.project_options_constructed);
    ASSERT_TRUE(test_state.check_options_called);
    ASSERT_FALSE(test_state.print_run_details_called);
    ASSERT_FALSE(test_state.run_function_called);
    ASSERT_TRUE(WorkerPool::stopped());

    return true;
}

bool test_integration_entrypoint_runs_function_with_worker_pool() {
    const auto result = test_run_program();

    ASSERT_EQUAL(result.exit_code, 0);
    ASSERT_TRUE(result.cerr.empty());

    ASSERT_TRUE(test_state.project_options_constructed);
    ASSERT_TRUE(test_state.check_options_called);
    ASSERT_TRUE(test_state.print_run_details_called);

    ASSERT_TRUE(test_state.run_function_called);
    ASSERT_TRUE(test_state.run_function_pool_started);
    ASSERT_EQUAL(test_state.run_function_n_workers, test_n_workers);
    ASSERT_TRUE(test_state.test_job_ran);
    ASSERT_TRUE(WorkerPool::stopped());

    return true;
}

bool test_integration_entrypoint_run_function_exception_caught_and_stops_worker_pool() {
    test_state.run_function_throws = true;

    const auto result = test_run_program();

    ASSERT_EQUAL(result.exit_code, 1);
    ASSERT_EQUAL(result.cerr, std::string{test_exception} + '\n');

    ASSERT_TRUE(test_state.project_options_constructed);
    ASSERT_TRUE(test_state.check_options_called);
    ASSERT_TRUE(test_state.print_run_details_called);

    ASSERT_TRUE(test_state.run_function_called);
    ASSERT_TRUE(test_state.run_function_pool_started);
    ASSERT_EQUAL(test_state.run_function_n_workers, test_n_workers);
    ASSERT_FALSE(test_state.test_job_ran);
    ASSERT_TRUE(WorkerPool::stopped());

    return true;
}

bool test_integration_entrypoint_project_options_exception_caught() {
    test_state.project_options_constructor_throws = true;

    const auto result = test_run_program();

    ASSERT_EQUAL(result.exit_code, 1);
    ASSERT_EQUAL(result.cerr, std::string{test_project_options_exception} + '\n');

    ASSERT_TRUE(test_state.project_options_constructed);
    ASSERT_FALSE(test_state.check_options_called);
    ASSERT_FALSE(test_state.print_run_details_called);
    ASSERT_FALSE(test_state.run_function_called);
    ASSERT_TRUE(WorkerPool::stopped());

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_integration_entrypoint_help_requested_returns),
        TEST(test_integration_entrypoint_version_requested_returns),
        TEST(test_integration_entrypoint_check_options_failure_returns),
        TEST(test_integration_entrypoint_runs_function_with_worker_pool),
        TEST(test_integration_entrypoint_run_function_exception_caught_and_stops_worker_pool),
        TEST(test_integration_entrypoint_project_options_exception_caught),
    };

    return Test::run_suite("test_integration_entrypoint", tests, test_setup, test_teardown);
}
