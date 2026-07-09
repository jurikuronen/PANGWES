/*
 * DistanceQueryEngine.cpp - Delegates distance computation to the mean-only or median distance query engine.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <cassert>
#include <cstddef>
#include <vector>

#include "common/utils/memory.hpp"
#include "unitig_distance/core/DistanceQueryEngine.hpp"
#include "unitig_distance/core/MeanDistanceQueryEngine.hpp"
#include "unitig_distance/core/MedianDistanceQueryEngine.hpp"

namespace PANGWES {

DistanceQueryEngine::DistanceQueryEngine(const UnitigWeights& unitig_weights,
                                         const SGGEdgesFilenames& sgg_edges_filenames,
                                         QueriesReader& queries_reader,
                                         std::size_t max_memory_usage_bytes,
                                         bool no_median_distance)
    : m_mean_distance_query_engine{},
      m_median_distance_query_engine{}
{
    if (no_median_distance) {
        m_mean_distance_query_engine = Memory::make_unique<MeanDistanceQueryEngine>(unitig_weights,
                                                                                    sgg_edges_filenames,
                                                                                    queries_reader);
    } else {
        m_median_distance_query_engine = Memory::make_unique<MedianDistanceQueryEngine>(unitig_weights,
                                                                                        sgg_edges_filenames,
                                                                                        queries_reader,
                                                                                        max_memory_usage_bytes);
    }
}

DistanceQueryEngine::~DistanceQueryEngine() = default;

std::vector<Distance> DistanceQueryEngine::compute_distances() {
    if (m_median_distance_query_engine) {
        return m_median_distance_query_engine->compute_distances();
    }

    assert(m_mean_distance_query_engine && "DistanceQueryEngine not initialized");

    return m_mean_distance_query_engine->compute_distances();
}

} // namespace PANGWES
