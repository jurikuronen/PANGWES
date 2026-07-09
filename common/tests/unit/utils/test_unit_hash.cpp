/*
 * test_unit_hash.cpp - Unit tests for hashing-related utility functions defined in utils/hash.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <algorithm>
#include <cstddef>
#include <vector>

#include "common/test_harness/Test.hpp"
#include "common/utils/hash.hpp"

namespace PANGWES {
namespace {

bool test_unit_hash_combine_seed_changes() {
    for (auto i = 0; i < 100; ++i) {
        std::size_t seed = 0;

        Hash::combine(seed, i);

        ASSERT_NOT_EQUAL(seed, 0);
    }

    return true;
}

bool test_unit_hash_combine_multiple_times() {
    constexpr auto max_values = 100;

    for (std::size_t i = 1; i < max_values; ++i) {
        std::vector<std::size_t> values;
        std::size_t seed = 0;

        // Combine `i` times.
        for (std::size_t j = 0; j < i; ++j) {
            Hash::combine(seed, max_values * i + j);
            values.push_back(seed);
        }

        // Check that the hash changed each time.
        std::sort(values.begin(), values.end());
        const auto unique_end = std::unique(values.begin(), values.end());

        ASSERT_TRUE(values.end() == unique_end);
    }

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_unit_hash_combine_seed_changes),
        TEST(test_unit_hash_combine_multiple_times)
    };

    return Test::run_suite("test_unit_hash", tests);
}
