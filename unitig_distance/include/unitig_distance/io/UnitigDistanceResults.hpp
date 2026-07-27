/*
 * UnitigDistanceResults.hpp - Results-writing function for unitig_distance results.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <memory>
#include <vector>

#include "common/io/FileWriterInterface.hpp"
#include "unitig_distance/core/Distance.hpp"
#include "unitig_distance/io/QueriesReader.hpp"

namespace PANGWES {
namespace UnitigDistanceResults {

/*
 * Writes out results of the unitig distance program.
 * The queries file will be re-read to output back the queries data.
*/
void write_results(std::unique_ptr<FileWriterInterface>& writer,
                   QueriesReader& queries_reader,
                   const std::vector<Distance>& distances,
                   bool output_one_based);

} // namespace UnitigDistanceResults
} // namespace PANGWES
