/*
 * UnitigDistanceResults.cpp - Results-writing function for unitig_distance results.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

#include "common/utils/Exception.hpp"
#include "unitig_distance/io/QueriesReader.hpp"
#include "unitig_distance/io/UnitigDistanceResults.hpp"

namespace PANGWES {
namespace {

constexpr auto MAX_SAMPLE_VARIANCE_DIGITS = 7;

// Same formatting as SpydrPick for floating points.
constexpr auto FLOATING_POINT_PRECISION = 6;

/*
 * Reduces the number of displayed decimals in the sample variance when the integer part is large.
 *
 * The allowed number of displayed decimals is `MAX_SAMPLE_VARIANCE_DIGITS` - integer part digits.
 *
 * Returns "0" for value equal to zero.
*/
std::string format_sample_variance(double sample_variance) {
    if (sample_variance == 0.0) {
        return "0";
    }

    if (sample_variance < 0.0) {
        throw Exception(ErrorCode::INVALID_STATE, "sample variance can't be negative");
    }

    const auto integer_part_digits =
        sample_variance < 1.0 ? 0 : static_cast<int>(std::floor(std::log10(sample_variance))) + 1;
    const auto decimal_part_digits = std::max(0, MAX_SAMPLE_VARIANCE_DIGITS - integer_part_digits);

    std::ostringstream oss;

    oss << std::fixed << std::setprecision(decimal_part_digits) << sample_variance;

    return oss.str();
}

// Returns the rounded mean distance value converted to an integer.
std::int64_t format_mean_distance(double mean_distance) {
    return static_cast<std::int64_t>(std::round(mean_distance));
}

} // namespace

namespace UnitigDistanceResults {

void write_results(std::unique_ptr<FileWriterInterface>& writer,
                   QueriesReader& queries_reader,
                   const std::vector<Distance>& distances,
                   bool output_one_based)
{
    const auto one_based = static_cast<std::size_t>(output_one_based);

    writer->out() << std::setprecision(FLOATING_POINT_PRECISION) << std::fixed;

    queries_reader.start_reading();

    for (std::size_t idx = 0; idx < distances.size(); ++idx) {
        QueriesReader::QueryData query_data{};

        if (!queries_reader.getquery(query_data)) {
            throw Exception(ErrorCode::QUERY_COUNT_MISMATCH,
                            "failed to re-read query for computed distance at index ", idx);
        }

        const auto unitig1_id = query_data.unitig1_id + one_based;
        const auto unitig2_id = query_data.unitig2_id + one_based;
        const auto& aracne_flag_and_mi_score_fields = query_data.aracne_flag_and_mi_score_fields;

        // Default values indicating the given distance statistic is not available.
        std::int64_t mean_distance{-1};
        std::string sample_variance{"-1"};
        std::int64_t min_distance{-1};
        std::int64_t max_distance{-1};
        std::int64_t median_distance{-1};

        const auto& distance = distances[idx];
        const auto count = distance.count();

        if (count > 0) {
            mean_distance = format_mean_distance(distance.mean_distance());
            min_distance = static_cast<std::int64_t>(distance.min_distance());
            max_distance = static_cast<std::int64_t>(distance.max_distance());

            if (count > 1) {
                sample_variance = format_sample_variance(distance.sample_variance());
            }

            if (distance.has_median_distance()) {
                median_distance = static_cast<std::int64_t>(distance.median_distance());
            }
        }

        writer->out() << unitig1_id << ' '
                      << unitig2_id << ' '
                      << mean_distance << ' '
                      << aracne_flag_and_mi_score_fields << ' '
                      << count << ' '
                      << sample_variance << ' '
                      << min_distance << ' '
                      << max_distance << ' '
                      << median_distance
                      << '\n';
    }

    queries_reader.stop_reading();
}

} // namespace UnitigDistanceResults
} // namespace PANGWES
