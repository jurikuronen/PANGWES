#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include "Errors.hpp"
#include "LineReader.hpp"
#include "Links.hpp"
#include "Parser.hpp"
#include "Paths.hpp"
#include "Segments.hpp"

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " [../../input.gfa1] [../../output]" << std::endl;
        return 1;
    }

    auto time_program_start = gfa1_parser::time_now();

    std::cout << "gfa1_parser | MIT License | Copyright (c) 2021 Juri Kuronen\n\n";

    // Read command line arguments.
    const std::string gfa1_path(argv[1]);
    const std::string output_stem(argv[2]);

    // Output file paths.
    const std::string edges_filename = output_stem + ".edges";
    const std::string unitigs_filename = output_stem + ".unitigs";
    const std::string fasta_filename = output_stem + ".fasta";
    const std::string counts_filename = output_stem + ".counts";
    const std::string paths_filename = output_stem + ".paths";
    const std::string path_directory = output_stem + "_paths";

    // Create the path directory if it doesn't exist.
    if (!gfa1_parser::directory_exists(path_directory)) {
        if (gfa1_parser::create_directory(path_directory) != 0) {
            std::cerr << "Failed to create directory \"" << path_directory << "\"." << std::endl;
            return -1;
        }
    }

    // Open stream to GFA1 file.
    std::ifstream ifs(gfa1_path);
    if (!ifs.good()) {
        std::cerr << "Can't open \"" << gfa1_path << "\"." << std::endl;
        return 1;
    }

    // Initialization.
    Segments segments;
    Links links(4ULL * 1024 * 1024);
    Paths paths;
    PathData path_data;

    Errors errors;
    LineReader line_reader(errors);
    Parser parser(segments, links, paths, path_data, errors);

    // Main file reading loop.
    bool ok = true;
    const std::size_t block_size = 1000ULL * 1000 * 10;

    auto time_main_loop_start = gfa1_parser::time_now();
    std::cout << "Reading GFA1 lines..." << std::flush;
    for (std::size_t idx = 0; ok && ifs.peek() != EOF; ) {
        ok = line_reader.read_next_block(ifs, idx, idx + block_size);
        if (ok) ok = parser.process_segment_line_data(line_reader.segment_line_data());
        if (ok) ok = parser.process_link_line_data(line_reader.link_line_data());
        if (ok) ok = parser.process_path_line_data(line_reader.path_line_data());
        line_reader.clear();
        std::cout << "\rRead " << gfa1_parser::neat_number_str(idx) << " GFA1 lines. Stored "
                  << gfa1_parser::neat_number_str(segments.size()) << " segments, "
                  << gfa1_parser::neat_number_str(links.n_links()) << " links and "
                  << gfa1_parser::neat_number_str(path_data.n_path_lines()) << " path lines." 
                  << " Time elapsed: " << gfa1_parser::time_elapsed(time_main_loop_start) << '.' << std::flush;
    }
    std::cout << std::endl;

    if (!ok) {
        errors.print_errors();
        return 1;
    }

    // Verify that a segment line existed for every segment name given in link lines.
    for (std::size_t idx = 0; idx < segments.size(); ++idx) {
        if (segments[idx].empty()) {
            std::cerr << "Segment line missing for segment with name " << segments.idx_to_gfa1_name(idx) << "." << std::endl;
            return 1;
        }
    }

    std::cout << "Writing " << edges_filename << '.' << std::endl;
    links.write_out(edges_filename);

    std::cout << "Writing " << unitigs_filename << '.' << std::endl;
    segments.write_out(unitigs_filename);

    paths.set_n_segments(segments.size());

    std::cout << "Processing paths..." << std::flush;
    if (!parser.process_paths(fasta_filename, paths_filename, path_directory)) {
        errors.print_errors();
        return 1;
    }
    std::cout << "Wrote " << fasta_filename << '.' << std::endl;
    std::cout << "Wrote " << paths_filename << '.' << std::endl;
    std::cout << "Wrote paths to " << path_directory << "/." << std::endl;

    std::cout << "Writing " << counts_filename << '.' << std::endl;
    paths.write_counts(counts_filename);

    std::cout << "gfa1_parser finished in " << gfa1_parser::time_elapsed(time_program_start) << '.' << std::endl;

}
