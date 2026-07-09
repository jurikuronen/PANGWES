/*
 * Exception.hpp - Custom exception class.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace PANGWES {

// X macro list of error codes used to generate ErrorCode and related data.
#define ERROR_CODES \
    ERROR_CODE_OP(EMPTY_DATA) \
    ERROR_CODE_OP(EMPTY_FILENAME) \
    ERROR_CODE_OP(INVALID_ARGUMENT) \
    ERROR_CODE_OP(INVALID_CIGAR_OPERATION) \
    ERROR_CODE_OP(INVALID_DATA) \
    ERROR_CODE_OP(INVALID_GFA_FORMAT) \
    ERROR_CODE_OP(INVALID_KMER_LENGTH) \
    ERROR_CODE_OP(INVALID_PROGRAM_OPTION) \
    ERROR_CODE_OP(INVALID_RECORD_TYPE) \
    ERROR_CODE_OP(INVALID_STATE) \
    ERROR_CODE_OP(INVALID_UNITIG_ID) \
    ERROR_CODE_OP(FAILED_TO_CLOSE_FILE) \
    ERROR_CODE_OP(FAILED_TO_CREATE_DIRECTORY) \
    ERROR_CODE_OP(FAILED_TO_GENERATE_UNIQUE_NAME) \
    ERROR_CODE_OP(FAILED_TO_GET_FILE_SIZE) \
    ERROR_CODE_OP(FAILED_TO_OPEN_FILE) \
    ERROR_CODE_OP(FILE_BAD_DATA) \
    ERROR_CODE_OP(FILE_DUPLICATE_DATA) \
    ERROR_CODE_OP(FILE_EMPTY) \
    ERROR_CODE_OP(FILE_WRONG_COLUMN_COUNT) \
    ERROR_CODE_OP(INDEX_OUT_OF_RANGE) \
    ERROR_CODE_OP(OPERATION_FOR_STOPPED_WORKER_POOL) \
    ERROR_CODE_OP(QUERY_COUNT_MISMATCH) \
    ERROR_CODE_OP(UNDEFINED_VALUE) \
    ERROR_CODE_OP(UNSIGNED_INTEGER_OVERFLOW)

// Expand ERROR_CODES into enum entries.
#define ERROR_CODE_OP(error_code_) error_code_,
enum class ErrorCode : std::uint8_t {
    ERROR_CODES

    end = UNSIGNED_INTEGER_OVERFLOW
};
#undef ERROR_CODE_OP

// Returns a description string for an error code.
std::string to_description(ErrorCode error_code);

// Converts an error code to a string.
std::string to_string(ErrorCode error_code);

// Custom exception class.
class Exception : public std::runtime_error {
public:
    // Constructs an exception with std::exception::what() from to_description(error_code).
    explicit Exception(ErrorCode error_code);

    /*
     * Constructs an exception with additional context added to std::exception::what(). All context arguments are
     * appended to the to_description(error_code) string inside parentheses after a space. For example:
     *     Exception(error_code, "expected ", 4, " fields")
     * appends " (expected 4 fields)" to the constructed std::exception::what() string.
    */
    template <typename... ContextArgs>
    explicit Exception(ErrorCode error_code, ContextArgs&&... context)
        : std::runtime_error(build_what(error_code, std::forward<ContextArgs>(context)...)),
          m_error_code{error_code}
    { }

    // Returns the stored error code.
    ErrorCode error_code() const noexcept;

private:
    ErrorCode m_error_code;

    // Appends additional context to the error description.
    template <typename... ContextArgs>
    static std::string build_what(ErrorCode error_code, ContextArgs&&... context) {
        using pack_expander = int[];

        std::ostringstream oss;

        oss << to_description(error_code) << " (";

        /*
         * Make use of the fact that a pack expansion may appear in brace-enclosed initializer lists to insert all
         * context data into the string stream.
        */
        (void)pack_expander{0, (oss << std::forward<ContextArgs>(context), 0)... };

        oss << ")";

        return oss.str();
    }
};

} // namespace PANGWES
