/*
    Container for data parsed from a GFA1 line.
*/
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "gfa1_parser.hpp"

class LineData {
public:
    LineData(const std::string& line, uint64_t line_number)
    : m_fields(gfa1_parser::get_fields(line)),
      m_line_number(line_number)
    { }

    std::vector<std::string>& fields() { return m_fields; }
    uint64_t line_number() { return m_line_number; }

    std::vector<std::string>::iterator begin() { return m_fields.begin(); }
    std::vector<std::string>::iterator end() { return m_fields.end(); }

private:
    std::vector<std::string> m_fields;
    uint64_t m_line_number;


};
