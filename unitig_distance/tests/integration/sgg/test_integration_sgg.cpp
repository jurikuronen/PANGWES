/*
 * test_integration_sgg.cpp - Integration tests for sgg/SGG.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <unordered_set>
#include <vector>

#include "common/io/FileReader.hpp"
#include "common/test_harness/Test.hpp"
#include "common/utils/memory.hpp"
#include "mocks/MockFileReader.hpp"
#include "test_integration_sgg_data.hpp"
#include "unitig_distance/core/DistanceQueryBatch.hpp"
#include "unitig_distance/core/UnitigWeights.hpp"
#include "unitig_distance/sgg/SGG.hpp"
#include "unitig_distance/sgg/SGGEdges.hpp"
#include "unitig_distance/sgg/sgg_utils.hpp"

namespace PANGWES {
namespace {

/*
 * Component 1 in SGGEdges returned in `create_test_sgg_edges_with_2_disconnected_components`:
 *
 *              0l - 0r - 1l - 1r - 2l - 2r - 3l - 3r
 *                                        \        |
 *                                         \- 4l - 4r
*/
const std::vector<std::size_t> test_sgg_edges_component1_real_nodes{0, 5};
const std::vector<std::size_t> test_sgg_edges_component1_path_nodes{1, 2, 3, 4, 6, 7, 8, 9};

/*
 * Component 2 in SGGEdges returned in `create_test_sgg_edges_with_2_disconnected_components`:
 *
 *              5l - 5r - 6l - 6r - 7l - 7r - 8l - 8r
 *                                   \     \       /
 *                                    \ - - 9l - 9r
*/
const std::vector<std::size_t> test_sgg_edges_component2_real_nodes{10, 14, 15, 18};
const std::vector<std::size_t> test_sgg_edges_component2_path_nodes{11, 12, 13, 16, 17, 19};

/*
 * Creates SGGEdges from mocked data. They naturally don't form valid overlaps or connections in a de Bruijn graph, but
 * for testing purposes it doesn't matter.
*/
SGGEdges create_test_sgg_edges_with_2_disconnected_components() {
    const auto unitig_weights = UnitigWeights(Memory::make_unique<FileReader>(test_unitigs_filename), test_kmer_length);

    Mocks::MockIfStream::set_contents(
    {
        "0 1 FF 30M", "1 2 FF 30M", "2 3 FF 30M", "2 4 FF 30M", "3 4 FR 30M",
        "5 6 FF 30M", "6 7 FF 30M", "7 8 FF 30M", "7 9 RF 30M", "7 9 FF 30M", "8 9 FR 30M",
    });

    return SGGEdges(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file), unitig_weights);
}

// Verify the SGG is constructed correctly from the test data.
bool check_sgg(const std::string& sgg_edges_filename,
               const std::vector<SGGTestData::ExpectedSGGNodeInfo>& expected_sgg_data)
{
    const auto unitig_weights = UnitigWeights(Memory::make_unique<FileReader>(test_unitigs_filename), test_kmer_length);
    const auto sgg_edges = SGGEdges(Memory::make_unique<FileReader>(sgg_edges_filename), unitig_weights);
    const auto sgg = SGG(sgg_edges);

    ASSERT_EQUAL(sgg.size(), expected_sgg_data.size());

    // Store expected real nodes so that we can verify path nodes later.
    std::unordered_set<std::size_t> real_nodes;

    for (const auto& node_data : expected_sgg_data) {
        const auto node = node_data.node;
        const auto& node_edge_list = node_data.edge_list;

        real_nodes.insert(node);

        ASSERT_TRUE(sgg.contains(SGGUtils::node_to_unitig(node)));

        // The node `v` shouldn't have become a path node in the SGG.
        ASSERT_TRUE(!sgg.is_on_path(node));

        const auto node_mapped = sgg.node_mapping(node);
        const auto& sgg_node_edge_list = sgg.adj().at(node_mapped);

        ASSERT_EQUAL(sgg_node_edge_list.size(), node_edge_list.size());

        for (const auto& edge : node_edge_list) {
            const auto neighbor_mapped = sgg.node_mapping(edge.endpoint);
            const auto expected_weight = edge.weight;

            const auto edge_it = SGGUtils::find_edge(sgg_node_edge_list, neighbor_mapped);
            ASSERT_FALSE(edge_it == sgg_node_edge_list.end());

            ASSERT_EQUAL(neighbor_mapped, edge_it->endpoint);
            ASSERT_EQUAL(expected_weight, edge_it->weight);
        }
    }

    // Verify that all other nodes became path nodes.
    for (std::size_t node = 0; node < sgg_edges.size(); ++node) {
        if (real_nodes.count(node) > 0 || !sgg_edges.contains(SGGUtils::node_to_unitig(node))) {
            continue;
        }

        ASSERT_TRUE(sgg.is_on_path(node));
    }

    return true;
}

bool check_sgg_distances(const std::string& sgg_edges_filename,
                         const std::vector<SGGTestData::ExpectedSGGDistanceData>& expected_distance_data)
{
    const auto unitig_weights = UnitigWeights(Memory::make_unique<FileReader>(test_unitigs_filename), test_kmer_length);
    const auto sgg_edges = SGGEdges(Memory::make_unique<FileReader>(sgg_edges_filename), unitig_weights);
    const auto sgg = SGG(sgg_edges);

    for (const auto& data : expected_distance_data) {
        const auto node_from = data.node_from;
        const auto node_to = data.node_to;
        const auto expected_distance = data.distance;

        const auto distances = sgg.compute_unitig_distances(node_from, {DistanceQueryTarget{node_to, 0}});

        ASSERT_EQUAL(distances.size(), 1);

        const auto distance = distances.front();

        ASSERT_EQUAL(distance, expected_distance);
    }

    return true;
}

// Simple mock inheritance to get access to internal containers.
class MockSGG : public SGG {
public:
    MockSGG(const SGGEdges& sgg_edges)
        : SGG(sgg_edges)
    { }

    const std::vector<SGGPath>& paths() const {
        return m_paths;
    }

    const SGGNodeMap& node_map() const {
        return m_node_map;
    }
};

// Checks that SGG reports reserved memory correctly.
bool check_sgg_memory_usage(const std::string& sgg_edges_filename) {
    const auto unitig_weights = UnitigWeights(Memory::make_unique<FileReader>(test_unitigs_filename), test_kmer_length);
    const auto sgg_edges = SGGEdges(Memory::make_unique<FileReader>(sgg_edges_filename), unitig_weights);
    const auto sgg = MockSGG(sgg_edges);

    const auto expected_reserved_bytes = sizeof(SGG) + Memory::container_reserved_bytes(sgg.adj()) +
                                         Memory::container_reserved_bytes(sgg.paths()) +
                                         sgg.node_map().reserved_bytes();

    // Estimate a lower bound of static memory allocation.
    const auto path_node_count = sgg_edges.n_nodes() - sgg.size();
    const auto path_count = sgg.paths().size();
    assert(path_count > 0 && "SGG had no paths");
    // With path_node_count nodes in path_count paths, there must be at least one path of at least this size.
    const auto min_path_size_estimate = path_node_count / path_count;
    const auto internal_nodes_in_path_min_estimate_bytes = sizeof(std::vector<std::size_t>) +
                                                           min_path_size_estimate * sizeof(std::size_t);

    const auto bits_per_block = sizeof(std::size_t) * std::numeric_limits<unsigned char>::digits;
    const auto internal_is_target_bytes = sizeof(std::vector<bool>) +
                                          ((sgg.size() + bits_per_block - 1) / bits_per_block) * sizeof(std::size_t);

    const auto internal_graph_distances_bytes = sizeof(std::vector<uint64_t>) + sgg.size() * sizeof(std::uint64_t);

    // This covers the three thread-local vector objects.
    const auto expected_minimum_static_bytes = internal_nodes_in_path_min_estimate_bytes +
                                               internal_is_target_bytes +
                                               internal_graph_distances_bytes;

    ASSERT_EQUAL(sgg.reserved_bytes(), expected_reserved_bytes);
    ASSERT_GREATER_EQUAL(sgg.static_bytes(), expected_minimum_static_bytes);

    return true;
}

// Verifies that the SGG contains the given real or path nodes (depending on `is_on_path`).
bool check_sgg_contains_nodes(const SGG& sgg, const std::vector<std::size_t>& nodes, bool is_on_path) {
    for (const auto node : nodes) {
        ASSERT_TRUE(sgg.contains(SGGUtils::node_to_unitig(node)));
        ASSERT_EQUAL(sgg.is_on_path(node), is_on_path);
    }

    return true;
}

// Verifies that all nodes within their own component are connected (0 <= distance < INF).
bool check_same_component_distances(const SGG& sgg, const std::vector<std::size_t>& nodes) {
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        const auto unitig1_id = SGGUtils::node_to_unitig(nodes[i]);

        for (std::size_t j = i + 1; j < nodes.size(); ++j) {
            const auto unitig2_id = SGGUtils::node_to_unitig(nodes[j]);
            const auto sgg_distances = sgg.compute_unitig_distances(unitig1_id, {DistanceQueryTarget{unitig2_id, 0}});

            ASSERT_FALSE(sgg_distances.empty());

            ASSERT_GREATER_EQUAL(sgg_distances.front(), 0);
            ASSERT_LESS(sgg_distances.front(), INF_DISTANCE);
        }
    }

    return true;
}

// Verifies that distances for node pairs from different components are undefined.
bool check_different_component_distances(const SGG& sgg,
                                         const std::vector<std::size_t>& nodes1,
                                         const std::vector<std::size_t>& nodes2)
{
    for (const auto& node1 : nodes1) {
        const auto unitig1_id = SGGUtils::node_to_unitig(node1);
        for (const auto& node2 : nodes2) {
            const auto unitig2_id = SGGUtils::node_to_unitig(node2);
            const auto sgg_distances = sgg.compute_unitig_distances(unitig1_id, {DistanceQueryTarget{unitig2_id, 0}});

            // Source unitig is contained in the SGG, so the results vector should not be empty.
            ASSERT_FALSE(sgg_distances.empty());

            ASSERT_EQUAL(sgg_distances.front(), INF_DISTANCE);
        }
    }

    return true;
}

bool test_integration_sgg_constructor() {
    for (auto i = 0; i < test_n_paths; ++i) {
        const auto sgg_edges_filename = std::string{test_paths_dir} + "/" + std::to_string(i + 1) + ".edges";

        if (!check_sgg(sgg_edges_filename, SGGTestData::expected_sgg_info[i])) {
            return false;
        }
    }
    return true;
}

bool test_integration_sgg_constructor_disconnected_components() {
    const auto sgg_edges = create_test_sgg_edges_with_2_disconnected_components();
    const auto sgg = SGG(sgg_edges);

    const auto expected_size = test_sgg_edges_component1_real_nodes.size() +
                               test_sgg_edges_component2_real_nodes.size();

    ASSERT_EQUAL(sgg.size(), expected_size);

    return check_sgg_contains_nodes(sgg, test_sgg_edges_component1_real_nodes, false) &&
           check_sgg_contains_nodes(sgg, test_sgg_edges_component1_path_nodes, true) &&
           check_sgg_contains_nodes(sgg, test_sgg_edges_component2_real_nodes, false) &&
           check_sgg_contains_nodes(sgg, test_sgg_edges_component2_path_nodes, true);
}

bool test_integration_sgg_compute_unitig_distances() {
    for (auto i = 0; i < test_n_paths; ++i) {
        const auto sgg_edges_filename = std::string{test_paths_dir} + "/" + std::to_string(i + 1) + ".edges";

        if (!check_sgg_distances(sgg_edges_filename, SGGTestData::expected_sgg_distances[i])) {

            return false;
        }
    }

    return true;
}

bool test_integration_sgg_compute_unitig_distances_disconnected_components() {
    const auto sgg_edges = create_test_sgg_edges_with_2_disconnected_components();
    const auto sgg = SGG(sgg_edges);

    // Collect all nodes within a component.
    auto test_sgg_edges_component1 = test_sgg_edges_component1_real_nodes;
    test_sgg_edges_component1.insert(test_sgg_edges_component1.end(),
                                     test_sgg_edges_component1_path_nodes.begin(),
                                     test_sgg_edges_component1_path_nodes.end());


    auto test_sgg_edges_component2 = test_sgg_edges_component2_real_nodes;
    test_sgg_edges_component2.insert(test_sgg_edges_component2.end(),
                                     test_sgg_edges_component2_path_nodes.begin(),
                                     test_sgg_edges_component2_path_nodes.end());

    if (!check_same_component_distances(sgg, test_sgg_edges_component1) ||
        !check_same_component_distances(sgg, test_sgg_edges_component2))
    {
        return false;
    }

    return check_different_component_distances(sgg, test_sgg_edges_component1, test_sgg_edges_component2);
}

bool test_integration_sgg_reserved_memory() {
    for (auto i = 0; i < test_n_paths; ++i) {
        const auto sgg_edges_filename = std::string{test_paths_dir} + "/" + std::to_string(i + 1) + ".edges";

        if (!check_sgg_memory_usage(sgg_edges_filename)) {

            return false;
        }
    }

    return true;
}

// This test checks that the SGG DFS loop works correctly with disconnected nodes when the initial node is a path node.
bool test_integration_sgg_constructor_disconnect_components_start_on_path() {
    const auto unitig_weights = UnitigWeights(Memory::make_unique<FileReader>(test_unitigs_filename), test_kmer_length);

    /*
     * Component 1: 0l - 0r - 3l - 3r - 4l - 4r - 5l - 5r
     *               \                  /     \        |
     *                \ - - - - - - - -        \- 6l - 6r
     *
     * Component 2: 1l - 1r - 2l
     *               \     \  |
     *                \ - - - 2r
    */
    Mocks::MockIfStream::set_contents(
    {
        "0 3 FF 30M", "0 4 RF 30M", "3 4 FF 30M", "4 5 FF 30M", "4 6 FF 30M", "5 6 FR 30M",
        "1 2 FF 30M", "1 2 RR 30M", "1 2 FR 30M",
    });

    const auto sgg_edges = SGGEdges(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file), unitig_weights);
    const auto sgg = SGG(sgg_edges);

    const std::vector<std::size_t> comp1_real_nodes{8, 9};
    const std::vector<std::size_t> comp1_path_nodes{0, 1, 6, 7, 10, 11, 12, 13};

    // After processing component 1, the loop should return back to node 2l and add component 2 to the SGG.
    const std::vector<std::size_t> comp2_real_nodes{3, 5};
    const std::vector<std::size_t> comp2_path_nodes{2, 4};

    // Check that SGG contains all the nodes.
    if (!check_sgg_contains_nodes(sgg, comp1_real_nodes, false) ||
        !check_sgg_contains_nodes(sgg, comp1_path_nodes, true) ||
        !check_sgg_contains_nodes(sgg, comp2_real_nodes, false) ||
        !check_sgg_contains_nodes(sgg, comp2_path_nodes, true))
    {
        return false;
    }

    auto comp1_nodes = comp1_real_nodes;
    comp1_nodes.insert(comp1_nodes.end(), comp1_path_nodes.begin(), comp1_path_nodes.end());

    auto comp2_nodes = comp2_real_nodes;
    comp2_nodes.insert(comp2_nodes.end(), comp2_path_nodes.begin(), comp2_path_nodes.end());

    if (!check_same_component_distances(sgg, comp1_nodes)) {
        return false;
    }

    if (!check_same_component_distances(sgg, comp2_nodes)) {
        return false;
    }

    return check_different_component_distances(sgg, comp1_nodes, comp2_nodes);
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_integration_sgg_constructor),
        TEST(test_integration_sgg_constructor_disconnected_components),
        TEST(test_integration_sgg_constructor_disconnect_components_start_on_path),
        TEST(test_integration_sgg_compute_unitig_distances),
        TEST(test_integration_sgg_compute_unitig_distances_disconnected_components),
        TEST(test_integration_sgg_reserved_memory),
    };

    return Test::run_suite("test_integration_sgg", tests);
}
