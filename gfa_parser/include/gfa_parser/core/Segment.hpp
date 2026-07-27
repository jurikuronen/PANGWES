/*
 * Segment.hpp - Segment record storage.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <string>

namespace PANGWES {

// Stores one segment name and sequence from an `S` record.
class Segment {
public:
    // Constructs a segment from its name and sequence.
    Segment(std::string name, std::string sequence);

    Segment(const Segment&) = delete;
    Segment& operator=(const Segment&) = delete;
    Segment(Segment&&) noexcept = default;
    Segment& operator=(Segment&&) noexcept = default;

    // Returns the segment name.
    const std::string& name() const noexcept;

    // Returns the segment sequence.
    const std::string& sequence() const noexcept;

private:
    std::string m_name;
    std::string m_sequence;
};

} // namespace PANGWES
