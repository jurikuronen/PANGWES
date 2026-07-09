/*
 * CIGAR.hpp - Logic for parsing CIGAR strings.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace PANGWES {

// CIGAR operation types as defined by the Sequence Alignment Map (SAM) Format Specification.
enum class CIGAROperationType : std::uint8_t {
    M,       // alignment match (can be a sequence match or mismatch)
    I,       // insertion to the reference
    D,       // deletion from the reference
    N,       // skipped region from the reference
    S,       // soft clipping (clipped sequences present in SEQ)
    H,       // hard clipping (clipped sequences NOT present in SEQ)
    P,       // padding (silent deletion from padded reference)
    EQUALS,  // sequence match
    X,       // sequence mismatch
};

/*
 * Restricted CIGAR string parser class.
 *
 * Within the scope of `unitig_distance`, only ungapped overlaps between segments are supported. This means CIGAR
 * strings composed only of a single alignment match (`M`) or a single sequence match (`=`) operation.
 *
 * Other CIGAR operation types describe gapped, clipped, skipped, padded or explicitly mismatching alignments, which are
 * unsuitable for connecting segments in compacted de Bruijn graphs.
 *
 * Note that `M` may represent either sequence matches or mismatches, whereas `=` denotes an exact sequence match. This
 * is due to legacy reasons, as the original CIGAR format lacked other operators than the first three: `M`, `I` and `D`.
 * Therefore, overlap lengths calculated from `M` operations require later overlap validation by the program.
 *
 * After construction from `cigar_string`:
 * - `overlap()` returns the length of the overlap, or zero for unavailable or unsupported CIGAR strings.
 * - `is_exact_sequence_match_operation()` returns true if the CIGAR string described an exact sequence match.
*/
class CIGAR {
public:
    /*
     * Parses overlap from `cigar_string`.
     *
     * Supported CIGAR strings consist only of a single `M` or `=` operation. For any other cases, the stored overlap
     * becomes zero.
     *
     * The special SAM value, `*`, denotes an unavailable CIGAR string and also results in a zero overlap.
     *
     * Throws if `cigar_string` is empty, malformed or an operation length overflows `std::size_t`.
    */
    explicit CIGAR(const std::string& cigar_string);

    // Returns the parsed overlap length. Returns zero for syntactically valid but unsupported cases.
    std::size_t overlap() const noexcept;

    // Returns true if the CIGAR string described an exact sequence match (`=`).
    bool is_exact_sequence_match_operation() const noexcept;

protected:
    // Provided for mocking in unit tests.
    CIGAR() noexcept;

private:
    std::size_t m_overlap;
    bool m_is_exact_sequence_match_operation;
};

} // namespace PANGWES
