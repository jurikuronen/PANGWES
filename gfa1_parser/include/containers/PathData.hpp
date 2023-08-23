/*
    Container for data parsed from a GFA1 path line.
*/
#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

class PathData {
public:
    PathData() : m_n_path_lines(0) { }

    void add_path_data(uint64_t path_idx, std::string&& segment_names, std::string&& overlaps) {
        if (n_paths() == path_idx) {
            // Make space for new path.
            m_segment_names.emplace_back();
            m_overlaps.emplace_back();
        }
        m_segment_names[path_idx].push_back(std::move(segment_names));
        m_overlaps[path_idx].push_back(std::move(overlaps));
        ++m_n_path_lines;
    }

    uint64_t n_path_lines() const { return m_n_path_lines; }
    uint64_t n_path_lines(std::size_t path_idx) const { return m_segment_names[path_idx].size(); }
    uint64_t n_paths() const { return m_segment_names.size(); }

    const std::string& segment_names(std::size_t path_idx, std::size_t idx) const { return m_segment_names[path_idx][idx]; }
    const std::string& overlaps(std::size_t path_idx, std::size_t idx) { return m_overlaps[path_idx][idx]; }

    void clear(std::size_t path_idx) {
        gfa1_parser::clear(m_segment_names[path_idx]);
        gfa1_parser::clear(m_overlaps[path_idx]);
        m_n_path_lines = 0;
    }

private:
    std::vector<std::vector<std::string>> m_segment_names;
    std::vector<std::vector<std::string>> m_overlaps;

    uint64_t m_n_path_lines;

};
