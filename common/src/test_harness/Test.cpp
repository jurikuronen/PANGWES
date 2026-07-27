/*
 * Test.cpp - Lightweight test harness/runner.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <cstddef>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "common/test_harness/Test.hpp"

namespace PANGWES {
namespace {

// Returns a string used to indent test output by the given number of levels.
std::string test_indent(std::size_t levels) {
    return std::string(2 * levels, ' ');
}

// Pads test output with dots to produce aligned output.
std::string test_pad_with_dots(std::size_t current_length) {
    // Line length becomes 100 with three levels of indention.
    constexpr std::size_t fixed_length = 94;

    assert(fixed_length > current_length);

    return std::string(fixed_length - current_length, '.');
}

} // namespace

Test::TestResult::TestResult() noexcept
    : setup_error{},
      test_error{},
      teardown_error{},
      setup_ok{false},
      test_ok{false},
      teardown_ok{false}
{ }

bool Test::TestResult::pass() const noexcept {
    return setup_ok && test_ok && teardown_ok;
}

void Test::TestResult::print_errors() const {
    if (!setup_error.empty()) {
        std::cout << test_indent(3) << setup_error << std::endl;
    }
    if (!test_error.empty()) {
        std::cout << test_indent(3) << test_error << std::endl;
    }
    if (!teardown_error.empty()) {
        std::cout << test_indent(3) << teardown_error << std::endl;
    }
}

Test::Test(std::string test_name, TestFuncT test_func) noexcept
    : m_test_name{std::move(test_name)},
      m_test_func{std::move(test_func)}
{ }

int Test::run_suite(const std::string& test_suite_name,
                    const std::initializer_list<const Test>& suite_tests,
                    const TestFuncT& test_setup,
                    const TestFuncT& test_teardown)
{
    return run_suite(test_suite_name,
                     std::vector<Test>(suite_tests.begin(), suite_tests.end()),
                     test_setup,
                     test_teardown);
}

int Test::run_suite(const std::string& test_suite_name,
                    const std::vector<Test>& suite_tests,
                    const TestFuncT& test_setup,
                    const TestFuncT& test_teardown)
{
    auto n_test_passes = 0;

    for (const auto& test : suite_tests) {
        const auto& test_name = test.test_name();

        const auto test_result = test.run_test(test_setup, test_teardown);

        std::cout << test_indent(2) << test.test_name() << test_pad_with_dots(test_name.size())
                  << (test_result.pass() ? "ok" : "FAIL") << std::endl;

        if (test_result.pass()) {
            ++n_test_passes;
        } else {
            test_result.print_errors();
        }
    }

    std::cout << test_indent(1) << test_suite_name << " (" << n_test_passes << "/" << suite_tests.size() << ")"
              << std::endl;

    const auto success = (n_test_passes == static_cast<int>(suite_tests.size()));

    return success ? 0 : 1;
}

const std::string& Test::test_name() const {
    return m_test_name;
}

Test::TestResult Test::run_test(const TestFuncT& test_setup, const TestFuncT& test_teardown) const {
    TestResult result;

    const auto run_step = [](const TestFuncT& test_func,
                             bool& test_step_result,
                             std::string& error,
                             const std::string& test_step_context)
    {
        try {
            // Clear the global error message string. If any error occurs, it will be stored in this string.
            test_error_msg().clear();

            test_step_result = test_func();

            if (!test_step_result) {
                error = test_error_msg();
            }
        } catch (const std::exception& exception) {
            test_step_result = false;
            // Something unexpected happened in the test; report this.
            error = std::string("Unexpected ") + test_step_context + std::string(" error: ") + exception.what();
        }
    };

    run_step(test_setup, result.setup_ok, result.setup_error, "test setup");

    // Only run the actual test function if the test setup succeeded.
    if (result.setup_ok) {
        run_step(m_test_func, result.test_ok, result.test_error, "test");
    }

    run_step(test_teardown, result.teardown_ok, result.teardown_error, "test teardown");

    return result;
}

std::string& test_error_msg() {
    static std::string error_msg{};

    return error_msg;
}

} // namespace PANGWES
