/*
 * ProgramOptions.hpp - Base class providing command-line argument parsing helpers.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace PANGWES {

/*
 * Base class providing helpers to inspect and read command-line options from `argc` and `argv`.
 *
 * Project-specific options classes should derive from this class and use the protected helpers to parse options.
*/
class ProgramOptions {
public:
    // Stores the command-line arguments provided to the main function.
    ProgramOptions(int argc, char** argv);

    virtual ~ProgramOptions() noexcept = default;

    // Returns the k-mer length, or 0 if not provided.
    virtual std::uint64_t k() const noexcept = 0;

    // Returns the number of threads to use, or 1 if not provided.
    virtual std::size_t n_threads() const noexcept = 0;

    // Returns the number of workers to initialize the worker pool with.
    virtual std::size_t n_workers() const noexcept = 0;

    // Returns true if verbose output was requested.
    virtual bool verbose() const noexcept = 0;

    // Returns true if the user supplied the "-h/--help" argument.
    virtual bool help_requested() const noexcept = 0;

    // Returns true if the user supplied the "-v/--version" argument.
    virtual bool version_requested() const noexcept = 0;

    // Prints a summary of this run's options (only if `verbose()` is true).
    virtual void print_run_details() const = 0;

protected:
    // Returns the begin pointer to `argv` (skips `argv[0]`).
    char** argv_begin() const noexcept;

    // Returns the end pointer to `argv`.
    char** argv_end() const noexcept;

    // Returns a pointer to the first occurrence of an option string in `argv`, or `argv_end()` if not found.
    char** argv_find(const std::string& program_option) const noexcept;

    // Returns true if the short or long option string is present.
    bool find_arg(const std::string& program_option, const std::string& program_option_long) const noexcept;

    /*
     * Reads and returns an unsigned integer option value if provided and valid. Otherwise, returns 0 if the option is
     * not present, and throws if the value is invalid.
    */
    std::uint64_t read_unsigned_value(const std::string& program_option, const std::string& program_option_long);

    // Reads and returns a string option's value if present. Otherwise, returns an empty string.
    std::string read_string_value(const std::string& program_option, const std::string& program_option_long);

    // Prints a formatted list of option descriptions. Intended to be called from `print_help()`.
    static void print_options_info(const std::vector<std::pair<std::string, std::string>>& options_info);

private:
    const int m_argc;
    char** const m_argv;

    /*
     * Returns a pointer to the value following the option string (the short option tried first), or `nullptr` if the
     * option is not present.
    */
    const char* find_arg_value(const std::string& program_option,
                               const std::string& program_option_long) const noexcept;

    // Prints a list of available options.
    virtual void print_help() const = 0;
};

} // namespace PANGWES
