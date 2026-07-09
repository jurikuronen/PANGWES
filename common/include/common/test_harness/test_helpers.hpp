/*
 * test_helpers.hpp - Helper macros to verify test assertions used by the test harness.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cassert>
#include <cmath>
#include <cstdint>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>

#include "common/test_harness/test_type_traits.hpp"
#include "common/type_traits/type_traits.hpp"

/*
 * Helper macros to verify test assertions and trigger an early exit in case of an assertion failure. Suitable for
 * `TestFuncT` type functions (see Test.hpp).
 *
 * These checks immediately return `false` from the current test function and store information about the encountered
 * error in the global string returned by `test_error_msg()` (see Test.hpp).
 *
 * The following checks are provided:
 *
 * Boolean checks for conditions convertible to a Boolean:
 * - ASSERT_TRUE(condition)
 * - ASSERT_FALSE(condition)
 *
 * Comparison operators for arithmetic and string-like operands:
 * - ASSERT_EQUAL(a, b)
 * - ASSERT_NOT_EQUAL(a, b)
 * - ASSERT_GREATER(a, b)
 * - ASSERT_GREATER_EQUAL(a, b)
 * - ASSERT_LESS(a, b)
 * - ASSERT_LESS_EQUAL(a, b)
 * Note: floating-point comparisons use a fixed tolerance of 1e-6.
 *
 * Comparison operator for enum types:
 * - ASSERT_ENUMS_EQUAL(a, b)
 *
 * Comparison operator for containers:
 * - ASSERT_CONTAINERS_EQUAL(a, b)
 *
 * Throw assertion for expressions:
 * - EXPECT_THROW(expression, expected_error_code)
 * Note: this calls `(void)(expression)` and expects `Utils::Exception(expected_error_code)` to be thrown.
 *
 * Notes:
 * - The asserted values must be streamable to `std::ostream` for diagnostics.
 * - Don't pass expressions with side effects (e.g. `ASSERT_EQUAL(i++, 3)`).
*/

// Builds an error message by streaming an expression into a temporary std::ostringstream.
#define BUILD_ERROR_MSG(stream_expr_) \
    ([&](){ std::ostringstream oss; oss << stream_expr_; return oss.str(); }())

/*
 * Triggers an early exit for the current test and saves the error message, given as a stream expression.
 *
 * The function `test_error_msg()` is defined in Test.hpp and returns a static global string.
*/
#define EXIT_TEST_WITH_FAILURE(error_msg_stream_expr_) \
    do { \
        test_error_msg() = BUILD_ERROR_MSG(error_msg_stream_expr_ << " (" << __FILE__ << ":" << __LINE__ << ")"); \
        return false; \
    } while(0)

// Fails the test if the condition evaluates to true.
#define ASSERT_FALSE(condition_) \
    do { \
        if (test_helper_assert_true((condition_))) { \
            EXIT_TEST_WITH_FAILURE("[" #condition_ "]" << " is not false"); \
        } \
    } while (0)

// Fails the test if the condition evaluates to false.
#define ASSERT_TRUE(condition_) \
    do { \
        if (!test_helper_assert_true((condition_))) { \
            EXIT_TEST_WITH_FAILURE("[" #condition_ "]" << " is not true"); \
        } \
    } while (0)

// Fails the test if the two values are not equal.
#define ASSERT_EQUAL(a_, b_) \
    do { \
        if (!test_helper_assert_equal((a_), (b_))) { \
            EXIT_TEST_WITH_FAILURE("[" #a_ " == " #b_ << "] failed with values [" << (a_) << " == " << (b_) << "]"); \
        } \
    } while (0)

// Fails the test if the two values are equal.
#define ASSERT_NOT_EQUAL(a_, b_) \
    do { \
        if (test_helper_assert_equal((a_), (b_))) { \
            EXIT_TEST_WITH_FAILURE("[" #a_ " != " #b_ << "] failed with values [" << (a_) << " != " << (b_) << "]"); \
        } \
    } while (0)

// Fails the test if the two enums are not equal.
#define ASSERT_ENUMS_EQUAL(a_, b_) \
    do { \
        static_assert(std::is_enum<Traits::decay_t<decltype(a_)>>::value, \
                      "ASSERT_ENUMS_EQUAL: " #a_ " must be an enum"); \
        static_assert(std::is_enum<Traits::decay_t<decltype(b_)>>::value, \
                      "ASSERT_ENUMS_EQUAL: " #b_ " must be an enum"); \
        static_assert(std::is_same<Traits::decay_t<decltype(a_)>, Traits::decay_t<decltype(b_)>>::value, \
                      "ASSERT_ENUMS_EQUAL: " #a_ " and " #b_ " must be of the same type"); \
        if (!((a_) == (b_))) { \
            EXIT_TEST_WITH_FAILURE( \
                "[" #a_ " == " #b_ << "] failed with values " << \
                "[" << test_helper_to_string(a_) << " == " << test_helper_to_string(b_) << "]" \
            ); \
        } \
    } while (0)

// Fails the test if the two containers are not equal, meaning they contain the same elements.
#define ASSERT_CONTAINERS_EQUAL(a_, b_) \
    do { \
        using a_type_ = typename Traits::decay_t<decltype(a_)>; \
        using b_type_ = typename Traits::decay_t<decltype(b_)>; \
        static_assert(Traits::is_container<a_type_>::value, \
                      "ASSERT_CONTAINERS_EQUAL: " #a_ " must be a container"); \
        static_assert(Traits::is_container<b_type_>::value, \
                      "ASSERT_CONTAINERS_EQUAL: " #b_ " must be a container"); \
        static_assert(std::is_same<a_type_, b_type_>::value, \
                      "ASSERT_CONTAINERS_EQUAL: " #a_ " and " #b_ " must be of the same type"); \
        const std::pair<int64_t, int64_t> sizes_ = test_helper_container_sizes((a_), (b_)); \
        if (sizes_.first != sizes_.second) { \
            EXIT_TEST_WITH_FAILURE( \
                "[" #a_ " == " #b_ << "] failed because the containers were of different sizes [" << \
                sizes_.first << " == " << sizes_.second << "]" \
            ); \
        } \
        const auto result = test_helper_assert_container_equal((a_), (b_)); \
        const auto index_signed_ = std::get<0>(result); \
        const auto a_at_index_ = std::get<1>(result); \
        const auto b_at_index_ = std::get<2>(result); \
        if (index_signed_ >= 0) { \
            EXIT_TEST_WITH_FAILURE( \
                "[" #a_ " == " #b_ << "] failed at index " << index_signed_ << " with values " << \
                test_helper_to_string(a_at_index_) << " == " << \
                test_helper_to_string(b_at_index_) << "]" \
            ); \
        } \
    } while (0)

// Fails the test if the first value is not greater than the second value.
#define ASSERT_GREATER(a_, b_) \
    do { \
        if (!test_helper_assert_greater((a_), (b_))) { \
            EXIT_TEST_WITH_FAILURE("[" #a_ " > " #b_ << "] failed with values [" << (a_) << " > " << (b_) << "]"); \
        } \
    } while (0)

// Fails the test if the first value is not greater or equal than the second value.
#define ASSERT_GREATER_EQUAL(a_, b_) \
    do { \
        if (!test_helper_assert_greater_equal((a_), (b_))) { \
            EXIT_TEST_WITH_FAILURE("[" #a_ " >= " #b_ << "] failed with values [" << (a_) << " >= " << (b_) << "]"); \
        } \
    } while (0)

// Fails the test if the first value is not less than the second value.
#define ASSERT_LESS(a_, b_) \
    do { \
        if (!test_helper_assert_less((a_), (b_))) { \
            EXIT_TEST_WITH_FAILURE("[" #a_ " < " #b_ << "] failed with values [" << (a_) << " < " << (b_) << "]"); \
        } \
    } while (0)

// Fails the test if the first value is not less or equal than the second value.
#define ASSERT_LESS_EQUAL(a_, b_) \
    do { \
        if (!test_helper_assert_less_equal((a_), (b_))) { \
            EXIT_TEST_WITH_FAILURE("[" #a_ " <= " #b_ << "] failed with values [" << (a_) << " <= " << (b_) << "]"); \
        } \
    } while (0)

// Expects the provided function call expression to throw and fails if not.
#define EXPECT_THROW(expression_, expected_exception_error_code_) \
    do { \
        try { \
            (void)(expression_); \
            EXIT_TEST_WITH_FAILURE("Expected " #expression_ " to throw an exception"); \
        } catch(const Exception& exception) { \
            ASSERT_ENUMS_EQUAL(exception.error_code(), expected_exception_error_code_); \
        } catch(const std::exception& exception) { \
            EXIT_TEST_WITH_FAILURE(#expression_ << " threw an unexpected exception: " << exception.what()); \
        } \
    } while (0)

namespace PANGWES {

// Converts the provided T to a string via `to_string(T)` when available.
template <typename T, Traits::enable_if_t<TestTraits::has_to_string<T>::value>* = nullptr>
std::string test_helper_to_string(const T& type) {
    // Enable ADL to find std::to_string.
    using std::to_string;
    return to_string(type);
}

// Converts the provided enum to a string via underlying integral value as fallback.
template <typename Enum, Traits::enable_if_t<!TestTraits::has_to_string<Enum>::value &&
                                             std::is_enum<Enum>::value>* = nullptr>
std::string test_helper_to_string(const Enum& enum_value) {
    return std::to_string(static_cast<int>(Traits::to_underlying(enum_value)));
}

// Fallback for types not convertible to a string.
template <typename T, Traits::enable_if_t<!TestTraits::has_to_string<T>::value && !std::is_enum<T>::value>* = nullptr>
std::string test_helper_to_string(const T& type) {
    (void)type;

    return "not displayable";
}

// Asserts the provided condition is true with explicit conversion to Boolean.
template <typename Condition>
Traits::enable_if_t<std::is_convertible<Condition, bool>::value, bool>
test_helper_assert_true(const Condition& condition) {
    return static_cast<bool>(condition);
}

// Asserts that two integral operands are equal.
template <typename LHS, typename RHS>
Traits::enable_if_t<std::is_integral<LHS>::value && std::is_integral<RHS>::value, bool>
test_helper_assert_equal(const LHS& lhs, const RHS& rhs) {
    if ((std::is_signed<LHS>::value && std::is_unsigned<RHS>::value && static_cast<int64_t>(lhs) < 0) ||
        (std::is_unsigned<LHS>::value && std::is_signed<RHS>::value && static_cast<int64_t>(rhs) < 0))
    {
        return false;
    }

    return static_cast<std::size_t>(lhs) == static_cast<std::size_t>(rhs);
}

// Asserts that two arithmetic operands with floating-point types are equal within a tolerance.
template <typename LHS, typename RHS>
Traits::enable_if_t<(std::is_arithmetic<LHS>::value && std::is_arithmetic<RHS>::value) &&
            (std::is_floating_point<LHS>::value || std::is_floating_point<RHS>::value), bool>
test_helper_assert_equal(const LHS& lhs, const RHS& rhs) {
    constexpr auto tolerance = 1e-6;

    return std::abs(static_cast<double>(lhs) - static_cast<double>(rhs)) < tolerance;
}

// Asserts that two string-like operands are equal.
template <typename LHS, typename RHS>
Traits::enable_if_t<TestTraits::is_string_like<LHS>::value && TestTraits::is_string_like<RHS>::value, bool>
test_helper_assert_equal(const LHS& lhs, const RHS& rhs) {
    return lhs == rhs;
}

// Returns the sizes of the two provided containers.
template <typename LHS, typename RHS>
Traits::enable_if_t<Traits::is_container<LHS>::value && Traits::is_container<RHS>::value, std::pair<int64_t, int64_t>>
test_helper_container_sizes(const LHS& lhs, const RHS& rhs) {
    return std::make_pair(lhs.size(), rhs.size());
}

// Asserts that two containers are equal.
template <typename LHS, typename RHS>
Traits::enable_if_t<Traits::is_container<LHS>::value && Traits::is_container<RHS>::value,
                        std::tuple<int64_t, Traits::element_type_t<const LHS>, Traits::element_type_t<const RHS>>>
test_helper_assert_container_equal(const LHS& lhs, const RHS& rhs) {
    assert(lhs.size() == rhs.size() && "unexpected different sizes for containers");

    for (int64_t i = 0; i < static_cast<int64_t>(lhs.size()); ++i) {
        if (lhs[i] != rhs[i]) {
            // Signals failure at index i.
            return std::make_tuple(i, lhs[i], rhs[i]);
        }
    }

    // Hard-coded value indicating success.
    return std::make_tuple(int64_t{-1}, Traits::element_type_t<const LHS>{}, Traits::element_type_t<const RHS>{});
}

// Asserts that the first provided integral operand is greater than the second.
template <typename LHS, typename RHS>
Traits::enable_if_t<std::is_integral<LHS>::value && std::is_integral<RHS>::value, bool>
test_helper_assert_greater(const LHS& lhs, const RHS& rhs) {
    if ((std::is_signed<LHS>::value && std::is_unsigned<RHS>::value && static_cast<int64_t>(lhs) < 0)) {
        return false;
    }
    if ((std::is_unsigned<LHS>::value && std::is_signed<RHS>::value && static_cast<int64_t>(rhs) < 0)) {
        return true;
    }

    return static_cast<std::size_t>(lhs) > static_cast<std::size_t>(rhs);
}

/*
 * Asserts that the first provided arithmetic operand with floating-point type is greater than the second by a
 * tolerance.
*/
template <typename LHS, typename RHS>
Traits::enable_if_t<(std::is_arithmetic<LHS>::value && std::is_arithmetic<RHS>::value) &&
            (std::is_floating_point<LHS>::value || std::is_floating_point<RHS>::value), bool>
test_helper_assert_greater(const LHS& lhs, const RHS& rhs) {
    constexpr auto tolerance = 1e-6;

    return static_cast<double>(lhs) - static_cast<double>(rhs) >= tolerance;
}

// Asserts that the first provided operand is greater than or equal to the second.
template <typename LHS, typename RHS>
bool test_helper_assert_greater_equal(const LHS& lhs, const RHS& rhs) {
    return test_helper_assert_greater(lhs, rhs) || test_helper_assert_equal(lhs, rhs);
}

// Asserts that the first provided operand is less than the second.
template <typename LHS, typename RHS>
bool test_helper_assert_less(const LHS& lhs, const RHS& rhs) {
    return test_helper_assert_greater(rhs, lhs);
}

// Asserts that the first provided operand is less than or equal to the second.
template <typename LHS, typename RHS>
bool test_helper_assert_less_equal(const LHS& lhs, const RHS& rhs) {
    return test_helper_assert_greater_equal(rhs, lhs);
}

} // namespace PANGWES
