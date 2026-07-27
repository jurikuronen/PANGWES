/*
 * GFAData.hpp - Class for reading, storing and validating relevant GFA data for unitig_distance.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/io/FileReaderInterface.hpp"
#include "common/utils/ConcurrentSet.hpp"
#include "gfa_parser/core/Link.hpp"
#include "gfa_parser/core/ReferencePathData.hpp"
#include "gfa_parser/core/SegmentNameMap.hpp"

namespace PANGWES {

class GFAParserOutputCoordinator;

/*
 * Class for reading and storing GFA data.
 *
 * Currently only GFA 1.0 supported.
 *
 * Reads a GFA 1.0 file in two passes:
 * - The first pass reads and validates segments and links, and counts references and sequences from the Path lines.
 * - The second pass re-reads the path lines and links the path segments with the segments read during the first pass.
 *   Once all path lines for some reference have been read, the corresponding ReferencePathData is passed onto the
 *   output coordinator for writing out.
*/
class GFAData {
public:
    // Constructs GFAData to validate links according to the provided overlap.
    GFAData(std::unique_ptr<FileReaderInterface> reader, std::size_t expected_overlap);

    GFAData(const GFAData&) = delete;
    GFAData& operator=(const GFAData&) = delete;
    GFAData(GFAData&&) noexcept = delete;
    GFAData& operator=(GFAData&&) noexcept = delete;

    // Reads and validates segments and links, and counts references and sequences from the Path lines.
    void read_segments_links_and_reference_counts();

    /*
     * Reads path lines to construct ReferencePathData. Ready ReferencePathData are passed to the output coordinator
     * for writing out.
    */
    void read_reference_path_data(GFAParserOutputCoordinator& output_coordinator);

    // Returns the segment-name mapping and stored segment sequences.
    const SegmentNameMap& segment_name_map() const noexcept;

    // Returns a set containing all links that were invalidated during the first pass.
    const ConcurrentSet<Link, LinkHash>& invalid_links() const noexcept;

    // Returns the number of reference paths to read and write, counted during the first pass.
    std::size_t n_reference_paths() const noexcept;

private:
    SegmentNameMap m_segment_name_map;
    ConcurrentSet<Link, LinkHash> m_invalid_links;
    std::vector<std::unique_ptr<ReferencePathData>> m_reference_paths;
    std::unordered_map<std::string, std::size_t> m_reference_path_map;
    std::unique_ptr<FileReaderInterface> m_reader;
    std::size_t m_expected_overlap;
};

} // namespace PANGWES
