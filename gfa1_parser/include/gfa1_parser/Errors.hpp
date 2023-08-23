/*
    Simple class for tracking program errors.
*/
#pragma once

#include <cstdint>
#include <iostream>
//#include <memory>
//#include <mutex>

class Errors {
public:
    Errors() = default;
    //Errors() : m_mtx(new std::mutex) { }

    void report(const std::string& description) { add_error(description); }

    void report(const std::string& description, uint64_t line_number) { add_error(std::string{description + " on line " + std::to_string(line_number) + "."}); }

    void print_errors() {
        for (const auto& error_str : m_errors) {
            std::cerr << error_str << std::endl;
        }
        m_errors.clear();
    }

    std::size_t size() const { return m_errors.size(); }

private:
    //std::unique_ptr<std::mutex> m_mtx;

    std::vector<std::string> m_errors;

    void add_error(const std::string& error_str) {
        //std::lock_guard<std::mutex> lock(*m_mtx);
        m_errors.emplace_back(error_str);
    }

};
