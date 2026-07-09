/*
 * ProgressLogger.hpp - Helper class for logging progress.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cassert>
#include <cstdint>
#include <functional>
#include <limits>
#include <utility>

#include "common/utils/Timer.hpp"

namespace PANGWES {

// Called with current value and elapsed time in milliseconds since ProgressLogger construction.
using ProgressLoggerCallback = std::function<void(std::uint64_t, std::uint64_t)>;

/*
 * Progress-logging implementation.
 *
 * Not thread-safe.
 *
 * Templated on Timer to allow testing with MockTimer.
*/
template <typename Timer>
class ProgressLoggerImplementation {
public:
    ProgressLoggerImplementation() noexcept
        : m_timer{},
          m_log_callback{},
          m_log_value_interval{},
          m_log_time_interval_ms{},
          m_log_next_value{},
          m_log_next_time_ms{},
          m_previous_logged_value{std::numeric_limits<std::size_t>::max()}
    { }

    ProgressLoggerImplementation(ProgressLoggerCallback callback,
                                 std::uint64_t log_value_interval,
                                 std::uint64_t log_time_interval_ms) noexcept
        : m_timer{},
          m_log_callback{std::move(callback)},
          m_log_value_interval{log_value_interval},
          m_log_time_interval_ms{log_time_interval_ms},
          m_log_next_value{log_value_interval},
          m_log_next_time_ms{log_time_interval_ms},
          m_previous_logged_value{std::numeric_limits<std::size_t>::max()}
    { }

    ProgressLoggerImplementation(const ProgressLoggerImplementation&) = delete;
    ProgressLoggerImplementation& operator=(const ProgressLoggerImplementation&) = delete;
    ProgressLoggerImplementation(ProgressLoggerImplementation&& other) = default;
    ProgressLoggerImplementation& operator=(ProgressLoggerImplementation&& other) noexcept = default;

    // Logs progress if `value` was not logged yet.
    void log(std::uint64_t value) {
        assert(m_log_callback && "m_log_callback not set");

        if (value == m_previous_logged_value) {
            return;
        }

        const auto elapsed_time_ms = m_timer.total_ms(true);
        m_log_callback(value, elapsed_time_ms);

        m_previous_logged_value = value;
    }

    /*
     * Logs progress when `value` was not logged yet and both the value and elapsed time thresholds are reached.
     * Advances the thresholds if logging took place.
    */
    void log_if_due(std::uint64_t value) {
        assert(m_log_callback && "m_log_callback not set");

        if (value == m_previous_logged_value || value < m_log_next_value) {
            return;
        }

        const auto elapsed_time_ms = m_timer.total_ms(true);
        if (elapsed_time_ms < m_log_next_time_ms) {
            return;
        }

        m_log_callback(value, elapsed_time_ms);

        m_previous_logged_value = value;
        m_log_next_value = value + m_log_value_interval;
        m_log_next_time_ms = elapsed_time_ms + m_log_time_interval_ms;
    }

private:
    Timer m_timer;
    ProgressLoggerCallback m_log_callback;
    std::uint64_t m_log_value_interval;
    std::uint64_t m_log_time_interval_ms;
    std::uint64_t m_log_next_value;
    std::uint64_t m_log_next_time_ms;
    std::uint64_t m_previous_logged_value;
};

using ProgressLogger = ProgressLoggerImplementation<Timer>;

} // namespace PANGWES
