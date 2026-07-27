/*
 * test_unit_sgg_node_map.cpp - Unit tests for sgg/SGGNodeMap.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <algorithm>
#include <cstddef>
#include <limits>
#include <vector>

#include "common/test_harness/Test.hpp"
#include "common/utils/utils.hpp"
#include "unitig_distance/sgg/SGGNodeMap.hpp"

namespace PANGWES {
namespace {

// Helper structure for creating a SGGNodeMap.
struct TestMappedNode {
    std::size_t node;
    std::size_t node_mapping;
    std::size_t path_mapping;
};

// Checks that the SGGNodeMap contains the correct mapping information.
bool check_node_map_data(const SGGNodeMap& node_map, const std::vector<TestMappedNode>& expected_node_data) {
    for (std::size_t idx = 0; idx < expected_node_data.size(); ++idx) {
        const auto node = expected_node_data[idx].node;
        const auto expected_node_mapping = expected_node_data[idx].node_mapping;
        const auto expected_path_mapping = expected_node_data[idx].path_mapping;

        ASSERT_EQUAL(node_map.node_mapping(node), expected_node_mapping);
        ASSERT_EQUAL(node_map.path_mapping(node), expected_path_mapping);

        if (expected_node_mapping != SGG_NODE_NOT_MAPPED) {
            ASSERT_TRUE(node_map.is_mapped(node));
        }

        if (expected_path_mapping != SGG_NODE_NOT_MAPPED) {
            ASSERT_TRUE(node_map.is_on_path(node));
        }
    }

    return true;
}

// Makes an SGGNodeMap from the provided node data.
SGGNodeMap test_make_node_map(const std::vector<TestMappedNode>& test_node_data) {
    SGGNodeMap node_map;

    assert(!test_node_data.empty());

    const auto max_element = std::max_element(test_node_data.begin(),
                                              test_node_data.end(),
                                              [](const TestMappedNode& node1, const TestMappedNode& node2)
    {
        return node1.node < node2.node;
    });

    node_map.resize(max_element->node + 1);

    for (const auto& node_data : test_node_data) {
        const auto node = node_data.node;
        const auto node_mapping = node_data.node_mapping;
        const auto path_mapping = node_data.path_mapping;

        node_map.map_node(node, node_mapping, path_mapping);
    }

    return node_map;
}

// Tests that the default constructor doesn't throw and initializes an empty node map.
bool test_unit_sgg_node_map_constructor() {
    SGGNodeMap node_map;

    ASSERT_EQUAL(node_map.size(), 0);

    return true;
}

bool test_unit_sgg_node_map_resize() {
    SGGNodeMap node_map;

    node_map.resize(5);
    ASSERT_EQUAL(node_map.size(), 5);

    node_map.resize(10);
    ASSERT_EQUAL(node_map.size(), 10);

    // Resize back to a smaller size.
    node_map.resize(5);
    ASSERT_EQUAL(node_map.size(), 5);

    // Omit testing resize with value-initialization (unused use-case).

    return true;
}

bool test_unit_sgg_node_map_unmapped_nodes() {
    SGGNodeMap node_map;

    node_map.resize(1);

    // Resize by default initializes the elements not mapped.
    ASSERT_EQUAL(node_map.node_mapping(0), SGG_NODE_NOT_MAPPED);
    ASSERT_EQUAL(node_map.path_mapping(0), SGG_NODE_NOT_MAPPED);
    ASSERT_FALSE(node_map.is_mapped(0));
    ASSERT_FALSE(node_map.is_on_path(0));

    // Nodes ids past the container size should also not be mapped.
    ASSERT_EQUAL(node_map.node_mapping(node_map.size()), SGG_NODE_NOT_MAPPED);
    ASSERT_EQUAL(node_map.path_mapping(node_map.size()), SGG_NODE_NOT_MAPPED);
    ASSERT_FALSE(node_map.is_mapped(node_map.size()));
    ASSERT_FALSE(node_map.is_on_path(node_map.size()));

    return true;
}

bool test_unit_sgg_node_map_map_node_as_sgg_node() {
    const auto test_node_data = std::vector<TestMappedNode>{
        {5, 0, SGG_NODE_NOT_MAPPED},
        {15, 1, SGG_NODE_NOT_MAPPED},
        {0, 2, SGG_NODE_NOT_MAPPED}
    };

    const auto node_map = test_make_node_map(test_node_data);

    return check_node_map_data(node_map, test_node_data);
}

bool test_unit_sgg_node_map_map_node_default_not_on_path() {
    SGGNodeMap node_map;

    node_map.resize(1);

    node_map.map_node(0, 10);

    ASSERT_EQUAL(node_map.node_mapping(0), 10);
    ASSERT_EQUAL(node_map.path_mapping(0), SGG_NODE_NOT_MAPPED);
    ASSERT_FALSE(node_map.is_on_path(0));

    return true;
}

bool test_unit_sgg_node_map_map_node_as_sgg_path() {
    const auto test_node_data = std::vector<TestMappedNode>{
        {5, 0, 0}, {15, 1, 0}, {0, 2, 0},
        // Map proper nodes with same indices as the path nodes.
        {1, 0, SGG_NODE_NOT_MAPPED}, {2, 1, SGG_NODE_NOT_MAPPED},
        {11, 5, 1}, {12, 15, 1}, {13, 22, 1},
    };

    const auto node_map = test_make_node_map(test_node_data);

    return check_node_map_data(node_map, test_node_data);
}

bool test_unit_sgg_node_map_reserved_bytes() {
    const auto node_map = test_make_node_map(std::vector<TestMappedNode>{
        {0, 0, SGG_NODE_NOT_MAPPED}, {1, 1, SGG_NODE_NOT_MAPPED}, {2, 2, SGG_NODE_NOT_MAPPED},
        {3, 3, SGG_NODE_NOT_MAPPED}, {4, 4, SGG_NODE_NOT_MAPPED}, {5, 0, 0},
        {5, 1, 0}, {5, 2, 0}, {5, 3, 0},
    });

    std::size_t expected_reserved_bytes =
        sizeof(node_map) + node_map.capacity() * sizeof(std::pair<std::size_t, std::size_t>);

    ASSERT_EQUAL(node_map.reserved_bytes(), expected_reserved_bytes);

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_unit_sgg_node_map_constructor),
        TEST(test_unit_sgg_node_map_resize),
        TEST(test_unit_sgg_node_map_unmapped_nodes),
        TEST(test_unit_sgg_node_map_map_node_as_sgg_node),
        TEST(test_unit_sgg_node_map_map_node_default_not_on_path),
        TEST(test_unit_sgg_node_map_map_node_as_sgg_path),
        TEST(test_unit_sgg_node_map_reserved_bytes),
    };

    return Test::run_suite("test_unit_sgg_node_map", tests);
}
