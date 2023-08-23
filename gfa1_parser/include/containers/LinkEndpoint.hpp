/*
    Link data type for a link originating from from_id.
*/
#pragma once

#include <cstdint>
#include <string>
#include <utility>

using edge_type_t = std::pair<char, char>;

class LinkEndpoint {
public:
    LinkEndpoint(uint64_t to_id, uint64_t overlap, edge_type_t edge_type)
    : m_to_id(to_id),
      m_overlap(overlap),
      m_edge_type(edge_type)
    { }

    const std::string to_string() const { return std::to_string(to_id()) + ' ' + first() + second() + ' ' + std::to_string(overlap()) + 'M'; }

    bool operator==(const LinkEndpoint& other) const {
        return to_id() == other.to_id() &&
               edge_type() == other.edge_type() &&
               overlap() == other.overlap();
    }

    bool operator<(const LinkEndpoint& other) const {
        return to_id() == other.to_id() ? to_id() < other.to_id() :
               edge_type() == other.edge_type() ? edge_type() < other.edge_type() :
               overlap() < other.overlap();
    }

private:
    uint64_t m_to_id;
    uint64_t m_overlap;
    edge_type_t m_edge_type;

    uint64_t to_id() const { return m_to_id; }
    uint64_t overlap() const { return m_overlap; }
    edge_type_t edge_type() const { return m_edge_type; }

    char first() const { return m_edge_type.first; }
    char second() const { return m_edge_type.second; }

};
