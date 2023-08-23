# gfa1_parser
This tool parses [GFA1](https://github.com/GFA-spec/GFA-spec/blob/master/GFA1.md) files produced by [Cuttlefish](https://github.com/COMBINE-lab/cuttlefish) that encode compacted de Bruijn graphs constructed from genome reference(s). It creates input files suitable for use with [SpydrPick](https://github.com/santeripuranen/SpydrPick) and [unitig_distance](https://github.com/jurikuronen/unitig_distance).

## Installation from source with a C++11 compliant compiler
```
git clone https://github.com/jurikuronen/gfa1_parser
cd gfa1_parser
make
```
This will create an executable named `gfa1_parser` inside the `bin` directory.

## Usage
`gfa1_parser` is called with two command line arguments. The first should be a path to a [GFA1](https://github.com/GFA-spec/GFA-spec/blob/master/GFA1.md) file and the second a (suffixless) name that output files should take. As an example, calling
```
./bin/gfa1_parser cdbg.gfa1 cdbg
```
takes as input a [GFA1](https://github.com/GFA-spec/GFA-spec/blob/master/GFA1.md) file called `cdbg.gfa1` and creates the following files as output:
- `cdbg.edges`
- `cdbg.unitigs`
- `cdbg.fasta`
- `cdbg.counts`
- `cdbg_paths/*.edges`
- `cdbg_paths/*.counts`
- `cdbg.paths`

Please see the next section for more information.

## Output files
The fasta file is to be used with [SpydrPick](https://github.com/santeripuranen/SpydrPick) and the remaining output files with [unitig_distance](https://github.com/jurikuronen/unitig_distance). 

#### `output.edges` and `output.unitigs`
The edges and unitigs files are cleanly and compactly remapped lists of unitigs parsed from [Segment lines](https://github.com/GFA-spec/GFA-spec/blob/master/GFA1.md#s-segment-line) and [Link lines](https://github.com/GFA-spec/GFA-spec/blob/master/GFA1.md#l-link-line).

#### `output.fasta`
The fasta file works as an input alignment file for [SpydrPick](https://github.com/santeripuranen/SpydrPick). It contains each genome reference as a fasta sequence whose sequence encodes the binary occurrence pattern of unitigs. 

Note: if you have your own sample weights to correct for population structure, you should confirm that they align with the references in the fasta file (see [SpydrPick#advanced-usage](https://github.com/santeripuranen/SpydrPick#advanced-usage) for more information about SpydrPick's weighting scheme). An easy way to check the order is to run `grep ">" output.fasta` in terminal.

#### `output.counts`
The counts file counts the maximum number of times a unitig occurred in some genome reference, which can be used to filter for repetitive elements.

#### `output_paths/*.edges` and `output_paths/*.counts`
These are edges and counts files for single genome references parsed from [Path lines](https://github.com/GFA-spec/GFA-spec/blob/master/GFA1.md#p-path-line).

#### `output.paths`
The paths file is a listing of the edge files in the paths folder.

## Plans for the future
A multi-threaded version will be implemented in the future.

