#pragma once

#include <fstream>
#include <string>
#include <tuple>
#include <vector>

#include "ProgramOptions.hpp"
#include "types.hpp"

class Utils {
public:
    static std::vector<std::string> get_fields(const std::string& line, char delim = ' ') {
        std::vector<std::string> fields;
        std::stringstream ss(line);
        for (std::string field; std::getline(ss, field, delim); ) fields.push_back(std::move(field));
        return fields;
    }

    static bool file_is_good(const std::string& filename) {
        return std::ifstream(filename).good();
    }

    static std::string neat_number_str(int_t number) {
        std::vector<int_t> parts;
        do parts.push_back(number % 1000);
        while (number /= 1000);
        std::string number_str = std::to_string(parts.back());
        for (int_t i = parts.size() - 2; i >= 0; --i) {
            number_str += ' ' + std::string(3 - std::to_string(parts[i]).size(), '0') + std::to_string(parts[i]);
        }
        return number_str;
    }

    static std::string neat_decimal_str(int_t nom, int_t denom) {
        std::string int_str = std::to_string(nom / denom);
        std::string dec_str = std::to_string(nom * 100 / denom % 100);
        return int_str + "." + std::string(2 - dec_str.size(), '0') + dec_str;
    }

    static real_t fixed_distance(real_t distance, real_t max_distance = REAL_T_MAX) { return distance >= max_distance ? -1.0 : distance; }

    static bool is_numeric(const std::string& str) {
        double x;
        return (std::stringstream(str) >> x).eof();
    }

    template <typename T>
    static void clear(T& container) { T().swap(container); }

    static bool sanity_check_input_files() {
        if (ProgramOptions::operating_mode != OperatingMode::OUTLIER_TOOLS) {
            if (!Utils::file_is_good(ProgramOptions::edges_filename)) {
                std::cerr << "Error: Can't open " << ProgramOptions::edges_filename << std::endl;
                return false;
            }

            if (ProgramOptions::has_operating_mode(OperatingMode::CDBG)) {
                if (!Utils::file_is_good(ProgramOptions::unitigs_filename)) {
                    std::cerr << "Error: Can't open " << ProgramOptions::unitigs_filename << std::endl;
                    return false;
                }

                if (ProgramOptions::has_operating_mode(OperatingMode::SGGS)) {
                    if (!Utils::file_is_good(ProgramOptions::sggs_filename)) {
                        std::cerr << "Error: Can't open " << ProgramOptions::sggs_filename << std::endl;
                        return false;
                    }
                    std::ifstream ifs(ProgramOptions::sggs_filename);
                    for (std::string path_edges; std::getline(ifs, path_edges); ) {
                        if (!Utils::file_is_good(path_edges)) {
                            std::cerr << "Error: Can't open " << path_edges << std::endl;
                            return false;
                        }
                    }
                }
            }
        }

        if (!ProgramOptions::queries_filename.empty()) {
            if (!Utils::file_is_good(ProgramOptions::queries_filename)) {
                std::cerr << "Error: Can't open " << ProgramOptions::queries_filename << std::endl;
                return false;
            }
        }

        return true;
    }

    /*
        Attempts to automatically deduce the format the queries file is using.
        -1: invalid format.
         0: v w
         1: v w s
         2: v w d s
         3: v w d f s
         4: v w d s c
         5: v w d f s c
    */
    static int_t deduce_queries_format(const std::string& line) {
        auto fields_sz = get_fields(line).size();
        bool ot_mode = ProgramOptions::operating_mode == OperatingMode::OUTLIER_TOOLS;
        switch (fields_sz) {
            case 2: return 0;
            case 3: return 1;
            case 4: return 2;
            case 5: return (ot_mode ? 4 : 3);
            case 6: return 5;
        }
        return -1;
    }

    static std::string get_queries_format_string(int_t queries_format) {
        switch (queries_format) {
            case 0: return "v w";
            case 1: return "v w score";
            case 2: return "v w distance score";
            case 3: return "v w distance flag score";
            case 4: return "v w distance score count";
            case 5: return "v w distance flag score count";
        }
        return "invalid format";
    }

    static std::size_t get_queries_n_fields(int_t queries_format) {
        switch (queries_format) {
            case 0: return 2;
            case 1: return 3;
            case 2: return 4;
            case 3: return 5;
            case 4: return 5;
            case 5: return 6;
        }
        return 0;
    }

    static std::tuple<int_t, int_t, int_t, int_t> get_field_indices(int_t queries_format) {
        bool ot_mode = ProgramOptions::operating_mode == OperatingMode::OUTLIER_TOOLS;

        int_t distance_field = ot_mode ? 2 : 0;
        int_t flag_field = 0;
        int_t score_field = 0;
        int_t count_field = 0;

        if (queries_format == 1) {
            score_field = 2;
        } else if (queries_format == 2) {
            score_field = 3;
        } else if (queries_format == 3) {
            flag_field = 3;
            score_field = 4;
        } else if (queries_format == 4) {
            score_field = 3;
            count_field = 4;
        } else if (queries_format == 5) {
            flag_field = 3;
            score_field = 4;
            count_field = 5;
        }

        return std::make_tuple(distance_field, flag_field, score_field, count_field);
    }

private:

};
