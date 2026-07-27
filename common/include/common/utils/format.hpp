/*
 * format.hpp - Text formatting-related utility functions.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cassert>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>

namespace PANGWES {
namespace Format {

// Converts a number into a string with the thousands separated by spaces.
inline std::string pretty_uint(std::uint64_t number) {
    std::ostringstream oss;

    const auto high = number / 1000;
    const auto low = number % 1000;

    if (high > 0) {
        oss << pretty_uint(high) << ' ' << std::setw(3) << std::setfill('0') << low;
    } else {
        oss << low;
    }

    return oss.str();
}

// Converts milliseconds into a time string with "D day(s)", "Hh Mm", "Mm Ss" or "S.mmms" format.
inline std::string duration_to_string(std::uint64_t milliseconds) {
    const auto seconds = milliseconds / 1000;
    const auto minutes = seconds / 60;
    const auto hours = minutes / 60;
    const auto days = hours / 24;

    std::ostringstream oss;

    if (days > 0) {
        oss << days << (days == 1 ? " day" : " days");
    } else if (hours > 0) {
        oss << (hours % 24) << "h " << (minutes % 60) << 'm';
    } else if (minutes > 0) {
        oss << (minutes % 60) << "m " << (seconds % 60) << 's';
    } else {
        oss << (seconds % 60) << '.' << std::setfill('0') << std::setw(3) << (milliseconds % 1000) << 's';
    }

    return oss.str();
}

// Converts Unix time into a date block with "[YYYY Mon DD - HH:MM:SS]" format.
inline std::string date_block(std::uint64_t unix_time_ms) {
    static std::mutex mtx{};

    // Convert to seconds for local std::tm.
    const auto unix_time_s = static_cast<std::time_t>(unix_time_ms / 1000);

    // Need a lock because std::localtime is not thread-safe.
    std::lock_guard<std::mutex> lock(mtx);

    // Convert to a local std::tm.
    const auto tm_local = *std::localtime(&unix_time_s);

    char buffer[64];
    const auto count = std::strftime(buffer, sizeof(buffer), "[%Y %b %d - %H:%M:%S]", &tm_local);

    return (count > 0) ? std::string(buffer, count) : std::string{"[invalid time]"};
}

} // namespace Format
} // namespace PANGWES
