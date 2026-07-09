/*
 * GFAParserOptions.hpp - Reads command-line arguments specific to gfa_parser.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "common/program_options/ProgramOptions.hpp"
#include "gfa_parser/core/GFAFormat.hpp"

namespace PANGWES {

// Parses command-line arguments for gfa_parser and stores them as program options.
class GFAParserOptions final : public ProgramOptions {
public:
    // Constructs the options by processing command-line arguments provided to the main function.
    GFAParserOptions(int argc, char** argv);

    // Returns the path to the GFA file, or empty string if not provided.
    const std::string& gfa_filename() const noexcept;

    // Returns the GFA format specification version of the GFA file (default: `GFAFormat::GFA1`).
    GFAFormat gfa_format() const noexcept;

    // Returns the output ".unitigs" filename derived from the output stem (default: "out.unitigs").
    std::string out_unitigs_filename() const noexcept;

    // Returns the output ".fasta" filename derived from the output stem (default: "out.fasta").
    std::string out_fasta_filename() const noexcept;

    // Returns the output SGG ".paths" filename derived from the output stem (default: "out.paths").
    std::string out_sgg_paths_filename() const noexcept;

    // Returns the output SGG paths directory name derived from the output stem (default: "out_paths").
    std::string out_sgg_paths_directory() const noexcept;

    // Returns the k-mer length, or 0 if not provided.
    std::uint64_t k() const noexcept override final;

    // Returns the number of threads to use, or 1 if not provided.
    std::size_t n_threads() const noexcept override final;

    // Returns the number of workers to initialize the worker pool with.
    std::size_t n_workers() const noexcept override final;

    // Returns true if verbose output was requested.
    bool verbose() const noexcept override final;

    // Returns true if the user supplied the "-h/--help" argument.
    bool help_requested() const noexcept override final;

    // Returns true if the user supplied the "-v/--version" argument.
    bool version_requested() const noexcept override final;

    // Prints a summary of this run's options (only if `verbose()` is true).
    void print_run_details() const override final;

private:
    std::string m_gfa_filename;
    std::string m_out_stem;
    GFAFormat m_gfa_format;
    std::uint64_t m_k;
    std::size_t m_n_threads;
    std::size_t m_n_workers;
    bool m_verbose;

    bool m_help_requested;
    bool m_version_requested;

    // Prints a list of available options.
    void print_help() const override final;
};

} // namespace PANGWES
