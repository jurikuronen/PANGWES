/*
 * SGGEdges.cpp - Class for storing edge data of a single-genome graph (SGG).
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <exception>
#include <tuple>
#include <utility>

#include "common/io/Log.hpp"
#include "common/utils/Exception.hpp"
#include "common/utils/memory.hpp"
#include "common/utils/utils.hpp"
#include "unitig_distance/sgg/SGGEdges.hpp"
#include "unitig_distance/sgg/sgg_utils.hpp"

namespace PANGWES {
namespace {

enum class Orientation : std::uint8_t {
    Forward,
    Reverse
};

struct EdgeOrientation {
    Orientation from;
    Orientation to;
};

// Parses an orientation from an input character.
inline Orientation parse_orientation(char orientation_char) {
    switch (orientation_char) {
        case 'F': return Orientation::Forward;
        case 'R': return Orientation::Reverse;
        default: throw Exception(ErrorCode::INVALID_DATA, "invalid orientation: ", orientation_char);
    }
}

// Parses orientations from the edge orientation input field.
inline EdgeOrientation read_orientations(const std::string& field) {
    if (field.size() != 2) {
        throw Exception(ErrorCode::INVALID_DATA, "invalid orientation");
    }

    EdgeOrientation orientation;

    orientation.from = parse_orientation(field[0]);
    orientation.to = parse_orientation(field[1]);

    return orientation;
}

/*
 * Converts a de Bruijn graph edge between two canonical unitigs into the corresponding endpoint nodes of the genome
 * sequence graph.
 *
 * Each unitig with id U is represented by two nodes: its left (U * 2) and right (U * 2 + 1) sides.
 *
 * Edge orientation encodes which sides of the unitigs are connected:
 * - FF: Forward strand to forward strand
 * - FR: Forward strand to reverse strand
 * - RF: Reverse strand to forward strand
 * - RR: Reverse strand to reverse strand
*/
inline std::pair<std::size_t, std::size_t> dbg_edge_to_nodes(std::size_t unitig_from,
                                                             std::size_t unitig_to,
                                                             Orientation from_orientation,
                                                             Orientation to_orientation)
{
    return {
        // F* edge means the edge connects to the from-unitig's "right" side.
        from_orientation == Orientation::Forward ? SGGUtils::unitig_to_right_node(unitig_from)
                                                 : SGGUtils::unitig_to_left_node(unitig_from),
        // *R edge means the edge connects to the to-unitig's "right" side.
        to_orientation == Orientation::Reverse ? SGGUtils::unitig_to_right_node(unitig_to)
                                               : SGGUtils::unitig_to_left_node(unitig_to)
    };
}

inline void add_self_edge(std::size_t node, AdjacencyListT& adj, const UnitigWeights& unitig_weights) {
    const auto node_other_side = SGGUtils::node_other_side(node);

    assert(node_other_side < adj.size() && adj[node_other_side].empty());

    const auto unitig_id = SGGUtils::node_to_unitig(node);

    if (!unitig_weights.contains(unitig_id)) {
        throw Exception(ErrorCode::INVALID_UNITIG_ID);
    }

    SGGUtils::add_edge(adj, node, node_other_side, unitig_weights.weight(unitig_id));
}

} // namespace

SGGEdges::SGGEdges(std::unique_ptr<FileReaderInterface> reader, const UnitigWeights& unitig_weights)
    : m_adj{},
      m_n_nodes{}
{
    constexpr auto N_REQUIRED_EDGES_FIELDS = 3;
    constexpr auto UNITIG1_ID_FIELD = 0;
    constexpr auto UNITIG2_ID_FIELD = 1;
    constexpr auto ORIENTATION_FIELD = 2;
    constexpr auto DEPRECATED_OVERLAP_FIELD = 3;

    AdjacencyListT adj(unitig_weights.size() * 2);
    std::size_t n_nodes = 0;

    reader->open();

    for (std::string line; reader->getline(line); ) {
        if (line.empty()) {
            continue;
        }

        const auto fields = Utils::get_fields_ws(line);

        // Expect lines to be formatted as [unitig1_id unitig2_id orientation (overlap)] with overlap optional.
        if (fields.size() < N_REQUIRED_EDGES_FIELDS) {
            throw Exception(ErrorCode::FILE_WRONG_COLUMN_COUNT,
                            "SGG edges file \"", reader->filename(), "\" line ", reader->line_number());
        }

        try {
            // Older versions of gfa_parser output "0M" into the edges files.
            if (fields.size() == N_REQUIRED_EDGES_FIELDS + 1 && fields.at(DEPRECATED_OVERLAP_FIELD) == "0M") {
                continue;
            }

            const auto unitig_from = std::stoll(fields[UNITIG1_ID_FIELD]);
            const auto unitig_to = std::stoll(fields[UNITIG2_ID_FIELD]);

            if (unitig_from < 0 || unitig_to < 0) {
                throw Exception(ErrorCode::INVALID_UNITIG_ID);
            }

            const auto edge_orientation = read_orientations(fields[ORIENTATION_FIELD]);

            std::size_t node_from{};
            std::size_t node_to{};
            std::tie(node_from, node_to) = dbg_edge_to_nodes(static_cast<std::size_t>(unitig_from),
                                                             static_cast<std::size_t>(unitig_to),
                                                             edge_orientation.from,
                                                             edge_orientation.to);
            // Self-edges not allowed.
            if (node_from == node_to) {
                continue;
            }

            if (adj.at(node_from).empty()) {
                add_self_edge(node_from, adj, unitig_weights);

                n_nodes += 2;
            }

            if (adj.at(node_to).empty()) {
                add_self_edge(node_to, adj, unitig_weights);

                n_nodes += 2;
            }

            SGGUtils::add_edge(adj, node_from, node_to, 1);

        } catch (const std::exception& exception) {
            throw Exception(ErrorCode::FILE_BAD_DATA,
                            "SGG edges file \"", reader->filename(), "\" line ", reader->line_number(),
                            ": ", exception.what());
        }
    }

    reader->close();

    if (n_nodes == 0) {
        throw Exception(ErrorCode::FILE_EMPTY, "SGG edges file \"", reader->filename(), "\"");
    }

    m_n_nodes = n_nodes;
    m_adj = std::move(adj);
}

std::size_t SGGEdges::n_nodes() const noexcept {
    return m_n_nodes;
}

std::size_t SGGEdges::size() const noexcept {
    return m_adj.size();
}

std::size_t SGGEdges::capacity() const noexcept {
    return m_adj.capacity();
}

std::size_t SGGEdges::reserved_bytes() const noexcept {
    return sizeof(*this) + Memory::container_reserved_bytes(m_adj);
}

bool SGGEdges::empty() const noexcept {
    return n_nodes() == 0;
}

bool SGGEdges::contains(std::size_t unitig_id) const noexcept {
    const auto unitig_left_node = SGGUtils::unitig_to_left_node(unitig_id);

    return unitig_left_node < size() && !m_adj[unitig_left_node].empty();
}

EdgeListT& SGGEdges::operator[](std::size_t idx) {
    return m_adj[idx];
}

const EdgeListT& SGGEdges::operator[](std::size_t idx) const {
    return m_adj[idx];
}

} // namespace PANGWES
