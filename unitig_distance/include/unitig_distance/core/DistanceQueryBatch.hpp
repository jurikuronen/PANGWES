/*
 * DistanceQueryBatch.hpp - A batch of distance queries sharing a common source unitig.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cstddef>
#include <vector>

#include "unitig_distance/io/QueriesReader.hpp"

namespace PANGWES {

// Pair consisting of a target unitig's ID and the index of the corresponding query.
struct DistanceQueryTarget {
    std::size_t target_unitig_id;
    std::size_t query_index;
};

/*
 * Represents a batch of distance queries that share a common source unitig.
 *
 * Batching queries by source allows running a single shortest-path computation per source (per single-genome graph) to
 * answer a lot of queries at once.
*/
class DistanceQueryBatch {
public:
    // Constructs a batch of distance queries originating from a single source unitig.
    DistanceQueryBatch(std::size_t source_unitig_id, std::vector<DistanceQueryTarget>&& targets) noexcept;

    DistanceQueryBatch(const DistanceQueryBatch&) = delete;
    DistanceQueryBatch& operator=(const DistanceQueryBatch&) = delete;
    DistanceQueryBatch(DistanceQueryBatch&&) noexcept = default;
    DistanceQueryBatch& operator=(DistanceQueryBatch&&) noexcept = default;

    // Returns the source unitig's ID from which distances are computed.
    std::size_t source_unitig_id() const noexcept;

    // Returns the list of target unitigs and associated query indices for this batch.
    const std::vector<DistanceQueryTarget>& targets() const noexcept;

    // Returns the number of bytes of dynamic storage reserved by this object (excludes allocator overhead).
    std::size_t reserved_bytes() const noexcept;

    /*
     * Reads distance queries and computes distance query batches that greatly reduce the number of graph-distance
     * computations required to answer all queries.
     *
     * After reading all queries, the algorithm greedily selects source unitigs that participate in the largest number
     * of remaining unprocessed queries and groups those queries into a single batch.
     *
     * Each query index will appear in exactly one batch.
     *
     * Throws on queries reading failures, including an empty queries file.
    */
    static std::vector<DistanceQueryBatch> compute_distance_query_batches(QueriesReader& queries_reader);

private:
    std::size_t m_source_unitig_id;
    std::vector<DistanceQueryTarget> m_targets;
};

// Returns the largest number of queries in a batch, or zero if the batches are empty.
std::size_t maximum_batch_n_queries(const std::vector<DistanceQueryBatch>& distance_query_batches) noexcept;

} // namespace PANGWES
