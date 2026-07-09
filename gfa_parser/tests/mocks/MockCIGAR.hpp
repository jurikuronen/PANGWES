/*
 * MockCIGAR.hpp - Mock implementation of CIGAR in core/CIGAR.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cstddef>

#include "gfa_parser/core/CIGAR.hpp"

namespace PANGWES {
namespace Mocks {

// Simple mock implementation of CIGAR via inheritance.
class MockCIGAR final : public CIGAR {
public:
    // Construct by directly setting the "parsed" overlap and exact sequence match operation member variables.
    MockCIGAR(std::size_t overlap, bool is_exact_sequence_match_operation) noexcept
        : CIGAR()
    {
        set_overlap(overlap);

        if (is_exact_sequence_match_operation) {
            set_exact_sequence_match();
        }
    }
};

} // namespace Mocks
} // namespace PANGWES
