/*
 * unitig_distance_option_names.hpp - Named constants used by UnitigDistanceOptions.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

namespace PANGWES {

// Output file extension for unitig_distance.
constexpr auto UD_OUT_FILE_EXTENSION             = ".ud";

// Program options for unitig_distance.
constexpr auto UNITIGS_FILENAME_OPTION           = "-U";
constexpr auto UNITIGS_FILENAME_LONG_OPTION      = "--unitigs-file";
constexpr auto QUERIES_FILENAME_OPTION           = "-Q";
constexpr auto QUERIES_FILENAME_LONG_OPTION      = "--queries-file";
constexpr auto SGG_PATHS_FILENAME_OPTION         = "-S";
constexpr auto SGG_PATHS_FILENAME_LONG_OPTION    = "--sgg-paths-file";
constexpr auto N_QUERIES_OPTION                  = "-n";
constexpr auto N_QUERIES_LONG_OPTION             = "--n-queries";
constexpr auto QUERIES_ONE_BASED_OPTION          = "-1q";
constexpr auto QUERIES_ONE_BASED_LONG_OPTION     = "--queries-one-based";
constexpr auto OUTPUT_ONE_BASED_OPTION           = "-1o";
constexpr auto OUTPUT_ONE_BASED_LONG_OPTION      = "--output-one-based";
constexpr auto MEMORY_OPTION                     = "-m";
constexpr auto MEMORY_LONG_OPTION                = "--memory";
constexpr auto NO_MEDIAN_OPTION                  = "-x";
constexpr auto NO_MEDIAN_LONG_OPTION             = "--no-median-distance";

} // namespace PANGWES
