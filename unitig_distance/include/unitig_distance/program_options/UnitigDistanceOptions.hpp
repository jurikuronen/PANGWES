/*
 * UnitigDistanceOptions.hpp - Reads command-line arguments specific to unitig_distance.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "common/program_options/ProgramOptions.hpp"
#include "common/program_options/program_options_option_names.hpp"
#include "unitig_distance/program_options/unitig_distance_option_names.hpp"

namespace PANGWES {

// Parses command-line arguments for unitig_distance and stores them as program options.
class UnitigDistanceOptions final : public ProgramOptions {
public:
    // Constructs the options by processing command-line arguments provided to the main function.
    UnitigDistanceOptions(int argc, char** argv);

    // Returns the path to the .unitigs file, or empty string if not provided.
    const std::string& unitigs_filename() const noexcept;

    // Returns the path to the queries file, or empty string if not provided.
    const std::string& queries_filename() const noexcept;

    // Returns the path to the SGG .paths filename, or empty string if not provided.
    const std::string& sgg_paths_filename() const noexcept;

    // Returns the output filename derived from the output stem (default stem: "out").
    std::string out_filename() const noexcept;

    // Returns the k-mer length, or 0 if not provided.
    std::uint64_t k() const noexcept override final;

    // Returns the number of queries to read, or a maximal value if not provided.
    std::size_t n_queries() const noexcept;

    // Returns the number of threads to use, or 1 if not provided.
    std::size_t n_threads() const noexcept override final;

    // Returns the number of workers to initialize the worker pool with.
    std::size_t n_workers() const noexcept override final;

    // Returns the maximum memory usage budget in bytes.
    std::size_t memory_bytes() const noexcept;

    // Returns true if median distance calculations should be skipped.
    bool no_median_distance() const noexcept;

    // Returns true if using one-based unitig numbering for the queries file.
    bool queries_one_based() const noexcept;

    // Returns true if using one-based unitig numbering in the output.
    bool output_one_based() const noexcept;

    // Returns true if verbose output was requested.
    bool verbose() const noexcept override final;

    // Returns true if the user supplied the "-h/--help" argument.
    bool help_requested() const noexcept override final;

    // Returns true if the user supplied the "-v/--version" argument.
    bool version_requested() const noexcept override final;

    // Prints a summary of this run's options (only if `verbose()` is true).
    void print_run_details() const override final;

private:
    std::string m_unitigs_filename;
    std::string m_queries_filename;
    std::string m_sgg_paths_filename;
    std::string m_out_stem;
    std::uint64_t m_k;
    std::size_t m_n_queries;
    std::size_t m_n_threads;
    std::size_t m_n_workers;
    std::size_t m_memory_bytes;
    bool m_no_median_distance;
    bool m_queries_one_based;
    bool m_output_one_based;
    bool m_verbose;

    bool m_help_requested;
    bool m_version_requested;

    // Prints a list of available options.
    void print_help() const override final;
};

} // namespace PANGWES
