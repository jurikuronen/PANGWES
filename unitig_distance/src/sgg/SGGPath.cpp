/*
 * SGGPath.cpp - Single-genome graph (SGG) path composed of compacted de Bruijn graph (cdBG) nodes.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <cassert>
#include <cstdint>
#include <cmath>
#include <vector>

#include "common/utils/memory.hpp"
#include "unitig_distance/sgg/SGGPath.hpp"
#include "unitig_distance/sgg/sgg_utils.hpp"
#include "unitig_distance/sgg/sgg_types.hpp"

namespace PANGWES {
namespace {

} // namespace

SGGPath::SGGPath(std::size_t start_node,
                 const Edge& first_edge,
                 const SGGEdges& sgg_edges,
                 std::vector<std::size_t>& nodes_in_path)
{
    std::vector<std::uint64_t> cumulative_distances{first_edge.weight};
    auto current_node = first_edge.endpoint;
    auto previous_node = start_node;

    /*
     * Traverse the path starting from `start_node` via `first_edge`, until a node with degree other than 2 is
     * encountered. That node is then designated the end node. Each intermediate node encountered along the traversal,
     * excluding the start and end nodes, are appended to the `nodes_in_path` vector.
     *
     * If traversal returns to `start_node`, a loop is detected and traversal stops there. In this case, the start and
     * end nodes are the same.
     *
     * The provided graph representation (`sgg_edges`) should contain no duplicate edges.
    */
    while (current_node != start_node && sgg_edges[current_node].size() == 2) {
        nodes_in_path.push_back(current_node);

        const auto next_edge = SGGUtils::find_next_edge_in_path(sgg_edges[current_node], previous_node);

        cumulative_distances.push_back(cumulative_distances.back() + next_edge.weight);

        previous_node = current_node;
        current_node = next_edge.endpoint;
    }

    m_cumulative_distances = std::move(cumulative_distances);
    m_start_node = start_node;
    m_end_node = current_node;
}

std::size_t SGGPath::reserved_bytes() const noexcept {
    return sizeof(*this) + Memory::container_reserved_bytes(m_cumulative_distances);
}

std::size_t SGGPath::start_node() const noexcept {
    return m_start_node;
}

std::size_t SGGPath::end_node() const noexcept {
    return m_end_node;
}

std::uint64_t SGGPath::path_weight() const noexcept {
    return m_cumulative_distances.back();
}

std::uint64_t SGGPath::distance_along_path_to_start(std::size_t path_index) const {
    // The final entry corresponds to the edge reaching the end node (internal use only).
    assert(path_index + 1 < m_cumulative_distances.size());

    return m_cumulative_distances[path_index];
}

std::uint64_t SGGPath::distance_along_path_to_end(std::size_t path_index) const {
    return path_weight() - distance_along_path_to_start(path_index);
}

std::uint64_t SGGPath::distance_along_path(std::size_t node1_path_index, std::size_t node2_path_index) const {
    // The final entry corresponds to the edge reaching the end node (internal use only).
    assert(node1_path_index + 1 < m_cumulative_distances.size());
    assert(node2_path_index + 1 < m_cumulative_distances.size());

    const auto node1_distance = m_cumulative_distances[node1_path_index];
    const auto node2_distance = m_cumulative_distances[node2_path_index];

    return node1_distance > node2_distance ? node1_distance - node2_distance : node2_distance - node1_distance;
}

} // namespace PANGWES
