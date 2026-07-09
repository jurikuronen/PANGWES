/*
 * test_unit_file_writer.cpp - Unit tests for io/FileWriter.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <string>

#include "common/test_harness/Test.hpp"
#include "common/utils/Exception.hpp"
#include "mocks/MockFileWriter.hpp"

namespace PANGWES {
namespace {

bool test_setup() {
    // Clean slate.
    Mocks::MockOfStream::set_is_open_return(true);
    Mocks::MockOfStream::set_file_already_exists_return(false, false);

    return true;
}

// This test verifies that the constructor does not throw in normal conditions.
bool test_unit_file_writer_constructor() {
    auto mock_writer = Mocks::MockFileWriter(Mocks::mock_file);

    ASSERT_EQUAL(mock_writer.filename(), Mocks::mock_file);

    return true;
}

bool test_unit_file_writer_out() {
    constexpr auto expected_contents = "test output";

    auto mock_writer = Mocks::MockFileWriter(Mocks::mock_file);

    mock_writer.out() << expected_contents;

    ASSERT_EQUAL(Mocks::MockOfStream::contents(), expected_contents);

    return true;
}

bool test_unit_file_writer_constructor_no_filename_provided() {
    EXPECT_THROW(Mocks::MockFileWriter(""), ErrorCode::EMPTY_FILENAME);

    return true;
}

bool test_unit_file_writer_constructor_open_failure() {
    Mocks::MockOfStream::set_is_open_return(false);

    EXPECT_THROW(Mocks::MockFileWriter(Mocks::mock_file), ErrorCode::FAILED_TO_OPEN_FILE);

    return true;
}

bool test_unit_file_writer_constructor_generate_unique_filename() {
    const auto expected_filename = Mocks::mock_file + std::string{".1"};

    Mocks::MockOfStream::set_file_already_exists_return(true, false);

    auto mock_writer = Mocks::MockFileWriter(Mocks::mock_file);

    ASSERT_EQUAL(mock_writer.filename(), expected_filename);

    return true;
}

bool test_unit_file_writer_constructor_generate_unique_filename_failure() {
    Mocks::MockOfStream::set_file_already_exists_return(true, true);

    EXPECT_THROW(Mocks::MockFileWriter(Mocks::mock_file), ErrorCode::FAILED_TO_GENERATE_UNIQUE_NAME);

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_unit_file_writer_constructor),
        TEST(test_unit_file_writer_out),
        TEST(test_unit_file_writer_constructor_no_filename_provided),
        TEST(test_unit_file_writer_constructor_open_failure),
        TEST(test_unit_file_writer_constructor_generate_unique_filename),
        TEST(test_unit_file_writer_constructor_generate_unique_filename_failure),
    };

    return Test::run_suite("test_unit_file_writer", tests, test_setup);
}
