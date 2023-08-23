/*
    Container for GFA1 paths.
*/

#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Path.hpp"

class Paths {
public:
    Paths() : m_n_segments(0) { }

    uint64_t n_segments() const { return m_n_segments; }
    void set_n_segments(uint64_t n_segments) { m_n_segments = n_segments; }

    Path& path(const std::string& reference) { return (*this)[mapped_idx(reference)]; }
    const Path& path(const std::string& reference) const { return (*this)[mapped_idx(reference)]; }

    uint64_t mapped_idx(const std::string& reference) const { return m_path_map.at(reference); }

    bool contains(const std::string& reference) const { return m_path_map.find(reference) != m_path_map.end(); }

    uint64_t add_path(std::string&& reference, std::string&& sequence) {
        uint64_t mapped_idx = size();
        m_path_map.emplace(reference, mapped_idx);
        m_paths.emplace_back(std::move(reference), std::move(sequence));
        return mapped_idx;
    }

    std::pair<std::string, std::string> parse_cuttlefish_reference_and_sequence(const std::string& pathname) const {
        static const std::string reference_start_identifier = "Reference:";
        static const std::string sequence_start_identifier = "_Sequence:";
        static const uint64_t reference_skip_ahead = reference_start_identifier.size();
        static const uint64_t sequence_skip_ahead = sequence_start_identifier.size();
        auto reference_start_idx = pathname.find(reference_start_identifier); // Should be 0.
        auto sequence_start_idx = pathname.find(sequence_start_identifier);
        if (reference_start_idx == std::string::npos || sequence_start_idx == std::string::npos) {
            return std::make_pair(std::string(), std::string());
        }
        uint64_t reference_start = reference_start_idx + reference_skip_ahead;
        uint64_t reference_length = sequence_start_idx - reference_start;
        uint64_t sequence_start = sequence_start_idx + sequence_skip_ahead;
        std::string reference = pathname.substr(reference_start, reference_length);
        std::string sequence = pathname.substr(sequence_start);
        return std::make_pair(reference, sequence);
    }

    std::size_t size() const { return m_paths.size(); }

    Path& operator[](std::size_t idx) { return m_paths[idx]; }
    const Path& operator[](std::size_t idx) const { return m_paths[idx]; }

    void add_counts(const std::unordered_map<uint64_t, uint64_t>& counts) {
        for (const auto& x : counts) {
            uint64_t unitig, unitig_count;
            std::tie(unitig, unitig_count) = x;
            m_counts_all[unitig] = std::max(m_counts_all[unitig], unitig_count);
        }
    }

    void write_counts(const std::string& counts_filename) const {
        std::ofstream ofs(counts_filename);
        auto counts = gfa1_parser::map_to_vector(m_counts_all, m_n_segments);
        for (uint64_t idx = 0; idx < m_n_segments; ++idx) {
            if (counts[idx] > 1) ofs << idx << ' ' << counts[idx] << '\n';
        }
    }

    void write_to_fasta(std::ofstream& ofs, const Path& path) {
        ofs << '>' << path.sequence() << '\n';
        static const uint64_t FASTA_COLS = 60;
        auto counts = gfa1_parser::map_to_vector(path.counts(), m_n_segments);
        for (uint64_t idx = 0; idx < m_n_segments; ++idx) {
            static const std::vector<char> chars{'a', 'c'};
            ofs << chars[counts[idx] > 0];
            if ((idx + 1) % FASTA_COLS == 0) ofs << '\n';
        }
        if (m_n_segments % FASTA_COLS != 0) ofs << '\n';
    }

private:
    std::vector<Path> m_paths;
    std::unordered_map<std::string, uint64_t> m_path_map;

    std::unordered_map<uint64_t, uint64_t> m_counts_all;

    uint64_t m_n_segments;

};


