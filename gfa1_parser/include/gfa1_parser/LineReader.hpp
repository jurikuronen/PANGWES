/*
    Class for reading the next block of lines from a GFA1 file.
*/
#pragma once

#include <fstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "Errors.hpp"
#include "LineData.hpp"

using line_data_container_t = std::vector<LineData>;

class LineReader {
public:
    LineReader(Errors& errors) : m_errors(errors) { }

    line_data_container_t& segment_line_data() { return m_segment_line_data; }
    line_data_container_t& link_line_data() { return m_link_line_data; }
    line_data_container_t& path_line_data() { return m_path_line_data; }

    void clear() {
        // Use std::vector::clear here as the memory will soon be reused.
        m_segment_line_data.clear();
        m_link_line_data.clear();
        m_path_line_data.clear();
    }

    bool read_next_block(std::ifstream& ifs, std::size_t& idx, std::size_t block_end) {
        for (std::string line; idx != block_end && std::getline(ifs, line); ++idx) {
            uint64_t line_number = idx + 1;
            if (line.size() == 0) {
                m_errors.report("Empty line", line_number);
                return false;
            }
            char record_type = line[0];
            switch (record_type) {
                case 'S': m_segment_line_data.emplace_back(line, line_number); break;
                case 'L': m_link_line_data.emplace_back(line, line_number); break;
                case 'P': m_path_line_data.emplace_back(line, line_number); break;
                default: break;
            }
        }
        return true;
    }

private:
    line_data_container_t m_segment_line_data;
    line_data_container_t m_link_line_data;
    line_data_container_t m_path_line_data;

    Errors& m_errors;

};

