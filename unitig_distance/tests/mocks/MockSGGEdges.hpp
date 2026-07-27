/*
 * MockSGGEdges.hpp - Mock implementation of SGGEdges.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cstddef>

#include "unitig_distance/sgg/SGGEdges.hpp"

namespace PANGWES {
namespace Mocks {

// Simple mock of SGGEdges providing access to internal state.
class MockSGGEdges : public SGGEdges {
public:
    // Constructs a mocked SGGEdges with the given size. Set to 0 for SGGEdges::empty() to return true.
    MockSGGEdges(std::size_t size = 0) {
        m_n_nodes = size;
        resize(size);
    }

    // Resize the internal adjacency list for mocked SGGEdges construction.
    void resize(std::size_t size) {
        m_adj.resize(size);
    }
};

} // namespace Mocks
} // namespace PANGWES
