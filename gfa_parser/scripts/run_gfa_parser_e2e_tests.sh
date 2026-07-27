#!/bin/bash
#
# run_gfa_parser_e2e_tests.sh - Run gfa_parser end-to-end tests.
#
# MIT License (see LICENSE in the repository root).
# Copyright (c) 2020-2026 Juri Kuronen
#
# Called by the `make check-e2e` target.
#
# Arguments:
# - Path to the gfa_parser executable.
# - Path to the results verification script.
# - Test data directory.
#
set -u
set -o pipefail

if [[ $# -lt 3 ]]; then
    echo "Wrong number of arguments: got $#, expected 3"
    exit 1
fi

gfa_parser_executable="$1"
results_verification_script="$2"
test_data_dir="$3"

if [[ ! -x "$gfa_parser_executable" ]]; then
    echo "gfa_parser executable not found or not executable: $gfa_parser_executable"
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

trap 'find "$tmp_dir/" -type f -name "test_*k31*" -delete; \
      find "$tmp_dir/" -type f -name "*.edges" -delete; \
      find "$tmp_dir/" -type d -empty -delete' EXIT

run_dataset_tests() {
    local dataset_stem="$1"
    local expected_gfa_file="$test_data_dir/${dataset_stem}.gfa1"
    local -a gfa_parser_common_options=(
        --gfa-file "$expected_gfa_file"
        --gfa-format 1
        --k-mer-length 31
    )

    if [[ ! -f "$expected_gfa_file" ]]; then
        echo "Could not find expected GFA file: $expected_gfa_file"
        exit 1
    fi

    echo ""
    echo "============================================================"
    echo "Dataset: $dataset_stem"
    echo "============================================================"

    echo ""
    echo "------------------------------------------------------------"
    echo "Single-threaded run."
    echo "------------------------------------------------------------"

    if ! "$gfa_parser_executable" "${gfa_parser_common_options[@]}" \
                                  --output-stem "$tmp_dir/${dataset_stem}"
    then
        echo "$gfa_parser_executable encountered an error"
        exit 1
    fi

    echo ""
    echo "Verifying single-threaded run results..."

    "$results_verification_script" "$test_data_dir" "$dataset_stem" "$tmp_dir/${dataset_stem}" || exit 1

    echo ""
    echo "------------------------------------------------------------"
    echo "Multi-threaded run."
    echo "------------------------------------------------------------"

    n_threads=$(nproc)

    if [[ "$n_threads" -eq 1 ]]; then
        # Default to a few threads to test multithreaded execution on systems with only one processing unit available.
        n_threads=4
    fi

    if ! "$gfa_parser_executable" "${gfa_parser_common_options[@]}" \
                                  --output-stem "$tmp_dir/${dataset_stem}_multi_thread" \
                                  --threads "$n_threads"
    then
        echo "$gfa_parser_executable encountered an error"
        exit 1
    fi

    echo ""
    echo "Verifying multi-threaded run results..."
    "$results_verification_script" "$test_data_dir" "$dataset_stem" "$tmp_dir/${dataset_stem}_multi_thread" || exit 1
}

run_dataset_tests "test_efc_k31"
run_dataset_tests "test_maela_k31"

exit 0
