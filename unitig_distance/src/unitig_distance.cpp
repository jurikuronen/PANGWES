#include <iostream>
#include <string>

#include "QueriesReader.hpp"
#include "GraphBuilder.hpp"
#include "GraphDistances.hpp"
#include "OperatingMode.hpp"
#include "OutlierTools.hpp"
#include "PrintUtils.hpp"
#include "ProgramOptions.hpp"
#include "ResultsWriter.hpp"
#include "SearchJobs.hpp"
#include "SingleGenomeGraphDistances.hpp"
#include "Timer.hpp"
#include "types.hpp"
#include "Utils.hpp"

static int fail_with_error(const std::string& error) { std::cerr << error << std::endl; return 1; }

int main(int argc, char** argv) {
    Timer timer;

    // Read command line arguments.
    ProgramOptions::read_command_line_arguments(argc, argv);
    if (ProgramOptions::verbose) PrintUtils::print_license();
    if (!ProgramOptions::valid_state || !Utils::sanity_check_input_files()) return 1;
    if (ProgramOptions::verbose) ProgramOptions::print_run_details();

    // Read queries.
    const auto queries = QueriesReader::read_queries(timer);
    if (queries.size() == 0) return fail_with_error("Error: Failed to read queries.");
    if (ProgramOptions::verbose) PrintUtils::print_tbss_tsmasm(timer, "Read", Utils::neat_number_str(queries.size()), "lines from queries file");

    // Set up outlier tools.
    const OutlierTools ot(queries, timer);
    if (ProgramOptions::verbose) PrintUtils::print_tbss_tsmasm(timer, "Set up outlier tools");

    // Operating in outliers tool mode only.
    if (ProgramOptions::operating_mode == OperatingMode::OUTLIER_TOOLS) {
        ot.determine_and_output_outliers(queries.distances(), ProgramOptions::out_outliers_filename(), ProgramOptions::out_outlier_stats_filename());
        return 0;
    }

    // Compute search jobs.
    const SearchJobs search_jobs(queries);
    if (ProgramOptions::verbose) PrintUtils::print_tbss_tsmasm(timer, "Prepared", Utils::neat_number_str(search_jobs.size()), "search jobs");

    // Construct the graph according to operating mode.
    const auto graph = GraphBuilder::build_correct_graph();
    if (graph.size() == 0) return fail_with_error("Error: Failed to construct main graph.");
    if (ProgramOptions::verbose) {
        PrintUtils::print_tbss_tsmasm_noendl(timer, "Constructed main graph");
        graph.print_details();
    }

    // Calculate distances in the single genome graphs if the single genome graph files were provided.
    if (ProgramOptions::has_operating_mode(OperatingMode::SGGS)) {
        const auto sgg_distances = calculate_sgg_distances(graph, search_jobs, timer);

        if (sgg_distances.size() == 0) return 1;

        // Output single genome graphs graph distances.
        ResultsWriter::output_results(ProgramOptions::out_sgg_filename(), queries, sgg_distances);
        if (ProgramOptions::verbose) PrintUtils::print_tbss_tsmasm(timer, "Output single genome graph mean distances to file", ProgramOptions::out_sgg_filename());

        // Determine outliers.
        if (ProgramOptions::has_operating_mode(OperatingMode::OUTLIER_TOOLS)) {
            ot.determine_and_output_outliers(sgg_distances, ProgramOptions::out_sgg_outliers_filename(), ProgramOptions::out_sgg_outlier_stats_filename());
        }

    }

    // Run normal graph
    if (!ProgramOptions::run_sggs_only) {
        if (ProgramOptions::verbose) PrintUtils::print_tbssasm(timer, "Calculating distances in the main graph");

        // Calculate distances.
        const auto graph_distances = GraphDistances(graph, timer).solve(search_jobs);
        timer.set_mark();

        ResultsWriter::output_results(ProgramOptions::out_filename(), queries, graph_distances);
        if (ProgramOptions::verbose) PrintUtils::print_tbss_tsmasm(timer, "Output main graph distances to file", ProgramOptions::out_filename());

        // Determine outliers.
        if (ProgramOptions::has_operating_mode(OperatingMode::OUTLIER_TOOLS)) {
            ot.determine_and_output_outliers(graph_distances, ProgramOptions::out_outliers_filename(), ProgramOptions::out_outlier_stats_filename());
        }

    }

    if (ProgramOptions::verbose) PrintUtils::print_tbss(timer, "Finished");
}
