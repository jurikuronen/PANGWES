/*
 * test_unit_progress_logger.cpp - Unit tests for utils/ProgressLogger.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <cassert>
#include <cstddef>
#include <cstdint>

#include "common/test_harness/Test.hpp"
#include "common/utils/ProgressLogger.hpp"
#include "mocks/MockTimer.hpp"

namespace PANGWES {
namespace {

using MockProgressLogger = ProgressLoggerImplementation<Mocks::MockTimer>;

constexpr std::uint64_t test_log_value_interval = 5;
constexpr std::uint64_t test_log_time_interval_ms = 10000;

std::size_t log_count{};

void progress_logger_callback(std::uint64_t value, std::uint64_t elapsed_time_ms) {
    (void)value;
    (void)elapsed_time_ms;

    ++log_count;
}

bool test_setup() {
    Mocks::MockClock::set_time(0);
    log_count = 0;

    return true;
}

bool test_unit_progress_logger_log() {
    MockProgressLogger progress_logger(progress_logger_callback, test_log_value_interval, test_log_time_interval_ms);

    // log() logs progress always.
    for (std::size_t iteration = 1; iteration <= 10; ++iteration) {
        progress_logger.log(iteration);

        ASSERT_EQUAL(log_count, iteration);
    }

    return true;
}

bool test_unit_progress_logger_log_doesnt_log_same_value_twice() {
    MockProgressLogger progress_logger(progress_logger_callback, test_log_value_interval, test_log_time_interval_ms);

    for (std::size_t iteration = 1; iteration <= 10; ++iteration) {
        // log(0) must also be logged, so this verifies that the previous logged value is not initialized at 0.
        progress_logger.log(0);

        // Value 0 will only be logged once.
        ASSERT_EQUAL(log_count, 1);
    }

    return true;
}

bool test_unit_progress_logger_log_logs_a_smaller_value_than_previous() {
    MockProgressLogger progress_logger(progress_logger_callback, test_log_value_interval, test_log_time_interval_ms);

    for (std::size_t iteration = 1; iteration <= 10; ++iteration) {
        assert(iteration <= 10 && "unsigned integer overflow");

        progress_logger.log(10 - iteration);

        ASSERT_EQUAL(log_count, iteration);
    }

    return true;
}

bool test_unit_progress_logger_log_if_due_no_thresholds_reached() {
    MockProgressLogger progress_logger(progress_logger_callback, test_log_value_interval, test_log_time_interval_ms);

    progress_logger.log_if_due(0);

    ASSERT_EQUAL(log_count, 0);

    return true;
}

bool test_unit_progress_logger_log_if_due_only_value_threshold_reached() {
    MockProgressLogger progress_logger(progress_logger_callback, test_log_value_interval, test_log_time_interval_ms);

    progress_logger.log_if_due(test_log_value_interval);
    ASSERT_EQUAL(log_count, 0);

    // Advance the time to just under the time threshold.
    Mocks::MockClock::advance(test_log_time_interval_ms - 1);
    progress_logger.log_if_due(test_log_value_interval);
    ASSERT_EQUAL(log_count, 0);

    return true;
}

bool test_unit_progress_logger_log_if_due_only_time_threshold_reached() {
    MockProgressLogger progress_logger(progress_logger_callback, test_log_value_interval, test_log_time_interval_ms);

    Mocks::MockClock::advance(test_log_time_interval_ms);
    progress_logger.log_if_due(0);
    ASSERT_EQUAL(log_count, 0);

    // Check value just under the value threshold.
    progress_logger.log_if_due(test_log_value_interval - 1);
    ASSERT_EQUAL(log_count, 0);

    return true;
}

bool test_unit_progress_logger_log_if_due_both_thresholds_reached() {
    MockProgressLogger progress_logger(progress_logger_callback, test_log_value_interval, test_log_time_interval_ms);

    // Both thresholds almost reached: no logging yet.
    Mocks::MockClock::advance(test_log_time_interval_ms - 1);
    progress_logger.log_if_due(test_log_value_interval - 1);
    ASSERT_EQUAL(log_count, 0);

    // Both thresholds reached: should log.
    Mocks::MockClock::advance(1);
    progress_logger.log_if_due(test_log_value_interval);
    ASSERT_EQUAL(log_count, 1);

    return true;
}

bool test_unit_progress_logger_log_if_due_advances_thresholds() {
    MockProgressLogger progress_logger(progress_logger_callback, test_log_value_interval, test_log_time_interval_ms);

    Mocks::MockClock::advance(test_log_time_interval_ms);
    progress_logger.log_if_due(test_log_value_interval);

    // Verify that calling log_if_due() with the same value (and time) immediately doesn't log.
    progress_logger.log_if_due(test_log_value_interval);
    ASSERT_EQUAL(log_count, 1);

    // Verify that the earlier successful log_if_due() call advanced the thresholds as intended.
    Mocks::MockClock::advance(test_log_time_interval_ms - 1);
    progress_logger.log_if_due(test_log_value_interval + (test_log_value_interval - 1));
    ASSERT_EQUAL(log_count, 1);

    return true;
}

bool test_unit_progress_logger_log_doesnt_advance_thresholds() {
    MockProgressLogger progress_logger(progress_logger_callback, test_log_value_interval, test_log_time_interval_ms);

    // Log one time less than `test_log_value_interval` to not set previous logged value to the threshold.
    constexpr auto log_iterations = test_log_value_interval - 1;

    for (std::size_t iteration = 0; iteration < log_iterations; ++iteration) {
        progress_logger.log(iteration);
    }
    ASSERT_EQUAL(log_count, log_iterations);

    // No logging expected just below the thresholds.
    Mocks::MockClock::advance(test_log_time_interval_ms - 1);
    progress_logger.log_if_due(test_log_value_interval - 1);
    ASSERT_EQUAL(log_count, log_iterations);

    // Logging expected at the exact thresholds, proof that log() didn't modify the thresholds.
    Mocks::MockClock::advance(1);
    progress_logger.log_if_due(test_log_value_interval);
    ASSERT_EQUAL(log_count, log_iterations + 1);

    return true;
}

bool test_unit_progress_logger_log_if_due_doesnt_log_same_value_twice() {
    MockProgressLogger progress_logger(progress_logger_callback, test_log_value_interval, test_log_time_interval_ms);

    progress_logger.log(test_log_value_interval);
    ASSERT_EQUAL(log_count, 1);

    // Both thresholds reached, but log() already logged this value.
    Mocks::MockClock::advance(test_log_time_interval_ms);
    progress_logger.log_if_due(test_log_value_interval);
    ASSERT_EQUAL(log_count, 1);

    return true;
}

bool test_unit_progress_logger_log_if_due_zero_value_interval() {
    MockProgressLogger progress_logger(progress_logger_callback, 0, test_log_time_interval_ms);

    for (std::size_t iteration = 0; iteration < 10; ++iteration) {
        // Should be only dependent on time.
        Mocks::MockClock::advance(test_log_time_interval_ms);
        progress_logger.log_if_due(2 * iteration);
        ASSERT_EQUAL(log_count, 2 * iteration + 1);

        // Previous logged value should still prevent logging the same value.
        Mocks::MockClock::advance(test_log_time_interval_ms);
        progress_logger.log_if_due(2 * iteration);
        ASSERT_EQUAL(log_count, 2 * iteration + 1);

        // Another value should work.
        progress_logger.log_if_due(2 * iteration + 1);
        ASSERT_EQUAL(log_count, 2 * iteration + 2);
    }

    return true;
}

bool test_unit_progress_logger_log_if_due_zero_time_interval() {
    MockProgressLogger progress_logger(progress_logger_callback, test_log_value_interval, 0);

    for (std::size_t iteration = 1; iteration <= 10; ++iteration) {
        // Should be only dependent on value.
        progress_logger.log_if_due(iteration * test_log_value_interval);
        ASSERT_EQUAL(log_count, iteration);
    }

    return true;
}

bool test_unit_progress_logger_log_if_due_both_intervals_zero() {
    MockProgressLogger progress_logger(progress_logger_callback, 0, 0);

    // Should now log always.
    for (std::size_t iteration = 1; iteration <= 10; ++iteration) {
        progress_logger.log_if_due(iteration);

        ASSERT_EQUAL(log_count, iteration);
    }

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_unit_progress_logger_log),
        TEST(test_unit_progress_logger_log_doesnt_log_same_value_twice),
        TEST(test_unit_progress_logger_log_logs_a_smaller_value_than_previous),
        TEST(test_unit_progress_logger_log_if_due_no_thresholds_reached),
        TEST(test_unit_progress_logger_log_if_due_only_value_threshold_reached),
        TEST(test_unit_progress_logger_log_if_due_only_time_threshold_reached),
        TEST(test_unit_progress_logger_log_if_due_both_thresholds_reached),
        TEST(test_unit_progress_logger_log_if_due_advances_thresholds),
        TEST(test_unit_progress_logger_log_doesnt_advance_thresholds),
        TEST(test_unit_progress_logger_log_if_due_doesnt_log_same_value_twice),
        TEST(test_unit_progress_logger_log_if_due_zero_value_interval),
        TEST(test_unit_progress_logger_log_if_due_zero_time_interval),
        TEST(test_unit_progress_logger_log_if_due_both_intervals_zero),
    };

    return Test::run_suite("test_unit_progress_logger", tests, test_setup);
}
