/*
 * MockFileWriter.hpp - Mock implementation of FileWriter in io/FileWriter.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <fstream>
#include <sstream>
#include <string>
#include <utility>

#include "common/io/FileWriter.hpp"
#include "mocks/MockData.hpp"

namespace PANGWES {
namespace Mocks {

// Mock of the std::ofstream implementation of FileWriter.
class MockOfStream {
    public:
        // Resets the buffer when FileWriterImplementation's constructor is called.
        static void open(const std::string&) noexcept {
            buffer().str("");
            buffer().clear();
        }

        // Returns the value configured with "set_is_open_return()" (default: true).
        static bool is_open() noexcept {
            return is_open_result();
        }

        // Returns the contents of the buffer as a single string.
        static std::string contents() {
            return buffer().str();
        }

        // Sets the mocked `is_open()` return value.
        static void set_is_open_return(bool result) noexcept {
            is_open_result() = result;
        }

        /*
         * Sets the mocked `file_already_exists_return()` return value.
         * If persistent, the result persists, otherwise set to true afterwards.
        */
        static void set_file_already_exists_return(bool result, bool persistent) noexcept
        {
            file_already_exists_result() = std::make_pair(result, persistent);
        }

        // Returns the value configured with "set_file_already_exists_return()" (default: false).
        static bool file_already_exists() noexcept {
            const auto result = file_already_exists_result().first;
            const auto persistent = file_already_exists_result().second;

            if (result && !persistent) {
                file_already_exists_result().first = false;
            }

            return result;
        }

        // Mocks to use the static `buffer()`.
        operator std::ostream&() noexcept {
            return buffer();
        }

        // Mocks to use the static `buffer()`.
        operator std::ostream&() const noexcept {
            return buffer();
        }

    private:
        // Returns the static buffer.
        static std::ostringstream& buffer() {
            static std::ostringstream oss{};

            return oss;
        }

        // Returns the mocked stream's file_already_exists() result value: {result, persistent}.
        static std::pair<bool, bool>& file_already_exists_result() {
            static std::pair<bool, bool> result{};

            return result;
        }

        // Returns the mocked stream's is_open() result value.
        static bool& is_open_result() {
            static bool result{true};

            return result;
        }
};

// File writer using MockOfStream.
using MockFileWriter = FileWriterImplementation<MockOfStream>;

} // namespace Mocks

namespace FileWriterUtils {

// Mocked check for file existence.
template <>
struct FileExists<Mocks::MockOfStream> {
    static bool check(const std::string& filename) {
        return Mocks::MockOfStream::file_already_exists();
    }
};

} // namespace FileWriterUtils

} // namespace PANGWES
