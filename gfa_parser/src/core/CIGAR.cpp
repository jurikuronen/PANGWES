/*
 * CIGAR.cpp - Logic for parsing CIGAR strings.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <cctype>
#include <cstddef>
#include <limits>
#include <string>

#include "common/utils/Exception.hpp"
#include "gfa_parser/core/CIGAR.hpp"

namespace PANGWES {
namespace {

/*
 * Computes and returns the length of the CIGAR operation starting at position `pos`.
 *
 * Throws if the CIGAR operation length overflows.
 *
 * On return, `pos` is updated to point to the next index after the extracted length.
*/
std::size_t compute_cigar_operation_length_and_update_position(const std::string& cigar_string, std::size_t& position)
{
    std::size_t cigar_operation_length = 0;

    while (position < cigar_string.size() &&
           std::isdigit(static_cast<unsigned char>(cigar_string[position])) != 0)
    {
        const auto digit = static_cast<std::size_t>(cigar_string[position] - '0');

        if (cigar_operation_length > (std::numeric_limits<std::size_t>::max() - digit) / 10) {
            throw Exception(ErrorCode::UNSIGNED_INTEGER_OVERFLOW,
                            "CIGAR operation length too large for CIGAR string \"",
                            cigar_string, "\"");
        }

        cigar_operation_length = (cigar_operation_length * 10) + digit;
        ++position;
    }

    return cigar_operation_length;
}

// Parses `CIGAROperationType` from a character in the CIGAR string.
CIGAROperationType parse_cigar_operation_type(char cigar_operation_type_char) {
    switch (cigar_operation_type_char) {
        case 'M': return CIGAROperationType::M;
        case 'I': return CIGAROperationType::I;
        case 'D': return CIGAROperationType::D;
        case 'N': return CIGAROperationType::N;
        case 'S': return CIGAROperationType::S;
        case 'H': return CIGAROperationType::H;
        case 'P': return CIGAROperationType::P;
        case '=': return CIGAROperationType::EQUALS;
        case 'X': return CIGAROperationType::X;
        default: throw Exception(ErrorCode::INVALID_CIGAR_OPERATION, cigar_operation_type_char);
    }
}

} // namespace

CIGAR::CIGAR(const std::string& cigar_string)
    : m_overlap{},
      m_is_exact_sequence_match_operation{}
{
    if (cigar_string.empty()) {
        throw Exception(ErrorCode::INVALID_DATA, "CIGAR string must not be empty");
    }

    /*
     * In SAM Format, '*' denotes unavailable or unspecified CIGAR string, so we return here. If the CIGAR string does
     * not match '*' exactly and '*' is encountered later, an error will be thrown by `parse_cigar_operation_type()`.
    */
    if (cigar_string == "*") {
        return;
    }

    std::size_t position = 0;
    std::size_t overlap = 0;
    bool is_exact_sequence_match_operation = false;

    // CIGAR operations must start with length.
    if (std::isdigit(static_cast<unsigned char>(cigar_string[position])) == 0) {
        throw Exception(ErrorCode::INVALID_DATA,
                        "CIGAR operation must start with length; in CIGAR string \"",
                        cigar_string,
                        "\", detected CIGAR operation starting instead with ",
                        cigar_string[position]);
    }

    // After computing, `pos` points to the next index after the length.
    const auto cigar_operation_length = compute_cigar_operation_length_and_update_position(cigar_string, position);

    // After the CIGAR operation length, there MUST be a CIGAR operation type.
    if (position == cigar_string.size()) {
        throw Exception(ErrorCode::INVALID_DATA,
                        "CIGAR operation length MUST follow with a CIGAR operation type; CIGAR string \"",
                        cigar_string, "\" position ", position);
    }

    const auto cigar_operation_type = parse_cigar_operation_type(cigar_string[position]);

    if (++position != cigar_string.size()) {
        throw Exception(ErrorCode::INVALID_DATA,
                        "CIGAR string must contain exactly one operation: \"", cigar_string, "\"");
    }

    switch (cigar_operation_type) {
        case CIGAROperationType::M:
            overlap = cigar_operation_length;
            break;
        case CIGAROperationType::EQUALS:
            overlap = cigar_operation_length;
            is_exact_sequence_match_operation = true;
            break;
        default:
            // Other CIGAR operations are unsupported; return and keep `m_overlap` as zero.
            break;
    }

    m_overlap = overlap;
    m_is_exact_sequence_match_operation = is_exact_sequence_match_operation;
}

std::size_t CIGAR::overlap() const noexcept {
    return m_overlap;
}

bool CIGAR::is_exact_sequence_match_operation() const noexcept {
    return m_is_exact_sequence_match_operation;
}

CIGAR::CIGAR() noexcept
    : m_overlap{},
      m_is_exact_sequence_match_operation{}
{ }

} // namespace PANGWES
