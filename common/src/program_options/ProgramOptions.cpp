/*
 * ProgramOptions.cpp - Command line arguments reader that stores options used by the program.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "common/program_options/ProgramOptions.hpp"
#include "common/utils/Exception.hpp"
#include "common/utils/utils.hpp"

namespace PANGWES {

ProgramOptions::ProgramOptions(int argc, char** argv)
    : m_argc{argc},
      m_argv{argv}
{ }

char** ProgramOptions::argv_begin() const noexcept {
    return m_argv + 1;
}

char** ProgramOptions::argv_end() const noexcept {
    return m_argv + m_argc;
}

char** ProgramOptions::argv_find(const std::string& program_option) const noexcept {
    return std::find_if(argv_begin(), argv_end(), [&](const char* arg){
        return arg != nullptr && program_option == arg;
    });
}

bool ProgramOptions::find_arg(const std::string& program_option,
                              const std::string& program_option_long) const noexcept
{
    return argv_find(program_option) != argv_end() || argv_find(program_option_long) != argv_end();
}

std::uint64_t ProgramOptions::read_unsigned_value(const std::string& program_option,
                                                  const std::string& program_option_long)
{
    const char* const value = find_arg_value(program_option, program_option_long);

    if (value == nullptr) {
        return 0;
    }

    std::uint64_t unsigned_value{};

    try {
        unsigned_value = Utils::parse_unsigned_value(value);
    } catch (...) {
        throw Exception(ErrorCode::INVALID_PROGRAM_OPTION, program_option, '/', program_option_long, " value: ", value);
    }

    return unsigned_value;
}

std::string ProgramOptions::read_string_value(const std::string& program_option, const std::string& program_option_long)
{
    const char* const value = find_arg_value(program_option, program_option_long);

    return value != nullptr ? std::string{value} : std::string{};
}

void ProgramOptions::print_options_info(const std::vector<std::pair<std::string, std::string>>& options_info) {
    constexpr auto left_width = 40;
    constexpr auto right_width = 60;

    // Use `std::cout` instead of Log: this list must always be printed when requested regardless of verbosity.
    for (const auto& options : options_info) {
        const auto& option_info = options.first;
        auto option_description = options.second;

        // Check if this option pair is for a line break only ({"", ""}).
        if (option_info.empty() && option_description.empty()) {
            std::cout << std::endl;

            continue;
        }

        std::cout << std::left << std::setw(left_width) << option_info;

        // Split description output onto multiple lines if necessary.
        while (option_description.size() > right_width) {
            // Find word break point.
            auto break_point = option_description.rfind(' ', right_width);

            // Defensive programming against long words / no spaces.
            if (break_point == std::string::npos || break_point == 0) {
                break_point = right_width;
            }

            // Print until the break point.
            std::cout << option_description.substr(0, break_point) << std::endl;

            // Remove the printed part from the description string.
            option_description.erase(0, break_point);

            // If the description string now starts with a space, remove it.
            if (!option_description.empty() && option_description.front() == ' ') {
                option_description.erase(0, 1);
            }

            // If the description string is not empty, pre-pad from the left for the next print-out.
            if (!option_description.empty()) {
                std::cout << std::left << std::setw(left_width) << "";
            }
        }

        std::cout << option_description << std::endl;
    }
}

const char* ProgramOptions::find_arg_value(const std::string& program_option,
                                           const std::string& program_option_long) const noexcept
{
    auto* argv_it = argv_find(program_option);

    // Try long option if the short option is not present.
    if (argv_it == argv_end()) {
        argv_it = argv_find(program_option_long);
    }

    if (argv_it == argv_end()) {
        return nullptr;
    }

    // Move iterator to the value.
    ++argv_it;

    return argv_it != argv_end() ? *argv_it : nullptr;
}

} // namespace PANGWES
