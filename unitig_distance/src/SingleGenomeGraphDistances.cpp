#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "DistanceVector.hpp"
#include "Graph.hpp"
#include "PrintUtils.hpp"
#include "SearchJobs.hpp"
#include "SingleGenomeGraph.hpp"
#include "SingleGenomeGraphBuilder.hpp"
#include "SingleGenomeGraphDistances.hpp"
#include "Timer.hpp"
#include "types.hpp"

DistanceVector calculate_sgg_distances(const Graph& graph, const SearchJobs& search_jobs, Timer& timer) {
    DistanceVector sgg_distances(search_jobs.n_queries(), 0.0, 0);

    sgg_distances.set_mean_distances();

    // Read single genome graph edge files.
    std::vector<std::string> path_edge_files;
    std::ifstream ifs(ProgramOptions::sggs_filename);
    for (std::string path_edges; std::getline(ifs, path_edges); ) path_edge_files.emplace_back(path_edges);
    std::size_t n_sggs = path_edge_files.size(), batch_size = ProgramOptions::n_threads;

    if (n_sggs == 0) {
        std::cerr << "Error: Couldn't read single genome graph files." << std::endl;
        return DistanceVector();
    }

    // Printing variables for verbose mode.
    Timer t_sgg, t_sgg_distances, t_deconstruct;
    int_t print_interval = (n_sggs + 4) / 5, print_i = 1, n_nodes = 0, n_edges = 0;
    if (print_interval % batch_size) print_interval += batch_size - (print_interval % batch_size); // Round up.
    bool print_now = false;

    if (ProgramOptions::verbose) PrintUtils::print_tbssasm(timer, "Calculating distances in the single genome graphs");

    for (std::size_t i = 0; i < n_sggs; i += batch_size) {
        if (ProgramOptions::verbose) {
            t_deconstruct.add_time_since_mark();
            if (print_now) {
                auto stslasl = t_deconstruct.get_stopwatch_time_since_lap_and_set_lap();
                PrintUtils::print_tbss(timer, "Deconstructing single genome graphs", print_i, "-", i, "/", n_sggs, "took", stslasl);
                print_i = i + 1;
            }
            t_sgg.set_mark();
        }

        if (ProgramOptions::verbose) print_now = (i + batch_size) % print_interval == 0 || (i + batch_size) >= n_sggs;

        auto batch = std::min(i + batch_size, n_sggs) - i;

        // Construct a batch of single genome graphs.
        std::vector<SingleGenomeGraph> sg_graphs(batch);
        auto construct_sgg = [&graph, &sg_graphs](int_t thr, const std::string& path_edges) {
            sg_graphs[thr] = SingleGenomeGraphBuilder::build_sgg(graph, path_edges);
        };

        std::vector<std::thread> threads;
        for (std::size_t thr = 0; thr < batch; ++thr) threads.emplace_back(construct_sgg, thr, path_edge_files[i + thr]);
        for (auto& thr : threads) thr.join();

        for (const auto& sg_graph : sg_graphs) {
            if (sg_graph.size() == 0) {
                std::cerr << "Error: Failed to construct single genome graph." << std::endl;
                return DistanceVector();
            }
        }

        if (ProgramOptions::verbose) {
            t_sgg.add_time_since_mark();
            if (print_now) {
                auto stslasl = t_sgg.get_stopwatch_time_since_lap_and_set_lap();
                PrintUtils::print_tbss(timer, "Constructing single genome graphs", print_i, "-", i + batch, "/", n_sggs, "took", stslasl);
            }
            // Update n_nodes and n_edges.
            for (const auto& sg_graph : sg_graphs) {
                n_nodes += sg_graph.size();
                for (const auto& adj : sg_graph) n_edges += adj.size();
            }
            t_sgg_distances.set_mark();
        }

        // Calculate distances in the single genome graphs.
        for (const auto& sg_graph : sg_graphs) {
            auto sgg_batch_distances = SingleGenomeGraphDistances(sg_graph).solve(search_jobs);
            // Combine results across threads.
            for (const auto& distances : sgg_batch_distances) {
                for (const auto& result : distances) {
                    int_t original_idx;
                    Distance distance;
                    std::tie(original_idx, distance) = result;
                    sgg_distances[original_idx] += distance;
                }
            }
        }

        if (ProgramOptions::verbose) {
            t_sgg_distances.add_time_since_mark();
            if (print_now) {
                auto stslasl = t_sgg_distances.get_stopwatch_time_since_lap_and_set_lap();
                PrintUtils::print_tbss(timer, "Calculating distances in the single genome graphs", print_i, "-", i + batch, "/", n_sggs, "took", stslasl);
            }
            t_deconstruct.set_mark();
        }
    }

    if (ProgramOptions::verbose) {
        t_deconstruct.add_time_since_mark();
        auto stslasl = t_deconstruct.get_stopwatch_time_since_lap_and_set_lap();
        PrintUtils::print_tbss(timer, "Deconstructing single genome graphs and distances", print_i, "-", n_sggs, "/", n_sggs, "took", stslasl);
    }

    // Set distance correctly for disconnected queries.
    for (auto& distance : sgg_distances) if (distance.count() == 0) distance = Distance(REAL_T_MAX, 0);

    if (ProgramOptions::verbose) {
        n_nodes /= n_sggs;
        n_edges /= 2 * n_sggs;
        PrintUtils::print_tbss(timer, "Constructing", n_sggs, "single genome graphs took", t_sgg.get_stopwatch_time());
        PrintUtils::print_tbss(timer, "The compressed single genome graphs have on average", Utils::neat_number_str(n_nodes), "connected nodes and", 
                               Utils::neat_number_str(n_edges), "edges");
        PrintUtils::print_tbss(timer, "Calculating distances in the", n_sggs, "single genome graphs took", t_sgg_distances.get_stopwatch_time());
        PrintUtils::print_tbssasm(timer, "Deconstructing", n_sggs, "single genome graphs took", t_deconstruct.get_stopwatch_time());
    }

    return sgg_distances;
}
