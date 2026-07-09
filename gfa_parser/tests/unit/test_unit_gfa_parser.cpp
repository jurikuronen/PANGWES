/*
 * test_unit_gfa_parser.cpp - Unit tests for gfa_parser.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <iostream>
#include <sstream>

#include "common/program_options/program_options_option_names.hpp"
#include "common/test_harness/Test.hpp"
#include "gfa_parser/gfa_parser.hpp"
#include "gfa_parser/program_options/GFAParserOptions.hpp"
#include "gfa_parser/program_options/gfa_parser_option_names.hpp"
#include "mocks/MockArgumentsBuilder.hpp"

namespace PANGWES {
namespace {

using Mocks::MockArgumentsBuilder;

constexpr auto test_gfa_filename = "test.gfa";
constexpr auto test_valid_k = "3";

std::ostringstream error_output;
std::streambuf* cerr_buffer = nullptr;

bool test_setup() {
    // Suppress std::cerr by redirecting it to an std::ostringstream buffer.
    cerr_buffer = std::cerr.rdbuf(error_output.rdbuf());

    return true;
}

bool test_teardown() {
    // Restore std::cerr for iostream cleanup at program exit.
    std::cerr.rdbuf(cerr_buffer);
    cerr_buffer = nullptr;

    return true;
}

bool test_unit_check_gfa_parser_options_accepts_valid_options() {
    MockArgumentsBuilder<> test_arguments({ GFA_FILENAME_OPTION, test_gfa_filename, K_OPTION, test_valid_k });
    GFAParserOptions options(test_arguments.argc(), test_arguments.argv());

    ASSERT_TRUE(check_gfa_parser_options(options));

    return true;
}

bool test_unit_check_gfa_parser_options_without_gfa_file_returns_false() {
    MockArgumentsBuilder<> test_arguments({ K_OPTION, test_valid_k });
    GFAParserOptions options(test_arguments.argc(), test_arguments.argv());

    ASSERT_FALSE(check_gfa_parser_options(options));

    return true;
}

bool test_unit_check_gfa_parser_options_without_k_returns_false() {
    MockArgumentsBuilder<> test_arguments({ GFA_FILENAME_OPTION, test_gfa_filename });
    GFAParserOptions options(test_arguments.argc(), test_arguments.argv());

    ASSERT_FALSE(check_gfa_parser_options(options));

    return true;
}

bool test_unit_check_gfa_parser_options_with_unsupported_gfa_format_returns_false() {
    constexpr auto test_unsupported_gfa_format = "2";

    MockArgumentsBuilder<> test_arguments({ GFA_FILENAME_OPTION, test_gfa_filename,
                                            GFA_FORMAT_OPTION, test_unsupported_gfa_format,
                                            K_OPTION, test_valid_k });
    GFAParserOptions options(test_arguments.argc(), test_arguments.argv());

    ASSERT_FALSE(check_gfa_parser_options(options));

    return true;
}

bool test_unit_check_gfa_parser_options_with_even_k_returns_false() {
    constexpr auto test_even_k = "4";

    MockArgumentsBuilder<> test_arguments({ GFA_FILENAME_OPTION, test_gfa_filename, K_OPTION, test_even_k });
    GFAParserOptions options(test_arguments.argc(), test_arguments.argv());

    ASSERT_FALSE(check_gfa_parser_options(options));

    return true;
}

bool test_unit_check_gfa_parser_options_with_too_small_k_returns_false() {
    constexpr auto test_too_small_k = "1";

    MockArgumentsBuilder<> test_arguments({ GFA_FILENAME_OPTION, test_gfa_filename, K_OPTION, test_too_small_k });
    GFAParserOptions options(test_arguments.argc(), test_arguments.argv());

    ASSERT_FALSE(check_gfa_parser_options(options));

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_unit_check_gfa_parser_options_accepts_valid_options),
        TEST(test_unit_check_gfa_parser_options_without_gfa_file_returns_false),
        TEST(test_unit_check_gfa_parser_options_without_k_returns_false),
        TEST(test_unit_check_gfa_parser_options_with_unsupported_gfa_format_returns_false),
        TEST(test_unit_check_gfa_parser_options_with_even_k_returns_false),
        TEST(test_unit_check_gfa_parser_options_with_too_small_k_returns_false),
    };

    return Test::run_suite("test_unit_gfa_parser", tests, test_setup, test_teardown);
}
