/*
 * QueriesReader.cpp - Reader for unitig_distance queries.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <utility>

#include "common/io/Log.hpp"
#include "common/utils/Exception.hpp"
#include "common/utils/format.hpp"
#include "common/utils/memory.hpp"
#include "common/utils/utils.hpp"
#include "unitig_distance/io/QueriesReader.hpp"

namespace PANGWES {
namespace {

// Only queries formatted like SpydrPick output are supported.
constexpr auto N_REQUIRED_SPYDRPICK_FIELDS = 5;

constexpr auto UNITIG1_ID_FIELD = 0;
constexpr auto UNITIG2_ID_FIELD = 1;
constexpr auto ARACNE_FLAG_FIELD = 3;
constexpr auto SCORE_FIELD = 4;

} // namespace

QueriesReader::QueriesReader(std::unique_ptr<FileReaderInterface> reader,
                             std::size_t n_queries,
                             bool queries_one_based)
    : m_reader{std::move(reader)},
      m_n_queries{n_queries},
      m_unitig_index_base_offset{static_cast<std::int64_t>(queries_one_based)},
      m_n_queries_read{}
{ }

void QueriesReader::start_reading() {
    m_reader->open();
    m_n_queries_read = 0;

    Log::out_without_date_block() << "INFO: Reading queries." << std::endl;
}

void QueriesReader::stop_reading() {
    m_reader->close();

    Log::out_without_date_block() << "INFO: Read " << Format::pretty_uint(m_n_queries_read) << " queries." << std::endl;
}

bool QueriesReader::getquery(QueryData& data) {
    // Reached the user-defined maximum number of queries to read.
    if (m_n_queries_read >= m_n_queries) {
        return false;
    }

    std::string line{};

    // Read until next non-empty line or end-of-file.
    while (m_reader->getline(line)) {
        if (!line.empty()) {
            break;
        }
    }

    if (line.empty()) {
        return false;
    }

    auto fields = Utils::get_fields_ws(line);

    // Queries must be formatted like SpydrPick output.
    if (fields.size() < N_REQUIRED_SPYDRPICK_FIELDS) {
        throw Exception(ErrorCode::FILE_WRONG_COLUMN_COUNT, "queries file line ", line_number());
    }

    // Read the IDs as signed integers for validity checking.
    std::int64_t unitig1_id{};
    std::int64_t unitig2_id{};

    try {
        unitig1_id = std::stoll(fields[UNITIG1_ID_FIELD]);
        unitig2_id = std::stoll(fields[UNITIG2_ID_FIELD]);
    } catch (...) {
        throw Exception(ErrorCode::FILE_BAD_DATA, "invalid unitig ids on queries file line ", line_number());
    }

    if (unitig1_id < m_unitig_index_base_offset || unitig2_id < m_unitig_index_base_offset) {
        throw Exception(ErrorCode::INVALID_UNITIG_ID, "queries file line ", line_number());
    }

    if (unitig1_id == unitig2_id) {
        throw Exception(ErrorCode::FILE_DUPLICATE_DATA, "unitig id pair on queries file line ", line_number());
    }

    data.unitig1_id = static_cast<std::size_t>(unitig1_id - m_unitig_index_base_offset);
    data.unitig2_id = static_cast<std::size_t>(unitig2_id - m_unitig_index_base_offset);
    data.aracne_flag_and_mi_score_fields = std::move(fields[ARACNE_FLAG_FIELD]) + " " + std::move(fields[SCORE_FIELD]);

    ++m_n_queries_read;

    return true;
}

std::string QueriesReader::filename() const noexcept {
    return m_reader->filename();
}

std::size_t QueriesReader::n_queries_read() const noexcept {
    return m_n_queries_read;
}

std::size_t QueriesReader::line_number() const noexcept {
    return m_reader ? m_reader->line_number() : 0;
}

} // namespace PANGWES
