#pragma once

#include <algorithm>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "types.hpp"
#include "Utils.hpp"

using edges_t = std::vector<std::pair<int_t, real_t>>;
using edges_itr_t = typename edges_t::iterator;
using edges_const_itr_t = typename edges_t::const_iterator;

using adj_itr_t = typename std::vector<edges_t>::iterator;
using adj_const_itr_t = typename std::vector<edges_t>::const_iterator;

class Graph {
public:
    Graph() : Graph(false, false) { }
    ~Graph() = default;
    Graph(const Graph& other) = default;
    Graph(Graph&& other)
    : m_adj(std::move(other.m_adj)),
      m_self_edge_weights(std::move(other.m_self_edge_weights)),
      m_one_based(other.m_one_based),
      m_two_sided(other.m_two_sided)
    { }

    Graph(bool one_based, bool two_sided = false) : m_adj(), m_self_edge_weights(), m_one_based(one_based), m_two_sided(two_sided) { }

    bool contains(int_t v) const { return v < (int_t) m_adj.size(); }

    void add_node() { m_adj.emplace_back(); }

    void add_edge(int_t v, int_t w, real_t weight) {
        if (v == w) return;
        auto it = find_edge(v, w);
        if (it == end(v)) {
            // New edge.
            (*this)[v].emplace_back(w, weight);
            (*this)[w].emplace_back(v, weight);
        } else {
            // Edge exists, update to shorter edge weight.
            auto current_weight = it->second;
            if (current_weight <= weight) return;
            it->second = weight;
            find_edge(w, v)->second = weight;
        }
    }

    void add_two_sided_node(real_t self_edge_weight) {
        add_node();
        add_node();
        add_edge(size() - 2, size() - 1, self_edge_weight);
        m_self_edge_weights.push_back(self_edge_weight);
    }

    bool has_edge(int_t v, int_t w) const { return find_edge(v, w) != end(v); }

    void remove_edge(int_t v, int_t w) {
        auto it = find_edge(v, w);
        if (it != end(v)) {
            (*this)[v].erase(it);
            auto it2 = find_edge(w, v);
            (*this)[w].erase(it2);
        }
    }

    void disconnect_node(int_t v) {
        auto adj_v = (*this)[v];
        for (auto w : adj_v) remove_edge(v, w.first);
    }

    int_t degree(int_t v) const { return (*this)[v].size(); }

    std::size_t size() const { return m_adj.size(); }

    void resize(std::size_t sz) { m_adj.resize(sz); }

    // Useful functions if graph stores two sides for each node.
    std::size_t true_size() const { return size() / 2; }
    int_t left_node(int_t v) const { return v * 2; }
    int_t right_node(int_t v) const { return v * 2 + 1; }
    int_t other_side(int_t v) const { return v ^ 1; }
    real_t get_self_edge_weight(int_t v) const { return two_sided() ? m_self_edge_weights[v / 2] : 0.0; }
    bool one_based() const { return m_one_based; }
    bool two_sided() const { return m_two_sided; }

    // Print details about the graph.
    void print_details() const {
        int_t n_nodes = 0, n_edges = 0, max_degree = 0;
        for (std::size_t i = 0; i < size(); ++i) {
            auto sz = degree(i);
            if (two_sided()) {
                sz += degree(i + 1);
                if (sz >= 2) sz -= 2; // Remove "self-edge" from these calculations.
                ++i;
            }
            n_nodes += sz > 0;
            n_edges += sz;
            max_degree = std::max(max_degree, sz);
        }
        std::string out_str = "Graph has " +  Utils::neat_number_str(n_nodes) + " connected" + (two_sided() ? " (half) " : " ") + "nodes and "
                            + Utils::neat_number_str(n_edges / 2) + " edges. Avg and max degree are " 
                            + Utils::neat_decimal_str(n_edges, n_nodes) + " and " + std::to_string(max_degree) + ".";
        std::cout << out_str << std::endl;
    }

    // Accessors and iterators.
    edges_t& operator[](std::size_t idx) { return m_adj[idx]; }
    const edges_t& operator[](std::size_t idx) const { return m_adj[idx]; }
    adj_itr_t begin() { return m_adj.begin(); }
    adj_itr_t end() { return m_adj.end(); }
    adj_const_itr_t begin() const { return m_adj.begin(); }
    adj_const_itr_t end() const { return m_adj.end(); }
    edges_itr_t begin(int_t v) { return m_adj[v].begin(); }
    edges_itr_t end(int_t v) { return m_adj[v].end(); }
    edges_const_itr_t begin(int_t v) const { return m_adj[v].begin(); }
    edges_const_itr_t end(int_t v) const { return m_adj[v].end(); }

    void swap(Graph& other) {
        Graph tmp = std::move(*this);
        *this = std::move(other);
        other = std::move(tmp);
    }

    Graph& operator=(const Graph& other) = delete;
    Graph& operator=(Graph&& other) {
        m_adj = std::move(other.m_adj);
        m_self_edge_weights = std::move(other.m_self_edge_weights);
        m_one_based = other.m_one_based;
        m_two_sided = other.m_two_sided;
        return *this;
    }

    // Compute the shortest distance between a source and a target.
    real_t distance(int_t source, int_t target, real_t max_distance = REAL_T_MAX) const {
        return distance(std::vector<std::pair<int_t, real_t>>{{source, 0.0}}, std::vector<int_t>{target}, max_distance).front();
    }

    // Compute shortest distance between source(s) and targets.
    std::vector<real_t> distance(
        const std::vector<std::pair<int_t, real_t>>& sources,
        const std::vector<int_t>& targets,
        real_t max_distance = REAL_T_MAX) const
    {
        std::vector<real_t> dist(size(), max_distance);

        std::vector<bool> is_target(size());
        for (auto w : targets) is_target[w] = true;
        int_t targets_left = targets.size();

        std::set<std::pair<real_t, int_t>> queue; // (distance, node) pairs.
        for (auto s : sources) {
            int_t v;
            real_t initial_distance;
            std::tie(v, initial_distance) = s;
            dist[v] = initial_distance;
            queue.emplace(initial_distance, v);
        }

        // Start search.
        while (!queue.empty()) {
            auto v = queue.begin()->second;
            queue.erase(queue.begin());
            if (is_target[v]) {
                --targets_left;
                is_target[v] = false;
                if (two_sided()) {
                    --targets_left;
                    is_target[other_side(v)] = false;
                }
                if (targets_left == 0) break; // Calculated distances for all targets.
            }            
            for (auto vw : (*this)[v]) {
                int_t w;
                real_t weight;
                std::tie(w, weight) = vw;
                if (dist[v] + weight < dist[w]) {
                    queue.erase({dist[w], w});
                    dist[w] = dist[v] + weight;
                    queue.insert({dist[w], w});
                }
            }
        }
        std::vector<real_t> target_dist;
        for (auto target : targets) target_dist.push_back(dist[target]);
        return target_dist;
    }

private:
    std::vector<edges_t> m_adj;

    // Filled if graph nodes are two-sided.
    std::vector<real_t> m_self_edge_weights;

    bool m_one_based;
    bool m_two_sided;

    edges_itr_t find_edge(int_t v, int_t w) {
        auto it = begin(v);
        while (it != end(v) && it->first != w) ++it;
        return it;
    }

    edges_const_itr_t find_edge(int_t v, int_t w) const {
        auto it = begin(v);
        while (it != end(v) && it->first != w) ++it;
        return it;
    }

};
