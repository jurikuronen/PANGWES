/*
 * gfa_parser.cpp - GFA parser program logic.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <iostream>

#include "common/io/FileReader.hpp"
#include "common/io/Log.hpp"
#include "common/utils/Exception.hpp"
#include "common/utils/format.hpp"
#include "common/utils/memory.hpp"
#include "common/utils/Timer.hpp"
#include "common/utils/WorkerPool.hpp"
#include "gfa_parser/core/GFAData.hpp"
#include "gfa_parser/core/GFAFormat.hpp"
#include "gfa_parser/gfa_parser.hpp"
#include "gfa_parser/io/GFAParserOutputCoordinator.hpp"
#include "gfa_parser/program_options/GFAParserOptions.hpp"

namespace PANGWES {

bool check_gfa_parser_options(const GFAParserOptions& options) noexcept {
    bool options_ok = true;

    if (options.gfa_filename().empty()) {
        std::cerr << "No GFA file provided." << std::endl;
        options_ok = false;
    }

    if (options.gfa_format() != GFAFormat::GFA1) {
        std::cerr << "Unsupported GFA format (only GFA 1.0 currently supported)" << std::endl;
        options_ok = false;
    }

    if (options.k() % 2 == 0) {
        std::cerr << "k-mer length must be odd" << std::endl;
        options_ok = false;
    }

    if (options.k() < 3) {
        std::cerr << "k-mer length must be at least 3" << std::endl;
        options_ok = false;
    }

    return options_ok;
}

void run_gfa_parser(const GFAParserOptions& options) {
    Timer main_timer;

    const auto gfa_format = options.gfa_format();
    const auto out_unitigs_filename = options.out_unitigs_filename();
    const auto out_fasta_filename = options.out_fasta_filename();
    const auto out_sgg_paths_filename = options.out_sgg_paths_filename();
    const auto out_sgg_paths_directory = options.out_sgg_paths_directory();
    const auto expected_overlap = options.k() - 1;

    // Prepare GFAData with the given expected global overlap.
    GFAData gfa_data(Memory::make_unique<FileReader>(options.gfa_filename()), expected_overlap);

    Log::out() << "Starting the first read pass of the " << to_string(options.gfa_format()) << " file." << std::endl;

    /*
     * First read pass:
     * - Read segments for the ".unitigs" file.
     * - Read links and count the number of references for the ".edges" files.
    */
    gfa_data.read_segments_links_and_reference_counts();

    Log::out() << "Completed the first read pass of the " << to_string(gfa_format) << " file in "
               << Format::duration_to_string(main_timer.last_interval_ms(true)) << "." << std::endl;

    // Construct output writers and initialize internal data for writing the ".edges" files.
    GFAParserOutputCoordinator output_coordinator(out_unitigs_filename,
                                                  out_fasta_filename,
                                                  out_sgg_paths_filename,
                                                  out_sgg_paths_directory,
                                                  gfa_data.n_reference_paths(),
                                                  expected_overlap);

    /*
     * GFAParserOutputCoordinator writes output files while the GFA file is being read. If an error occurs, explicit
     * exception handling and control of the worker pool are required to stop ongoing writing jobs and remove any
     * partially created files.
    */
    try {
        output_coordinator.post_write_unitigs_job(gfa_data.segment_name_map());


        Log::out() << "Starting the second read pass of the " << to_string(options.gfa_format()) << " file."
                   << std::endl;

        /*
         * Second read pass:
         * - Read path lines to construct ReferencePathData.
         * - While reading, pass ready ReferencePathData to the output coordinator for writing the ".fasta" and ".edges"
         *   files.
        */
        gfa_data.read_reference_path_data(output_coordinator);

        Log::out() << "Completed the second read pass of the " << to_string(gfa_format) << " file in "
                   << Format::duration_to_string(main_timer.last_interval_ms(true)) << "." << std::endl;

        WorkerPool::wait();

        output_coordinator.write_paths();
    } catch (...) {
        WorkerPool::stop();

        output_coordinator.remove_created_files();

        throw;
    }

    Log::out() << "Finished. Total runtime: "
               << Format::duration_to_string(main_timer.total_ms(true)) << "." << std::endl;
}

} // namespace PANGWES
