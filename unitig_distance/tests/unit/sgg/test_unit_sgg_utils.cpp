/*
 * test_unit_sgg_utils.cpp - Unit tests for single-genome graph-related utility functions defined in sgg/sgg_utils.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <cstddef>
#include <cstdint>

#include "common/test_harness/Test.hpp"
#include "unitig_distance/sgg/sgg_utils.hpp"

namespace PANGWES {
namespace {

bool test_unit_sgg_utils_node_unitig_conversions() {
    // Each unitig id U is represented by two de Bruijn graph nodes: its left (U * 2) and right (U * 2 + 1) sides.
    for (std::size_t unitig_id = 0; unitig_id < 256; ++unitig_id) {
        const auto node_left = unitig_id * 2;
        const auto node_right = unitig_id * 2 + 1;

        ASSERT_EQUAL(node_left, SGGUtils::node_other_side(node_right));
        ASSERT_EQUAL(node_right, SGGUtils::node_other_side(node_left));

        ASSERT_EQUAL(unitig_id, SGGUtils::node_to_unitig(node_left));
        ASSERT_EQUAL(unitig_id, SGGUtils::node_to_unitig(node_right));

        ASSERT_EQUAL(node_left, SGGUtils::unitig_to_left_node(unitig_id));
        ASSERT_EQUAL(node_right, SGGUtils::unitig_to_right_node(unitig_id));
    }

    return true;
}

bool test_unit_sgg_utils_find_edge() {
    const auto edges = EdgeListT{ Edge{10, 1}, Edge{5, 2}, Edge{15, 3} };
    constexpr auto nonexisting_endpoint = 100;

    for (const auto endpoint : {10, 5, 15}) {
        const auto edge_it = SGGUtils::find_edge(edges, endpoint);

        ASSERT_FALSE(edge_it == edges.end());
        ASSERT_EQUAL(edge_it->endpoint, endpoint);
    }

    ASSERT_TRUE(SGGUtils::find_edge(edges, nonexisting_endpoint) == edges.end());

    return true;
}

bool test_unit_sgg_utils_find_next_edge_in_path() {
    const auto edges = EdgeListT{ Edge{2, 1}, Edge{0, 1} };
    constexpr auto parent = 0;
    constexpr auto next_node = 2;

    const auto next_edge = SGGUtils::find_next_edge_in_path(edges, parent);

    ASSERT_EQUAL(next_edge.endpoint, next_node);

    return true;
}

bool test_unit_sgg_utils_find_next_edge_in_path_other_endpoint() {
    const auto edges = EdgeListT{ Edge{0, 1}, Edge{2, 1} };
    constexpr auto parent = 0;
    constexpr auto next_node = 2;

    const auto next_edge = SGGUtils::find_next_edge_in_path(edges, parent);

    ASSERT_EQUAL(next_edge.endpoint, next_node);

    return true;
}

bool test_unit_sgg_utils_find_next_edge_in_path_bad_edge_list() {
    const auto edges_0 = EdgeListT{};
    const auto edges_1 = EdgeListT{ Edge{0, 0} };
    const auto edges_3 = EdgeListT{ Edge{0, 0}, Edge{1, 1}, Edge{2, 2} };

    EXPECT_THROW(SGGUtils::find_next_edge_in_path(edges_0, 0), ErrorCode::INVALID_ARGUMENT);
    EXPECT_THROW(SGGUtils::find_next_edge_in_path(edges_1, 0), ErrorCode::INVALID_ARGUMENT);
    EXPECT_THROW(SGGUtils::find_next_edge_in_path(edges_3, 0), ErrorCode::INVALID_ARGUMENT);

    return true;
}

// Checks that an edge from `node1` to `node2` with weight `weight` was added correctly.
bool check_add_edge(const AdjacencyListT& adj, std::size_t node1, std::size_t node2, std::uint64_t weight) {
    const auto& node1_edge_list = adj.at(node1);
    const auto& node2_edge_list = adj.at(node2);

    ASSERT_TRUE(node1_edge_list.size() == 1);
    ASSERT_TRUE(node2_edge_list.size() == 1);
    ASSERT_TRUE(node1_edge_list.front().endpoint == node2);
    ASSERT_TRUE(node1_edge_list.front().weight == weight);
    ASSERT_TRUE(node2_edge_list.front().endpoint == node1);
    ASSERT_TRUE(node2_edge_list.front().weight == weight);

    return true;
}

bool test_unit_sgg_utils_add_edge() {
    AdjacencyListT adj(2);

    SGGUtils::add_edge(adj, 0, 1, 10);

    return check_add_edge(adj, 0, 1, 10);
}

bool test_unit_sgg_utils_add_edge_self_edge() {
    AdjacencyListT adj(2);

    SGGUtils::add_edge(adj, 0, 0, 10);

    ASSERT_TRUE(adj.at(0).empty());

    return true;
}

bool test_unit_sgg_utils_add_edge_duplicate_edge() {
    AdjacencyListT adj(2);

    SGGUtils::add_edge(adj, 0, 1, 10);

    // Add the same edge again.
    SGGUtils::add_edge(adj, 0, 1, 10);

    // Check that the corresponding edge-lists are correct and still have size 1.
    if (!check_add_edge(adj, 0, 1, 10)) {
        return false;
    }

    // Add the same edge again with opposite end points.
    SGGUtils::add_edge(adj, 1, 0, 10);

    // Check that the corresponding edge-lists are unaffected.
    return check_add_edge(adj, 0, 1, 10);
}

bool test_unit_sgg_utils_add_edge_update_edge() {
    AdjacencyListT adj(2);

    SGGUtils::add_edge(adj, 0, 1, 10);

    // Try to update the weight to a larger weight.
    SGGUtils::add_edge(adj, 0, 1, 15);

    // Check that the corresponding edge-lists are unaffected.
    if (!check_add_edge(adj, 0, 1, 10)) {
        return false;
    }

    // Update the weight to a smaller weight.
    SGGUtils::add_edge(adj, 0, 1, 5);

    // Check that the weight got updated.
    return check_add_edge(adj, 0, 1, 5);
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_unit_sgg_utils_node_unitig_conversions),
        TEST(test_unit_sgg_utils_find_edge),
        TEST(test_unit_sgg_utils_find_next_edge_in_path),
        TEST(test_unit_sgg_utils_find_next_edge_in_path_other_endpoint),
        TEST(test_unit_sgg_utils_find_next_edge_in_path_bad_edge_list),
        TEST(test_unit_sgg_utils_add_edge),
        TEST(test_unit_sgg_utils_add_edge_self_edge),
        TEST(test_unit_sgg_utils_add_edge_duplicate_edge),
        TEST(test_unit_sgg_utils_add_edge_update_edge),
    };

    return Test::run_suite("test_unit_sgg_utils", tests);
}
