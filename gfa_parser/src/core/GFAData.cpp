/*
 * GFAData.cpp - Class for storing relevant GFA data for unitig_distance.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "common/io/FileReaderInterface.hpp"
#include "common/io/Log.hpp"
#include "common/utils/Exception.hpp"
#include "common/utils/format.hpp"
#include "common/utils/hash.hpp"
#include "common/utils/memory.hpp"
#include "common/utils/ProgressLogger.hpp"
#include "common/utils/utils.hpp"
#include "common/utils/WorkerPool.hpp"
#include "gfa_parser/core/CIGAR.hpp"
#include "gfa_parser/core/GFAData.hpp"
#include "gfa_parser/core/GFAFormat.hpp"
#include "gfa_parser/core/Link.hpp"
#include "gfa_parser/core/Orientation.hpp"
#include "gfa_parser/core/parser_utils.hpp"
#include "gfa_parser/core/ReferencePathData.hpp"
#include "gfa_parser/core/SegmentNameMap.hpp"
#include "gfa_parser/io/GFAParserOutputCoordinator.hpp"

namespace PANGWES {
namespace {

// Provided for ProgressLogger to log file-reading progress when both thresholds are reached.
constexpr std::uint64_t LOGGING_INTERVAL_LINE = 50UL * 1000 * 1000;
constexpr std::uint64_t LOGGING_INTERVAL_TIME_MS = 10000;

// Callback for ProgressLogger.
void progress_logger_callback(std::uint64_t line_number, std::uint64_t elapsed_time_ms) {
    // Only GFA 1.0 is currently supported.
    static const auto gfa_format_string = to_string(GFAFormat::GFA1);

    Log::out_without_date_block() << "INFO: Reading " << gfa_format_string << " file line "
                                  << Format::pretty_uint(line_number) << ". Elapsed time: "
                                  << Format::duration_to_string(elapsed_time_ms) << "." << std::endl;
}

// A vector of (link line fields, line_number) pairs.
using LinkLineFieldsVectorT = std::vector<std::pair<std::vector<std::string>, std::size_t>>;

constexpr auto SEGMENT_LINE_FIELDS_TO_READ = 3;
constexpr auto LINK_LINE_FIELDS_TO_READ = 6;
constexpr auto PATH_LINE_NAME_FIELDS_TO_READ = 2;
constexpr auto PATH_LINE_SEGMENTS_FIELDS_TO_READ = 3;

constexpr auto LINK_LINE_FROM_NAME_FIELD = 1;
constexpr auto LINK_LINE_FROM_ORIENTATION_FIELD = 2;
constexpr auto LINK_LINE_TO_NAME_FIELD = 3;
constexpr auto LINK_LINE_TO_ORIENTATION_FIELD = 4;
constexpr auto LINK_LINE_OVERLAP_FIELD = 5;

// Size must be at least two to fit both the segment name and an orientation.
constexpr auto MIN_SEGMENT_NAME_SIZE = 2;

// Number of Link lines to batch for worker parsing.
constexpr std::size_t LINKS_BATCH_SIZE = std::size_t{256} * 1024;

// Reserve enough memory to limit reallocations.
constexpr auto RESERVE_CAPACITY = std::size_t{2} * 1000 * 1000;

// Minimally parsed link data from Link lines (`L`) with the names still in raw format.
struct LinkLineData {
    std::string from_name;
    std::string to_name;
    Orientation from_orientation;
    Orientation to_orientation;
    std::size_t overlap;
    std::size_t line_number;

    bool operator==(const LinkLineData& other) const {
        return from_name == other.from_name &&
               to_name == other.to_name &&
               from_orientation == other.from_orientation &&
               to_orientation == other.to_orientation &&
               overlap == other.overlap;
    }
};

// Hash function for ConcurrentSet.
struct LinkLineDataHash {
    std::size_t operator()(const LinkLineData& link_line_data) const {
        const auto from_orientation_value = Traits::to_underlying(link_line_data.from_orientation);
        const auto to_orientation_value = Traits::to_underlying(link_line_data.to_orientation);

        assert(from_orientation_value <= 1 && "orientation value should be storable with 1 bit");
        assert(to_orientation_value <= 1 && "orientation value should be storable with 1 bit");

        // Pack orientations together to reduce hash combining.
        const auto orientation_values = (from_orientation_value << 1) | to_orientation_value;

        std::size_t seed = 0;

        Hash::combine(seed, link_line_data.from_name);
        Hash::combine(seed, link_line_data.to_name);
        Hash::combine(seed, orientation_values);
        Hash::combine(seed, link_line_data.overlap);

        return seed;
    }
};

} // namespace

GFAData::GFAData(std::unique_ptr<FileReaderInterface> reader, std::size_t expected_overlap)
    : m_segment_name_map{},
      m_invalid_links{},
      m_reference_paths{},
      m_reference_path_map{},
      m_reader{std::move(reader)},
      m_expected_overlap{expected_overlap}
{ }

void GFAData::read_segments_links_and_reference_counts() {
    ConcurrentSet<LinkLineData, LinkLineDataHash> links;
    LinkLineFieldsVectorT link_line_fields;
    std::size_t path_line_count = 0;

    links.reserve(RESERVE_CAPACITY);
    m_segment_name_map.reserve(RESERVE_CAPACITY);
    link_line_fields.reserve(LINKS_BATCH_SIZE);

    // Process Link lines as worker pool jobs.
    const auto post_link_line_processing_jobs = [this, &links](LinkLineFieldsVectorT& fields_batch) {
        if (fields_batch.empty()) {
            return;
        }

        // Take ownership of the data for the jobs and clear so the reader thread can start filling it again.
        const auto fields_batch_ptr = std::make_shared<LinkLineFieldsVectorT>(std::move(fields_batch));
        fields_batch.clear();
        fields_batch.reserve(LINKS_BATCH_SIZE);

        const auto n_link_lines = fields_batch_ptr->size();
        const auto n_jobs = WorkerPool::n_workers() > 0 ? WorkerPool::n_workers() : 1;
        const auto batch_size = (n_link_lines + n_jobs - 1) / n_jobs;

        std::vector<JobT> jobs;
        jobs.reserve(n_jobs);

        for (std::size_t begin_idx = 0; begin_idx < n_link_lines; begin_idx += batch_size) {
            const auto end_idx = std::min(begin_idx + batch_size, n_link_lines);

            jobs.push_back([this, &links, fields_batch_ptr, begin_idx, end_idx]() {
                for (std::size_t idx = begin_idx; idx < end_idx; ++idx) {
                    auto& fields = (*fields_batch_ptr)[idx].first;
                    const auto line_number = (*fields_batch_ptr)[idx].second;

                    try {
                        if (fields[LINK_LINE_FROM_ORIENTATION_FIELD].size() != 1) {
                            throw Exception(ErrorCode::INVALID_DATA, "invalid from orientation");
                        }
                        if (fields[LINK_LINE_TO_ORIENTATION_FIELD].size() != 1) {
                            throw Exception(ErrorCode::INVALID_DATA, "invalid to orientation");
                        }

                        auto from_name = std::move(fields[LINK_LINE_FROM_NAME_FIELD]);
                        auto to_name = std::move(fields[LINK_LINE_TO_NAME_FIELD]);
                        const auto from_orientation
                            = parse_orientation(fields[LINK_LINE_FROM_ORIENTATION_FIELD].front());
                        const auto to_orientation = parse_orientation(fields[LINK_LINE_TO_ORIENTATION_FIELD].front());

                        // Skip self-links.
                        if (from_name == to_name && from_orientation != to_orientation) {
                            continue;
                        }

                        const auto overlap = CIGAR(fields[LINK_LINE_OVERLAP_FIELD]).overlap();

                        if (overlap != 0 && overlap < m_expected_overlap) {
                            throw Exception(ErrorCode::INVALID_DATA,
                                            "required overlap at least ",
                                            m_expected_overlap,
                                            "; got ",
                                            overlap);
                        }

                        (void)links.insert(LinkLineData{std::move(from_name),
                                                        std::move(to_name),
                                                        from_orientation,
                                                        to_orientation,
                                                        overlap,
                                                        line_number});
                    } catch (const std::exception& exception) {
                        throw Exception(ErrorCode::FILE_BAD_DATA,
                                        "Link line on line ",
                                        line_number,
                                        ": ",
                                        exception.what());
                    }
                }
            });
        }

        WorkerPool::post(jobs);
    };

    ProgressLogger progress_logger(progress_logger_callback, LOGGING_INTERVAL_LINE, LOGGING_INTERVAL_TIME_MS);

    // Start reading the GFA file.
    m_reader->open();

    for (std::string line; m_reader->getline(line); ) {
        progress_logger.log_if_due(m_reader->line_number());

        if (line.empty()) {
            continue;
        }

        // The first character must be the record type.
        switch (line[0]) {
            // Map segments names internally to integers and store the segment sequences.
            case 'S': {
                auto fields = Utils::get_fields(line, '\t', SEGMENT_LINE_FIELDS_TO_READ);
                if (fields.size() < SEGMENT_LINE_FIELDS_TO_READ) {
                    throw Exception(ErrorCode::FILE_WRONG_COLUMN_COUNT,
                                    "Segment line on line ", m_reader->line_number());
                }

                auto& segment_name = fields[1];
                auto& segment_sequence = fields[2];

                if (m_segment_name_map.contains(segment_name)) {
                    throw Exception(ErrorCode::FILE_DUPLICATE_DATA, "Segment Name in Segment line on line ",
                                    m_reader->line_number());
                }

                try {
                    (void)m_segment_name_map.add_and_map_segment(std::move(segment_name), std::move(segment_sequence));
                } catch (const std::exception& exception) {
                    throw Exception(ErrorCode::FILE_BAD_DATA, "Segment line on line ", m_reader->line_number(), ": ",
                                    exception.what());
                }

                break;
            }

            // Store link line data in a vector to periodically post as a batch to the worker pool for processing.
            case 'L': {
                auto fields = Utils::get_fields(line, '\t', LINK_LINE_FIELDS_TO_READ);
                if (fields.size() < LINK_LINE_FIELDS_TO_READ) {
                    throw Exception(ErrorCode::FILE_WRONG_COLUMN_COUNT, "Link line on line ", m_reader->line_number());
                }

                link_line_fields.push_back({std::move(fields), m_reader->line_number()});

                if (link_line_fields.size() == LINKS_BATCH_SIZE) {
                    post_link_line_processing_jobs(link_line_fields);
                }

                break;
            }

            // Count the number of references and sequences per reference for directing the second reading pass.
            case 'P': {
                auto fields = Utils::get_fields(line, '\t', PATH_LINE_NAME_FIELDS_TO_READ);
                if (fields.size() < PATH_LINE_NAME_FIELDS_TO_READ) {
                    throw Exception(ErrorCode::FILE_WRONG_COLUMN_COUNT, "Path line on line ", m_reader->line_number());
                }

                try {
                    const auto parsed_path_name = ParserUtils::parse_cuttlefish_path_name(fields[1]);
                    const auto inserted_result =
                        m_reference_path_map.insert(std::make_pair(parsed_path_name.reference_name,
                                                                   m_reference_paths.size()));
                    const auto reference_path_index = inserted_result.first->second;
                    const bool insert_successful = inserted_result.second;

                    // New reference detected; append to the vector.
                    if (insert_successful) {
                        m_reference_paths.emplace_back(Memory::make_unique<ReferencePathData>(
                            parsed_path_name.reference_name,
                            parsed_path_name.sequence_name));
                    }

                    // Increase the sequence count for this reference.
                    m_reference_paths[reference_path_index]->increment_sequence_count();
                    ++path_line_count;

                } catch (const std::exception& exception) {
                    throw Exception(ErrorCode::FILE_BAD_DATA, "Path line on line ", m_reader->line_number(), ": ",
                                    exception.what());
                }

                break;
            }
            // Other syntactically valid record types are ignored.
            case '#': continue;
            case 'H': continue;
            case 'C': continue;
            case 'W': continue;
            case 'J': continue;
            default: throw Exception(ErrorCode::INVALID_RECORD_TYPE, line[0]);
        }
    }

    m_reader->close();

    post_link_line_processing_jobs(link_line_fields);

    Log::out_without_date_block() << "INFO: Read " << Format::pretty_uint(m_reader->line_number()) << " "
                                  << to_string(GFAFormat::GFA1) << " file lines." << std::endl;

    if (m_segment_name_map.empty()) {
        throw Exception(ErrorCode::FILE_BAD_DATA, "failed to read any segments");
    }

    if (m_reference_paths.empty()) {
        throw Exception(ErrorCode::FILE_BAD_DATA, "failed to read any paths");
    }

    // Wait for link line jobs to finish.
    WorkerPool::wait();

    const auto n_links = links.size();

    if (n_links == 0) {
        throw Exception(ErrorCode::FILE_BAD_DATA, "failed to read any links");
    }

    Log::out_without_date_block() << "INFO: Validating " << Format::pretty_uint(n_links) << " links."
                                  << std::endl;

    // ConcurrentSet doesn't support random access, so copy the links here.
    std::vector<LinkLineData> links_for_validation;
    links_for_validation.reserve(n_links);
    for (const auto& link_line : links) {
        links_for_validation.push_back(link_line);
    }

    const auto n_jobs = WorkerPool::n_workers() > 0 ? WorkerPool::n_workers() : 1;
    const auto batch_size = (n_links + n_jobs - 1) / n_jobs;

    std::vector<JobT> jobs;
    jobs.reserve(n_jobs);

    for (std::size_t begin_idx = 0; begin_idx < n_links; begin_idx += batch_size) {
        const auto end_idx = std::min(begin_idx + batch_size, n_links);

        jobs.push_back([this, &links_for_validation, begin_idx, end_idx]() {
            for (std::size_t idx = begin_idx; idx < end_idx; ++idx) {
                const auto& link_line = links_for_validation[idx];

                try {
                    const auto from_id = m_segment_name_map.segment_name_mapping(link_line.from_name);
                    if (from_id == SEGMENT_NOT_MAPPED) {
                        throw Exception(ErrorCode::INVALID_DATA, "unknown Segment: \"", link_line.from_name, "\"");
                    }

                    const auto to_id = m_segment_name_map.segment_name_mapping(link_line.to_name);
                    if (to_id == SEGMENT_NOT_MAPPED) {
                        throw Exception(ErrorCode::INVALID_DATA, "unknown Segment: \"", link_line.to_name, "\"");
                    }

                    // Self-links were skipped earlier.
                    Link link(from_id, to_id, link_line.from_orientation, link_line.to_orientation);

                    if (link_line.overlap == 0) {
                        (void)m_invalid_links.insert(std::move(link));
                        continue;
                    }

                    const auto& from_sequence = m_segment_name_map.segment_sequence(from_id);
                    const auto& to_sequence = m_segment_name_map.segment_sequence(to_id);

                    if (!ParserUtils::is_exact_overlap(from_sequence,
                                                       to_sequence,
                                                       link_line.from_orientation,
                                                       link_line.to_orientation,
                                                       link_line.overlap))
                    {
                        (void)m_invalid_links.insert(std::move(link));
                    }
                } catch (const std::exception& exception) {
                    throw Exception(ErrorCode::FILE_BAD_DATA,
                                    "Link line on line ",
                                    link_line.line_number,
                                    ": ",
                                    exception.what());
                }
            }
        });
    }

    WorkerPool::post(jobs);
    WorkerPool::wait();

    Log::out_without_date_block() << "INFO: Finished validating links." << std::endl;

    const auto n_valid_links = n_links - m_invalid_links.size();
    if (n_valid_links == 0) {
        throw Exception(ErrorCode::FILE_BAD_DATA, "failed to read any valid links");
    }

    Log::out_without_date_block() << "INFO: Stored " << Format::pretty_uint(m_segment_name_map.size()) << " segments."
                                  << std::endl;
    Log::out_without_date_block() << "INFO: Stored " << Format::pretty_uint(n_valid_links) << " valid links."
                                  << std::endl;
    Log::out_without_date_block() << "INFO: Prepared " << Format::pretty_uint(m_reference_paths.size())
                                  << " references from " << Format::pretty_uint(path_line_count)
                                  << " Path lines for the second read pass." << std::endl;
}

void GFAData::read_reference_path_data(GFAParserOutputCoordinator& output_coordinator) {
    std::vector<bool> reference_path_initialized(m_reference_paths.size(), false);

    ProgressLogger progress_logger(progress_logger_callback, LOGGING_INTERVAL_LINE, LOGGING_INTERVAL_TIME_MS);

    // Re-open the file and start reading the GFA file for the second time.
    m_reader->open();

    for (std::string line; m_reader->getline(line); ) {
        progress_logger.log_if_due(m_reader->line_number());

        if (line.empty()) {
            continue;
        }

        switch (line[0]) {
            // Now fully process each sequence.
            case 'P': {
                auto fields = Utils::get_fields(line, '\t', PATH_LINE_SEGMENTS_FIELDS_TO_READ);
                if (fields.size() < PATH_LINE_SEGMENTS_FIELDS_TO_READ) {
                    throw Exception(ErrorCode::FILE_WRONG_COLUMN_COUNT, "Path line on line ", m_reader->line_number());
                }

                std::size_t reference_path_index = 0;

                try {
                    const auto parsed_path_name = ParserUtils::parse_cuttlefish_path_name(fields[1]);
                    const auto reference_path_iter = m_reference_path_map.find(parsed_path_name.reference_name);

                    if (reference_path_iter == m_reference_path_map.end()) {
                        throw Exception(ErrorCode::INVALID_DATA,
                                        "unknown reference path \"",
                                        parsed_path_name.reference_name,
                                        "\"");
                    }

                    reference_path_index = reference_path_iter->second;
                } catch (const std::exception& exception) {
                    throw Exception(ErrorCode::FILE_BAD_DATA,
                                    "Path line on line ",
                                    m_reader->line_number(),
                                    ": ",
                                    exception.what());
                }

                auto& reference_path_data = *m_reference_paths[reference_path_index];
                if (!reference_path_initialized[reference_path_index]) {
                    reference_path_data.initialize_unitig_occurrence_data(m_segment_name_map.size());
                    reference_path_initialized[reference_path_index] = true;
                }

                const auto segment_names_ptr = std::make_shared<std::string>(std::move(fields[2]));
                const auto line_number = m_reader->line_number();
                auto* reference_path_data_ptr = &reference_path_data;

                // Process this Path line as a worker pool job.
                const JobT path_job = [this,
                                       &output_coordinator,
                                       segment_names_ptr,
                                       line_number,
                                       reference_path_data_ptr,
                                       reference_path_index]()
                {
                    try {
                        auto segment_names = Utils::get_fields(*segment_names_ptr, ',');
                        if (segment_names.empty()) {
                            throw Exception(ErrorCode::INVALID_DATA, "empty segment names field");
                        }

                        std::vector<std::size_t> segment_ids;
                        segment_ids.reserve(segment_names.size());

                        std::size_t previous_segment_id = 0;
                        auto previous_orientation = Orientation::PLUS;

                        for (std::size_t idx = 0; idx < segment_names.size(); ++idx) {
                            auto& segment_name_data = segment_names[idx];

                            if (segment_name_data.size() < MIN_SEGMENT_NAME_SIZE) {
                                throw Exception(ErrorCode::INVALID_DATA,
                                                "too short path segment names entry, \"",
                                                segment_name_data,
                                                "\", at index ",
                                                idx);
                            }

                            // Orientation must be the last character.
                            const auto orientation = parse_orientation(segment_name_data.back());

                            // After this, `segment_name_data` is the segment name.
                            segment_name_data.pop_back();

                            const auto segment_id = m_segment_name_map.segment_name_mapping(segment_name_data);
                            if (segment_id == SEGMENT_NOT_MAPPED) {
                                throw Exception(ErrorCode::INVALID_DATA,
                                                "unknown Segment: \"", segment_name_data, "\"");
                            }

                            segment_ids.push_back(segment_id);

                            if (idx > 0 &&
                                !Link::is_self_edge(previous_segment_id, segment_id, previous_orientation, orientation))
                            {
                                Link link(previous_segment_id, segment_id, previous_orientation, orientation);

                                if (!m_invalid_links.contains(link)) {
                                    reference_path_data_ptr->add_link(std::move(link));
                                }
                            }

                            previous_segment_id = segment_id;
                            previous_orientation = orientation;
                        }

                        reference_path_data_ptr->mark_unitig_presence(segment_ids);
                        if (reference_path_data_ptr->decrement_sequence_count_and_check_if_all_sequences_processed()) {
                            output_coordinator.write_reference_path_data(*reference_path_data_ptr,
                                                                         reference_path_index);
                        }
                    } catch (const std::exception& exception) {
                        throw Exception(ErrorCode::FILE_BAD_DATA,
                                        "Path line on line ",
                                        line_number,
                                        ": ",
                                        exception.what());
                    }
                };

                WorkerPool::post({path_job});

                if (WorkerPool::is_async()) {
                    WorkerPool::wait_until_unfinished_jobs_at_most(WorkerPool::n_workers() * 2);
                }

                break;
            }

            // Other syntactically valid record types are ignored.
            case '#': continue;
            case 'H': continue;
            case 'S': continue;
            case 'L': continue;
            case 'C': continue;
            case 'W': continue;
            case 'J': continue;
            default: throw Exception(ErrorCode::INVALID_RECORD_TYPE, line[0]);
        }
    }

    m_reader->close();

    Log::out_without_date_block() << "INFO: Read " << Format::pretty_uint(m_reader->line_number()) << " "
                                  << to_string(GFAFormat::GFA1) << " file lines." << std::endl;
}

const SegmentNameMap& GFAData::segment_name_map() const noexcept {
    return m_segment_name_map;
}

const ConcurrentSet<Link, LinkHash>& GFAData::invalid_links() const noexcept {
    return m_invalid_links;
}

std::size_t GFAData::n_reference_paths() const noexcept {
    return m_reference_paths.size();
}

} // namespace PANGWES
