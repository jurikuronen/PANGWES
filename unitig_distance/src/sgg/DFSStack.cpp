/*
 * DFSStack.cpp - Stack class used in single-genome graph (SGG) construction.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include "unitig_distance/sgg/DFSStack.hpp"

namespace PANGWES {

void DFSStack::push_back(std::size_t parent, std::size_t node, std::uint64_t weight) {
    m_stack.push_back({parent, node, weight});
}

bool DFSStack::empty() const noexcept {
    return m_stack.empty();
}

std::size_t DFSStack::next_parent() noexcept {
    return m_stack.back().parent;
}

std::size_t DFSStack::next_node() noexcept {
    return m_stack.back().node;
}

std::uint64_t DFSStack::next_weight() noexcept {
    return m_stack.back().weight;
}

void DFSStack::pop_back() noexcept {
    m_stack.pop_back();
}

} // namespace PANGWES
