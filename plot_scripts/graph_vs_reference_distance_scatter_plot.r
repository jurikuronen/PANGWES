# graph_vs_reference_distance_scatter_plot.r - Draw a scatter plot comparing graph and reference-genome distances from
#                                              mapped unitig_distance results.
#
# MIT License (see LICENSE in the repository root).
# Copyright (c) 2020-2026 Juri Kuronen

# User-definable parameters.
config <- list(
    input_file = "",
    output_file = "unitig_distance_graph_vs_reference_genome_distance_plot.png",
    plot_title = "Graph vs reference-genome distance",

    # Discard unitig pairs with connected SGGs count below this value.
    connected_sgg_count_filter = 0,

    # Length of the reference genome used for mapping. MUST be set for reference-genome distance calculation.
    reference_genome_length = 0,

    # Discard unitig pairs with relative standard deviation above this value.
    rsd_max = 1,

    # Draw plot using the median graph distances, if available.
    draw_median_distance = FALSE,

    plot_width = 1920,
    plot_height = 1080,
    plot_pointsize = 16,

    # 19 - solid circle.
    plot_symbol = 19,
    point_color = "#3685be",
    point_cex = 0.15,

    # Minimum coordinate for log10 axes to leave room for zero log values.
    axis_min = -0.1
)

# Disable scientific notation.
options(scipen = 999)


# Read command-line arguments when run with Rscript.
if (!interactive()) {
    args <- commandArgs(trailingOnly = TRUE)
    if (length(args) != 5) {
        stop(paste("Call with: Rscript graph_vs_reference_distance_scatter_plot.r <input_file> <output_file>",
                   "<connected_sgg_count_filter> <reference_genome_length> <draw_median_distance>"))
    }
    config$input_file <- args[1]
    config$output_file <- args[2]
    config$connected_sgg_count_filter <- as.numeric(args[3])
    config$reference_genome_length <- as.numeric(args[4])
    config$draw_median_distance <- as.logical(as.numeric(args[5]))
}

# Read mapped unitig_distance output.
cat("Reading input: ", config$input_file, ".\n", sep = "")
data <- read.table(config$input_file, header = FALSE)

cat("Applying filters.\n")

median_distance_field <- 10

if (ncol(data) < 14) {
    stop(paste("Incorrect data format: expected at least 13 columns, got", ncol(data)))
}

# Check if the median graph distances are defined.
if (all(data[, median_distance_field] == -1)) {
    if (config$draw_median_distance) {
        cat("Data does not contain median distances (all -1); setting draw_median_distance to FALSE.\n")
    }
    config$draw_median_distance <- FALSE
}

data <- data[, 1:14]
names(data) <- c("unitig1", "unitig2", "mean_distance", "aracne_flag", "score", "connected_sgg_count",
                 "sample_variance", "min_distance", "max_distance", "median_distance", "mapped_unitig1",
                 "mapped_unitig2", "unitig1_sequence", "unitig2_sequence")

# Select which graph distance type to draw.
if (config$draw_median_distance) {
    distance_field <- "median_distance"
    plot_distance_text <- "Median graph distance"
} else {
    distance_field <- "mean_distance"
    plot_distance_text <- "Mean graph distance"
}

# Calculate circular reference-genome distances from mapped positions.
data$reference_genome_distance <- abs(data$mapped_unitig1 - data$mapped_unitig2)
data$reference_genome_distance <- pmin(data$reference_genome_distance,
                                       abs(config$reference_genome_length - data$reference_genome_distance))

# Discard disconnected unitig pairs (with graph distances set to -1) and any unexpected non-positive distances.
data <- data[data[[distance_field]] > 0 & data$reference_genome_distance > 0, ]

# Sanity check: filtering based on median graph distances should not leave -1 mean graph distances.
if (config$draw_median_distance && any(data$mean_distance <= 0)) {
    stop("Invalid data: found rows with a positive median distance but undefined mean distance.")
}

# Discard unitig pairs with connected SGGs count below the configured value.
data <- data[data$connected_sgg_count >= config$connected_sgg_count_filter, ]

# Compute relative standard deviation.
# Treat undefined sample variance (-1) for count-one pairs as zero so small inputs remain plottable.
# Real analyses are expected to use a connected SGG count filter of at least 2.
data$rsd <- sqrt(pmax(data$sample_variance, 0)) / data$mean_distance

# Discard unitig pairs with RSD above the configured value.
data <- data[data$rsd <= config$rsd_max, ]

cat("Drawing the Graph vs Reference distance plot.\n")

# Draw graph distance against reference-genome distance.
max_graph_distance_log10 <- ceiling(log10(max(data[[distance_field]])))
max_reference_genome_distance_log10 <- ceiling(log10(max(data$reference_genome_distance)))
x_ticks <- 0:max_graph_distance_log10
y_ticks <- 0:max_reference_genome_distance_log10
x_tick_labels <- parse(text = c("1", "10", paste0("10^", 2:max_graph_distance_log10)))
y_tick_labels <- parse(text = c("1", "10", paste0("10^", 2:max_reference_genome_distance_log10)))

png(config$output_file, width = config$plot_width, height = config$plot_height, pointsize = config$plot_pointsize)

plot(log10(data[[distance_field]]), log10(data$reference_genome_distance), type = "n",
     xlim = c(config$axis_min, max_graph_distance_log10),
     ylim = c(config$axis_min, max_reference_genome_distance_log10),
     xlab = "", ylab = "", xaxt = "n", yaxt = "n", bty = "n", xaxs = "i", yaxs = "i")

# Dotted horizontal guide lines at 10^1, 10^2, 10^3, ...
abline(h = 1:max_reference_genome_distance_log10, col = "grey75", lty = "dotted")

points(log10(data[[distance_field]]), log10(data$reference_genome_distance), col = config$point_color,
       pch = config$plot_symbol, cex = config$point_cex)

segments(config$axis_min, config$axis_min, max_graph_distance_log10, config$axis_min)
segments(config$axis_min, config$axis_min, config$axis_min, max_reference_genome_distance_log10)
axis(1, at = x_ticks, labels = x_tick_labels, lwd = 0, lwd.ticks = 1)
axis(2, at = y_ticks, labels = y_tick_labels, lwd = 0, lwd.ticks = 1, las = 1)

title(xlab = plot_distance_text, line = 2.5)
title(ylab = "Reference-genome distance", line = 2.5)
title(main = config$plot_title)

invisible(dev.off())

cat("Done. Wrote the plot to: ", config$output_file, ".\n", sep = "")
