/*
 * Exception.cpp - Custom exception class.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <cassert>

#include "common/utils/Exception.hpp"
#include "common/type_traits/type_traits.hpp"

namespace PANGWES {
namespace {

// Expand ERROR_CODES into error-code name strings.
#define ERROR_CODE_OP(error_code_) #error_code_,
constexpr const char* const error_code_str[] = {
    ERROR_CODES
};
#undef ERROR_CODE_OP

} // namespace

Exception::Exception(ErrorCode error_code)
    : std::runtime_error(to_description(error_code)),
      m_error_code{error_code}
{ }

ErrorCode Exception::error_code() const noexcept {
    return m_error_code;
}

std::string to_description(ErrorCode error_code) {
    assert(Traits::to_underlying(error_code) <= Traits::to_underlying(ErrorCode::end) && "Unknown error code");

    switch (error_code) {
        case ErrorCode::EMPTY_DATA:                         return "Empty data";
        case ErrorCode::EMPTY_FILENAME:                     return "No filename provided";
        case ErrorCode::INVALID_ARGUMENT:                   return "Invalid argument to function";
        case ErrorCode::INVALID_CIGAR_OPERATION:            return "Invalid CIGAR operation";
        case ErrorCode::INVALID_DATA:                       return "Invalid data format";
        case ErrorCode::INVALID_GFA_FORMAT:                 return "Invalid GFA format";
        case ErrorCode::INVALID_KMER_LENGTH:                return "Invalid k-mer length";
        case ErrorCode::INVALID_PROGRAM_OPTION:             return "Invalid program option";
        case ErrorCode::INVALID_RECORD_TYPE:                return "Invalid record type";
        case ErrorCode::INVALID_STATE:                      return "Invalid state";
        case ErrorCode::INVALID_UNITIG_ID:                  return "Invalid unitig ID";
        case ErrorCode::FAILED_TO_CLOSE_FILE:               return "Failed to close file";
        case ErrorCode::FAILED_TO_CREATE_DIRECTORY:         return "Failed to create directory";
        case ErrorCode::FAILED_TO_GENERATE_UNIQUE_NAME:     return "Failed to generate unique name";
        case ErrorCode::FAILED_TO_GET_FILE_SIZE:            return "Failed to get file size";
        case ErrorCode::FAILED_TO_OPEN_FILE:                return "Failed to open file";
        case ErrorCode::FILE_BAD_DATA:                      return "Unexpected data format in file";
        case ErrorCode::FILE_DUPLICATE_DATA:                return "Duplicate data in file";
        case ErrorCode::FILE_EMPTY:                         return "Empty file";
        case ErrorCode::FILE_WRONG_COLUMN_COUNT:            return "Wrong column count in file";
        case ErrorCode::INDEX_OUT_OF_RANGE:                 return "Index out of range";
        case ErrorCode::OPERATION_FOR_STOPPED_WORKER_POOL:  return "Attempted to use a stopped worker pool";
        case ErrorCode::QUERY_COUNT_MISMATCH:               return "Query count mismatch";
        case ErrorCode::UNDEFINED_VALUE:                    return "Undefined value";
        case ErrorCode::UNSIGNED_INTEGER_OVERFLOW:          return "Unsigned integer overflow";
        default: break;
    }

    return "Unknown error code";
}

std::string to_string(ErrorCode error_code) {
    assert(Traits::to_underlying(error_code) <= Traits::to_underlying(ErrorCode::end) && "Unknown error code");

    return error_code_str[Traits::to_underlying(error_code)];
}

} // namespace PANGWES
