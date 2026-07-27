/*
 * test_unit_utils.cpp - Unit tests for various utility functions defined in utils/utils.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <cstddef>
#include <limits>
#include <string>

#include "common/test_harness/Test.hpp"
#include "common/utils/utils.hpp"

namespace PANGWES {
namespace {

constexpr auto SIZE_T_MAX = std::numeric_limits<std::size_t>::max();

constexpr auto N_FIELDS_MAX = 1000;

bool check_ws_separated_fields_from_string(const std::string& string,
                                           const std::vector<std::string>& expected_fields,
                                           std::size_t n_fields = N_FIELDS_MAX)
{
    std::vector<std::string> fields;

    if (n_fields == N_FIELDS_MAX) {
        fields = Utils::get_fields_ws(string);
    } else {
        fields = Utils::get_fields_ws(string, n_fields);
    }

    ASSERT_EQUAL(fields.size(), expected_fields.size());

    for (std::size_t idx = 0; idx < fields.size(); ++idx) {
        ASSERT_EQUAL(fields[idx], expected_fields[idx]);
    }

    return true;
}

bool check_delimited_fields_from_string(char delimiter,
                                        const std::string& string,
                                        const std::vector<std::string>& expected_fields,
                                        std::size_t n_fields = N_FIELDS_MAX)
{
    std::vector<std::string> fields;

    if (n_fields == N_FIELDS_MAX) {
        fields = Utils::get_fields(string, delimiter);
    } else {
        fields = Utils::get_fields(string, delimiter, n_fields);
    }

    ASSERT_EQUAL(fields.size(), expected_fields.size());

    for (std::size_t idx = 0; idx < fields.size(); ++idx) {
        ASSERT_EQUAL(fields[idx], expected_fields[idx]);
    }

    return true;
}

bool test_unit_utils_get_fields_ws_space_delimited() {
    const std::string test_space_delimited_string = "field1 field2 field3";

    return check_ws_separated_fields_from_string(test_space_delimited_string, {"field1", "field2", "field3"});
}

bool test_unit_utils_get_fields_ws_tab_delimited() {
    const std::string test_tab_delimited_string = "field1\tfield2\tfield3";

    return check_ws_separated_fields_from_string(test_tab_delimited_string, {"field1", "field2", "field3"});
}

bool test_unit_utils_get_fields_ws_leading_whitespace() {
    const std::string test_str_leading_ws = "\t\t \t  \t  field1 field2 field3";

    return check_ws_separated_fields_from_string(test_str_leading_ws, {"field1", "field2", "field3"});
}

bool test_unit_utils_get_fields_ws_trailing_whitespace() {
    const std::string test_str_trailing_ws = "field1 field2 field3 \t  \t\t  ";

    return check_ws_separated_fields_from_string(test_str_trailing_ws, {"field1", "field2", "field3"});
}

bool test_unit_utils_get_fields_ws_extra_whitespace() {
    const std::string test_str_extra_ws = "\t    \tfield1  \t field2   field3 \t    \t\t";

    return check_ws_separated_fields_from_string(test_str_extra_ws, {"field1", "field2", "field3"});
}

bool test_unit_utils_get_fields_ws_not_whitespace() {
    const std::string test_comma_delimited_string = "field1,field2,field3";
    const std::string test_semicolon_delimited_string = "field1 ;\t field2,;field3";

    return check_ws_separated_fields_from_string(test_comma_delimited_string, {"field1,field2,field3"}) &&
           check_ws_separated_fields_from_string(test_semicolon_delimited_string, {"field1", ";", "field2,;field3"});
}

bool test_unit_utils_get_fields_ws_empty_string() {
    return check_ws_separated_fields_from_string("", {});
}

bool test_unit_utils_get_fields_ws_read_n_fields() {
    const std::string test_string = "field1 field2 field3 field4";

    return check_ws_separated_fields_from_string(test_string, {}, 0) &&
           check_ws_separated_fields_from_string(test_string, {"field1"}, 1) &&
           check_ws_separated_fields_from_string(test_string, {"field1", "field2"}, 2) &&
           check_ws_separated_fields_from_string(test_string, {"field1", "field2", "field3"}, 3) &&
           check_ws_separated_fields_from_string(test_string, {"field1", "field2", "field3", "field4"}, 4);
}

bool test_unit_utils_get_fields() {
    const std::string test_comma_delimited_string = "field1,field2,,field4";
    const std::string test_colon_delimited_string = "field1:field2::field4";
    const std::string test_mixed_delimiters_string = "field1,field2:field3 field4";

    return check_delimited_fields_from_string(',', test_comma_delimited_string, {"field1", "field2", "", "field4"}) &&
           check_delimited_fields_from_string(':', test_colon_delimited_string, {"field1", "field2", "", "field4"}) &&
           check_delimited_fields_from_string(',', test_mixed_delimiters_string, {"field1", "field2:field3 field4"}) &&
           check_delimited_fields_from_string(':', test_mixed_delimiters_string, {"field1,field2", "field3 field4"}) &&
           check_delimited_fields_from_string(' ', test_mixed_delimiters_string, {"field1,field2:field3", "field4"});
}

bool test_unit_utils_get_fields_empty_string() {
    return check_delimited_fields_from_string(',', "", {});
}


bool test_unit_utils_get_fields_read_n_fields() {
    const std::string test_string = "field1,field2,field3,field4";


    return check_delimited_fields_from_string(',', test_string, {}, 0) &&
           check_delimited_fields_from_string(',', test_string, {"field1"}, 1) &&
           check_delimited_fields_from_string(',', test_string, {"field1", "field2"}, 2) &&
           check_delimited_fields_from_string(',', test_string, {"field1", "field2", "field3"}, 3) &&
           check_delimited_fields_from_string(',', test_string, {"field1", "field2", "field3", "field4"}, 4);
}

bool test_unit_utils_parse_unsigned_value_parses_numerical_string() {
    ASSERT_EQUAL(Utils::parse_unsigned_value("42"), 42);

    return true;
}

bool test_unit_utils_parse_unsigned_value_negative_numerical_string_throws() {
    EXPECT_THROW(Utils::parse_unsigned_value("-42"), ErrorCode::INVALID_DATA);

    return true;
}

bool test_unit_utils_parse_unsigned_value_string_value_throws() {
    EXPECT_THROW(Utils::parse_unsigned_value("str"), ErrorCode::INVALID_DATA);

    return true;
}

bool test_unit_utils_parse_unsigned_value_partial_parse_throws() {
    EXPECT_THROW(Utils::parse_unsigned_value("42G"), ErrorCode::INVALID_DATA);

    return true;
}

bool test_unit_utils_safe_add() {
    ASSERT_EQUAL(Utils::safe_add(std::size_t{20}, std::size_t{22}), 42);
    ASSERT_EQUAL(Utils::safe_add(2, std::size_t{40}), 42);
    ASSERT_EQUAL(Utils::safe_add(SIZE_T_MAX, 0), SIZE_T_MAX);
    ASSERT_EQUAL(Utils::safe_add(SIZE_T_MAX - 1, 1), SIZE_T_MAX);

    return true;
}

bool test_unit_utils_safe_add_overflow_throws() {
    EXPECT_THROW(Utils::safe_add(SIZE_T_MAX, 1), ErrorCode::UNSIGNED_INTEGER_OVERFLOW);
    EXPECT_THROW(Utils::safe_add(1, SIZE_T_MAX), ErrorCode::UNSIGNED_INTEGER_OVERFLOW);

    return true;
}

bool test_unit_utils_safe_add_negative_argument_throws() {
    EXPECT_THROW(Utils::safe_add(std::size_t{0}, -1), ErrorCode::INVALID_ARGUMENT);
    EXPECT_THROW(Utils::safe_add(-1, std::size_t{0}), ErrorCode::INVALID_ARGUMENT);

    return true;
}

bool test_unit_utils_safe_multiply() {
    ASSERT_EQUAL(Utils::safe_multiply(std::size_t{6}, std::size_t{7}), 42);
    ASSERT_EQUAL(Utils::safe_multiply(2, std::size_t{21}), 42);
    ASSERT_EQUAL(Utils::safe_multiply(0, SIZE_T_MAX), 0);
    ASSERT_EQUAL(Utils::safe_multiply(SIZE_T_MAX, 0), 0);
    ASSERT_EQUAL(Utils::safe_multiply(SIZE_T_MAX, 1), SIZE_T_MAX);
    ASSERT_EQUAL(Utils::safe_multiply(SIZE_T_MAX / 2, 2), SIZE_T_MAX - 1);

    return true;
}

bool test_unit_utils_safe_multiply_overflow_throws() {
    EXPECT_THROW(Utils::safe_multiply((SIZE_T_MAX / 2) + 1, 2), ErrorCode::UNSIGNED_INTEGER_OVERFLOW);
    EXPECT_THROW(Utils::safe_multiply(2, (SIZE_T_MAX / 2) + 1), ErrorCode::UNSIGNED_INTEGER_OVERFLOW);

    return true;
}

bool test_unit_utils_safe_multiply_negative_argument_throws() {
    EXPECT_THROW(Utils::safe_multiply(std::size_t{0}, -1), ErrorCode::INVALID_ARGUMENT);
    EXPECT_THROW(Utils::safe_multiply(-1, std::size_t{0}), ErrorCode::INVALID_ARGUMENT);

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_unit_utils_get_fields_ws_space_delimited),
        TEST(test_unit_utils_get_fields_ws_tab_delimited),
        TEST(test_unit_utils_get_fields_ws_leading_whitespace),
        TEST(test_unit_utils_get_fields_ws_trailing_whitespace),
        TEST(test_unit_utils_get_fields_ws_extra_whitespace),
        TEST(test_unit_utils_get_fields_ws_not_whitespace),
        TEST(test_unit_utils_get_fields_ws_empty_string),
        TEST(test_unit_utils_get_fields_ws_read_n_fields),
        TEST(test_unit_utils_get_fields),
        TEST(test_unit_utils_get_fields_empty_string),
        TEST(test_unit_utils_get_fields_read_n_fields),
        TEST(test_unit_utils_parse_unsigned_value_parses_numerical_string),
        TEST(test_unit_utils_parse_unsigned_value_negative_numerical_string_throws),
        TEST(test_unit_utils_parse_unsigned_value_string_value_throws),
        TEST(test_unit_utils_parse_unsigned_value_partial_parse_throws),
        TEST(test_unit_utils_safe_add),
        TEST(test_unit_utils_safe_add_overflow_throws),
        TEST(test_unit_utils_safe_add_negative_argument_throws),
        TEST(test_unit_utils_safe_multiply),
        TEST(test_unit_utils_safe_multiply_overflow_throws),
        TEST(test_unit_utils_safe_multiply_negative_argument_throws),
    };

    return Test::run_suite("test_unit_utils", tests);
}
