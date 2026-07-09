/*
 * ReferencePathData.hpp - Class for storing path data associated with one reference sequence.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <atomic>
#include <cstddef>
#include <mutex>
#include <string>
#include <vector>

#include "common/utils/ConcurrentSet.hpp"
#include "gfa_parser/core/Link.hpp"

namespace PANGWES {

// Stores segment and link data read from path lines associated with one reference sequence.
class ReferencePathData {
public:
    ReferencePathData();

    /*
     * Constructs reference path data and builds the FASTA header for this reference's pseudo-FASTA sequence from a
     * reference name and the first seen sequence name.
    */
    ReferencePathData(const std::string& reference_name, const std::string& first_sequence_name);

    ReferencePathData(const ReferencePathData&) = delete;
    ReferencePathData& operator=(const ReferencePathData&) = delete;
    ReferencePathData(ReferencePathData&&) = delete;
    ReferencePathData& operator=(ReferencePathData&&) = delete;

    // Initializes the unitig occurrence data for `n_segments`. Throws if `n_segments == 0`.
    void initialize_unitig_occurrence_data(std::size_t n_segments);

    // Clears and releases the memory reserved by the links.
    void clear_and_release_reserved_memory();

    // Returns the stored links.
    const ConcurrentSet<Link, LinkHash>& links() const noexcept;

    // Moves out the unitig occurrence data for this reference.
    std::vector<bool> take_unitig_occurrence_data();

    // Returns the string for this reference's pseudo-FASTA header, excluding the leading '>' marker.
    const std::string& string_for_fasta_header() const noexcept;

    /*
     * Marks the given segment (unitig) ids as present in this reference.
     *
     * Throws if attempting to call this before `initialize_unitig_occurrence_data()`.
    */
    void mark_unitig_presence(const std::vector<std::size_t>& segment_ids);

    // Adds a link to this reference. No insertion happens if the link already exists.
    void add_link(Link&& link);

    // Increments the number of unprocessed sequences for this reference.
    void increment_sequence_count() noexcept;

    /*
     * Decrements the number of unprocessed sequences for this reference.
     *
     * Returns true if this decrement changed the count to zero, meaning that all sequences associated with this
     * reference have been processed.
     *
     * Throws if attempting to decrement with no unprocessed sequences left.
    */
    bool decrement_sequence_count_and_check_if_all_sequences_processed();

private:
    ConcurrentSet<Link, LinkHash> m_links;
    std::vector<bool> m_unitig_occurrence_data;
    std::string m_string_for_fasta_header;
    std::atomic<std::size_t> m_sequence_count;
    std::mutex m_mutex;
};

} // namespace PANGWES
