/*
 * test_unit_reference_path_data.cpp - Unit tests for core/ReferencePathData.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <atomic>
#include <cstddef>
#include <string>
#include <thread>
#include <vector>

#include "common/test_harness/Test.hpp"
#include "common/utils/Exception.hpp"
#include "gfa_parser/core/ReferencePathData.hpp"

namespace PANGWES {
namespace {

bool test_unit_reference_path_data_constructor_builds_fasta_header() {
    constexpr auto test_reference1_name = "1";
    constexpr auto test_reference1_first_sequence_name = "seq1.1.fa";

    const auto fasta_header = std::string{test_reference1_name} + "_" + test_reference1_first_sequence_name;

    ReferencePathData reference_path_data(test_reference1_name, test_reference1_first_sequence_name);

    const auto& constructed_fasta_header = reference_path_data.string_for_fasta_header();

    ASSERT_EQUAL(constructed_fasta_header, fasta_header);

    return true;
}

bool test_unit_reference_path_data_increment_and_decrement_sequence_count() {
    constexpr auto max_change_count_times = 100;

    for (std::size_t change_count_times = 1; change_count_times < max_change_count_times; ++change_count_times) {
        ReferencePathData reference_path_data{};

        for (std::size_t increase_count = 0; increase_count < change_count_times; ++increase_count) {
            reference_path_data.increment_sequence_count();
        }

        for (std::size_t decrease_count = 0; decrease_count < change_count_times - 1; ++decrease_count) {
            ASSERT_FALSE(reference_path_data.decrement_sequence_count_and_check_if_all_sequences_processed());
        }

        // Perform the final decrement, all sequences should now be processed.
        ASSERT_TRUE(reference_path_data.decrement_sequence_count_and_check_if_all_sequences_processed());
    }

    return true;
}

bool test_unit_reference_path_data_decrement_sequence_count_at_count_zero_throws() {
    ReferencePathData reference_path_data{};

    EXPECT_THROW(reference_path_data.decrement_sequence_count_and_check_if_all_sequences_processed(),
                 ErrorCode::INVALID_STATE);

    return true;
}

bool test_unit_reference_path_data_initialize_unitig_occurrence_data_with_0_segments_throws() {
    ReferencePathData reference_path_data{};

    EXPECT_THROW(reference_path_data.initialize_unitig_occurrence_data(0), ErrorCode::INVALID_ARGUMENT);

    return true;
}

bool test_unit_reference_path_data_mark_unitig_presence() {
    constexpr std::size_t test_n_segments = 6;

    const auto empty_unitig_occurrence_data = std::vector<bool>{false, false, false, false, false, false};
    const auto expected_unitig_occurrence_data = std::vector<bool>{true, false, true, false, false, true};

    // Check that `take_unitig_occurrence_data()` verifies an all-false vector with nothing marked.
    ReferencePathData reference_path_data_nothing_marked{};
    reference_path_data_nothing_marked.initialize_unitig_occurrence_data(test_n_segments);

    const auto unitig_occurrence_data_nothing_marked = reference_path_data_nothing_marked.take_unitig_occurrence_data();

    ASSERT_CONTAINERS_EQUAL(unitig_occurrence_data_nothing_marked, empty_unitig_occurrence_data);

    // Mark some unitig occurrences and check that the correct fields are set to true.
    ReferencePathData reference_path_data{};
    reference_path_data.initialize_unitig_occurrence_data(test_n_segments);

    reference_path_data.mark_unitig_presence({0, 2, 5, 2});
    const auto unitig_occurrence_data = reference_path_data.take_unitig_occurrence_data();

    ASSERT_CONTAINERS_EQUAL(unitig_occurrence_data, expected_unitig_occurrence_data);

    return true;
}

bool test_unit_reference_path_data_mark_unitig_presence_without_initialization_throws() {
    ReferencePathData reference_path_data{};

    EXPECT_THROW(reference_path_data.mark_unitig_presence({0}), ErrorCode::INVALID_STATE);

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_unit_reference_path_data_constructor_builds_fasta_header),
        TEST(test_unit_reference_path_data_increment_and_decrement_sequence_count),
        TEST(test_unit_reference_path_data_decrement_sequence_count_at_count_zero_throws),
        TEST(test_unit_reference_path_data_initialize_unitig_occurrence_data_with_0_segments_throws),
        TEST(test_unit_reference_path_data_mark_unitig_presence),
        TEST(test_unit_reference_path_data_mark_unitig_presence_without_initialization_throws),
    };

    return Test::run_suite("test_unit_reference_path_data", tests);
}
