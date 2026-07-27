/*
 * test_unit_unitig_distance.cpp - Unit tests for unitig_distance.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <iostream>
#include <sstream>

#include "common/program_options/program_options_option_names.hpp"
#include "common/test_harness/Test.hpp"
#include "mocks/MockArgumentsBuilder.hpp"
#include "unitig_distance/program_options/UnitigDistanceOptions.hpp"
#include "unitig_distance/program_options/unitig_distance_option_names.hpp"
#include "unitig_distance/unitig_distance.hpp"

namespace PANGWES {
namespace {

using Mocks::MockArgumentsBuilder;

constexpr auto test_unitigs_filename = "test.unitigs";
constexpr auto test_queries_filename = "test.queries";
constexpr auto test_sgg_paths_filename = "test.paths";
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

bool test_unit_check_unitig_distance_options_accepts_valid_options() {
    MockArgumentsBuilder<> test_arguments({ UNITIGS_FILENAME_OPTION, test_unitigs_filename,
                                            QUERIES_FILENAME_OPTION, test_queries_filename,
                                            SGG_PATHS_FILENAME_OPTION, test_sgg_paths_filename,
                                            K_OPTION, test_valid_k });
    UnitigDistanceOptions options(test_arguments.argc(), test_arguments.argv());

    ASSERT_TRUE(check_unitig_distance_options(options));

    return true;
}

bool test_unit_check_unitig_distance_options_without_unitigs_file_returns_false() {
    MockArgumentsBuilder<> test_arguments({ QUERIES_FILENAME_OPTION, test_queries_filename,
                                            SGG_PATHS_FILENAME_OPTION, test_sgg_paths_filename,
                                            K_OPTION, test_valid_k });
    UnitigDistanceOptions options(test_arguments.argc(), test_arguments.argv());

    ASSERT_FALSE(check_unitig_distance_options(options));

    return true;
}

bool test_unit_check_unitig_distance_options_without_queries_file_returns_false() {
    MockArgumentsBuilder<> test_arguments({ UNITIGS_FILENAME_OPTION, test_unitigs_filename,
                                            SGG_PATHS_FILENAME_OPTION, test_sgg_paths_filename,
                                            K_OPTION, test_valid_k });
    UnitigDistanceOptions options(test_arguments.argc(), test_arguments.argv());

    ASSERT_FALSE(check_unitig_distance_options(options));

    return true;
}

bool test_unit_check_unitig_distance_options_without_sgg_paths_file_returns_false() {
    MockArgumentsBuilder<> test_arguments({ UNITIGS_FILENAME_OPTION, test_unitigs_filename,
                                            QUERIES_FILENAME_OPTION, test_queries_filename,
                                            K_OPTION, test_valid_k });
    UnitigDistanceOptions options(test_arguments.argc(), test_arguments.argv());

    ASSERT_FALSE(check_unitig_distance_options(options));

    return true;
}

bool test_unit_check_unitig_distance_options_without_k_returns_false() {
    MockArgumentsBuilder<> test_arguments({ UNITIGS_FILENAME_OPTION, test_unitigs_filename,
                                            QUERIES_FILENAME_OPTION, test_queries_filename,
                                            SGG_PATHS_FILENAME_OPTION, test_sgg_paths_filename });
    UnitigDistanceOptions options(test_arguments.argc(), test_arguments.argv());

    ASSERT_FALSE(check_unitig_distance_options(options));

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_unit_check_unitig_distance_options_accepts_valid_options),
        TEST(test_unit_check_unitig_distance_options_without_unitigs_file_returns_false),
        TEST(test_unit_check_unitig_distance_options_without_queries_file_returns_false),
        TEST(test_unit_check_unitig_distance_options_without_sgg_paths_file_returns_false),
        TEST(test_unit_check_unitig_distance_options_without_k_returns_false),
    };

    return Test::run_suite("test_unit_unitig_distance", tests, test_setup, test_teardown);
}
