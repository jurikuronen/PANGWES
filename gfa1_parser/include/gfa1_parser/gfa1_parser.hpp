/* 
    Various utility functions used by the program.
*/
#pragma once

#include <algorithm>
#include <chrono>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <sys/types.h>
#include <sys/stat.h>

#ifdef WIN32
#include <direct.h>
#endif

using edge_type_t = std::pair<char, char>;

namespace gfa1_parser {

    std::vector<std::string> get_fields(const std::string& line, char delim = '\t') {
        std::vector<std::string> fields;
        std::stringstream ss(line);
        for (std::string field; std::getline(ss, field, delim); ) fields.push_back(std::move(field));
        return fields;
    }

    template <typename T>
    std::vector<uint64_t> map_to_vector(const T& map, std::size_t sz) {
        std::vector<uint64_t> vector(sz);
        for (const auto& x : map) {
            uint64_t id, count;
            std::tie(id, count) = x;
            vector[id] = count;
        }
        return vector;
    }

    template <typename T>
    std::vector<uint64_t> map_keys_to_vector(const T& map) {
        std::vector<uint64_t> vector;
        for (const auto& x : map) vector.push_back(x.first);
        std::sort(vector.begin(), vector.end());
        return vector;
    }

    template <typename T>
    void clear(T& container) { T().swap(container); }

    void swap(uint64_t& from_id, uint64_t& to_id, char& from_orient, char& to_orient) {
        std::swap(from_id, to_id);
        if (from_orient == to_orient) {
            // Swap FF into RR and vice versa.
            static constexpr int plus = '+', minus = '-';
            (from_orient ^= plus) ^= minus;
            (to_orient ^= plus) ^= minus;
        }
    }

    edge_type_t get_edge_type(char from_orient, char to_orient) {
        static const std::vector<edge_type_t> conv = [] {
            int plus = '+', minus = '-';
            std::vector<edge_type_t> conv(3 * minus + 1);
            conv[2 * plus + plus] = std::make_pair('F', 'F');
            conv[2 * plus + minus] = std::make_pair('F', 'R');
            conv[2 * minus + plus] = std::make_pair('R', 'F');
            conv[2 * minus + minus] = std::make_pair('R', 'R');
            return conv;
        }();
        return conv[2 * from_orient + to_orient];
    }

    std::string neat_number_str(uint64_t number) {
        std::vector<uint64_t> parts;
        do parts.push_back(number % 1000);
        while (number /= 1000);
        std::string number_str = std::to_string(parts.back());
        for (int64_t i = parts.size() - 2; i >= 0; --i) {
            number_str += ' ' + std::string(3 - std::to_string(parts[i]).size(), '0') + std::to_string(parts[i]);
        }
        return number_str;
    }

    bool directory_exists(const std::string& directory) {
        struct stat info;
        return stat(directory.c_str(), &info) == 0 && info.st_mode & S_IFDIR;
    }

    int create_directory(const std::string& directory) {
        #ifdef WIN32
        return _mkdir(directory.c_str());
        #else
        return mkdir(directory.c_str(), 0755);
        #endif
    }

    using clock = std::chrono::high_resolution_clock;
    clock::time_point time_now() { return clock::now(); }
    std::string time_elapsed(const clock::time_point& tp) {
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(clock::now() - tp).count();
        if (duration < 60) return std::to_string(duration) + 's';
        else return std::to_string(duration / 60) + "m " + std::to_string(duration % 60) + 's';
    }
}
