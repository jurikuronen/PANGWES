/*
 * Orientation.hpp - Orientation information of a segment.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cstdint>

#include "common/utils/Exception.hpp"

namespace PANGWES {

// Enumeration for Segment orientation within a path.
enum class Orientation : std::uint8_t {
    PLUS,
    MINUS
};

// Parse Orientation from a string.
inline Orientation parse_orientation(char orientation_chr) {
    switch (orientation_chr) {
        case '+': return Orientation::PLUS;
        case '-': return Orientation::MINUS;
        default: throw Exception(ErrorCode::INVALID_DATA, "invalid orientation: ", orientation_chr);
    }
}

} // namespace PANGWES
