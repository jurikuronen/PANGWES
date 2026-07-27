/*
 * entrypoint.hpp - Common main() implementation for PAN-GWES components.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cstddef>
#include <exception>
#include <iostream>

#include "common/architecture/architecture.hpp"
#include "common/io/Log.hpp"
#include "common/project_information/project_information.hpp"
#include "common/utils/WorkerPool.hpp"

namespace PANGWES {
namespace Entry {

// Helper structure to ensure that the worker pool is shutdown when exiting the program.
struct WorkerPoolScoped {
    explicit WorkerPoolScoped(std::size_t n_workers) {
        WorkerPool::init_and_start(n_workers);
    }

    ~WorkerPoolScoped() {
        WorkerPool::stop();
    }

    WorkerPoolScoped(const WorkerPoolScoped&) = delete;
    WorkerPoolScoped& operator=(const WorkerPoolScoped&) = delete;
    WorkerPoolScoped(WorkerPoolScoped&&) = delete;
    WorkerPoolScoped& operator=(WorkerPoolScoped&&) = delete;
};

template <typename ProjectOptions, typename CheckOptionsFunction, typename RunFunction>
int run_program(int argc,
                char** argv,
                const char* project_name,
                CheckOptionsFunction check_options_func,
                RunFunction run_func)
{
    ProjectInformation::print_project_information(project_name);

    try {
        // Process command line arguments.
        ProjectOptions options(argc, argv);

        // True if the user supplied the "help" or "version" options or didn't provide any options.
        if (options.help_requested() || options.version_requested()) {
            return 0;
        }

        // Set verbose state to the Log stream globally.
        Log::set_verbose(options.verbose());

        // Check that mandatory options were provided and are okay.
        if (!check_options_func(options)) {
            return 1;
        }

        // Print run details from read options if verbose.
        options.print_run_details();

        WorkerPoolScoped pool(options.n_workers());

        run_func(options);

    } catch (const std::exception& exception) {
        std::cerr << exception.what() << std::endl;

        return 1;
    }

    return 0;
}

} // namespace Entry
} // namespace PANGWES
