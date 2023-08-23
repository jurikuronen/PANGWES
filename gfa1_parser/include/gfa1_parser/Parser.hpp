/*
    Parsing logic for the different line types.
*/
#pragma once

#include <cstdint>
#include <fstream>
#include <string>
#include <utility>

#include "gfa1_parser.hpp"

#include "Errors.hpp"
#include "LineData.hpp"
#include "LinkEndpoint.hpp"
#include "Links.hpp"
#include "PathData.hpp"
#include "Paths.hpp"
#include "Segments.hpp"

using line_data_t = LineData;
using line_data_container_t = std::vector<line_data_t>;

class Parser {
public:
    Parser(Segments& segments, Links& links, Paths& paths, PathData& path_data, Errors& errors) 
      : m_segments(segments),
        m_links(links),
        m_paths(paths),
        m_path_data(path_data),
        m_errors(errors)
    { }

    bool process_segment_line_data(line_data_container_t& segment_line_data) {
        for (auto& line_data : segment_line_data) {
            auto& fields = line_data.fields();
            if (fields.size() < 3) {
                m_errors.report("Segment line has incorrect format.", line_data.line_number());
                return false;
            }

            std::string&& name = std::move(fields[1]);
            std::string&& sequence = std::move(fields[2]);

            if (m_segments.contains(name)) {
                auto& current_sequence = m_segments.sequence(name);
                if (current_sequence.empty()) {
                    // Name mapped previously by a link line, missing sequence given by this segment line.
                    current_sequence = std::move(sequence);
                } else if (current_sequence != sequence) {
                    // Multiple segment lines for the same name.
                    m_errors.report(std::string() + "Segment with name " + name + " has multiple sequences.", line_data.line_number());
                    return false;
                }
            } else {
                m_segments.map_name_and_sequence(std::move(name), std::move(sequence));
            }
        }
        return true;
    }

    bool process_link_line_data(line_data_container_t& link_line_data) {
        for (auto& line_data : link_line_data) {
            auto& fields = line_data.fields();
            if (fields.size() < 6) {
                m_errors.report("Link line has incorrect format.", line_data.line_number());
                return false;
            }

            std::string& from = fields[1];
            char from_orient = fields[2][0];
            std::string& to = fields[3];
            char to_orient = fields[4][0];
            uint64_t overlap = std::stoull(fields[5]);

            uint64_t from_id = m_segments.contains(from) ? m_segments.mapped_idx(from) : m_segments.map_name(std::move(from));
            uint64_t to_id = m_segments.contains(to) ? m_segments.mapped_idx(to) : m_segments.map_name(std::move(to));

            m_links.add_link(from_id, to_id, from_orient, to_orient, overlap);
        }
        return true;
    }

    bool process_path_line_data(line_data_container_t& path_line_data) {
        for (auto& line_data : path_line_data) {
            auto& fields = line_data.fields();

            const std::string& pathname = fields.at(1);

            std::string reference, sequence;
            std::tie(reference, sequence) = m_paths.parse_cuttlefish_reference_and_sequence(pathname);
            if (reference.empty() && sequence.empty()) {
                m_errors.report("Failed to parse path name", line_data.line_number());
                return false;
            }

            uint64_t path_idx = m_paths.contains(reference) ? m_paths.mapped_idx(reference) : m_paths.add_path(std::move(reference), std::move(sequence));
            m_path_data.add_path_data(path_idx, std::move(fields.at(2)), std::move(fields.at(3)));
        }
        return true;
    }

    bool process_paths(const std::string& fasta_filename, const std::string& paths_filename, const std::string& path_directory) {    
        std::ofstream ofs_fasta(fasta_filename);    
        std::ofstream ofs_paths(paths_filename);
        std::size_t n_paths = m_path_data.n_paths();
        auto time_path_loop_start = gfa1_parser::time_now();
        for (std::size_t path_idx = 0; path_idx < m_path_data.n_paths(); ++path_idx) {
            Path& path = m_paths[path_idx];
            path.set_capacity(m_paths.n_segments());

            if (!process_path(path_idx)) return false;
            m_paths.add_counts(path.counts());
            
            m_paths.write_to_fasta(ofs_fasta, path);
            
            const std::string& path_edges_filename = path_directory + "/" + path.reference() + ".edges";
            path.write_links(path_edges_filename);

            const std::string& path_counts_filename = path_directory + "/" + path.reference() + ".counts";
            path.write_counts(path_counts_filename);

            ofs_paths << path_edges_filename << '\n';

            m_path_data.clear(path_idx);
            path.clear();

            if ((path_idx + 1) % 10 == 0) std::cout << "\rProcessed paths " << path_idx + 1 << '/' << n_paths 
                                                    << ". Time elapsed: " << gfa1_parser::time_elapsed(time_path_loop_start) << '.' << std::flush;
        }
        std::cout << "\rProcessed paths " << n_paths << '/' << n_paths 
                  << ". Time elapsed: " << gfa1_parser::time_elapsed(time_path_loop_start) << '.' << std::endl;
        return true;
    }

private:
    Segments& m_segments;
    Links& m_links;
    Paths& m_paths;
    PathData& m_path_data;

    Errors& m_errors;

    bool process_path(std::size_t path_idx) {
        Path& path = m_paths[path_idx];
        if (path.capacity() == 0) {
            m_errors.report("Path with internal index " + std::to_string(path_idx) + " capacity not set (is zero)");
            return false;
        }
        for (std::size_t idx = 0; idx < m_path_data.n_path_lines(path_idx); ++idx) {
            auto segment_names = gfa1_parser::get_fields(m_path_data.segment_names(path_idx, idx), ',');
            if (segment_names.size() == 1) continue; // Path is a single unitig.
            std::vector<uint64_t> segment_ids;
            std::vector<char> orients;
            for (auto& segment : segment_names) {
                orients.push_back(segment.back());
                segment.pop_back();
                if (!m_segments.contains(segment)) {
                    m_errors.report("Path with internal index " + std::to_string(path_idx) + " contains an unmapped segment");
                    return false;
                }
                segment_ids.push_back(m_segments.mapped_idx(segment));
                path.add_count(segment_ids.back());
            }

            auto overlaps = gfa1_parser::get_fields(m_path_data.overlaps(path_idx, idx), ',');
            // Check for a bug that existed in Cuttlefish <= 1.0.0.
            uint64_t index_correction = overlaps.size() == segment_ids.size();
            if (segment_ids.size() - 1 + index_correction != overlaps.size()) {
                m_errors.report("Path with internal index " + std::to_string(path_idx) + " has wrong 'segment_ids' and 'overlaps' counts");
                return false;
            }
            for (std::size_t segment_idx = 1; segment_idx < segment_ids.size(); ++segment_idx) {
                auto from_id = segment_ids[segment_idx - 1];
                auto to_id = segment_ids[segment_idx];
                char from_orient = orients[segment_idx - 1];
                char to_orient = orients[segment_idx];
                uint64_t overlap = std::stoull(overlaps[segment_idx - 1 + index_correction]);

                path.add_link(from_id, to_id, from_orient, to_orient, overlap);
            }
        }
        return true;
    }

};


