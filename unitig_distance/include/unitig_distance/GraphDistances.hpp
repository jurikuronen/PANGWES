#pragma once

#include <iostream>
#include <set>
#include <thread>
#include <utility>
#include <vector>

#include "Distance.hpp"
#include "DistanceVector.hpp"
#include "Graph.hpp"
#include "PrintUtils.hpp"
#include "ProgramOptions.hpp"
#include "SearchJobs.hpp"
#include "Timer.hpp"
#include "types.hpp"

class GraphDistances {
public:
    GraphDistances() = delete;
    GraphDistances(const Graph& graph, const Timer& timer)
    : m_graph(graph),
      m_timer(timer)
    { }

    // Calculate distances for general graphs and compacted de Bruijn graphs.
    DistanceVector solve(const SearchJobs& search_jobs) {
        DistanceVector res(search_jobs.n_queries(), REAL_T_MAX);

        auto calculate_distance_block = [this, &search_jobs, &res](std::size_t thr, std::size_t block_start, std::size_t block_end) {
            bool two_sided = m_graph.two_sided();
            for (std::size_t i = thr + block_start; i < block_end; i += ProgramOptions::n_threads) {
                const auto& job = search_jobs[i];

                auto v = job.v();
                if ((two_sided && !m_graph.contains(m_graph.left_node(v))) || !m_graph.contains(v)) continue;

                auto sources = get_sources(v);
                auto targets = get_targets(job.ws());
                auto target_dist = m_graph.distance(sources, targets, ProgramOptions::max_distance);

                for (std::size_t w_idx = 0; w_idx < job.size(); ++w_idx) {
                    auto original_idx = job.original_index(w_idx);
                    if (two_sided) {
                        // target_dist contains w's both sides for each w_idx.
                        res[original_idx] = std::min(target_dist[w_idx * 2], target_dist[w_idx * 2 + 1]);
                    } else {
                        res[original_idx] = target_dist[w_idx];
                    }
                }
            }
        };

        for (std::size_t block_start = 0; block_start < search_jobs.size(); block_start += 10000) {
            Timer t;
            std::size_t block_end = std::min(block_start + 10000, search_jobs.size());
            std::vector<std::thread> threads;
            for (int_t thr = 0; thr < ProgramOptions::n_threads; ++thr) threads.emplace_back(calculate_distance_block, thr, block_start, block_end);
            for (auto& thr : threads) thr.join();
            if (ProgramOptions::verbose) PrintUtils::print_tbss_tsm(m_timer, "Calculated distances for block", block_start + 1, '-', block_end, '/', search_jobs.size());
        }

        return res;
    }

private:
    const Graph& m_graph;
    const Timer& m_timer;

    bool m_verbose;
    
    std::vector<std::pair<int_t, real_t>> get_sources(int_t v) {
        std::vector<std::pair<int_t, real_t>> sources;
        if (m_graph.two_sided()) {
            sources.emplace_back(m_graph.left_node(v), 0.0);
            sources.emplace_back(m_graph.right_node(v), 0.0);
        } else {
            sources.emplace_back(v, 0.0);
        }
        return sources;
    }

    std::vector<int_t> get_targets(const std::vector<int_t>& ws) {
        std::vector<int_t> targets;
        for (auto w : ws) {
            if (m_graph.two_sided()) {
                targets.push_back(m_graph.left_node(w));
                targets.push_back(m_graph.right_node(w));
            } else {
                targets.push_back(w);
            }
        }
        return targets;
    }

};
