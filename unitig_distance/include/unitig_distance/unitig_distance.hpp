/*
 * unitig_distance.hpp - Entry point for the unitig distance program.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include "unitig_distance/program_options/UnitigDistanceOptions.hpp"

namespace PANGWES {

// Returns true if all mandatory arguments were provided and are okay.
bool check_unitig_distance_options(const UnitigDistanceOptions& options) noexcept;

// Runs the main program.
void run_unitig_distance(const UnitigDistanceOptions& options);

} // namespace PANGWES
