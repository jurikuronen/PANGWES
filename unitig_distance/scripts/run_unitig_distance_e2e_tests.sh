#!/bin/bash
#
# run_unitig_distance_e2e_tests.sh - Run unitig_distance end-to-end tests.
#
# MIT License (see LICENSE in the repository root).
# Copyright (c) 2020-2026 Juri Kuronen
#
# Called by the `make check-e2e` target.
#
# Arguments:
# - Path to the unitig_distance executable.
# - Path to the results verification script.
# - Test data directory.
#
set -u
set -o pipefail

if [[ $# -lt 3 ]]; then
    echo "Wrong number of arguments: got $#, expected 3"
    exit 1
fi

unitig_distance_executable="$1"
results_verification_script="$2"
test_data_dir="$3"

if [[ ! -x "$unitig_distance_executable" ]]; then
    echo "unitig_distance executable not found or not executable: $unitig_distance_executable"
    exit 1
fi

if [[ ! -x "$results_verification_script" ]]; then
    echo "Test runner script not found or not executable: $results_verification_script"
    exit 1
fi

if [[ ! -d "$test_data_dir" ]]; then
    echo "Could not find test data directory: $test_data_dir"
    exit 1
fi

tmp_dir="$(mktemp -d)" || { echo "mktemp -d failed"; exit 1; }

trap 'find "$tmp_dir/" -type f -name "test*" -delete; \
      find "$tmp_dir/" -type d -empty -delete;' EXIT

run_dataset_tests() {
    local dataset_stem="$1"
    local calculate_median_distances="$2";
    local test_paths_file="$tmp_dir/${dataset_stem}.paths"
    local expected_results_file="$test_data_dir/${dataset_stem}.queries"

    if [[ ! -f "$expected_results_file" ]]; then
        echo "Could not find expected results file: $expected_results_file"
        exit 1
    fi

    # Recreate .paths file with correct full paths.
    find "$test_data_dir/${dataset_stem}_paths" -type f > "$test_paths_file"

    local -a unitig_distance_common_options=(
        --unitigs-file "$test_data_dir/${dataset_stem}.unitigs"
        --k-mer-length 31
        --sgg-paths-file "$test_paths_file"
        --queries-file "$test_data_dir/${dataset_stem}.queries"
    )

    local median_suffix=""

    if [[ "$calculate_median_distances" -eq 1 ]]; then
        median_suffix="_median"
    else
        unitig_distance_common_options+=(--no-median-distance)
    fi

    local test_out_single_thread="${dataset_stem}_singlethreaded${median_suffix}"
    local test_out_multi_thread="${dataset_stem}_multithreaded${median_suffix}"

    echo ""
    echo "============================================================"
    echo "Dataset: $dataset_stem"
    echo "Calculate median: $calculate_median_distances"
    echo "============================================================"

    echo ""
    echo "------------------------------------------------------------"
    echo "Single-threaded run."
    echo "------------------------------------------------------------"

    if ! "$unitig_distance_executable" "${unitig_distance_common_options[@]}" \
                                       --output-stem "$tmp_dir/$test_out_single_thread"
    then
        echo "$unitig_distance_executable encountered an error"
        exit 1
    fi

    results_file_single_thread=$(find "$tmp_dir/" -type f -iname "${test_out_single_thread}*" | head -n 1)

    echo ""
    echo "Verifying single-threaded run results..."

    "$results_verification_script" "$expected_results_file" \
                                   "$results_file_single_thread" \
                                   "$calculate_median_distances" || exit 1

    echo ""
    echo "------------------------------------------------------------"
    echo "Multi-threaded run."
    echo "------------------------------------------------------------"

    n_threads=$(nproc)

    if [[ "$n_threads" -eq 1 ]]; then
        # Default to a few threads to test multithreaded execution on systems with only one processing unit available.
        n_threads=4
    fi

    if ! "$unitig_distance_executable" "${unitig_distance_common_options[@]}" \
                                       --output-stem "$tmp_dir/$test_out_multi_thread" \
                                       --threads "$n_threads"
    then
        echo "$unitig_distance_executable encountered an error"
        exit 1
    fi

    results_file_multi_thread=$(find "$tmp_dir/" -type f -iname "${test_out_multi_thread}*" | head -n 1)

    echo ""
    echo "Verifying multi-threaded run results..."

    "$results_verification_script" "$expected_results_file" \
                                   "$results_file_multi_thread" \
                                   "$calculate_median_distances" || exit 1
}

run_dataset_tests "test_efc_k31" 0
run_dataset_tests "test_maela_k31" 0
run_dataset_tests "test_efc_k31" 1
run_dataset_tests "test_maela_k31" 1

exit 0
