#pragma once

#include <fstream>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

#include "Graph.hpp"
#include "ProgramOptions.hpp"
#include "types.hpp"
#include "Utils.hpp"

class GraphBuilder {
public:
    static Graph build_ordinary_graph(const std::string& edges_filename, bool one_based = false) {
        Graph graph(one_based);
        std::vector<std::tuple<int_t, int_t, real_t>> edges;
        std::ifstream ifs(edges_filename);
        int_t max_v = 0;
        for (std::string line; std::getline(ifs, line); ) {
            auto fields = Utils::get_fields(line);
            if (fields.size() < 2) {
                std::cerr << "Error: Wrong number of fields in graph edges file: " << edges_filename << std::endl;
                return Graph();
            }
            int_t v = std::stoll(fields[0]) - one_based;
            int_t w = std::stoll(fields[1]) - one_based;
            real_t weight = fields.size() >= 3 && Utils::is_numeric(fields[2]) ? std::stod(fields[2]) : 1.0;
            edges.emplace_back(v, w, weight);
            max_v = std::max(max_v, std::max(v, w));
        }
        graph.resize(max_v + 1);
        for (const auto& edge : edges) {
            int_t v, w;
            real_t weight;
            std::tie(v, w, weight) = edge;
            graph.add_edge(v, w, weight);
        }
        return graph;
    }

    /* Construct a compacted de Bruijn graph constructed from multiple genome references.
       This graph stores two nodes for each unitig: one for its left side and one for its right side, considered from the canonical form. */
    static Graph build_cdbg(const std::string& unitigs_filename, const std::string& edges_filename, int_t kmer_length, bool one_based = false) {
        Graph graph(one_based, true);
        std::ifstream ifs_unitigs(unitigs_filename);
        for (std::string line; std::getline(ifs_unitigs, line); ) {
            auto fields = Utils::get_fields(line);
            if (fields.size() < 2) {
                std::cerr << "Error: Wrong number of fields in compacted de Bruijn graph unitigs file: " << unitigs_filename << std::endl;
                return Graph();
            }
            real_t self_edge_weight = (real_t) fields[1].size() - kmer_length;
            if (self_edge_weight < 0.0) {
                std::cerr << "self_edge_weight = " << self_edge_weight << " < 0.0 -- wrong k-mer length?" << std::endl;
                return Graph();
            }
            graph.add_two_sided_node(self_edge_weight);
        }

        std::ifstream ifs_edges(edges_filename);
        for (std::string line; std::getline(ifs_edges, line); ) {
            auto fields = Utils::get_fields(line);
            if (fields.size() < 3) {
                std::cerr << "Error: Wrong number of fields in compacted de Bruijn graph edges file:" << edges_filename << std::endl;
                return Graph();
            }
            bool good_overlap = fields.size() < 4 || std::stoll(fields[3]) != 0;
            if (!good_overlap) continue; // Non-overlapping edges ignored.
            std::string edge_type = fields[2];
            int_t v = 2 * (std::stoll(fields[0]) - one_based) + (edge_type[0] == 'F'); // F* edge means link comes from v's right side.
            int_t w = 2 * (std::stoll(fields[1]) - one_based) + (edge_type[1] == 'R'); // *R edge means link goes to w's right side.
            graph.add_edge(v, w, 1.0); // Weight 1.0 by definition.
        }
        return graph;
    }

    // Construct an edge-induced subgraph from the compacted de Bruijn graph. Will be used to construct a single genome graph.
    static Graph build_cdbg_subgraph(const Graph& cdbg, const std::string& edges_filename) {
        if (!cdbg.two_sided()) {
            std::cerr << "Error: build_cdbg_subgraph called with non-two-sided graph." << std::endl;
            return Graph();
        }
        Graph graph(cdbg.one_based(), false);
        std::vector<std::pair<int_t, int_t>> edges;
        std::ifstream ifs_edges(edges_filename);
        int_t max_v = 0;
        for (std::string line; std::getline(ifs_edges, line); ) {
            auto fields = Utils::get_fields(line);
            if (fields.size() < 3) {
                std::cout << "Error: Wrong number of fields in single genome graph edges file: " << edges_filename << std::endl;
                return Graph();
            }
            bool good_overlap = fields.size() < 4 || std::stoll(fields[3]) != 0;
            if (!good_overlap) continue; // Non-overlapping edges ignored.
            std::string edge_type = fields[2];
            int_t v = 2 * (std::stoll(fields[0]) - graph.one_based()) + (edge_type[0] == 'F'); // F* edge means link comes from v's right side.
            int_t w = 2 * (std::stoll(fields[1]) - graph.one_based()) + (edge_type[1] == 'R'); // *R edge means link goes to w's right side.
            edges.emplace_back(v, w);
            max_v = std::max(max_v, std::max(v, w));
        }
        graph.resize((max_v | 1) + 1);

        for (const auto& edge : edges) {
            int_t v, w;
            std::tie(v, w) = edge;
            // Get self-edges from the original graph.
            if (graph.degree(v) == 0) graph.add_edge(v, graph.other_side(v), cdbg.get_self_edge_weight(v));
            if (graph.degree(w) == 0) graph.add_edge(w, graph.other_side(w), cdbg.get_self_edge_weight(w));
            graph.add_edge(v, w, 1.0); // Weight 1.0 by definition.
        }
        return graph;
    }

    static Graph build_correct_graph() {
        if (ProgramOptions::has_operating_mode(OperatingMode::GENERAL)) return build_ordinary_graph(ProgramOptions::edges_filename, ProgramOptions::graphs_one_based);
        if (ProgramOptions::has_operating_mode(OperatingMode::CDBG)) return build_cdbg(ProgramOptions::unitigs_filename, ProgramOptions::edges_filename, ProgramOptions::k, ProgramOptions::graphs_one_based);
        std::cout << "Error: Program logic error." << std::endl;
        return Graph();
    }

};
