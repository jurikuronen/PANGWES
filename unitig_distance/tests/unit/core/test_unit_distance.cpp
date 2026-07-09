/*
 * test_unit_distance.cpp - Unit tests for core/Distance.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <random>
#include <vector>

#include "common/test_harness/Test.hpp"
#include "common/utils/Exception.hpp"
#include "unitig_distance/core/Distance.hpp"

namespace PANGWES {
namespace {

template <typename Values = std::initializer_list<std::uint64_t>>
double test_brute_calculate_variance(const Values& values) {
    assert(values.size() >= 2 && "test_brute_calculate_variance() must be called with at least two values");

    auto sum = 0.0;
    auto sum2 = 0.0;

    for (const auto value : values) {
        sum += value;
        sum2 += value * value;
    }

    return (sum2 - sum * sum / values.size()) / (values.size() - 1);
}

bool test_unit_distance_constructor() {
    Distance distance{};

    ASSERT_EQUAL(distance.count(), 0);
    ASSERT_FALSE(distance.has_median_distance());

    return true;
}

bool test_unit_distance_mean_distance_with_no_distances_throws() {
    Distance distance{};

    EXPECT_THROW(distance.mean_distance(), ErrorCode::UNDEFINED_VALUE);

    return true;
}

bool test_unit_distance_sample_variance_with_no_distances_throws() {
    Distance distance{};

    EXPECT_THROW(distance.sample_variance(), ErrorCode::UNDEFINED_VALUE);

    return true;
}

bool test_unit_distance_min_distance_with_no_distances_throws() {
    Distance distance{};

    EXPECT_THROW(distance.min_distance(), ErrorCode::UNDEFINED_VALUE);

    return true;
}

bool test_unit_distance_max_distance_with_no_distances_throws() {
    Distance distance{};

    EXPECT_THROW(distance.max_distance(), ErrorCode::UNDEFINED_VALUE);

    return true;
}

bool test_unit_distance_median_distance_without_set_median_distance_throws() {
    Distance distance{};

    EXPECT_THROW(distance.median_distance(), ErrorCode::UNDEFINED_VALUE);

    return true;
}

bool test_unit_distance_adding_one_distance() {
    Distance distance{};

    distance.add_distance(5);

    ASSERT_EQUAL(distance.mean_distance(), 5.0);
    ASSERT_EQUAL(distance.min_distance(), 5.0);
    ASSERT_EQUAL(distance.max_distance(), 5.0);
    ASSERT_EQUAL(distance.count(), 1);

    return true;
}

bool test_unit_distance_sample_variance_for_one_distance_throws() {
    Distance distance{};

    distance.add_distance(5);

    EXPECT_THROW(distance.sample_variance(), ErrorCode::UNDEFINED_VALUE);

    return true;
}

bool test_unit_distance_adding_two_distances() {
    Distance distance{};

    distance.add_distance(5);
    distance.add_distance(10);

    const auto expected_variance = test_brute_calculate_variance({5, 10});

    ASSERT_EQUAL(distance.mean_distance(), (5 + 10) / 2.0);
    ASSERT_EQUAL(distance.sample_variance(), expected_variance);
    ASSERT_EQUAL(distance.min_distance(), 5.0);
    ASSERT_EQUAL(distance.max_distance(), 10.0);
    ASSERT_EQUAL(distance.count(), 2);

    return true;
}

bool test_unit_distance_adding_many_random_distances() {
    constexpr auto test_n = 100;
    constexpr auto test_iters = 100;
    std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<std::uint64_t> distribution(1, 10000);

    for (auto iter = 0; iter < test_iters; ++iter) {
        std::vector<std::uint64_t> sampled_distances(test_n);

        Distance distance{};
        auto expected_mean_distance = 0.0;
        auto expected_min_distance = std::numeric_limits<std::uint64_t>::max();
        auto expected_max_distance = std::numeric_limits<std::uint64_t>::min();

        std::generate(sampled_distances.begin(), sampled_distances.end(), [&]() {
            const auto new_distance = distribution(gen);

            expected_mean_distance += static_cast<double>(new_distance);
            expected_min_distance = std::min(expected_min_distance, new_distance);
            expected_max_distance = std::max(expected_max_distance, new_distance);

            distance.add_distance(new_distance);

            return new_distance;
        });

        /* Compare naively calculated expected mean and sample variance against values calculated with Welford's
           online algorithm. */
        expected_mean_distance /= static_cast<double>(test_n);
        const auto expected_variance = test_brute_calculate_variance(sampled_distances);

        ASSERT_EQUAL(distance.mean_distance(), expected_mean_distance);
        ASSERT_EQUAL(distance.sample_variance(), expected_variance);
        ASSERT_EQUAL(distance.min_distance(), expected_min_distance);
        ASSERT_EQUAL(distance.max_distance(), expected_max_distance);
        ASSERT_EQUAL(distance.count(), test_n);
    }

    return true;
}

bool test_unit_distance_set_median_distance() {
    constexpr auto test_median_distance = 10;

    Distance distance{};

    ASSERT_FALSE(distance.has_median_distance());

    distance.set_median_distance(test_median_distance);

    ASSERT_TRUE(distance.has_median_distance());
    ASSERT_EQUAL(distance.median_distance(), test_median_distance);

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_unit_distance_constructor),
        TEST(test_unit_distance_mean_distance_with_no_distances_throws),
        TEST(test_unit_distance_sample_variance_with_no_distances_throws),
        TEST(test_unit_distance_min_distance_with_no_distances_throws),
        TEST(test_unit_distance_max_distance_with_no_distances_throws),
        TEST(test_unit_distance_median_distance_without_set_median_distance_throws),
        TEST(test_unit_distance_adding_one_distance),
        TEST(test_unit_distance_sample_variance_for_one_distance_throws),
        TEST(test_unit_distance_adding_two_distances),
        TEST(test_unit_distance_adding_many_random_distances),
        TEST(test_unit_distance_set_median_distance),
    };

    return Test::run_suite("test_unit_distance", tests);
}
