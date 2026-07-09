/*
 * Segment.cpp - Segment record storage.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <algorithm>
#include <cctype>

#include "common/utils/Exception.hpp"
#include "gfa_parser/core/Segment.hpp"

namespace PANGWES {

Segment::Segment(std::string name, std::string sequence) {
    if (name.empty()) {
        throw Exception(ErrorCode::INVALID_DATA, "empty Segment Name");
    }

    if (sequence.empty()) {
        throw Exception(ErrorCode::INVALID_DATA, "Segment Sequence is required for gfa_parser");
    }

    if (sequence[0] == '*') {
        throw Exception(ErrorCode::INVALID_DATA, "Segment Sequence must be specified for gfa_parser");
    }

    // The specification requires no codepoint values higher than 127.
    if (std::any_of(name.begin(), name.end(), [](unsigned char chr){ return chr > 127; })) {
        throw Exception(ErrorCode::INVALID_DATA, "Invalid characters in Segment Name; Name: ", name);
    }

    // Omit checking some other requirements on the name as they cause no issues for our use-case.

    // Very soft check for a valid nucleic acid sequence.
    if (std::any_of(sequence.begin(),
                    sequence.end(),
                    [](unsigned char chr) { return std::isalpha(chr) == 0; }))
    {
        throw Exception(ErrorCode::INVALID_DATA,
                               "Segment Sequence must consist only of alphabetic characters; Sequence: ",
                               sequence);
    }

    m_name = std::move(name);
    m_sequence = std::move(sequence);
}

const std::string& Segment::name() const noexcept {
    return m_name;
}

const std::string& Segment::sequence() const noexcept {
    return m_sequence;
}

} // namespace PANGWES
