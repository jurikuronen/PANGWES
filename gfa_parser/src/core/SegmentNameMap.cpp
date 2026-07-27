/*
 * SegmentNameMap.cpp - Class for storing GFA Segments with mapping information.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <cassert>

#include "common/utils/Exception.hpp"
#include "gfa_parser/core/SegmentNameMap.hpp"

namespace PANGWES {

void SegmentNameMap::reserve(std::size_t capacity) {
    m_segments.reserve(capacity);
    m_segment_name_map.reserve(capacity);
}

std::size_t SegmentNameMap::size() const noexcept {
    return m_segments.size();
}

bool SegmentNameMap::empty() const noexcept {
    return size() == 0;
}

bool SegmentNameMap::contains(const std::string& segment_name) const noexcept {
    return segment_name_mapping(segment_name) != SEGMENT_NOT_MAPPED;
}

const std::string& SegmentNameMap::segment_name(std::size_t segment_mapping) const {
    if (segment_mapping >= size()) {
        throw Exception(ErrorCode::INDEX_OUT_OF_RANGE);
    }

    return m_segments[segment_mapping].name();
}

const std::string& SegmentNameMap::segment_sequence(std::size_t segment_mapping) const {
    if (segment_mapping >= size()) {
        throw Exception(ErrorCode::INDEX_OUT_OF_RANGE);
    }

    return m_segments[segment_mapping].sequence();
}

std::size_t SegmentNameMap::segment_name_mapping(const std::string& segment_name) const noexcept {
    const auto segment_mapping_it = m_segment_name_map.find(segment_name);

    return segment_mapping_it == m_segment_name_map.end() ? SEGMENT_NOT_MAPPED : segment_mapping_it->second;
}

std::size_t SegmentNameMap::add_and_map_segment(std::string&& segment_name, std::string&& segment_sequence) {
    assert(!contains(segment_name) && "tried to add and map a duplicate Segment Name");

    const auto segment_mapping = m_segments.size();

    m_segment_name_map[segment_name] = segment_mapping;
    m_segments.emplace_back(std::move(segment_name), std::move(segment_sequence));

    return segment_mapping;
}

} // namespace PANGWES
