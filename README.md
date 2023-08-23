# PANGWES
This repository contains programs and tools for performing pangenome-spanning epistasis and co-selection analysis via de Bruijn graphs.

Two open-source C++ programs are implemented here:
1. [PANGWES/gfa1_parser](gfa1_parser)
2. [PANGWES/unitig_distance](unitig_distance)

These programs will be merged together into a single pangwes program in the future.

Additionally, the [PANGWES/scripts](scripts) directory contains a script for drawing a GWES Manhattan plot from the results.

PANGWES also relies on two external programs:
1. [COMBINE-lab/cuttlefish](https://github.com/COMBINE-lab/cuttlefish) for building a compacted de Bruijn graph from references.
2. [santeripuranen/SpydrPick](https://github.com/santeripuranen/SpydrPick) for calculating mutual information scores to obtain the list of top candidate unitig pairs.

## Installation
Install
- [PANGWES/gfa1_parser](gfa1_parser)
- [PANGWES/unitig_distance](unitig_distance)
- [COMBINE-lab/cuttlefish](https://github.com/COMBINE-lab/cuttlefish)
- [santeripuranen/SpydrPick](https://github.com/santeripuranen/SpydrPick)

according to the instructions in the respective repositories.

## Example
The following runs the pipeline for a list of input genome assembly files `assemblies_list.txt` with k-mer length set to 61.

```
# Construct a colored compacted de Bruijn graph from maximal unitigs.
./cuttlefish build --list      /path/to/assemblies_list.txt \
                   --kmer-len  61                           \
                   --output    cdbg_k61                     \
                   --work-dir  /tmp                         \
                   --threads   8

# Parse the resulting Graphical Fragment Assembly (GFA) formatted file.
./gfa1_parser cdbg_k61.gfa1 cdbg_k61

# Calculate mutual information scores and obtain the list of top candidate unitig pairs.
./SpydrPick --alignmentfile  cdbg_k61.fasta \
            --maf-threshold  0.05           \
            --mi-values      50000000       \
            --threads        8              \
            --verbose

# Calculate the graph distances between unitigs in the single genome subgraphs.
unitig_distance --unitigs-file    cdbg_k61.unitigs                \
                --edges-file      cdbg_k61.edges                  \
                --k-mer-length    61                              \
                --sgg-paths-file  cdbg_k61.paths                  \
                --queries-file    cdbg.*spydrpick_couplings*edges \
                --threads         8                               \
                --queries-one-based                               \
                --run-sggs-only                                   \
                --output-stem cdbg                                \
                --verbose

# Draw a GWES Manhattan plot to visualize the results.
Rscript gwes_plot.r ...
```