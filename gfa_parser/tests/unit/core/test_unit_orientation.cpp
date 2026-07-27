/*
 * test_unit_orientation.cpp - Unit tests for core/Orientation.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include "common/test_harness/Test.hpp"
#include "common/utils/Exception.hpp"
#include "gfa_parser/core/Orientation.hpp"

namespace PANGWES {
namespace {

constexpr char PLUS_ORIENTATION_CHAR = '+';
constexpr char MINUS_ORIENTATION_CHAR = '-';

bool test_unit_orientation_valid_orientation_char() {
    const auto orientation_plus = parse_orientation(PLUS_ORIENTATION_CHAR);
    ASSERT_ENUMS_EQUAL(orientation_plus, Orientation::PLUS);

    const auto orientation_minus = parse_orientation(MINUS_ORIENTATION_CHAR);
    ASSERT_ENUMS_EQUAL(orientation_minus, Orientation::MINUS);

    return true;
}

bool test_unit_orientation_invalid_orientation_char_throws() {
    for (auto chr = '!'; chr <= '~'; ++chr) {
        if (chr == PLUS_ORIENTATION_CHAR || chr == MINUS_ORIENTATION_CHAR) {
            continue;
        }

        EXPECT_THROW(parse_orientation(chr), ErrorCode::INVALID_DATA);
    }

    EXPECT_THROW(parse_orientation(' '), ErrorCode::INVALID_DATA);
    EXPECT_THROW(parse_orientation('\t'), ErrorCode::INVALID_DATA);
    EXPECT_THROW(parse_orientation('\0'), ErrorCode::INVALID_DATA);

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_unit_orientation_valid_orientation_char),
        TEST(test_unit_orientation_invalid_orientation_char_throws),
    };

    return Test::run_suite("test_unit_orientation", tests);
}
