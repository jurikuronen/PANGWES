/*
 * DistanceQueryBatch.cpp - A batch of distance queries sharing a common source unitig.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstddef>
#include <functional>
#include <limits>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "common/utils/Exception.hpp"
#include "common/utils/memory.hpp"
#include "unitig_distance/core/DistanceQueryBatch.hpp"

namespace PANGWES {
namespace {

// Value indicating that all queries have been processed.
constexpr auto QUERY_TRACKER_FINISHED_VALUE = std::numeric_limits<std::size_t>::max();

/*
 * Helper class for greedily constructing the distance query batches.
 *
 * Maintains two data structures:
 * - `m_queries_by_unitig_id`: for each unitig, stores all associated queries and the number of queries that have not
 *                             yet been assigned to a batch.
 * - `m_unitig_ids_by_unprocessed_count`: efficiently finds the unitig with the most remaining unprocessed queries;
                                          an ordered set of (unprocessed_count, unitig_id) pairs.
*/
class UnprocessedQueryTracker {
public:
    using SizeTPair = std::pair<std::size_t, std::size_t>;

    ~UnprocessedQueryTracker() noexcept {
        Memory::clear_and_release_reserved_memory(m_queries_by_unitig_id);
        Memory::clear_and_release_reserved_memory(m_unitig_ids_by_unprocessed_count);
    }

    // Reads the queries to initialize the tracker.
    explicit UnprocessedQueryTracker(QueriesReader& queries_reader) {
        queries_reader.start_reading();

        // Read queries and aggregate query indices by unitig id.
        for (QueriesReader::QueryData query_data; queries_reader.getquery(query_data); ) {
            const auto unitig1_id = query_data.unitig1_id;
            const auto unitig2_id = query_data.unitig2_id;
            const auto query_index = queries_reader.n_queries_read() - 1;

            assert(unitig1_id != unitig2_id && "invalid distance query");
            assert(queries_reader.n_queries_read() != 0 && "unsigned integer overflow");

            m_queries_by_unitig_id[unitig1_id].queries.push_back({unitig2_id, query_index});
            m_queries_by_unitig_id[unitig2_id].queries.push_back({unitig1_id, query_index});
        }

        queries_reader.stop_reading();

        if (m_queries_by_unitig_id.empty()) {
            throw Exception(ErrorCode::FILE_EMPTY, "queries file");
        }

        // Set unprocessed query counts and insert into an ordered set.
        for (const auto& data : m_queries_by_unitig_id) {
            const auto unitig_id = data.first;
            const auto unitig_id_query_count = m_queries_by_unitig_id.at(unitig_id).queries.size();

            if (unitig_id_query_count > 0) {
                m_queries_by_unitig_id.at(unitig_id).unprocessed_queries_count = unitig_id_query_count;
                m_unitig_ids_by_unprocessed_count.emplace(unitig_id_query_count, unitig_id);
            }
        }
    }

    /*
     * Returns the unitig's ID with the most remaining unprocessed queries and marks it as processed. Returns
     * QUERY_TRACKER_FINISHED_VALUE when no unprocessed queries remain.
    */
    std::size_t pop_unitig_id_with_most_queries() {
        // Invariant: if there were any queries, the set can never be empty as 0-count entries are retained.
        assert(!m_unitig_ids_by_unprocessed_count.empty());

        const auto unitigs_by_unprocessed_count_it = m_unitig_ids_by_unprocessed_count.cbegin();

        const auto max_unprocessed_queries_count = unitigs_by_unprocessed_count_it->first;

        if (max_unprocessed_queries_count == 0) {
            return QUERY_TRACKER_FINISHED_VALUE;
        }

        // Mark unitig as processed.
        const auto unitig_id = unitigs_by_unprocessed_count_it->second;

        m_queries_by_unitig_id.at(unitig_id).unprocessed_queries_count = 0;
        m_unitig_ids_by_unprocessed_count.erase(unitigs_by_unprocessed_count_it);

        return unitig_id;
    }

    // Returns true if the given unitig still has unprocessed queries.
    bool unprocessed(std::size_t unitig_id) const {
        return m_queries_by_unitig_id.at(unitig_id).unprocessed_queries_count > 0;
    }

    // Decrements the unprocessed query count for the given unitig and updates its ordering in the priority set.
    void decrement_query_count(std::size_t unitig_id) {
        assert(m_queries_by_unitig_id.count(unitig_id) > 0);

        auto& unitig_id_unprocessed_queries_count = m_queries_by_unitig_id.at(unitig_id).unprocessed_queries_count;

        m_unitig_ids_by_unprocessed_count.erase(std::make_pair(unitig_id_unprocessed_queries_count, unitig_id));
        --unitig_id_unprocessed_queries_count;
        m_unitig_ids_by_unprocessed_count.emplace(unitig_id_unprocessed_queries_count, unitig_id);
    }

    // Returns all queries associated with the given unitig.
    const std::vector<DistanceQueryTarget>& at(std::size_t unitig_id) const {
        return m_queries_by_unitig_id.at(unitig_id).queries;
    }

private:
    struct UnitigIdQueryData {
        std::vector<DistanceQueryTarget> queries;
        std::size_t unprocessed_queries_count = 0;
    };

    std::unordered_map<std::size_t, UnitigIdQueryData> m_queries_by_unitig_id;
    std::set<SizeTPair, std::greater<SizeTPair>> m_unitig_ids_by_unprocessed_count;
};

} // namespace

DistanceQueryBatch::DistanceQueryBatch(std::size_t source_unitig_id,
                                       std::vector<DistanceQueryTarget>&& targets) noexcept
    : m_source_unitig_id{source_unitig_id},
      m_targets{std::move(targets)}
{ }

std::size_t DistanceQueryBatch::source_unitig_id() const noexcept {
    return m_source_unitig_id;
}

const std::vector<DistanceQueryTarget>& DistanceQueryBatch::targets() const noexcept {
    return m_targets;
}

std::size_t DistanceQueryBatch::reserved_bytes() const noexcept {
    return sizeof(*this) + Memory::container_reserved_bytes(m_targets);
}

std::vector<DistanceQueryBatch> DistanceQueryBatch::compute_distance_query_batches(QueriesReader& queries_reader) {
    std::vector<DistanceQueryBatch> distance_query_batches;

    auto query_tracker = UnprocessedQueryTracker(queries_reader);

    // Greedily construct batches by repeatedly selecting the unitig with the most remaining unprocessed queries.
    for (auto source_unitig_id = query_tracker.pop_unitig_id_with_most_queries();
         source_unitig_id != QUERY_TRACKER_FINISHED_VALUE;
         source_unitig_id = query_tracker.pop_unitig_id_with_most_queries())
    {
        std::vector<DistanceQueryTarget> targets;

        for (const auto& query : query_tracker.at(source_unitig_id)) {
            const auto target_unitig_id = query.target_unitig_id;
            const auto query_idx = query.query_index;

            if (query_tracker.unprocessed(target_unitig_id)) {
                targets.push_back({target_unitig_id, query_idx});
                query_tracker.decrement_query_count(target_unitig_id);
            }
        }

        if (!targets.empty()) {
            distance_query_batches.emplace_back(source_unitig_id, std::move(targets));
        }
    }

    return distance_query_batches;
}

std::size_t maximum_batch_n_queries(const std::vector<DistanceQueryBatch>& distance_query_batches) noexcept {
    if (distance_query_batches.empty()) {
        return 0;
    }

    return std::max_element(distance_query_batches.begin(),
                            distance_query_batches.end(),
                            [](const DistanceQueryBatch& lhs, const DistanceQueryBatch& rhs)
    {
        return lhs.targets().size() < rhs.targets().size();
    })->targets().size();
}

} // namespace PANGWES
