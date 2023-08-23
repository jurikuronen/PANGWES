#pragma once

#include <algorithm>
#include <functional>
#include <vector>

#include "Distance.hpp"
#include "types.hpp"

class DistanceVector {
public:
    DistanceVector() : m_mean_distances(false) { }
    DistanceVector(const DistanceVector& other) = default;
    DistanceVector(DistanceVector&& other) = default;

    DistanceVector(std::size_t sz) : m_distances(sz), m_mean_distances(false) { }
    DistanceVector(std::size_t sz, real_t distance_value) : m_distances(sz, Distance(distance_value)), m_mean_distances(false) { }
    DistanceVector(std::size_t sz, real_t distance_value, int_t count_value) : m_distances(sz, Distance(distance_value, count_value)), m_mean_distances(false) { }

    std::vector<real_t> distances() const {
        std::vector<real_t> vector(size());
        std::transform(begin(), end(), vector.begin(), std::mem_fn(&Distance::distance));
        return vector;
    }

    std::vector<int_t> counts() const {
        std::vector<int_t> vector(size());
        std::transform(begin(), end(), vector.begin(), std::mem_fn(&Distance::count));
        return vector;
    }

    void emplace_back(real_t distance, int_t count = 1) { m_distances.emplace_back(distance, count); }

    void resize(std::size_t sz) { m_distances.resize(sz); }

    std::size_t size() const { return m_distances.size(); }

    bool storing_mean_distances() const { return m_mean_distances; }
    void set_mean_distances(bool value = true) { m_mean_distances = value; }

    Distance& operator[](std::size_t idx) { return m_distances[idx]; }
    const Distance& operator[](std::size_t idx) const { return m_distances[idx]; }

    typename std::vector<Distance>::iterator begin() { return m_distances.begin(); }
    typename std::vector<Distance>::iterator end() { return m_distances.end(); }
    typename std::vector<Distance>::const_iterator begin() const { return m_distances.begin(); }
    typename std::vector<Distance>::const_iterator end() const { return m_distances.end(); }

private:
    std::vector<Distance> m_distances;

    bool m_mean_distances;

};
