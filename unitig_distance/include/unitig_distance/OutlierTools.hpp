#pragma once

#include <algorithm>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "DistanceVector.hpp"
#include "ProgramOptions.hpp"
#include "Queries.hpp"
#include "ResultsWriter.hpp"
#include "types.hpp"
#include "Utils.hpp"

class OutlierTools {
public:
    OutlierTools() = delete;
    OutlierTools(const Queries& queries, Timer& timer)
    : m_queries(queries),
      m_timer(timer),
      m_largest_score(0.0),
      m_n_vs(0)
    {
        if (ProgramOptions::has_operating_mode(OperatingMode::OUTLIER_TOOLS) && queries.extended_format()) {
            std::tie(m_largest_score, m_n_vs) = calculate_query_values();
        } else {
            std::tie(m_largest_score, m_n_vs) = std::make_pair(0.0, 0);
        }
    }

    // Estimate outlier thresholds. Also estimate linkage disequilibrium distance if ld_distance < 0.
    void determine_and_output_outliers(const DistanceVector& distances, const std::string& outliers_filename, const std::string& outlier_stats_filename) const {
        if (!m_queries.extended_format()) {
            std::cout << "    OutlierTools: No scores for unitig pairs available. Cannot determine outliers." << std::endl;
            return;
        }

        int_t count_threshold = distances.storing_mean_distances() ? ProgramOptions::sgg_count_threshold : 0;
        Parameters params(count_threshold);

        // Estimate ld distance if necessary.
        if (ProgramOptions::ld_distance < 0) {
            real_t largest_distance = calculate_largest_distance(distances, count_threshold);
            real_t min_distance = ProgramOptions::ld_distance_min;
            real_t required_score = ProgramOptions::ld_distance_score * m_largest_score;

            if (largest_distance < min_distance) {
                if (ProgramOptions::verbose) {
                    std::cout << "    OutlierTools: Distances in queries are smaller than the provided minimum ld distance (" << (int_t) largest_distance
                              << '<' << min_distance << "). Ignoring the given value." << std::endl;
                }
                min_distance = 0.0;
            }

            determine_ld_automatically(distances, min_distance, largest_distance, required_score, params);
        }
        // Calculate outlier threshold values with given ld distance if required, otherwise the values are already in place.
        else if (ProgramOptions::outlier_threshold < 0.0) calculate_parameters(distances, params);

        // Collect outliers.
        auto outlier_indices = collect_outliers(distances, params);

        // Output outliers.
        if (outlier_indices.size() > 0) {
            ResultsWriter::output_results(outliers_filename, m_queries, distances, outlier_indices);

            std::ofstream ofs(outlier_stats_filename);
            ofs << (int_t) params.ld_distance << ' ' << params.outlier_threshold
                << ' ' << params.extreme_outlier_threshold << ' ' << params.count_threshold << '\n';

            if (ProgramOptions::verbose) {
                PrintUtils::print_tbss_tsmasm(m_timer, "Output", Utils::neat_number_str(outlier_indices.size()),
                                              "outliers to files", outliers_filename, "and", outlier_stats_filename);
            }
        } else if (ProgramOptions::verbose) {
            PrintUtils::print_tbss_tsmasm_noendl(m_timer, "Outlier tools finished");
            std::cout << "No outliers could be collected with the current values." << std::endl;
        }
    }

private:
    struct Parameters {
        Parameters() = delete;
        Parameters(int_t ct)
        : ld_distance(ProgramOptions::ld_distance),
          outlier_threshold(ProgramOptions::outlier_threshold),
          extreme_outlier_threshold(ProgramOptions::outlier_threshold),
          v_coverage(0),
          count_threshold(ct),
          max_score(0.0)
        { }

        real_t ld_distance;
        real_t outlier_threshold;
        real_t extreme_outlier_threshold;
        int_t v_coverage;
        int_t count_threshold;
        real_t max_score;
    };
    const Queries& m_queries;
    Timer& m_timer;

    real_t m_largest_score;
    int_t m_n_vs;

    std::pair<real_t, int_t> calculate_query_values() {
        std::unordered_set<int_t> vs;
        real_t largest_score = 0.0;
        for (std::size_t i = 0; i < m_queries.size(); ++i) {
            vs.insert(m_queries.v(i));
            vs.insert(m_queries.w(i));
            largest_score = std::max(largest_score, m_queries.score(i));
        }
        return std::make_pair(largest_score, vs.size());
    }

    real_t calculate_largest_distance(const DistanceVector& distances, const Parameters& params) const {
        real_t largest_distance = 0.0;
        for (auto d : distances) {
            if (d.count() < params.count_threshold) continue;
            largest_distance = std::max(largest_distance, Utils::fixed_distance(d));
        }
        return largest_distance;
    }

    void determine_ld_automatically(const DistanceVector& distances, int_t a, int_t b, real_t required_score, Parameters& params) const {
        int_t iter = 0;
        while (b - a > 1) {
            params.ld_distance = (a + b) / 2;
            calculate_parameters(distances, params);
            if (params.max_score < required_score) {
                b = params.ld_distance;
            } else {
                a = params.ld_distance;
            }
            if (ProgramOptions::verbose) {
                std::cout << "    OutlierTools: Iteration " << ++iter
                          << ", outlier threshold=" << params.outlier_threshold << ", extreme outlier threshold=" << params.extreme_outlier_threshold
                          << ", ld distance=" << (int_t) params.ld_distance
                          << ", coverage=" << params.v_coverage << " (" << Utils::neat_decimal_str(100 * params.v_coverage, m_n_vs) << "%)" << std::endl;
            }
        }
    }

    void calculate_parameters(const DistanceVector& distances, Parameters& params) const {
        auto distribution = get_distribution(distances, params);
        if (distribution.size() == 0) {
            params.max_score = 0.0;
            return;
        }

        real_t q1 = get_q(distribution, 1);
        real_t q3 = get_q(distribution, 3);

        params.outlier_threshold = calculate_outlier_threshold(q1, q3);
        params.extreme_outlier_threshold = calculate_extreme_outlier_threshold(q1, q3);
        params.v_coverage = distribution.size();
        params.max_score = max_score_from_end(distribution);
    }

    std::vector<real_t> get_distribution(const DistanceVector& distances, const Parameters& params) const {
        std::vector<real_t> v_scores(m_queries.largest_v() + 1);

        for (std::size_t i = 0; i < m_queries.size(); ++i) {
            if (distances[i].count() < params.count_threshold) continue;
            if (Utils::fixed_distance(distances[i]) < params.ld_distance) continue;
            int_t v = m_queries.v(i);
            int_t w = m_queries.w(i);
            real_t score = m_queries.score(i);
            v_scores[v] = std::max(v_scores[v], score);
            v_scores[w] = std::max(v_scores[w], score);
        }

        std::vector<real_t> distribution;
        for (auto score : v_scores) if (score > 0.0) distribution.push_back(score);
        return distribution;
    }

    real_t get_q(std::vector<real_t>& distribution, int_t q) const {
        int_t q_idx = std::min(distribution.size() - 1, q * distribution.size() / 4);
        std::nth_element(distribution.begin(), distribution.begin() + q_idx, distribution.end());
        return distribution[q_idx];
    }

    real_t max_score_from_end(std::vector<real_t>& distribution) const {
        int_t idx = std::min((int_t) distribution.size() - 1, ProgramOptions::ld_distance_nth_score);
        std::nth_element(distribution.begin(), distribution.begin() + idx, distribution.end(), std::greater<real_t>());
        return distribution[idx];
    }

    real_t calculate_outlier_threshold(real_t q1, real_t q3) const { return q3 + 1.5 * (q3 - q1); }
    real_t calculate_extreme_outlier_threshold(real_t q1, real_t q3) const { return q3 + 3.0 * (q3 - q1); }

    std::vector<int_t> collect_outliers(const DistanceVector& distances, const Parameters& params) const {
        std::vector<int_t> outlier_indices;
        for (std::size_t i = 0; i < m_queries.size(); ++i) {
            if (distances[i].count() < params.count_threshold) continue;
            if (Utils::fixed_distance(distances[i]) < params.ld_distance) continue;
            if (m_queries.score(i) < params.outlier_threshold) continue;
            outlier_indices.push_back(i);
        }
        return outlier_indices;
    }

};

