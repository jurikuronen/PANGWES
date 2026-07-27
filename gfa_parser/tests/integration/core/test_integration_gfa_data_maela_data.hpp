/*
 * test_integration_gfa_data_maela_data.hpp - Expected values for GFAData constructed from Maela test data.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cstddef>
#include <vector>

namespace PANGWES {

constexpr auto test_maela_gfa_filename = TEST_DATA_DIR "/test_maela_k31.gfa1";

namespace MaelaTestData {

constexpr std::size_t expected_n_segments = 21569;
constexpr std::size_t expected_n_reference_paths = 8;

const auto expected_n_sequences_per_reference = std::vector<std::size_t>
{
    8, 8, 8, 8, 8, 8, 8, 8,
};

} // namespace MaelaTestData
} // namespace PANGWES
