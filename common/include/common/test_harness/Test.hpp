/*
 * Test.hpp - Lightweight test harness/runner.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <functional>
#include <initializer_list>
#include <string>
#include <vector>

#include "common/architecture/architecture.hpp"
#include "common/test_harness/test_helpers.hpp"

/*
 * This file provides a lightweight test harness/runner for running test cases.
 *
 * Use the helper macro `TEST(test_func)` to construct test cases:
 *     const auto tests = {
 *         TEST(test_foo),
 *         // ...
 *         TEST(test_bar),
 *     };
 *
 * And run with:
 *     Test::run_suite("suite_name", tests);
 *
 * This prints a list of run tests and their result (ok/FAIL) in a formatted manner. In case of a failing test, an
 * additional error message will be printed. Finally, a summary result (ok/FAIL) for the whole suite will be printed.
 *
 * For writing test cases, the following assertion helpers are provided (see test_helpers.hpp for details):
 * - ASSERT_TRUE(condition)
 * - ASSERT_FALSE(condition)
 * - ASSERT_EQUAL(a, b)
 * - ASSERT_NOT_EQUAL(a, b)
 * - ASSERT_GREATER(a, b)
 * - ASSERT_GREATER_EQUAL(a, b)
 * - ASSERT_LESS(a, b)
 * - ASSERT_LESS_EQUAL(a, b)
 * - ASSERT_ENUMS_EQUAL(a, b)
 * - ASSERT_CONTAINERS_EQUAL(a, b)
 * - EXPECT_THROW(expression, expected_error_code)
*/

// Helper macro that should be used for constructing a Test object from a test function.
#define TEST(test_func_) Test(#test_func_, static_cast<TestFuncT>(test_func_))

namespace PANGWES {

// Function type for a test case, setup or teardown. The function should return a Boolean and take no arguments.
using TestFuncT = std::function<bool()>;

// Test case class consisting of the test's name and the test function.
class Test {
public:
    // Constructs a test case with the given name and test function.
    Test(std::string test_name, TestFuncT test_func) noexcept;

    /*
     * Runs a test suite and reports results to stdout.
     *
     * The tests may have a shared test setup and/or a test teardown function.
     *
     * Returns 0 if all tests pass, 1 otherwise.
    */
    static int run_suite(const std::string& test_suite_name,
                         const std::vector<Test>& suite_tests,
                         const TestFuncT& test_setup = [](){ return true; },
                         const TestFuncT& test_teardown = [](){ return true; });

    // Compatibility overload for older GCC versions which misdeduce `const auto tests = { ... }`.
    static int run_suite(const std::string& test_suite_name,
                         const std::initializer_list<const Test>& suite_tests,
                         const TestFuncT& test_setup = [](){ return true; },
                         const TestFuncT& test_teardown = [](){ return true; });


private:
    const std::string m_test_name;
    const TestFuncT m_test_func;

    // Returns the name of the test (usually the same as the test function).
    const std::string& test_name() const;

    /*
     * Test result structure consisting of statuses and possible errors for all test steps (setup, test function and
     * teardown).
    */
    struct TestResult {
        std::string setup_error;
        std::string test_error;
        std::string teardown_error;
        bool setup_ok;
        bool test_ok;
        bool teardown_ok;

        // Default constructor: sets strings to empty and Booleans to false.
        TestResult() noexcept;

        /*
         * Returns true if the test passed. A test is considered passed if all test steps (setup, test function and
         * teardown) were run successfully.
        */
        bool pass() const noexcept;

        // Prints any stored errors for each test step.
        void print_errors() const;
    };

    /*
     * Clear any previous error message and run the following steps:
     * 1) execute the test setup function.
     * 2) execute the stored test function (only in case the test setup succeeded).
     * 3) execute the test teardown function.
     *
     * Returns a test result object that provides `bool pass()` and `void print_errors()` utilities.
    */
    TestResult run_test(const TestFuncT& test_setup, const TestFuncT& test_teardown) const;
};

// Access to the global test error string.
std::string& test_error_msg();

} // namespace PANGWES
