/*
 * test_unit_log.cpp - Unit tests for io/Log.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <string>

#include "common/test_harness/Test.hpp"
#include "mocks/MockLog.hpp"

namespace PANGWES {
namespace {

using Mocks::MockLog;
using Mocks::MockOStream;

std::string get_mock_stream_contents() {
    auto contents = MockOStream::contents();

    // Normalize by removing any '\r' characters.
    contents.erase(std::remove(contents.begin(), contents.end(), '\r'), contents.end());

    return contents;
}

bool test_setup() {
    // Enable logging.
    MockLog::set_verbose(true);

    // Clean slate.
    MockOStream::clear();

    return true;
}

bool test_unit_log_out_without_date_block() {
    constexpr auto expected_contents = "test\n";

    MockLog::out_without_date_block() << "test" << std::endl;

    const auto contents = get_mock_stream_contents();
    ASSERT_FALSE(contents.empty());
    ASSERT_EQUAL(contents, expected_contents);

    return true;
}

bool test_unit_log_out_without_date_block_various_types() {
    constexpr auto expected_contents = "test13.11a\n";

    MockLog::out_without_date_block() << "test" << 1 << 3.1 << true << 'a' << std::endl;

    const auto contents = get_mock_stream_contents();
    ASSERT_FALSE(contents.empty());
    ASSERT_EQUAL(contents, expected_contents);

    return true;
}

bool test_unit_log_out() {
    // Set mock clock to 2000 Jan 01 - 00:00:00, normalizing for the system's time zone.
    Mocks::MockClock::set_utc_time(946684800000LL);

    const auto test_advance_ms = std::vector<std::uint64_t>{
        1050,
        1000 * 60 * 5 + 1000 * 5,
        1000 * 60 * 60 * 2 + 1000 * 60 * 10
    };
    const auto expected_date_block_strings_after_advance = std::vector<std::string>
    {
        "[2000 Jan 01 - 00:00:01]",
        "[2000 Jan 01 - 00:05:06]",
        "[2000 Jan 01 - 02:15:06]"
    };
    constexpr auto expected_contents_text = "test";

    for (std::size_t i = 0; i < test_advance_ms.size(); ++i) {
        Mocks::MockTimer mock_timer;
        Mocks::MockClock::advance(test_advance_ms[i]);

        MockOStream::clear();
        MockLog::out() << expected_contents_text << std::endl;

        const auto contents = get_mock_stream_contents();
        const auto expected_contents = expected_date_block_strings_after_advance[i] +
                                       " " + expected_contents_text + '\n';

        ASSERT_EQUAL(contents, expected_contents);
    }

    return true;
}

bool test_unit_log_out_without_date_block_io_manipulators() {
    constexpr auto expected_contents = "ff\n";

    MockLog::out_without_date_block() << std::hex << 255 << std::endl;

    const auto contents = get_mock_stream_contents();
    ASSERT_EQUAL(contents, expected_contents);

    return true;
}

bool test_unit_log_out_without_date_block_io_manipulators_chaining() {
    constexpr auto expected_contents = "0001\n";

    MockLog::out_without_date_block() << std::setw(4) << std::setfill('0') << 1 << std::endl;

    const auto contents = get_mock_stream_contents();
    ASSERT_EQUAL(contents, expected_contents);

    return true;
}

bool test_unit_log_out_without_date_block_only_std_endl_flushes() {
    // need to normalize linebreaks...
    constexpr auto expected_contents = "test\n\n";

    MockLog::out_without_date_block() << "test";
    ASSERT_TRUE(MockOStream::contents().empty());

    MockLog::out_without_date_block() << std::flush;
    ASSERT_TRUE(MockOStream::contents().empty());

    MockLog::out_without_date_block() << '\n';
    ASSERT_TRUE(MockOStream::contents().empty());

    MockLog::out_without_date_block() << std::endl;
    const auto contents = get_mock_stream_contents();
    ASSERT_FALSE(contents.empty());
    ASSERT_EQUAL(contents, expected_contents);

    return true;
}

bool test_unit_log_out_without_date_block_empty_flush() {
    constexpr auto expected_contents = "\n";

    MockLog::out_without_date_block() << std::endl;
    const auto contents = get_mock_stream_contents();
    ASSERT_FALSE(contents.empty());
    ASSERT_EQUAL(contents, expected_contents);

    return true;
}

bool test_unit_log_out_without_date_block_multiple_entries_appended() {
    constexpr auto expected_contents = "test1\ntest2\ntest3\n";

    MockLog::out_without_date_block() << "test1" << std::endl;
    MockLog::out_without_date_block() << "test2" << std::endl;
    MockLog::out_without_date_block() << "test3" << std::endl;

    const auto contents = get_mock_stream_contents();
    ASSERT_FALSE(contents.empty());
    ASSERT_EQUAL(contents, expected_contents);

    return true;
}

bool test_unit_log_out_without_date_block_multiple_std_endl() {
    constexpr auto expected_contents = "test\n\n\n";

    MockLog::out_without_date_block() << "test" << std::endl << std::endl << std::endl;

    const auto contents = get_mock_stream_contents();
    ASSERT_FALSE(contents.empty());
    ASSERT_EQUAL(contents, expected_contents);

    return true;
}

bool test_unit_log_verbose_false_disables_logging() {
    MockLog::set_verbose(false);
    MockLog::out_without_date_block() << "test" << std::endl;

    ASSERT_TRUE(MockOStream::contents().empty());

    MockLog::set_verbose(true);

    ASSERT_TRUE(MockOStream::contents().empty());

    return true;
}

bool test_unit_log_verbose_changed_midstream() {
    MockLog::out_without_date_block() << "test";
    MockLog::set_verbose(false);
    MockLog::out_without_date_block() << std::endl;

    ASSERT_TRUE(MockOStream::contents().empty());

    /*
     * Technically, "test" remains in the buffer now. However, this program considers setting verbose to false during
     * ongoing logging as unrecommended behavior and we allow the thread-local buffers to be left in a corrupted state.
    */

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_unit_log_out_without_date_block),
        TEST(test_unit_log_out_without_date_block_various_types),
        TEST(test_unit_log_out),
        TEST(test_unit_log_out_without_date_block_io_manipulators),
        TEST(test_unit_log_out_without_date_block_io_manipulators_chaining),
        TEST(test_unit_log_out_without_date_block_only_std_endl_flushes),
        TEST(test_unit_log_out_without_date_block_empty_flush),
        TEST(test_unit_log_out_without_date_block_multiple_entries_appended),
        TEST(test_unit_log_out_without_date_block_multiple_std_endl),
        TEST(test_unit_log_verbose_false_disables_logging),
        TEST(test_unit_log_verbose_changed_midstream),
    };

    return Test::run_suite("test_unit_log", tests, test_setup);
}
