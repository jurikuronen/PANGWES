/*
 * GFAParserOutputCoordinator.cpp - Class for coordinating gfa_parser output writes.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <algorithm>
#include <cstddef>
#include <mutex>
#include <ostream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "common/io/FileWriter.hpp"
#include "common/io/filesystem.hpp"
#include "common/utils/Exception.hpp"
#include "common/utils/memory.hpp"
#include "common/utils/WorkerPool.hpp"
#include "gfa_parser/core/Link.hpp"
#include "gfa_parser/core/Orientation.hpp"
#include "gfa_parser/core/ReferencePathData.hpp"
#include "gfa_parser/core/SegmentNameMap.hpp"
#include "gfa_parser/io/GFAParserOutputCoordinator.hpp"

namespace PANGWES {
namespace {

// If a directory with the given name already exists, append ".1", ".2", ..., until we get a unique directory name.
std::string make_unique_directory(const std::string& directory_name) {
    constexpr auto MAX_UNIQUE_DIRECTORY_TRIES = 256;

    if (Filesystem::directory_exists(directory_name)) {
        for (auto i = 1; i <= MAX_UNIQUE_DIRECTORY_TRIES; ++i) {
            const auto directory_candidate = directory_name + "." + std::to_string(i);

            if (!Filesystem::directory_exists(directory_candidate)) {
                if (!Filesystem::create_directory(directory_candidate)) {
                    throw Exception(ErrorCode::FAILED_TO_CREATE_DIRECTORY, directory_candidate);
                }
                return directory_candidate;
            }
        }

        throw Exception(ErrorCode::FAILED_TO_GENERATE_UNIQUE_NAME, " for ", directory_name);
    }

    if (!Filesystem::create_directory(directory_name)) {
        throw Exception(ErrorCode::FAILED_TO_CREATE_DIRECTORY, directory_name);
    }

    return directory_name;
}

/*
 * Constructs a pseudo-FASTA sequence from `unitig_occurrence_data` and appends it as a pseudo-sequence to the open
 * ".fasta" file.
*/
void append_fasta_entry(std::ostream& out,
                        const std::string& fasta_header,
                        const std::vector<bool>& unitig_occurrence_data)
{
    constexpr std::size_t FASTA_COLS = 80;

    out << '>' << fasta_header << '\n';

    for (std::size_t idx = 0; idx < unitig_occurrence_data.size(); ++idx) {
        out << (unitig_occurrence_data[idx] ? 'c' : 'a');

        if ((idx + 1) % FASTA_COLS == 0) {
            out << '\n';
        }
    }

    if (unitig_occurrence_data.size() % FASTA_COLS != 0) {
        out << '\n';
    }
}

} // namespace

GFAParserOutputCoordinator::GFAParserOutputCoordinator(const std::string& unitigs_filename,
                                                       const std::string& fasta_filename,
                                                       const std::string& paths_filename,
                                                       const std::string& edges_directory,
                                                       std::size_t n_reference_paths,
                                                       std::size_t expected_overlap)
    : m_unitigs_writer{},
      m_fasta_writer{},
      m_paths_writer{},
      m_resolved_unitigs_filename{},
      m_resolved_fasta_filename{},
      m_resolved_paths_filename{},
      m_resolved_edges_directory{},
      m_resolved_edges_filenames(n_reference_paths),
      m_edges_written(n_reference_paths, 0),
      m_expected_overlap(expected_overlap),
      m_fasta_headers(n_reference_paths),
      m_unitig_occurrence_data(n_reference_paths),
      m_current_unitig_occurrence_idx{},
      m_fasta_mutex{}
{
    try {
        m_unitigs_writer = Memory::make_unique<FileWriter>(unitigs_filename);
        m_resolved_unitigs_filename = m_unitigs_writer->filename();
        Log::out_without_date_block() << "INFO: Opened file \"" << m_resolved_unitigs_filename << "\" for writing."
                                      << std::endl;

        m_fasta_writer = Memory::make_unique<FileWriter>(fasta_filename);
        m_resolved_fasta_filename = m_fasta_writer->filename();
        Log::out_without_date_block() << "INFO: Opened file \"" << m_resolved_fasta_filename << "\" for writing."
                                      << std::endl;

        m_paths_writer = Memory::make_unique<FileWriter>(paths_filename);
        m_resolved_paths_filename = m_paths_writer->filename();
        Log::out_without_date_block() << "INFO: Opened file \"" << m_resolved_paths_filename << "\" for writing."
                                      << std::endl;

        m_resolved_edges_directory = make_unique_directory(edges_directory);
        Log::out_without_date_block() << "INFO: Created directory \"" << m_resolved_edges_directory << "\"."
                                      << std::endl;
    } catch (...) {
        remove_created_files();
        throw;
    }
}

const std::string& GFAParserOutputCoordinator::resolved_unitigs_filename() const noexcept {
    return m_resolved_unitigs_filename;
}

const std::string& GFAParserOutputCoordinator::resolved_fasta_filename() const noexcept {
    return m_resolved_fasta_filename;
}

const std::string& GFAParserOutputCoordinator::resolved_paths_filename() const noexcept {
    return m_resolved_paths_filename;
}

const std::string& GFAParserOutputCoordinator::resolved_edges_directory() const noexcept {
    return m_resolved_edges_directory;
}

void GFAParserOutputCoordinator::remove_created_files() {
    m_unitigs_writer.reset();
    m_fasta_writer.reset();
    m_paths_writer.reset();

    Filesystem::remove_file_if_exists(m_resolved_unitigs_filename);
    Filesystem::remove_file_if_exists(m_resolved_fasta_filename);
    Filesystem::remove_file_if_exists(m_resolved_paths_filename);

    for (std::size_t idx = 0; idx < m_edges_written.size(); ++idx) {
        if (m_edges_written[idx] == 1) {
            Filesystem::remove_file_if_exists(m_resolved_edges_filenames[idx]);
        }
    }

    Filesystem::remove_empty_directory_if_exists(m_resolved_edges_directory);
}

void GFAParserOutputCoordinator::post_write_unitigs_job(const SegmentNameMap& segment_name_map) {
    const JobT unitigs_writer_job = [this, &segment_name_map]() {
        for (std::size_t segment_mapping = 0; segment_mapping < segment_name_map.size(); ++segment_mapping) {
            m_unitigs_writer->out() << segment_mapping << ' '
                                    << segment_name_map.segment_sequence(segment_mapping) << '\n';
        }

        Log::out_without_date_block() << "INFO: Wrote unitigs to \"" << m_resolved_unitigs_filename << "\"."
                                      << std::endl;
    };

    WorkerPool::post({unitigs_writer_job});
}

void GFAParserOutputCoordinator::write_paths() {
    for (std::size_t idx = 0; idx < m_resolved_edges_filenames.size(); ++idx) {
        if (m_edges_written[idx] == 0) {
            throw Exception(ErrorCode::INVALID_STATE, "missing .edges file for reference path index ", idx);
        }

        m_paths_writer->out() << m_resolved_edges_filenames[idx] << '\n';
    }

    /*
     * Log also about SGG edges files, as calling this function is preceded by `WorkerPool::wait()`, which unblocks when
     * the last SGG edges file was  written.
    */
    Log::out_without_date_block() << "INFO: Wrote all SGG edge files to \"" << m_resolved_edges_directory << "\"."
                                  << std::endl;

    Log::out_without_date_block() << "INFO: Wrote SGG edge-file paths to \"" << m_resolved_paths_filename << "\"."
                                  << std::endl;
}

void GFAParserOutputCoordinator::write_reference_path_data(ReferencePathData& reference_path_data,
                                                           std::size_t reference_path_index)
{
    if (reference_path_data.links().size() == 0) {
        throw Exception(ErrorCode::EMPTY_DATA,
                        "reference path \"",
                        reference_path_data.string_for_fasta_header(),
                        "\" at index ",
                        reference_path_index,
                        " contains no valid edges");
    }

    // Create the ".edges" writing job before entering the FASTA-writing section on this thread.
    const JobT edges_writer_job = [this, &reference_path_data, reference_path_index]() {
        const auto to_sgg_orientation_char = [](Orientation orientation) {
            return orientation == Orientation::PLUS ? 'F' : 'R';
        };

        std::vector<std::tuple<std::size_t, std::size_t, char, char>> edges;

        for (const auto& link : reference_path_data.links()) {
            edges.emplace_back(link.from_id(),
                               link.to_id(),
                               to_sgg_orientation_char(link.from_orientation()),
                               to_sgg_orientation_char(link.to_orientation()));
        }
        reference_path_data.clear_and_release_reserved_memory();

        std::sort(edges.begin(), edges.end());

        // Convert path index to 1-based for the file name.
        auto edges_writer = Memory::make_unique<FileWriter>(m_resolved_edges_directory + "/" +
                                                            std::to_string(reference_path_index + 1) + ".edges");
        m_resolved_edges_filenames[reference_path_index] = edges_writer->filename();
        m_edges_written[reference_path_index] = 1;

        for (const auto& edge : edges) {
            edges_writer->out() << std::get<0>(edge) << ' '
                                << std::get<1>(edge) << ' '
                                << std::get<2>(edge) << std::get<3>(edge) << ' '
                                << m_expected_overlap << '\n';
        }
    };

    WorkerPool::post({edges_writer_job});

    // Take the data required to construct a pseudo-FASTA data entry for this ReferencePathData.
    std::lock_guard<std::mutex> lock(m_fasta_mutex);
    m_fasta_headers[reference_path_index] = reference_path_data.string_for_fasta_header();
    m_unitig_occurrence_data[reference_path_index] = reference_path_data.take_unitig_occurrence_data();

    // If we were waiting for this `reference_path_index`, prepare and append all pseudo-FASTA sequences in order.
    while (m_current_unitig_occurrence_idx < m_unitig_occurrence_data.size() &&
           !m_unitig_occurrence_data[m_current_unitig_occurrence_idx].empty())
    {
        append_fasta_entry(m_fasta_writer->out(),
                           m_fasta_headers[m_current_unitig_occurrence_idx],
                           m_unitig_occurrence_data[m_current_unitig_occurrence_idx]);

        // Release memory.
        Memory::clear_and_release_reserved_memory(m_fasta_headers[m_current_unitig_occurrence_idx]);
        Memory::clear_and_release_reserved_memory(m_unitig_occurrence_data[m_current_unitig_occurrence_idx]);

        ++m_current_unitig_occurrence_idx;
    }

    if (m_current_unitig_occurrence_idx == m_unitig_occurrence_data.size()) {
        Log::out_without_date_block() << "INFO: Wrote all FASTA output to \"" << m_resolved_fasta_filename << "\"."
                                      << std::endl;
    }
}

} // namespace PANGWES
