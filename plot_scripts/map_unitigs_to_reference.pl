#!/usr/bin/perl
#
# map_unitigs_to_reference.pl - Map unitig_distance results to reference-genome positions with BWA-MEM.
#
# MIT License (see LICENSE in the repository root).
# Copyright (c) 2020-2026 Juri Kuronen

use warnings;
use strict;
use v5.20;

# Set to 1 to write out the input used for BWA-MEM to "<mapped_unitig_distance_results_output_file>.bwa_mem_input.fq".
my $write_bwa_mem_input = 0;

die "Call with: $0 <unitigs_file> <unitig_distance_results_file> <n_unitig_distance_rows> " .
    "<mapped_unitig_distance_results_output_file> \"bwa_mem_command\"" if @ARGV < 5;
my (
    $unitigs_file,
    $unitig_distance_results_file,
    $n_unitig_distance_rows,
    $mapped_unitig_distance_results_output_file,
    @bwa_mem_cmd_parts
) = @ARGV;

my $unitig_distance_results_file_one_based = $unitig_distance_results_file =~ /_1_based/;

# Write BWA-MEM input to a separate file if $write_bwa_mem_input is non-zero.
my $bwa_mem_input_file = $write_bwa_mem_input
                         ? "$mapped_unitig_distance_results_output_file.bwa_mem_input.fq"
                         : $mapped_unitig_distance_results_output_file;

# Parse BWA-MEM command.
my $bwa_mem_cmd = join(" ", @bwa_mem_cmd_parts);

# Read unitig sequences.
open(my $fh_unitigs, '<', "$unitigs_file") or die $!;
my @unitigs = map { (split /\s+/)[1] } <$fh_unitigs>;
close($fh_unitigs);
say "Read " . scalar(@unitigs) . " unitigs.";

# Read unitig_distance results and store the original row plus corrected unitig indices.
open(my $fh_unitig_distance_results, "<", $unitig_distance_results_file) or die $!;
my @unitig_distance_rows;
my %unitigs_to_map;
while (<$fh_unitig_distance_results>) {
    last if @unitig_distance_rows >= $n_unitig_distance_rows;
    chomp;

    my $unitig_distance_row = $_;
    my ($unitig1, $unitig2) = split /\s+/, $unitig_distance_row;
    my $unitig1_index = $unitig1 - $unitig_distance_results_file_one_based;
    my $unitig2_index = $unitig2 - $unitig_distance_results_file_one_based;

    push @unitig_distance_rows, [$unitig_distance_row, $unitig1_index, $unitig2_index];
    $unitigs_to_map{$unitig1_index} = 1;
    $unitigs_to_map{$unitig2_index} = 1;
}
close($fh_unitig_distance_results);
say "Read " . scalar(@unitig_distance_rows) . " rows of unitig_distance results.";

# Create BWA-MEM input FASTQ.
open(my $bwa_mem_input_fh, ">", $bwa_mem_input_file) or die $!;
for my $unitig_index (sort { $a <=> $b } keys %unitigs_to_map) {
    say $bwa_mem_input_fh "\@$unitig_index\n$unitigs[$unitig_index]\n+\n", "~" x length($unitigs[$unitig_index]);
}
close($bwa_mem_input_fh);

# Run BWA-MEM on the input-file created above and keep only exact and high-quality primary mappings.
my %mapped_pos;
my %has_multiple_mappings;
open(my $fh_bwa_mem_output, '-|', "$bwa_mem_cmd $bwa_mem_input_file")
    or die "BWA-MEM command failed: $bwa_mem_cmd $bwa_mem_input_file";
while (<$fh_bwa_mem_output>) {
    # Alignment entries start with a numeric unitig ID and a numeric flag.
    next unless /^\d+\t\d+/;
    chomp;

    # For field descriptions, see Sequence Alignment Map (SAM) Format Specification.
    my @fields = split /\t/;
    my ($qname, $flag, undef, $pos, $mapq, $cigar) = @fields[0 .. 5];
    my @tags = @fields[11 .. $#fields];

    # Ignore detected multiple-mappings.
    next if exists $has_multiple_mappings{$qname};

    # Ignore mappings for unitigs that were not present in the unitig_distance results.
    next unless exists $unitigs_to_map{$qname};

    # Ignore unmapped unitigs.
    next if $flag & 4;

    # Skip secondary/alternative alignments.
    next if $flag & 0x100;
    next if grep { /^XA:Z:/ } @tags;

    # Skip supplementary/chimeric alignments.
    next if $flag & 0x800;
    next if grep { /^SA:Z:/ } @tags;

    # Accept only M or = operations. NM:i:0 further rejects mismatches hidden inside M.
    next unless $cigar =~ /^(?:\d+[M=])+$/;
    next unless grep { $_ eq "NM:i:0" } @tags;

    # Require a high mapping quality.
    next unless $mapq >= 30;

    if (exists $mapped_pos{$qname}) {
        say "Rejecting unitig $qname: multiple accepted mappings";
        delete $mapped_pos{$qname};
        $has_multiple_mappings{$qname} = 1;
        next;
    }

    $mapped_pos{$qname} = $pos;
}
close($fh_bwa_mem_output) or die "BWA-MEM command failed: $bwa_mem_cmd $bwa_mem_input_file";

# Write out mapped unitig_distance rows.
open(my $fh_mapped_unitig_distance_results, ">", $mapped_unitig_distance_results_output_file) or die $!;
my $n_out_rows = 0;
for my $unitig_distance_row (@unitig_distance_rows) {
    my ($unitig_distance_row_text, $unitig1_index, $unitig2_index) = $unitig_distance_row->@*;
    my $pos1 = $mapped_pos{$unitig1_index};
    my $pos2 = $mapped_pos{$unitig2_index};

    # Skip unitig pairs where either unitig failed the mapping filters.
    next unless defined $pos1 && defined $pos2;

    say $fh_mapped_unitig_distance_results join(" ",
                                                $unitig_distance_row_text,
                                                $pos1,
                                                $pos2,
                                                $unitigs[$unitig1_index],
                                                $unitigs[$unitig2_index]);
    ++$n_out_rows;
}
close($fh_mapped_unitig_distance_results);

say "Wrote $n_out_rows rows of mapped unitig distance results to $mapped_unitig_distance_results_output_file.";
