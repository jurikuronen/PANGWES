/*
 * test_unit_queries_reader.cpp - Unit tests for io/QueriesReader.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <cstddef>
#include <limits>
#include <string>
#include <vector>

#include "common/io/Log.hpp"
#include "common/test_harness/Test.hpp"
#include "common/utils/Exception.hpp"
#include "common/utils/memory.hpp"
#include "mocks/MockFileReader.hpp"
#include "mocks/MockQueriesData.hpp"
#include "unitig_distance/io/QueriesReader.hpp"

namespace PANGWES {
namespace {

// Copied here from io/QueriesReader.hpp.
constexpr auto N_REQUIRED_SPYDRPICK_FIELDS = 5;

constexpr auto SIZE_T_MAX = std::numeric_limits<std::size_t>::max();

// QueriesReader ignores SpydrPick output's third field (genome_distance) as that will be replaced by graph distance.
const auto mock_queries_data = Mocks::MockQueriesData(std::vector<Mocks::MockQueryData>{
    {358501, 358580, "0 0 0.647011"},
    {358218, 358219, "1 1 0.646989"},
    {49148, 49151, "2 1 0.646987"},
    {322746, 322823, "3 1 0.646984"},
    {43025, 43026, "4 1 0.646973"},
    {418242, 418243, "5 1 0.646972"},
    {442283, 442284, "6 1 0.646969"},
    {289013, 289014, "7 1 0.646968"},
    {99515, 99516, "8 1 0.646968"},
    {54744, 54745, "9 1 0.646960"},
});

// Verifies that getquery() works as expected on the above `mock_queries_data`.
bool check_getquery_on_mock_queries_data(std::size_t n_queries = SIZE_T_MAX, bool one_based = false) {
    // Set data for MockFileReader's stream and reset's is_open()'s mocked state.
    Mocks::MockIfStream::set_contents(mock_queries_data.to_contents());

    auto queries_reader = QueriesReader(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file),
                                        n_queries,
                                        one_based);

    ASSERT_EQUAL(queries_reader.n_queries_read(), 0);

    // MockFileReader doesn't require opening/closing files to test getquery() (via getline()).
    for (std::size_t i = 0; i < mock_queries_data.size() && i < n_queries; ++i) {
        QueriesReader::QueryData query_data;

        // getquery() reads until a valid query line as long as getline() succeeds.
        ASSERT_TRUE(queries_reader.getquery(query_data));

        const auto expected_unitig1_id = mock_queries_data.unitig1_id(i) - static_cast<std::size_t>(one_based);
        const auto expected_unitig2_id = mock_queries_data.unitig2_id(i) - static_cast<std::size_t>(one_based);
        const auto expected_aracne_flag_and_mi_score_fields = mock_queries_data.aracne_flag_and_mi_score_fields(i);

        ASSERT_EQUAL(query_data.unitig1_id, expected_unitig1_id);
        ASSERT_EQUAL(query_data.unitig2_id, expected_unitig2_id);
        ASSERT_EQUAL(query_data.aracne_flag_and_mi_score_fields, expected_aracne_flag_and_mi_score_fields);

        ASSERT_EQUAL(queries_reader.n_queries_read(), i + 1);
    }

    // Special logic for n_queries tests.
    if (n_queries < mock_queries_data.size()) {
        QueriesReader::QueryData query_data;

        // Trying to read the next query should fail.
        ASSERT_FALSE(queries_reader.getquery(query_data));

        // n_queries_read should not get updated.
        ASSERT_EQUAL(queries_reader.n_queries_read(), n_queries);
    }

    return true;
}

bool test_unit_queries_reader_getquery() {
    return check_getquery_on_mock_queries_data();
}


bool test_unit_queries_reader_getquery_one_based() {
    return check_getquery_on_mock_queries_data(SIZE_T_MAX, true);
}

bool test_unit_queries_reader_getquery_one_based_zero_unitig_id_throws() {
    QueriesReader::QueryData query_data;

    // Unitig 1 is zero.
    Mocks::MockIfStream::set_contents({"0 2 0 0 0"});
    auto reader_first = QueriesReader(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file), SIZE_T_MAX, true);
    EXPECT_THROW(reader_first.getquery(query_data), ErrorCode::INVALID_UNITIG_ID);

    // Unitig 2 is zero.
    Mocks::MockIfStream::set_contents({"1 0 0 0 0"});
    auto reader_second = QueriesReader(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file), SIZE_T_MAX, true);
    EXPECT_THROW(reader_second.getquery(query_data), ErrorCode::INVALID_UNITIG_ID);

    return true;
}

bool test_unit_queries_reader_getquery_with_empty_lines() {
    auto contents = mock_queries_data.to_contents();

    // Insert some empty lines.
    contents.insert(contents.begin(), "");
    contents.insert(contents.begin() + 3, "");

    Mocks::MockIfStream::set_contents(contents);

    return check_getquery_on_mock_queries_data();
}

bool test_unit_queries_reader_getquery_until_n_queries() {
    return check_getquery_on_mock_queries_data(mock_queries_data.size() / 2);
}

bool test_unit_queries_reader_getquery_until_n_queries_with_empty_lines() {
    auto contents = mock_queries_data.to_contents();

    // Insert some empty lines.
    contents.insert(contents.begin(), "");
    contents.insert(contents.begin() + 3, "");

    Mocks::MockIfStream::set_contents(contents);

    return check_getquery_on_mock_queries_data(mock_queries_data.size() / 2);
}

bool test_unit_queries_reader_getquery_wrong_column_count_throws() {
    std::string invalid_row{};

    for (auto n_columns = 1; n_columns < N_REQUIRED_SPYDRPICK_FIELDS; ++n_columns) {
        invalid_row += "0 ";

        QueriesReader::QueryData query_data;

        Mocks::MockIfStream::set_contents({invalid_row});
        auto queries_reader = QueriesReader(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file));
        EXPECT_THROW(queries_reader.getquery(query_data), ErrorCode::FILE_WRONG_COLUMN_COUNT);
    }

    return true;
}

bool test_unit_queries_reader_getquery_ignores_third_field() {
    QueriesReader::QueryData query_data;

    // Queries expect numeric data. This sets invalid data into the third field that is expected to be ignored.
    Mocks::MockIfStream::set_contents({"1 2 a 1 0.5"});

    auto reader_first = QueriesReader(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file));
    reader_first.getquery(query_data);

    const auto expected_unitig1_id = 1;
    const auto expected_unitig2_id = 2;
    const auto* const expected_aracne_flag_and_mi_score_fields = "1 0.5";

    ASSERT_EQUAL(query_data.unitig1_id, expected_unitig1_id);
    ASSERT_EQUAL(query_data.unitig2_id, expected_unitig2_id);
    ASSERT_EQUAL(query_data.aracne_flag_and_mi_score_fields, expected_aracne_flag_and_mi_score_fields);

    return true;
}

bool test_unit_queries_reader_getquery_ignores_fields_above_required() {
    std::string extra_fields;

    for (auto n_extra_fields = 1; n_extra_fields < 100; ++n_extra_fields) {
        extra_fields += " 0";

        std::vector<std::string> mock_contents;

        for (std::size_t idx = 0; idx < mock_queries_data.size(); ++idx) {
            mock_contents.push_back(mock_queries_data.to_string(idx) + extra_fields);
        }

        Mocks::MockIfStream::set_contents(mock_contents);

        if (!check_getquery_on_mock_queries_data()) {
            return false;
        }
    }

    return true;
}

// Queries expect numeric data.
bool test_unit_queries_reader_getquery_invalid_unitig_id_throws() {
    QueriesReader::QueryData query_data;

    // Unitig 1 is non-numeric.
    Mocks::MockIfStream::set_contents({"a 2 0 0 0"});
    auto reader_first = QueriesReader(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file), SIZE_T_MAX, true);
    EXPECT_THROW(reader_first.getquery(query_data), ErrorCode::FILE_BAD_DATA);

    // Unitig 2 is non-numeric.
    Mocks::MockIfStream::set_contents({"1 b 0 0 0"});
    auto reader_second = QueriesReader(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file), SIZE_T_MAX, true);
    EXPECT_THROW(reader_second.getquery(query_data), ErrorCode::FILE_BAD_DATA);

    return true;
}

bool test_unit_queries_reader_getquery_duplicate_unitig_ids_throws() {
    QueriesReader::QueryData query_data;

    Mocks::MockIfStream::set_contents({"1 1 0 0 0"});
    auto queries_reader = QueriesReader(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file));

    EXPECT_THROW(queries_reader.getquery(query_data), ErrorCode::FILE_DUPLICATE_DATA);

    return true;
}

bool test_unit_queries_reader_start_reading_twice_throws() {
    Mocks::MockIfStream::set_contents(mock_queries_data.to_contents());

    // Set to return false for open()'s initial check and true after called the stream's open().
    Mocks::MockIfStream::set_is_open_return({true, false});

    auto queries_reader = QueriesReader(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file));
    queries_reader.start_reading();

    // Now set to return true for FileReader::open()'s initial is_open() check to trigger the exception.
    Mocks::MockIfStream::set_is_open_return({true});
    EXPECT_THROW(queries_reader.start_reading(), ErrorCode::FAILED_TO_OPEN_FILE);

    return true;
}

bool test_unit_queries_reader_start_reading_resets_queries_read() {
    QueriesReader::QueryData query_data;

    Mocks::MockIfStream::set_contents(mock_queries_data.to_contents());

    auto queries_reader = QueriesReader(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file));

    // MockFileReader doesn't require the initial opening of the file; can directly call getquery().
    ASSERT_TRUE(queries_reader.getquery(query_data));
    ASSERT_EQUAL(queries_reader.n_queries_read(), 1);

    queries_reader.start_reading();
    ASSERT_EQUAL(queries_reader.n_queries_read(), 0);

    // MockFileReader::open() doesn't affect the MockIfStream's state.
    ASSERT_TRUE(queries_reader.getquery(query_data));
    ASSERT_EQUAL(queries_reader.n_queries_read(), 1);

    return true;
}

bool test_unit_queries_reader_stop_reading_without_start_reading_throws() {
    Mocks::MockIfStream::set_is_open_return({false});

    auto queries_reader = QueriesReader(Memory::make_unique<Mocks::MockFileReader>(Mocks::mock_file));

    EXPECT_THROW(queries_reader.stop_reading(), ErrorCode::FAILED_TO_CLOSE_FILE);

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_unit_queries_reader_getquery),
        TEST(test_unit_queries_reader_getquery_one_based),
        TEST(test_unit_queries_reader_getquery_one_based_zero_unitig_id_throws),
        TEST(test_unit_queries_reader_getquery_with_empty_lines),
        TEST(test_unit_queries_reader_getquery_until_n_queries),
        TEST(test_unit_queries_reader_getquery_until_n_queries_with_empty_lines),
        TEST(test_unit_queries_reader_getquery_wrong_column_count_throws),
        TEST(test_unit_queries_reader_getquery_ignores_third_field),
        TEST(test_unit_queries_reader_getquery_ignores_fields_above_required),
        TEST(test_unit_queries_reader_getquery_invalid_unitig_id_throws),
        TEST(test_unit_queries_reader_getquery_duplicate_unitig_ids_throws),
        TEST(test_unit_queries_reader_start_reading_twice_throws),
        TEST(test_unit_queries_reader_start_reading_resets_queries_read),
        TEST(test_unit_queries_reader_stop_reading_without_start_reading_throws),
    };

    return Test::run_suite("test_unit_queries_reader", tests);
}
