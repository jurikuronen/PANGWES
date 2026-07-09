/*
 * SGG.hpp - Class for constructing single-genome graphs (SGGs) from SGGEdges.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

#include "unitig_distance/core/DistanceQueryBatch.hpp"
#include "unitig_distance/sgg/DFSStack.hpp"
#include "unitig_distance/sgg/SGGEdges.hpp"
#include "unitig_distance/sgg/SGGNodeMap.hpp"
#include "unitig_distance/sgg/SGGPath.hpp"

namespace PANGWES {

/*
 * SGG is built from SGGEdges, representing edges between compacted de Bruijn graph (cdBG) nodes, by further compressing
 * paths present in the graph into SGGPaths.
 *
 * - Non-path nodes (degree != 2) and path endpoint nodes are stored as explicit nodes in `m_adj` and mapped as real
 *   nodes in `m_node_map`.
 * - In-path nodes are implicitly stored in `m_paths` utilizing mapping information provided by `m_node_map`.
*/
class SGG {
public:
    // Constructs a compacted SGG from SGGEdges.
    explicit SGG(const SGGEdges& sgg_edges);

    SGG(const SGG&) = delete;
    SGG& operator=(const SGG&) = delete;
    SGG(SGG&&) noexcept = default;
    SGG& operator=(SGG&&) noexcept = default;

    // Returns true if the given unitig was mapped into this SGG.
    bool contains(std::size_t unitig_id) const noexcept;

    // Returns the number of explicit nodes with degree > 0 (size of the internal adjacency list).
    std::size_t size() const noexcept;

    // Returns the number of bytes of dynamic storage reserved by this object (excludes allocator overhead).
    std::size_t reserved_bytes() const noexcept;

    // Returns the number of bytes used by the current thread's static buffers.
    std::size_t static_bytes() const;

    // Returns true if the graph contains no nodes with degree > 0.
    bool empty() const noexcept;

    // Mapping accessors.
    bool is_mapped(std::size_t node) const noexcept;
    bool is_on_path(std::size_t node) const noexcept;
    std::size_t node_mapping(std::size_t node) const noexcept;
    std::size_t path_mapping(std::size_t node) const noexcept;

    // Returns the SGG's adjacency list.
    const AdjacencyListT& adj() const noexcept;

    // Returns the stored path for `path_index`.
    const SGGPath& path(std::size_t path_index) const;

    /*
     * Computes shortest-path distances from a source unitig to all target unitigs in `targets`.
     *
     * Procedure:
     * 1) Convert each unitig with id U into its two sides in the graph representation:
     *    - left node (U * 2)
     *    - right node (U * 2 + 1).
     * 2) For each target, calculate graph-distances for the endpoint pairs: source left/right to target left/right.
     * 3) Calculate unitig-distances from the resulting graph-distances.
     *
     * Returns:
     * - An empty vector if the source unitig is not contained in this SGG.
     * - Otherwise, a vector aligned with `targets`, where each index gives the corresponding source-target
     *   unitig-distance.
    */
    std::vector<std::uint64_t> compute_unitig_distances(std::size_t source_unitig_id,
                                                        const std::vector<DistanceQueryTarget>& targets) const;

protected:
    AdjacencyListT m_adj;
    std::vector<SGGPath> m_paths;
    SGGNodeMap m_node_map;

private:
    // Adds an edge between two explicit (non-path) nodes.
    void add_edge(std::size_t node_from, std::size_t node_to, std::uint64_t weight);

    /*
     * Adds a new node to the graph.
     * - If `path_mapping == SGG_NODE_NOT_MAPPED`, `node` is added as an explicit node with edges stored in
     *   `m_adj[node]`.
     * - If `path_mapping != SGG_NODE_NOT_MAPPED`, `node` is mapped to be the `node_mapping`'th node in
     *   `m_paths[path_mapping]`.
    */
    void add_and_map_node(std::size_t node,
                          std::size_t node_mapping,
                          std::size_t path_mapping = SGG_NODE_NOT_MAPPED);

    // Add node `v`'s neighbors into the stack used by the constructor's depth-first search algorithm.
    void dfs_add_neighbors(DFSStack& stack, std::size_t node, const EdgeListT& node_edge_list) const;

    // Processes the path and returns the compressed edge from `parent`, via `first_edge`, to the path's end node.
    Edge process_path(const SGGEdges& sgg_edges, std::size_t parent, const Edge& first_edge);

    // Post-processes graph-distances into unitig-distances.
    std::vector<std::uint64_t>
    post_process_graph_distances(std::size_t source_unitig_id,
                                 const std::vector<DistanceQueryTarget>& targets) const noexcept;
};

} // namespace PANGWES
