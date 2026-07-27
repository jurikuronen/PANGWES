/*
 * program_options_option_names.hpp - Named constants for common program options.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

namespace PANGWES {

// Output file name related defaults.
constexpr auto DEFAULT_OUT_STEM                  = "out";
constexpr auto ZERO_BASED_STR                    = "_0_based";
constexpr auto ONE_BASED_STR                     = "_1_based";

// Common shared program options.
constexpr auto OUT_STEM_OPTION                   = "-o";
constexpr auto OUT_STEM_LONG_OPTION              = "--output-stem";
constexpr auto K_OPTION                          = "-k";
constexpr auto K_LONG_OPTION                     = "--k-mer-length";
constexpr auto N_THREADS_OPTION                  = "-t";
constexpr auto N_THREADS_LONG_OPTION             = "--threads";
constexpr auto HELP_OPTION                       = "-h";
constexpr auto HELP_LONG_OPTION                  = "--help";
constexpr auto QUIET_OPTION                      = "-q";
constexpr auto QUIET_LONG_OPTION                 = "--quiet";
constexpr auto VERSION_OPTION                    = "-v";
constexpr auto VERSION_LONG_OPTION               = "--version";

} // namespace PANGWES
