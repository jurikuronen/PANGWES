/*
 * test_unit_parser_utils.cpp - Unit tests for core/parser_utils.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include "common/test_harness/Test.hpp"
#include "common/utils/Exception.hpp"
#include "gfa_parser/core/parser_utils.hpp"

namespace PANGWES {
namespace {

bool test_unit_parser_utils_complement_base() {
    ASSERT_EQUAL(ParserUtils::complement_base('A'), 'T');
    ASSERT_EQUAL(ParserUtils::complement_base('C'), 'G');
    ASSERT_EQUAL(ParserUtils::complement_base('G'), 'C');
    ASSERT_EQUAL(ParserUtils::complement_base('T'), 'A');

    ASSERT_EQUAL(ParserUtils::complement_base('a'), 't');
    ASSERT_EQUAL(ParserUtils::complement_base('c'), 'g');
    ASSERT_EQUAL(ParserUtils::complement_base('g'), 'c');
    ASSERT_EQUAL(ParserUtils::complement_base('t'), 'a');

    return true;
}

bool test_unit_parser_utils_complement_base_throws_on_invalid_base() {
    EXPECT_THROW(ParserUtils::complement_base('0'), ErrorCode::INVALID_DATA);

    return true;
}

/*
 * Check the examples given in GFA 1.0 Format Specification:
 *   H    VN:Z:1.0
 *   S    11    ACCTT
 *   S    12    TCAAGG
 *   S    13    CTTGATT
 *   L    11    +    12    -    4M
 *   L    12    -    13    +    5M
 *   L    11    +    13    +    3M
*/
bool test_unit_parser_utils_is_exact_overlap() {
    const auto* segment_11 = "ACCTT";
    const auto* segment_12 = "TCAAGG";
    const auto* segment_13 = "CTTGATT";

    ASSERT_TRUE(ParserUtils::is_exact_overlap( segment_11, segment_12, Orientation::PLUS,  Orientation::MINUS, 4));
    ASSERT_FALSE(ParserUtils::is_exact_overlap(segment_11, segment_12, Orientation::PLUS,  Orientation::PLUS,  4));
    ASSERT_FALSE(ParserUtils::is_exact_overlap(segment_11, segment_12, Orientation::MINUS, Orientation::PLUS,  4));
    ASSERT_FALSE(ParserUtils::is_exact_overlap(segment_11, segment_12, Orientation::MINUS, Orientation::MINUS, 4));

    ASSERT_TRUE(ParserUtils::is_exact_overlap( segment_12, segment_13, Orientation::MINUS, Orientation::PLUS,  5));
    ASSERT_FALSE(ParserUtils::is_exact_overlap(segment_12, segment_13,  Orientation::PLUS, Orientation::PLUS,  5));
    ASSERT_FALSE(ParserUtils::is_exact_overlap(segment_12, segment_13,  Orientation::PLUS, Orientation::MINUS, 5));
    ASSERT_FALSE(ParserUtils::is_exact_overlap(segment_12, segment_13, Orientation::MINUS, Orientation::MINUS, 5));

    ASSERT_TRUE(ParserUtils::is_exact_overlap( segment_11, segment_13, Orientation::PLUS,  Orientation::PLUS,  3));
    ASSERT_FALSE(ParserUtils::is_exact_overlap(segment_11, segment_13, Orientation::PLUS,  Orientation::MINUS, 3));
    ASSERT_FALSE(ParserUtils::is_exact_overlap(segment_11, segment_13, Orientation::MINUS, Orientation::PLUS,  3));
    ASSERT_FALSE(ParserUtils::is_exact_overlap(segment_11, segment_13, Orientation::MINUS, Orientation::MINUS, 3));

    return true;
}

bool test_unit_parser_utils_is_exact_overlap_returns_false_on_too_large_overlap() {
    ASSERT_FALSE(ParserUtils::is_exact_overlap("AAA", "ACT", Orientation::PLUS, Orientation::PLUS, 3));
    ASSERT_FALSE(ParserUtils::is_exact_overlap("AAA", "ACT", Orientation::PLUS, Orientation::PLUS, 5));

    return true;
}

bool test_unit_parser_utils_parse_cuttlefish_path_name() {
    const auto parsed_path_name = ParserUtils::parse_cuttlefish_path_name("Reference:1_Sequence:1.fa");

    ASSERT_EQUAL(parsed_path_name.reference_name, "1");
    ASSERT_EQUAL(parsed_path_name.sequence_name, "1.fa");

    return true;
}

bool test_unit_parser_utils_parse_cuttlefish_path_name_throws_on_failure() {
    // "_Sequence:" missing.
    EXPECT_THROW(ParserUtils::parse_cuttlefish_path_name("Reference:1_1.fa"), ErrorCode::INVALID_DATA);

    // "Reference:" missing.
    EXPECT_THROW(ParserUtils::parse_cuttlefish_path_name("1_Sequence:1.fa"), ErrorCode::INVALID_DATA);

    // Both missing.
    EXPECT_THROW(ParserUtils::parse_cuttlefish_path_name("1_1.fa"), ErrorCode::INVALID_DATA);

    // Reversed order.
    EXPECT_THROW(ParserUtils::parse_cuttlefish_path_name("_Sequence:1.faReference:1"), ErrorCode::INVALID_DATA);

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_unit_parser_utils_complement_base),
        TEST(test_unit_parser_utils_complement_base_throws_on_invalid_base),
        TEST(test_unit_parser_utils_is_exact_overlap),
        TEST(test_unit_parser_utils_is_exact_overlap_returns_false_on_too_large_overlap),
        TEST(test_unit_parser_utils_parse_cuttlefish_path_name),
        TEST(test_unit_parser_utils_parse_cuttlefish_path_name_throws_on_failure),
    };

    return Test::run_suite("test_unit_parser_utils", tests);
}
