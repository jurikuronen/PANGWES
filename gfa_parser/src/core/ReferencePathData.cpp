/*
 * ReferencePathData.cpp - Class for storing path data associated with one reference sequence.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <atomic>
#include <cassert>
#include <cstddef>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "common/utils/Exception.hpp"
#include "gfa_parser/core/ReferencePathData.hpp"

namespace PANGWES {

ReferencePathData::ReferencePathData()
    : m_links{},
      m_unitig_occurrence_data{},
      m_string_for_fasta_header{},
      m_sequence_count{},
      m_mutex{}
{ }

ReferencePathData::ReferencePathData(const std::string& reference_name, const std::string& first_sequence_name)
    : m_links{},
      m_unitig_occurrence_data{},
      m_string_for_fasta_header{reference_name + '_' + first_sequence_name},
      m_sequence_count{},
      m_mutex{}
{ }

void ReferencePathData::initialize_unitig_occurrence_data(std::size_t n_segments) {
    if (n_segments == 0) {
        throw Exception(ErrorCode::INVALID_ARGUMENT,
                        "attempting to initialize unitig occurrence data size to 0 segments");
    }

    m_unitig_occurrence_data.resize(n_segments);
}

void ReferencePathData::clear_and_release_reserved_memory() {
    m_links.clear_and_release_reserved_memory();
}

const ConcurrentSet<Link, LinkHash>& ReferencePathData::links() const noexcept {
    return m_links;
}

const std::string& ReferencePathData::string_for_fasta_header() const noexcept {
    return m_string_for_fasta_header;
}

std::vector<bool> ReferencePathData::take_unitig_occurrence_data() {
    return std::move(m_unitig_occurrence_data);
}

void ReferencePathData::mark_unitig_presence(const std::vector<std::size_t>& segment_ids) {
    if (m_unitig_occurrence_data.empty()) {
        throw Exception(ErrorCode::INVALID_STATE, "uninitialized unitig occurrence data");
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    // Segment IDs are consecutive from 0 until `n_segments - 1`.
    for (const auto segment_id : segment_ids) {
        assert(segment_id < m_unitig_occurrence_data.size() && "index out of bounds");

        m_unitig_occurrence_data[segment_id] = true;
    }
}

void ReferencePathData::add_link(Link&& link) {
    (void)m_links.insert(std::move(link));
}

void ReferencePathData::increment_sequence_count() noexcept {
    ++m_sequence_count;
}

bool ReferencePathData::decrement_sequence_count_and_check_if_all_sequences_processed() {
    /*
     * Atomically subtracts one from the count and returns the previous value.
     *
     * Acquire-release ordering (std::memory_order_acq_rel) is required here so that the worker doing the final
     * decrement sees all work completed by other workers.
    */
    const auto previous_count = m_sequence_count.fetch_sub(1, std::memory_order_acq_rel);

    if (previous_count == 0) {
        throw Exception(ErrorCode::INVALID_STATE,
                        "attempted to decrement sequence count with no unprocessed sequences left");
    }

    return previous_count == 1;
}

} // namespace PANGWES
