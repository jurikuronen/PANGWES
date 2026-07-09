/*
 * test_unit_sgg.cpp - Unit tests for sgg/SGG.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include "common/test_harness/Test.hpp"
#include "common/utils/Exception.hpp"
#include "mocks/MockSGGEdges.hpp"
#include "unitig_distance/sgg/SGG.hpp"

namespace PANGWES {
namespace {

bool test_unit_sgg_constructor_unexpected_mapped_path_endpoint_throws() {
    /*
     * Construct faulty SGGEdges data to trigger `try_find_path_endpoint()` finding an already mapped path endpoint:
     *     0 <-> 1
     *     2  -> 0
     *     2  -> 1
     *
     * Edges from 2 are not reciprocated, which is faulty SGGEdges data. The DFS starts from 0 and stops at the
     * 0 <-> 1 component. When the search processes node 2, it is detected as a path node with two neighbors and
     * `try_find_path_endpoint()` is called. No matter which neighbor the function tries first, it is detected as a path
     * endpoint due to having degree one. This node was already mapped by the search starting from 0, so it expected
     * that the `is_mapped()` check now fires and throws ErrorCode::INVALID_STATE.
    */
    Mocks::MockSGGEdges sgg_edges(3);
    sgg_edges[0] = {{1, 1}};
    sgg_edges[1] = {{0, 1}};
    sgg_edges[2] = {{0, 1}, {1, 1}};

    EXPECT_THROW(SGG(sgg_edges), ErrorCode::INVALID_STATE);

    return true;
}

bool test_unit_sgg_constructor_empty_data_throws() {
    // Construct an empty mocked SGGEdges.
    const auto sgg_edges = Mocks::MockSGGEdges(0);

    EXPECT_THROW(SGG(sgg_edges), ErrorCode::EMPTY_DATA);

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_unit_sgg_constructor_unexpected_mapped_path_endpoint_throws),
        TEST(test_unit_sgg_constructor_empty_data_throws),
    };

    return Test::run_suite("test_unit_sgg", tests);
}
