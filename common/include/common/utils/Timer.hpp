/*
 * Timer.hpp - Lightweight time-keeping class.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <chrono>
#include <cstdint>
#include <type_traits>

namespace PANGWES {

/*
 * Time-keeping class implementation.
 *
 * Not thread-safe.
 *
 * Templated on Clock to allow deterministic testing with a mock clock.
*/
template <typename Clock>
class TimerImplementation {
    using MsT = std::chrono::milliseconds;

public:
    TimerImplementation() noexcept
        : m_mark{Clock::now()},
          m_accumulated{Clock::duration::zero()},
          m_last_interval{Clock::duration::zero()}
    { }

    // Sets the time point mark to the current time.
    void mark() noexcept  {
        m_mark = Clock::now();
    }

    // Measures now() - mark, stores it as the last interval, adds it to the total and sets mark to now().
    void accumulate() noexcept  {
        m_last_interval = Clock::now() - m_mark;
        m_accumulated += m_last_interval;
        mark();
    }

    /*
     * Returns the total accumulated time in milliseconds.
     *
     * If `accumulate_first` is true, calls `accumulate()` before reading the value.
    */
    std::uint64_t total_ms(bool accumulate_first = false) noexcept {
        if (accumulate_first) {
            accumulate();
        }

        return std::chrono::duration_cast<MsT>(m_accumulated).count();
    }

    /*
     * Returns the duration of the last measured interval in milliseconds.
     *
     * If `accumulate_first` is true, calls `accumulate()` before reading the value.
    */
    std::uint64_t last_interval_ms(bool accumulate_first = false) noexcept {
        if (accumulate_first) {
            accumulate();
        }

        return std::chrono::duration_cast<MsT>(m_last_interval).count();
    }

    // Returns the milliseconds since the Unix epoch.
    static std::uint64_t unix_time_ms() noexcept {
        // Time since epoch for steady_clock doesn't correspond to the Unix epoch, fall back to system_clock.
        if (std::is_same<Clock, std::chrono::steady_clock>::value) {
            return std::chrono::duration_cast<MsT>(std::chrono::system_clock::now().time_since_epoch()).count();
        }

        return std::chrono::duration_cast<MsT>(Clock::now().time_since_epoch()).count();
    }

private:
    // Time point mark. Set to current time when a timer object is created.
    typename Clock::time_point m_mark;

    // Stores accumulated total time.
    typename Clock::duration m_accumulated;

    // Stores duration of last interval.
    typename Clock::duration m_last_interval;
};

using Timer = TimerImplementation<std::chrono::steady_clock>;

} // namespace PANGWES
