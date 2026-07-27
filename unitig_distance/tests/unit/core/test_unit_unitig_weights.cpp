/*
 * test_unit_unitig_weights.cpp - Unit tests for core/UnitigWeights.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "common/io/Log.hpp"
#include "common/test_harness/Test.hpp"
#include "common/utils/Exception.hpp"
#include "common/utils/memory.hpp"
#include "mocks/MockFileReader.hpp"
#include "unitig_distance/core/UnitigWeights.hpp"

namespace PANGWES {
namespace {

constexpr auto mock_kmer_length = 61;
const auto mock_unitig_weights_data = std::vector<std::string>{
    "0 ACTGAAAATGGCGTTCAATTTGCTACCTATTTAAACGAAGGATTAATGACAGGCATCTTTTTA",
    "1 CTACGAAAAGATTCGCTTTTCCACAAAAGATTTACCAGAATCACAGTTTTTGTATGGCGAACAAGCACCC",
    "2 ATTCGCTTTTCCACAAAAGATTTACCAGAATCACAGTTTTTGTATGGCGAACAAGCACCCGA",
    "3 CAGTGACAAGTAATGGCTCGGGTGCTTGTTCGCCATACAAAAACTGTGATTCTGGTAAATCTTTTGTGGAAAAGCGA",
    "4 ATTTACCAGAATCACAGTTTTTGTATGGCGAACAAGCACCCGAGCCATTACTTGTCACTGAAAA",
    "5 ACCAGAATCACAGTTTTTGTATGGCGAACAAGCACCCGAGCCATTACTTGTCACTGAAAATGGC",
    "6 CGCCATTTTCAGTGACAAGTAATGGCTCGGGTGCTTGTTCGCCATACAAAAACTGTGATTC",
    "7 AAATTGAACGCCATTTTCAGTGACAAGTAATGGCTCGGGTGCTTGTTCGCCATACAAAAACTGTGATT",
    "8 GGTAGCAAATTGAACGCCATTTTCAGTGACAAGTAATGGCTCGGGTGCTTGTTCGCCATACAAAAA",
    "9 TAAATAGGTAGCAAATTGAACGCCATTTTCAGTGACAAGTAATGGCTCGGGTGCTTGTTCGCCATA",
};

std::uint64_t test_get_mock_unitig_weight(std::size_t mock_data_idx) {
    assert(mock_data_idx < mock_unitig_weights_data.size() && "index out of bounds");
    assert(mock_unitig_weights_data[mock_data_idx].size() >= mock_kmer_length - 2ULL && "unitig too short");

    /*
     * Remove the id part (assumed to be single-digit in the mock data) from the string length to get the unitig's
     * length. Finally, remove the k-mer length to get the weight.
    */
    return mock_unitig_weights_data[mock_data_idx].size() - mock_kmer_length - 2ULL;
}

bool test_setup() {
    Mocks::MockIfStream::set_contents(mock_unitig_weights_data);

    return true;
}

bool test_verify_unitig_weights_data(const UnitigWeights& unitig_weights) {
    ASSERT_EQUAL(unitig_weights.size(), mock_unitig_weights_data.size());

    for (std::size_t unitig_id = 0; unitig_id < unitig_weights.size(); ++unitig_id) {
        ASSERT_TRUE(unitig_weights.contains(unitig_id));

        const auto expected_unitig_length = test_get_mock_unitig_weight(unitig_id);
        ASSERT_EQUAL(unitig_weights.weight(unitig_id), expected_unitig_length);
    }

    return true;
}

bool test_unit_unitig_weights_constructor() {
    const auto unitig_weights = UnitigWeights(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file),
                                              mock_kmer_length);

    return test_verify_unitig_weights_data(unitig_weights);
}

bool test_unit_unitig_weights_contains_returns_false_for_unread_unitig() {
    const auto unitig_weights = UnitigWeights(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file),
                                              mock_kmer_length);

    ASSERT_FALSE(unitig_weights.contains(unitig_weights.size()));

    return true;
}

bool test_unit_unitig_weights_constructor_some_empty_lines() {
    auto contents = mock_unitig_weights_data;

    // Insert some empty lines.
    contents.insert(contents.begin(), "");
    contents.insert(contents.begin() + 3, "");
    contents.insert(contents.begin() + 6, "");

    Mocks::MockIfStream::set_contents(contents);

    const auto unitig_weights = UnitigWeights(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file),
                                              mock_kmer_length);

    return test_verify_unitig_weights_data(unitig_weights);
}

bool test_unit_unitig_weights_constructor_only_empty_lines() {
    Mocks::MockIfStream::set_contents({"", "", ""});

    EXPECT_THROW(UnitigWeights(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file),
                               mock_kmer_length),
                 ErrorCode::FILE_EMPTY);

    return true;
}

bool test_unit_unitig_weights_constructor_wrong_column_count() {
    // First line column count wrong.
    Mocks::MockIfStream::set_contents({"1", "2"});

    EXPECT_THROW(UnitigWeights(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file),
                               mock_kmer_length),
                 ErrorCode::FILE_WRONG_COLUMN_COUNT);

    // First line column count correct, second line column count wrong.
    Mocks::MockIfStream::set_contents({mock_unitig_weights_data.front(), "2"});

    EXPECT_THROW(UnitigWeights(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file),
                               mock_kmer_length),
                 ErrorCode::FILE_WRONG_COLUMN_COUNT);

    return true;
}

bool test_unit_unitig_weights_constructor_bad_kmer_length() {
    EXPECT_THROW(UnitigWeights(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file),
                               mock_kmer_length * 2ULL),
                 ErrorCode::INVALID_KMER_LENGTH);

    return true;
}

bool test_unit_unitig_weights_reserved_bytes() {
    const auto unitig_weights = UnitigWeights(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file),
                                              mock_kmer_length);

    // Can't access container capacities; compute the minimum bytes to contain all data.
    const auto minimum_reserved_bytes = sizeof(UnitigWeights) + unitig_weights.size() * sizeof(std::size_t);


    ASSERT_GREATER_EQUAL(unitig_weights.reserved_bytes(), minimum_reserved_bytes);

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_unit_unitig_weights_constructor),
        TEST(test_unit_unitig_weights_contains_returns_false_for_unread_unitig),
        TEST(test_unit_unitig_weights_constructor_some_empty_lines),
        TEST(test_unit_unitig_weights_constructor_only_empty_lines),
        TEST(test_unit_unitig_weights_constructor_wrong_column_count),
        TEST(test_unit_unitig_weights_constructor_bad_kmer_length),
        TEST(test_unit_unitig_weights_reserved_bytes),
    };

    return Test::run_suite("test_unit_unitig_weights", tests, test_setup);
}
