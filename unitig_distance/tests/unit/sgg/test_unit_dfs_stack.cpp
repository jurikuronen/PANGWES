/*
 * test_unit_dfs_stack.cpp - Unit tests for sgg/DFSStack.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <cstddef>
#include <cstdint>
#include <vector>

#include "common/test_harness/Test.hpp"
#include "unitig_distance/sgg/DFSStack.hpp"

namespace PANGWES {
namespace {

bool test_unit_dfs_stack_empty_by_default() {
    DFSStack stack;

    ASSERT_TRUE(stack.empty());

    return true;
}

bool test_unit_dfs_stack_add_element() {
    constexpr auto parent = 1;
    constexpr auto node = 2;
    constexpr auto weight = 3;

    DFSStack stack;

    stack.push_back(parent, node, weight);

    ASSERT_FALSE(stack.empty());

    ASSERT_EQUAL(stack.next_parent(), parent);
    ASSERT_EQUAL(stack.next_node(), node);
    ASSERT_EQUAL(stack.next_weight(), weight);

    return true;
}

bool test_unit_dfs_stack_top_is_last_pushed_element() {
    DFSStack stack;

    stack.push_back(1, 2, 3);
    stack.push_back(4, 5, 6);

    ASSERT_FALSE(stack.empty());

    ASSERT_EQUAL(stack.next_parent(), 4);
    ASSERT_EQUAL(stack.next_node(), 5);
    ASSERT_EQUAL(stack.next_weight(), 6);

    stack.pop_back();

    ASSERT_EQUAL(stack.next_parent(), 1);
    ASSERT_EQUAL(stack.next_node(), 2);
    ASSERT_EQUAL(stack.next_weight(), 3);

    return true;
}

bool test_unit_dfs_stack_delete_element() {
    DFSStack stack;

    stack.push_back(0, 0, 0);

    ASSERT_FALSE(stack.empty());

    stack.pop_back();

    ASSERT_TRUE(stack.empty());

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_unit_dfs_stack_empty_by_default),
        TEST(test_unit_dfs_stack_add_element),
        TEST(test_unit_dfs_stack_top_is_last_pushed_element),
        TEST(test_unit_dfs_stack_delete_element),
    };

    return Test::run_suite("test_unit_dfs_stack", tests);
}
