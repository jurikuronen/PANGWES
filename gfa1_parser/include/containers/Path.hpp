/*
    Path data type.
*/
#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

#include "LinkEndpoint.hpp"
#include "Links.hpp"

class Path {
public:
    Path(std::string&& reference, std::string&& sequence)
    : m_reference(std::move(reference)),
      m_sequence(std::move(sequence))
    { }

    const std::string& reference() const { return m_reference; }
    const std::string& sequence() const { return m_sequence; }

    const std::unordered_map<uint64_t, uint64_t>& counts() const { return m_counts; }

    void add_link(uint64_t v, uint64_t w, char v_orient, char w_orient, uint64_t overlap) {
        m_links.add_link(v, w, v_orient, w_orient, overlap);
    }

    void add_count(uint64_t v) { ++m_counts[v]; }

    std::size_t capacity() const { return m_links.capacity(); }
    void set_capacity(std::size_t capacity) { m_links.set_capacity(capacity); }

    void write_links(const std::string& out_filename) const {
        m_links.write_out(out_filename);
    }

    void write_counts(const std::string& out_filename) const {
        std::ofstream ofs(out_filename);
        auto ids = gfa1_parser::map_keys_to_vector(m_counts);
        for (auto id : ids) {
            auto count = m_counts.at(id);
            if (count > 1) ofs << id << ' ' << count << '\n';
        }
    }

    void clear() {
        m_links.clear();
        gfa1_parser::clear(m_counts);
    }

private:
    std::string m_reference;
    std::string m_sequence;

    Links m_links;

    std::unordered_map<uint64_t, uint64_t> m_counts; // Stores unitig counts for ids.

};
