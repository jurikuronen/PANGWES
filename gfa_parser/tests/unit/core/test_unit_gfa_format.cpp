/*
 * test_unit_gfa_format.cpp - Unit tests for core/GFAFormat.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include "common/test_harness/Test.hpp"
#include "common/utils/Exception.hpp"
#include "gfa_parser/core/GFAFormat.hpp"

namespace PANGWES {
namespace {

constexpr auto GFA_FORMAT_1_INT = 1;
constexpr auto GFA_FORMAT_2_INT = 2;

bool test_unit_gfa_format_to_gfa_format() {
    ASSERT_ENUMS_EQUAL(to_gfa_format(GFA_FORMAT_1_INT), GFAFormat::GFA1);
    ASSERT_ENUMS_EQUAL(to_gfa_format(GFA_FORMAT_2_INT), GFAFormat::GFA2);

    return true;
}

bool test_unit_gfa_format_to_gfa_format_invalid_format() {
    EXPECT_THROW(to_gfa_format(GFA_FORMAT_1_INT - 1), ErrorCode::INVALID_GFA_FORMAT);
    EXPECT_THROW(to_gfa_format(GFA_FORMAT_2_INT + 1), ErrorCode::INVALID_GFA_FORMAT);

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_unit_gfa_format_to_gfa_format),
        TEST(test_unit_gfa_format_to_gfa_format_invalid_format),
    };

    return Test::run_suite("test_unit_gfa_format", tests);
}
