/*
 * SGGEdgesFilenames.hpp - Class for storing single-genome graph (SGG) edge-list filenames.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "common/io/FileReaderInterface.hpp"

namespace PANGWES {

/*
 * Stores the edge-list filenames corresponding to SGGs, i.e. individual color subgraphs of the global compacted de
 * Bruijn graph.
*/
class SGGEdgesFilenames {
public:
    // Constructs the list of SGG edge-list filenames by reading them from a SGG paths input file.
    explicit SGGEdgesFilenames(std::unique_ptr<FileReaderInterface> reader);

    SGGEdgesFilenames(const SGGEdgesFilenames&) = delete;
    SGGEdgesFilenames& operator=(const SGGEdgesFilenames&) = delete;
    SGGEdgesFilenames(SGGEdgesFilenames&&) = default;
    SGGEdgesFilenames& operator=(SGGEdgesFilenames&&) = default;

    // Returns the number of single-genome graph edges filenames read.
    std::size_t size() const noexcept;

    // Returns the number of bytes of dynamic storage reserved by this object (excludes allocator overhead).
    std::size_t reserved_bytes() const noexcept;

    // Returns filename indices ordered by descending file size.
    std::vector<std::size_t> indices_by_descending_file_size() const;

    // Returns the edges filename for the single-genome graph at the given index.
    const std::string& operator[](std::size_t idx) const;

    // Returns the first iterator of the edges filenames.
    std::vector<std::string>::const_iterator begin() const noexcept;

    // Returns the end iterator of the edges filenames.
    std::vector<std::string>::const_iterator end() const noexcept;

private:
    std::vector<std::string> m_sgg_edges_filenames;
};

} // namespace PANGWES
