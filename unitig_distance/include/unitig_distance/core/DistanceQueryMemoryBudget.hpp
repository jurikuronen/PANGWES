/*
 * DistanceQueryMemoryBudget.hpp - Class for calculating the memory budget for distance-query computations.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cstddef>

#include "unitig_distance/core/UnitigWeights.hpp"
#include "unitig_distance/sgg/SGGEdgesFilenames.hpp"

namespace PANGWES {

/*
 * Calculates the distance-matrix memory budget used by MedianDistanceQueryEngine.
 *
 * SGG memory usage is measured by constructing SGGs selected by descending edges-file size.
 *
 * Accounts for a small amount of dynamic memory allowance.
 *
 * Because PANGWES does not implement its own allocator, exact memory budgeting is difficult. The memory budget
 * therefore uses only 90% of the memory limit.
*/
class DistanceQueryMemoryBudget {
public:
    DistanceQueryMemoryBudget() = delete;

    // Estimates SGG memory usage by constructing a batch of SGGs selected by descending edges-file size.
    static void estimate_sgg_memory_usage(const UnitigWeights& unitig_weights, const SGGEdgesFilenames& sgg_edges_filenames);

    // Waits for SGG memory estimation and calculates the maximum distance matrix rows from the available memory.
    static std::size_t maximum_distance_matrix_rows(std::size_t max_memory_usage_bytes,
                                                    std::size_t base_memory_bytes,
                                                    std::size_t matrix_columns,
                                                    std::size_t max_matrix_rows);
};

} // namespace PANGWES
