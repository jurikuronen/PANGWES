/*
 * main.cpp - Main program for gfa_parser.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include "common/entry/entrypoint.hpp"
#include "gfa_parser/program_options/GFAParserOptions.hpp"
#include "gfa_parser/gfa_parser.hpp"

int main(int argc, char** argv) {
    using namespace PANGWES;

    return Entry::run_program<GFAParserOptions>(argc,
                                                argv,
                                                "gfa_parser",
                                                check_gfa_parser_options,
                                                run_gfa_parser);
}
