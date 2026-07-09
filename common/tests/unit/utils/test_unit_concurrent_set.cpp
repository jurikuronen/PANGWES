/*
 * test_unit_concurrent_set.cpp - Unit tests for utils/ConcurrentSet.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <cstddef>
#include <functional>
#include <numeric>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "common/test_harness/Test.hpp"
#include "common/utils/ConcurrentSet.hpp"
#include "common/utils/Exception.hpp"

namespace PANGWES {
namespace {

constexpr auto test_n_keys = 1000;
constexpr auto test_few_shards = 4;

// Shared test keys used by the tests.
std::vector<std::size_t> test_keys = []() {
    std::vector<std::size_t> keys(test_n_keys);

    // Create unique keys.
    std::iota(keys.begin(), keys.end(), 12345);

    return keys;
}();

// A custom structure key for testing.
struct TestStructKey {
    const std::size_t x;

    bool operator==(const TestStructKey& other) const noexcept {
        return x == other.x;
    }
};

// A custom hash for the above structure for testing.
struct TestStructKeyHash {
    std::size_t operator()(const TestStructKey& key) const {
        return std::hash<std::size_t>()(key.x);
    }
};

// A custom hash returning a constant value.
struct TestConstantHash {
    std::size_t operator()(std::size_t key) const {
        (void)key;
        return 0;
    }
};

bool test_unit_concurrent_set_constructor_throws_on_zero_n_shards() {
    EXPECT_THROW(ConcurrentSet<int>(0), ErrorCode::INVALID_ARGUMENT);

    return true;
}

template <typename Key, typename Hash = std::hash<Key>>
bool check_inserting_two_keys(const Key& expected_key1, const Key& expected_key2, bool do_reserve = false) {
    ConcurrentSet<Key, Hash> set{};

    if (do_reserve) {
        set.reserve(1000);
    }

    ASSERT_EQUAL(set.size(), 0);

    ASSERT_TRUE(set.insert(expected_key1));
    ASSERT_EQUAL(set.size(), 1);

    ASSERT_TRUE(set.insert(expected_key2));
    ASSERT_EQUAL(set.size(), 2);

    ASSERT_FALSE(set.insert(expected_key1));
    ASSERT_EQUAL(set.size(), 2);

    ASSERT_TRUE(set.contains(expected_key1));
    ASSERT_TRUE(set.contains(expected_key2));

    return true;
}

template <typename Key, typename Hash = std::hash<Key>>
bool check_inserting_two_moved_keys(const Key& expected_key1, const Key& expected_key2) {
    ConcurrentSet<Key, Hash> set{};

    auto key1 = expected_key1;
    auto key2 = expected_key2;
    auto duplicate_key1 = expected_key1;

    ASSERT_EQUAL(set.size(), 0);

    ASSERT_TRUE(set.insert(std::move(key1)));
    ASSERT_EQUAL(set.size(), 1);

    ASSERT_TRUE(set.insert(std::move(key2)));
    ASSERT_EQUAL(set.size(), 2);

    ASSERT_FALSE(set.insert(std::move(duplicate_key1)));
    ASSERT_EQUAL(set.size(), 2);

    ASSERT_TRUE(set.contains(expected_key1));
    ASSERT_TRUE(set.contains(expected_key2));

    return true;
}

bool test_unit_concurrent_set_insert_strings() {
    return check_inserting_two_keys(std::string{"str1"}, std::string{"str2"});
}

bool test_unit_concurrent_set_insert_unsigned_ints() {
    return check_inserting_two_keys(std::size_t{10}, std::size_t{20});
}

bool test_unit_concurrent_set_insert_custom_key() {
    return check_inserting_two_keys<TestStructKey, TestStructKeyHash>(TestStructKey{1}, TestStructKey{2});
}

bool test_unit_concurrent_set_insert_strings_with_move() {
    return check_inserting_two_moved_keys(std::string{"str1"}, std::string{"str2"});
}

bool test_unit_concurrent_set_insert_unsigned_ints_with_move() {
    return check_inserting_two_moved_keys(std::size_t{10}, std::size_t{20});
}

bool test_unit_concurrent_set_insert_custom_key_with_move() {
    return check_inserting_two_moved_keys<TestStructKey, TestStructKeyHash>(TestStructKey{1}, TestStructKey{2});
}

bool test_unit_concurrent_set_reserve_before_inserting() {
    return check_inserting_two_keys(std::string{"1"}, std::string{"2"}, true) &&
           check_inserting_two_keys(1, 2, true);
}

bool test_unit_concurrent_set_contains_returns_false_for_nonexisting_key() {
    ConcurrentSet<std::size_t> set{};

    ASSERT_FALSE(set.contains(1));
    ASSERT_FALSE(set.contains(2));

    set.insert(1);

    ASSERT_TRUE(set.contains(1));
    ASSERT_FALSE(set.contains(2));

    return true;
}

template <typename SetT, typename IterateOverKeysMethodT>
bool check_iteration_visits_all_keys(IterateOverKeysMethodT iterate_over_keys) {
    SetT set(test_few_shards);
    std::set<std::size_t> expected_keys;

    for (const auto& key : test_keys) {
        ASSERT_TRUE(set.insert(key));
        expected_keys.insert(key);
    }

    ASSERT_EQUAL(expected_keys.size(), test_n_keys);

    const auto keys_seen = iterate_over_keys(set);

    ASSERT_TRUE(keys_seen == expected_keys);

    return true;
}

bool test_unit_concurrent_set_iterator_visits_all_keys() {
    const auto iterate_over_keys = [](const ConcurrentSet<std::size_t>& set) {
        std::set<std::size_t> keys_seen;

        for (auto key_it = set.begin(); key_it != set.end(); ++key_it) {
            keys_seen.insert(*key_it);
        }

        return keys_seen;
    };

    return check_iteration_visits_all_keys<ConcurrentSet<std::size_t>>(iterate_over_keys);
}

bool test_unit_concurrent_set_range_based_for_loop_visits_all_keys() {
    const auto iterate_over_keys = [](const ConcurrentSet<std::size_t>& set) {
        std::set<std::size_t> keys_seen;

        for (const auto& key : set) {
            keys_seen.insert(key);
        }

        return keys_seen;
    };

    return check_iteration_visits_all_keys<ConcurrentSet<std::size_t>>(iterate_over_keys);
}

bool test_unit_concurrent_set_range_based_for_loop_visits_all_keys_with_hash_collisions() {
    const auto iterate_over_keys = [](const ConcurrentSet<std::size_t, TestConstantHash>& set) {
        std::set<std::size_t> keys_seen;

        for (auto key_it = set.begin(); key_it != set.end(); ++key_it) {
            keys_seen.insert(*key_it);
        }

        return keys_seen;
    };

    return check_iteration_visits_all_keys<ConcurrentSet<std::size_t, TestConstantHash>>(iterate_over_keys);
}

bool test_unit_concurrent_set_clear_and_release_reserved_memory_removes_keys() {
    ConcurrentSet<std::size_t> set;

    for (const auto test_key : test_keys) {
        ASSERT_TRUE(set.insert(test_key));
        ASSERT_TRUE(set.contains(test_key));
    }

    ASSERT_EQUAL(set.size(), test_keys.size());

    set.clear_and_release_reserved_memory();

    ASSERT_EQUAL(set.size(), 0);

    for (const auto test_key : test_keys) {
        ASSERT_FALSE(set.contains(test_key));
        // Re-insertion works.
        ASSERT_TRUE(set.insert(test_key));
    }

    ASSERT_EQUAL(set.size(), test_keys.size());

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_unit_concurrent_set_constructor_throws_on_zero_n_shards),
        TEST(test_unit_concurrent_set_insert_strings),
        TEST(test_unit_concurrent_set_insert_unsigned_ints),
        TEST(test_unit_concurrent_set_insert_custom_key),
        TEST(test_unit_concurrent_set_insert_strings_with_move),
        TEST(test_unit_concurrent_set_insert_unsigned_ints_with_move),
        TEST(test_unit_concurrent_set_insert_custom_key_with_move),
        TEST(test_unit_concurrent_set_reserve_before_inserting),
        TEST(test_unit_concurrent_set_contains_returns_false_for_nonexisting_key),
        TEST(test_unit_concurrent_set_iterator_visits_all_keys),
        TEST(test_unit_concurrent_set_range_based_for_loop_visits_all_keys),
        TEST(test_unit_concurrent_set_range_based_for_loop_visits_all_keys_with_hash_collisions),
        TEST(test_unit_concurrent_set_clear_and_release_reserved_memory_removes_keys),
    };

    return Test::run_suite("test_unit_concurrent_set", tests);
}
