#include "ProgramOptions.hpp"
#include "types.hpp"

int ProgramOptions::argc;
char** ProgramOptions::argv;

std::string ProgramOptions::unitigs_filename = "";
std::string ProgramOptions::edges_filename = "";
std::string ProgramOptions::queries_filename = "";
std::string ProgramOptions::sggs_filename = "";
std::string ProgramOptions::out_stem = "out";
int_t ProgramOptions::k = 0; 
int_t ProgramOptions::n_queries = INT_T_MAX;
int_t ProgramOptions::queries_format = -1;
real_t ProgramOptions::max_distance = REAL_T_MAX;
int_t ProgramOptions::n_threads = 1; 
int_t ProgramOptions::sgg_count_threshold = 10;
int_t ProgramOptions::ld_distance = -1;
int_t ProgramOptions::ld_distance_min = 1000;
real_t ProgramOptions::ld_distance_score = 0.8; 
int_t ProgramOptions::ld_distance_nth_score = 10;
real_t ProgramOptions::outlier_threshold = -1.0;
bool ProgramOptions::graphs_one_based = false;
bool ProgramOptions::queries_one_based = false;
bool ProgramOptions::output_one_based = false;
bool ProgramOptions::run_sggs_only = false;
bool ProgramOptions::output_outliers = false;
bool ProgramOptions::verbose = false;

bool ProgramOptions::valid_state = true;

OperatingMode ProgramOptions::operating_mode = OperatingMode::DEFAULT;
