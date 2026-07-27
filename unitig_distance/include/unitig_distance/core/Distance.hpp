/*
 * Distance.hpp - Aggregated statistics of shortest-path distances for a unitig pair across single-genome graphs (SGGs),
 *                i.e. colors of the compacted de Bruijn graph.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cstddef>
#include <cstdint>

namespace PANGWES {

/*
 * Aggregated distance statistics class for a unitig pair across SGGs.
 *
 * Note:
 * - Mean, minimum and maximum distances are defined when `count() > 0`.
 * - Sample variance is defined when `count() > 1`.
 * - Median distance is defined if set with `set_median_distance()`; check availability with `has_median_distance()`.
*/
class Distance {
public:
    // Constructs a distance object with no observed distances.
    Distance() noexcept;

    /*
     * Returns the mean shortest-path distance for the unitig pair across the SGGs.
     *
     * Throws if `count() == 0`.
    */
    double mean_distance() const;

    // Returns the number of SGGs where the unitig pair was present and connected (so a distance could be computed).
    std::size_t count() const noexcept;

    /*
     * Returns the sample variance of the shortest-path distances across the SGGs.
     *
     * Throws if `count() < 2`.
    */
    double sample_variance() const;

    /*
     * Returns the minimum shortest-path distance observed for the unitig pair across the SGGs.
     *
     * Throws if `count() == 0`.
    */
    std::uint64_t min_distance() const;

    /*
     * Returns the maximum shortest-path distance observed for the unitig pair across the SGGs.
     *
     * Throws if `count() == 0`.
    */
    std::uint64_t max_distance() const;

    // Returns true if the median distance has been calculated and set with `set_median_distance()`.
    bool has_median_distance() const noexcept;

    /*
     * Returns the median shortest-path distance for the unitig pair across the SGGs.
     *
     * Throws if the median distance has not been set with `set_median_distance()`.
     *
     * Note: Always returns an observed distance. For an even `count()`, returns the lower of the two middle distances.
    */
    std::uint64_t median_distance() const;

    /*
     * Sets the shortest-path median distance for the unitig pair across the SGGs. This value must be computed
     * externally and is not updated by `add_distance()`.
    */
    void set_median_distance(std::uint64_t median_distance) noexcept;

    /*
     * Inserts a new shortest-path distance data point computed for some SGG.
     *
     * Note: the median distance must be computed separately and stored with `set_median_distance()`.
    */
    void add_distance(std::uint64_t distance) noexcept;

private:
    double m_mean_distance;
    /*
     * Sum of squares of differences from the current mean. Used to compute the sample variance via Welford's online
     * algorithm.
    */
    double m_m2;
    std::size_t m_count;
    std::uint64_t m_min_distance;
    std::uint64_t m_max_distance;
    std::uint64_t m_median_distance;
};

} // namespace PANGWES
