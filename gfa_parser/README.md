# PAN-GWES/gfa_parser

**gfa_parser** is a command-line program for parsing [GFA 1.0](https://github.com/GFA-spec/GFA-spec/blob/master/GFA1.md)
files that represent **compacted de Bruijn graphs (cdBGs)** constructed from assemblies using
[COMBINE-lab/cuttlefish](https://github.com/COMBINE-lab/cuttlefish). It converts these files into input formats suitable
for [unitig_distance](../unitig_distance) and [santeripuranen/SpydrPick](https://github.com/santeripuranen/SpydrPick).

**gfa_parser** is designed for Linux and HPC environments, including systems with older Linux distributions and
compilers:
- Written in C++11 and developed using the [GNU C++ compiler](https://gcc.gnu.org/).
- No external dependencies.
- Builds with a single `Makefile`.
- Supports multithreaded execution through an internal worker pool.

**gfa_parser** is part of the PAN-GWES pipeline described in *Kuronen et al. (2024)* (see [Cite](#cite)). It is intended
to be used together with:
1. [COMBINE-lab/cuttlefish](https://github.com/COMBINE-lab/cuttlefish), which constructs a GFA-formatted cdBG from
   reference sequences.
2. [santeripuranen/SpydrPick](https://github.com/santeripuranen/SpydrPick), which computes mutual-information scores and
   produces a list of high-scoring candidate unitig pairs.
3. [unitig_distance](../unitig_distance), which computes shortest-path distance statistics between pairs of
   unitigs across single-genome graphs (SGGs).

## Table of contents
- [Installation](#installation)
- [Quick start](#quick-start)
- [Command-line options](#command-line-options)
- [Parsing GFA 1.0 files](#parsing-gfa-10-files)
- [Output](#output)
  - [Unitigs file (STEM.unitigs)](#unitigs-file-stem-unitigs)
  - [FASTA file (STEM.fasta)](#fasta-file-stem-fasta)
  - [Paths file (STEM.paths)](#paths-file-stem-paths)
  - [Path edge-list files (STEM_paths/*.edges)](#path-edge-list-files-stem_pathsedges)
- [Tests](#tests)
- [License](#license)
- [Cite](#cite)

## Installation

Run:

```
git clone https://github.com/jurikuronen/PANGWES
cd PANGWES
make -C gfa_parser
```

This creates the executable at `PANGWES/bin/gfa_parser`.

## Quick start

Given a GFA 1.0 file named `cdbg_k61.gfa1` produced by **Cuttlefish**, run:

```
gfa_parser --gfa-file      /path/to/cdbg_k61.gfa1  \
           --gfa-format    1                       \
           --k-mer-length  61                      \
           --output-stem   /path/to/cdbg_k61
```

The command writes several files using `/path/to/cdbg_k61` as the output stem. See [Output](#output) for a
description of these files.

## Command-line options

```
Graphical Fragment Assembly (GFA) format options:
    -G, --gfa-file PATH
    -F, --gfa-format FORMAT
    -k, --k-mer-length K

Other options:
    -o, --output-stem STEM
    -t, --threads N
    -q, --quiet
    -h, --help
    -v, --version
```

More information is available by running `gfa_parser --help`.

## Parsing GFA 1.0 files

The parser is intentionally restricted to the requirements of the current PAN-GWES workflow.

**gfa_parser** reads only Segment (`S`), Link (`L`) and Path (`P`) lines and ignores all other GFA 1.0 line types. It
validates only links defined by Link lines and assumes that every adjacency in a Path line has a corresponding Link
line.

The `PathName` field of each GFA 1.0 Path (`P`) line is expected to follow the Cuttlefish-style naming convention. For
example:

```
P	Reference:1_Sequence:test_efc_1.fa
```

See the [test data directory](../test_data) for examples.

Only GFA 1.0 files are currently supported.

The parser produces deterministic output.

## Output

Given an output stem of `STEM`, **gfa_parser** writes the following files and directories.

### Unitigs file (`STEM.unitigs`)

Contains whitespace-separated lines in the following format:

```
UNITIG_ID UNITIG_SEQUENCE
```

Unitig IDs are assigned sequentially and are consistent with the IDs used in the
[path edge-list files](#path-edge-list-files-stem_pathsedges).

### FASTA file (`STEM.fasta`)

A FASTA-formatted pseudo-alignment file that encodes binary unitig-occurrence patterns:

- `c`: unitig present in the reference.
- `a`: unitig absent from the reference.

The file is intended for use as input to **SpydrPick**.

### Paths file (`STEM.paths`)

Contains the path to each SGG `.edges` file, one path per line. The files are stored in `STEM_paths/`.

### Path edge-list files (`STEM_paths/*.edges`)

Each file contains an edge list derived from a GFA 1.0 Path (`P`) line for one reference. These files represent SGG edge
lists for **unitig_distance**.

Each file contains whitespace-separated lines in the following format:

```
UNITIG_FROM_ID UNITIG_TO_ID ORIENTATION OVERLAP
```

Where:
- `UNITIG_FROM_ID` and `UNITIG_TO_ID` are 0-based indices into the `.unitigs` file.
- `ORIENTATION` is one of `FF`, `FR`, `RF` or `RR` (forward-forward, forward-reverse, reverse-forward and
  reverse-reverse).
- `OVERLAP` is the de Bruijn graph overlap (`k - 1`) written as a plain integer for bookkeeping purposes.

Zero-overlap edges are omitted from the output.

## Tests

From the `PANGWES` repository root, run the unit and integration tests with:

```
make -C gfa_parser check
```

Run the end-to-end tests with:

```
make -C gfa_parser check-e2e
```

Test data is available in the [test_data directory](../test_data).

## License

MIT License (see [LICENSE](../LICENSE)).

## Cite

**PAN-GWES/gfa_parser** was developed as part of an academic project. Please cite:

- Kuronen J, Horsfield ST, Pöntinen AK, Mallawaarachchi S, Arredondo-Alonso S, Thorpe H, Gladstone RA, Willems RJL,
  Bentley SD, Croucher NJ, Pensar J, Lees JA, Tonkin-Hill G, Corander J. 2024.
  Pangenome-spanning epistasis and coselection analysis via de Bruijn graphs.
  Genome Research, 34(7):1081–1088.
  https://doi.org/10.1101/gr.278485.123

- https://github.com/jurikuronen/PANGWES
