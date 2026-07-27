/*
 * test_unit_segment_name_map.cpp - Unit tests for core/SegmentNameMap.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include "common/test_harness/Test.hpp"
#include "common/utils/Exception.hpp"
#include "gfa_parser/core/SegmentNameMap.hpp"

namespace PANGWES {
namespace {

constexpr auto test_name_1 = "1";
constexpr auto test_sequence_1 = "ACGT";

constexpr auto test_name_2 = "2";
constexpr auto test_sequence_2 = "TGCA";

bool test_unit_segment_name_map_empty_by_default() {
    SegmentNameMap segment_name_map{};

    ASSERT_TRUE(segment_name_map.empty());
    ASSERT_EQUAL(segment_name_map.size(), 0);
    ASSERT_FALSE(segment_name_map.contains(test_name_1));
    ASSERT_EQUAL(segment_name_map.segment_name_mapping(test_name_1), SEGMENT_NOT_MAPPED);

    return true;
}

bool test_unit_segment_name_map_maps_names_to_sequences_and_indices() {
    constexpr auto expected_mapping_1 = 0;
    constexpr auto expected_mapping_2 = 1;
    constexpr auto unmapped_name = "3";

    SegmentNameMap segment_name_map{};

    const auto mapping_1 = segment_name_map.add_and_map_segment(test_name_1, test_sequence_1);
    const auto mapping_2 = segment_name_map.add_and_map_segment(test_name_2, test_sequence_2);

    ASSERT_EQUAL(mapping_1, expected_mapping_1);
    ASSERT_EQUAL(mapping_2, expected_mapping_2);

    ASSERT_FALSE(segment_name_map.empty());
    ASSERT_EQUAL(segment_name_map.size(), 2);

    ASSERT_TRUE(segment_name_map.contains(test_name_1));
    ASSERT_TRUE(segment_name_map.contains(test_name_2));
    ASSERT_FALSE(segment_name_map.contains(unmapped_name));

    ASSERT_EQUAL(segment_name_map.segment_name_mapping(test_name_1), expected_mapping_1);
    ASSERT_EQUAL(segment_name_map.segment_name_mapping(test_name_2), expected_mapping_2);
    ASSERT_EQUAL(segment_name_map.segment_name_mapping(unmapped_name), SEGMENT_NOT_MAPPED);

    ASSERT_EQUAL(segment_name_map.segment_name(expected_mapping_1), test_name_1);
    ASSERT_EQUAL(segment_name_map.segment_name(expected_mapping_2), test_name_2);
    ASSERT_EQUAL(segment_name_map.segment_sequence(expected_mapping_1), test_sequence_1);
    ASSERT_EQUAL(segment_name_map.segment_sequence(expected_mapping_2), test_sequence_2);

    return true;
}

bool test_unit_segment_name_map_segment_access_out_of_range() {
    SegmentNameMap segment_name_map{};

    EXPECT_THROW(segment_name_map.segment_name(0), ErrorCode::INDEX_OUT_OF_RANGE);
    EXPECT_THROW(segment_name_map.segment_sequence(0), ErrorCode::INDEX_OUT_OF_RANGE);

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_unit_segment_name_map_empty_by_default),
        TEST(test_unit_segment_name_map_maps_names_to_sequences_and_indices),
        TEST(test_unit_segment_name_map_segment_access_out_of_range),
    };

    return Test::run_suite("test_unit_segment_name_map", tests);
}
