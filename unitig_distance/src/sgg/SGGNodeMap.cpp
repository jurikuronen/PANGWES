/*
 * SGGNodeMap.cpp - Class for storing single-genome graph (SGG) mapping information.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <cassert>
#include <cstddef>
#include <utility>

#include "common/utils/memory.hpp"
#include "unitig_distance/sgg/SGGNodeMap.hpp"

namespace PANGWES {

std::size_t SGGNodeMap::size() const noexcept {
    return m_node_map.size();
}

std::size_t SGGNodeMap::capacity() const noexcept {
    return m_node_map.capacity();
}

std::size_t SGGNodeMap::reserved_bytes() const noexcept {
    return sizeof(*this) + Memory::container_reserved_bytes(m_node_map);
}

void SGGNodeMap::resize(std::size_t size) {
    m_node_map.resize(size, {SGG_NODE_NOT_MAPPED, SGG_NODE_NOT_MAPPED});
}

void SGGNodeMap::map_node(std::size_t node, std::size_t node_mapping, std::size_t path_mapping) {
    m_node_map.at(node) = NodeMapElement{node_mapping, path_mapping};
}

std::size_t SGGNodeMap::node_mapping(std::size_t node) const noexcept {
    if (node >= size()) {
        return SGG_NODE_NOT_MAPPED;
    }

    return m_node_map[node].node_mapping;
}

std::size_t SGGNodeMap::path_mapping(std::size_t node) const noexcept {
    if (node >= size()) {
        return SGG_NODE_NOT_MAPPED;
    }

    return m_node_map[node].path_mapping;
}

bool SGGNodeMap::is_mapped(std::size_t node) const noexcept {
    return node_mapping(node) != SGG_NODE_NOT_MAPPED;
}

bool SGGNodeMap::is_on_path(std::size_t node) const noexcept {
    return path_mapping(node) != SGG_NODE_NOT_MAPPED;
}

} // namespace PANGWES
