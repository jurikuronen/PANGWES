# gwes_plot.r - Draw a GWES Manhattan plot from unitig_distance results.
#
# MIT License (see LICENSE in the repository root).
# Copyright (c) 2020-2026 Juri Kuronen

# User-definable parameters.
config <- list(
    input_file = "",
    output_file = "unitig_distance_gwes_plot.png",
    score_name = "Mutual information",
    plot_title = "GWES Manhattan Plot",

    # Discard unitig pairs with connected SGGs count below this value.
    connected_sgg_count_filter = 0,

    # Discard unitig pairs with relative standard deviation above this value.
    rsd_max = 1,

    # Draw plot using the median graph distances, if available.
    draw_median_distance = FALSE,

    plot_width = 1920,
    plot_height = 1080,
    plot_pointsize = 16,

    # 19 - solid circle.
    plot_symbol = 19,
    point_color = rgb(0, 115, 190, maxColorValue = 255),
    point_cex = 0.2
)

# Disable scientific notation.
options(scipen = 999)

# Read command-line arguments when run with Rscript.
if (!interactive()) {
    args <- commandArgs(trailingOnly = TRUE)
    if (length(args) != 4) {
        stop(paste("Call with: Rscript gwes_plot.r <input_file> <output_file> <connected_sgg_count_filter>",
                   "<draw_median_distance>"))
    }
    config$input_file <- args[1]
    config$output_file <- args[2]
    config$connected_sgg_count_filter <- as.numeric(args[3])
    config$draw_median_distance <- as.logical(as.numeric(args[4]))
}

# Read unitig_distance output.
cat("Reading input: ", config$input_file, ".\n", sep = "")
data <- read.table(config$input_file, header = FALSE)

cat("Applying filters.\n")

median_distance_field <- 10

# First check that the data has the correct number of columns.
if (ncol(data) < 9) {
    stop(paste("Incorrect data format: expected at least 9 columns, got", ncol(data)))
} else if (ncol(data) < median_distance_field) {
    # Compatibility check: add the median distance field (as undefined) for pre-1.0.0 PANGWES results.
    data[, median_distance_field] <- -1
    if (config$draw_median_distance) {
        cat("Data does not contain the median distances field; setting draw_median_distance to FALSE.\n")
    }
    config$draw_median_distance <- FALSE
} else if (all(data[, median_distance_field] == -1)) {
    # Finally, check if the median graph distances are defined.
    if (config$draw_median_distance) {
        cat("Data does not contain median distances (all -1); setting draw_median_distance to FALSE.\n")
    }
    config$draw_median_distance <- FALSE
}

# In case the input has been mapped, it contains extra columns that are not needed by this script.
data <- data[, 1:median_distance_field]
names(data) <- c("unitig1", "unitig2", "mean_distance", "aracne_flag", "score", "connected_sgg_count",
                 "sample_variance", "min_distance", "max_distance", "median_distance")

# Select which graph distance type to draw.
if (config$draw_median_distance) {
    distance_field <- "median_distance"
    plot_distance_text <- "Median graph distance"
} else {
    distance_field <- "mean_distance"
    plot_distance_text <- "Mean graph distance"
}

# Discard disconnected unitig pairs (with graph distances set to -1).
data <- data[data[[distance_field]] > 0, ]

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

cat("Drawing the GWES plot.\n")

# Draw the GWES Manhattan plot.
max_distance <- max(data[[distance_field]])
min_score <- min(data$score)
max_score <- max(data$score)
distance_exponent <- round(log10(max_distance)) - 1

png(config$output_file, width = config$plot_width, height = config$plot_height, pointsize = config$plot_pointsize)

plot(data[[distance_field]], data$score, type = "n",
     xlim = c(0, max_distance), ylim = c(min_score, max_score),
     xlab = "", ylab = "", xaxt = "n", yaxt = "n", bty = "n", xaxs = "i", yaxs = "i")
points(data[[distance_field]], data$score, col = config$point_color, pch = config$plot_symbol, cex = config$point_cex)

axis(1, at = seq(0, max_distance, 10^distance_exponent), tick = FALSE,
     labels = seq(0, max_distance / 10^distance_exponent), line = -0.8)
axis(2, at = seq(0.05, 1, 0.05), labels = FALSE, tcl = -0.5)
axis(2, at = seq(0.1, 1, 0.1), labels = seq(0.1, 1, 0.1), las = 1, tcl = -0.5)

title(xlab = paste(plot_distance_text, "between unitigs (bp)"), line = 1.2)
title(xlab = substitute(x10^exp, list(exp = distance_exponent)), line = 1.4, adj = 1)
title(ylab = config$score_name, line = 2.5)
title(main = config$plot_title)

invisible(dev.off())

cat("Done. Wrote the plot to: ", config$output_file, ".\n", sep = "")
