/*
 * test_unit_cigar.cpp - Unit tests for core/CIGAR.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <algorithm>
#include <cstddef>
#include <limits>
#include <string>
#include <vector>

#include "common/test_harness/Test.hpp"
#include "common/utils/Exception.hpp"
#include "gfa_parser/core/CIGAR.hpp"

namespace PANGWES {
namespace {

// Constants for maximum possible CIGAR operation length before unsigned integer overflow.
constexpr auto cigar_operation_max_length = std::numeric_limits<std::size_t>::max();
const auto cigar_operation_max_length_str = std::to_string(cigar_operation_max_length);
const auto cigar_operation_max_length_prefix = std::to_string(cigar_operation_max_length / 10);

const std::vector<char> unsupported_operations{'I', 'D', 'N', 'S', 'H', 'P', 'X'};
const char unavailable_operation_char = '*';

// After constructing `CIGAR` from `cigar_string`, check expected overlap and exact sequence match operation.
bool check_cigar_result(const std::string& cigar_string,
                        std::size_t expected_overlap,
                        bool expected_is_exact_sequence_match_operation)
{
    const auto cigar = CIGAR(cigar_string);

    ASSERT_EQUAL(cigar.overlap(), expected_overlap);
    ASSERT_EQUAL(cigar.is_exact_sequence_match_operation(), expected_is_exact_sequence_match_operation);

    return true;
}

bool test_unit_cigar_single_M_operation() {
    return check_cigar_result("0M", 0, false) && check_cigar_result("2M", 2, false) &&
           check_cigar_result("60M", 60, false) && check_cigar_result("00090M", 90, false);
}

bool test_unit_cigar_multiple_M_operations_throws() {
    EXPECT_THROW(CIGAR("0M0M"), ErrorCode::INVALID_DATA);
    EXPECT_THROW(CIGAR("2M3M"), ErrorCode::INVALID_DATA);
    EXPECT_THROW(CIGAR("60M3000M"), ErrorCode::INVALID_DATA);
    EXPECT_THROW(CIGAR("0M30M00M100M0090M"), ErrorCode::INVALID_DATA);

    return true;
}

bool test_unit_cigar_single_equals_operation() {
    return check_cigar_result("0=", 0, true) && check_cigar_result("2=", 2, true) &&
           check_cigar_result("60=", 60, true) && check_cigar_result("00090=", 90, true);
}

bool test_unit_cigar_multiple_equals_operations_throws() {
    EXPECT_THROW(CIGAR("0=0="), ErrorCode::INVALID_DATA);
    EXPECT_THROW(CIGAR("2=3="), ErrorCode::INVALID_DATA);
    EXPECT_THROW(CIGAR("60=3000="), ErrorCode::INVALID_DATA);
    EXPECT_THROW(CIGAR("0=30=00=100=0090="), ErrorCode::INVALID_DATA);

    return true;
}

bool test_unit_cigar_combined_M_and_equals_operations_throws() {
    EXPECT_THROW(CIGAR("0=0M0="), ErrorCode::INVALID_DATA);
    EXPECT_THROW(CIGAR("3=3M"), ErrorCode::INVALID_DATA);
    EXPECT_THROW(CIGAR("5M3="), ErrorCode::INVALID_DATA);
    EXPECT_THROW(CIGAR("60=3000=1M"), ErrorCode::INVALID_DATA);
    EXPECT_THROW(CIGAR("0=30=00M100=0090="), ErrorCode::INVALID_DATA);

    return true;
}

bool test_unit_cigar_unsupported_operations_result_in_zero_overlap() {
    return std::all_of(unsupported_operations.begin(),
                       unsupported_operations.end(),
                       [](char unsupported_cigar_op) {
                           return check_cigar_result(std::string{"2"} + unsupported_cigar_op, 0, false) &&
                                  check_cigar_result(std::string{"30"} + unsupported_cigar_op, 0, false);
                       });
}

bool test_unit_cigar_M_and_equals_operations_with_unsupported_operations_throws() {
    return std::all_of(unsupported_operations.begin(),
                       unsupported_operations.end(),
                       [](char unsupported_cigar_op) {
                           EXPECT_THROW(CIGAR(std::string{"2M2"} + unsupported_cigar_op), ErrorCode::INVALID_DATA);
                           EXPECT_THROW(CIGAR(std::string{"2"} + unsupported_cigar_op + "2M"), ErrorCode::INVALID_DATA);
                           EXPECT_THROW(CIGAR(std::string{"2"} + unsupported_cigar_op + "2="), ErrorCode::INVALID_DATA);
                           EXPECT_THROW(CIGAR(std::string{"2=2"} + unsupported_cigar_op), ErrorCode::INVALID_DATA);

                           return true;
                       });
}

bool test_unit_cigar_multiple_unsupported_operations_throws() {
    return std::all_of(unsupported_operations.begin(),
                       unsupported_operations.end(),
                       [](char unsupported_cigar_op)
                       {
                           return std::all_of(unsupported_operations.begin(),
                                              unsupported_operations.end(),
                                              [unsupported_cigar_op](char unsupported_cigar_op2)
                           {
                               const std::string two{"2"};

                               EXPECT_THROW(CIGAR(two + unsupported_cigar_op2 + two + unsupported_cigar_op),
                                            ErrorCode::INVALID_DATA);
                               EXPECT_THROW(CIGAR(two + unsupported_cigar_op + two + unsupported_cigar_op2),
                                            ErrorCode::INVALID_DATA);

                               return true;
                           });
                       });
}

bool test_unit_cigar_empty_cigar_string_throws() {
    EXPECT_THROW(CIGAR(""), ErrorCode::INVALID_DATA);

    return true;
}

bool test_unit_cigar_operation_not_starting_with_length_throws() {
    EXPECT_THROW(CIGAR("M"), ErrorCode::INVALID_DATA);
    EXPECT_THROW(CIGAR("="), ErrorCode::INVALID_DATA);

    for (const auto unsupported_cigar_op : unsupported_operations) {
        EXPECT_THROW(CIGAR(std::string{unsupported_cigar_op}), ErrorCode::INVALID_DATA);
    }

    return true;
}

bool test_unit_cigar_operation_length_not_following_with_operation_type_throws() {
    EXPECT_THROW(CIGAR("2"), ErrorCode::INVALID_DATA);
    EXPECT_THROW(CIGAR("30"), ErrorCode::INVALID_DATA);

    return true;
}

bool test_unit_cigar_unknown_cigar_operation_throws() {
    EXPECT_THROW(CIGAR("30Q"), ErrorCode::INVALID_CIGAR_OPERATION);
    EXPECT_THROW(CIGAR("30m"), ErrorCode::INVALID_CIGAR_OPERATION);

    return true;
}

bool test_unit_cigar_whitespace_after_operation_throws() {
    EXPECT_THROW(CIGAR("30M "), ErrorCode::INVALID_DATA);
    EXPECT_THROW(CIGAR("30M\t"), ErrorCode::INVALID_DATA);

    return true;
}

bool test_unit_cigar_garbage_inside_operation_throws() {
    EXPECT_THROW(CIGAR(" 2M"), ErrorCode::INVALID_DATA);
    EXPECT_THROW(CIGAR(" 2="), ErrorCode::INVALID_DATA);
    EXPECT_THROW(CIGAR("+2M"), ErrorCode::INVALID_DATA);
    EXPECT_THROW(CIGAR("+2="), ErrorCode::INVALID_DATA);
    EXPECT_THROW(CIGAR("-2M"), ErrorCode::INVALID_DATA);
    EXPECT_THROW(CIGAR("-2="), ErrorCode::INVALID_DATA);

    // The constructor assumes ' ' is a CIGAR operation, therefore it should throw a `INVALID_CIGAR_OPERATION` error.
    EXPECT_THROW(CIGAR("2 M"), ErrorCode::INVALID_CIGAR_OPERATION);
    EXPECT_THROW(CIGAR("2 ="), ErrorCode::INVALID_CIGAR_OPERATION);

    // Likewise for '\t'.
    EXPECT_THROW(CIGAR("2\tM"), ErrorCode::INVALID_CIGAR_OPERATION);
    EXPECT_THROW(CIGAR("2\t="), ErrorCode::INVALID_CIGAR_OPERATION);

    return true;
}

// Validate that overflow protection works on CIGAR operation lengths near the overflow boundary.
bool check_cigar_operation_overflow_protection_on_the_boundary(char cigar_operation_char, bool supported_operation) {
    const auto max_length_last_digit = static_cast<int>(cigar_operation_max_length_str.back() - '0');

    for (int digit = 0; digit <= 9; ++digit) {
        const auto digit_char = static_cast<char>(digit + '0');
        const auto cigar_string = cigar_operation_max_length_prefix + digit_char + cigar_operation_char;

        if (digit <= max_length_last_digit) {
            CIGAR cigar(cigar_string);

            if (supported_operation) {
                // Replace the last digit of `cigar_operation_max_length` with `digit`.
                ASSERT_EQUAL(cigar.overlap(), (cigar_operation_max_length / 10) * 10 + digit);
            } else {
                ASSERT_EQUAL(cigar.overlap(), 0);
            }
        } else {
            EXPECT_THROW(CIGAR(cigar_string), ErrorCode::UNSIGNED_INTEGER_OVERFLOW);
        }
    }

    return true;
}

bool test_unit_cigar_operation_length_overflow_protection_on_the_boundary() {
    for (const auto unsupported_cigar_op : unsupported_operations) {
        if (!check_cigar_operation_overflow_protection_on_the_boundary(unsupported_cigar_op, false)) {
            return false;
        }
    }

    return check_cigar_operation_overflow_protection_on_the_boundary('M', true) &&
           check_cigar_operation_overflow_protection_on_the_boundary('=', true);
}

bool test_unit_cigar_operation_length_overflow_protection() {
    for (int digit1 = 0; digit1 <= 9; ++digit1) {
        const auto digit1_char = static_cast<char>(digit1 + '0');

        for (int digit2 = 0; digit2 <= 9; ++digit2) {
            const auto digit2_char = static_cast<char>(digit2 + '0');
            const auto cigar_string = cigar_operation_max_length_prefix + digit1_char + digit2_char + 'M';

            EXPECT_THROW(CIGAR(cigar_string), ErrorCode::UNSIGNED_INTEGER_OVERFLOW);
        }
    }

    return true;
}

bool test_unit_cigar_unavailable_char() {
    return check_cigar_result(std::string{unavailable_operation_char}, 0, false);
}

bool test_unit_cigar_unavailable_char_must_not_occur_later_in_cigar_string() {
    EXPECT_THROW(CIGAR(std::string{unavailable_operation_char} + unavailable_operation_char), ErrorCode::INVALID_DATA);
    EXPECT_THROW(CIGAR(std::string{"2"} + unavailable_operation_char), ErrorCode::INVALID_CIGAR_OPERATION);

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_unit_cigar_single_M_operation),
        TEST(test_unit_cigar_multiple_M_operations_throws),
        TEST(test_unit_cigar_single_equals_operation),
        TEST(test_unit_cigar_multiple_equals_operations_throws),
        TEST(test_unit_cigar_combined_M_and_equals_operations_throws),
        TEST(test_unit_cigar_unsupported_operations_result_in_zero_overlap),
        TEST(test_unit_cigar_M_and_equals_operations_with_unsupported_operations_throws),
        TEST(test_unit_cigar_multiple_unsupported_operations_throws),
        TEST(test_unit_cigar_empty_cigar_string_throws),
        TEST(test_unit_cigar_operation_not_starting_with_length_throws),
        TEST(test_unit_cigar_operation_length_not_following_with_operation_type_throws),
        TEST(test_unit_cigar_unknown_cigar_operation_throws),
        TEST(test_unit_cigar_whitespace_after_operation_throws),
        TEST(test_unit_cigar_garbage_inside_operation_throws),
        TEST(test_unit_cigar_operation_length_overflow_protection_on_the_boundary),
        TEST(test_unit_cigar_operation_length_overflow_protection),
        TEST(test_unit_cigar_unavailable_char),
        TEST(test_unit_cigar_unavailable_char_must_not_occur_later_in_cigar_string),
    };

    return Test::run_suite("test_unit_cigar", tests);
}
