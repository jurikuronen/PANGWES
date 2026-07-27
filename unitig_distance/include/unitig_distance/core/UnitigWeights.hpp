/*
 * UnitigWeights.hpp - Class for storing unitig weights data.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "common/io/FileReaderInterface.hpp"

namespace PANGWES {

/*
 * Class for storing unitig weights data read from an input file.
 *
 * It is expected that the input file's lines follow the format [UNITIG_ID UNITIG_SEQUENCE].
 *
 * - UNITIG_ID is unused by this program.
 * - The UNITIG_SEQUENCE's length is used to calculate the weight, which is defined as unitig length - k-mer length.
 *   This corresponds to the number of edges compacted by the unitig compared to the k-mer.
*/
class UnitigWeights {
public:
    // Constructs unitig weights by reading from a file.
    UnitigWeights(std::unique_ptr<FileReaderInterface> reader, std::uint64_t kmer_length);

    UnitigWeights(const UnitigWeights&) = delete;
    UnitigWeights& operator=(const UnitigWeights&) = delete;
    UnitigWeights(UnitigWeights&&) noexcept = default;
    UnitigWeights& operator=(UnitigWeights&&) noexcept = default;

    // Returns the number of unitig weights.
    std::size_t size() const noexcept;

    // Returns the number of bytes of dynamic storage reserved by this object (excludes allocator overhead).
    std::size_t reserved_bytes() const noexcept;

    // Returns true if the given unitig was read.
    bool contains(std::size_t unitig_id) const noexcept;

    // Returns the weight (unitig length - k-mer length) of the given unitig.
    std::uint64_t weight(std::size_t unitig_id) const;

protected:
    // Provided for mocking in unit tests.
    UnitigWeights() = default;

private:
    std::vector<std::uint64_t> m_weights;
};

} // namespace PANGWES
