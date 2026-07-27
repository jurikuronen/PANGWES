#!/usr/bin/perl
#
# verify_gfa_parser_results.pl - Compare computed gfa_parser results against expected test data.
#
# MIT License (see LICENSE in the repository root).
# Copyright (c) 2020-2026 Juri Kuronen
#
# Called by `run_gfa_parser_e2e_tests.sh`.
#
# Takes an expected results file path and a newly computed results file path as arguments.
#
# Arguments:
# - Test data directory where expected results live.
# - File path stem for computed results.
#
use strict;
use warnings;
use v5.10;

exit 1 if @ARGV != 3;

my ($test_data_dir, $expected_results_stem, $computed_results_stem) = @ARGV;

my $expected_unitigs_file = "$test_data_dir/$expected_results_stem.unitigs";
my $expected_fasta_file = "$test_data_dir/$expected_results_stem.fasta";
my $expected_paths_file = "$test_data_dir/$expected_results_stem.paths";

my $computed_unitigs_file = $computed_results_stem . ".unitigs";
my $computed_fasta_file = $computed_results_stem . ".fasta";
my $computed_paths_file = $computed_results_stem . ".paths";


########################################################################################################################
# Logic for determining that the unitigs were computed correctly.                                                      #
########################################################################################################################

sub extract_unitigs_data_from_file {
    my $filename = shift;
    my %unitigs;
    my %seen_unitig_ids;
    my $i = 0;

    open(my $FH, "<", $filename) or die $!;
    while (<$FH>) {
        chomp;
        ++$i;

        my @data = split /\s+/;

        if (@data < 2) {
            say "Unitigs file \"$filename\" line $i has only " . scalar(@data) . " fields (required >= 2).";
            exit 1;
        }

        my $unitig_id = $data[0];
        my $unitig_str = $data[1];
        my $expected_unitig_id = $i - 1;

        if ($unitig_id ne "$expected_unitig_id") {
            say "Wrong unitig ID found in unitigs file \"$filename\" line $i: got \"$unitig_id\"; " .
                "expected \"$expected_unitig_id\".";
            exit 1;
        }

        if (exists $seen_unitig_ids{$unitig_id}) {
            say "Duplicate unitig ID found in unitigs file \"$filename\" line $i: \"$unitig_id\" already mapped to " .
                "\"$seen_unitig_ids{$unitig_id}\"; attempted to map to \"$unitig_str\".";
            exit 1;
        }

        if (exists $unitigs{$unitig_str}) {
            say "Duplicate unitig sequence found in unitigs file \"$filename\" line $i: \"$unitig_str\" already " .
                "mapped to \"$unitigs{$unitig_str}\"; attempted to map to \"$unitig_id\".";
            exit 1;
        }

        $seen_unitig_ids{$unitig_id} = $unitig_str;
        $unitigs{$unitig_str} = $unitig_id;
    }
    close($FH);

    return \%unitigs;
}

sub check_unitigs {
    my ($expected_unitigs, $computed_unitigs) = @_;

    my $n_expected = scalar keys $expected_unitigs->%*;
    my $n_computed = scalar keys $computed_unitigs->%*;

    if ($n_expected != $n_computed) {
        say "Count of computed unitigs doesn't match expectation; expected $n_expected, got $n_computed.";
        exit 1;
    }

    for my $unitig_str (keys $expected_unitigs->%*) {
        if (not exists $computed_unitigs->{$unitig_str}) {
            say "Computed unitigs don't contain expected unitig sequence \"$unitig_str\".";
            exit 1;
        }

        if ($expected_unitigs->{$unitig_str} != $computed_unitigs->{$unitig_str}) {
            say "Unitig sequence \"$unitig_str\" has ID \"$computed_unitigs->{$unitig_str}\", expected " .
                "\"$expected_unitigs->{$unitig_str}\".";
            exit 1;
        }
    }

    say "Unitigs OK.";
}

my $expected_unitigs = extract_unitigs_data_from_file($expected_unitigs_file);
my $computed_unitigs = extract_unitigs_data_from_file($computed_unitigs_file);

check_unitigs($expected_unitigs, $computed_unitigs);

########################################################################################################################
# Logic for determining that the FASTA file was computed correctly.                                                    #
########################################################################################################################

sub extract_fasta_data_from_file {
    my $filename = shift;
    my @data;
    my $current_header = "";
    my $current_sequence = "";
    my $first_header_read = 0;
    my $i = 0;

    open(my $FH, "<", $filename) or die $!;

    while (<$FH>) {
        chomp;
        ++$i;

        next if $_ eq "";

        # New sequence starts.
        if (/^>/) {
            if ($first_header_read) {
                if ($current_sequence eq "") {
                    say "Empty sequence detected in FASTA file \"$filename\" before line $i.";
                    exit 1;
                }

                push @data, [$current_header, $current_sequence];
                $current_sequence = "";
            }

            $current_header = $_;
            $first_header_read = 1;
            next;
        }

        if (not $first_header_read) {
            say "FASTA file \"$filename\" line $i contains sequence data before the first FASTA header.";
            exit 1;
        }

        if (/[^ac]/) {
            say "FASTA file \"$filename\" line $i contains invalid character(s): \"$_\" (allowed: a, c).";
            exit 1;
        }

        # Append sequence data from this row to the current sequence.
        $current_sequence .= $_;
    }
    close($FH);

    if (not $first_header_read) {
        say "FASTA file \"$filename\" contains no FASTA headers.";
        exit 1;
    }

    if ($current_sequence eq "") {
        say "Empty sequence detected at the end of FASTA file \"$filename\".";
        exit 1;
    }

    push @data, [$current_header, $current_sequence];

    return \@data;
}

sub check_fasta_data {
    my ($expected_fasta_data, $computed_fasta_data) = @_;

    my $n_expected = scalar $expected_fasta_data->@*;
    my $n_computed = scalar $computed_fasta_data->@*;

    if ($n_expected != $n_computed) {
        say "Count of FASTA sequences doesn't match expectation; expected $n_expected, got $n_computed.";
        exit 1;
    }

    for (my $i = 0; $i < $n_expected; ++$i) {
        my $computed_fasta_header = $computed_fasta_data->[$i][0];
        my $expected_fasta_header = $expected_fasta_data->[$i][0];
        if ($expected_fasta_header ne $computed_fasta_header) {
            say "Computed FASTA header " . ($i + 1) . " (\"$computed_fasta_header\") does not match " .
                "expected FASTA header " . ($i + 1) . " (\"$expected_fasta_header\").";
            exit 1;
        }

        my $computed_fasta_sequence = $computed_fasta_data->[$i][1];
        my $expected_fasta_sequence = $expected_fasta_data->[$i][1];
        if ($expected_fasta_sequence ne $computed_fasta_sequence) {
            say "Computed FASTA sequence " . ($i + 1) . " does not match expected FASTA sequence " . ($i + 1) . ".";
            exit 1;
        }
    }

    say "FASTA data OK.";
}

my $expected_fasta_data = extract_fasta_data_from_file($expected_fasta_file);
my $computed_fasta_data = extract_fasta_data_from_file($computed_fasta_file);

check_fasta_data($expected_fasta_data, $computed_fasta_data);

########################################################################################################################
# Logic for determining that the .paths file has the right contents and the *.edges files were computed correctly.     #
########################################################################################################################

# Read a single .edges file.
sub extract_edges_data_from_file {
    my ($filename) = @_;
    my %graph;
    my $i = 0;

    open(my $FH, "<", $filename) or die $!;
    while (<$FH>) {
        chomp;
        ++$i;

        my @data = split /\s+/;

        if (@data < 3) {
            say "SGG edges file \"$filename\" line $i has only " . scalar(@data) . " fields (required >= 3).";
            exit 1;
        }

        my ($unitig1_id, $unitig2_id, $orientation, $overlap) = @data;

        # The expected results, computed with an older version of gfa_parser, wrote out zero-overlaps and self-edges.
        next if defined $overlap && $overlap eq "0M";
        next if $unitig1_id == $unitig2_id && ($orientation eq "FR" || $orientation eq "RF");

        if (exists $graph{$unitig1_id}{$unitig2_id}{$orientation}) {
            say "Duplicate edge found in SGG edges file \"$filename\" on line $i.";
            exit 1;
        }

        $graph{$unitig1_id}{$unitig2_id}{$orientation} = 1;
    }
    close($FH);

    unless (scalar keys %graph) {
        say "Could not read edge data from SGG edges file \"$filename\".";
        exit 1;
    }

    return \%graph;
}

# Read all .edges files listed in the .paths file, including the filename.
sub extract_paths_data_from_files {
    my ($filename, $base_dir) = @_;

    open(my $FH, "<", $filename) or die $!;
    my $paths = [ map {
        chomp;
        my $edges_file = defined $base_dir ? "$base_dir/$_" : $_;
        # Extract the `_paths/<filename>` suffix.
        my ($edges_filename) = $_ =~ m{(_paths/[^/]+)$};
        [$edges_filename, extract_edges_data_from_file($edges_file)]
    } <$FH> ];
    close($FH);

    return $paths;
}

sub check_graphs_match {
    my ($expected_graph, $computed_graph) = @_;

    my @expected_unitig_ids = keys $expected_graph->%*;
    my @computed_unitig_ids = keys $computed_graph->%*;

    return 0 if @expected_unitig_ids != @computed_unitig_ids;

    for my $unitig1_id (@expected_unitig_ids) {
        return 0 if not exists $computed_graph->{$unitig1_id};

        my @expected_neighbors = keys $expected_graph->{$unitig1_id}->%*;
        my @computed_neighbors = keys $computed_graph->{$unitig1_id}->%*;

        return 0 if @expected_neighbors != @computed_neighbors;

        for my $unitig2_id (@expected_neighbors) {
            return 0 if not exists $computed_graph->{$unitig1_id}{$unitig2_id};

            my @expected_orientations = keys $expected_graph->{$unitig1_id}{$unitig2_id}->%*;
            my @computed_orientations = keys $computed_graph->{$unitig1_id}{$unitig2_id}->%*;

            return 0 if @expected_orientations != @computed_orientations;

            for my $orientation (@expected_orientations) {
                return 0 if not exists $computed_graph->{$unitig1_id}{$unitig2_id}{$orientation};
                return 0 if $expected_graph->{$unitig1_id}{$unitig2_id}{$orientation}
                    ne $computed_graph->{$unitig1_id}{$unitig2_id}{$orientation};
            }
        }
    }

    return 1;
}

sub check_paths_data {
    my ($expected_paths_data, $computed_paths_data) = @_;

    my $n_expected = scalar $expected_paths_data->@*;
    my $n_computed = scalar $computed_paths_data->@*;

    if ($n_expected != $n_computed) {
        say "Count of SGG edges files doesn't match expectation; expected $n_expected, got $n_computed.";
        exit 1;
    }

    for (my $i = 0; $i < $n_expected; ++$i) {
        my ($expected_edges_filename, $expected_graph) = @{$expected_paths_data->[$i]};
        my ($computed_edges_filename, $computed_graph) = @{$computed_paths_data->[$i]};

        if ($expected_edges_filename ne $computed_edges_filename) {
            say "Expected .paths file line " . ($i + 1) . " to be \"$expected_edges_filename\, got \"" .
                $computed_edges_filename . "\".";
            exit 1;
        }

        if (!check_graphs_match($expected_graph, $computed_graph)) {
            say "No computed SGG edges file matches expected SGG edges file " . ($i + 1) . ".";
            exit 1;
        }
    }

    say "SGG edges OK.";
}

my $expected_paths_data = extract_paths_data_from_files($expected_paths_file, $test_data_dir);
my $computed_paths_data = extract_paths_data_from_files($computed_paths_file);

check_paths_data($expected_paths_data, $computed_paths_data);

say "";
say "SUCCESS";
