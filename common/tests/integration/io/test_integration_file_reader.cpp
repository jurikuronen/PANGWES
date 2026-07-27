/*
 * test_integration_file_reader.cpp - Integration tests for io/FileReader.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <string>

#include "common/io/FileReader.hpp"
#include "common/test_harness/Test.hpp"

namespace PANGWES {
namespace {

constexpr auto test_file = TEST_DATA_DIR "/test_efc_k31.fasta";

bool test_integration_file_reader_reopening_resets_line_number() {
    FileReader file_reader(test_file);

    file_reader.open();

    std::string line;
    ASSERT_EQUAL(file_reader.line_number(), 0);
    ASSERT_TRUE(file_reader.getline(line));
    ASSERT_EQUAL(file_reader.line_number(), 1);

    file_reader.close();
    file_reader.open();

    ASSERT_EQUAL(file_reader.line_number(), 0);

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_integration_file_reader_reopening_resets_line_number),
    };

    return Test::run_suite("test_integration_file_reader", tests);
}
