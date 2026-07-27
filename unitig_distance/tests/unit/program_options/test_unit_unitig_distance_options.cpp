/*
 * test_unit_unitig_distance_options.cpp - Unit tests for program_options/UnitigDistanceOptions.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <cstddef>
#include <limits>
#include <string>

#include "common/test_harness/program_options_test_helpers.hpp"
#include "common/test_harness/Test.hpp"
#include "common/utils/Exception.hpp"
#include "common/utils/memory.hpp"
#include "mocks/MockArgumentsBuilder.hpp"
#include "unitig_distance/program_options/UnitigDistanceOptions.hpp"

namespace PANGWES {
namespace {

using Mocks::MockArgumentsBuilder;

bool test_unit_unitig_distance_options_one_based_output_set_correctly() {
    const auto strip_based_str = [](const std::string& out_filename)
    {
        if (out_filename.find(ONE_BASED_STR) != std::string::npos) {
            return ONE_BASED_STR;
        }
        if (out_filename.find(ZERO_BASED_STR) != std::string::npos) {
            return ZERO_BASED_STR;
        }
        return "UNKNOWN_BASED_STR";
    };

    // Verify that output one-based option sets one-based output.
    for (const auto& option : { OUTPUT_ONE_BASED_OPTION, OUTPUT_ONE_BASED_LONG_OPTION })
    {
        MockArgumentsBuilder<bool> test_arguments(option);
        UnitigDistanceOptions options(test_arguments.argc(), test_arguments.argv());

        ASSERT_EQUAL(strip_based_str(options.out_filename()), ONE_BASED_STR);
    }

    // Verify that queries one-based option doesn't unintentionally set one-based output.
    for (const auto& option : { QUERIES_ONE_BASED_OPTION, QUERIES_ONE_BASED_LONG_OPTION })
    {
        MockArgumentsBuilder<bool> test_arguments(option);
        UnitigDistanceOptions options(test_arguments.argc(), test_arguments.argv());

        ASSERT_EQUAL(strip_based_str(options.out_filename()), ZERO_BASED_STR);
    }

    // Verify that when both one-basedness options are provided, output will be one-based.
    MockArgumentsBuilder<bool> test_arguments({ OUTPUT_ONE_BASED_OPTION, QUERIES_ONE_BASED_OPTION });
    UnitigDistanceOptions options(test_arguments.argc(), test_arguments.argv());

    ASSERT_EQUAL(strip_based_str(options.out_filename()), ONE_BASED_STR);

    return true;
}

bool test_unit_unitig_distance_options_out_filename_set_correctly() {
    constexpr auto test_out_stem = "test";

    MockArgumentsBuilder<> test_arguments({ OUT_STEM_OPTION, test_out_stem, OUTPUT_ONE_BASED_OPTION });
    UnitigDistanceOptions options(test_arguments.argc(), test_arguments.argv());

    const auto out_filename = options.out_filename();
    const auto expected_out_filename = std::string{test_out_stem} + UD_OUT_FILE_EXTENSION + ONE_BASED_STR;

    ASSERT_EQUAL(out_filename, expected_out_filename);

    return true;
}

bool test_unit_unitig_distance_options_out_filename_default_value_set_correctly() {
    MockArgumentsBuilder<> test_arguments({ OUTPUT_ONE_BASED_OPTION });
    UnitigDistanceOptions options(test_arguments.argc(), test_arguments.argv());

    const auto out_filename = options.out_filename();
    const auto expected_out_filename = std::string{DEFAULT_OUT_STEM} + UD_OUT_FILE_EXTENSION + ONE_BASED_STR;

    ASSERT_EQUAL(out_filename, expected_out_filename);

    return true;
}

bool test_unit_unitig_distance_options_memory_arg_without_suffix_assumes_gib() {
    constexpr auto expected_memory_bytes = Memory::GiB * 15;

    for (const auto& option : { MEMORY_OPTION, MEMORY_LONG_OPTION }) {
        MockArgumentsBuilder<> test_arguments({ option, "15" });
        UnitigDistanceOptions options(test_arguments.argc(), test_arguments.argv());

        const auto memory_bytes = options.memory_bytes();

        ASSERT_EQUAL(memory_bytes, expected_memory_bytes);
    }

    return true;
}

bool test_unit_unitig_distance_options_memory_arg_default_value() {
    constexpr auto expected_default_memory_bytes = Memory::GiB * 20;

    // Provide some empty argument in order for UnitigDistanceOptions constructor to not return early.
    MockArgumentsBuilder<bool> test_arguments({ "" });
    UnitigDistanceOptions options(test_arguments.argc(), test_arguments.argv());

    ASSERT_EQUAL(options.memory_bytes(), expected_default_memory_bytes);

    return true;
}

bool test_unit_unitig_distance_options_memory_arg_with_M_suffix() {
    constexpr auto expected_memory_bytes = Memory::MiB * 365;

    for (const auto& option : { MEMORY_OPTION, MEMORY_LONG_OPTION }) {
        MockArgumentsBuilder<> test_arguments({ option, "365M" });
        UnitigDistanceOptions options(test_arguments.argc(), test_arguments.argv());

        const auto memory_bytes = options.memory_bytes();

        ASSERT_EQUAL(memory_bytes, expected_memory_bytes);
    }

    return true;
}

bool test_unit_unitig_distance_options_memory_arg_with_G_suffix() {
    constexpr auto expected_memory_bytes = Memory::GiB * 25;

    for (const auto& option : { MEMORY_OPTION, MEMORY_LONG_OPTION }) {
        MockArgumentsBuilder<> test_arguments({ option, "25G" });
        UnitigDistanceOptions options(test_arguments.argc(), test_arguments.argv());

        const auto memory_bytes = options.memory_bytes();

        ASSERT_EQUAL(memory_bytes, expected_memory_bytes);
    }

    return true;
}

bool test_unit_unitig_distance_options_memory_arg_with_bad_suffix() {
    for (const auto& option : { MEMORY_OPTION, MEMORY_LONG_OPTION }) {
        MockArgumentsBuilder<> test_arguments_M_no_value({ option, "M" });
        EXPECT_THROW(UnitigDistanceOptions(test_arguments_M_no_value.argc(), test_arguments_M_no_value.argv()),
                     ErrorCode::INVALID_PROGRAM_OPTION);

        MockArgumentsBuilder<> test_arguments_G_no_value({ option, "G" });
        EXPECT_THROW(UnitigDistanceOptions(test_arguments_G_no_value.argc(), test_arguments_G_no_value.argv()),
                     ErrorCode::INVALID_PROGRAM_OPTION);
    }

    return true;
}

bool test_unit_unitig_distance_options_memory_arg_too_large_throws() {
    const auto memory_overflow_mib_value = std::numeric_limits<std::size_t>::max() / Memory::MiB + 1;
    const auto memory_overflow_mib_value_string = std::to_string(memory_overflow_mib_value) + "M";
    const auto memory_overflow_gib_value = std::numeric_limits<std::size_t>::max() / Memory::GiB + 1;
    const auto memory_overflow_gib_value_string = std::to_string(memory_overflow_gib_value) + "G";

    for (const auto& option : { MEMORY_OPTION, MEMORY_LONG_OPTION }) {
        MockArgumentsBuilder<> test_arguments_mib({option, memory_overflow_mib_value_string.c_str()});
        EXPECT_THROW(UnitigDistanceOptions(test_arguments_mib.argc(), test_arguments_mib.argv()),
                     ErrorCode::INVALID_PROGRAM_OPTION);

        MockArgumentsBuilder<> test_arguments_gib({option, memory_overflow_gib_value_string.c_str()});
        EXPECT_THROW(UnitigDistanceOptions(test_arguments_gib.argc(), test_arguments_gib.argv()),
                     ErrorCode::INVALID_PROGRAM_OPTION);
    }

    return true;
}

bool test_unit_unitig_distance_options_n_workers_equal_to_n_threads_when_multithreaded() {
    // Check the default value.
    MockArgumentsBuilder<> thread_0_arguments({ N_THREADS_OPTION, "0" });
    UnitigDistanceOptions thread_0_options(thread_0_arguments.argc(), thread_0_arguments.argv());
    ASSERT_EQUAL(thread_0_options.n_workers(), 0);

    // Check threads set explicitly to 1.
    MockArgumentsBuilder<> thread_1_arguments({ N_THREADS_OPTION, "1" });
    UnitigDistanceOptions thread_1_options(thread_1_arguments.argc(), thread_1_arguments.argv());
    ASSERT_EQUAL(thread_1_options.n_workers(), 0);

    // Check multi-threaded value.
    MockArgumentsBuilder<> thread_5_arguments({ N_THREADS_OPTION, "5" });
    UnitigDistanceOptions thread_5_options(thread_5_arguments.argc(), thread_5_arguments.argv());
    ASSERT_EQUAL(thread_5_options.n_workers(), thread_5_options.n_threads());

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    // Defined for `PROGRAM_OPTIONS_TEST` macro.
    const auto* suite_name = "test_unit_unitig_distance_options";
    using ProgramOptionsT = UnitigDistanceOptions;

    const auto test_unit_unitig_distance_options_verbose_by_default =
        test_unit_program_options_verbose_by_default<ProgramOptionsT>;

    const auto test_unit_unitig_distance_options_quiet_sets_verbose_false =
        test_unit_program_options_quiet_sets_verbose_false<ProgramOptionsT>;

    const auto test_unit_unitig_distance_options_0_n_queries_defaults_to_max =
        test_unit_program_options_option_argument_defaults_to_value<ProgramOptionsT>(
            N_QUERIES_OPTION,
            N_QUERIES_LONG_OPTION,
            "0",
            std::numeric_limits<std::size_t>::max(),
            OPTION_ACCESSOR(n_queries));

    const auto test_unit_unitig_distance_options_0_n_threads_defaults_to_one =
        test_unit_program_options_option_argument_defaults_to_value<ProgramOptionsT>(
            N_THREADS_OPTION,
            N_THREADS_LONG_OPTION,
            "0",
            1,
            OPTION_ACCESSOR(n_threads));

    const auto test_unit_unitig_distance_options_bad_arg_value_to_k =
        test_unit_program_options_bad_arg_value<ProgramOptionsT>(K_OPTION, K_LONG_OPTION);

    const auto test_unit_unitig_distance_options_bad_arg_value_to_n_threads =
        test_unit_program_options_bad_arg_value<ProgramOptionsT>(N_THREADS_OPTION, N_THREADS_LONG_OPTION);

    const auto test_unit_unitig_distance_options_bad_arg_value_to_n_queries =
        test_unit_program_options_bad_arg_value<ProgramOptionsT>(N_QUERIES_OPTION, N_QUERIES_LONG_OPTION);

    const auto tests = {
        PROGRAM_OPTIONS_TEST(unitigs_filename, UNITIGS_FILENAME),
        PROGRAM_OPTIONS_TEST(queries_filename, QUERIES_FILENAME),
        PROGRAM_OPTIONS_TEST(sgg_paths_filename, SGG_PATHS_FILENAME),
        PROGRAM_OPTIONS_TEST(k, K),
        PROGRAM_OPTIONS_TEST(n_queries, N_QUERIES),
        PROGRAM_OPTIONS_TEST(n_threads, N_THREADS),
        PROGRAM_OPTIONS_TEST(queries_one_based, QUERIES_ONE_BASED),
        PROGRAM_OPTIONS_TEST(no_median_distance, NO_MEDIAN),
        PROGRAM_OPTIONS_TEST(output_one_based, OUTPUT_ONE_BASED),
        TEST(test_unit_unitig_distance_options_verbose_by_default),
        TEST(test_unit_unitig_distance_options_quiet_sets_verbose_false),
        TEST(test_unit_unitig_distance_options_one_based_output_set_correctly),
        TEST(test_unit_unitig_distance_options_out_filename_set_correctly),
        TEST(test_unit_unitig_distance_options_out_filename_default_value_set_correctly),
        TEST(test_unit_unitig_distance_options_0_n_queries_defaults_to_max),
        TEST(test_unit_unitig_distance_options_0_n_threads_defaults_to_one),
        TEST(test_unit_unitig_distance_options_bad_arg_value_to_k),
        TEST(test_unit_unitig_distance_options_bad_arg_value_to_n_threads),
        TEST(test_unit_unitig_distance_options_bad_arg_value_to_n_queries),
        TEST(test_unit_unitig_distance_options_memory_arg_default_value),
        TEST(test_unit_unitig_distance_options_memory_arg_without_suffix_assumes_gib),
        TEST(test_unit_unitig_distance_options_memory_arg_with_M_suffix),
        TEST(test_unit_unitig_distance_options_memory_arg_with_G_suffix),
        TEST(test_unit_unitig_distance_options_memory_arg_with_bad_suffix),
        TEST(test_unit_unitig_distance_options_memory_arg_too_large_throws),
        TEST(test_unit_unitig_distance_options_n_workers_equal_to_n_threads_when_multithreaded),
    };

    return Test::run_suite(suite_name, tests);
}
