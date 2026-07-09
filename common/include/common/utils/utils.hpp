/*
 * utils.hpp - Utility functions.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#include "common/utils/Exception.hpp"

namespace PANGWES {
namespace Utils {

constexpr auto MAX_N_FIELDS = std::numeric_limits<std::size_t>::max() / 2;

// Parses whitespace-separated fields from the given line.
inline std::vector<std::string> get_fields_ws(const std::string& line, std::size_t n_fields = MAX_N_FIELDS) {
    std::vector<std::string> fields;
    std::istringstream iss(line);

    std::size_t n_field = 0;

    for (std::string field; n_field++ < n_fields && iss >> field; ) {
        fields.push_back(field);
    }

    return fields;
}

// Parses fields from the given line based on a custom delimiter character.
inline std::vector<std::string> get_fields(const std::string& line,
                                           char delimiter,
                                           std::size_t n_fields = MAX_N_FIELDS)
{
    std::vector<std::string> fields;
    std::istringstream iss(line);

    std::size_t n_field = 0;

    for (std::string field; n_field++ < n_fields && std::getline(iss, field, delimiter); ) {
        fields.push_back(field);
    }

    return fields;
}

// Parses a string containing an unsigned integer. Throws if the string is not a valid unsigned integer.
inline std::uint64_t parse_unsigned_value(const std::string& value) {
    /*
     * Using a signed integer to check validity technically loses the last bit, but using values of that order would
     * already cause other problems.
    */
    std::int64_t signed_value{};

    std::stringstream stream(value);
    stream >> signed_value;

    // Must consume the whole string and be a valid unsigned integer.
    if (!stream || !stream.eof() || signed_value < 0) {
        throw Exception(ErrorCode::INVALID_DATA, value);
    }

    return static_cast<std::uint64_t>(signed_value);
}

// Adds two non-negative integers with overflow checks.
template <typename LHS, typename RHS>
inline auto safe_add(LHS lhs, RHS rhs, const std::string& error_msg = "") -> decltype(lhs + rhs) {
    using ResultT = decltype(lhs + rhs);

    static_assert(std::is_unsigned<ResultT>::value, "safe_add requires an unsigned result type");

    if ((std::is_signed<LHS>::value && lhs < 0) || (std::is_signed<RHS>::value && rhs < 0)) {
        if (error_msg.empty()) {
            throw Exception(ErrorCode::INVALID_ARGUMENT, "lhs: ", lhs, ", rhs: ", rhs);
        }
        throw Exception(ErrorCode::INVALID_ARGUMENT, error_msg, ": lhs: ", lhs, ", rhs: ", rhs);
    }

    const auto lhs_result_t = static_cast<ResultT>(lhs);
    const auto rhs_result_t = static_cast<ResultT>(rhs);

    if (lhs_result_t > std::numeric_limits<ResultT>::max() - rhs_result_t) {
        if (error_msg.empty()) {
            throw Exception(ErrorCode::UNSIGNED_INTEGER_OVERFLOW, "lhs: ", lhs, ", rhs: ", rhs);
        }
        throw Exception(ErrorCode::UNSIGNED_INTEGER_OVERFLOW, error_msg, ": lhs: ", lhs, ", rhs: ", rhs);
    }

    return lhs_result_t + rhs_result_t;
}

// Multiplies two non-negative integers with overflow checks.
template <typename LHS, typename RHS>
inline auto safe_multiply(LHS lhs, RHS rhs, const std::string& error_msg = "") -> decltype(lhs * rhs) {
    using ResultT = decltype(lhs * rhs);

    static_assert(std::is_unsigned<ResultT>::value, "safe_multiply requires an unsigned result type");

    if ((std::is_signed<LHS>::value && lhs < 0) || (std::is_signed<RHS>::value && rhs < 0)) {
        if (error_msg.empty()) {
            throw Exception(ErrorCode::INVALID_ARGUMENT, "lhs: ", lhs, ", rhs: ", rhs);
        }
        throw Exception(ErrorCode::INVALID_ARGUMENT, error_msg, ": lhs: ", lhs, ", rhs: ", rhs);
    }

    const auto lhs_result_t = static_cast<ResultT>(lhs);
    const auto rhs_result_t = static_cast<ResultT>(rhs);

    if (rhs_result_t != 0 && lhs_result_t > std::numeric_limits<ResultT>::max() / rhs_result_t) {
        if (error_msg.empty()) {
            throw Exception(ErrorCode::UNSIGNED_INTEGER_OVERFLOW, "lhs: ", lhs, ", rhs: ", rhs);
        }
        throw Exception(ErrorCode::UNSIGNED_INTEGER_OVERFLOW, error_msg, ": lhs: ", lhs, ", rhs: ", rhs);
    }

    return lhs_result_t * rhs_result_t;
}

} // namespace Utils
} // namespace PANGWES
