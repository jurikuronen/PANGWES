# Changelog

Notable changes to this project will be documented in this file. Experimental versions prior to 0.1.0 are not tracked.

## v1.0.0 (2026-07-27)

### General

- Added CHANGELOG.md.
- Unified the build system for **unitig_distance** and **gfa_parser** using shared Makefile rules, targets and flags.
- Added unit, integration and end-to-end tests.
- Added development, debug and sanitizer build and test targets.
- Added PAN-GWES version management via the [VERSION](VERSION) file and Git revision.
- Added static analysis via [clang-tidy](https://clang.llvm.org/extra/clang-tidy),
  [ShellCheck](https://www.shellcheck.net), [perlcritic](https://metacpan.org/dist/Perl-Critic/view/bin/perlcritic)
  and [lintr](https://lintr.r-lib.org).
- Added test datasets in [test_data](test_data).
- Added CI via GitHub Actions.
- Renamed **gfa1_parser** to **gfa_parser**.
- Added exception handling.
- Added failure cleanup for output files and directories.
- Improved code documentation.
- Improved error logging.
- Added the [examples directory](examples) with pipeline instructions, example plots and performance benchmarks.
- Updated [README.md](README.md).

### common

- Added a new shared **common** project.
- Moved reusable code from **unitig_distance** into **common** for use by **gfa_parser**.
- Added a lightweight test harness.
- Added a worker pool component to replace manual thread management. This reduced thread downtime and significantly
  improved runtime (see
  [Performance benchmarks: v0.1.0–v1.0.0 comparison](examples/README.md#v010v100-comparison-2026-07-27)).
- Added output file and directory name collision protection.

### gfa_parser

- Added multithreaded operation.
- Added command-line option handling consistent with **unitig_distance**.
- Added stricter validation of the [GFA 1.0](https://github.com/GFA-spec/GFA-spec/blob/master/GFA1.md) format.
- Added conformance checking for CIGAR strings used by the Sequence Alignment/Map (SAM) format.
- Added explicit sequence overlap validation.
- Changed **gfa_parser** to read GFA 1.0 files in two passes to support deterministic multithreaded operation and
  overlap validation.
- Reduced memory use on large GFA files by an order of magnitude (see
  [Performance benchmarks: v0.1.0–v1.0.0 comparison](examples/README.md#v010v100-comparison-2026-07-27)).
- No longer writes zero-overlap links to `.edges` files. The optional overlap column is still written as a plain number,
  mostly for bookkeeping.
- No longer outputs the `.edges` file corresponding to the full compacted de Bruijn graph (cdBG) because
  **unitig_distance** was updated to operate on single-genome graphs only (i.e. the colors of the cdBG).
- No longer outputs the `.counts` files containing unitig occurrence counts for each reference genome because they were
  not used by **unitig_distance**.
- Updated [gfa_parser/README.md](gfa_parser/README.md).

### unitig_distance

- Changed **unitig_distance** to output the unbiased sample variance instead of the sum of squares of differences from
  the mean (`M2`) (see
  [Welford's algorithm](https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance#Welford's_online_algorithm)).
- Added calculation and output of median shortest-path distances, enabled by default.
- Added memory-aware query batching for median-distance calculation.
- Added the `-m [ --memory ]` program option to configure the memory limit for median-distance calculation
  (default: `20G`). The limit determines the maximum query batch size.
- Added the `-x [ --no-median-distance ]` program option to disable median-distance calculation. When disabled,
  `median_distance` is set to `-1` in the output. This reassigns the `-x` short option previously used by the now-removed
  `--output-outliers` option.
- Added the `-q [ --quiet ]` program option to suppress non-error messages and removed the `-v [ --verbose ]` option;
  **unitig_distance** is now verbose by default. This reassigns the `-q` short option previously used by the now-removed
  `--queries-format` option.
- Added the `-v [ --version ]` program option to exit after printing version information. This reassigns the `-v` short
  option previously used by the now-removed `--verbose` option.
- Removed the outlier-analysis tools because they were difficult to automate reliably and more suitable tools are
  available for this task. As a result, the following related options were removed:
    - `-x [ --output-outliers ]`
    - `-Cc [ --sgg-count-threshold ]`
    - `-l [ --ld-distance ]`
    - `-lm [ --ld-distance-min ]`
    - `-ls [ --ld-distance-score ]`
    - `-ln [ --ld-distance-nth-score ]`
    - `-ot [ --outlier-threshold ]`
- Removed general graph and cdBG operating modes. As a result, the following options were removed:
    - `-E [ --edges-file ]`
    - `-r [ --run-sggs-only ]`
- Removed the 1-based indexing option for graphs (input graph files are 0-based as provided by **gfa_parser**) and the
  all-one-based option:
    - `-1g [ --graphs-one-based ]`
    - `-1 [ --all-one-based ]`
- Removed the `-d [ --max-distance ]` option as it had negligible impact on performance.
- Dropped support for non-SpydrPick query formats. As a result, the `-q [ --queries-format ]` option was removed.
- Updated [unitig_distance/README.md](unitig_distance/README.md).

### Scripts

- Renamed the `scripts` directory to `plot_scripts`.
- Added `plot_scripts/map_unitigs_to_reference.pl` for mapping **unitig_distance** results to a reference genome with
  BWA-MEM.
- Added `plot_scripts/graph_vs_reference_distance_scatter_plot.r` for comparing graph distances against reference-genome
  distances.
- Updated `plot_scripts/gwes_plot.r` to match **unitig_distance** output and added a relative standard deviation filter.
- Updated [plot_scripts/README.md](plot_scripts/README.md).

## v0.3.0\_alpha (2024-07-24)

> Historical entry. The corresponding version is not available in this repository.

- Updated plotting script for improved performance using dependencies.

## v0.2.0 (2024-06-20)

> Historical entry. The corresponding version is not available in this repository.

- Minor bug fix for compatibility with C++ compilation/bioconda release.

## v0.1.0 (2023-03-05)

- First tracked release prepared for paper publication:

> Kuronen J, Horsfield ST, Pöntinen AK, Mallawaarachchi S, Arredondo-Alonso S, Thorpe H, Gladstone RA, Willems RJL,
> Bentley SD, Croucher NJ, Pensar J, Lees JA, Tonkin-Hill G, Corander J.
> 2024.
> Pangenome-spanning epistasis and coselection analysis via de Bruijn graphs.
> *Genome Research* **34**(7):1081–1088.
> [https://doi.org/10.1101/gr.278485.123](https://doi.org/10.1101/gr.278485.123)

- Included **gfa1_parser**, which converts [Cuttlefish](https://github.com/COMBINE-lab/cuttlefish)-provided GFA 1.0 files
  into inputs for [SpydrPick](https://github.com/santeripuranen/SpydrPick) and **unitig_distance**.
- Included **unitig_distance**, which calculates shortest-path distance statistics (mean, count,
[M2](https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance#Welford's_online_algorithm), minimum, maximum)
  for large collections of queries in general graphs, cdBGs and SGGs using query batching, path compression and parallel
  graph searches.
- Included experimental tools for score-based outlier analysis with automatic linkage-disequilibrium cutoff estimation,
  Tukey outlier thresholds, SGG count filtering and support for analysing previously calculated distances.
- Included an R script for drawing a GWES Manhattan plot from **unitig_distance** output.
