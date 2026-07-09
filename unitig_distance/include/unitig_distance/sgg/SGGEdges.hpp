/*
 * SGGEdges.hpp - Class for storing edge data of a single-genome graph (SGG).
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cstddef>
#include <memory>

#include "common/io/FileReaderInterface.hpp"
#include "unitig_distance/core/UnitigWeights.hpp"
#include "unitig_distance/sgg/sgg_types.hpp"

namespace PANGWES {

/*
 * Stores edge data of a SGG read from an input file that contains compacted de Bruijn graph (cdBG) edges.
 *
 * The data will already be in graph format, but the edges will get further processed by the SGG class.
*/
class SGGEdges {
public:
    // Constructs the edges data by reading from a file. Edge weights are derived using the provided unitig weights.
    SGGEdges(std::unique_ptr<FileReaderInterface> reader, const UnitigWeights& unitig_weights);

    SGGEdges(const SGGEdges&) = delete;
    SGGEdges& operator=(const SGGEdges&) = delete;
    SGGEdges(SGGEdges&&) noexcept = default;
    SGGEdges& operator=(SGGEdges&&) noexcept = default;

    // Returns the number of nodes with degree > 0.
    std::size_t n_nodes() const noexcept;

    // Returns the size of the internal adjacency list.
    std::size_t size() const noexcept;

    // Returns the allocated capacity of the internal adjacency list.
    std::size_t capacity() const noexcept;

    // Returns the number of bytes of dynamic storage reserved by this object (excludes allocator overhead).
    std::size_t reserved_bytes() const noexcept;

    // Returns true if the represented graph contains no nodes with degree > 0.
    bool empty() const noexcept;

    // Returns true if the given unitig had edges in the input file.
    bool contains(std::size_t unitig_id) const noexcept;

    // Returns the edge list for the given node.
    EdgeListT& operator[](std::size_t idx);
    const EdgeListT& operator[](std::size_t idx) const;

protected:
    AdjacencyListT m_adj;
    std::size_t m_n_nodes;

    // Provided for mocking in unit tests.
    SGGEdges() = default;
};

} // namespace PANGWES
