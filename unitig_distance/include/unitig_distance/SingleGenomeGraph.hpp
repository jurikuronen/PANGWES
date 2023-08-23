#pragma once

#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "Graph.hpp"
#include "types.hpp"

class SingleGenomeGraph : public Graph {
public:
    SingleGenomeGraph() : Graph() { }
    ~SingleGenomeGraph() = default;
    SingleGenomeGraph(const SingleGenomeGraph& other) = delete;
    SingleGenomeGraph(SingleGenomeGraph&& other) : Graph(std::move(other)), m_paths(std::move(other.m_paths)), m_node_map(std::move(other.m_node_map)) { }

    bool is_on_path(int_t original_idx) const { return path_idx(original_idx) != INT_T_MAX; }

    bool contains(int_t original_idx) const { return original_idx < (int_t) m_node_map.size() && is_mapped(original_idx); }

    bool contains_original(int_t v) const { return contains(left_node(v)); }

    int_t path_idx(int_t original_idx) const { return m_node_map[original_idx].first; }

    int_t mapped_idx(int_t original_idx) const { return m_node_map[original_idx].second; }

    bool is_mapped(int_t original_idx) const { return mapped_idx(original_idx) != INT_T_MAX; }

    // Used by SingleGenomeGraphBuilder.
    void map_node(int_t original_idx, int_t path_idx, int_t mapped_idx) { m_node_map[original_idx] = std::make_pair(path_idx, mapped_idx); }
    void resize_node_map(std::size_t sz) { m_node_map.resize(sz, std::make_pair(INT_T_MAX, INT_T_MAX)); }
    std::size_t n_paths() const { return m_paths.size(); }
    void add_new_path(int_t start_node, int_t end_node, std::vector<real_t>&& D) { m_paths.emplace_back(start_node, end_node, std::move(D)); }

    // Path accessors.
    int_t start_node(int_t path_idx) const { return m_paths[path_idx].start_node; }

    int_t end_node(int_t path_idx) const { return m_paths[path_idx].end_node; }

    std::pair<int_t, real_t> distance_to_start(int_t path_idx, int_t idx) const {
        const auto& path = m_paths[path_idx];
        return std::make_pair(path.start_node, path.distance_to_start(idx));
    }

    std::pair<int_t, real_t> distance_to_end(int_t path_idx, int_t idx) const {
        const auto& path = m_paths[path_idx];
        return std::make_pair(path.end_node, path.distance_to_end(idx));
    }

    real_t distance_in_path(int_t path_idx, int_t idx_1, int_t idx_2) const { return m_paths[path_idx].distance_in_path(idx_1, idx_2); }

    void swap(SingleGenomeGraph& other) {
        SingleGenomeGraph tmp = std::move(*this);
        *this = std::move(other);
        other = std::move(tmp);
    }

    SingleGenomeGraph& operator=(const SingleGenomeGraph& other) = delete;
    SingleGenomeGraph& operator=(SingleGenomeGraph&& other) {
        Graph::operator=(std::move(other));
        m_paths = std::move(other.m_paths);
        m_node_map = std::move(other.m_node_map);
        return *this;
    }

private:
    struct Path {
        int_t start_node;
        int_t end_node;
        std::vector<real_t> DP;

        Path(int_t start, int_t end, std::vector<real_t>&& D) : start_node(start), end_node(end), DP(std::move(D)) { }

        real_t distance_to_start(int_t idx) const { return DP[idx]; }
        real_t distance_to_end(int_t idx) const { return DP.back() - DP[idx]; }
        real_t distance_in_path(int_t idx_1, int_t idx_2) const { return std::abs(DP[idx_1] - DP[idx_2]); }

    };
    std::vector<Path> m_paths;

    std::vector<std::pair<int_t, int_t>> m_node_map; // Map original graph indices to this graph as (path_idx, mapped_idx) pairs.

};

