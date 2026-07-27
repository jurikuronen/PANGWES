/*
 * SGGPath.hpp - Single-genome graph (SGG) path composed of compacted de Bruijn graph (cdBG) nodes.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "unitig_distance/sgg/SGGEdges.hpp"

namespace PANGWES {

/*
 * Represents a simple path in a particular SGG, or color subgraph of the global cdBG.
 *
 * All nodes in the path correspond cdBG nodes. However, in terms of the SGG, this path represents a maximal unitig with
 * respect to the SGG.
 *
 * Stores the start and end nodes of the path and the distance of each in-path node to the start node along the path
 * via cumulative edge weights.
*/
class SGGPath {
public:
    /*
     * Constructs a path by traversing the graph (represented by `sgg_edges`) starting from `start_node` via
     * `first_edge` until a node with degree other than 2 is encountered. That node is then designated the end node.
     * All nodes encountered along the path, excluding the start and end nodes, are appended to the `nodes_in_path`
     * vector. The index of each node in this vector corresponds to the index of the edge that reaches that node in the
     * internal `m_cumulative_distances` vector, which stores the cumulative edge weight distances to the start node
     * along the path.
     *
     * The provided graph representation (`sgg_edges`) should contain no duplicate edges.
    */
    SGGPath(std::size_t start_node,
            const Edge& first_edge,
            const SGGEdges& sgg_edges,
            std::vector<std::size_t>& nodes_in_path);

    SGGPath(const SGGPath&) = delete;
    SGGPath& operator=(const SGGPath&) = delete;
    SGGPath(SGGPath&&) = default;
    SGGPath& operator=(SGGPath&&) = default;

    // Returns the number of bytes of dynamic storage reserved by this object (excludes allocator overhead).
    std::size_t reserved_bytes() const noexcept;

    // Returns the start node of the path.
    std::size_t start_node() const noexcept;

    // Returns the end node of the path.
    std::size_t end_node() const noexcept;

    // Returns the total weight (sum of edge weights) of the path.
    std::uint64_t path_weight() const noexcept;

    /*
     * Returns the distance along the path to the start node from the node reached by the edge at index `path_index`
     * in `m_cumulative_distances`.
    */
    std::uint64_t distance_along_path_to_start(std::size_t path_index) const;

    /*
     * Returns the distance along the path to the end node from the node reached by the edge at index `path_index`
     * in `m_cumulative_distances`.
    */
    std::uint64_t distance_along_path_to_end(std::size_t path_index) const;

    /*
     * Returns the distance along the path between two nodes reached by the edges at the given indices in
     * `m_cumulative_distances`.
    */
    std::uint64_t distance_along_path(std::size_t node1_path_index, std::size_t node2_path_index) const;

private:
    // Cumulative edge weight sums along the path from the start node towards the end node.
    std::vector<std::uint64_t> m_cumulative_distances;
    std::size_t m_start_node;
    std::size_t m_end_node;
};

} // namespace PANGWES
