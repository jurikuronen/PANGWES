/*
    Container for GFA1 links.
*/

#pragma once

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <vector>

#include "LinkEndpoint.hpp"

using link_t = LinkEndpoint;
using links_t = std::vector<LinkEndpoint>;

class Links {
public:
    Links() : Links(0) { }
    Links(std::size_t sz) : m_links(sz), m_n_links(0) { }

    void add_link(uint64_t from_id, uint64_t to_id, char from_orient, char to_orient, uint64_t overlap) {
        if (from_id > to_id) gfa1_parser::swap(from_id, to_id, from_orient, to_orient);
        link_t new_link(to_id, overlap, gfa1_parser::get_edge_type(from_orient, to_orient));
        add_link(from_id, std::move(new_link));
    }

    void add_link(uint64_t from_id, link_t&& new_link) {
        if (from_id >= m_links.size()) resize_to_fit(from_id);
        if (!contains(from_id, new_link)) {
            m_links[from_id].push_back(std::move(new_link));
            ++m_n_links;
        }
    }

    bool contains(uint64_t from_id, const link_t& link) const {
        const auto& links = m_links[from_id];
        return std::find(links.begin(), links.end(), link) != links.end();
    }

    void clear() {
        gfa1_parser::clear(m_links);
        m_n_links = 0;
    }

    uint64_t n_links() const { return m_n_links; }

    std::size_t capacity() const { return m_links.size(); }
    void set_capacity(std::size_t capacity) { m_links.resize(capacity); }

    void write_out(const std::string& out_filename) const {
        std::ofstream ofs(out_filename);
        for (uint64_t from_id = 0; from_id < m_links.size(); ++from_id) {
            const auto& links = m_links[from_id];
            for (const auto& link : links) {
                ofs << from_id << ' ' << link.to_string() << '\n';
            }
        }
    }

private:
    std::vector<links_t> m_links;

    uint64_t m_n_links;

    void resize_to_fit(std::size_t sz) {
        std::size_t new_sz = m_links.size();
        while (new_sz <= sz) new_sz *= 2;
        set_capacity(new_sz);
    }

};


