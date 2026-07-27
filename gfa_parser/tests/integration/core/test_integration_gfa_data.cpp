/*
 * test_integration_gfa_data.cpp - Integration tests for core/GFAData.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <cstddef>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "common/io/FileReader.hpp"
#include "common/test_harness/Test.hpp"
#include "common/utils/Exception.hpp"
#include "common/utils/memory.hpp"
#include "common/utils/WorkerPool.hpp"
#include "gfa_parser/core/GFAData.hpp"
#include "gfa_parser/core/Link.hpp"
#include "gfa_parser/core/Orientation.hpp"
#include "mocks/MockFileReader.hpp"
#include "test_integration_gfa_data_efc_data.hpp"
#include "test_integration_gfa_data_maela_data.hpp"

namespace PANGWES {
namespace {

// Overlap in the used test data files.
constexpr std::size_t TEST_DATA_EXPECTED_OVERLAP = 30;

// Starts the worker pool for GFAData involving tests and verifies that the pool is running correctly.
bool test_init_and_start_worker_pool() {
    constexpr auto test_n_workers = 4;

    WorkerPool::stop();
    WorkerPool::init_and_start(test_n_workers);

    ASSERT_FALSE(WorkerPool::stopped());
    ASSERT_TRUE(WorkerPool::is_async());

    return true;
}

bool test_setup() {
    WorkerPool::stop();
    WorkerPool::init_and_start(0);

    return true;
}

bool test_teardown() {
    WorkerPool::stop();

    return true;
}

// Converts path segment names and overlaps to csv input format.
template <typename... Strings>
std::string to_csv(const Strings&... strings) {
    std::string out;

    for (const auto& string : std::vector<std::string>{strings...}) {
        out += string + ",";
    }

    // Remove last ','.
    out.pop_back();

    return out;
}

// Creates a test segment line.
std::string segment_line(const std::string& segment_name, const std::string& segment_sequence) {
    return std::string{"S\t"} + segment_name + "\t" + segment_sequence;
}

// Creates a test link line.
std::string link_line(const std::string& from_name,
                      const std::string& from_orientation,
                      const std::string& to_name,
                      const std::string& to_orientation,
                      const std::string& overlap)
{
    return std::string{"L\t"} + from_name + "\t" + from_orientation + "\t" +
           to_name + "\t" + to_orientation + "\t" + overlap;
}

// Creates a test path line with path name formatted like Cuttlefish's path names.
std::string path_line(const std::string& reference_name,
                      const std::string& sequence_name,
                      const std::string& segment_names,
                      const std::string& overlaps)
{
    return std::string{"P\tReference:"} + reference_name + "_Sequence:" + sequence_name + "\t" +
           segment_names + "\t" + overlaps;
}

// Feed input lines into MockIfStream and run the first GFAData read pass via MockFileReader.
std::unique_ptr<GFAData> make_gfa_data(std::vector<std::string> lines, std::size_t expected_overlap) {
    // Set is_open()'s mocked result to false for the initial check and then true for the stream's open() and close().
    Mocks::MockIfStream::set_is_open_return({true, true, false});

    Mocks::MockIfStream::set_contents(std::move(lines));

    auto gfa_data = Memory::make_unique<GFAData>(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file),
                                                 expected_overlap);
    gfa_data->read_segments_links_and_reference_counts();

    return gfa_data;
}

// Runs the first GFAData read pass from a real file.
std::unique_ptr<GFAData> read_gfa_data_file(const std::string& filename, std::size_t expected_overlap) {
    auto gfa_data = Memory::make_unique<GFAData>(Memory::make_unique<FileReader>(filename), expected_overlap);
    gfa_data->read_segments_links_and_reference_counts();

    return gfa_data;
}

/*
 * Verifies that `gfa_data` has the expected number of segments, reference paths and invalidated links after the first
 * read pass.
*/
bool verify_gfa_data_after_first_pass(const GFAData& gfa_data,
                                      std::size_t expected_n_segments,
                                      std::size_t expected_n_reference_paths,
                                      std::size_t expected_n_invalid_links = 0)
{
    ASSERT_EQUAL(gfa_data.segment_name_map().size(), expected_n_segments);
    ASSERT_EQUAL(gfa_data.n_reference_paths(), expected_n_reference_paths);
    ASSERT_EQUAL(gfa_data.invalid_links().size(), expected_n_invalid_links);

    return true;
}

// Verifies deterministic segment name mappings.
bool verify_segment_mapping(const GFAData& gfa_data, const std::vector<std::string>& expected_segment_names) {
    ASSERT_EQUAL(gfa_data.segment_name_map().size(), expected_segment_names.size());

    for (std::size_t idx = 0; idx < expected_segment_names.size(); ++idx) {
        ASSERT_EQUAL(gfa_data.segment_name_map().segment_name(idx), expected_segment_names[idx]);
        ASSERT_EQUAL(gfa_data.segment_name_map().segment_name_mapping(expected_segment_names[idx]), idx);
    }

    return true;
}

bool test_integration_gfa_data_first_pass_reads_minimal_valid_input() {
    const auto gfa_data = make_gfa_data({
        segment_line("1", "ACG"),
        segment_line("2", "CGT"),
        link_line("1", "+", "2", "+", "2M"),
        path_line("1", "1.fa", to_csv("1+", "2+"), "2M"),
    }, 2);

    return verify_gfa_data_after_first_pass(*gfa_data, 2, 1) &&
           verify_segment_mapping(*gfa_data, {"1", "2"});
}

bool test_integration_gfa_data_first_pass_throws_on_no_segments() {
    EXPECT_THROW(
        make_gfa_data({
            link_line("1", "+", "2", "+", "2M"),
            path_line("1", "1.fa", to_csv("1+", "2+"), "2M"),
        }, 2),
        ErrorCode::FILE_BAD_DATA
    );

    return true;
}

bool test_integration_gfa_data_first_pass_throws_on_no_paths() {
    EXPECT_THROW(
        make_gfa_data({
            segment_line("1", "ACG"),
            segment_line("2", "CGT"),
            link_line("1", "+", "2", "+", "2M"),
        }, 2),
        ErrorCode::FILE_BAD_DATA
    );

    return true;
}

bool test_integration_gfa_data_first_pass_throws_on_duplicate_segment_name() {
    EXPECT_THROW(
        make_gfa_data({
            segment_line("1", "ACG"),
            segment_line("1", "CGT"),
            link_line("1", "+", "2", "+", "2M"),
            path_line("1", "1.fa", to_csv("1+", "2+"), "2M"),
        }, 2),
        ErrorCode::FILE_DUPLICATE_DATA
    );

    return true;
}

bool test_integration_gfa_data_first_pass_throws_on_invalid_record_type() {
    for (auto chr = '!'; chr <= '~'; ++chr) {
        if (std::strchr("SP#HLCWJ", chr) != nullptr) {
            continue;
        }
        EXPECT_THROW(
            make_gfa_data({
                segment_line("1", "ACG"),
                segment_line("2", "CGT"),
                std::string{chr},
                link_line("1", "+", "2", "+", "2M"),
                path_line("1", "1.fa", to_csv("1+", "2+"), "2M"),
            }, 2),
            ErrorCode::INVALID_RECORD_TYPE
        );
    }

    return true;
}

bool test_integration_gfa_data_first_pass_throws_on_wrong_segment_field_count() {
    EXPECT_THROW(
        make_gfa_data({
            segment_line("1", "ACG"),
            "S\t2",
            link_line("1", "+", "2", "+", "2M"),
            path_line("1", "1.fa", to_csv("1+", "2+"), "2M"),
        }, 2),
        ErrorCode::FILE_WRONG_COLUMN_COUNT
    );

    return true;
}

bool test_integration_gfa_data_first_pass_throws_on_wrong_path_field_count() {
    EXPECT_THROW(
        make_gfa_data({
            segment_line("1", "ACG"),
            segment_line("2", "CGT"),
            link_line("1", "+", "2", "+", "2M"),
            "P",
        }, 2),
        ErrorCode::FILE_WRONG_COLUMN_COUNT
    );

    return true;
}

bool test_integration_gfa_data_first_pass_throws_on_no_valid_links() {
    EXPECT_THROW(
        make_gfa_data({
            segment_line("1", "ACG"),
            segment_line("2", "CGT"),
            link_line("1", "+", "2", "+", "0M"),
            path_line("1", "1.fa", to_csv("1+", "2+"), "0M"),
        }, 2),
        ErrorCode::FILE_BAD_DATA
    );

    return true;
}

bool test_integration_gfa_data_first_pass_ignores_unsupported_valid_record_types() {
    const auto gfa_data = make_gfa_data({
        segment_line("1", "ACG"),
        segment_line("2", "CGT"),
        "#", // Comment.
        "H", // Header.
        "C", // Containment.
        "W", // Walk (since v1.1).
        "J", // Jump (since v1.2).
        link_line("1", "+", "2", "+", "2M"),
        path_line("1", "1.fa", to_csv("1+", "2+"), "2M"),
    }, 2);

    return verify_gfa_data_after_first_pass(*gfa_data, 2, 1);
}

bool test_integration_gfa_data_first_pass_allows_multiple_sequences_per_reference() {
    const auto gfa_data = make_gfa_data({
        segment_line("1", "ACG"),
        segment_line("2", "CGT"),
        segment_line("3", "GTAA"),
        link_line("1", "+", "2", "+", "2M"),
        link_line("2", "+", "3", "+", "2M"),
        path_line("1", "1.fa", to_csv("1+", "2+"), "2M"),
        path_line("1", "2.fa", to_csv("2+", "3+"), "2M"),
    }, 2);

    return verify_gfa_data_after_first_pass(*gfa_data, 3, 1);
}

bool test_integration_gfa_data_first_pass_reads_efc_data() {
    const auto gfa_data = read_gfa_data_file(test_efc_gfa_filename, TEST_DATA_EXPECTED_OVERLAP);

    ASSERT_EQUAL(gfa_data->segment_name_map().size(), EFCTestData::expected_n_segments);
    ASSERT_EQUAL(gfa_data->n_reference_paths(), EFCTestData::expected_n_reference_paths);

    return true;
}

bool test_integration_gfa_data_first_pass_reads_efc_data_multithreaded() {
    return test_init_and_start_worker_pool() && test_integration_gfa_data_first_pass_reads_efc_data();
}

bool test_integration_gfa_data_first_pass_reads_maela_data() {
    const auto gfa_data = read_gfa_data_file(test_maela_gfa_filename, TEST_DATA_EXPECTED_OVERLAP);

    ASSERT_EQUAL(gfa_data->segment_name_map().size(), MaelaTestData::expected_n_segments);
    ASSERT_EQUAL(gfa_data->n_reference_paths(), MaelaTestData::expected_n_reference_paths);

    return true;
}

bool test_integration_gfa_data_first_pass_reads_maela_data_multithreaded() {
    return test_init_and_start_worker_pool() && test_integration_gfa_data_first_pass_reads_maela_data();
}

bool test_integration_gfa_data_first_pass_maps_segments_deterministically() {
    const auto gfa_data = make_gfa_data({
        segment_line("123", "ATGCCAGACGG"),
        segment_line("456", "ACCGTCTGGCA"),
        link_line("123", "+", "456", "-", "10M"),
        path_line("1", "1.fa", to_csv("123+", "456-"), "10M"),
    }, 10);

    return verify_gfa_data_after_first_pass(*gfa_data, 2, 1) &&
           verify_segment_mapping(*gfa_data, {"123", "456"});
}

// Tests that "1" gets mapped to internal index 0 and vice versa for "0".
bool test_integration_gfa_data_first_pass_maps_segments_with_overlapping_ids() {
    const auto gfa_data = make_gfa_data({
        segment_line("1", "TTAGTTGTGCC"),
        segment_line("0", "TGGCACAACTA"),
        link_line("1", "+", "0", "-", "10M"),
        path_line("1", "1.fa", to_csv("1+", "0-"), "10M"),
    }, 10);

    return verify_gfa_data_after_first_pass(*gfa_data, 2, 1) &&
           verify_segment_mapping(*gfa_data, {"1", "0"});
}

bool test_integration_gfa_data_first_pass_throws_on_link_with_unmapped_segment() {
    EXPECT_THROW(
        make_gfa_data({
            segment_line("123", "ATGCCAGACGG"),
            link_line("123", "+", "12345", "-", "10M"),
            path_line("1", "1.fa", to_csv("123+", "12345-"), "10M"),
        }, 10),
        ErrorCode::FILE_BAD_DATA
    );

    return true;
}

// The first read pass only counts Path references; it should not validate Path segment names or overlaps.
bool test_integration_gfa_data_first_pass_ignores_path_segments_and_overlaps() {
    const auto gfa_data = make_gfa_data({
        segment_line("1", "TTTTTTTTTTT"),
        segment_line("2", "GATGCCAGACG"),
        segment_line("3", "ATGCCAGACGC"),
        link_line("2", "+", "3", "+", "10M"),
        path_line("1", "1.fa", to_csv("1+", "2+"), "100M"),
        path_line("1", "2.fa", to_csv("2+", "missing+"), "10M"),
    }, 10);

    return verify_gfa_data_after_first_pass(*gfa_data, 3, 1) &&
           verify_segment_mapping(*gfa_data, {"1", "2", "3"});
}

bool test_integration_gfa_data_first_pass_supports_lowercase_bases() {
    const auto gfa_data = make_gfa_data({
        segment_line("1", "ttagttgtgcc"),
        segment_line("2", "tagttgtgccg"),
        link_line("1", "+", "2", "+", "10M"),
        path_line("1", "1.fa", to_csv("1+", "2+"), "10M"),
    }, 10);

    return verify_gfa_data_after_first_pass(*gfa_data, 2, 1);
}

bool test_integration_gfa_data_first_pass_adds_invalid_overlap_to_invalid_links() {
    const auto gfa_data = make_gfa_data({
        segment_line("1", "AAAAAAAAAAA"),
        segment_line("2", "CCCCCCCCCCC"),
        segment_line("3", "CCCCCCCCCCA"),
        link_line("1", "+", "2", "-", "10M"),
        link_line("2", "+", "3", "+", "10M"),
        path_line("1", "1.fa", to_csv("2+", "3+"), "10M"),
    }, 10);

    ASSERT_EQUAL(gfa_data->invalid_links().size(), 1);
    ASSERT_TRUE(gfa_data->invalid_links().contains(Link(0, 1, Orientation::PLUS, Orientation::MINUS)));

    return verify_gfa_data_after_first_pass(*gfa_data, 3, 1, 1);
}

bool test_integration_gfa_data_first_pass_adds_zero_overlap_to_invalid_links() {
    const auto gfa_data = make_gfa_data({
        segment_line("1", "AAA"),
        segment_line("2", "ACG"),
        segment_line("3", "CGT"),
        link_line("1", "+", "2", "+", "0M"),
        link_line("2", "+", "3", "+", "2M"),
        path_line("1", "1.fa", to_csv("2+", "3+"), "2M"),
    }, 2);

    ASSERT_EQUAL(gfa_data->invalid_links().size(), 1);
    ASSERT_TRUE(gfa_data->invalid_links().contains(Link(0, 1, Orientation::PLUS, Orientation::PLUS)));

    return verify_gfa_data_after_first_pass(*gfa_data, 3, 1, 1);
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_integration_gfa_data_first_pass_reads_minimal_valid_input),
        TEST(test_integration_gfa_data_first_pass_throws_on_no_segments),
        TEST(test_integration_gfa_data_first_pass_throws_on_no_paths),
        TEST(test_integration_gfa_data_first_pass_throws_on_duplicate_segment_name),
        TEST(test_integration_gfa_data_first_pass_throws_on_invalid_record_type),
        TEST(test_integration_gfa_data_first_pass_throws_on_wrong_segment_field_count),
        TEST(test_integration_gfa_data_first_pass_throws_on_wrong_path_field_count),
        TEST(test_integration_gfa_data_first_pass_throws_on_no_valid_links),
        TEST(test_integration_gfa_data_first_pass_ignores_unsupported_valid_record_types),
        TEST(test_integration_gfa_data_first_pass_allows_multiple_sequences_per_reference),
        TEST(test_integration_gfa_data_first_pass_reads_efc_data),
        TEST(test_integration_gfa_data_first_pass_reads_efc_data_multithreaded),
        TEST(test_integration_gfa_data_first_pass_reads_maela_data),
        TEST(test_integration_gfa_data_first_pass_reads_maela_data_multithreaded),
        TEST(test_integration_gfa_data_first_pass_maps_segments_deterministically),
        TEST(test_integration_gfa_data_first_pass_maps_segments_with_overlapping_ids),
        TEST(test_integration_gfa_data_first_pass_throws_on_link_with_unmapped_segment),
        TEST(test_integration_gfa_data_first_pass_ignores_path_segments_and_overlaps),
        TEST(test_integration_gfa_data_first_pass_supports_lowercase_bases),
        TEST(test_integration_gfa_data_first_pass_adds_invalid_overlap_to_invalid_links),
        TEST(test_integration_gfa_data_first_pass_adds_zero_overlap_to_invalid_links),
    };

    return Test::run_suite("test_integration_gfa_data", tests, test_setup, test_teardown);
}
