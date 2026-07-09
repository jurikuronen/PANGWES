/*
 * test_unit_sgg_edges_filenames.cpp - Unit tests for sgg/SGGEdgesFilenames.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <cstddef>
#include <string>
#include <vector>

#include "common/test_harness/Test.hpp"
#include "common/utils/Exception.hpp"
#include "common/utils/memory.hpp"
#include "mocks/MockFileReader.hpp"
#include "unitig_distance/sgg/SGGEdgesFilenames.hpp"

namespace PANGWES {
namespace {

bool test_unit_sgg_edges_filenames_constructor() {
    const std::vector<std::string> expected_filenames{
        "file", "file.edges", "dir/file", "/dir/file", "/dir/subdir/file.edges"
    };

    Mocks::MockIfStream::set_contents(expected_filenames);

    const auto filenames = SGGEdgesFilenames(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file));

    ASSERT_EQUAL(filenames.size(), expected_filenames.size());

    for (std::size_t idx = 0; idx < expected_filenames.size(); ++idx) {
        ASSERT_EQUAL(expected_filenames[idx], filenames[idx]);
    }

    return true;
}

bool test_unit_sgg_edges_filenames_iterators() {
    const std::vector<std::string> expected_filenames{
        "file", "file.edges", "dir/file", "/dir/file", "/dir/subdir/file.edges"
    };

    Mocks::MockIfStream::set_contents(expected_filenames);

    const auto filenames = SGGEdgesFilenames(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file));
    std::vector<std::string> iterated_filenames;

    for (const auto& filename : filenames) {
        iterated_filenames.push_back(filename);
    }

    ASSERT_CONTAINERS_EQUAL(iterated_filenames, expected_filenames);

    return true;
}

bool test_unit_sgg_edges_filenames_constructor_empty_lines() {
    std::vector<std::string> contents{ "a", "a", "a", "a", "a" };

    for (std::size_t idx = 0; idx < contents.size() - 1; ++idx) {
        // Clear line `idx`.
        contents[idx] = "";
        Mocks::MockIfStream::set_contents(contents);

        const auto filenames = SGGEdgesFilenames(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file));
        const auto expected_size = contents.size() - idx - 1;

        assert(expected_size <= contents.size() && "integer overflow");

        ASSERT_EQUAL(filenames.size(), expected_size);
    }

    // Clear the last line.
    contents.back() = "";
    Mocks::MockIfStream::set_contents(contents);

    EXPECT_THROW(SGGEdgesFilenames(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file)),
                 ErrorCode::FILE_EMPTY);

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_unit_sgg_edges_filenames_constructor),
        TEST(test_unit_sgg_edges_filenames_iterators),
        TEST(test_unit_sgg_edges_filenames_constructor_empty_lines),
    };

    return Test::run_suite("test_unit_sgg_edges_filenames", tests);
}
