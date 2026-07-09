/*
 * test_integration_distance_query_common.hpp - Common data and helpers for distance query integration tests.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "common/io/FileReader.hpp"
#include "common/utils/memory.hpp"
#include "common/utils/WorkerPool.hpp"
#include "mocks/MockFileReader.hpp"
#include "unitig_distance/core/UnitigWeights.hpp"
#include "unitig_distance/sgg/SGGEdgesFilenames.hpp"

namespace PANGWES {

// Currently only EFC test data is used for integration tests.
constexpr auto unitigs_filename = TEST_DATA_DIR "/test_efc_k31.unitigs";
constexpr auto queries_filename = TEST_DATA_DIR "/test_efc_k31.queries";
constexpr auto sgg_edges_directory = TEST_DATA_DIR "/test_efc_k31_paths";
constexpr auto kmer_length = 31;
constexpr std::size_t n_sggs = 8;

inline UnitigWeights make_unitig_weights() {
    return UnitigWeights(Memory::make_unique<FileReader>(unitigs_filename), kmer_length);
}

// Construct SGGEdgesFilenames via MockFileReader since the "test_efc_k31.paths" file contains relative paths.
inline SGGEdgesFilenames make_sgg_edges_filenames() {
    std::vector<std::string> filenames;
    filenames.reserve(n_sggs);

    for (std::size_t sgg_index = 0; sgg_index < n_sggs; ++sgg_index) {
        filenames.push_back(std::string{sgg_edges_directory} + "/" + std::to_string(sgg_index + 1) + ".edges");
    }

    Mocks::MockIfStream::set_contents(filenames);

    return SGGEdgesFilenames(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file));
}

inline SGGEdgesFilenames make_sgg_edges_filenames(const std::vector<std::string>& filenames) {
    Mocks::MockIfStream::set_contents(filenames);

    return SGGEdgesFilenames(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file));
}

} // namespace PANGWES
