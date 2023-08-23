#pragma once

#include <fstream>
#include <numeric>

#include "DistanceVector.hpp"
#include "ProgramOptions.hpp"
#include "Queries.hpp"
#include "Utils.hpp"

class ResultsWriter {
public:
    static void output_results(const std::string& out_filename, const Queries& queries, const DistanceVector& dv) {
        std::vector<int_t> indices(queries.size());
        std::iota(indices.begin(), indices.end(), 0);
        output_results(out_filename, queries, dv, indices);
    }

    static void output_results(const std::string& out_filename, const Queries& queries, const DistanceVector& dv, const std::vector<int_t>& indices) {
        std::ofstream ofs(out_filename);
        
        bool write_counts = dv.storing_mean_distances();

        int_t queries_format = queries.queries_format();

        // Used for checking which fields to output.
        int_t distance_field, flag_field, score_field, count_field;
        std::tie(distance_field, flag_field, score_field, count_field) = Utils::get_field_indices(queries_format);

        for (auto idx : indices) {
            ofs << queries.v(idx) + ProgramOptions::output_one_based << ' ' << queries.w(idx) + ProgramOptions::output_one_based;
            ofs << ' ' << (int_t) Utils::fixed_distance(dv[idx].distance(), ProgramOptions::max_distance);
            if (flag_field) ofs << ' ' << queries.flag(idx);
            if (score_field) ofs << ' ' << queries.score(idx);
            if (write_counts) ofs << ' ' << dv[idx].count();
            ofs << ' ' << dv[idx].m2();
            ofs << ' ' << dv[idx].min();
            ofs << ' ' << dv[idx].max();
            ofs << '\n';
        }
    }

};
