/*
 * SGGEdgesFilenames.cpp - Class for storing single-genome graph (SGG) edge-list filenames.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <algorithm>
#include <cassert>
#include <numeric>
#include <string>
#include <vector>

#include "common/io/filesystem.hpp"
#include "common/io/Log.hpp"
#include "common/utils/Exception.hpp"
#include "common/utils/memory.hpp"
#include "unitig_distance/sgg/SGGEdgesFilenames.hpp"

namespace PANGWES {

SGGEdgesFilenames::SGGEdgesFilenames(std::unique_ptr<FileReaderInterface> reader) {
    std::vector<std::string> sgg_edges_filenames;

    reader->open();

    for (std::string line; reader->getline(line); ) {
        if (line.empty()) {
            continue;
        }

        sgg_edges_filenames.push_back(line);
    }

    reader->close();

    if (sgg_edges_filenames.empty()) {
        throw Exception(ErrorCode::FILE_EMPTY, "SGG .paths file");
    }

    m_sgg_edges_filenames = std::move(sgg_edges_filenames);
}

std::size_t SGGEdgesFilenames::size() const noexcept {
    return m_sgg_edges_filenames.size();
}

std::size_t SGGEdgesFilenames::reserved_bytes() const noexcept {
    return sizeof(*this) + Memory::container_reserved_bytes(m_sgg_edges_filenames);
}

std::vector<std::size_t> SGGEdgesFilenames::indices_by_descending_file_size() const {
    std::vector<std::size_t> file_sizes(size());
    std::vector<std::size_t> indices(size());
    std::iota(indices.begin(), indices.end(), 0);

    for (std::size_t index = 0; index < size(); ++index) {
        file_sizes[index] = Filesystem::file_size(m_sgg_edges_filenames[index]);
    }

    std::sort(indices.begin(), indices.end(), [&file_sizes](std::size_t lhs, std::size_t rhs) {
        return file_sizes[lhs] == file_sizes[rhs] ? lhs < rhs : file_sizes[lhs] > file_sizes[rhs];
    });

    return indices;
}

const std::string& SGGEdgesFilenames::operator[](std::size_t idx) const {
    assert(idx < size() && "index out of bounds");

    return m_sgg_edges_filenames[idx];
}

std::vector<std::string>::const_iterator SGGEdgesFilenames::begin() const noexcept {
    return m_sgg_edges_filenames.begin();
}

std::vector<std::string>::const_iterator SGGEdgesFilenames::end() const noexcept {
    return m_sgg_edges_filenames.end();
}

} // namespace PANGWES
