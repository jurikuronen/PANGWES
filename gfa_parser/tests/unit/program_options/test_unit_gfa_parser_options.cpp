/*
 * test_unit_gfa_parser_options.cpp - Unit tests for program_options/GFAParserOptions.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <string>

#include "common/program_options/program_options_option_names.hpp"
#include "common/test_harness/program_options_test_helpers.hpp"
#include "common/test_harness/Test.hpp"
#include "common/utils/Exception.hpp"
#include "mocks/MockArgumentsBuilder.hpp"
#include "gfa_parser/core/GFAFormat.hpp"
#include "gfa_parser/program_options/GFAParserOptions.hpp"
#include "gfa_parser/program_options/gfa_parser_option_names.hpp"

namespace PANGWES {
namespace {

using Mocks::MockArgumentsBuilder;

bool test_unit_gfa_parser_options_gfa_format() {
    const auto check_gfa_format_option = [](const std::string& option) {
        MockArgumentsBuilder<> test_arguments_gfa1({ option.c_str(), "1" });
        GFAParserOptions options_gfa1(test_arguments_gfa1.argc(), test_arguments_gfa1.argv());

        ASSERT_ENUMS_EQUAL(options_gfa1.gfa_format(), GFAFormat::GFA1);

        MockArgumentsBuilder<> test_arguments_gfa2({ option.c_str(), "2" });
        GFAParserOptions options_gfa2(test_arguments_gfa2.argc(), test_arguments_gfa2.argv());

        ASSERT_ENUMS_EQUAL(options_gfa2.gfa_format(), GFAFormat::GFA2);

        return true;
    };

    return check_gfa_format_option(GFA_FORMAT_OPTION) && check_gfa_format_option(GFA_FORMAT_LONG_OPTION);
}

bool test_unit_gfa_parser_options_bad_arg_value_to_gfa_format() {
    const auto check_bad_arg_value_to_gfa_format = [](const std::string& option) {
        MockArgumentsBuilder<> string_argument({ option.c_str(), "a" });

        EXPECT_THROW(GFAParserOptions(string_argument.argc(), string_argument.argv()),
                     ErrorCode::INVALID_PROGRAM_OPTION);

        MockArgumentsBuilder<> signed_argument({ option.c_str(), "-5" });

        EXPECT_THROW(GFAParserOptions(signed_argument.argc(), signed_argument.argv()),
                     ErrorCode::INVALID_PROGRAM_OPTION);

        MockArgumentsBuilder<> unsigned_out_of_enum_range_argument({ option.c_str(), "3" });

        EXPECT_THROW(GFAParserOptions(unsigned_out_of_enum_range_argument.argc(),
                                      unsigned_out_of_enum_range_argument.argv()), ErrorCode::INVALID_PROGRAM_OPTION);

        return true;
    };

    return check_bad_arg_value_to_gfa_format(GFA_FORMAT_OPTION) &&
           check_bad_arg_value_to_gfa_format(GFA_FORMAT_LONG_OPTION);
}

bool test_unit_gfa_parser_options_out_filenames_set_correctly() {
    constexpr auto test_out_stem = "test";

    MockArgumentsBuilder<> test_arguments({ OUT_STEM_OPTION, test_out_stem });
    GFAParserOptions options(test_arguments.argc(), test_arguments.argv());

    const auto out_unitigs_filename = options.out_unitigs_filename();
    const auto out_fasta_filename = options.out_fasta_filename();
    const auto out_sgg_paths_filename = options.out_sgg_paths_filename();
    const auto out_sgg_paths_directory = options.out_sgg_paths_directory();

    const auto expected_out_unitigs_filename = std::string{test_out_stem} + UNITIGS_OUT_FILE_EXTENSION;
    const auto expected_out_fasta_filename = std::string{test_out_stem} + FASTA_OUT_FILE_EXTENSION;
    const auto expected_out_sgg_paths_filename = std::string{test_out_stem} + SGG_PATHS_OUT_FILE_EXTENSION;
    const auto expected_out_sgg_paths_directory = std::string{test_out_stem} + SGG_PATHS_DIRECTORY_SUFFIX;

    ASSERT_EQUAL(out_unitigs_filename, expected_out_unitigs_filename);
    ASSERT_EQUAL(out_fasta_filename, expected_out_fasta_filename);
    ASSERT_EQUAL(out_sgg_paths_filename, expected_out_sgg_paths_filename);
    ASSERT_EQUAL(out_sgg_paths_directory, expected_out_sgg_paths_directory);

    return true;
}

bool test_unit_gfa_parser_options_out_filenames_default_values_set_correctly() {
    // Provide some empty argument in order for GFAParserOptions constructor to not return early.
    MockArgumentsBuilder<> test_arguments({ "" });
    GFAParserOptions options(test_arguments.argc(), test_arguments.argv());

    const auto out_unitigs_filename = options.out_unitigs_filename();
    const auto out_fasta_filename = options.out_fasta_filename();
    const auto out_sgg_paths_filename = options.out_sgg_paths_filename();
    const auto out_sgg_paths_directory = options.out_sgg_paths_directory();

    const auto expected_out_unitigs_filename = std::string{DEFAULT_OUT_STEM} + UNITIGS_OUT_FILE_EXTENSION;
    const auto expected_out_fasta_filename = std::string{DEFAULT_OUT_STEM} + FASTA_OUT_FILE_EXTENSION;
    const auto expected_out_sgg_paths_filename = std::string{DEFAULT_OUT_STEM} + SGG_PATHS_OUT_FILE_EXTENSION;
    const auto expected_out_sgg_paths_directory = std::string{DEFAULT_OUT_STEM} + SGG_PATHS_DIRECTORY_SUFFIX;

    ASSERT_EQUAL(out_unitigs_filename, expected_out_unitigs_filename);
    ASSERT_EQUAL(out_fasta_filename, expected_out_fasta_filename);
    ASSERT_EQUAL(out_sgg_paths_filename, expected_out_sgg_paths_filename);
    ASSERT_EQUAL(out_sgg_paths_directory, expected_out_sgg_paths_directory);

    return true;
}

bool test_unit_gfa_parser_options_n_workers_less_than_n_threads() {
    // Check the default value.
    MockArgumentsBuilder<> thread_0_arguments({ N_THREADS_OPTION, "0" });
    GFAParserOptions thread_0_options(thread_0_arguments.argc(), thread_0_arguments.argv());
    ASSERT_EQUAL(thread_0_options.n_threads(), 1);
    ASSERT_EQUAL(thread_0_options.n_workers(), 0);

    // Check threads set explicitly to 1.
    MockArgumentsBuilder<> thread_1_arguments({ N_THREADS_OPTION, "1" });
    GFAParserOptions thread_1_options(thread_1_arguments.argc(), thread_1_arguments.argv());
    ASSERT_EQUAL(thread_1_options.n_threads(), 1);
    ASSERT_EQUAL(thread_1_options.n_workers(), 0);

    // Check multi-threaded value.
    MockArgumentsBuilder<> thread_5_arguments({ N_THREADS_OPTION, "5" });
    GFAParserOptions thread_5_options(thread_5_arguments.argc(), thread_5_arguments.argv());
    ASSERT_EQUAL(thread_5_options.n_threads(), 5);
    ASSERT_EQUAL(thread_5_options.n_workers(), 4);

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    // Defined for `PROGRAM_OPTIONS_TEST` macro.
    const auto* suite_name = "test_unit_gfa_parser_options";
    using ProgramOptionsT = GFAParserOptions;

    const auto test_unit_gfa_parser_options_verbose_by_default =
        test_unit_program_options_verbose_by_default<ProgramOptionsT>;

    const auto test_unit_gfa_parser_options_quiet_sets_verbose_false =
        test_unit_program_options_quiet_sets_verbose_false<ProgramOptionsT>;

    const auto test_unit_gfa_parser_options_0_n_threads_defaults_to_one =
        test_unit_program_options_option_argument_defaults_to_value<ProgramOptionsT>(
            N_THREADS_OPTION,
            N_THREADS_LONG_OPTION,
            "0",
            1,
            OPTION_ACCESSOR(n_threads));

    const auto test_unit_gfa_parser_options_bad_arg_value_to_k =
        test_unit_program_options_bad_arg_value<ProgramOptionsT>(K_OPTION, K_LONG_OPTION);

    const auto test_unit_gfa_parser_options_bad_arg_value_to_n_threads =
        test_unit_program_options_bad_arg_value<ProgramOptionsT>(N_THREADS_OPTION, N_THREADS_LONG_OPTION);

    const auto tests = {
        PROGRAM_OPTIONS_TEST(gfa_filename, GFA_FILENAME),
        PROGRAM_OPTIONS_TEST(k, K),
        PROGRAM_OPTIONS_TEST(n_threads, N_THREADS),
        TEST(test_unit_gfa_parser_options_verbose_by_default),
        TEST(test_unit_gfa_parser_options_quiet_sets_verbose_false),
        TEST(test_unit_gfa_parser_options_gfa_format),
        TEST(test_unit_gfa_parser_options_bad_arg_value_to_gfa_format),
        TEST(test_unit_gfa_parser_options_out_filenames_set_correctly),
        TEST(test_unit_gfa_parser_options_out_filenames_default_values_set_correctly),
        TEST(test_unit_gfa_parser_options_0_n_threads_defaults_to_one),
        TEST(test_unit_gfa_parser_options_bad_arg_value_to_k),
        TEST(test_unit_gfa_parser_options_bad_arg_value_to_n_threads),
        TEST(test_unit_gfa_parser_options_n_workers_less_than_n_threads),
    };

    return Test::run_suite(suite_name, tests);
}
