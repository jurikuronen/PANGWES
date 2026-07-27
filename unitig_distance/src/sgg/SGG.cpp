/*
 * SGG.cpp - Class for constructing single-genome graphs (SGGs) from SGGEdges.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <set>
#include <vector>

#include "common/utils/Exception.hpp"
#include "common/utils/memory.hpp"
#include "unitig_distance/sgg/SGG.hpp"
#include "unitig_distance/sgg/sgg_utils.hpp"
#include "unitig_distance/sgg/sgg_types.hpp"

namespace PANGWES {
namespace {

// Path-construction buffer to avoid repeated allocations.
thread_local std::vector<std::size_t> s_nodes_in_path;

// Distance-computation buffers to avoid repeated allocations.
thread_local std::vector<std::uint64_t> s_graph_distances;
thread_local std::vector<bool> s_is_target;

/*
 * Searches for either of the path's endpoints that `start_node` is in, or returns `start_node` if `start_node` is on a
 * loop path.
*/
std::size_t try_find_path_endpoint(const SGGEdges& sgg_edges, std::size_t start_node) {
    assert(start_node < sgg_edges.size() && sgg_edges[start_node].size() == 2);

    auto previous_node = start_node;
    // Choose arbitrary neighbor.
    auto current_node = sgg_edges[start_node].front().endpoint;

    while (current_node != start_node && sgg_edges[current_node].size() == 2) {
        const auto next_edge = SGGUtils::find_next_edge_in_path(sgg_edges[current_node], previous_node);

        previous_node = current_node;
        current_node = next_edge.endpoint;
    }

    return current_node;
}

} // namespace

SGG::SGG(const SGGEdges& sgg_edges) {
    if (sgg_edges.empty()) {
        throw Exception(ErrorCode::EMPTY_DATA, "SGGEdges");
    }

    // Prepare node map to fit all nodes in `sgg_edges`.
    m_node_map.resize(sgg_edges.size());

    // Run a DFS on the edges to compress paths.
    for (std::size_t start_node = 0; start_node < sgg_edges.size(); ++start_node) {
        if (sgg_edges[start_node].empty() || is_mapped(start_node)) {
            continue;
        }

        DFSStack stack;

        // If `start_node` is on a path, try to find the path's endpoint to avoid starting the search from a path node.
        if (sgg_edges[start_node].size() == 2) {
            const auto path_endpoint = try_find_path_endpoint(sgg_edges, start_node);

            if (is_mapped(path_endpoint)) {
                throw Exception(ErrorCode::INVALID_STATE, "path endpoint ", path_endpoint, " already mapped as ",
                                node_mapping(path_endpoint));
            }

            add_and_map_node(path_endpoint, size());
            dfs_add_neighbors(stack, path_endpoint, sgg_edges[path_endpoint]);
        } else {
            add_and_map_node(start_node, size());
            dfs_add_neighbors(stack, start_node, sgg_edges[start_node]);
        }

        while (!stack.empty()) {
            const auto parent = stack.next_parent();
            /*
             * These are non-const because `node` could be a path node on the path that starts from `parent`. In such a
             * case, the path will be compressed later and `node` will be updated to the path's end node and `weight` to
             *  the path's total weight.
            */
            auto weight = stack.next_weight();
            auto node = stack.next_node();
            stack.pop_back();

            assert(is_mapped(parent) && "parent node must have been processed");
            assert(!is_on_path(parent) && "parent node can't have been on a path");
            assert(node != parent && "node can't be its own parent");

            if (is_mapped(node)) {
                /*
                 * Might have added an edge previously previously via a path from `node` ending at `parent`. In that
                 * case, this edge might be shorter, so the edge weight may need to be updated.
                */
                if (!is_on_path(node)) {
                    add_edge(node_mapping(parent), node_mapping(node), weight);
                }

                continue;
            }

            // Path detected: compress it into a single edge.
            if (sgg_edges[node].size() == 2) {
                /*
                 * Processes the path starting from `parent` via `first_edge`, mapping all path nodes encountered along
                 * the way. The returned edge is the compressed edge from `parent` to the path's end node.
                */
                const auto compressed_edge = process_path(sgg_edges, parent, {node, weight});

                node = compressed_edge.endpoint;

                // Path looped back to parent, no need to do anything anymore.
                if (node == parent) {
                    continue;
                }

                weight = compressed_edge.weight;
            }

            assert(!is_on_path(node) && "algorithm error: node can't be path node at this step");

            // If a path was detected, `node` is now a path end node and could already be mapped.
            if (!is_mapped(node)) {
                add_and_map_node(node, size());

                dfs_add_neighbors(stack, node, sgg_edges[node]);
            }

            // Add the edge or update the edge weight.
            add_edge(node_mapping(parent), node_mapping(node), weight);
        }
    }
}

bool SGG::contains(std::size_t unitig_id) const noexcept {
    // It is sufficient to check either of the sides of the corresponding node.
    return m_node_map.is_mapped(SGGUtils::unitig_to_left_node(unitig_id));
}

std::size_t SGG::size() const noexcept {
    return m_adj.size();
}

std::size_t SGG::reserved_bytes() const noexcept {
    return sizeof(*this) + Memory::container_reserved_bytes(m_adj) + Memory::container_reserved_bytes(m_paths) +
           m_node_map.reserved_bytes();
}

std::size_t SGG::static_bytes() const {
    const auto construction_bytes = sizeof(s_nodes_in_path) + Memory::container_reserved_bytes(s_nodes_in_path);

    // Prepare the buffers if compute_unitig_distances() was not called yet.
    s_graph_distances.reserve(size());
    s_is_target.reserve(size());

    const auto computation_bytes = sizeof(s_graph_distances) + Memory::container_reserved_bytes(s_graph_distances) +
                                   sizeof(s_is_target) + Memory::container_reserved_bytes(s_is_target);

    return construction_bytes + computation_bytes;
}

bool SGG::empty() const noexcept {
    return size() == 0;
}

bool SGG::is_mapped(std::size_t node) const noexcept {
    return m_node_map.is_mapped(node);
}

bool SGG::is_on_path(std::size_t node) const noexcept {
    return m_node_map.is_on_path(node);
}

std::size_t SGG::node_mapping(std::size_t node) const noexcept {
    return m_node_map.node_mapping(node);
}

std::size_t SGG::path_mapping(std::size_t node) const noexcept {
    return m_node_map.path_mapping(node);
}

const AdjacencyListT& SGG::adj() const noexcept {
    return m_adj;
}

const SGGPath& SGG::path(std::size_t path_index) const {
    return m_paths.at(path_index);
}

std::vector<std::uint64_t> SGG::compute_unitig_distances(std::size_t source_unitig_id,
                                                         const std::vector<DistanceQueryTarget>& targets) const
{
    if (!contains(source_unitig_id)) {
        return {};
    }

    // Stores (distance, node) pairs.
    std::set<std::pair<std::uint64_t, std::size_t>> queue;

    // Prepare information about already found targets.
    s_is_target.assign(size(), false);
    std::size_t targets_left = 0;

    /*
     * If the given `node` is a path node, add both path endpoints as start nodes to the queue. Otherwise, add the node
     * itself.
    */
    const auto add_start_node_to_queue = [this, &queue](std::size_t node) {
        const auto node_mapped = node_mapping(node);

        if (!is_on_path(node)) {
            s_graph_distances.at(node_mapped) = 0;
            queue.emplace(0, node_mapped);
        } else {
            const auto& path = m_paths.at(path_mapping(node));
            const auto path_start_node = node_mapping(path.start_node());
            const auto path_end_node = node_mapping(path.end_node());

            const auto node_dist_to_start = path.distance_along_path_to_start(node_mapped);
            s_graph_distances.at(path_start_node) = std::min(s_graph_distances.at(path_start_node), node_dist_to_start);
            queue.emplace(node_dist_to_start, path_start_node);

            const auto node_dist_to_end = path.distance_along_path_to_end(node_mapped);
            s_graph_distances.at(path_end_node) = std::min(s_graph_distances.at(path_end_node), node_dist_to_end);
            queue.emplace(node_dist_to_end, path_end_node);
        }
    };

    // If the given `target_node` is a path node, add both path endpoints as targets. Otherwise, add the node itself.
    const auto add_target_node = [this, &targets_left](std::size_t target_node) {
        // Only sets `s_is_target[node]` and updates `targets_left` if `node` wasn't already marked.
        const auto update_is_target = [&targets_left](std::size_t node_mapped) {
            if (!s_is_target.at(node_mapped)) {
                s_is_target[node_mapped] = true;
                ++targets_left;
            }
        };

        if (!is_on_path(target_node)) {
            update_is_target(node_mapping(target_node));
        } else {
            const auto& path = m_paths.at(path_mapping(target_node));

            update_is_target(node_mapping(path.start_node()));
            update_is_target(node_mapping(path.end_node()));
        }
    };

    // Likewise, each target unitig's both sides of its graph-representation must be added as targets in the search.
    for (const auto& target : targets) {
        const auto target_unitig_id = target.target_unitig_id;

        if (!contains(target_unitig_id)) {
            continue;
        }

        add_target_node(SGGUtils::unitig_to_left_node(target_unitig_id));
        add_target_node(SGGUtils::unitig_to_right_node(target_unitig_id));
    }

    // Early return in case the graph contained no target unitigs.
    if (targets_left == 0) {
        return std::vector<std::uint64_t>(targets.size(), INF_DISTANCE);
    }

    // Prepare a list of distances for each node.
    s_graph_distances.assign(size(), INF_DISTANCE);

    /*
     * The source unitig is represented by two nodes in the graph: its left and right side, and both are valid start
     * nodes of the search.
    */
    add_start_node_to_queue(SGGUtils::unitig_to_left_node(source_unitig_id));
    add_start_node_to_queue(SGGUtils::unitig_to_right_node(source_unitig_id));

    // Start search.
    while (!queue.empty()) {
        // Selects the node with smallest `source` to `node` distance currently in the queue.
        const auto node = queue.cbegin()->second;

        queue.erase(queue.cbegin());

        if (s_is_target[node]) {
            --targets_left;
            s_is_target[node] = false;

            // Found all targets; stop.
            if (targets_left == 0) {
                break;
            }
        }

        for (const auto& edge : m_adj[node]) {
            const auto neighbor = edge.endpoint;
            const auto weight = edge.weight;

            // Check if we can relax the edge (v, w).
            if (s_graph_distances[node] + weight < s_graph_distances[neighbor]) {
                queue.erase({s_graph_distances[neighbor], neighbor});
                s_graph_distances[neighbor] = s_graph_distances[node] + weight;
                queue.insert({s_graph_distances[neighbor], neighbor});
            }
        }
    }

    return post_process_graph_distances(source_unitig_id, targets);
}

void SGG::add_edge(std::size_t node_from, std::size_t node_to, std::uint64_t weight) {
    SGGUtils::add_edge(m_adj, node_from, node_to, weight);
}

void SGG::add_and_map_node(std::size_t node, std::size_t node_mapping, std::size_t path_mapping) {
    m_node_map.map_node(node, node_mapping, path_mapping);

    if (path_mapping == SGG_NODE_NOT_MAPPED) {
        m_adj.emplace_back();
    }
}

void SGG::dfs_add_neighbors(DFSStack& stack, std::size_t node, const EdgeListT& node_edge_list) const {
    for (const auto& edge : node_edge_list) {
        const auto neighbor = edge.endpoint;

        if (!is_mapped(neighbor)) {
            stack.push_back(node, neighbor, edge.weight);
        }
    }
}

Edge SGG::process_path(const SGGEdges& sgg_edges, std::size_t parent, const Edge& first_edge)
{
    s_nodes_in_path.clear();

    auto path = SGGPath(parent, first_edge, sgg_edges, s_nodes_in_path);

    const auto new_path = m_paths.size();
    for (std::size_t i = 0; i < s_nodes_in_path.size(); ++i) {
        const auto node = s_nodes_in_path[i];

        assert(!is_mapped(node) && "algorithm error: already mapped path node");

        add_and_map_node(node, i, new_path);
    }

    assert(path.start_node() == parent && "path not created properly");

    const auto path_end_node = path.end_node();
    const auto path_weight = path.path_weight();

    m_paths.push_back(std::move(path));

    return {path_end_node, path_weight};
}

/*
 * For each target, obtain the source-target unitig-distance as the minimum graph distance over the endpoint pairs:
 * source left/right node to target left/right nodes.
 *
 * The `s_graph_distances` vector stores shortest-path distances only for explicit (non-path) nodes with the source
 * unitig's endpoint nodes already handled even when the endpoint(s) lied on a path.
 *
 * The targets require additional handling:
 * 1) If a target unitig's endpoint node lies on a path, compute its distance as the minimum over both path endpoints:
 *    `distances[source_node, target_node_path_endpoint] + `distance_along_path_to_endpoint(target_node)`.
 * 2) If both the source and target nodes lie on the same path, also consider the direct distance along the path and
 *    take the minimum.
*/
std::vector<std::uint64_t>
SGG::post_process_graph_distances(std::size_t source_unitig_id,
                                  const std::vector<DistanceQueryTarget>& targets) const noexcept
{
    std::vector<std::uint64_t> target_distances;
    target_distances.reserve(targets.size());

    /*
     * Returns the minimum distance over both path endpoints if `source_node` is not on a path. If it is, considers also
     * the direct distance along the path and return the minimum of the computed distances.
    */
    const auto calculate_target_distance_in_a_path = [this](std::size_t source_node, std::size_t target_node) {
        auto distance = INF_DISTANCE;

        const auto target_mapped = node_mapping(target_node);
        const auto target_path = path_mapping(target_node);
        const auto& path = m_paths[target_path];
        const auto path_start_node = node_mapping(path.start_node());
        const auto path_end_node = node_mapping(path.end_node());

        // Case (2): both nodes lie on the same path.
        if (target_path == path_mapping(source_node)) {
            distance = path.distance_along_path(node_mapping(source_node), target_mapped);
        }

        // Both endpoints for case (1).
        return std::min(distance,
                        std::min(s_graph_distances[path_start_node] + path.distance_along_path_to_start(target_mapped),
                                 s_graph_distances[path_end_node] + path.distance_along_path_to_end(target_mapped)));
    };

    /*
     * Returns the distance directly if `target_node` is not on a path. Otherwise, returns the minimum after handling
     * cases (1) and (2) for both source endpoints.
    */
    const auto calculate_target_distance =
        [this, source_unitig_id, calculate_target_distance_in_a_path](std::size_t target_node)
    {
        if (!is_on_path(target_node)) {
            return s_graph_distances[node_mapping(target_node)];
        }

        // Handle cases (1) and (2).
        const auto source_left = SGGUtils::unitig_to_left_node(source_unitig_id);
        const auto source_right = SGGUtils::unitig_to_right_node(source_unitig_id);

        return std::min(calculate_target_distance_in_a_path(source_left, target_node),
                        calculate_target_distance_in_a_path(source_right, target_node));
    };

    for (const auto& target : targets) {
        const auto target_unitig_id = target.target_unitig_id;

        if (!contains(target_unitig_id)) {
            // We must return a vector of the same size as `targets`.
            target_distances.push_back(INF_DISTANCE);

            continue;
        }

        const auto target_left = SGGUtils::unitig_to_left_node(target_unitig_id);
        const auto target_right = SGGUtils::unitig_to_right_node(target_unitig_id);

        const auto distance = std::min(calculate_target_distance(target_left),
                                       calculate_target_distance(target_right));

        target_distances.push_back(distance);
    }

    return target_distances;
}

} // namespace PANGWES
