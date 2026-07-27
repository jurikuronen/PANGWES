# PAN-GWES examples

This directory contains PAN-GWES pipeline instructions, example plots and performance benchmarks.

The pipeline example uses 375 **Enterococcus faecalis** assemblies from the
[EFC collection](../test_data/README.md#efc-dataset). An example dataset containing 337 **Enterococcus faecalis**
assemblies is available
[here](https://uio-my.sharepoint.com/:u:/g/personal/sudarakm_uio_no/ET0J10TDy9VCiIS8ymLFYxYBrN0IqxsE83iJzUl-9_SWpQ?e=YpKoPg).

Tiny test datasets for checking that the programs work correctly are available in the
[test data directory](../test_data).

## Table of contents

- [Running the PAN-GWES pipeline](#running-the-pan-gwes-pipeline)
  - [Example plots](#example-plots)
- [Performance benchmarks](#performance-benchmarks)
  - [v0.1.0–v1.0.0 comparison](#v010v100-comparison-2026-07-27)

## Running the PAN-GWES pipeline

Run the following commands from your working directory, adjusting paths and program options as needed. Set the thread
counts according to the number of threads available on your system, for example as reported by `nproc`.

1. Create a file listing the assembly paths:

```
ls /path/to/assemblies/* > efc_assemblies.txt
```

2. Build a [GFA 1.0](https://github.com/GFA-spec/GFA-spec/blob/master/GFA1.md)-formatted compacted de Bruijn graph
   with [Cuttlefish](https://github.com/COMBINE-lab/cuttlefish), using your choice of k-mer length:

```
# Increase the concurrently open file-handle limit as Cuttlefish produces a large number of temporary files.
ulimit -n 4096

cuttlefish build --list      efc_assemblies.txt  \
                 --format    1                   \
                 --kmer-len  61                  \
                 --threads   16                  \
                 --output    efc_k61             \
                 --work-dir  /tmp
```

3. Parse the [GFA 1.0](https://github.com/GFA-spec/GFA-spec/blob/master/GFA1.md) file into inputs for
   [unitig_distance](../unitig_distance) and [SpydrPick](https://github.com/santeripuranen/SpydrPick) with
   [gfa_parser](../gfa_parser):

```
gfa_parser --gfa-file      efc_k61.gfa1  \
           --gfa-format    1             \
           --k-mer-length  61            \
           --threads       16            \
           --output-stem   efc_k61
```

4. Compute mutual-information scores with [SpydrPick](https://github.com/santeripuranen/SpydrPick) to produce a list of
   high-scoring candidate unitig pairs:

```
SpydrPick --alignmentfile   efc_k61.fasta    \
          --maf-threshold   0.05             \
          --mi-values       50000000         \
          --sample-weights  efc_k61.weights  \
          --threads         16               \
          --verbose
```

If `--sample-weights` is omitted, [SpydrPick](https://github.com/santeripuranen/SpydrPick) attempts to correct for
population structure by assigning weights across samples in the input alignment
([SpydrPick#advanced-usage](https://github.com/santeripuranen/SpydrPick#advanced-usage)). However, the FASTA file
produced by `gfa_parser` is a pseudo-alignment file that encodes binary unitig-occurrence patterns rather than
nucleotide sequences. Supplying appropriate sample weights with `--sample-weights` is therefore recommended.

5. Compute shortest-path distances between the unitig pairs with [unitig_distance](../unitig_distance):

```
unitig_distance --unitigs-file       efc_k61.unitigs                               \
                --k-mer-length       61                                            \
                --sgg-paths-file     efc_k61.paths                                 \
                --queries-file       efc_k61.*.spydrpick_couplings.1-based.*edges  \
                --queries-one-based                                                \
                --threads            16                                            \
                --output-stem        efc_k61
```

Median-distance calculations use a default memory limit of 20 GiB. Use `--memory` to set a limit appropriate for your
system. For a faster mean-only run, disable median-distance calculation with `--no-median-distance`.

6. Draw a GWES Manhattan plot from the **unitig_distance** output:

```
Rscript gwes_plot.r efc_k61.ud_0_based                   \
                    efc_k61_gwes_plot_mean_distance.png  \
                    19                                   \
                    0
```

Set the final argument, `draw_median_distance`, to `1` to draw median distances. The third argument,
`connected_sgg_count_filter`, is set to `19`, or approximately 5% of 375. By default, unitig pairs with relative
standard deviation above 1 are discarded. This threshold can be modified in the script or RStudio.

For a graph-distance versus reference-genome-distance scatterplot, see the [plotting instructions](../plot_scripts).

### Example plots

The following example plots were drawn from the results. The mean-distance plots are shown on the left and the
median-distance plots on the right. Click any preview to open the full-size plot.

**GWES Manhattan plots**

<p align="center">
  <a href="plots/efc_k61_gwes_plot_mean_distance.png">
    <img src="plots/previews/efc_k61_gwes_plot_mean_distance_preview.png"
         alt="GWES Manhattan plot - mean distance"
         width="48%">
  </a>
  <a href="plots/efc_k61_gwes_plot_median_distance.png">
    <img src="plots/previews/efc_k61_gwes_plot_median_distance_preview.png"
         alt="GWES Manhattan plot - median distance"
         width="48%">
  </a>
</p>

**Graph-distance vs reference-genome distance scatter plots**

<p align="center">
  <a href="plots/efc_k61_mean_graph_distance_vs_reference_genome_distance_plot.png">
    <img src="plots/previews/efc_k61_mean_graph_distance_vs_reference_genome_distance_plot_preview.png"
         alt="Mean graph distance vs reference-genome distance"
         width="48%">
  </a>
  <a href="plots/efc_k61_median_graph_distance_vs_reference_genome_distance_plot.png">
    <img src="plots/previews/efc_k61_median_graph_distance_vs_reference_genome_distance_plot_preview.png"
         alt="Median graph distance vs reference-genome distance"
         width="48%">
  </a>
</p>

## Performance benchmarks

### v0.1.0–v1.0.0 comparison (2026-07-27)

These benchmarks compare the runtime and memory usage of PAN-GWES v0.1.0 and v1.0.0.

They were run on Ubuntu 24.04 (6.8.0-124-generic) on a system with an AMD Ryzen 9 5950X CPU (16 cores, 32 threads),
64 GB of DDR4-3600 memory and a Samsung 980 M.2 NVMe SSD. The programs were compiled with g++ 13.3.0.

The benchmarks used 375 **Enterococcus faecalis** genomes from the [EFC collection](../test_data/README.md#efc-dataset)
and 3,069 **Streptococcus pneumoniae** genomes from the [Maela collection](../test_data/README.md#maela-dataset).
The [GFA 1.0](https://github.com/GFA-spec/GFA-spec/blob/master/GFA1.md)-formatted cdBGs were constructed with
[Cuttlefish](https://github.com/COMBINE-lab/cuttlefish) using a k-mer length of 61.

Results for v1.0.0 are shown in **bold**.

#### gfa_parser

| Dataset   |    Version | Threads |     Runtime | Peak memory usage |
| --------- | ---------: | ------: | ----------: | ----------------: |
| Maela     |     v0.1.0 |       1 |     29m 47s |         15.80 GiB |
| **Maela** | **v1.0.0** |   **1** | **27m 30s** |      **1.19 GiB** |
| **Maela** | **v1.0.0** |  **32** | **09m 46s** |      **1.37 GiB** |

Note: **gfa_parser** v0.1.0 supported only single-threaded execution.

Compared with the single-threaded v0.1.0 benchmark, v1.0.0 reduced peak memory usage by 92%. It reduced runtime by 8%
with one thread and by 67% with 32 threads.

#### unitig_distance

[SpydrPick](https://github.com/santeripuranen/SpydrPick) produced 49.4 million unitig-pair queries for the EFC dataset
and 52.6 million for the Maela dataset.

For these datasets, reading the queries and writing the output account for slightly over one minute of the reported
runtime.

##### Mean distances

| Dataset   |    Version | Threads |     Runtime | Peak memory usage |
| --------- | ---------: | ------: | ----------: | ----------------: |
| EFC       |     v0.1.0 |      16 |     21m 52s |          7.93 GiB |
| **EFC**   | **v1.0.0** |  **16** | **03m 42s** |      **6.64 GiB** |
| EFC       |     v0.1.0 |      32 |     16m 46s |          9.10 GiB |
| **EFC**   | **v1.0.0** |  **32** | **03m 23s** |      **7.94 GiB** |
| Maela     |     v0.1.0 |      32 |  1h 50m 49s |         11.65 GiB |
| **Maela** | **v1.0.0** |  **32** | **16m 17s** |      **9.88 GiB** |

Compared with the v0.1.0 benchmarks, v1.0.0 reduced runtime by 83% and 80% in the 16- and 32-thread EFC benchmarks,
respectively, and by 85% in the 32-thread Maela benchmark. Peak memory usage was also lower in the v1.0.0 benchmarks.

##### Median distances

| Dataset   |    Version | Threads | Memory limit |        Runtime |
| --------- | ---------: | ------: | -----------: | -------------: |
| **EFC**   | **v1.0.0** |  **16** |   **20 GiB** |    **04m 30s** |
| **EFC**   | **v1.0.0** |  **32** |   **20 GiB** |    **04m 20s** |
| **EFC**   | **v1.0.0** |  **16** |   **56 GiB** |    **04m 05s** |
| **EFC**   | **v1.0.0** |  **32** |   **56 GiB** |    **03m 50s** |
| **Maela** | **v1.0.0** |  **32** |   **20 GiB** | **2h 22m 10s** |
| **Maela** | **v1.0.0** |  **32** |   **56 GiB** |    **43m 27s** |

Median-distance benchmarks are included for completeness, as median calculation was added in v1.0.0 and no v0.1.0
comparison is available.
