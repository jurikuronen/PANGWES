/*
 * DFSStack.hpp - Stack class used in single-genome graph (SGG) construction.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace PANGWES {

// Stack class used in the depth-first search launched by a single-genome graph's (SGG) constructor.
class DFSStack {
public:
    // Inserts a {parent, node, weight} element at the end of the stack vector.
    void push_back(std::size_t parent, std::size_t node, std::uint64_t weight);

    // Returns true if the stack is empty.
    bool empty() const noexcept;

    // Returns the parent at the top of the stack. Precondition: stack must be non-empty.
    std::size_t next_parent() noexcept;

    // Returns the node at the top of the stack. Precondition: stack must be non-empty.
    std::size_t next_node() noexcept;

    // Returns the weight at the top of the stack. Precondition: stack must be non-empty.
    std::uint64_t next_weight() noexcept;

    // Deletes the last element. Precondition: stack must be non-empty.
    void pop_back() noexcept;

private:
    struct DFSStackElement {
        std::size_t parent;
        std::size_t node;
        std::uint64_t weight;
    };

    std::vector<DFSStackElement> m_stack;
};

} // namespace PANGWES
