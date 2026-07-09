/*
 * test_unit_file_reader.cpp - Unit tests for io/FileReader.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <string>
#include <vector>

#include "common/test_harness/Test.hpp"
#include "common/utils/Exception.hpp"
#include "mocks/MockFileReader.hpp"

namespace PANGWES {
namespace {

constexpr auto test_line = "test_line";

bool test_setup() {
    /*
     * Clean slate. This also resets is_open()'s mocked state to default "working ones", so tests should carefully
     * specify them.
     *
     * Note that MockReader doesn't require opening/closing the file for getline().
    */
    Mocks::MockIfStream::set_contents({});
    Mocks::MockIfStream::set_fail_result(false);

    return true;
}

// This test verifies that the constructor does not throw in normal conditions.
bool test_unit_file_reader_constructor() {
    auto mock_reader = Mocks::MockFileReader(Mocks::mock_file);

    ASSERT_EQUAL(mock_reader.filename(), Mocks::mock_file);
    ASSERT_EQUAL(mock_reader.line_number(), 0);

    return true;
}

bool test_unit_file_reader_constructor_no_filename_provided() {
    EXPECT_THROW(Mocks::MockFileReader(""), ErrorCode::EMPTY_FILENAME);

    return true;
}

bool test_unit_file_reader_open_failing_stream_open_throws() {
    // Set is_open()'s mocked result to false for both the initial check and after having called the stream's open().
    Mocks::MockIfStream::set_is_open_return({false, false});

    auto mock_reader = Mocks::MockFileReader(Mocks::mock_file);

    EXPECT_THROW(mock_reader.open(), ErrorCode::FAILED_TO_OPEN_FILE);

    return true;
}

bool test_unit_file_reader_open_on_empty_file_throws() {
    // Set is_open()'s mocked result to false for the initial check and true after calling the stream's open().
    Mocks::MockIfStream::set_is_open_return({true, false});
    Mocks::MockIfStream::set_peek_return(false);

    auto mock_reader = Mocks::MockFileReader(Mocks::mock_file);

    EXPECT_THROW(mock_reader.open(), ErrorCode::FILE_EMPTY);

    return true;
}

bool test_unit_file_reader_close_without_open_first_throws() {
    // Set is_open()'s mocked result to false for the initial check.
    Mocks::MockIfStream::set_is_open_return({false});

    auto mock_reader = Mocks::MockFileReader(Mocks::mock_file);

    EXPECT_THROW(mock_reader.close(), ErrorCode::FAILED_TO_CLOSE_FILE);

    return true;
}

bool test_unit_file_reader_close_failure_throws() {
    Mocks::MockIfStream::set_is_open_return({true});
    Mocks::MockIfStream::set_fail_result(true);

    auto mock_reader = Mocks::MockFileReader(Mocks::mock_file);

    EXPECT_THROW(mock_reader.close(), ErrorCode::FAILED_TO_CLOSE_FILE);

    return true;
}

bool test_unit_file_reader_open_twice_throws() {
    auto mock_reader = Mocks::MockFileReader(Mocks::mock_file);

    // Set any contents so opening won't fail with FILE_EMPTY.
    Mocks::MockIfStream::set_contents({test_line});
    Mocks::MockIfStream::set_peek_return(true);

    Mocks::MockIfStream::set_is_open_return({true, true, false});

    // Consumes "false" (initial check), then "true" (is_open() after stream's open()).
    mock_reader.open();

    // Throws on last "true": attempting to open a file whose stream already reports is_open() == true.
    EXPECT_THROW(mock_reader.open(), ErrorCode::FAILED_TO_OPEN_FILE);

    return true;
}

bool test_unit_file_reader_getline() {
    auto mock_reader = Mocks::MockFileReader(Mocks::mock_file);

    Mocks::MockIfStream::set_contents({test_line});

    std::string line;
    const auto result = mock_reader.getline(line);

    ASSERT_TRUE(result);
    ASSERT_EQUAL(line, test_line);
    ASSERT_EQUAL(mock_reader.line_number(), 1);

    return true;
}

bool test_unit_file_reader_getline_at_end_of_file() {
    auto mock_reader = Mocks::MockFileReader(Mocks::mock_file);

    Mocks::MockIfStream::set_contents({test_line});

    std::string line;
    const auto result_first_getline = mock_reader.getline(line);

    ASSERT_TRUE(result_first_getline);
    ASSERT_EQUAL(line, test_line);
    ASSERT_EQUAL(mock_reader.line_number(), 1);

    line.clear();
    const auto result_second_getline = mock_reader.getline(line);

    ASSERT_FALSE(result_second_getline);
    ASSERT_TRUE(line.empty());
    ASSERT_EQUAL(mock_reader.line_number(), 1);

    return true;
}

bool test_unit_file_reader_getline_multiple_lines() {
    auto mock_reader = Mocks::MockFileReader(Mocks::mock_file);

    const auto expected_lines = std::vector<std::string>{"test1", "test 2", "t e s t 3", "t4", "t 5"};
    Mocks::MockIfStream::set_contents(expected_lines);

    for (std::size_t i = 0; i < expected_lines.size(); ++i) {
        const auto& expected_line = expected_lines[i];
        const auto expected_line_number = i + 1;

        std::string line;
        const auto result = mock_reader.getline(line);

        ASSERT_TRUE(result);
        ASSERT_EQUAL(line, expected_line);
        ASSERT_EQUAL(mock_reader.line_number(), expected_line_number);
    }

    return true;
}

bool test_unit_file_reader_getline_with_empty_lines() {
    auto mock_reader = Mocks::MockFileReader(Mocks::mock_file);

    const auto expected_lines = std::vector<std::string>{"a", "", "", "", "a", ""};
    Mocks::MockIfStream::set_contents(expected_lines);

    for (std::size_t i = 0; i < expected_lines.size(); ++i) {
        const auto& expected_line = expected_lines[i];
        const auto expected_line_number = i + 1;

        std::string line;
        const auto result = mock_reader.getline(line);

        ASSERT_TRUE(result);
        ASSERT_EQUAL(line, expected_line);
        ASSERT_EQUAL(mock_reader.line_number(), expected_line_number);
    }

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_unit_file_reader_constructor),
        TEST(test_unit_file_reader_constructor_no_filename_provided),
        TEST(test_unit_file_reader_open_failing_stream_open_throws),
        TEST(test_unit_file_reader_open_on_empty_file_throws),
        TEST(test_unit_file_reader_close_without_open_first_throws),
        TEST(test_unit_file_reader_close_failure_throws),
        TEST(test_unit_file_reader_open_twice_throws),
        TEST(test_unit_file_reader_getline),
        TEST(test_unit_file_reader_getline_at_end_of_file),
        TEST(test_unit_file_reader_getline_multiple_lines),
        TEST(test_unit_file_reader_getline_with_empty_lines),
    };

    return Test::run_suite("test_unit_file_reader", tests, test_setup);
}
