/*
 * GFAParserOutputCoordinator.hpp - Class for coordinating gfa_parser output writes.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "common/io/FileWriterInterface.hpp"

namespace PANGWES {

// Forward declarations.
class ReferencePathData;
class SegmentNameMap;

/*
 * Class for coordinating gfa_parser's output creation from data provided by GFAData.
 *
 * Controls the writers for all the files and tracks created directories. In case an error occurs, all partially created
 * directories are removed.
*/
class GFAParserOutputCoordinator {
public:
    /*
     * Constructs output writers for ".unitigs", ".fasta" and ".paths", which already creates and opens the files.
     * Initializes internal data for writing the ".edges" files.
     *
     * Throws if constructing any output writer failed and removes the already-opened empty files.
    */
    GFAParserOutputCoordinator(const std::string& unitigs_filename,
                               const std::string& fasta_filename,
                               const std::string& paths_filename,
                               const std::string& edges_directory,
                               std::size_t n_reference_paths,
                               std::size_t expected_overlap);

    GFAParserOutputCoordinator(const GFAParserOutputCoordinator&) = delete;
    GFAParserOutputCoordinator& operator=(const GFAParserOutputCoordinator&) = delete;
    GFAParserOutputCoordinator(GFAParserOutputCoordinator&&) = delete;
    GFAParserOutputCoordinator& operator=(GFAParserOutputCoordinator&&) = delete;

    // Returns the resolved ".unitigs" output filename.
    const std::string& resolved_unitigs_filename() const noexcept;

    // Returns the resolved ".fasta" output filename.
    const std::string& resolved_fasta_filename() const noexcept;

    // Returns the resolved ".paths" output filename.
    const std::string& resolved_paths_filename() const noexcept;

    // Returns the resolved output directory of the ".edges" files.
    const std::string& resolved_edges_directory() const noexcept;

    // Closes output writers and removes any files created so far. Called in cases where an error occurred.
    void remove_created_files();

    // Posts a job that writes the ".unitigs" output.
    void post_write_unitigs_job(const SegmentNameMap& segment_name_map);

    // Writes the ".paths" output from the resolved ".edges" filenames.
    void write_paths();

    /*
     * Posts a job that writes the ".edges" output using `reference_path_data`'s links and finally clears and
     * releases the memory reserved by the links. Next, takes the data required to construct a pseudo-FASTA entry for
     * `reference_path_data` and stores them in this GFAParserOutputCoordinator.
     *
     * Afterwards, checks if we were waiting for this `reference_path_index` and prepares and appends all ready
     * pseudo-FASTA sequence entries to the open ".fasta" file.
    */
    void write_reference_path_data(ReferencePathData& reference_path_data, std::size_t reference_path_index);

private:
    std::unique_ptr<FileWriterInterface> m_unitigs_writer;
    std::unique_ptr<FileWriterInterface> m_fasta_writer;
    std::unique_ptr<FileWriterInterface> m_paths_writer;
    std::string m_resolved_unitigs_filename;
    std::string m_resolved_fasta_filename;
    std::string m_resolved_paths_filename;
    std::string m_resolved_edges_directory;
    std::vector<std::string> m_resolved_edges_filenames;
    std::vector<std::uint8_t> m_edges_written;
    std::size_t m_expected_overlap;
    std::vector<std::string> m_fasta_headers;
    std::vector<std::vector<bool>> m_unitig_occurrence_data;
    std::size_t m_current_unitig_occurrence_idx;
    std::mutex m_fasta_mutex;
};

} // namespace PANGWES
