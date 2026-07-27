/*
 * test_unit_link.cpp - Unit tests for core/Link.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <cstddef>
#include <vector>

#include "common/test_harness/Test.hpp"
#include "common/utils/Exception.hpp"
#include "gfa_parser/core/Link.hpp"
#include "gfa_parser/core/Orientation.hpp"

namespace PANGWES {
namespace {

bool check_link(const Link& link,
                std::size_t expected_from_id,
                std::size_t expected_to_id,
                Orientation expected_from_orientation,
                Orientation expected_to_orientation) noexcept
{
    ASSERT_EQUAL(link.from_id(), expected_from_id);
    ASSERT_EQUAL(link.to_id(), expected_to_id);
    ASSERT_ENUMS_EQUAL(link.from_orientation(), expected_from_orientation);
    ASSERT_ENUMS_EQUAL(link.to_orientation(), expected_to_orientation);

    return true;
}

bool test_unit_link_constructor() {
    constexpr auto from_id = 0;
    constexpr auto to_id = 1;
    constexpr auto from_orientation = Orientation::PLUS;
    constexpr auto to_orientation = Orientation::MINUS;

    const Link link(from_id, to_id, from_orientation, to_orientation);

    return check_link(link, from_id, to_id, from_orientation, to_orientation);
}

bool test_unit_link_constructor_sorts_ids() {
    constexpr auto from_id = 1;
    constexpr auto to_id = 0;

    const auto link_pp = Link(from_id, to_id, Orientation::PLUS, Orientation::PLUS);
    const auto link_pm = Link(from_id, to_id, Orientation::PLUS, Orientation::MINUS);
    const auto link_mp = Link(from_id, to_id, Orientation::MINUS, Orientation::PLUS);
    const auto link_mm = Link(from_id, to_id, Orientation::MINUS, Orientation::MINUS);

           // ++ swapped to --.
    return check_link(link_pp, to_id, from_id, Orientation::MINUS, Orientation::MINUS) &&
           // +- kept as +-.
           check_link(link_pm, to_id, from_id, Orientation::PLUS, Orientation::MINUS) &&
           // -+ kept as -+.
           check_link(link_mp, to_id, from_id, Orientation::MINUS, Orientation::PLUS) &&
           // -- swapped to ++.
           check_link(link_mm, to_id, from_id, Orientation::PLUS, Orientation::PLUS);
}

bool test_unit_link_is_self_edge() {
    ASSERT_FALSE(Link::is_self_edge(0, 0, Orientation::PLUS, Orientation::PLUS));
    ASSERT_TRUE(Link::is_self_edge(0, 0, Orientation::PLUS, Orientation::MINUS));
    ASSERT_TRUE(Link::is_self_edge(0, 0, Orientation::MINUS, Orientation::PLUS));
    ASSERT_FALSE(Link::is_self_edge(0, 0, Orientation::MINUS, Orientation::MINUS));

    return true;
}


bool test_unit_link_constructor_self_edge_throws() {
    EXPECT_THROW(Link(0, 0, Orientation::PLUS, Orientation::MINUS), ErrorCode::INVALID_ARGUMENT);

    return true;
}

bool test_unit_link_equal_operator() {
    constexpr auto id1 = 0;
    constexpr auto id2 = 1;
    constexpr auto id3 = 2;

    const std::vector<Link> links{
        Link(id1, id2, Orientation::PLUS, Orientation::PLUS),
        Link(id1, id2, Orientation::PLUS, Orientation::MINUS),
        Link(id1, id2, Orientation::MINUS, Orientation::PLUS),
        Link(id1, id2, Orientation::MINUS, Orientation::MINUS),
        Link(id1, id3, Orientation::PLUS, Orientation::PLUS),
        Link(id1, id3, Orientation::PLUS, Orientation::MINUS),
        Link(id1, id3, Orientation::MINUS, Orientation::PLUS),
        Link(id1, id3, Orientation::MINUS, Orientation::MINUS),
        Link(id2, id3, Orientation::PLUS, Orientation::PLUS),
        Link(id2, id3, Orientation::PLUS, Orientation::MINUS),
        Link(id2, id3, Orientation::MINUS, Orientation::PLUS),
        Link(id2, id3, Orientation::MINUS, Orientation::MINUS),
    };

    for (std::size_t link1_idx = 0; link1_idx < links.size(); ++link1_idx) {
        for (std::size_t link2_idx = 0; link2_idx < links.size(); ++link2_idx) {
            if (link1_idx == link2_idx) {
                ASSERT_TRUE(links[link1_idx] == links[link2_idx]);
            } else {
                ASSERT_FALSE(links[link1_idx] == links[link2_idx]);
            }
        }
    }

    return true;
}

bool test_unit_link_hash() {
    const auto link = Link(0, 1, Orientation::PLUS, Orientation::PLUS);
    const auto different_link = Link(0, 2, Orientation::MINUS, Orientation::MINUS);

    ASSERT_EQUAL(LinkHash()(link), LinkHash()(link));
    ASSERT_NOT_EQUAL(LinkHash()(link), LinkHash()(different_link));

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_unit_link_constructor),
        TEST(test_unit_link_constructor_sorts_ids),
        TEST(test_unit_link_is_self_edge),
        TEST(test_unit_link_constructor_self_edge_throws),
        TEST(test_unit_link_equal_operator),
        TEST(test_unit_link_hash),
    };

    return Test::run_suite("test_unit_link", tests);
}
