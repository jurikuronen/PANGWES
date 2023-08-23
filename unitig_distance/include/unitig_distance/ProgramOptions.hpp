#pragma once

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#include "OperatingMode.hpp"
#include "types.hpp"

// A command line argument handler class.
class ProgramOptions {
public:
    static void read_command_line_arguments(int ac, char** av) {
        argc = ac;
        argv = av;
        if (has_arg("-h", "--help")) {
            print_help();
            valid_state = false;
            return;
        }
        set_value(unitigs_filename, "-U", "--unitigs-file");
        set_value(edges_filename, "-E", "--edges-file");
        set_value(queries_filename, "-Q", "--queries-file");
        set_value(sggs_filename, "-S", "--sgg-paths-file");
        set_value(out_stem, "-o", "--output-stem");
        set_value(k, "-k", "--k-mer-length");
        set_value(n_queries, "-n", "--n-queries");
        set_value(queries_format, "-q", "--queries-format");
        set_value(max_distance, "-d", "--max-distance");
        set_value(n_threads, "-t", "--threads");
        set_value(sgg_count_threshold, "-Cc", "--sgg-count-threshold");
        set_value(ld_distance, "-l", "--ld-distance");
        set_value(ld_distance_min, "-lm", "--ld-distance-min");
        set_value(ld_distance_score, "-ls", "--ld-distance-score");
        set_value(ld_distance_nth_score, "-ln", "--ld-distance-nth-score");
        set_value(outlier_threshold, "-ot", "--outlier-threshold");
        if (has_arg("-1", "--all-one-based")) {
            graphs_one_based = queries_one_based = output_one_based = true;
        } else {
            graphs_one_based = has_arg("-1g", "--graphs-one-based");
            queries_one_based = has_arg("-1q", "--queries-one-based");
            output_one_based = has_arg("-1o", "--output-one-based");
        }
        run_sggs_only = has_arg("-r", "--run-sggs-only");
        output_outliers = has_arg("-x", "--output-outliers");
        verbose = has_arg("-v", "--verbose");

        set_operating_mode();

        if (n_queries < 0) n_queries = INT_T_MAX;
        if (sggs_filename.empty()) sgg_count_threshold = 0;

        valid_state = all_required_arguments_provided();
    }

    static bool has_operating_mode(const OperatingMode& om) { return operating_mode_to_bool(operating_mode & om); }

    static std::string out_filename() { return out_stem + ".ud" + based_str(); }
    static std::string out_sgg_filename() { return out_stem + ".ud_sgg" + based_str(); }
    static std::string out_outliers_filename() { return out_stem + ".ud_outliers" + based_str(); }
    static std::string out_sgg_outliers_filename() { return out_stem + ".ud_sgg_outliers" + based_str(); }
    static std::string out_outlier_stats_filename() { return out_stem + ".ud_outlier_stats"; }
    static std::string out_sgg_outlier_stats_filename() { return out_stem + ".ud_sgg_outlier_stats"; }

    // Print details about this run.
    static void print_run_details() {
        std::vector<std::string> arguments;

        if (!edges_filename.empty()) {
            double_push_back(arguments, "  --edges-file", edges_filename);
            double_push_back(arguments, "  --graphs-one-based", graphs_one_based ? "TRUE" : "FALSE");
        }
        if (has_operating_mode(OperatingMode::CDBG)) {
            double_push_back(arguments, "  --unitigs-file", unitigs_filename);
            double_push_back(arguments, "  --k-mer-length", std::to_string(k));
        }
        if (has_operating_mode(OperatingMode::SGGS)) {
            double_push_back(arguments, "  --sgg-paths-file", sggs_filename);
            double_push_back(arguments, "  --run-sggs-only", run_sggs_only ? "TRUE" : "FALSE");
        }
        double_push_back(arguments, "  --queries-file", queries_filename);
        double_push_back(arguments, "  --queries-one-based", queries_one_based ? "TRUE" : "FALSE");
        double_push_back(arguments, "  --n-queries", n_queries == INT_T_MAX ? "ALL" : std::to_string(n_queries));
        double_push_back(arguments, "  --queries-format", queries_format < 0 ? "AUTOM" : std::to_string(queries_format));
        double_push_back(arguments, "  --max-distance", max_distance == REAL_T_MAX ? "INF" : std::to_string(max_distance));
        if (has_operating_mode(OperatingMode::OUTLIER_TOOLS)) {
            double_push_back(arguments, "  --output-outliers", output_outliers ? "TRUE" : "FALSE");
            double_push_back(arguments, "  --sgg-count-threshold", std::to_string(sgg_count_threshold));
            double_push_back(arguments, "  --ld-distance", ld_distance < 0 ? "AUTOM" : std::to_string(ld_distance));
            if (ld_distance < 0) {
                double_push_back(arguments, "  --ld-distance-min", std::to_string(ld_distance_min));
                double_push_back(arguments, "  --ld-distance-score", std::to_string(ld_distance_score));
                double_push_back(arguments, "  --ld-distance-nth-score", std::to_string(ld_distance_nth_score));
            } else {
                double_push_back(arguments, "  --outlier-threshold", std::to_string(outlier_threshold));
            }
        }
        double_push_back(arguments, "  --output-stem", out_stem);
        double_push_back(arguments, "  --output-one-based", output_one_based ? "TRUE" : "FALSE");
        double_push_back(arguments, "  --threads", std::to_string(n_threads));

        std::cout << "Using following arguments:" << std::endl;
        for (std::size_t i = 0; i < arguments.size(); i += 2) std::printf("%-30s %s\n", arguments[i].data(), arguments[i + 1].data());
        std::cout << std::endl;

        std::cout << "Operating mode: " << operating_mode << std::endl << std::endl;
    }

    static std::string unitigs_filename;
    static std::string edges_filename;
    static std::string queries_filename;
    static std::string sggs_filename;
    static std::string out_stem;
    static int_t k;
    static int_t n_queries;
    static int_t queries_format;
    static real_t max_distance;
    static int_t n_threads;
    static int_t sgg_count_threshold;
    static int_t ld_distance;
    static int_t ld_distance_min;
    static real_t ld_distance_score;
    static int_t ld_distance_nth_score;
    static real_t outlier_threshold;
    static bool graphs_one_based;
    static bool queries_one_based;
    static bool output_one_based;
    static bool run_sggs_only;
    static bool output_outliers;
    static bool verbose;
    static bool valid_state;
    static OperatingMode operating_mode;

private:
    static int argc;
    static char** argv;

    static void set_operating_mode() {
        if (output_outliers) operating_mode |= OperatingMode::OUTLIER_TOOLS;
        if (!edges_filename.empty()) {
            if (unitigs_filename.empty()) {
                operating_mode |= OperatingMode::GENERAL;
            } else {
                operating_mode |= OperatingMode::CDBG;
                if (!sggs_filename.empty()) operating_mode |= OperatingMode::SGGS;
            }
        }
    }

    static bool all_required_arguments_provided() {
        bool ok = true;
        // Always require queries.
        if (queries_filename.empty()) {
            std::cerr << "Error: Missing queries filename.\n";
            ok = false;
        }
        if (queries_format > 5) {
            std::cerr << "Error: Queries format must be less than 6.\n";
            ok = false;
        }
        // Normal operating modes.
        if (operating_mode != OperatingMode::OUTLIER_TOOLS) {
            if (edges_filename.empty()) {
                std::cerr << "Error: Missing edges filename.\n";
                ok = false;
            }
            if (has_operating_mode(OperatingMode::CDBG) && k <= 0) {
                std::cerr << "Error: Missing k-mer length.\n";
                ok = false;
            }
        } else {
            // Correct queries format required, i.e. 4 or 5.
            if (queries_format >= 0 && queries_format < 4) {
                std::cerr << "Error: Queries format must be 4 or 5 in outlier tools mode." << std::endl;
                ok = false;
            }
        }
        if (!ok) print_no_args();
        return ok;
    }

    static std::string based_str() { return output_one_based ? "_1_based" : "_0_based"; }

    static char** begin() { return argv + 1; }
    static char** end() { return argv + argc; }
    static char** find(const std::string& opt) { return std::find(begin(), end(), opt); }
    static char* has_arg(const std::string& opt) { return has_arg(opt, opt); }
    static char* has_arg(const std::string& opt, const std::string& alt) {
        // Don't need the value, return either nullptr or anything for a boolean check.
        return find(opt) != end() || find(alt) != end() ? begin()[0] : nullptr;
    }
    static char* find_arg_value(const std::string& opt) { return find_arg_value(opt, opt); }
    static char* find_arg_value(const std::string& opt, const std::string& alt) {
        auto it = find(opt);
        if (it != end() && ++it != end()) return *it;
        it = find(alt);
        return it != end() && ++it != end() ? *it : nullptr;
    }

    static void print_no_args() { std::cout << "Use '-h' or '--help' for a list of available options.\n"; }

    static void print_help() {
        std::vector<std::string> options{
            "Graph edges:", "",
            "  -E  [ --edges-file ] arg", "Path to file containing graph edges.",
            "  -1g [ --graphs-one-based ]", "Graph files use one-based numbering.",
            "", "",
            "CDBG operating mode:", "",
            "  -U  [ --unitigs-file ] arg", "Path to file containing unitigs.",
            "  -k  [ --k-mer-length ] arg", "k-mer length.",
            "", "",
            "CDBG and/or SGGS operating mode:", "",
            "  -S  [ --sgg-paths-file ] arg", "Path to file containing paths to single genome graph edge files.",
            "  -r  [ --run-sggs-only ]", "Calculate distances only in the single genome graphs.",
            "", "",
            "Distance queries:", "",
            "  -Q  [ --queries-file ] arg", "Path to queries file.",
            "  -1q [ --queries-one-based ]", "Queries file uses one-based numbering.",
            "  -n  [ --n-queries ] arg (=inf)", "Number of queries to read from the queries file.",
            "  -q  [ --queries-format ] arg (-1)", "Set queries format manually (0..5).",
            "  -d  [ --max-distance ] arg (=inf)", "Maximum allowed graph distance (for constraining the searches).",
            "", "",
            "Tools for determining outliers:", "",
            "  -x  [ --output-outliers ]", "Output a list of outliers and outlier statistics.",
            "  -Cc [ --sgg-count-threshold ] arg (=10)", "Filter low count single genome graph distances.",
            "  -l  [ --ld-distance ] arg (=-1)", "Linkage disequilibrium distance (automatically determined if negative).",
            "  -lm [ --ld-distance-min ] arg (=1000)", "Minimum ld distance for automatic ld distance determination.",
            "  -ls [ --ld-distance-score ] arg (=0.8)", "Score difference threshold for automatic ld distance determination.",
            "  -ln [ --ld-distance-nth-score ] arg (=10)", "Use nth max score for automatic ld distance determination.",
            "  -ot [ --outlier-threshold ] arg", "Set outlier threshold to a custom value.",
            "", "",
            "Other arguments.", "",
            "  -o  [ --output-stem ] arg (=out)", "Path for output files (without extension).",
            "  -1o [ --output-one-based ]", "Output files use one-based numbering.",
            "  -1  [ --all-one-based ]", "Use one-based numbering for everything.",
            "  -t  [ --threads ] arg (=1)", "Number of threads.",
            "  -v  [ --verbose ]", "Be verbose.",
            "  -h  [ --help ]", "Print this list.",
        };
        for (std::size_t i = 0; i < options.size(); i += 2) std::printf("%-45s %s\n", options[i].data(), options[i + 1].data());
    }

    static void double_push_back(std::vector<std::string>& arguments, const std::string& opt, const std::string& val) {
        arguments.push_back(opt);
        arguments.push_back(val);
    }

    template <typename T>
    static void set_value(T& value, const std::string& opt, const std::string& alt) { if (has_arg(opt, alt)) std::stringstream(find_arg_value(opt, alt)) >> value; }

};

