/*
 * sgg_types.hpp - Single-genome graph-related type definitions.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace PANGWES {

struct Edge {
    std::size_t endpoint;
    std::uint64_t weight;
};

// Equality comparison for Edge.
inline bool operator==(const Edge& edge1, const Edge& edge2) noexcept {
    return edge1.endpoint == edge2.endpoint && edge1.weight == edge2.weight;
}

// Inequality comparison for edge.
inline bool operator!=(const Edge& edge1, const Edge& edge2) noexcept {
    return !(edge1 == edge2);
}

// Returns a descriptive string of an Edge.
inline std::string to_string(const Edge& edge) {
    std::ostringstream oss;

    oss << "Edge{endpoint=" << edge.endpoint << ", weight=" << edge.weight << "}";

    return oss.str();
}

using EdgeListT = std::vector<Edge>;
using EdgeListItT = EdgeListT::iterator;
using EdgeListConstItT = EdgeListT::const_iterator;
using AdjacencyListT = std::vector<EdgeListT>;

// Infinite distance definition with overflow protection.
constexpr auto INF_DISTANCE = std::numeric_limits<std::uint64_t>::max() / 16;

constexpr std::size_t SGG_NODE_NOT_MAPPED = std::numeric_limits<std::size_t>::max();

} // namespace PANGWES
