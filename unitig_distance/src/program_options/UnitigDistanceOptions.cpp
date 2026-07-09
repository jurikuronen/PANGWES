/*
 * UnitigDistanceOptions.cpp - Reads command-line arguments specific to unitig_distance.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <cctype>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "common/io/Log.hpp"
#include "common/utils/format.hpp"
#include "common/utils/Exception.hpp"
#include "common/utils/memory.hpp"
#include "common/utils/utils.hpp"
#include "unitig_distance/program_options/UnitigDistanceOptions.hpp"

namespace PANGWES {
namespace {

std::size_t parse_memory_string(std::string memory_string) {
    if (memory_string.empty()) {
        // Default value when the memory argument was not provided.
        return Memory::GiB * 20;
    }

    char memory_suffix = memory_string.back();

    // Suffix provided case.
    if (std::isalpha(memory_suffix) != 0) {
        if (memory_suffix != 'M' && memory_suffix != 'G') {
            throw Exception(ErrorCode::INVALID_PROGRAM_OPTION, "invalid suffix in --memory argument: ", memory_string);
        }
        memory_string.pop_back();
    // No suffix: default to GiB.
    } else {
        memory_suffix = 'G';
    }

    std::size_t memory_value{};

    try {
        memory_value = static_cast<std::size_t>(Utils::parse_unsigned_value(memory_string));
    } catch (...) {
        throw Exception(ErrorCode::INVALID_PROGRAM_OPTION, "invalid value in --memory argument: ", memory_string);
    }

    const auto memory_multiplier = (memory_suffix == 'M') ? Memory::MiB : Memory::GiB;

    if (memory_value > std::numeric_limits<std::size_t>::max() / memory_multiplier) {
        throw Exception(ErrorCode::INVALID_PROGRAM_OPTION, "too large value in --memory argument: ", memory_string);
    }

    return memory_value * memory_multiplier;
}

} // namespace

UnitigDistanceOptions::UnitigDistanceOptions(int argc, char** argv)
    : ProgramOptions(argc, argv),
      m_unitigs_filename{},
      m_queries_filename{},
      m_sgg_paths_filename{},
      m_out_stem{},
      m_k{},
      m_n_queries{},
      m_n_threads{},
      m_n_workers{},
      m_memory_bytes{},
      m_no_median_distance{},
      m_queries_one_based{},
      m_output_one_based{},
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
     * and unitig_distance_option_names.hpp.
    */
    m_unitigs_filename = read_string_value(UNITIGS_FILENAME_OPTION, UNITIGS_FILENAME_LONG_OPTION);
    m_queries_filename = read_string_value(QUERIES_FILENAME_OPTION, QUERIES_FILENAME_LONG_OPTION);
    m_sgg_paths_filename = read_string_value(SGG_PATHS_FILENAME_OPTION, SGG_PATHS_FILENAME_LONG_OPTION);
    m_out_stem = read_string_value(OUT_STEM_OPTION, OUT_STEM_LONG_OPTION);

    m_k = read_unsigned_value(K_OPTION, K_LONG_OPTION);
    m_n_queries = read_unsigned_value(N_QUERIES_OPTION, N_QUERIES_LONG_OPTION);
    m_n_threads = read_unsigned_value(N_THREADS_OPTION, N_THREADS_LONG_OPTION);

    m_memory_bytes = parse_memory_string(read_string_value(MEMORY_OPTION, MEMORY_LONG_OPTION));

    m_no_median_distance = find_arg(NO_MEDIAN_OPTION, NO_MEDIAN_LONG_OPTION);

    m_queries_one_based = find_arg(QUERIES_ONE_BASED_OPTION, QUERIES_ONE_BASED_LONG_OPTION);
    m_output_one_based = find_arg(OUTPUT_ONE_BASED_OPTION, OUTPUT_ONE_BASED_LONG_OPTION);

    // Set verbose to false if the user supplied the "quiet" argument.
    m_verbose = !find_arg(QUIET_OPTION, QUIET_LONG_OPTION);

    // Check queries argument and set to default value if not provided or invalid (0).
    if (m_n_queries == 0) {
        m_n_queries = std::numeric_limits<std::size_t>::max();
    }

    // Check threads argument and set to default value if not provided or invalid (0).
    if (m_n_threads == 0) {
        m_n_threads = 1;
    }

    /*
     * For unitig_distance, the main thread mostly waits while workers are working, so the pool should use n_threads.
     * The exception is if n_threads == 1: then n_workers 0 is correct to enable CALLER_THREAD mode.
    */
    m_n_workers = (m_n_threads == 1) ? 0 : m_n_threads;
}

const std::string& UnitigDistanceOptions::unitigs_filename() const noexcept {
    return m_unitigs_filename;
}

const std::string& UnitigDistanceOptions::queries_filename() const noexcept {
    return m_queries_filename;
}

const std::string& UnitigDistanceOptions::sgg_paths_filename() const noexcept {
    return m_sgg_paths_filename;
}

std::string UnitigDistanceOptions::out_filename() const noexcept {
    return (m_out_stem.empty() ? DEFAULT_OUT_STEM : m_out_stem) +
           UD_OUT_FILE_EXTENSION +
           (m_output_one_based ? ONE_BASED_STR : ZERO_BASED_STR);
}

std::uint64_t UnitigDistanceOptions::k() const noexcept {
    return m_k;
}

std::size_t UnitigDistanceOptions::n_queries() const noexcept {
    return m_n_queries;
}

std::size_t UnitigDistanceOptions::n_threads() const noexcept {
    return m_n_threads;
}

std::size_t UnitigDistanceOptions::n_workers() const noexcept {
    return m_n_workers;
}

std::size_t UnitigDistanceOptions::memory_bytes() const noexcept {
    return m_memory_bytes;
}

bool UnitigDistanceOptions::no_median_distance() const noexcept {
    return m_no_median_distance;
}

bool UnitigDistanceOptions::queries_one_based() const noexcept {
    return m_queries_one_based;
}

bool UnitigDistanceOptions::output_one_based() const noexcept {
    return m_output_one_based;
}

bool UnitigDistanceOptions::verbose() const noexcept {
    return m_verbose;
}

bool UnitigDistanceOptions::help_requested() const noexcept {
    return m_help_requested;
}

bool UnitigDistanceOptions::version_requested() const noexcept {
    return m_version_requested;
}

void UnitigDistanceOptions::print_run_details() const {
    if (!m_verbose) {
        return;
    }

    constexpr auto run_details_left_width = 30;

    std::vector<std::pair<std::string, std::string>> options{
        {"  --unitigs-file", m_unitigs_filename},
        {"  --k-mer-length", std::to_string(m_k)},
        {"  --sgg-paths-file", m_sgg_paths_filename},
        {"  --queries-file", m_queries_filename},
        {"  --queries-one-based", m_queries_one_based ? "TRUE" : "FALSE"},
        {"  --n-queries", m_n_queries == std::numeric_limits<std::size_t>::max() ? "ALL" : std::to_string(m_n_queries)},
        {"  --no-median-distance", m_no_median_distance ? "TRUE" : "FALSE"},
        {"  --output-stem", m_out_stem.empty() ? DEFAULT_OUT_STEM : m_out_stem},
        {"  --output-one-based", m_output_one_based ? "TRUE" : "FALSE"},
        {"  --memory", Format::pretty_uint(Memory::bytes_to_mebibytes(m_memory_bytes)) + " MiB" },
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
void UnitigDistanceOptions::print_help() const {
    std::vector<std::pair<std::string, std::string>> options_info{
        {"Compacted de Bruijn graph (cdBG) options:", ""},
        {"    -U, --unitigs-file PATH", "Path to the .unitigs file containing global cdBG unitigs."},
        {"", ""},
        {"    -k, --k-mer-length K", "The k-mer length from which the cdBG unitigs were derived."},
        {"", ""},
        {"    -S, --sgg-paths-file PATH", "Path to the .paths file listing single-genome graph edge-list files."},
        {"", ""},
        {"Distance query options:", ""},
        {"    -Q, --queries-file PATH", "Path to a SpydrPick-format queries file. Only the unitig ID columns are "
                                        "used as input; the remaining fields are preserved in the output."},
        {"", ""},
        {"    -1q, --queries-one-based", "Use one-based unitig numbering for the queries file."},
        {"", ""},
        {"    -n, --n-queries N", "Number of queries to read from the queries file (default: all)."},
        {"", ""},
        {"    -x, --no-median-distance", "Disable median shortest-path distance calculation. The MEDIAN_DISTANCE "
                                         "output field is set to -1."},
        {"", ""},
        {"Other options:", ""},
        {"    -o, --output-stem STEM", "Output filename stem (default: out). The program writes STEM.ud_0_based or "
                                       "STEM.ud_1_based. If that filename already exists, a unique numeric "
                                       "suffix is appended to avoid collisions."},
        {"", ""},
        {"    -1o, --output-one-based", "Use one-based unitig numbering in the output file."},
        {"", ""},
        {"    -t, --threads N", "Number of threads (default: 1)."},
        {"", ""},
        {"    -m, --memory SIZE", "Maximum memory usage (default: 20G). Supports M (MiB) and G (GiB) suffixes. "
                                  "Values without a suffix are interpreted as GiB."},
        {"", ""},
        {"    -q, --quiet", "Suppress non-error messages."},
        {"", ""},
        {"    -h, --help", "Print this list."},
        {"", ""},
        {"    -v, --version", "Prints version information and exits."},
    };

    // Use `std::cout` instead of Log: this list must always be printed when requested regardless of verbosity.
    std::cout << "unitig_distance - computes shortest-path distances between unitig pairs across single-genome"
              << std::endl;
    std::cout << "                  graphs (SGGs), i.e. the colors of a compacted de Bruijn graph (cdBG)."
              << std::endl << std::endl;

    std::cout << "Usage:" << std::endl;
    std::cout << "  unitig_distance [options] -U <unitigs_file> -k <kmer_length> -S <sgg_paths_file> -Q <queries_file>"
              << std::endl << std::endl;

    ProgramOptions::print_options_info(options_info);
}

} // namespace PANGWES
