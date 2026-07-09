#!/bin/bash
#
# run_unit_and_integration_tests.sh - Run unit and integration test executables.
#
# MIT License (see LICENSE in the repository root).
# Copyright (c) 2020-2026 Juri Kuronen
#
# Called by the `make check`, `make check-unit`, and `make check-integration` targets.
#
# Takes a short description and a list of test executables as arguments. The executables should follow the lightweight
# test harness implemented in `common/test_harness`.
#
set -u
set -o pipefail

# Not an error; this can happen when `make check TESTS=...` filters to zero matches in some project.
if [[ $# -lt 2 ]]; then
    exit 0
fi

test_description="$1"
shift
TESTS=("$@")

total_tests=0
total_passes=0
failed_test_suites=()

test_output="$(mktemp)" || { echo "mktemp failed"; exit 1; }

trap 'rm -f "$test_output"' EXIT INT TERM

echo "Running $test_description.";

# Run each test executable and parse a `passes/n_tests` summary.
for test in "${TESTS[@]}"; do
    "$test" 2>&1 | tee "$test_output"
    test_status=$?
    n_tests="$(perl -nE 'print m{\d+/(\d+)}' "$test_output")"
    passes="$(perl -nE 'print m{(\d+)/\d+}' "$test_output")"
    n_tests="${n_tests:-0}"
    passes="${passes:-0}"
    total_tests=$((total_tests + n_tests))
    total_passes=$((total_passes + passes))

    # Mark the test suite as failed if it exited unsuccessfully, any tests failed or no tests were run at all.
    if [ "$test_status" -ne 0 ] || [ "$passes" -ne "$n_tests" ] || [ "$n_tests" -eq 0 ]; then
        failed_test_suites+=("$test")
    fi
done;

echo "$test_description ($total_passes/$total_tests)"

if [ "${#failed_test_suites[@]}" -gt 0 ]; then
    echo ""
    echo "Failed test suites:"
    for failed_test_suite in "${failed_test_suites[@]}"; do
        echo "  $(basename "$failed_test_suite")"
    done

    exit 1
fi

exit 0
