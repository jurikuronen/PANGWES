/*
 * SegmentNameMap.hpp - Class for storing GFA Segments with mapping information.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cstddef>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

#include "gfa_parser/core/Segment.hpp"

namespace PANGWES {

constexpr std::size_t SEGMENT_NOT_MAPPED = std::numeric_limits<std::size_t>::max();

// Stores GFA Segments and maps segment names to their internal ids.
class SegmentNameMap {
public:
    SegmentNameMap() = default;

    SegmentNameMap(const SegmentNameMap&) = delete;
    SegmentNameMap& operator=(const SegmentNameMap&) = delete;
    SegmentNameMap(SegmentNameMap&&) noexcept = default;
    SegmentNameMap& operator=(SegmentNameMap&&) noexcept = default;

    // Requests that the capacities of the internal containers be at least enough to contain `capacity` elements.
    void reserve(std::size_t capacity);

    // Returns the number of stored segments.
    std::size_t size() const noexcept;

    // Returns true if no segments have been stored.
    bool empty() const noexcept;

    // Returns true if a segment with the given name is stored.
    bool contains(const std::string& segment_name) const noexcept;

    // Returns the name of the segment mapped to the given mapping index. Throws if the index is out of range.
    const std::string& segment_name(std::size_t segment_mapping) const;

    // Returns the sequence of the segment mapped to the given mapping index. Throws if the index is out of range.
    const std::string& segment_sequence(std::size_t segment_mapping) const;

    // Returns the internal mapping of the given segment name, or `SEGMENT_NOT_MAPPED` if the segment wasn't mapped.
    std::size_t segment_name_mapping(const std::string& segment_name) const noexcept;

    /*
     * Stores and maps a segment to the next available index and returns the selected segment mapping index.
     *
     * To avoid mapping a duplicate segment name, the result of `contains()` should be checked first.
    */
    std::size_t add_and_map_segment(std::string&& segment_name, std::string&& segment_sequence);

private:
    std::vector<Segment> m_segments;
    std::unordered_map<std::string, std::size_t> m_segment_name_map;
};

} // namespace PANGWES
