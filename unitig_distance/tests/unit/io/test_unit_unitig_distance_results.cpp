/*
 * test_unit_unitig_distance_results.cpp - Unit tests for io/UnitigDistanceResults.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <memory>
#include <string>
#include <vector>

#include "common/io/FileWriterInterface.hpp"
#include "common/test_harness/Test.hpp"
#include "common/utils/Exception.hpp"
#include "common/utils/memory.hpp"
#include "mocks/MockFileReader.hpp"
#include "mocks/MockFileWriter.hpp"
#include "unitig_distance/core/Distance.hpp"
#include "unitig_distance/io/QueriesReader.hpp"
#include "unitig_distance/io/UnitigDistanceResults.hpp"

namespace PANGWES {
namespace {

bool test_unit_unitig_distance_results_query_count_mismatch_throws() {
    Mocks::MockIfStream::set_contents({"0 1 0 0 0"});

    auto queries_reader = QueriesReader(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file));
    std::unique_ptr<FileWriterInterface> writer = Memory::make_unique<Mocks::MockFileWriter>(Mocks::mock_file);
    const std::vector<Distance> distances(2);

    EXPECT_THROW(UnitigDistanceResults::write_results(writer, queries_reader, distances, false),
                 ErrorCode::QUERY_COUNT_MISMATCH);

    return true;
}

bool test_unit_unitig_distance_results_outputs_median_distance_when_set() {
    Mocks::MockIfStream::set_contents({"0 1 0 1 0.5"});

    auto queries_reader = QueriesReader(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file));
    std::unique_ptr<FileWriterInterface> writer = Memory::make_unique<Mocks::MockFileWriter>(Mocks::mock_file);

    Distance distance;
    distance.add_distance(10);
    distance.add_distance(20);
    distance.set_median_distance(10);

    UnitigDistanceResults::write_results(writer, queries_reader, {distance}, false);

    const auto expected_contents = std::string{"0 1 15 1 0.5 2 50.00000 10 20 10\n"};
    ASSERT_EQUAL(Mocks::MockOfStream::contents(), expected_contents);

    return true;
}

bool test_unit_unitig_distance_results_outputs_undefined_median_distance_when_not_set() {
    Mocks::MockIfStream::set_contents({"0 1 0 1 0.5"});

    auto queries_reader = QueriesReader(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file));
    std::unique_ptr<FileWriterInterface> writer = Memory::make_unique<Mocks::MockFileWriter>(Mocks::mock_file);

    UnitigDistanceResults::write_results(writer, queries_reader, {Distance{}}, false);

    const auto expected_contents = std::string{"0 1 -1 1 0.5 0 -1 -1 -1 -1\n"};
    ASSERT_EQUAL(Mocks::MockOfStream::contents(), expected_contents);

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_unit_unitig_distance_results_query_count_mismatch_throws),
        TEST(test_unit_unitig_distance_results_outputs_median_distance_when_set),
        TEST(test_unit_unitig_distance_results_outputs_undefined_median_distance_when_not_set),
    };

    return Test::run_suite("test_unit_unitig_distance_results", tests);
}
