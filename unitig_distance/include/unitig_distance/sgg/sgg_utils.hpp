/*
 * sgg_utils.hpp - Various single-genome graph-related utility functions.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>

#include "common/io/Log.hpp"
#include "common/utils/Exception.hpp"
#include "unitig_distance/sgg/sgg_types.hpp"

namespace PANGWES {
namespace SGGUtils {

// Returns an iterator to the first edge whose endpoint matches `endpoint`, or `edges.end()` if not found.
inline EdgeListItT find_edge(EdgeListT& edges, std::size_t endpoint) {
    return std::find_if(edges.begin(), edges.end(), [endpoint](const Edge& edge) {
        return edge.endpoint == endpoint;
    });
}

// Returns a const_iterator to the first edge whose endpoint matches `endpoint`, or `edges.end()` if not found.
inline EdgeListConstItT find_edge(const EdgeListT& edges, std::size_t endpoint) {
    return std::find_if(edges.begin(), edges.end(), [endpoint](const Edge& edge) {
        return edge.endpoint == endpoint;
    });
}

/*
 * Adds an edge (v, w) with weight to the provided adjacency list. If the edge already exists, update its weight to the
 * smaller weight.
*/
inline void add_edge(AdjacencyListT& adj, std::size_t node_from, std::size_t node_to, std::uint64_t weight) {
    assert(node_from < adj.size() && "index out of bounds");
    assert(node_to < adj.size() && "index out of bounds");

    // Self-edges not allowed.
    if (node_from == node_to) {
        Log::out_without_date_block() << "WARNING: Ignored self-edge " << node_from << "->" << node_to
                                      << " with weight " << weight << "." << std::endl;
        return;
    }

    auto edge_it = find_edge(adj[node_from], node_to);
    if (edge_it == adj[node_from].end()) {
        assert(SGGUtils::find_edge(adj[node_to], node_from) == adj[node_to].cend());

        // New edge.
        adj[node_from].push_back({node_to, weight});
        adj[node_to].push_back({node_from, weight});
    } else {
        // Edge exists, update edge weight to the smaller weight.
        const auto current_weight = edge_it->weight;
        if (current_weight <= weight) {
            return;
        }
        edge_it->weight = weight;

        // Update the other endpoint.
        auto edge_it2 = SGGUtils::find_edge(adj[node_to], node_from);

        assert(edge_it2 != adj[node_to].end());
        assert(edge_it2->weight == current_weight);

        edge_it2->weight = weight;
    }
}

/*
 * Returns the edge corresponding to the other endpoint than `previous_node`. Must be called with an edge list of
 * exactly two nodes.
*/
inline Edge find_next_edge_in_path(const EdgeListT& edges, std::size_t previous_node) {
    if (edges.size() != 2) {
        throw Exception(ErrorCode::INVALID_ARGUMENT, "must be edge list of a path-node");
    }

    return edges.front().endpoint != previous_node ? edges.front() : edges.back();
}

/*
 * Converts a compacted de Bruijn graph node representing a unitig U's left (U * 2) or right side (U * 2 + 1) into its
 * other endpoint.
*/
inline std::size_t node_other_side(std::size_t node) noexcept {
    return node ^ 1;
}

// Converts a compacted de Bruijn graph node into the corresponding unitig id.
inline std::size_t node_to_unitig(std::size_t node) noexcept {
    return node / 2;
}

// Converts a unitig U into its left (U * 2) compacted de Bruijn graph node representation.
inline std::size_t unitig_to_left_node(std::size_t unitig_id) noexcept {
    assert(unitig_id < std::numeric_limits<std::size_t>::max() / 2);

    return unitig_id * 2;
}

// Converts a unitig U into its right (U * 2 + 1) compacted de Bruijn graph node representation.
inline std::size_t unitig_to_right_node(std::size_t unitig_id) noexcept {
    assert(unitig_id < std::numeric_limits<std::size_t>::max() / 2);

    return unitig_id * 2 + 1;
}

} // namespace SGGUtils
} // namespace PANGWES
