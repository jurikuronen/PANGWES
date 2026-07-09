/*
 * QueriesReader.hpp - Reader for unitig_distance queries.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>

#include "common/io/FileReader.hpp"

namespace PANGWES {

/*
 * Wrapper class over FileReader to read unitig_distance queries from an input file.
 *
 * It is expected that the input file's lines follow SpydrPick's output format:
 *     [unitig1_id unitig2_id genome_distance ARACNE MI].
 *
 * As only unitig id fields are used by this program, the other data are provided only for writing back into the output,
 * with the exception of `genome_distance` that will be replaced by the mean shortest-path distance for the unitig pair
 * across the single-genome graphs computed by this program.
*/
class QueriesReader {
public:
    // Data parsed from one queries-file line.
    struct QueryData {
        std::size_t unitig1_id;
        std::size_t unitig2_id;
        std::string aracne_flag_and_mi_score_fields;
    };

    // Initializes the queries reader.
    explicit QueriesReader(std::unique_ptr<FileReaderInterface> reader,
                           std::size_t n_queries = std::numeric_limits<std::size_t>::max(),
                           bool queries_one_based = false);

    QueriesReader(QueriesReader&&) noexcept = default;
    QueriesReader& operator=(QueriesReader&&) noexcept = default;

    // Opens the queries file for reading. Throws if opening the file fails.
    void start_reading();

    // Closes the file reader. Throws if the file stream was not open.
    void stop_reading();

    /*
     * Attempts to read lines until the next valid query line and fills `data`. Returns true on success, false if at the
     * end of the file or after reading `n_queries` queries. Throws on any errors.
    */
    bool getquery(QueryData& data);

    // Returns the name of the stored queries file.
    std::string filename() const noexcept;

    // Returns the number of queries read.
    std::size_t n_queries_read() const noexcept;

    // Returns the 1-based count of lines read so far.
    std::size_t line_number() const noexcept;

private:
    std::unique_ptr<FileReaderInterface> m_reader;
    std::size_t m_n_queries;
    std::int64_t m_unitig_index_base_offset;
    std::size_t m_n_queries_read;
};

} // namespace PANGWES
