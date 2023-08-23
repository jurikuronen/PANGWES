# unitig_distance
unitig_distance is a command line program that calculates shortest path distances in a compacted de Bruijn graph which has been constructed from genome sequences. Since such graphs tend to be very large and do not admit straightforward decompositions, unitig_distance aims to speed up the calculations by smart arrangement of the graph search jobs and through parallel operation.

unitig_distance can be used together with programs such as [SpydrPick](https://github.com/santeripuranen/SpydrPick) that calculate pairwise scores for the unitigs, but cannot calculate their distances in the underlying compacted de Bruijn graph. For such use cases, see [Input files - Distance queries file](#distance-queries-file), [Usage - Calculating distances in a compacted de Bruijn graph](#calculating-distances-in-a-compacted-de-bruijn-graph) and [Usage - Determining outliers from supplied scores](#determining-outliers-from-supplied-scores).

See also the project [gfa1_parser](https://github.com/jurikuronen/gfa1_parser) which can be used to create suitable input files for unitig_distance from a [Graphical Fragment Assembly (GFA) file](https://github.com/GFA-spec/GFA-spec/blob/master/GFA1.md).

## Table of contents

- [Installation from source with a C++11 compliant compiler](#installation-from-source-with-a-c11-compliant-compiler)
- [Input files](#input-files)
  - [Graph files](#graph-files)
    - [General graph](#general-graph)
    - [Compacted de Bruijn graph](#compacted-de-bruijn-graph)
    - [Single genome graphs](#single-genome-graphs)
  - [Distance queries file](#distance-queries-file)
- [Usage](#usage)
  - [List of available options](#list-of-available-options)
  - [Calculating distances in a general graph](#calculating-distances-in-a-general-graph)
  - [Calculating distances in a compacted de Bruijn graph](#calculating-distances-in-a-compacted-de-bruijn-graph)
  - [Calculating mean distances in single genome graphs](#calculating-mean-distances-in-single-genome-graphs)
  - [Output format](#output-format)
  - [Determining outliers from supplied scores](#determining-outliers-from-supplied-scores)

## Installation from source with a C++11 compliant compiler
```
git clone https://github.com/jurikuronen/unitig_distance
cd unitig_distance
make
```
This will create an executable named `unitig_distance` inside the `bin` directory.

## Input files
All input files for unitig_distance should be text files with **space-separated values** whose paths and any additional options are provided with command line arguments. This section details how the input files should be prepared and provided.

### Graph files
#### General Graph
The simplest graph input file for unitig_distance is an edges file (`-E [ --edges-file ] arg`) where lines should have the format
```
v w (weight)
```
for vertices `v` and `w` connected by an edge with weight `weight`. The `weight` column is optional and defaults to the value 1.0. The vertices must be sequentially mapped starting from 0 or 1 (`-1g [ --graphs-one-based ]`).

#### Compacted de Bruijn Graph
To construct a compacted de Bruijn graph, unitig_distance needs to know the value of the k-mer length `k` (`-k [ --k-mer-length ] arg`) and requires a separate unitigs file (`-U [ --unitigs-file ] arg`) where each line has the format
```
id sequence
```
where `sequence` is the unitig's sequence. The unitigs should be in the correct order with respect to the vertex mapping. The column `id` is not used by unitig_distance, but is required to retain compatibility with other programs. The unitig `sequence` together with the k-mer length `k` will be used to determine self-edge weights.

The lines in the edges file (`-E [ --edges-file ] arg`) should have the extended format
```
v w edge_type (overlap)
```
where `v` and `w` correspond to distinct unitigs (according to the order in the unitigs file) which are connected according to the `edge_type` (FF, RR, FR or RF, indicating the overlap type of the forward/reverse complement sequences). The `overlap` column is optional and mostly used to distinguish between `k-1`-overlapping edges (default) and `0`-overlapping edges. Edges of the latter type are skipped in unitig_distance, since they often correspond to read errors in the genome sequences.

#### Single genome graphs
After providing the necessary files to construct a [compacted de Bruijn graph](#compacted-de-bruijn-graph), unitig_distance can also construct all the individual *single genome graphs* that compose the full graph. Each single genome graph requires a similar edges file as the full compacted de Bruijn graph. All such edge file paths should be collected in a single genome graph paths file (`-S [ --sgg-paths-file ] arg`) with one single genome graph edges file path per line. In the distance calculations, unitig_distance will report the mean distance across the single genome graphs. Distance calculation can also be restricted to the single genome graphs only (`-r [ --run-sggs-only]`).

### Distance queries file
The queries file (`-Q [ --queries-file ] arg`) may use one of the six input line formats below:
```
format_id       input_line_format                   output_line_format
0               v w                                 v w distance (count)
1               v w score                           v w distance score (count)
2               v w distance score                  v w distance score (count)
3               v w distance flag score             v w distance flag score (count)
4               v w distance score count            v w distance score (count)
5               v w distance flag score count       v w distance flag score (count)
```
Each line is a distance query for vertices `v` and `w` and format 0 is the simplest input type. For each vertex pair, unitig_distance will calculate their distance and optionally, in the case of single genome graphs, the *count* of single genome graphs where the query's vertex pair is connected.

Formats 1 to 5 are meant for advanced inputs that are the output of another program such as [SpydrPick](https://github.com/santeripuranen/SpydrPick) that has calculated the top pairwise scores for a set of unitig pairs. Then the input file may already contain a distance column (whose values will be corrected and replaced by unitig_distance), a flag column and a score column. The flag column is stored but ignored, but the score column may be used in outlier determination. All columns will be retained in the output files, but pay attention to the output line format.

Formats 4 to 5 are typically used when unitig_distance's output is re-used as input for the outlier tools mode (see [Usage - Determining outliers from supplied scores](#determining-outliers-from-supplied-scores)).

unitig_distance will try to automatically determine the queries format. It may also be specified manually with the `-q [ --queries-format ] arg` option, where `arg` should take one of the `format_id`s.

If the vertices are 1-based, please supply the option (`-1q [ --queries-one-based ]`). **Note: the distance queries file can use 1-based numbering for the vertices even if the graph files do not.**

Restricting the number of queries to be read from the queries file can be done with `-n [ --n-queries ] arg (=inf)`.

## Usage
This section contains examples of how to use unitig_distance. 

### List of available options
This list is available with the command line argument `-h [ --help ]`.
```
Graph edges:                                  
  -E  [ --edges-file ] arg                    Path to file containing graph edges.
  -1g [ --graphs-one-based ]                  Graph files use one-based numbering.
                                              
CDBG operating mode:                          
  -U  [ --unitigs-file ] arg                  Path to file containing unitigs.
  -k  [ --k-mer-length ] arg                  k-mer length.
                                              
CDBG and/or SGGS operating mode:              
  -S  [ --sgg-paths-file ] arg                Path to file containing paths to single genome graph edge files.
  -r  [ --run-sggs-only ]                     Calculate distances only in the single genome graphs.
                                              
Distance queries:                             
  -Q  [ --queries-file ] arg                  Path to queries file.
  -1q [ --queries-one-based ]                 Queries file uses one-based numbering.
  -n  [ --n-queries ] arg (=inf)              Number of queries to read from the queries file.
  -q  [ --queries-format ] arg (-1)           Set queries format manually (0..5).
  -d  [ --max-distance ] arg (=inf)           Maximum allowed graph distance (for constraining the searches).
                                              
Tools for determining outliers:               
  -x  [ --output-outliers ]                   Output a list of outliers and outlier statistics.
  -Cc [ --sgg-count-threshold ] arg (=10)     Filter low count single genome graph distances.
  -l  [ --ld-distance ] arg (=-1)             Linkage disequilibrium distance (automatically determined if negative).
  -lm [ --ld-distance-min ] arg (=1000)       Minimum ld distance for automatic ld distance determination.
  -ls [ --ld-distance-score ] arg (=0.8)      Score difference threshold for automatic ld distance determination.
  -ln [ --ld-distance-nth-score ] arg (=10)   Use nth max score for automatic ld distance determination.
  -ot [ --outlier-threshold ] arg             Set outlier threshold to a custom value.
                                              
Other arguments.                              
  -o  [ --output-stem ] arg (=out)            Path for output files (without extension).
  -1o [ --output-one-based ]                  Output files use one-based numbering.
  -1  [ --all-one-based ]                     Use one-based numbering for everything.
  -t  [ --threads ] arg (=1)                  Number of threads.
  -v  [ --verbose ]                           Be verbose.
  -h  [ --help ]                              Print this list.
```

### Calculating distances in a general graph
The following constructs a general graph from an edges file (`-E <path_to_edges_file>`) and calculates distances between 100 (`-n 100`) vertex pairs read from the queries file (`-Q <path_to_queries_file>`) in parallel using 4 threads (`-t 4`). The output will be written to `<output_stem>.ud_0_based`. Verbose-mode (`-v`) is set and a log will be written both to the terminal and to the file `<output_stem>.ud_log`.
```
./bin/unitig_distance -E <path_to_edges_file> \
                      -Q <path_to_queries_file> -n 100 \
                      -o <output_stem> -t 4 -v | tee <output_stem>.ud_log
```

### Calculating distances in a compacted de Bruijn graph
The following constructs a compacted de Bruijn graph from an edges file (`-E <path_to_edges_file>`) and a unitigs file (`-U <path_to_unitigs_file>`) with k-mer length 61 (`-k 61`) and calculates distances between all vertex pairs read from the queries file (`-Q <path_to_queries_file>`) in parallel using 16 threads (`-t 16`). The queries use one-based numbering (`-1q`). Verbose-mode (`-v`) is set and a log will be written both to the terminal and to the file `<output_stem>.ud_log`.
```
./bin/unitig_distance -E <path_to_edges_file> \
                      -U <path_to_unitigs_file> -k 61 \
                      -Q <path_to_queries_file> -1q \
                      -o <output_stem> -t 16 -v | tee <output_stem>.ud_log
```
The output will be written to `<output_stem>.ud_0_based`.

### Calculating mean distances in single genome graphs
Following from the above section ([Calculating distances in compacted de Bruijn graphs](#calculating-distances-in-compacted-de-bruijn-graphs)), the following constructs all the single genome graphs in the single genome graph paths file (`-S [ --sgg-paths-file ] arg`) and calculates distances in these graphs only (`-r [ --run-sggs-only ]`).
```
./bin/unitig_distance -E <path_to_edges_file> \
                      -U <path_to_unitigs_file> -k 61 \
                      -Q <path_to_queries_file> -1q \
                      -S <path_to_sggs_file> -r \
                      -o <output_stem> -t 16 -v | tee <output_stem>.ud_log
```
The output will be written to `<output_stem>.ud_sgg_0_based` in which the distance column contains the mean distance across the single genome graphs. An additional count column is added, which is the count of single genome graphs where the query's vertex pair is connected.

### Output format
unitig_distance's output follows the following line format:
```
v w distance (flag) (score) (count) (M2) (min_distance) (max_distance)
```
The `flag` and `score` columns are written if the original queries file contained them. The `count` column is written if the `distance` column refers to single genome graph mean distances (see [Usage - Calculating mean distances in single genome graphs](#calculating-mean-distances-in-single-genome-graphs)). In such cases, the columns `M2`, `min_distance` and `max_distance` will be provided as well. The `min_distance` and `max_distance` are the minimum and maximum distance in the single genome graphs for a given unitig pair and `M2` is the sum of squares of differences from the current mean from which the unbiased sample variance can be calculated as: $s^2_n = \frac{M_{2, n}}{n - 1}$ (see Welford's online algorithm for calculating the variance).

See [Input files - Distance queries file](#distance-queries-file) for an informative table.


### Determining outliers from supplied scores
When the queries contain pairwise scores for the unitigs, for example when the output of a program such as [SpydrPick](https://github.com/santeripuranen/SpydrPick) is provided as the distance queries file (see [Input files - Distance queries file](#distance-queries-file)), unitig_distance can automatically determine outliers and outlier stats for all graphs being worked on with the command line argument `-x [ --output-outliers ]`. When working with single genome graphs, vertex pairs in the queries that are connected in less than `sgg_count_threshold` (default: 10) single genome graphs will also be filtered out. This option can be modified with the command line argument `-Cc [ --sgg-count-threshold ] arg (=10)` with a value of 0 completely disabling it.

The outlier threshold estimation is currently experimental, intended to aid with post-processing the results and should not be depended on fully. The estimation only works well with sufficiently large inputs (ideally, the queries should cover all unitigs of interest). It is based on Tukey's outlier test, which assesses how extreme a score is compared to a global background distribution. As background distribution, an extreme value distribution is fitted from maximum unitig scores beyond a distance-based cutoff in order to deal with linkage disequilibrium. The linkage disequilibrium distance cutoff can be specified with the command line argument `-l [ --ld-distance ] arg (=-1)`, but unitig_distance can also attempt to determine it automatically if left at its default negative value (recommended). If the linkage disequilibrium distance cutoff has been set, it's also possible to set the outlier threshold to a custom value with the command line argument `-ot [ --outlier-threshold ] arg`, skipping Tukey's outlier test.

unitig_distance can run in outlier tools mode for already calculated distances by supplying the distances with the command line argument `-Q [ --queries-file ] arg`. In this mode, no graph files should be supplied, otherwise unitig_distance will simply recalculate the distances using the queries file as normal queries input. An example is provided below for how to use this mode.

**Outlier tools example.** Assume the distances have been calculated from a queries file that contained pairwise scores according to the example at [Calculating distances in single genome graphs](#calculating-distances-in-single-genome-graphs). The following runs unitig_distance in outlier tools mode (`-x`) using the single genome graph mean distances as the queries file (`-Q <output_stem>.ud_sgg_0_based`). In this file, the counts column exists as well and the count threshold is set to 50 (`-Cc 50`). Unitig_distance will determine the linkage disequilibrium distance cutoff automatically and calculates the outlier threshold and extreme outlier threshold values, which will be written to the outlier stats file. Verbose-mode (`-v`) is set and a log will be written to the terminal.
```
./bin/unitig_distance -x -Q <output_stem>.ud_sgg_0_based -Cc 50 -o <output_stem_sgg> -v
```
The linkage disequilibrium distance cutoff, outlier threshold, extreme outlier threshold and `sgg_count_threshold` values will be written to `<output_stem_sgg>.ud_outlier_stats`. Then, these values will be used to collect the queries which will be written to `<output_stem_sgg>.ud_outliers_0_based`. Notice the updated output name so that the file `<output_stem>.ud_outliers_0_based` won't be overwritten.

**Plotting the results.** It is recommended to check how the results look like graphically by visualizing the results with the provided R script at [unitig_distance/scripts/](scripts). Afterwards, it is easy to rerun unitig_distance in outlier tools mode with updated parameter values if necessary.

