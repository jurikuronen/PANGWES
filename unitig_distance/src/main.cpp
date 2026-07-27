/*
 * main.cpp - Main program for unitig_distance.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include "common/entry/entrypoint.hpp"
#include "unitig_distance/program_options/UnitigDistanceOptions.hpp"
#include "unitig_distance/unitig_distance.hpp"

int main(int argc, char** argv) {
    using namespace PANGWES;

    return Entry::run_program<UnitigDistanceOptions>(argc,
                                                     argv,
                                                     "unitig_distance",
                                                     check_unitig_distance_options,
                                                     run_unitig_distance);
}
