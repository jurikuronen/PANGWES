/*
 * DistanceQueryEngine.hpp - Delegates distance computation to the mean-only or median distance query engine.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "unitig_distance/core/Distance.hpp"
#include "unitig_distance/core/UnitigWeights.hpp"
#include "unitig_distance/io/QueriesReader.hpp"
#include "unitig_distance/sgg/SGGEdgesFilenames.hpp"

namespace PANGWES {

// Forward declarations.
class MeanDistanceQueryEngine;
class MedianDistanceQueryEngine;

// Distance computation delegator class.
class DistanceQueryEngine {
public:
    /*
     * Constructs the requested distance query engine based on `no_median_distance`.
     *
     * The selected engine will additionally read and compact the queries during construction.
    */
    DistanceQueryEngine(const UnitigWeights& unitig_weights,
                        const SGGEdgesFilenames& sgg_edges_filenames,
                        QueriesReader& queries_reader,
                        std::size_t max_memory_usage_bytes,
                        bool no_median_distance);
    ~DistanceQueryEngine();

    DistanceQueryEngine(const DistanceQueryEngine&) = delete;
    DistanceQueryEngine& operator=(const DistanceQueryEngine&) = delete;
    DistanceQueryEngine(DistanceQueryEngine&&) = delete;
    DistanceQueryEngine& operator=(DistanceQueryEngine&&) = delete;

    // Computes and returns all distances for the queries read by the constructor using the selected engine.
    std::vector<Distance> compute_distances();

private:
    std::unique_ptr<MeanDistanceQueryEngine> m_mean_distance_query_engine;
    std::unique_ptr<MedianDistanceQueryEngine> m_median_distance_query_engine;
};

} // namespace PANGWES
