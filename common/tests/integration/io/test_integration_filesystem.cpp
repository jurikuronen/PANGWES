/*
 * test_integration_filesystem.cpp - Integration tests for io/filesystem.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <cstddef>
#include <string>

#include "common/io/FileWriter.hpp"
#include "common/io/filesystem.hpp"
#include "common/test_harness/Test.hpp"
#include "common/utils/Exception.hpp"

namespace PANGWES {
namespace {

constexpr auto test_paths_dir = TEST_DATA_DIR "/test_efc_k31_paths";

bool test_integration_filesystem_file_size() {
    constexpr auto test_file = TEST_DATA_DIR "/test_efc_k31.fasta";
    constexpr auto test_file_size_bytes = 149856;

    ASSERT_EQUAL(Filesystem::file_size(test_file), test_file_size_bytes);

    return true;
}

bool test_integration_filesystem_file_size_empty_filename_throws() {
    EXPECT_THROW(Filesystem::file_size(""), ErrorCode::EMPTY_FILENAME);

    return true;
}

bool test_integration_filesystem_file_size_nonexistent_file_throws() {
    EXPECT_THROW(Filesystem::file_size(TEST_DATA_DIR "/nonexistent_file"), ErrorCode::FAILED_TO_GET_FILE_SIZE);

    return true;
}

bool test_integration_filesystem_file_size_directory_throws() {
    EXPECT_THROW(Filesystem::file_size(test_paths_dir), ErrorCode::FAILED_TO_GET_FILE_SIZE);

    return true;
}

bool test_integration_filesystem_directory_exists_real_directory() {
    ASSERT_TRUE(Filesystem::directory_exists(test_paths_dir));

    return true;
}


bool test_integration_filesystem_directory_exists_nonexisting_directory() {
    ASSERT_FALSE(Filesystem::directory_exists(TEST_DATA_DIR "/nonexistent_directory"));

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_integration_filesystem_file_size),
        TEST(test_integration_filesystem_file_size_empty_filename_throws),
        TEST(test_integration_filesystem_file_size_nonexistent_file_throws),
        TEST(test_integration_filesystem_file_size_directory_throws),
        TEST(test_integration_filesystem_directory_exists_real_directory),
        TEST(test_integration_filesystem_directory_exists_nonexisting_directory),
    };

    return Test::run_suite("test_integration_filesystem", tests);
}
