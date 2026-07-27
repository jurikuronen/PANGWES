/*
 * test_unit_program_options.cpp - Unit tests for program_options/ProgramOptions.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <cstddef>
#include <cstdint>
#include <string>

#include "common/program_options/ProgramOptions.hpp"
#include "common/test_harness/program_options_test_helpers.hpp"
#include "common/test_harness/Test.hpp"
#include "common/utils/Exception.hpp"
#include "mocks/MockArgumentsBuilder.hpp"

namespace PANGWES {
namespace {

using Mocks::MockArgumentsBuilder;

bool test_unit_program_options_argv_begin_skips_program_name() {
    MockArgumentsBuilder<> test_arguments({"-a"});
    TestProgramOptions program_options(test_arguments.argc(), test_arguments.argv());

    ASSERT_FALSE(program_options.argv_begin() == program_options.argv_end());
    ASSERT_EQUAL(std::string{*program_options.argv_begin()}, "-a");

    return true;
}

bool test_unit_program_options_argv_end_points_past_last_argument() {
    MockArgumentsBuilder<> test_arguments({"-a", "-b"});
    TestProgramOptions program_options(test_arguments.argc(), test_arguments.argv());

    // Distance between begin and end should equal the number of arguments.
    ASSERT_EQUAL(static_cast<std::size_t>(program_options.argv_end() - program_options.argv_begin()), 2);

    return true;
}

bool test_unit_program_options_argv_find_finds_existing_option() {
    MockArgumentsBuilder<> test_arguments({"-a", "-b"});
    TestProgramOptions program_options(test_arguments.argc(), test_arguments.argv());

    char** argv_it = program_options.argv_find("-b");

    ASSERT_FALSE(argv_it == program_options.argv_end());
    ASSERT_EQUAL(std::string(*argv_it), "-b");

    return true;
}

bool test_unit_program_options_argv_find_returns_end_if_not_found() {
    MockArgumentsBuilder<> test_arguments({"-a"});
    TestProgramOptions program_options(test_arguments.argc(), test_arguments.argv());

    char** argv_it = program_options.argv_find("-b");

    ASSERT_TRUE(argv_it == program_options.argv_end());

    return true;
}

bool test_unit_program_options_find_arg_true_if_short_option_present() {
    MockArgumentsBuilder<> test_arguments({"-a"});
    TestProgramOptions program_options(test_arguments.argc(), test_arguments.argv());

    ASSERT_TRUE(program_options.find_arg("-a", "--a-long"));

    return true;
}

bool test_unit_program_options_find_arg_true_if_long_option_present() {
    MockArgumentsBuilder<> test_arguments({"--a-long"});
    TestProgramOptions program_options(test_arguments.argc(), test_arguments.argv());

    ASSERT_TRUE(program_options.find_arg("-a", "--a-long"));

    return true;
}

bool test_unit_program_options_find_arg_false_if_neither_option_present() {
    MockArgumentsBuilder<> test_arguments({"-a"});
    TestProgramOptions program_options(test_arguments.argc(), test_arguments.argv());

    ASSERT_FALSE(program_options.find_arg("-b", "--b-long"));

    return true;
}

bool test_unit_program_options_read_unsigned_value_returns_zero_if_missing() {
    MockArgumentsBuilder<> test_arguments({"-a"});
    TestProgramOptions program_options(test_arguments.argc(), test_arguments.argv());

    ASSERT_EQUAL(program_options.read_unsigned_value("-b", "--b-long"), 0);

    return true;
}

bool test_unit_program_options_read_unsigned_value_reads_short_option_value() {
    MockArgumentsBuilder<> test_arguments({"-a", "42"});
    TestProgramOptions program_options(test_arguments.argc(), test_arguments.argv());

    ASSERT_EQUAL(program_options.read_unsigned_value("-a", "--a-long"), 42);

    return true;
}

bool test_unit_program_options_read_unsigned_value_reads_long_option_value() {
    MockArgumentsBuilder<> test_arguments({"--a-long", "42"});
    TestProgramOptions program_options(test_arguments.argc(), test_arguments.argv());

    ASSERT_EQUAL(program_options.read_unsigned_value("-a", "--a-long"), 42);

    return true;
}

bool test_unit_program_options_read_unsigned_value_negative_value_throws() {
    MockArgumentsBuilder<> test_arguments({"-a", "-42"});
    TestProgramOptions program_options(test_arguments.argc(), test_arguments.argv());

    EXPECT_THROW(program_options.read_unsigned_value("-a", "--a-long"), ErrorCode::INVALID_PROGRAM_OPTION);

    return true;
}

bool test_unit_program_options_read_unsigned_value_non_numeric_value_throws() {
    MockArgumentsBuilder<> test_arguments({"-a", "str"});
    TestProgramOptions program_options(test_arguments.argc(), test_arguments.argv());

    EXPECT_THROW(program_options.read_unsigned_value("-a", "--a-long"), ErrorCode::INVALID_PROGRAM_OPTION);

    return true;
}

bool test_unit_program_options_read_unsigned_value_partial_parse_throws() {
    MockArgumentsBuilder<> test_arguments({"-a", "42a"});
    TestProgramOptions program_options(test_arguments.argc(), test_arguments.argv());

    EXPECT_THROW(program_options.read_unsigned_value("-a", "--a-long"), ErrorCode::INVALID_PROGRAM_OPTION);

    return true;
}

bool test_unit_program_options_read_unsigned_value_short_read_if_both_present() {
    MockArgumentsBuilder<> test_arguments({"-a", "42", "--a-long", "43"});
    TestProgramOptions program_options(test_arguments.argc(), test_arguments.argv());

    ASSERT_EQUAL(program_options.read_unsigned_value("-a", "--a-long"), 42);

    return true;
}

bool test_unit_program_options_read_string_value_returns_empty_if_missing() {
    MockArgumentsBuilder<> test_arguments({"-a"});
    TestProgramOptions program_options(test_arguments.argc(), test_arguments.argv());

    ASSERT_EQUAL(program_options.read_string_value("-b", "--b-long"), std::string{});

    return true;
}

bool test_unit_program_options_read_string_value_reads_short_option_value() {
    MockArgumentsBuilder<> test_arguments({"-a", "a_value"});
    TestProgramOptions program_options(test_arguments.argc(), test_arguments.argv());

    ASSERT_EQUAL(program_options.read_string_value("-a", "--a-long"), "a_value");

    return true;
}

bool test_unit_program_options_read_string_value_reads_long_option_value() {
    MockArgumentsBuilder<> test_arguments({"--a-long", "a_value"});
    TestProgramOptions program_options(test_arguments.argc(), test_arguments.argv());

    ASSERT_EQUAL(program_options.read_string_value("-a", "--a-long"), "a_value");

    return true;
}

bool test_unit_program_options_read_string_value_short_read_if_both_present() {
    MockArgumentsBuilder<> test_arguments({"-a", "a_value", "--a-long", "a_long_value"});
    TestProgramOptions program_options(test_arguments.argc(), test_arguments.argv());

    ASSERT_EQUAL(program_options.read_string_value("-a", "--a-long"), "a_value");

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_unit_program_options_argv_begin_skips_program_name),
        TEST(test_unit_program_options_argv_end_points_past_last_argument),
        TEST(test_unit_program_options_argv_find_finds_existing_option),
        TEST(test_unit_program_options_argv_find_returns_end_if_not_found),
        TEST(test_unit_program_options_find_arg_true_if_short_option_present),
        TEST(test_unit_program_options_find_arg_true_if_long_option_present),
        TEST(test_unit_program_options_find_arg_false_if_neither_option_present),
        TEST(test_unit_program_options_read_unsigned_value_returns_zero_if_missing),
        TEST(test_unit_program_options_read_unsigned_value_reads_short_option_value),
        TEST(test_unit_program_options_read_unsigned_value_reads_long_option_value),
        TEST(test_unit_program_options_read_unsigned_value_negative_value_throws),
        TEST(test_unit_program_options_read_unsigned_value_non_numeric_value_throws),
        TEST(test_unit_program_options_read_unsigned_value_partial_parse_throws),
        TEST(test_unit_program_options_read_unsigned_value_short_read_if_both_present),
        TEST(test_unit_program_options_read_string_value_returns_empty_if_missing),
        TEST(test_unit_program_options_read_string_value_reads_short_option_value),
        TEST(test_unit_program_options_read_string_value_reads_long_option_value),
        TEST(test_unit_program_options_read_string_value_short_read_if_both_present),
    };

    return Test::run_suite("test_unit_program_options", tests);
}
