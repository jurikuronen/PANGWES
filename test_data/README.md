# PAN-GWES test data

This directory contains two datasets for automated testing, derived from bacterial genome collections described in the
cited studies below.

The pipeline instructions and example plots in the [examples directory](../examples) use the EFC source collection,
rather than the eight shortened EFC assemblies provided here.

## Datasets

### EFC dataset

This dataset is derived from long-read **Enterococcus faecalis** assemblies from the collection described in:

> Pöntinen AK, Top J, Arredondo-Alonso S, Tonkin-Hill G, Freitas AR, Novais C, Gladstone RA, Pesonen M, Meneses R,
> Pesonen H, Lees JA, Jamrozy D, Bentley SD, Lanza VF, Torres C, Peixe L, Coque TM, Parkhill J, Schürch AC,
> Willems RJL, Corander J.
> 2021.
> Apparent nosocomial adaptation of *Enterococcus faecalis* predates the modern hospital era.
> *Nature Communications* **12**, 1523.
> [https://doi.org/10.1038/s41467-021-21749-5](https://doi.org/10.1038/s41467-021-21749-5)

For automated testing, eight assemblies were selected and shortened so that the tests run quickly. These assemblies were
used to construct a compacted de Bruijn graph (cdBG) with
[Cuttlefish](https://github.com/COMBINE-lab/cuttlefish) version 2.0.0 using `k = 31`. The resulting graph is provided in
[GFA 1.0](https://github.com/GFA-spec/GFA-spec/blob/master/GFA1.md) format.

#### Test files

- `test_efc_k31.gfa1`: [GFA 1.0](https://github.com/GFA-spec/GFA-spec/blob/master/GFA1.md) representation of the cdBG.
- `test_efc_k31.fasta`: FASTA-formatted pseudo-alignment file encoding binary unitig-occurrence patterns, used as input
  to [SpydrPick](https://github.com/santeripuranen/SpydrPick).
- `test_efc_k31.unitigs`: List of maximal unitigs extracted from the cdBG.
- `test_efc_k31.paths`: List of paths to `*.edges` files used to build single-genome graphs (SGGs).
- `test_efc_k31_paths/*.edges`: SGG edge lists consisting of cdBG unitig pairs and their orientations.
- `test_efc_k31.queries`: Example unitig-pair queries and precomputed expected results in the
  [SpydrPick](https://github.com/santeripuranen/SpydrPick) output format.
- `test_efc_assemblies/*.fa`: Shortened assemblies used in the pipeline tests.

### Maela dataset

This dataset is derived from fragmented **Streptococcus pneumoniae** Maela assemblies described in:

> Chewapreecha C, Harris SR, Croucher NJ, Turner C, Marttinen P, Cheng L, Pessia A, Aanensen DM, Mather AE, Page AJ,
> Salter SJ, Harris D, Nosten F, Goldblatt D, Corander J, Parkhill J, Turner P, Bentley SD.
> 2014.
> Dense genomic sampling identifies highways of pneumococcal recombination.
> *Nature Genetics* **46**, 305–309.
> [https://doi.org/10.1038/ng.2895](https://doi.org/10.1038/ng.2895)

As with the EFC dataset, eight assemblies were selected and shortened for automated testing. The dataset is included to
cover assemblies that contain multiple FASTA sequences per assembly file. The Maela test files were prepared in the same
way as the EFC test files.

#### Test files

- `test_maela_k31.gfa1`
- `test_maela_k31.fasta`
- `test_maela_k31.unitigs`
- `test_maela_k31.paths`
- `test_maela_k31_paths/*.edges`
- `test_maela_k31.queries`
- `test_maela_assemblies/*.fa`

### About the test data

The source data are available from the [European Nucleotide Archive](https://www.ebi.ac.uk/ena/browser/home); see the
cited studies.

## Automated testing

### Integration tests

Integration tests (run with `make check-integration` in the repository root) verify that multiple software components
work correctly together using these test datasets. The tests also cover I/O and multithreading.

### End-to-end tests

End-to-end tests (run with `make check-e2e` in the repository root) verify that `gfa_parser` produces the expected
outputs from the test GFA files and that `unitig_distance` produces the expected outputs from these prepared input
files.

### Pipeline tests

The pipeline tests run the PAN-GWES pipeline on the shortened EFC and Maela assemblies. They build a cdBG with
[Cuttlefish](https://github.com/COMBINE-lab/cuttlefish), parse the resulting GFA with `gfa_parser`, create unitig-pair
queries with [SpydrPick](https://github.com/santeripuranen/SpydrPick) and run `unitig_distance` on the prepared outputs.

## Manual runs

### Running gfa_parser on the EFC test GFA

From this directory, run:

```
../bin/gfa_parser --gfa-file test_efc_k31.gfa1 \
                  --gfa-format 1               \
                  --k-mer-length 31            \
                  --output-stem out_test_efc
```

The following output files will be created:

- `out_test_efc.fasta`
- `out_test_efc.unitigs`
- `out_test_efc.paths`
- `out_test_efc_paths/*.edges`

### Running gfa_parser on the Maela test GFA

From this directory, run:

```
../bin/gfa_parser --gfa-file test_maela_k31.gfa1 \
                  --gfa-format 1                 \
                  --k-mer-length 31              \
                  --output-stem out_test_maela
```

The following output files will be created:

- `out_test_maela.fasta`
- `out_test_maela.unitigs`
- `out_test_maela.paths`
- `out_test_maela_paths/*.edges`

### Running unitig_distance on the EFC test data

From this directory, run:

```
../bin/unitig_distance --unitigs-file test_efc_k31.unitigs \
                       --k-mer-length 31                   \
                       --sgg-paths-file test_efc_k31.paths \
                       --queries-file test_efc_k31.queries \
                       --output-stem out_test_efc
```

The output will be written to `out_test_efc.ud_0_based`.

### Running unitig_distance on the Maela test data

From this directory, run:

```
../bin/unitig_distance --unitigs-file test_maela_k31.unitigs \
                       --k-mer-length 31                     \
                       --sgg-paths-file test_maela_k31.paths \
                       --queries-file test_maela_k31.queries \
                       --output-stem out_test_maela
```

The output will be written to `out_test_maela.ud_0_based`.
