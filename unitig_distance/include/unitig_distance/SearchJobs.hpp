#pragma once

#include <set>
#include <utility>
#include <vector>

#include "Queries.hpp"
#include "types.hpp"

// Distance queries for node v.
class SearchJob {
public:
    SearchJob() = delete;
    SearchJob(int_t v) : m_v(v)  { }
    const int_t v() const { return m_v; }
    const std::vector<int_t>& ws() const { return m_ws; }
    const int_t original_index(std::size_t idx) const { return m_original_indices[idx]; }
    void add(int_t w, int_t idx) {
        m_ws.push_back(w);
        m_original_indices.push_back(idx);
    }
    std::size_t size() const { return m_ws.size(); }

private:
    int_t m_v;
    std::vector<int_t> m_ws;
    std::vector<int_t> m_original_indices;

};

// Clean up later.
class SearchJobs {
public:
    SearchJobs() = delete;
    SearchJobs(const Queries& queries) : m_n_queries(queries.size()) {
        auto sz = queries.largest_v() + 1;
        // Store queries by vertex, storing also the original indices.
        std::vector<std::vector<std::pair<int_t, int_t>>> queries_map(sz);
        for (std::size_t idx = 0; idx < queries.size(); ++idx) {
            auto v = queries.v(idx);
            auto w = queries.w(idx);
            queries_map[v].emplace_back(w, idx);
            queries_map[w].emplace_back(v, idx);
        }
        // Get query counts for the vertices.
        std::set<std::pair<int_t, int_t>> n_queries_set; // (v_n_queries, v) pairs.
        std::vector<int_t> n_queries(sz);
        for (int_t v = 0; v < sz; ++v) {
            int_t v_n_queries = queries_map[v].size();
            if (v_n_queries == 0) continue;
            n_queries_set.emplace(v_n_queries, v);
            n_queries[v] = v_n_queries;
        }
        // Calculate optimal search jobs.
        std::vector<bool> processed(sz);
        while (n_queries_set.rbegin()->first != 0) { // Always points to largest value (v with most queries).
            auto it = std::prev(n_queries_set.end());
            int_t v = it->second;
            n_queries_set.erase(it);
            n_queries[v] = 0;
            processed[v] = true;
            SearchJob job(v);
            // Add remaining (v, w) queries for v.
            for (auto q : queries_map[v]) {
                int_t w, idx;
                std::tie(w, idx) = q;
                if (processed[w]) continue;
                job.add(w, idx);
                // Update query count trackers for w.
                n_queries_set.erase(std::make_pair(n_queries[w], w));
                --n_queries[w];
                n_queries_set.emplace(n_queries[w], w);
            }
            m_search_jobs.push_back(std::move(job));
        }
    }

    std::size_t size() const { return m_search_jobs.size(); }

    int_t n_queries() const { return m_n_queries; }

    SearchJob& operator[](std::size_t idx) { return m_search_jobs[idx]; }
    const SearchJob& operator[](std::size_t idx) const { return m_search_jobs[idx]; }

private:
    std::vector<SearchJob> m_search_jobs;

    int_t m_n_queries;

};
