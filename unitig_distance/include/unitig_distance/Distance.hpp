#pragma once

#include "types.hpp"

class Distance {
public:
    Distance() : Distance(0.0, 0) { }
    Distance(real_t distance) : Distance(distance, 1) { }
    Distance(real_t distance, int_t count) : Distance(distance, count, 0.0, (distance ? distance : REAL_T_MAX), distance) { }
    Distance(real_t distance, int_t count, real_t m2, real_t min, real_t max)
    : m_distance(distance),
      m_count(count),
      m_m2(m2),
      m_min(min),
      m_max(max)
    { }

    real_t distance() const { return m_distance; }
    int_t count() const { return m_count; }
    real_t m2() const { return count() == 0 ? -1 : m_m2; }
    real_t min() const { return count() == 0 ? -1 : m_min; }
    real_t max() const { return count() == 0 ? -1 : m_max; }

    void set_distance(real_t distance) { m_distance = distance; }
    void set_count(int_t count) { m_count = count; }

    Distance operator+(const Distance& other) {
        auto distance_1 = distance();
        auto count_1 = count();
        auto distance_2 = other.distance();
        auto count_2 = other.count();
        
        auto new_count = count_1 + count_2;
        auto new_distance = (distance_1 * count_1 + distance_2 * count_2) / new_count;
        // Need to use member variables directly here.
        auto new_m2 = m_m2 + (distance_2 - distance_1) * (distance_2 - new_distance);
        auto new_min = std::min(m_min, std::min(distance_2, other.m_min));
        auto new_max = std::max(m_max, std::max(distance_2, other.m_max));

        return Distance(new_distance, new_count, new_m2, new_min, new_max);
    }

    Distance& operator+=(const Distance& other) {
        return *this = *this + other;
    }

    operator real_t() const { return m_distance; }

private:
    real_t m_distance;
    int_t m_count;
    real_t m_m2;
    real_t m_min;
    real_t m_max;

};

