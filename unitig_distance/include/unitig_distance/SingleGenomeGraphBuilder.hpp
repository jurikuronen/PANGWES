#pragma once

#include <string>

#include "GraphBuilder.hpp"
#include "SingleGenomeGraph.hpp"
#include "types.hpp"

class SingleGenomeGraphBuilder {
public:
    // Construct a compressed single genome graph, which is an edge-induced subgraph from the compacted de Bruijn graph.
    static SingleGenomeGraph build_sgg(const Graph& cdbg, const std::string& edges_filename) {
        SingleGenomeGraph sgg;

        Graph subgraph = GraphBuilder::build_cdbg_subgraph(cdbg, edges_filename);

        if (subgraph.size() == 0) return SingleGenomeGraph();

        sgg.resize_node_map(subgraph.size());

        std::vector<bool> visited(subgraph.size());

        // Run a DFS on the edge-induced subgraph to construct a graph with compressed paths.
        for (int_t v = 0; v < (int_t) subgraph.size(); ++v) {
            if (visited[v] || subgraph.degree(v) == 0) continue;
            add_and_map_node(sgg, v);
            visited[v] = true;
            std::vector<std::tuple<int_t, int_t, real_t>> stack;
            dfs_add_neighbors_to_stack(subgraph, visited, stack, v);
            while (!stack.empty()) {
                int_t parent, w;
                real_t weight;
                std::tie(parent, w, weight) = stack.back();
                stack.pop_back();
                if (visited[w]) {
                    if (!sgg.is_on_path(w)) sgg.add_edge(sgg.mapped_idx(parent), sgg.mapped_idx(w), weight); // Edge might not be added yet.
                    continue;
                }
                if (subgraph.degree(w) == 2) {
                    // Compress the path into a single edge, updating w and weight.
                    std::tie(w, weight) = process_path(sgg, subgraph, visited, parent, w, weight);
                    if (w == parent) continue; // Path looped back to parent.
                }
                if (!sgg.is_mapped(w)) add_and_map_node(sgg, w);
                sgg.add_edge(sgg.mapped_idx(parent), sgg.mapped_idx(w), weight);
                dfs_add_neighbors_to_stack(subgraph, visited, stack, w);
                visited[w] = true;
            }
        }

        return sgg;
    }

private:
    // Functions used by the builder's DFS search.
    static void dfs_add_neighbors_to_stack(
        const Graph& subgraph,
        const std::vector<bool>& visited,
        std::vector<std::tuple<int_t, int_t, real_t>>& stack,
        int_t original_idx)
    {
        for (auto neighbor : subgraph[original_idx]) {
            auto neighbor_idx = neighbor.first;
            auto weight = neighbor.second;
            stack.emplace_back(original_idx, neighbor_idx, weight);
        }
    }

    static void add_and_map_node(SingleGenomeGraph& sgg, int_t original_idx) { sgg.map_node(original_idx, INT_T_MAX, sgg.size()); sgg.add_node(); } // Add non-path node.

    static std::pair<int_t, real_t> process_path(SingleGenomeGraph& sgg, const Graph& subgraph, std::vector<bool>& visited, int_t path_start_node, int_t w, real_t weight) {
        // First node given.
        std::vector<int_t> nodes_in_path{w};
        std::vector<real_t> D{weight};
        auto prev_node = path_start_node;

        // Move along the path.
        while (subgraph.degree(w) == 2) {
            auto it = subgraph[w].begin();
            if (it->first == prev_node) ++it;
            prev_node = w;
            std::tie(w, weight) = *it;
            nodes_in_path.push_back(w);
            weight += D.back(); // Accumulate weight.
            D.push_back(weight);
            if (sgg.is_mapped(w)) break; // Reached end of path (or a loop was found).
        }

        // Map nodes in path as path nodes. Path start and end nodes treated separately.
        auto new_path_idx = sgg.n_paths();
        for (std::size_t i = 0; i + 1 < nodes_in_path.size(); ++i) {
            visited[nodes_in_path[i]] = true;
            sgg.map_node(nodes_in_path[i], new_path_idx, i);
        }

        // Add new path.
        auto mapped_path_end_node = sgg.is_mapped(w) ? sgg.mapped_idx(w) : sgg.size(); // Can use size() here because w will be added and mapped next.
        sgg.add_new_path(sgg.mapped_idx(path_start_node), mapped_path_end_node, std::move(D));
        return std::make_pair(w, weight); // Return updated w and weight.
    }

};
