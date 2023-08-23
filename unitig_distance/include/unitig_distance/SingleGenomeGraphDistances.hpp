#pragma once

#include <iostream>
#include <map>
#include <set>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Distance.hpp"
#include "DistanceVector.hpp"
#include "ProgramOptions.hpp"
#include "SearchJobs.hpp"
#include "SingleGenomeGraph.hpp"
#include "Timer.hpp"
#include "types.hpp"

DistanceVector calculate_sgg_distances(const Graph& graph, const SearchJobs& search_jobs, Timer& timer);

class SingleGenomeGraphDistances {
public:
    SingleGenomeGraphDistances() = delete;
    SingleGenomeGraphDistances(const SingleGenomeGraph& graph) : m_graph(graph), m_max_distance(ProgramOptions::max_distance) { }

    // Calculate distances for single genome graphs.
    std::vector<std::unordered_map<int_t, Distance>> solve(const SearchJobs& search_jobs) {
        auto n_threads = ProgramOptions::n_threads;
        std::vector<std::unordered_map<int_t, Distance>> sgg_batch_distances(n_threads);
        auto calculate_distance_block = [this, &search_jobs, &sgg_batch_distances, n_threads](std::size_t thr) {
            const auto& graph = m_graph;
            for (std::size_t i = thr; i < search_jobs.size(); i += n_threads) {
                const auto& job = search_jobs[i];

                auto v = job.v();
                if (!graph.contains_original(v)) continue;

                // First calculate distances between path start/end nodes.
                auto sources = get_sgg_sources(v);
                auto targets = get_sgg_targets(job.ws());
                auto target_dist = graph.distance(sources, targets, m_max_distance);

                // Map results.
                std::map<int_t, real_t> dist;
                for (std::size_t j = 0; j < targets.size(); ++j) dist[targets[j]] = target_dist[j];

                // Now fix distances for (v, w) that were in paths.
                std::vector<real_t> job_dist(job.ws().size(), m_max_distance);
                process_job_distances(job_dist, graph.left_node(v), job.ws(), dist);
                process_job_distances(job_dist, graph.right_node(v), job.ws(), dist);

                add_job_distances_to_sgg_distances(sgg_batch_distances[thr], job, job_dist);
            }
        };
        std::vector<std::thread> threads(n_threads);
        for (std::size_t thr = 0; thr < (std::size_t) n_threads; ++thr) threads[thr] = std::thread(calculate_distance_block, thr);
        for (auto& thr : threads) thr.join();
        return sgg_batch_distances;
    }

private:
    const SingleGenomeGraph& m_graph;

    real_t m_max_distance;

    // Update source distance if source exists, otherwise add new source.
    void add_source(std::vector<std::pair<int_t, real_t>>& sources, int_t mapped_idx, real_t distance) {
        auto it = sources.begin();
        while (it != sources.end() && it->first != (int_t) mapped_idx) ++it;
        if (it == sources.end()) sources.emplace_back(mapped_idx, distance);
        else it->second = std::min(it->second, distance);
    }

    // Add both sides of v as sources.
    std::vector<std::pair<int_t, real_t>> get_sgg_sources(int_t v) {
        std::vector<std::pair<int_t, real_t>> sources;
        for (int_t v_original_idx = m_graph.left_node(v); v_original_idx <= m_graph.right_node(v); ++v_original_idx) {
            auto v_mapped_idx = m_graph.mapped_idx(v_original_idx);
            // Add v normally if it's not on a path, otherwise add both path end points.
            if (m_graph.is_on_path(v_original_idx)) {
                auto v_path_idx = m_graph.path_idx(v_original_idx);
                int_t path_endpoint;
                real_t distance;
                // Add path start node.
                std::tie(path_endpoint, distance) = m_graph.distance_to_start(v_path_idx, v_mapped_idx);
                add_source(sources, path_endpoint, distance);
                // Add path end node.
                std::tie(path_endpoint, distance) = m_graph.distance_to_end(v_path_idx, v_mapped_idx);
                add_source(sources, path_endpoint, distance);
            } else {
                add_source(sources, v_mapped_idx, 0.0);
            }
        }
        return sources;
    }

    // Add both sides of each w as targets.
    std::vector<int_t> get_sgg_targets(const std::vector<int_t>& ws) {
        std::set<int_t> target_set;
        for (auto w : ws) {
            if (!m_graph.contains_original(w)) continue;
            for (int_t w_original_idx = m_graph.left_node(w); w_original_idx <= m_graph.right_node(w); ++w_original_idx) {
                if (m_graph.is_on_path(w_original_idx)) {
                    auto w_path_idx = m_graph.path_idx(w_original_idx);
                    target_set.insert(m_graph.start_node(w_path_idx));
                    target_set.insert(m_graph.end_node(w_path_idx));
                } else {
                    target_set.insert(m_graph.mapped_idx(w_original_idx));
                }
            }
        }
        std::vector<int_t> targets;
        for (auto t : target_set) targets.push_back(t);
        return targets;
    }

    // Correct (v, w) distance if w were on a path.
    real_t get_correct_distance(int_t v_path_idx, int_t v_mapped_idx, int_t w_original_idx, std::map<int_t, real_t>& dist) {
        auto w_path_idx = m_graph.path_idx(w_original_idx);
        auto w_mapped_idx = m_graph.mapped_idx(w_original_idx);
        if (w_path_idx == INT_T_MAX) return dist[w_mapped_idx]; // w not on path, distance from sources is correct already.
        // Get distance if v and w are on the same path, this distance could be shorter.
        real_t distance = v_path_idx == w_path_idx ? m_graph.distance_in_path(v_path_idx, v_mapped_idx, w_mapped_idx) : REAL_T_MAX;
        // w on path, add distances of (w, path_endpoint).
        int_t w_path_endpoint;
        real_t w_path_distance;
        std::tie(w_path_endpoint, w_path_distance) = m_graph.distance_to_start(w_path_idx, w_mapped_idx);
        distance = std::min(distance, dist[w_path_endpoint] + w_path_distance);
        std::tie(w_path_endpoint, w_path_distance) = m_graph.distance_to_end(w_path_idx, w_mapped_idx);
        distance = std::min(distance, dist[w_path_endpoint] + w_path_distance);
        return distance;
    }

    // Fix distances for (v, w) that were in paths.
    void process_job_distances(std::vector<real_t>& job_dist, int_t v_original_idx, const std::vector<int_t>& ws, std::map<int_t, real_t>& dist) {
        auto v_path_idx = m_graph.path_idx(v_original_idx);
        auto v_mapped_idx = m_graph.mapped_idx(v_original_idx);
        for (std::size_t w_idx = 0; w_idx < ws.size(); ++w_idx) { 
            auto w = ws[w_idx];
            if (!m_graph.contains_original(w)) continue;
            auto distance = get_correct_distance(v_path_idx, v_mapped_idx, m_graph.left_node(w), dist);
            distance = std::min(distance, get_correct_distance(v_path_idx, v_mapped_idx, m_graph.right_node(w), dist));
            job_dist[w_idx] = std::min(job_dist[w_idx], distance);
        }
    }

    void add_job_distances_to_sgg_distances(std::unordered_map<int_t, Distance>& sgg_distances, const SearchJob& job, const std::vector<real_t>& job_dist) {
        for (std::size_t w_idx = 0; w_idx < job_dist.size(); ++w_idx) {
            auto distance = job_dist[w_idx];
            if (distance >= m_max_distance) continue;
            auto original_idx = job.original_index(w_idx);
            sgg_distances.emplace(original_idx, distance);
        }
    }

};
