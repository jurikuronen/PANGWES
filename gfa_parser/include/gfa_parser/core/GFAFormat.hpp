/*
 * GFAFormat.hpp - Enumeration for GFA formats.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cstdint>
#include <string>

namespace PANGWES {

enum class GFAFormat : std::uint8_t {
    GFA1,
    GFA2
};

// Converts an integer representation of a GFA format to a GFA format enumeration.
GFAFormat to_gfa_format(std::uint64_t gfa_format_uint);

// Converts a GFA format enumeration to a string.
std::string to_string(GFAFormat gfa_format);

} // namespace PANGWES
