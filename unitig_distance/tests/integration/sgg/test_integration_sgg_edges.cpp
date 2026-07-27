/*
 * test_integration_sgg_edges.cpp - Integration tests for sgg/SGGEdges.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <string>

#include "common/test_harness/Test.hpp"
#include "common/io/FileReader.hpp"
#include "common/utils/memory.hpp"
#include "test_integration_sgg_data.hpp"
#include "unitig_distance/core/UnitigWeights.hpp"
#include "unitig_distance/sgg/SGGEdges.hpp"
#include "unitig_distance/sgg/sgg_utils.hpp"

namespace PANGWES {
namespace {

// Checks expectations for some SGG edges read from the test data files.
bool check_sgg_edges(const std::string& sgg_edges_filename,
                     const SGGTestData::ExpectedSGGEdgesData& expected_sgg_edges_data)
{
    const auto unitig_weights = UnitigWeights(Memory::make_unique<FileReader>(test_unitigs_filename), test_kmer_length);
    const auto sgg_edges = SGGEdges(Memory::make_unique<FileReader>(sgg_edges_filename), unitig_weights);

    ASSERT_EQUAL(sgg_edges.n_nodes(), expected_sgg_edges_data.adj_size);
    ASSERT_LESS_EQUAL(sgg_edges.size(), unitig_weights.size() * 2);

    for (const auto& node_data : expected_sgg_edges_data.non_path_nodes) {
        const auto node = node_data.node;
        const auto unitig = SGGUtils::node_to_unitig(node);

        ASSERT_TRUE(sgg_edges.contains(unitig));
        ASSERT_TRUE(unitig_weights.contains(unitig));

        const auto node_edges_size = node_data.edges_size;
        ASSERT_EQUAL(sgg_edges[node].size(), node_edges_size);
    }

    return true;
}

bool test_integration_sgg_edges() {
    for (auto i = 0; i < test_n_paths; ++i) {
        const auto sgg_edges_filename = std::string{test_paths_dir} + "/" + std::to_string(i + 1) + ".edges";

        if (!check_sgg_edges(sgg_edges_filename, SGGTestData::expected_sgg_edges_info[i])) {
            return false;
        }
    }

    return true;
}

bool check_sgg_edges_reserved_bytes(const std::string& sgg_edges_filename,
                                    const SGGTestData::ExpectedSGGEdgesData& expected_sgg_edges_data)
{
    const auto unitig_weights = UnitigWeights(Memory::make_unique<FileReader>(test_unitigs_filename), test_kmer_length);
    const auto sgg_edges = SGGEdges(Memory::make_unique<FileReader>(sgg_edges_filename), unitig_weights);

    std::size_t expected_reserved_bytes = sizeof(sgg_edges) +
                                          sgg_edges.capacity() * sizeof(Traits::element_type_t<SGGEdges>);
    for (std::size_t idx = 0; idx < sgg_edges.size(); ++idx) {
        const auto& edge_list = sgg_edges[idx];

        expected_reserved_bytes += edge_list.capacity() * sizeof(Traits::element_type_t<decltype(edge_list)>);
    }

    ASSERT_EQUAL(sgg_edges.reserved_bytes(), expected_reserved_bytes);

    return true;
}

bool test_integration_sgg_edges_reserved_bytes() {
    for (auto i = 0; i < test_n_paths; ++i) {
        const auto sgg_edges_filename = std::string{test_paths_dir} + "/" + std::to_string(i + 1) + ".edges";

        if (!check_sgg_edges_reserved_bytes(sgg_edges_filename, SGGTestData::expected_sgg_edges_info[i])) {
            return false;
        }
    }

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_integration_sgg_edges),
        TEST(test_integration_sgg_edges_reserved_bytes),
    };

    return Test::run_suite("test_integration_sgg_edges", tests);
}
