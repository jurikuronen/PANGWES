/*
 * test_integration_sgg_edges_filenames.cpp - Integration tests for sgg/SGGEdgesFilenames.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <cstddef>
#include <string>
#include <vector>

#include "common/test_harness/Test.hpp"
#include "common/utils/memory.hpp"
#include "mocks/MockFileReader.hpp"
#include "unitig_distance/sgg/SGGEdgesFilenames.hpp"

namespace PANGWES {
namespace {

bool check_indices_by_descending_file_size(const std::string& sgg_edges_directory,
                                           const std::vector<std::size_t>& expected_indices)
{
    std::vector<std::string> test_paths_content{};

    for (auto i = 1; i <= 8; ++i) {
        test_paths_content.push_back(std::string{sgg_edges_directory} + "/" + std::to_string(i) + ".edges");
    }

    Mocks::MockIfStream::set_contents(test_paths_content);

    const auto sgg_edges_filenames = SGGEdgesFilenames(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file));
    const auto indices = sgg_edges_filenames.indices_by_descending_file_size();

    ASSERT_CONTAINERS_EQUAL(indices, expected_indices);

    return true;
}

bool test_integration_sgg_edges_filenames_efc_indices_by_descending_file_size() {
    constexpr auto sgg_edges_directory = TEST_DATA_DIR "/test_efc_k31_paths";
    const std::vector<std::size_t> expected_indices{0, 7, 1, 6, 2, 5, 4, 3};

    return check_indices_by_descending_file_size(sgg_edges_directory, expected_indices);
}

bool test_integration_sgg_edges_filenames_maela_indices_by_descending_file_size() {
    constexpr auto sgg_edges_directory = TEST_DATA_DIR "/test_maela_k31_paths";
    const std::vector<std::size_t> expected_indices{3, 1, 7, 2, 6, 4, 5, 0};

    return check_indices_by_descending_file_size(sgg_edges_directory, expected_indices);
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_integration_sgg_edges_filenames_efc_indices_by_descending_file_size),
        TEST(test_integration_sgg_edges_filenames_maela_indices_by_descending_file_size),
    };

    return Test::run_suite("test_integration_sgg_edges_filenames", tests);
}
