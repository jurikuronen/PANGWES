# ----------------------------------------------------------------------
# Genome-wide epistasis studies (GWES) Manhattan plot script.
# Copyright (c) 2018-2022 Juri Kuronen
# ----------------------------------------------------------------------

# ----------------------------------------------------------------------
# File paths.
# ----------------------------------------------------------------------

input_full_filepath <- ""
output_full_filepath <- "gwes_plot.png" 
counts_full_filepath <- ""
count_criterion <- 0

# ----------------------------------------------------------------------
# Plotting parameters
# ----------------------------------------------------------------------

# Plot sizes.
plot_width <- 1920
plot_height <- 1080
plot_pointsize <- 16

# Plot title.
plot_title <- ""

# Number of edges to draw (0 - draw all).
n_queries <- 0

# Score name.
score_name <- "Mutual information"

# Linkage disequilibrium distance (0 - not drawn).
ld_dist <- 0

# Estimated outlier thresholds (0 - not drawn).
outlier_threshold <- 0
extreme_outlier_threshold <- 0

# Alternative linkage disequilibrium distance (0 - not drawn).
ld_dist_alt <- 0

# Alternative outlier thresholds (0 - not drawn).
outlier_threshold_alt <- 0
extreme_outlier_threshold_alt <- 0

# Colors.
color_direct <- rgb(0, 115, 190, maxColorValue = 255)
color_indirect <- rgb(192, 192, 192, maxColorValue = 255)

color_ld <- "red"
color_outlier <- "red"
color_extreme_outlier <- "red"

color_ld_alt <- "hotpink1"
color_outlier_alt <- "hotpink1"
color_extreme_outlier_alt <- "hotpink1"

# Various.
plot_symbol <- 19 # 19 - solid circle.
cex_direct <- 0.2
cex_indirect <- 0.1
cex_legend <- 1.2

# Disable scientific notation
options(scipen=999)

# ----------------------------------------------------------------------
# Read arguments.
# ----------------------------------------------------------------------

args <- commandArgs(trailingOnly = TRUE)

if (length(args) >= 1) { input_full_filepath <- args[1] }
if (length(args) >= 2) { output_full_filepath <- args[2] }
if (length(args) >= 3) { n_queries <- as.numeric(args[3]) }
if (length(args) >= 4) { score_name <- args[4] }
if (length(args) >= 5) { counts_full_filepath <- args[5] }
if (length(args) >= 6) { count_criterion <- as.numeric(args[6]) }
if (length(args) >= 7) { plot_title <- args[7] }
if (length(args) >= 8) { ld_dist <- as.numeric(args[8]) }
if (length(args) >= 9) { outlier_threshold <- as.numeric(args[9]) }
if (length(args) >= 10) { extreme_outlier_threshold <- as.numeric(args[10]) }
if (length(args) >= 11) { ld_dist_alt <- as.numeric(args[11]) }
if (length(args) >= 12) { outlier_threshold_alt <- as.numeric(args[12]) }
if (length(args) >= 13) { extreme_outlier_threshold_alt <- as.numeric(args[13]) }

if (length(args) >= 1) {
    cat("Read the following command line arguments:\n")
    arg_names <- c("input_full_filepath", "output_full_filepath", "n_queries", "score_name",
                   "counts_full_filepath", "count_criterion", "plot_title",
                   "ld_dist", "outlier_threshold", "extreme_outlier_threshold",
                   "ld_dist_alt", "outlier_threshold_alt", "extreme_outlier_threshold_alt")
    for (i in 1:length(args)) {
        cat(paste0(arg_names[i], "=", args[i], "\n"))
    }
}

# ----------------------------------------------------------------------
# Read input.
# ----------------------------------------------------------------------

time_reading_start  <- proc.time()

if (n_queries == 0) { n_queries = -1 }

input <- read.csv(input_full_filepath, header = FALSE, sep = " ", nrows = n_queries) # May take a few minutes.
if (n_queries <= 0 || n_queries > dim(input)[1]) { n_queries <- dim(input)[1] }

if (count_criterion > 0) {
  counts <- read.csv(counts_full_filepath, header = FALSE, sep = " ", nrows = n_queries) # May take a few minutes.
  if (dim(counts)[1] < n_queries) {
    # Could not read enough counts.
    count_criterion <- 0
  }
}
time_reading_end <- proc.time()
time_reading <- (time_reading_end - time_reading_start)[[3]]
cat(paste0("Time reading=", time_reading, "\n"))

if (count_criterion > 0) {
  n_K_filtered <- floor(sum(counts[, 3] < count_criterion, na.rm = TRUE) / 1000)
  input = input[counts[, 3] >= count_criterion, ]
}

input_max <- -1;

disconnected <- input[input[, 3] == input_max, ]
connected <- input[input[, 3] != input_max, ]

n_K_disconnected <- 0
n_K_connected <- 0
if (nrow(disconnected) > 0) { n_K_disconnected <- floor(nrow(disconnected) / 1000) }
if (nrow(connected) > 0) { n_K_connected <- floor(nrow(connected) / 1000) }

# ----------------------------------------------------------------------
# Create plot image. May take a few minutes.
# ----------------------------------------------------------------------

time_plotting_start <- proc.time()

min_mi <- min(connected[, 5], na.rm = TRUE)
max_mi <- max(connected[, 5], na.rm = TRUE)
max_distance <- max(connected[, 3], na.rm = TRUE)
exponent <- round(log10(max_distance)) - 1

png(output_full_filepath, width = plot_width, height = plot_height, pointsize = plot_pointsize)
plot(connected[!connected[, 4], 3], connected[!connected[, 4], 5], col = color_indirect, type = "p", pch = plot_symbol, cex = cex_indirect, 
     xlim = c(0, max_distance), ylim = c(min_mi, max_mi), xaxs = "i", yaxs = "i",
     xlab = "", ylab = "", xaxt = "n", yaxt = "n", bty = "n")
lines(connected[as.logical(connected[, 4]), 3], connected[as.logical(connected[, 4]), 5], col = color_direct, type = "p", pch = plot_symbol, cex = cex_direct)
axis(1, at = seq(0, max_distance, 10^exponent), tick = FALSE, labels = seq(0, max_distance / 10^exponent), line = -0.8)
axis(2, at = seq(0.05, 1, 0.05), labels = FALSE, tcl = -0.5)
axis(2, at = seq(0.1, 1, 0.1), labels = seq(0.1, 1, 0.1), las = 1, tcl = -0.5)
title(xlab = "Distance between unitigs (bp)", line = 1.2)
title(xlab = substitute(x10^exp, list(exp = exponent)), line = 1.4, adj = 1)
title(ylab = score_name, line = 2.5)

# Output plot title.
if (length(plot_title) > 0) { title(main = plot_title) }

# Linkage disequilibrium distance.
if (ld_dist > 0) { segments(ld_dist, min_mi, ld_dist, 1, col = color_ld, lty = 2) } 
if (ld_dist_alt > 0) { segments(ld_dist_alt, min_mi, ld_dist_alt, 1, col = color_ld_alt, lty = 2) }

# Outlier thresholds.
if (outlier_threshold > 0) {
  segments(0, outlier_threshold, max_distance, outlier_threshold, col = color_outlier, lty = 2) 
  text(0, outlier_threshold, "*", col = color_outlier, pos = 2, offset = 0.2, cex = 1, xpd = NA)
}
if (extreme_outlier_threshold > 0) {
  segments(0, extreme_outlier_threshold, max_distance, extreme_outlier_threshold, col = color_extreme_outlier, lty = 2) 
  text(0, extreme_outlier_threshold, "**", col = color_extreme_outlier, pos = 2, offset = 0.2, cex = 1, xpd = NA)
}
if (outlier_threshold_alt > 0) {
  segments(0, outlier_threshold_alt, max_distance, outlier_threshold_alt, col = color_outlier_alt, lty = 2) 
  text(0, outlier_threshold_alt, "*", col = color_outlier_alt, pos = 2, offset = 0.8, cex = 1, xpd = NA)
}
if (extreme_outlier_threshold_alt > 0) { # Extreme outlier threshold.
  segments(0, extreme_outlier_threshold_alt, max_distance, extreme_outlier_threshold_alt, col = color_extreme_outlier_alt, lty = 2) 
  text(0, extreme_outlier_threshold_alt, "**", col = color_extreme_outlier_alt, pos = 2, offset = 0.8, cex = 1, xpd = NA)
}

legend(x = max_distance, y = max_mi, cex = cex_legend, pch = plot_symbol, bty = "n", xjust = 1.2, yjust = 1.2,
       c("Indirect", "Direct"), col = c(color_indirect, color_direct))

text(0.90 * max_distance, min_mi + 0.8 * (max_mi - min_mi), paste0("#Connected: ", n_K_connected, "K"), cex = 0.8, adj = 0)
text(0.90 * max_distance, min_mi + 0.78 * (max_mi - min_mi), paste0("#Disconnected: ", n_K_disconnected, "K"), cex = 0.8, adj = 0)

if (count_criterion > 0) {
  text(0.90 * max_distance, min_mi + 0.76 * (max_mi - min_mi), paste0("#Filtered (<", count_criterion, "): ", n_K_filtered, "K"), cex = 0.8, adj = 0)
}

dev.off()

time_plotting_end <- proc.time()
time_plotting <- (time_plotting_end - time_plotting_start)[[3]]
cat(paste0("Time plotting=", time_plotting, "\n"))
