/*
 * test_unit_format.cpp - Unit tests for text formatting-related utility functions defined in utils/format.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "common/test_harness/Test.hpp"
#include "common/utils/format.hpp"
#include "mocks/MockTimer.hpp"

namespace PANGWES {
namespace {

struct TimeEntry {
    int64_t unit;
    int64_t ms;
};

// Helper function to create the time data.
std::vector<TimeEntry> test_make_time_data(const std::vector<int64_t>& time_units,
                                           const std::function<int64_t(int64_t)>& to_ms_func)
{
    std::vector<TimeEntry> time_data;
    time_data.reserve(time_units.size());

    for (const auto time_unit : time_units) {
        time_data.push_back({ time_unit, to_ms_func(time_unit) });
    }

    return time_data;
}

// Time data constants to loop over in time-related tests.
const auto time_data_days = test_make_time_data({1, 2, 4, 8, 16, 32},
                                                [](int64_t days){ return days * 24 * 60 * 60 * 1000; });
const auto time_data_hours = test_make_time_data({1, 2, 4, 8, 16, 23},
                                                 [](int64_t hours){ return hours * 60 * 60 * 1000; });
const auto time_data_minutes = test_make_time_data({1, 2, 4, 8, 16, 32, 59},
                                                   [](int64_t minutes){ return minutes * 60 * 1000; });
const auto time_data_seconds = test_make_time_data({1, 2, 4, 8, 16, 32, 59},
                                                   [](int64_t seconds){ return seconds * 1000; });
const auto time_data_milliseconds = test_make_time_data({1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 999},
                                                        [](int64_t milliseconds){ return milliseconds; });

bool test_unit_format_pretty_uint() {
    const std::vector<std::pair<int64_t, std::string>> test_data{
        {0LL, "0"}, {1LL, "1"}, {12LL, "12"}, {123LL, "123"}, {1234LL, "1 234"},
        {12345LL, "12 345"}, {123456LL, "123 456"}, {1234567LL, "1 234 567"},
        {12345678LL, "12 345 678"}, {123456789LL, "123 456 789"},
        {12345678910LL, "12 345 678 910"},
    };

    for (const auto& data : test_data) {
        const auto number = data.first;
        const auto& expected_pretty_str = data.second;

        ASSERT_EQUAL(expected_pretty_str, Format::pretty_uint(number));
    }

    return true;
}

bool test_unit_format_duration_to_string_zero() {
    ASSERT_EQUAL(Format::duration_to_string(0), "0.000s");

    return true;
}

// Check that the duration string is correct and only affected by the higher and lower unit.
bool check_unit_utils_duration_format(const std::vector<TimeEntry>& higher_units,
                                      const std::vector<TimeEntry>& lower_units,
                                      const std::vector<TimeEntry>& unaffected_units,
                                      const std::function<std::string(TimeEntry, TimeEntry)>& format_expected)
{
    for (const auto higher_unit : higher_units) {
        for (const auto lower_unit : lower_units) {
            for (const auto unaffected_unit : unaffected_units) {
                const auto duration_ms = higher_unit.ms + lower_unit.ms + unaffected_unit.ms;
                const auto expected_duration_string = format_expected(higher_unit, lower_unit);
                const auto duration_string = Format::duration_to_string(duration_ms);

                ASSERT_EQUAL(duration_string, expected_duration_string);
            }
        }
    }

    return true;
}

bool test_unit_format_duration_to_string_days() {
    const auto format_expected = [](TimeEntry days, TimeEntry hours) {
        (void)hours;

        return std::to_string(days.unit) + " day" + (days.unit > 1 ? "s" : "");
    };

    return check_unit_utils_duration_format(time_data_days, time_data_hours, {}, format_expected);
}

bool test_unit_format_duration_to_string_hours() {
    const auto format_expected = [](TimeEntry hours, TimeEntry minutes) {
        return std::to_string(hours.unit) + "h " + std::to_string(minutes.unit) + "m";
    };

    return check_unit_utils_duration_format(time_data_hours, time_data_minutes, time_data_seconds, format_expected);
}

bool test_unit_format_duration_to_string_minutes() {
    const auto format_expected = [](TimeEntry minutes, TimeEntry seconds) {
        return std::to_string(minutes.unit) + "m " + std::to_string(seconds.unit) + "s";
    };

    return check_unit_utils_duration_format(time_data_minutes, time_data_seconds, time_data_milliseconds,
                                            format_expected);
}

bool test_unit_format_duration_to_string_seconds() {
    const auto format_expected = [](TimeEntry seconds, TimeEntry milliseconds) {
        return std::to_string(seconds.unit) + "." + (milliseconds.unit < 10 ? "0" : "") +
               (milliseconds.unit < 100 ? "0" : "") + std::to_string(milliseconds.unit) + "s";
    };

    return check_unit_utils_duration_format(time_data_seconds, time_data_milliseconds, {}, format_expected);
}

bool test_unit_format_date_block() {
    const auto check_date_block = [](const std::string& expected_date_block) {
        const auto date_block = Format::date_block(Mocks::MockTimer::unix_time_ms());

        // Not affected by passed milliseconds.
        for (auto ms = 1; ms <= 999; ++ms) {
            Mocks::MockClock::advance(1);
            ASSERT_EQUAL(date_block, expected_date_block);
        }

        // Rewind the mocked time by the passed milliseconds so next test steps are not affected.
        Mocks::MockClock::rewind(999);

        return true;
    };

    // Set mock clock to 2000 Jan 01 - 00:00:00, normalizing for the system's time zone.
    Mocks::MockClock::set_utc_time(946684800000LL);
    if (!check_date_block("[2000 Jan 01 - 00:00:00]")) {
        return false;
    }

    // Try various {hour, minute} combos.
    for (const auto hour : time_data_hours) {
        Mocks::MockClock::advance(hour.ms);
        for (const auto minute : time_data_minutes) {
            const auto expected_date_block = std::string{"[2000 Jan 01 - "} +
                                             (hour.unit < 10 ? "0" : "") + std::to_string(hour.unit) + ":" +
                                             (minute.unit < 10 ? "0" : "") + std::to_string(minute.unit) + ":00]";

            Mocks::MockClock::advance(minute.ms);

            if (!check_date_block(expected_date_block)) {
                return false;
            }

            Mocks::MockClock::rewind(minute.ms);
        }
        Mocks::MockClock::rewind(hour.ms);
    }

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_unit_format_pretty_uint),
        TEST(test_unit_format_duration_to_string_zero),
        TEST(test_unit_format_duration_to_string_days),
        TEST(test_unit_format_duration_to_string_hours),
        TEST(test_unit_format_duration_to_string_minutes),
        TEST(test_unit_format_duration_to_string_seconds),
        TEST(test_unit_format_date_block),
    };

    return Test::run_suite("test_unit_format", tests);
}
