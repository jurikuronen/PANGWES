# PAN-GWES/unitig_distance
**unitig_distance** is a command-line program for computing **shortest-path distances between unitig pairs** across
**single-genome graphs (SGGs)**, i.e. the colors of a compacted de Bruijn graph (cdBG).

**unitig_distance** is designed for Linux and HPC environments, including systems with older Linux distributions and
compilers:
- Written in C++11 and developed using the [GNU C++ compiler](https://gcc.gnu.org/).
- No external dependencies.
- Builds with a single `Makefile`.
- Supports multithreaded execution through an internal worker pool.

**unitig_distance** is part of the PAN-GWES pipeline described in *Kuronen et al. (2024)* (see [Cite](#cite)). It is
intended to be used together with:
1. [COMBINE-lab/cuttlefish](https://github.com/COMBINE-lab/cuttlefish), which constructs a GFA-formatted cdBG from
   input genome assemblies.
2. [gfa_parser](../gfa_parser), which converts GFA files into input formats suitable for **unitig_distance**
   and **SpydrPick**.
3. [santeripuranen/SpydrPick](https://github.com/santeripuranen/SpydrPick), which computes mutual-information scores and
   produces a list of high-scoring candidate unitig pairs.

## Table of contents
- [Installation](#installation)
- [Quick start](#quick-start)
- [Command-line options](#command-line-options)
- [Input files](#input-files)
  - [Unitigs file (.unitigs)](#unitigs-file-unitigs)
  - [SGG paths file (.paths)](#sgg-paths-file-paths)
  - [SGG edge files (.edges)](#sgg-edge-files-edges)
  - [Queries file (SpydrPick output)](#queries-file-spydrpick-output)
- [Output](#output)
- [Plotting the results](#plotting-the-results)
- [Tests](#tests)
- [License](#license)
- [Cite](#cite)

## Installation

Run:

```
git clone https://github.com/jurikuronen/PANGWES
cd PANGWES
make -C unitig_distance
```

This creates the executable at `PANGWES/bin/unitig_distance`.

## Quick start

Given input files produced by **gfa_parser** and **SpydrPick**, run:

```
unitig_distance --unitigs-file       cdbg_k61.unitigs                               \
                --k-mer-length       61                                             \
                --sgg-paths-file     cdbg_k61.paths                                 \
                --queries-file       cdbg_k61.*.spydrpick_couplings.1-based.*edges  \
                --queries-one-based                                                 \
                --output-stem        cdbg_k61
```

The output will be written to `cdbg_k61.ud_0_based`. See [Output](#output) for a description of the output format.

## Command-line options

```
Compacted de Bruijn graph (cdBG) options:
    -U,  --unitigs-file PATH
    -k,  --k-mer-length K
    -S,  --sgg-paths-file PATH

Distance query options:
    -Q,  --queries-file PATH
    -1q, --queries-one-based
    -n,  --n-queries N
    -x,  --no-median-distance

Other options:
    -o,  --output-stem STEM
    -1o, --output-one-based
    -t,  --threads N
    -m,  --memory SIZE
    -q,  --quiet
    -h,  --help
    -v,  --version
```

More information is available by running `unitig_distance --help`.

## Input files

All input files are whitespace-separated text files.

### Unitigs file (.unitigs)

Provided with `-U` or `--unitigs-file`.

Contains one unitig per line in the following format:

```
UNITIG_ID UNITIG_SEQUENCE
```

The `UNITIG_ID` field must be consistent with `POS1` and `POS2` in the SGG edge files. **unitig_distance** ignores the
ID field and **assumes** the ordering `0, 1, 2, ...`.

The length of `UNITIG_SEQUENCE` is used to compute the weight of the self-link connecting the left and right sides of
the unitig. The weight is defined as the sequence length minus the k-mer length.

### SGG paths file (.paths)

Provided with `-S` or `--sgg-paths-file`.

Contains the path to each SGG edge-list file, one path per line:

```
/path/to/1.edges
/path/to/2.edges
...
```

Each listed file defines the edges for one SGG (color subgraph).

### SGG edge files (.edges)

Each SGG edge-list file must contain edges in the following format:

```
POS1 POS2 ORIENTATION [OVERLAP]
```

Where:
- `POS1` and `POS2` are **0-based unitig IDs** (indices into the `.unitigs` file).
- `ORIENTATION` is one of `FF`, `FR`, `RF` or `RR` (forward-forward, forward-reverse, reverse-forward and
  reverse-reverse).

SGG edge files may include a fourth de Bruijn graph `OVERLAP` field. Starting with PAN-GWES version 1.0.0,
**gfa_parser** writes the overlap (`k - 1`) as a plain integer for bookkeeping purposes. Older versions wrote
CIGAR-style values such as `30M`. **unitig_distance** accepts either form and otherwise ignores the field. For backward
compatibility, edges with the legacy value `0M` are skipped.

Empty SGG edge files are not accepted and cause **unitig_distance** to report an error.

### Queries file (**SpydrPick** output)

Provided with `-Q` or `--queries-file`.

Must follow **SpydrPick**'s output format:

```
POS1 POS2 GENOME_DISTANCE ARACNE_FLAG SCORE
```

Notes:
- Only `POS1` and `POS2` are used to run distance queries.
- The remaining columns are preserved in the output, except that `GENOME_DISTANCE` is replaced by the computed unitig
  graph distance.
- **SpydrPick**'s output uses 1-based indices by default; use `--queries-one-based` if needed.

## Output

Given an output stem of `STEM`, **unitig_distance** writes the output to:
- `STEM.ud_0_based` by default.
- `STEM.ud_1_based` if `--output-one-based` is set.

The default output stem is `out`.

If the output filename already exists, a unique suffix from `.1` to `.256` is appended to avoid overwriting the existing
file.

The output contains whitespace-separated lines in the following format:

```
POS1 POS2 MEAN_DISTANCE ARACNE MI COUNT SAMPLE_VARIANCE MIN_DISTANCE MAX_DISTANCE MEDIAN_DISTANCE
```

In addition to the fields preserved from **SpydrPick**:

- `MEAN_DISTANCE` is the **mean shortest-path distance for the unitig pair across the SGGs**, rounded to an integer.
- `COUNT` is the number of SGGs in which the unitig pair was present and connected, allowing a distance to be computed.
- `SAMPLE_VARIANCE` is the unbiased sample variance of the shortest-path distances.
- `MIN_DISTANCE` and `MAX_DISTANCE` are the minimum and maximum shortest-path distances.
- `MEDIAN_DISTANCE` is the median shortest-path distance. When `COUNT` is even, the lower of the two middle distances is
  used. It is set to `-1` if disabled with `--no-median-distance`.

A value of `-1` generally indicates that a distance or distance statistic is undefined.

## Plotting the results

**unitig_distance** results can be visualized as a GWES Manhattan plot using the plotting scripts in the
[plot_scripts directory](../plot_scripts).

The directory also provides a script for mapping unitigs to reference-genome positions, allowing the graph distances to
be compared against reference-genome distances.

## Tests

From the `PANGWES` repository root, run the unit and integration tests with:

```
make -C unitig_distance check
```

Run the end-to-end tests with:

```
make -C unitig_distance check-e2e
```

Test data is available in the [test_data directory](../test_data).

## License

MIT License (see [LICENSE](../LICENSE)).

## Cite

**PAN-GWES/unitig_distance** was developed as part of an academic project. Please cite:

- Kuronen J, Horsfield ST, Pöntinen AK, Mallawaarachchi S, Arredondo-Alonso S, Thorpe H, Gladstone RA, Willems RJL,
  Bentley SD, Croucher NJ, Pensar J, Lees JA, Tonkin-Hill G, Corander J.
  2024.
  Pangenome-spanning epistasis and coselection analysis via de Bruijn graphs.
  *Genome Research* **34**(7):1081–1088.
  [https://doi.org/10.1101/gr.278485.123](https://doi.org/10.1101/gr.278485.123)

- https://github.com/jurikuronen/PANGWES
