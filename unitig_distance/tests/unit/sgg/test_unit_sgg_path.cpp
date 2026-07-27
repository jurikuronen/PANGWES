/*
 * test_unit_sgg_path.cpp - Unit tests for sgg/SGGPath.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "common/test_harness/Test.hpp"
#include "mocks/MockSGGEdges.hpp"
#include "unitig_distance/sgg/SGGPath.hpp"
#include "unitig_distance/sgg/sgg_utils.hpp"

namespace PANGWES {
namespace {

using Mocks::MockSGGEdges;

struct TestEdge {
    std::size_t node_from;
    std::size_t node_to;
    std::uint64_t weight;
};

// Constructs SGGEdges from the given list of edges.
MockSGGEdges make_test_sgg_edges(const std::vector<TestEdge>& edges) {
    MockSGGEdges sgg_edges;

    for (const auto& edge : edges) {
        const auto node_from = edge.node_from;
        const auto node_to = edge.node_to;
        const auto weight = edge.weight;

        const auto max_node = std::max(node_from, node_to);

        if (sgg_edges.size() <= max_node) {
            sgg_edges.resize(max_node + 1);
        }

        sgg_edges[node_from].push_back({node_to, weight});
        sgg_edges[node_to].push_back({node_from, weight});
    }

    return sgg_edges;
}

/*
 * Calculates the distance between `node_start` and `node_end` in the test graph.
 *
 * Note: this function assumes that the nodes in the path in the test edges are ordered numerically.
*/
std::uint64_t test_calculate_path_distance(const MockSGGEdges& adj, std::size_t node_start, std::size_t node_end) {
    if (node_start > node_end) {
        return test_calculate_path_distance(adj, node_end, node_start);
    }

    std::uint64_t distance = 0;

    for (auto node = node_start; node < node_end; ++node) {
        const auto& edges = adj[node];

        assert(edges.size() <= 2 && "must be path edges");
        assert(SGGUtils::find_edge(edges, node + 1) != edges.end() && "nodes in test edges must be in numerical order");

        if (edges.size() == std::size_t{1} && edges[0].endpoint == node + 1) {
            distance += edges[0].weight;
        } else {
            distance += edges[1].weight;
        }
    }

    return distance;
}

bool test_unit_sgg_path_trivial_2_node_path() {
    const auto test_adj = make_test_sgg_edges({{0, 1, 1}, {1, 2, 2}, {1, 3, 100}});
    const auto expected_start_node = 0;
    const auto expected_end_node = 1;
    const auto expected_path_weight = 1;

    std::vector<std::size_t> nodes_in_path;
    const auto sgg_path = SGGPath(0, {1, 1}, test_adj, nodes_in_path);

    ASSERT_EQUAL(sgg_path.start_node(), expected_start_node);
    ASSERT_EQUAL(sgg_path.end_node(), expected_end_node);
    ASSERT_EQUAL(sgg_path.path_weight(), expected_path_weight);

    // Start node and end node are not counted.
    ASSERT_TRUE(nodes_in_path.empty());

    return true;
}

bool test_unit_sgg_path_proper_path() {
    // Constructed such that the edge with index i in the path reaches to the node corresponding i+1 in test_adj.
    const auto test_adj = make_test_sgg_edges({
        {0, 1, 1}, {1, 2, 2}, {2, 3, 3}, {3, 4, 4}, {4, 5, 5},
        {5, 6, 6}, {6, 7, 7}, {7, 8, 8}, {8, 9, 100}, {8, 10, 100}
    });
    const auto expected_start_node = 0;
    const auto expected_end_node = 8;
    const auto expected_path_weight = 36;
    const auto expected_nodes_in_path_size = 7;

    // Construct the path from `test_adj`.
    std::vector<std::size_t> nodes_in_path;
    const auto sgg_path = SGGPath(0, {1, 1}, test_adj, nodes_in_path);

    ASSERT_EQUAL(sgg_path.start_node(), expected_start_node);
    ASSERT_EQUAL(sgg_path.end_node(), expected_end_node);
    ASSERT_EQUAL(sgg_path.path_weight(), expected_path_weight);
    ASSERT_EQUAL(nodes_in_path.size(), expected_nodes_in_path_size);

    for (auto node_mapping = 0; node_mapping < expected_end_node - 1; ++node_mapping) {
        /*
         * The edges in the path are constructed such that the node in the path at index `node_mapping` is the node
         * `node_mapping + 1`.
        */
        const auto node = node_mapping + 1;
        const auto expected_distance_along_path_to_start = test_calculate_path_distance(test_adj,
                                                                                        node,
                                                                                        expected_start_node);
        const auto expected_distance_along_path_to_end = test_calculate_path_distance(test_adj,
                                                                                      node,
                                                                                      expected_end_node);

        ASSERT_EQUAL(sgg_path.distance_along_path_to_start(node_mapping), expected_distance_along_path_to_start);
        ASSERT_EQUAL(sgg_path.distance_along_path_to_end(node_mapping), expected_distance_along_path_to_end);

        for (auto node2_mapping = 0; node2_mapping < expected_end_node - 1; ++node2_mapping) {
            const auto node2 = node2_mapping + 1;
            const auto expected_distance_along_path = test_calculate_path_distance(test_adj, node, node2);

            ASSERT_EQUAL(sgg_path.distance_along_path(node_mapping, node2_mapping), expected_distance_along_path);
        }
    }

    return true;
}

// This test verifies that a loop also honors path traversal rules in the distance along path functions.
bool test_unit_sgg_path_loop() {
    const auto test_adj = make_test_sgg_edges({{0, 1, 1}, {1, 2, 2}, {2, 3, 3}, {3, 0, 4}});
    const auto expected_start_node = 0;
    const auto expected_end_node = 0;
    const auto expected_path_weight = 10;
    const auto expected_nodes_in_path_size = 3;

    std::vector<std::size_t> nodes_in_path;
    const auto sgg_path = SGGPath(0, {1, 1}, test_adj, nodes_in_path);

    ASSERT_EQUAL(sgg_path.start_node(), expected_start_node);
    ASSERT_EQUAL(sgg_path.end_node(), expected_end_node);
    ASSERT_EQUAL(sgg_path.path_weight(), expected_path_weight);
    ASSERT_EQUAL(nodes_in_path.size(), expected_nodes_in_path_size);

    // Cumulative edge weights along the tested path.
    std::vector<uint64_t> expected_distance_to_start{1, 3, 6, 10};

    for (auto node_mapping = 0; node_mapping < 3; ++node_mapping) {
        // Use `expected_distance_to_start` helper vector to get the expected distances.
        const auto node_expected_distance_along_path_to_start = expected_distance_to_start[node_mapping];
        const auto node_expected_distance_along_path_to_end = expected_distance_to_start.back() -
                                                              expected_distance_to_start[node_mapping];
        ASSERT_EQUAL(sgg_path.distance_along_path_to_start(node_mapping), node_expected_distance_along_path_to_start);
        ASSERT_EQUAL(sgg_path.distance_along_path_to_end(node_mapping), node_expected_distance_along_path_to_end);

        for (auto node2_mapping = 0; node2_mapping < 3; ++node2_mapping) {
            const auto node2_expected_distance_along_path_to_start = expected_distance_to_start[node2_mapping];

            // The distance between the nodes is the difference between the distances to start.
            const auto expected_distance_along_path =
                node_expected_distance_along_path_to_start > node2_expected_distance_along_path_to_start
                ? node_expected_distance_along_path_to_start - node2_expected_distance_along_path_to_start
                : node2_expected_distance_along_path_to_start - node_expected_distance_along_path_to_start;

            ASSERT_EQUAL(sgg_path.distance_along_path(node_mapping, node2_mapping), expected_distance_along_path);
        }
    }

    return true;
}

bool test_unit_sgg_path_reserved_bytes() {
    const auto test_adj = make_test_sgg_edges({
        {0, 1, 1}, {1, 2, 2}, {2, 3, 3}, {3, 4, 4}, {4, 5, 5},
        {5, 6, 6}, {6, 7, 7}, {7, 8, 8}, {8, 9, 100}, {8, 10, 100}
    });

    std::vector<std::size_t> nodes_in_path;
    const auto sgg_path = SGGPath(0, {1, 1}, test_adj, nodes_in_path);

    std::size_t expected_reserved_bytes = sizeof(sgg_path) + nodes_in_path.capacity() * sizeof(std::size_t);

    ASSERT_EQUAL(sgg_path.reserved_bytes(), expected_reserved_bytes);

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_unit_sgg_path_trivial_2_node_path),
        TEST(test_unit_sgg_path_proper_path),
        TEST(test_unit_sgg_path_loop),
        TEST(test_unit_sgg_path_reserved_bytes),
    };

    return Test::run_suite("test_unit_sgg_path", tests);
}
