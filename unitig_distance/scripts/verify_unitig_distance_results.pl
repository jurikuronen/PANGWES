#!/usr/bin/perl
#
# verify_unitig_distance_results.pl - Compare computed unitig_distance results against expected results.
#
# MIT License (see LICENSE in the repository root).
# Copyright (c) 2020-2026 Juri Kuronen
#
# Called by `run_unitig_distance_e2e_tests.sh`.
#
# Arguments:
# - Expected results file path.
# - Computed results file path.
# - Median distances calculated: 1 if yes, 0 if no.
#
use strict;
use warnings;
use v5.10;

exit 1 if @ARGV < 2 || @ARGV > 3;

my ($expected_results_file, $results_file, $median_distances_calculated) = @ARGV;

my (@expected_unitig_ids, @expected_mean_distances, @expected_other_fields, @expected_sgg_counts,
    @expected_sample_variances, @expected_min_distances, @expected_max_distances, @expected_median_distances);

my $i = 0;

open(my $FH_E, "<", $expected_results_file) or die $!;

# First read and store the expected results.
while (<$FH_E>) {
    chomp;
    ++$i;

    my @data = split /\s+/;

    if (@data < 10) {
        say "Expected results file line $i has only " . scalar(@data) . " fields (required >= 10).";
        exit 1;
    }

    push @expected_unitig_ids, $data[0] . " " . $data[1];
    push @expected_mean_distances, $data[2];
    push @expected_other_fields, $data[3] . " " . $data[4];
    push @expected_sgg_counts, $data[5];
    push @expected_sample_variances, $data[6];
    push @expected_min_distances, $data[7];
    push @expected_max_distances, $data[8];
    push @expected_median_distances, ($median_distances_calculated ? $data[9] : -1);
}
close($FH_E);

say "Read $i lines from the expected results file.";

# Reset line counter.
$i = 0;

open(my $FH, "<", $results_file) or die $!;

# Now compare the computed results against the expected results.
while (<$FH>) {
    if ($i >= scalar @expected_mean_distances) {
        say "Results file has an unexpected extra line " . ($i + 1) . ".";
        exit 1;
    }

    # Get expected results before incrementing line counter.
    my $expected_unitig_ids = $expected_unitig_ids[$i];
    my $expected_mean_distance = $expected_mean_distances[$i];
    my $expected_other_fields = $expected_other_fields[$i];
    my $expected_sgg_count = $expected_sgg_counts[$i];
    my $expected_sample_variance = $expected_sample_variances[$i];
    my $expected_min_distance = $expected_min_distances[$i];
    my $expected_max_distance = $expected_max_distances[$i];
    my $expected_median_distance = $median_distances_calculated ? $expected_median_distances[$i] : -1;

    chomp;
    ++$i;

    my @data = split /\s+/;

    if (@data < 10) {
        say "Results file line $i has only " . scalar(@data) . " fields (required >= 10).";
        exit 1;
    }

    my $unitig_ids = $data[0] . " " . $data[1];
    my $mean_distance = $data[2];
    my $other_fields = $data[3] . " " . $data[4];
    my $sgg_count = $data[5];
    my $sample_variance = $data[6];
    my $min_distance = $data[7];
    my $max_distance = $data[8];
    my $median_distance = $data[9];

    if ($unitig_ids ne $expected_unitig_ids) {
        say "Unexpected unitig IDs on line $i: $unitig_ids != $expected_unitig_ids.";
        exit 1;
    }

    if ($other_fields ne $expected_other_fields) {
        say "Unexpected other SpydrPick fields on line $i: $other_fields != $expected_other_fields.";
        exit 1;
    }

    if (abs($mean_distance - $expected_mean_distance) > 0) {

        say "Unexpected mean distance on line $i: $mean_distance != $expected_mean_distance.";
        exit 1;
    }

    if (abs($sgg_count - $expected_sgg_count) > 0) {
        say "Unexpected count on line $i: $sgg_count != $expected_sgg_count.";
        exit 1;
    }

    if (abs($min_distance - $expected_min_distance) > 0) {
        say "Unexpected min distance on line $i: $min_distance != $expected_min_distance.";
        exit 1;
    }

    if (abs($max_distance - $expected_max_distance) > 0) {
        say "Unexpected max distance on line $i: $max_distance != $expected_max_distance.";
        exit 1;
    }

    if ($sgg_count >= 2 && $expected_sample_variance > 0) {
        my $var_ratio = $sample_variance / $expected_sample_variance;
        $var_ratio = 1 / $var_ratio if $var_ratio > 1;

        if ($var_ratio < 0.99999) {
            say "Unexpected sample variance on line $i: $sample_variance != $expected_sample_variance.";
            exit 1;
        }
    } else {
        if ($sample_variance != $expected_sample_variance) {
            say "Unexpected sample variance on line $i: $sample_variance != $expected_sample_variance.";
            exit 1;
        }
    }

    if (abs($median_distance - $expected_median_distance) > 0) {
        say "Unexpected median distance on line $i: $median_distance != $expected_median_distance.";
        exit 1;
    }
}
close($FH);

if ($i == 0) {
    say "Empty results file.";
    exit 1;
}

say "Verified that $i lines are ok in the results file.";
say "";
say "SUCCESS";
