# unitig_distance - Various scripts
Useful scripts have been added here.

## R plot 
**Note: This file is outdated, will be updated soon.**

When unitig_distance is used on queries that contained pairwise scores for the unitigs, unitig_distance's output can be visualized as a *GWES Manhattan plot*. gwes_plot.r is an easy to use R script to create such a plot. You can fill in file paths and plotting parameters yourself or call it from the command line as follows:
```
Rscript gwes_plot.r <ud_output> <plot_name> <n_queries> <score_name> \
                    <counts_file> <count_criterion> <plot_title> \
                    <ld_distance> <outlier_threshold> <extreme_outlier_threshold> \
                    <ld_distance_alt> <outlier_threshold_alt> <extreme_outlier_threshold_alt>
```
The arguments should be given in the exact order as:
- `<ud_output>`: unitig_distance's output file.
- `<plot_name>`: name for the plot output, ending in `.png`.
- `<n_queries>`: number of queries to read from input files (<= 0 value reads all queries).
- `<score_name>`: name of the score used, e.g. "Mutual information"
- `<counts_file>`: unitig_distance's sgg counts file if outputting a plot for sgg distances/scores, set it as "" otherwise.
- `<count_criterion>`: unitig_distance's sgg counts filter criterion, set as 0 if unused.
- `<plot_title>`: title for the plot.
- `<ld_distance>`: linkage disequilibrium distance, set as 0 if unused.
- `<outlier_threshold>`: outlier threshold, set as 0 if unused.
- `<extreme_outlier_threshold>`: extreme outlier threshold, set as 0 if unused.
- `<ld_distance_alt>`: alternative linkage disequilibrium distance, set as 0 if unused.
- `<outlier_threshold_alt>`: alternative outlier threshold, set as 0 if unused.
- `<extreme_outlier_threshold_alt>`: alternative extreme outlier threshold, set as 0 if unused.

The last 3 arguments can be used if you want to plot two outlier estimation statistics in the same plot.

As an example, the following reads unitig_distance's output called `<output_stem>.ud_sgg_mean_0_based` and creates a plot called `<output_stem>_sgg_mean.png` after reading all queries from the input file. The score used is called `"Mutual information"`. No sgg counts were used in filtering or outlier threshold estimation, therefore the counts file and criterion are set to unused values. The plot's title will be `<output_stem>_sgg_mean (no sgg counts filtering)`. Linkage disequilibrium distance and the outlier thresholds are obtained from unitig_distance's outlier stats file, but they can also be input manually.
```
Rscript gwes_plot.r <output_stem>.ud_sgg_mean_0_based <output_stem>_sgg_mean.png 0 "Mutual information" \
                    "" 0 "<output_stem>_sgg_mean (no sgg counts filtering)" \
                    $(echo $(cut -f1-3 -d" " <output_stem>.ud_sgg_mean_outlier_stats))
```
