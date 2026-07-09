/*
 * test_unit_exception.cpp - Unit tests for utils/Exception.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <string>

#include "common/test_harness/Test.hpp"
#include "common/utils/Exception.hpp"

namespace PANGWES {
namespace {

constexpr auto code_idx_end = Traits::to_underlying(ErrorCode::end);

// Verify the constructor works on all valid error codes. Also tests to_description() because the constructor uses it.
bool test_unit_exception_constructor() {
    for (auto code_idx = 0; code_idx <= code_idx_end; ++code_idx) {
        const auto error_code = static_cast<ErrorCode>(code_idx);
        const Exception exception(error_code);

        ASSERT_EQUAL(exception.what(), to_description(error_code));
        ASSERT_ENUMS_EQUAL(exception.error_code(), error_code);
        ASSERT_NOT_EQUAL(to_description(error_code), "Unknown error code");
    }

    return true;
}

// Additionally to test_unit_exception_constructor() steps, check what() with the additional context constructor.
bool test_unit_exception_constructor_context() {
    for (auto code_idx = 0; code_idx <= code_idx_end; ++code_idx) {
        const auto error_code = static_cast<ErrorCode>(code_idx);

        const Exception exception1(error_code, "test context");
        ASSERT_EQUAL(exception1.what(), to_description(error_code) + " (test context)");
        ASSERT_ENUMS_EQUAL(exception1.error_code(), error_code);

        const Exception exception2(error_code, "test ", "context");
        ASSERT_EQUAL(exception2.what(), to_description(error_code) + " (test context)");
        ASSERT_ENUMS_EQUAL(exception2.error_code(), error_code);

        const Exception exception3(error_code, 1, "a", 'b', std::string{"c"});
        ASSERT_EQUAL(exception3.what(), to_description(error_code) + " (1abc)");
        ASSERT_ENUMS_EQUAL(exception3.error_code(), error_code);
    }

    return true;
}

// Verify to_string() returns the enumerator name for every error code.
bool test_unit_exception_to_string() {
// Expand ERROR_CODES into one assertion per error code.
#define ERROR_CODE_OP(error_code_) ASSERT_EQUAL(to_string(ErrorCode::error_code_), #error_code_);
    ERROR_CODES
#undef ERROR_CODE_OP

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_unit_exception_constructor),
        TEST(test_unit_exception_constructor_context),
        TEST(test_unit_exception_to_string),
    };

    return Test::run_suite("test_unit_exception", tests);
}
