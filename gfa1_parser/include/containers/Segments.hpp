/*
    Container for (Name, Sequence) pairs given in the GFA1 segment lines.
    The GFA1 Names are mapped to 0, ..., n_segments-1, which will be the new ids for each Sequence.
*/
#pragma once

#include <cstdint>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

class Segments {
public:
    Segments() = default;

    std::string& sequence(const std::string& GFA1_NAME) { return (*this)[mapped_idx(GFA1_NAME)]; }
    const std::string& sequence(const std::string& GFA1_NAME) const { return (*this)[mapped_idx(GFA1_NAME)]; }

    uint64_t mapped_idx(const std::string& GFA1_NAME) const {
        return m_name_map.at(GFA1_NAME);
    }

    bool contains(const std::string& GFA1_NAME) const {
        return m_name_map.find(GFA1_NAME) != m_name_map.end();
    }

    uint64_t map_name(std::string&& GFA1_NAME) {
        return map_name_and_sequence(std::move(GFA1_NAME), "");
    }

    uint64_t map_name_and_sequence(std::string&& GFA1_NAME, std::string&& GFA1_SEQUENCE) {
        uint64_t mapped_idx = size();
        m_name_map.emplace(std::move(GFA1_NAME), mapped_idx);
        m_sequences.push_back(std::move(GFA1_SEQUENCE));
        return mapped_idx;
    }

    std::size_t size() const { return m_sequences.size(); }

    std::string& operator[](std::size_t idx) { return m_sequences[idx]; }
    const std::string& operator[](std::size_t idx) const { return m_sequences[idx]; }

    void write_out(const std::string& out_filename) const {
        std::ofstream ofs(out_filename);
        for (std::size_t idx = 0; idx < size(); ++idx) {
            ofs << idx << ' ' << (*this)[idx] << '\n';
        }
    }

    // Needed for error-checking.
    std::string idx_to_gfa1_name(std::size_t idx) {
        for (const auto& x : m_name_map) if (x.second == idx) return x.first;
        return "";
    }

private:
    std::vector<std::string> m_sequences;
    std::unordered_map<std::string, std::size_t> m_name_map;

};

