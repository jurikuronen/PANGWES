# PAN-GWES

**PAN-GWES** is a phenotype- and alignment-free method for discovering coselected and epistatically interacting
genomic variation from bacterial genome assemblies.

**PAN-GWES** uses a colored and compacted de Bruijn graph (cdBG) to represent variation across the input assemblies and
approximate intragenome distances between loci without relying on a single reference genome. Within the graph, unitigs
represent loci, and each color corresponds to the single-genome graph (SGG) of one assembly. For each candidate unitig
pair, the intragenome distance is estimated from shortest-path distances calculated separately in every SGG where both
unitigs are present and connected. These estimates help distinguish short-distance pairs likely to be affected by
linkage disequilibrium.

The full pipeline uses [Cuttlefish](https://github.com/COMBINE-lab/cuttlefish) to construct the cdBG and
[SpydrPick](https://github.com/santeripuranen/SpydrPick) to compute mutual-information scores and produce a list of
high-scoring candidate unitig pairs. **Cuttlefish** and **SpydrPick** are required for the full pipeline but are not
included in this repository.

This repository provides two C++ command-line programs:

1. [gfa_parser](gfa_parser), which converts the GFA 1.0 output from **Cuttlefish** into inputs for **SpydrPick** and
   **unitig_distance**.
2. [unitig_distance](unitig_distance), which calculates shortest-path distance statistics for candidate unitig pairs
   across single-genome graphs.

## Table of contents

- [Installation](#installation)
  - [Conda package](#conda-package)
  - [From source](#from-source)
- [Run the PAN-GWES pipeline](#run-the-pan-gwes-pipeline)
- [Plot the results](#plot-the-results)
- [Automated testing](#automated-testing)
- [Release history](#release-history)
- [Issues and feature requests](#issues-and-feature-requests)
- [License](#license)
- [Cite](#cite)

## Installation

### Conda package

An older version (v0.1.0) of **PAN-GWES** is available as a Conda package, see
[Sudaraka88/PAN-GWES](https://github.com/Sudaraka88/PAN-GWES). The package will soon be updated to v1.0.0.

### From source

**gfa_parser** and **unitig_distance** are designed for Linux and HPC environments, including systems with older Linux
distributions and compilers. Both programs are written in C++11 and have no external dependencies.

Run:

```
git clone https://github.com/jurikuronen/PANGWES
cd PANGWES
make
```

This builds both programs and places the runnable command-line executables in:

- `PANGWES/bin/gfa_parser`
- `PANGWES/bin/unitig_distance`

See the program-specific documentation for additional build and usage instructions:

- [gfa_parser/README.md](gfa_parser/README.md)
- [unitig_distance/README.md](unitig_distance/README.md)

## Run the PAN-GWES pipeline

Instructions for running the **PAN-GWES** pipeline are available in
[Running the PAN-GWES pipeline](examples/README.md#running-the-pan-gwes-pipeline).

## Plot the results

Instructions for drawing GWES plots, mapping unitigs to a reference genome and comparing graph distances with
reference-genome distances are available in [plot_scripts/README.md](plot_scripts/README.md).

## Automated testing

**PAN-GWES** is supported by a comprehensive automated test suite that includes unit, integration, end-to-end and
pipeline tests.

See [test_data/README.md](test_data/README.md) for descriptions of the test datasets and instructions for running the
tests.

## Release history

Notable changes between releases are documented in [CHANGELOG.md](CHANGELOG.md).

## Issues and feature requests

Please open a [GitHub issue](https://github.com/jurikuronen/PANGWES/issues) to report a bug, request a feature or ask a
question.

## License

MIT License (see [LICENSE](LICENSE)).

## Cite

**PAN-GWES** was developed as part of an academic project. Please cite:

- Kuronen, J., Horsfield, S. T., Pöntinen, A. K., Mallawaarachchi, S., Arredondo-Alonso, S., Thorpe, H.,
  Gladstone, R. A., Willems, R. J. L., Bentley, S. D., Croucher, N. J., Pensar, J., Lees, J. A., Tonkin-Hill, G., &
  Corander, J. 2024. Pangenome-spanning epistasis and coselection analysis via de Bruijn graphs. Genome Research,
  34(7):1081-1088. https://doi.org/10.1101/gr.278485.123

- https://github.com/jurikuronen/PANGWES
