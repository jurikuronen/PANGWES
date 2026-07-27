/*
 * hash.hpp - Hashing-related utility functions.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cstddef>
#include <functional>

namespace PANGWES {
namespace Hash {

// Mixes the hash of `value` into `seed` by re-implementing the classic Boost `hash_combine` step.
template <typename T>
void combine(std::size_t& seed, const T& value) {
    constexpr auto golden_ratio_hash_constant = static_cast<std::size_t>(0x9e3779b9);
    constexpr auto boost_hash_combine_left_shift = 6;
    constexpr auto boost_hash_combine_right_shift = 2;

    seed ^= std::hash<T>()(value) +
            golden_ratio_hash_constant +
            (seed << boost_hash_combine_left_shift) +
            (seed >> boost_hash_combine_right_shift);
}

} // namespace Hash
} // namespace PANGWES
