/*
 * GFAParserOptions.cpp - Reads command-line arguments specific to gfa_parser.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "common/io/Log.hpp"
#include "common/program_options/program_options_option_names.hpp"
#include "common/utils/Exception.hpp"
#include "gfa_parser/core/GFAFormat.hpp"
#include "gfa_parser/program_options/GFAParserOptions.hpp"
#include "gfa_parser/program_options/gfa_parser_option_names.hpp"

namespace PANGWES {

GFAParserOptions::GFAParserOptions(int argc, char** argv)
    : ProgramOptions(argc, argv),
      m_gfa_filename{},
      m_out_stem{},
      m_gfa_format{GFAFormat::GFA1},
      m_k{},
      m_n_threads{},
      m_n_workers{},
      m_verbose{},
      m_help_requested{},
      m_version_requested{}
{
    // Return early after printing help if the user supplied the "help" argument or didn't provide any arguments.
    if (argc <= 1 || find_arg(HELP_OPTION, HELP_LONG_OPTION)) {
        print_help();
        m_help_requested = true;
        return;
    }

    // Return early if the user supplied the "version" argument.
    if (find_arg(VERSION_OPTION, VERSION_LONG_OPTION)) {
        m_version_requested = true;
        return;
    }

    /*
     * Set option values given in the arguments. Searched option strings are defined in program_options_option_names.hpp
     * and gfa_parser_option_names.hpp.
    */
    m_gfa_filename = read_string_value(GFA_FILENAME_OPTION, GFA_FILENAME_LONG_OPTION);
    m_out_stem = read_string_value(OUT_STEM_OPTION, OUT_STEM_LONG_OPTION);

    // GFA Format is given as an integral value.
    std::uint64_t gfa_format_uint = read_unsigned_value(GFA_FORMAT_OPTION, GFA_FORMAT_LONG_OPTION);

    if (gfa_format_uint != 0) {
        try {
            m_gfa_format = to_gfa_format(gfa_format_uint);
        } catch (...) {
            throw Exception(ErrorCode::INVALID_PROGRAM_OPTION, "GFA format out of range: ", gfa_format_uint);
        }
    }

    m_k = read_unsigned_value(K_OPTION, K_LONG_OPTION);
    m_n_threads = read_unsigned_value(N_THREADS_OPTION, N_THREADS_LONG_OPTION);

    // Check threads argument and set to default value if not provided or invalid (0).
    if (m_n_threads == 0) {
        m_n_threads = 1;
    }

    // gfa_parser utilizes the main thread extensively, so the pool should be initialized with -1 worker.
    m_n_workers = m_n_threads - 1;

    // Set verbose to false if the user supplied the "quiet" argument.
    m_verbose = !find_arg(QUIET_OPTION, QUIET_LONG_OPTION);
}

const std::string& GFAParserOptions::gfa_filename() const noexcept {
    return m_gfa_filename;
}

GFAFormat GFAParserOptions::gfa_format() const noexcept {
    return m_gfa_format;
}

std::string GFAParserOptions::out_unitigs_filename() const noexcept {
    return (m_out_stem.empty() ? DEFAULT_OUT_STEM : m_out_stem) + UNITIGS_OUT_FILE_EXTENSION;
}

std::string GFAParserOptions::out_fasta_filename() const noexcept {
    return (m_out_stem.empty() ? DEFAULT_OUT_STEM : m_out_stem) + FASTA_OUT_FILE_EXTENSION;
}

std::string GFAParserOptions::out_sgg_paths_filename() const noexcept {
    return (m_out_stem.empty() ? DEFAULT_OUT_STEM : m_out_stem) + SGG_PATHS_OUT_FILE_EXTENSION;
}

std::string GFAParserOptions::out_sgg_paths_directory() const noexcept {
    return (m_out_stem.empty() ? DEFAULT_OUT_STEM : m_out_stem) + SGG_PATHS_DIRECTORY_SUFFIX;
}

std::uint64_t GFAParserOptions::k() const noexcept {
    return m_k;
}

std::size_t GFAParserOptions::n_threads() const noexcept {
    return m_n_threads;
}

std::size_t GFAParserOptions::n_workers() const noexcept {
    return m_n_workers;
}

bool GFAParserOptions::verbose() const noexcept {
    return m_verbose;
}

bool GFAParserOptions::help_requested() const noexcept {
    return m_help_requested;
}

bool GFAParserOptions::version_requested() const noexcept {
    return m_version_requested;
}

void GFAParserOptions::print_run_details() const {
    if (!m_verbose) {
        return;
    }

    constexpr auto run_details_left_width = 30;

    std::vector<std::pair<std::string, std::string>> options{
        {"  --gfa-file", m_gfa_filename},
        {"  --gfa-format", to_string(m_gfa_format)},
        {"  --output-stem", m_out_stem},
        {"  --k-mer-length", std::to_string(m_k)},
        {"  --threads", std::to_string(m_n_threads)},
    };

    Log::out_without_date_block() << "\nUsing the following options:" << std::endl;

    for (const auto& option : options) {
        const auto& option_name = option.first;
        const auto& option_value = option.second;

        Log::out_without_date_block() << std::left << std::setw(run_details_left_width) << option_name << ' '
                                      << option_value << std::endl;
    }

    Log::out_without_date_block() << std::endl;
}

// Prints a list of available options.
void GFAParserOptions::print_help() const {
    std::vector<std::pair<std::string, std::string>> options_info{
        {"Graphical Fragment Assembly (GFA) format options:", ""},
        {"    -G, --gfa-file PATH", "Path to the input GFA file."},
        {"", ""},
        {"    -F, --gfa-format FORMAT", "GFA format version as an integer (default: 1). Only GFA 1.0 is currently "
                                        "supported."},
        {"", ""},
        {"    -k, --k-mer-length K", "The k-mer length used in constructing the input GFA file. "
                                     "Used for overlap validation."},
        {"", ""},
        {"Other options:", ""},
        {"    -o, --output-stem STEM", "Output filename stem (default: out). The program writes STEM.unitigs, "
                                       "STEM.fasta, STEM.paths and SGG edge lists under STEM_paths/*.edges. "
                                       "If an output filename already exists, a unique numeric suffix is appended to "
                                       "avoid collisions."},
        {"", ""},
        {"    -t, --threads N", "Number of threads (default: 1)."},
        {"", ""},
        {"    -q, --quiet", "Suppress non-error messages."},
        {"", ""},
        {"    -h, --help", "Print this list."},
        {"", ""},
        {"    -v, --version", "Prints version information and exits."},
    };

    // Use `std::cout` instead of Log: this list must always be printed when requested regardless of verbosity.
    std::cout << "gfa_parser - parses GFA files that encode compacted de Bruijn graphs (cdBGs) constructed from"
              << std::endl;
    std::cout << "             reference genomes. Creates input files suitable for SpydrPick and unitig_distance."
              << std::endl << std::endl;

    std::cout << "Usage:" << std::endl;
    std::cout << "  gfa_parser [options] -G <gfa_file>"
              << std::endl << std::endl;

    ProgramOptions::print_options_info(options_info);
}

} // namespace PANGWES
