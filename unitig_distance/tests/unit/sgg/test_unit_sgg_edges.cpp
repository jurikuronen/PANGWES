/*
 * test_unit_sgg_edges.cpp - Unit tests for sgg/SGGEdges.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <string>
#include <vector>

#include "common/test_harness/Test.hpp"
#include "common/type_traits/type_traits.hpp"
#include "common/utils/Exception.hpp"
#include "common/utils/memory.hpp"
#include "mocks/MockFileReader.hpp"
#include "unitig_distance/sgg/SGGEdges.hpp"
#include "unitig_distance/sgg/sgg_utils.hpp"

namespace PANGWES {
namespace {

// Duplicated here from sgg/SGGEdges.cpp.
constexpr auto N_REQUIRED_EDGES_FIELDS = 3;

// Simple mock of UnitigWeights because the default constructor is not part of the public API.
class MockUnitigWeights : public UnitigWeights {
public:
    MockUnitigWeights() = default;
    MockUnitigWeights(std::unique_ptr<FileReaderInterface> reader, std::uint64_t kmer_length)
        : UnitigWeights(std::move(reader), kmer_length)
    { }
};

// Set in test_setup().
MockUnitigWeights test_unitig_weights{};

// These tests rely on UnitigWeights created with mock data.
bool test_setup() {
    constexpr auto mock_kmer_length = 5;

    // Mock unitigs for testing purposes. They naturally don't form valid overlaps or connections in a de Bruijn graph.
    Mocks::MockIfStream::set_contents({
        "0 AAAAA",
        "1 AAAAAA",
        "2 AAAAAAA",
        "3 AAAAAAAA",
        "4 AAAAAAAAA",
        "5 AAAAAAAAAA",
        "6 AAAAAAAAAAA",
        "7 AAAAAAAAAAAA",
        "8 AAAAAAAAAAAAA",
        "9 AAAAAAAAAAAAAA"
    });
    test_unitig_weights = MockUnitigWeights(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file),
                                            mock_kmer_length);

    ASSERT_EQUAL(test_unitig_weights.size(), 10);

    for (auto i = 0; i < 10; ++i) {
        ASSERT_EQUAL(test_unitig_weights.weight(i), i);
    }

    return true;
}

bool test_unit_sgg_edges_constructor() {
    Mocks::MockIfStream::set_contents(
    {
        "0 1 FF",
        "1 2 FF",
        "2 3 FF",
        "3 4 FF"
    });

    const auto sgg_edges = SGGEdges(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file),
                                    test_unitig_weights);

    ASSERT_FALSE(sgg_edges.empty());
    // Five unique nodes, each with left and right side.
    ASSERT_EQUAL(sgg_edges.n_nodes(), 10);
    // Max node is 4's right side.
    ASSERT_GREATER_EQUAL(sgg_edges.size(), SGGUtils::unitig_to_right_node(4) + 1);

    return true;
}

bool test_unit_sgg_edges_constructor_empty_lines() {
    auto contents = std::vector<std::string>(
    {
        "0 1 FF",
        "1 2 FF",
        "2 3 FF",
    });

    for (std::size_t idx = contents.size() - 1; idx > 0; --idx) {
        // Clear line `idx`.
        contents[idx] = "";
        Mocks::MockIfStream::set_contents(contents);

        const auto sgg_edges = SGGEdges(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file),
                                        test_unitig_weights);
        // At line `idx`, there's `idx + 1` unique nodes, each with left and right side.
        const auto expected_size = 2 * (idx + 1);

        ASSERT_EQUAL(sgg_edges.n_nodes(), expected_size);
    }

    // Clear the last (first) line.
    contents.front() = "";
    Mocks::MockIfStream::set_contents(contents);

    EXPECT_THROW(SGGEdges(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file), test_unitig_weights),
                 ErrorCode::FILE_EMPTY);

    return true;
}

bool test_unit_sgg_edges_constructor_wrong_column_count() {
    constexpr auto valid_row = "0 1 FF";
    std::string invalid_row{};

    for (auto n_columns = 1; n_columns < N_REQUIRED_EDGES_FIELDS; ++n_columns) {
        // Contents don't matter for this test.
        invalid_row += ". ";

        // First line column count wrong.
        Mocks::MockIfStream::set_contents({invalid_row, valid_row, valid_row});
        EXPECT_THROW(SGGEdges(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file), test_unitig_weights),
                     ErrorCode::FILE_WRONG_COLUMN_COUNT);

        // Second line column count wrong.
        Mocks::MockIfStream::set_contents({valid_row, invalid_row, valid_row});
        EXPECT_THROW(SGGEdges(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file), test_unitig_weights),
                     ErrorCode::FILE_WRONG_COLUMN_COUNT);

        // Third line column count wrong.
        Mocks::MockIfStream::set_contents({valid_row, valid_row, invalid_row});
        EXPECT_THROW(SGGEdges(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file), test_unitig_weights),
                     ErrorCode::FILE_WRONG_COLUMN_COUNT);
    }

    return true;
}

bool test_unit_sgg_edges_constructor_bad_data() {
    constexpr auto valid_row = "0 1 FF";

    using T = std::string;
    for (const auto& invalid_row : {T{"a 1 FF"}, T{"0 a FF"}, T{"0 1 0"}, T{"0 1 AB"}}) {
        // First line has bad data.
        Mocks::MockIfStream::set_contents({invalid_row, valid_row, valid_row});
        EXPECT_THROW(SGGEdges(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file), test_unitig_weights),
                     ErrorCode::FILE_BAD_DATA);

        // Second line has bad data.
        Mocks::MockIfStream::set_contents({valid_row, invalid_row, valid_row});
        EXPECT_THROW(SGGEdges(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file), test_unitig_weights),
                     ErrorCode::FILE_BAD_DATA);

        // Third line has bad data.
        Mocks::MockIfStream::set_contents({valid_row, valid_row, invalid_row});
        EXPECT_THROW(SGGEdges(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file), test_unitig_weights),
                     ErrorCode::FILE_BAD_DATA);
    }

    return true;
}

bool test_unit_sgg_edges_constructor_invalid_unitig_id() {
    // Read an out-of-bounds positive unitig ID (test_unitig_weights max ID is 9).
    Mocks::MockIfStream::set_contents({"0 10 FF"});

    // Caught by `adj.at(node)` after unitig to node conversion.
    EXPECT_THROW(SGGEdges(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file), test_unitig_weights),
                 ErrorCode::FILE_BAD_DATA);

    // Read an invalid negative unitig ID.
    Mocks::MockIfStream::set_contents({"0 -1 FF"});

    // This will be a nested error: the check throws INVALID_UNITIG_ID, which is packed into FILE_BAD_DATA.
    EXPECT_THROW(SGGEdges(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file), test_unitig_weights),
                 ErrorCode::FILE_BAD_DATA);

    return true;
}

// Check that self-edges are properly ignored.
bool test_unit_sgg_edges_constructor_self_edges() {
    Mocks::MockIfStream::set_contents({ "0 1 FR", "0 0 FR" });

    const auto sgg_edges = SGGEdges(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file),
                                    test_unitig_weights);

    ASSERT_FALSE(sgg_edges.empty());
    // Two unique nodes, each with left and right side.
    ASSERT_EQUAL(sgg_edges.n_nodes(), 4);
    // Max node is 1's right side.
    ASSERT_GREATER_EQUAL(sgg_edges.size(), SGGUtils::unitig_to_right_node(1) + 1);

    return true;
}

bool test_unit_sgg_edges_constructor_duplicate_edges() {
    Mocks::MockIfStream::set_contents({ "0 1 FR", "0 1 FR" });

    const auto sgg_edges = SGGEdges(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file),
                                    test_unitig_weights);

    ASSERT_FALSE(sgg_edges.empty());
    // Two unique nodes, each with left and right side.
    ASSERT_EQUAL(sgg_edges.n_nodes(), 4);
    // Max node is 1's right side.
    ASSERT_GREATER_EQUAL(sgg_edges.size(), SGGUtils::unitig_to_right_node(1) + 1);

    return true;
}

bool test_unit_sgg_edges_constructor_unitig_left_to_right_edges() {
    Mocks::MockIfStream::set_contents({ "0 1 FR", "0 0 FF", "8 9 FR", "8 8 FF" });

    const auto sgg_edges = SGGEdges(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file),
                                    test_unitig_weights);

    ASSERT_FALSE(sgg_edges.empty());
    // Four unique nodes, each with left and right side.
    ASSERT_EQUAL(sgg_edges.n_nodes(), 8);
    // Max node is 9's right side.
    ASSERT_GREATER_EQUAL(sgg_edges.size(), SGGUtils::unitig_to_right_node(9) + 1);


    const auto v0_left_edges = sgg_edges[SGGUtils::unitig_to_left_node(0)];
    ASSERT_GREATER(v0_left_edges.size(), 0);

    // Check that the first edge added for v0_left is its right side.
    ASSERT_EQUAL(v0_left_edges.front().endpoint, SGGUtils::unitig_to_right_node(0));

    /*
     * Unitig 0's length is equal to the mock k-mer length, so its weight is 0. Here we check that the edge
     * "0 0 FF", that is the edge (v0_left, v0_right), does not update the weight to 1.
    */
    ASSERT_EQUAL(v0_left_edges.front().weight, 0);

    const auto v8_left_edges = sgg_edges[SGGUtils::unitig_to_left_node(8)];
    ASSERT_GREATER(v8_left_edges.size(), 0);

    // Check that the first edge added for v8_left is its right side.
    ASSERT_EQUAL(v8_left_edges.front().endpoint, SGGUtils::unitig_to_right_node(8));

    /*
     * Unitig 8's length is 13, so its weight is 13-5=8. Here we check that the edge "8 8 FF", that is the edge
     * (v8_left, v8_right), updates the edge weight to 1.
    */
    ASSERT_EQUAL(v8_left_edges.front().weight, 1);

    return true;
}

bool check_sgg_edges_reserved_bytes(const SGGEdges& sgg_edges) {
    std::size_t expected_reserved_bytes = sizeof(sgg_edges) +
                                          sgg_edges.capacity() * sizeof(Traits::element_type_t<SGGEdges>);
    for (std::size_t idx = 0; idx < sgg_edges.capacity(); ++idx) {
        const auto& edge_list = sgg_edges[idx];

        expected_reserved_bytes += edge_list.capacity() * sizeof(Traits::element_type_t<decltype(edge_list)>);
    }

    ASSERT_EQUAL(sgg_edges.reserved_bytes(), expected_reserved_bytes);

    return true;
}

bool test_unit_sgg_edges_reserved_bytes() {
    Mocks::MockIfStream::set_contents(
    {
        "0 1 FF",
        "1 2 FF",
        "2 3 FF",
        "3 4 FF"
    });

    const auto sgg_edges = SGGEdges(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file),
                                    test_unitig_weights);

    return check_sgg_edges_reserved_bytes(sgg_edges);
}

bool test_unit_sgg_edges_deprecated_0M_overlap_ignored() {
    Mocks::MockIfStream::set_contents({ "0 1 FR 5M", "1 2 FR 5M", "2 3 FR 0M" });

    const auto sgg_edges = SGGEdges(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file),
                                    test_unitig_weights);

    ASSERT_FALSE(sgg_edges.empty());
    // The edge (2, 3) is skipped (0M overlap), leaving three unique nodes, each with left and right side.
    ASSERT_EQUAL(sgg_edges.n_nodes(), 6);
    // Max node is 2's right side.
    ASSERT_GREATER_EQUAL(sgg_edges.size(), SGGUtils::unitig_to_right_node(2) + 1);

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_unit_sgg_edges_constructor),
        TEST(test_unit_sgg_edges_constructor_empty_lines),
        TEST(test_unit_sgg_edges_constructor_wrong_column_count),
        TEST(test_unit_sgg_edges_constructor_bad_data),
        TEST(test_unit_sgg_edges_constructor_invalid_unitig_id),
        TEST(test_unit_sgg_edges_constructor_self_edges),
        TEST(test_unit_sgg_edges_constructor_duplicate_edges),
        TEST(test_unit_sgg_edges_constructor_unitig_left_to_right_edges),
        TEST(test_unit_sgg_edges_reserved_bytes),
        TEST(test_unit_sgg_edges_deprecated_0M_overlap_ignored),
    };

    return Test::run_suite("test_unit_sgg_edges", tests, test_setup);
}
