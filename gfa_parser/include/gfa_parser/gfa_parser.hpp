/*
 * gfa_parser.hpp - Entry point for the GFA parser program.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include "gfa_parser/program_options/GFAParserOptions.hpp"

namespace PANGWES {

// Returns true if all mandatory arguments were provided and are okay.
bool check_gfa_parser_options(const GFAParserOptions& options) noexcept;

// Runs the main program.
void run_gfa_parser(const GFAParserOptions& options);

} // namespace PANGWES
