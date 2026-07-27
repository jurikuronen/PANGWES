/*
 * unitig_distance.cpp - Unitig distance program logic.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <initializer_list>
#include <iostream>
#include <memory>
#include <string>

#include "common/io/filesystem.hpp"
#include "common/io/FileReader.hpp"
#include "common/io/FileWriter.hpp"
#include "common/io/Log.hpp"
#include "common/utils/format.hpp"
#include "common/utils/memory.hpp"
#include "common/utils/Timer.hpp"
#include "unitig_distance/core/DistanceQueryEngine.hpp"
#include "unitig_distance/core/UnitigWeights.hpp"
#include "unitig_distance/io/QueriesReader.hpp"
#include "unitig_distance/io/UnitigDistanceResults.hpp"
#include "unitig_distance/program_options/UnitigDistanceOptions.hpp"
#include "unitig_distance/sgg/SGGEdgesFilenames.hpp"
#include "unitig_distance/unitig_distance.hpp"

namespace PANGWES {
namespace {

/*
 * Attempts to construct a file reader from all the provided files.
 *
 * Failures (file doesn't exist, file can't be read, etc.) throw an error which will be caught.
*/
template <typename FilenameContainer = std::initializer_list<std::string>>
void check_files(const FilenameContainer& filenames) {
    for (const auto& filename : filenames) {
        (void)Memory::make_unique<FileReader>(filename)->open();
    }
}

} // namespace

bool check_unitig_distance_options(const UnitigDistanceOptions& options) noexcept {
    bool options_ok = true;

    if (options.unitigs_filename().empty()) {
        std::cerr << "No unitigs file provided." << std::endl;
        options_ok = false;
    }

    if (options.queries_filename().empty()) {
        std::cerr << "No queries file provided." << std::endl;
        options_ok = false;
    }

    if (options.sgg_paths_filename().empty()) {
        std::cerr << "No single-genome graph paths file provided." << std::endl;
        options_ok = false;
    }

    if (options.k() == 0) {
        std::cerr << "No k-mer length provided." << std::endl;
        options_ok = false;
    }

    return options_ok;
}

void run_unitig_distance(const UnitigDistanceOptions& options) {
    Timer main_timer;

    check_files({options.unitigs_filename(), options.queries_filename(), options.sgg_paths_filename()});

    // Read unitigs, single genome graph edges filenames and queries from the provided filenames.
    const auto sgg_edges_filenames = SGGEdgesFilenames(Memory::make_unique<FileReader>(options.sgg_paths_filename()));

    Log::out() << "Read " << Format::pretty_uint(sgg_edges_filenames.size()) << " SGG edge-file paths." << std::endl;

    check_files(sgg_edges_filenames);

    const auto unitig_weights = UnitigWeights(Memory::make_unique<FileReader>(options.unitigs_filename()), options.k());

    Log::out() << "Read " << Format::pretty_uint(unitig_weights.size()) << " unitigs." << std::endl;

    /*
     * Initialize QueriesReader from the queries filename.
     * The queries will be read twice: first to read unitig pairs for the graph distances and again when writing output.
    */
    auto queries_reader = QueriesReader(Memory::make_unique<FileReader>(options.queries_filename()),
                                        options.n_queries(),
                                        options.queries_one_based());

    /*
     * Initialize DistanceQueryEngine from the unitig weights, SGG edges filenames and queries read with the
     * QueriesReader during construction.
    */
    DistanceQueryEngine distance_query_engine(unitig_weights,
                                              sgg_edges_filenames,
                                              queries_reader,
                                              options.memory_bytes(),
                                              options.no_median_distance());

    Log::out() << "Read queries and initialized the distance-query engine in "
               << Format::duration_to_string(main_timer.last_interval_ms(true)) << "." << std::endl;

    // Read queries with the QueriesReader and compute graph distances for all queried unitig pairs.
    const auto distances = distance_query_engine.compute_distances();

    Log::out() << "Computed shortest-path distances for all queried unitig pairs in "
               << Format::duration_to_string(main_timer.last_interval_ms(true)) << "." << std::endl;

    /*
     * Prepare a file writer for the specified output filename. The writer selects a final filename to protect against
     * filename collisions.
    */
    std::unique_ptr<FileWriterInterface> file_writer = Memory::make_unique<FileWriter>(options.out_filename());
    const auto resolved_out_filename = file_writer->filename();
    Log::out() << "Opened file \"" << resolved_out_filename << "\" for writing." << std::endl;

    try {
        UnitigDistanceResults::write_results(file_writer,
                                             queries_reader,
                                             distances,
                                             options.output_one_based());
    } catch (...) {
        // Remove the partially written output file after an error.
        file_writer.reset();
        Filesystem::remove_file_if_exists(resolved_out_filename);

        throw;
    }

    Log::out() << "Wrote results to file \"" << resolved_out_filename << "\" in "
               << Format::duration_to_string(main_timer.last_interval_ms(true)) << "." << std::endl;

    Log::out() << "Finished. Total runtime: " << Format::duration_to_string(main_timer.total_ms(true)) << "."
               << std::endl;
}

} // namespace PANGWES
