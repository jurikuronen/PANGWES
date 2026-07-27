/*
 * MockTimer.hpp - Mock implementation of Timer in utils/Timer.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <chrono>
#include <cstdint>
#include <ctime>
#include <mutex>
#include <stdexcept>

#include "common/utils/Timer.hpp"

namespace PANGWES {
namespace Mocks {

// Mock the clock implementation of Timer.
class MockClock {
    public:
        using duration = std::chrono::milliseconds;
        using time_point = std::chrono::time_point<MockClock>;

        // Advance the mocked time by some duration.
        static void advance(std::uint64_t milliseconds) noexcept {
            mock_time_point() += duration{milliseconds};
        }

        // Rewind the mocked time by some duration.
        static void rewind(std::uint64_t milliseconds) noexcept {
            mock_time_point() -= duration{milliseconds};
        }

        // Returns the current mocked time.
        static time_point now() noexcept {
            return mock_time_point();
        }

        // Sets the current time to `ms_since_epoch` milliseconds after epoch.
        static void set_time(std::uint64_t time_since_epoch_ms) noexcept {
            mock_time_point() = time_point{duration::zero()};

            advance(time_since_epoch_ms);
        }

        // Sets the current time to `ms_since_epoch` milliseconds after epoch in mocked UTC time.
        static void set_utc_time(std::uint64_t time_since_epoch_ms) {
            static std::mutex mtx{};

            const auto time = static_cast<std::time_t>(time_since_epoch_ms / 1000);

            // Need a lock because std::localtime is not thread-safe.
            std::lock_guard<std::mutex> lock(mtx);

            std::tm local_tm = *std::localtime(&time);
            std::tm utc_tm  = *std::gmtime(&time);

            // Disable daylight saving time.
            local_tm.tm_isdst = -1;
            utc_tm.tm_isdst = -1;

            const auto local_s = std::mktime(&local_tm);
            const auto utc_as_local_s = std::mktime(&utc_tm);

            const auto offset_s = static_cast<std::int64_t>(utc_as_local_s - local_s);

            set_time(time_since_epoch_ms + offset_s * 1000);
        }

    private:
        static time_point& mock_time_point() {
            static time_point timepoint{};

            return timepoint;
        }
};

using MockTimer = TimerImplementation<MockClock>;

} // namespace Mocks
} // namespace PANGWES
