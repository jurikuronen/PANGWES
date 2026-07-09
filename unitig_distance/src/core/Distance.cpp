/*
 * Distance.cpp - Aggregated statistics of shortest-path distances for a unitig pair across single-genome graphs (SGGs),
 *                i.e. colors of the compacted de Bruijn graph.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <algorithm>
#include <limits>

#include "common/utils/Exception.hpp"
#include "unitig_distance/core/Distance.hpp"

namespace PANGWES {

Distance::Distance() noexcept
    : m_mean_distance{},
      m_m2{},
      m_count{},
      m_min_distance{std::numeric_limits<std::uint64_t>::max()},
      m_max_distance{std::numeric_limits<std::uint64_t>::min()},
      m_median_distance{std::numeric_limits<std::uint64_t>::max()}
{ }

double Distance::mean_distance() const {
    if (count() == 0) {
        throw Exception(ErrorCode::UNDEFINED_VALUE, "mean distance undefined when `count() == 0`");
    }

    return m_mean_distance;
}

std::size_t Distance::count() const noexcept {
    return m_count;
}

double Distance::sample_variance() const {
    const auto n_distances = count();

    if (n_distances < 2) {
        throw Exception(ErrorCode::UNDEFINED_VALUE, "sample variance undefined when `count() < 2`");
    }

    return m_m2 / (static_cast<double>(n_distances - 1));
}

std::uint64_t Distance::min_distance() const {
    if (count() == 0) {
        throw Exception(ErrorCode::UNDEFINED_VALUE, "minimum distance undefined when `count() == 0`");
    }

    return m_min_distance;
}

std::uint64_t Distance::max_distance() const {
    if (count() == 0) {
        throw Exception(ErrorCode::UNDEFINED_VALUE, "maximum distance undefined when `count() == 0`");
    }

    return m_max_distance;
}

bool Distance::has_median_distance() const noexcept {
    return m_median_distance != std::numeric_limits<std::uint64_t>::max();
}

std::uint64_t Distance::median_distance() const {
    if (!has_median_distance()) {
        throw Exception(ErrorCode::UNDEFINED_VALUE,
                        "median distance undefined when not set with `set_median_distance()`");
    }

    return m_median_distance;
}

void Distance::set_median_distance(std::uint64_t median_distance) noexcept {
    m_median_distance = median_distance;
}

void Distance::add_distance(std::uint64_t distance) noexcept {
    // Difference between the new distance and the previous mean distance.
    const auto distance_delta = static_cast<double>(distance) - m_mean_distance;

    // Increment number of observations.
    ++m_count;

    // Update the mean distance using the already-incremented new count.
    m_mean_distance += distance_delta / static_cast<double>(m_count);

    // Update the sum of squared deviations using the already-updated new mean.
    m_m2 += distance_delta * (static_cast<double>(distance) - m_mean_distance);

    // Update observed minimum and maximum distances.
    m_min_distance = std::min(m_min_distance, distance);
    m_max_distance = std::max(m_max_distance, distance);
}

} // namespace PANGWES
