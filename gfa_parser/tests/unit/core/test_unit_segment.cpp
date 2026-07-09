/*
 * test_unit_segment.cpp - Unit tests for core/Segment.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <string>

#include "common/test_harness/Test.hpp"
#include "common/utils/Exception.hpp"
#include "gfa_parser/core/Segment.hpp"

namespace PANGWES {
namespace {

const std::string test_name = "1";
constexpr auto test_sequence = "ACGT";

bool test_unit_segment_constructor() {
    Segment segment(test_name, test_sequence);

    ASSERT_EQUAL(segment.name(), test_name);
    ASSERT_EQUAL(segment.sequence(), test_sequence);

    return true;
}

bool test_unit_segment_constructor_empty_name_throws() {
    EXPECT_THROW(Segment("", test_sequence), ErrorCode::INVALID_DATA);

    return true;
}

bool test_unit_segment_constructor_empty_sequence_throws() {
    EXPECT_THROW(Segment(test_name, ""), ErrorCode::INVALID_DATA);

    return true;
}

bool test_unit_segment_constructor_unspecified_sequence_throws() {
    EXPECT_THROW(Segment(test_name, "*"), ErrorCode::INVALID_DATA);

    return true;
}

bool test_unit_segment_constructor_invalid_characters_in_name_throws() {
    for (int codepoint = 0; codepoint <= 127; ++codepoint) {
        const std::string chr_string(1, static_cast<std::string::value_type>(codepoint));

        Segment segment_single_char(chr_string, test_sequence);
        ASSERT_EQUAL(segment_single_char.name(), chr_string);

        Segment segment_string(test_name + chr_string, test_sequence);
        ASSERT_EQUAL(segment_string.name(), test_name + chr_string);
    }

    for (int codepoint = 128; codepoint <= 255; ++codepoint) {
        const std::string chr_string(1, static_cast<std::string::value_type>(codepoint));

        EXPECT_THROW(Segment(chr_string, test_sequence), ErrorCode::INVALID_DATA);
        EXPECT_THROW(Segment(test_name + chr_string, test_sequence), ErrorCode::INVALID_DATA);
    }

    return true;
}

bool test_unit_segment_constructor_non_alphabetic_characters_in_sequence_throws() {
    for (int codepoint = 0; codepoint <= 255; ++codepoint) {
        const std::string chr_string(1, static_cast<std::string::value_type>(codepoint));

        if ((codepoint >= 'a' && codepoint <= 'z') || (codepoint >= 'A' && codepoint <= 'Z')) {
            Segment segment_single_char(test_name, chr_string);
            ASSERT_EQUAL(segment_single_char.sequence(), chr_string);

            Segment segment_string(test_name, test_sequence + chr_string);
            ASSERT_EQUAL(segment_string.sequence(), test_sequence + chr_string);
        } else {
            EXPECT_THROW(Segment(test_name, chr_string), ErrorCode::INVALID_DATA);
            EXPECT_THROW(Segment(test_name, test_sequence + chr_string), ErrorCode::INVALID_DATA);
        }
    }

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_unit_segment_constructor),
        TEST(test_unit_segment_constructor_empty_name_throws),
        TEST(test_unit_segment_constructor_empty_sequence_throws),
        TEST(test_unit_segment_constructor_unspecified_sequence_throws),
        TEST(test_unit_segment_constructor_invalid_characters_in_name_throws),
        TEST(test_unit_segment_constructor_non_alphabetic_characters_in_sequence_throws),
    };

    return Test::run_suite("test_unit_segment", tests);
}
