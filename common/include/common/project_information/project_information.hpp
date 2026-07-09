/*
 * project_information.hpp - Project information printing utilities for PANGWES components.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <iostream>

#ifndef PANGWES_VERSION
#define PANGWES_VERSION "(unknown version)"
#endif

#ifndef GIT_REV_SHORT
#define GIT_REV_SHORT "n/a"
#endif

namespace PANGWES {
namespace ProjectInformation {

constexpr auto PIPELINE_NAME     = "PAN-GWES";
constexpr auto PROJECT_AUTHOR    = "Juri Kuronen";
constexpr auto PROJECT_LICENSE   = "MIT License";
constexpr auto PROJECT_COPYRIGHT = "Copyright (c) 2020-2026";

// Prints the project information to stdout.
inline void print_project_information(const char* project_name) {
    std::cout << PIPELINE_NAME << '/' << project_name << ' '
              << PANGWES_VERSION << ' ' << "(revision: " << GIT_REV_SHORT << ")\n\n";
    std::cout << PROJECT_COPYRIGHT << ' ' << PROJECT_AUTHOR << '\n';
    std::cout << "Licensed under " << PROJECT_LICENSE << "\n" << std::flush;
}

} // namespace ProjectInformation
} // namespace PANGWES
