/*
 * UnitigWeights.cpp - Class for storing unitig weights data.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <string>
#include <vector>

#include "common/io/FileReaderInterface.hpp"
#include "common/io/Log.hpp"
#include "common/utils/Exception.hpp"
#include "common/utils/memory.hpp"
#include "common/utils/utils.hpp"
#include "unitig_distance/core/UnitigWeights.hpp"

namespace PANGWES {
namespace {

constexpr auto N_REQUIRED_FIELDS = 2;
constexpr auto UNITIG_STRING_FIELD = 1;

} // namespace

UnitigWeights::UnitigWeights(std::unique_ptr<FileReaderInterface> reader, std::uint64_t kmer_length)
{
    std::vector<std::uint64_t> weights;

    reader->open();

    for (std::string line; reader->getline(line); ) {
        if (line.empty()) {
            continue;
        }

        const auto fields = Utils::get_fields_ws(line);
        if (fields.size() < N_REQUIRED_FIELDS) {
            throw Exception(ErrorCode::FILE_WRONG_COLUMN_COUNT, "unitigs file line ", reader->line_number());
        }

        /*
         * Compute the unitig's weight from the length. This corresponds to the number of edges compacted by the unitig
         * compared to the k-mer.
        */
        const auto unitig_length = static_cast<std::uint64_t>(fields.at(UNITIG_STRING_FIELD).size());
        if (unitig_length < kmer_length) {
            throw Exception(ErrorCode::INVALID_KMER_LENGTH,
                            "unitig length ", unitig_length, " is less than k-mer length ",
                            kmer_length, " at unitigs file line ", reader->line_number());
        }

        weights.push_back(unitig_length - kmer_length);
    }

    reader->close();

    if (weights.empty()) {
        throw Exception(ErrorCode::FILE_EMPTY, "unitigs file");
    }

    m_weights = std::move(weights);
}

std::size_t UnitigWeights::size() const noexcept {
    return m_weights.size();
}

std::size_t UnitigWeights::reserved_bytes() const noexcept {
    return sizeof(*this) + Memory::container_reserved_bytes(m_weights);
}

bool UnitigWeights::contains(std::size_t unitig_id) const noexcept {
    return unitig_id < size();
}

std::uint64_t UnitigWeights::weight(std::size_t unitig_id) const {
    return m_weights.at(unitig_id);
}

} // namespace PANGWES
