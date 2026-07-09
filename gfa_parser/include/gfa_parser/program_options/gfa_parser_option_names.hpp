/*
 * gfa_parser_option_names.hpp - Named constants used by GFAParserOptions.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

namespace PANGWES {

// Output file extensions for gfa_parser.
constexpr auto UNITIGS_OUT_FILE_EXTENSION               = ".unitigs";
constexpr auto FASTA_OUT_FILE_EXTENSION                 = ".fasta";
constexpr auto SGG_PATHS_OUT_FILE_EXTENSION             = ".paths";
constexpr auto SGG_PATHS_DIRECTORY_SUFFIX               = "_paths";

// Program options.
constexpr auto GFA_FILENAME_OPTION                       = "-G";
constexpr auto GFA_FILENAME_LONG_OPTION                  = "--gfa-file";
constexpr auto GFA_FORMAT_OPTION                         = "-F";
constexpr auto GFA_FORMAT_LONG_OPTION                    = "--gfa-format";

} // namespace PANGWES
