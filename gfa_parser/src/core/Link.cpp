/*
 * Link.cpp - Link data storage.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <cassert>
#include <utility>

#include "common/type_traits/type_traits.hpp"
#include "common/utils/hash.hpp"
#include "common/utils/Exception.hpp"
#include "gfa_parser/core/Link.hpp"

namespace PANGWES {

Link::Link(std::size_t from_id,
           std::size_t to_id,
           Orientation from_orientation,
           Orientation to_orientation)
    : m_from_id{from_id},
      m_to_id{to_id},
      m_from_orientation{from_orientation},
      m_to_orientation{to_orientation}
{
    if (is_self_edge(from_id, to_id, from_orientation, to_orientation)) {
        throw Exception(ErrorCode::INVALID_ARGUMENT, "attempted to construct a self-edge");
    }

    if (m_from_id > m_to_id) {
        std::swap(m_from_id, m_to_id);

        // For strand orientations, only "Forward(+)-Forward(+)" changes to "Reverse(-)-Reverse(-)" and vice versa.
        if (m_from_orientation == m_to_orientation) {
            m_from_orientation = (m_from_orientation == Orientation::PLUS) ? Orientation::MINUS : Orientation::PLUS;
            m_to_orientation = (m_to_orientation == Orientation::PLUS) ? Orientation::MINUS : Orientation::PLUS;
        }
    }
}

std::size_t Link::from_id() const noexcept {
    return m_from_id;
}

std::size_t Link::to_id() const noexcept {
    return m_to_id;
}

Orientation Link::from_orientation() const noexcept {
    return m_from_orientation;
}

Orientation Link::to_orientation() const noexcept {
    return m_to_orientation;
}

bool Link::operator==(const Link& other) const noexcept {
    return m_from_id == other.m_from_id &&
           m_to_id == other.m_to_id &&
           m_from_orientation == other.m_from_orientation &&
           m_to_orientation == other.m_to_orientation;
}


bool Link::is_self_edge(std::size_t from_id,
                        std::size_t to_id,
                        Orientation from_orientation,
                        Orientation to_orientation)
{
    return from_id == to_id && from_orientation != to_orientation;
}

std::size_t LinkHash::operator()(const Link& link) const noexcept {
    const auto from_orientation_value = Traits::to_underlying(link.from_orientation());
    const auto to_orientation_value = Traits::to_underlying(link.to_orientation());

    assert(from_orientation_value <= 1 && "orientation value should be storable with 1 bit");
    assert(to_orientation_value <= 1 && "orientation value should be storable with 1 bit");

    // Pack orientations together to reduce hash combining.
    const auto orientation_values = (from_orientation_value << 1) | to_orientation_value;

    std::size_t seed = 0;

    Hash::combine(seed, link.from_id());
    Hash::combine(seed, link.to_id());
    Hash::combine(seed, orientation_values);

    return seed;
}

} // namespace PANGWES
