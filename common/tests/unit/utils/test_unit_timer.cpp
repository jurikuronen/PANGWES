/*
 * test_unit_timer.cpp - Unit tests for utils/Timer.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <cstdint>

#include "common/test_harness/Test.hpp"
#include "mocks/MockTimer.hpp"

namespace PANGWES {
namespace {

constexpr auto test_time_to_advance_ms = 5000;

// Advance the MockClock by `time_to_advance_ms` milliseconds and accumulate passed time in `mock_timer`.
void test_advance_and_accumulate(Mocks::MockTimer& mock_timer, std::uint64_t time_to_advance_ms) noexcept {
    Mocks::MockClock::advance(time_to_advance_ms);
    mock_timer.accumulate();
}

bool test_unit_timer_constructor() {
    Mocks::MockTimer mock_timer;

    ASSERT_EQUAL(mock_timer.total_ms(), 0);
    ASSERT_EQUAL(mock_timer.last_interval_ms(), 0);

    return true;
}

bool test_unit_timer_accumulate_adds_to_total_time() {
    Mocks::MockTimer mock_timer;

    test_advance_and_accumulate(mock_timer, test_time_to_advance_ms);

    ASSERT_EQUAL(mock_timer.total_ms(), test_time_to_advance_ms);

    return true;
}

bool test_unit_timer_accumulate_stores_last_interval_duration() {
    Mocks::MockTimer mock_timer;

    test_advance_and_accumulate(mock_timer, test_time_to_advance_ms);

    ASSERT_EQUAL(mock_timer.last_interval_ms(), test_time_to_advance_ms);

    return true;
}

bool test_unit_timer_accumulate_on_multiple_intervals() {
    Mocks::MockTimer mock_timer;

    for (auto i = 1; i < 1000; ++i) {
        test_advance_and_accumulate(mock_timer, test_time_to_advance_ms);

        // Total time should keep increasing and last interval duration should stay constant.
        ASSERT_EQUAL(mock_timer.total_ms(), i * test_time_to_advance_ms);
        ASSERT_EQUAL(mock_timer.last_interval_ms(), test_time_to_advance_ms);
    }

    return true;
}

bool test_unit_timer_total_ms_doesnt_accumulate_by_default() {
    Mocks::MockTimer mock_timer;

    Mocks::MockClock::advance(test_time_to_advance_ms);

    // Time passed, but stored durations should be zero before `accumulate()` or `accumulate_first = true` passed.
    ASSERT_EQUAL(mock_timer.total_ms(false), 0);

    return true;
}

bool test_unit_timer_total_ms_with_accumulate_flag() {
    Mocks::MockTimer mock_timer;

    Mocks::MockClock::advance(test_time_to_advance_ms);

    // Time accumulated by `total_ms()` since `accumulate_first = true` passed.
    ASSERT_EQUAL(mock_timer.total_ms(true), test_time_to_advance_ms);

    return true;
}

bool test_unit_timer_last_interval_ms_doesnt_accumulate_by_default() {
    Mocks::MockTimer mock_timer;

    Mocks::MockClock::advance(test_time_to_advance_ms);

    // Time passed, but stored durations should be zero before `accumulate()` or `accumulate_first = true` passed.
    ASSERT_EQUAL(mock_timer.last_interval_ms(false), 0);

    return true;
}

bool test_unit_timer_last_interval_ms_with_accumulate_flag() {
    Mocks::MockTimer mock_timer;

    Mocks::MockClock::advance(test_time_to_advance_ms);

    // Time accumulated by `last_interval_ms()` since `accumulate_first = true` passed.
    ASSERT_EQUAL(mock_timer.last_interval_ms(true), test_time_to_advance_ms);

    return true;
}

bool test_unit_timer_mark_sets_mark_to_current_time() {
    Mocks::MockTimer mock_timer;

    Mocks::MockClock::advance(test_time_to_advance_ms);

    // Calling `mark` should reset the timer's time point mark to the current time.
    mock_timer.mark();
    mock_timer.accumulate();

    ASSERT_EQUAL(mock_timer.total_ms(), 0);
    ASSERT_EQUAL(mock_timer.last_interval_ms(), 0);

    return true;
}

bool test_unit_timer_accumulate_resets_mark() {
    Mocks::MockTimer mock_timer;

    test_advance_and_accumulate(mock_timer, test_time_to_advance_ms);

    for (auto i = 0; i < 1000; ++i) {
        mock_timer.accumulate();

        /*
         * Calling accumulate() should have reset the mark, so no additional time should get accumulated to the
         * total and the last interval should have a duration of zero.
        */
        ASSERT_EQUAL(mock_timer.total_ms(), test_time_to_advance_ms);
        ASSERT_EQUAL(mock_timer.last_interval_ms(), 0);
    }

    return true;
}

bool test_unit_timer_unix_time() {
    constexpr auto start_time_ms = 123456;

    Mocks::MockClock::set_time(start_time_ms);
    ASSERT_EQUAL(start_time_ms, Mocks::MockTimer::unix_time_ms());

    Mocks::MockTimer mock_timer;

    for (auto i = 0; i < 1000; ++i) {
        Mocks::MockClock::advance(test_time_to_advance_ms);
        mock_timer.accumulate();

        ASSERT_EQUAL(start_time_ms + mock_timer.total_ms(), mock_timer.unix_time_ms());
    }

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_unit_timer_constructor),
        TEST(test_unit_timer_accumulate_adds_to_total_time),
        TEST(test_unit_timer_accumulate_stores_last_interval_duration),
        TEST(test_unit_timer_accumulate_on_multiple_intervals),
        TEST(test_unit_timer_total_ms_doesnt_accumulate_by_default),
        TEST(test_unit_timer_total_ms_with_accumulate_flag),
        TEST(test_unit_timer_last_interval_ms_doesnt_accumulate_by_default),
        TEST(test_unit_timer_last_interval_ms_with_accumulate_flag),
        TEST(test_unit_timer_mark_sets_mark_to_current_time),
        TEST(test_unit_timer_accumulate_resets_mark),
        TEST(test_unit_timer_unix_time),
    };

    return Test::run_suite("test_unit_timer", tests);
}
