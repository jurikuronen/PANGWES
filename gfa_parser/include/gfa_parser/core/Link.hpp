/*
 * Link.hpp - Link data storage.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cstddef>

#include "gfa_parser/core/Orientation.hpp"

namespace PANGWES {

// Stores link data between two segments (`L` record without `Overlap`).
class Link {
public:
    /*
     * Constructs a Link between segments from the given ids in sorted order. That is, if `from_id > to_id`, swaps the
     * ids and orientations.
     *
     * Throws if the given ids and orientations result in a self-edge.
    */
    Link(std::size_t from_id, std::size_t to_id, Orientation from_orientation, Orientation to_orientation);

    Link(const Link&) = default;
    Link& operator=(const Link&) = default;
    Link(Link&&) noexcept = default;
    Link& operator=(Link&&) noexcept = default;

    // Returns the id of the "from" segment.
    std::size_t from_id() const noexcept;

    // Returns the id of the "to" segment.
    std::size_t to_id() const noexcept;

    // Returns the orientation of the "from" segment.
    Orientation from_orientation() const noexcept;

    // Returns the orientation of the "to" segment.
    Orientation to_orientation() const noexcept;

    // Equality comparison for Link.
    bool operator==(const Link& other) const noexcept;

    /*
     * Returns true if the given ids and orientations result in a self-edge. That is:
     * - `from_id == to_id`.
     * - `from_orientation != to_orientation` (describes a connection from a side of the segment to that same side).
    */
    static bool is_self_edge(std::size_t from_id,
                             std::size_t to_id,
                             Orientation from_orientation,
                             Orientation to_orientation);

private:
    std::size_t m_from_id;
    std::size_t m_to_id;
    Orientation m_from_orientation;
    Orientation m_to_orientation;
};

// Hash function for ConcurrentSet.
struct LinkHash {
    std::size_t operator()(const Link& link) const noexcept;
};

} // namespace PANGWES
