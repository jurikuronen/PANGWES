/*
 * MockFileReader.hpp - Mock implementation of FileReader in io/FileReader.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <deque>
#include <fstream>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

#include "common/io/FileReader.hpp"
#include "common/test_harness/test_type_traits.hpp"
#include "mocks/MockData.hpp"

namespace PANGWES {
namespace Mocks {

// Mock the std::ifstream implementation of FileReader.
class MockIfStream {
    public:
        using traits_type = std::istream::traits_type;

        // No-op.
        static void open(const std::string& filename) noexcept {
            (void)filename;
        }

        // No op.
        static void clear() noexcept { }

        // No-op.
        static void close() noexcept { }

        // Returns the value configured with "set_fail_return()" (default: false).
        static bool fail() noexcept {
            return fail_result();
        }

        // Returns the next value configured with "set_is_open_return()".
        static bool is_open() noexcept {
            auto& results = is_open_results();

            assert(!results.empty() && "no mocked result for is_open(); misconfigured test");

            const bool result = results.back();
            results.pop_back();

            return result;
        }

        // Returns true. Use set_is_open_return() to mock whether stream could be associated with a file.
        static bool good() noexcept {
            return true;
        }

        /*
         * Returns either a valid character or EOF, simulating a non-empty vs empty file, depending on the value
         * configured with "set_peek_return(bool)" (default: true=valid character).
        */
        static std::istream::traits_type::int_type peek() noexcept {
            constexpr auto eof = std::ifstream::traits_type::eof();
            constexpr auto not_eof = std::istream::traits_type::to_int_type('a');

            return peek_result() ? not_eof : eof;
        }

        // Returns lines from contents configured with "set_contents()" (default: empty).
        static bool getline(std::string& line) noexcept {
            auto& contents = get_contents();

            if (!contents.empty()) {
                line = std::move(contents.front());
                contents.pop_front();

                return true;
            }

            return false;
        }

        // Sets the stream's is_open() mocked result values.
        static void set_is_open_return(const std::vector<bool>& results) noexcept {
            is_open_results() = results;
        }

        // Sets the stream's `fail()` mocked return value.
        static void set_fail_result(bool result) noexcept {
            fail_result() = result;
        }

        // Sets the stream's peek() mocked result value.
        static void set_peek_return(bool result) noexcept {
            peek_result() = result;
        }

        // Set the contents the mocked file reader will read. Will reset the reader's mocked is_open() state.
        template <typename ContentsContainer = std::initializer_list<std::string>>
        static void set_contents(ContentsContainer new_contents) {
            static_assert(TestTraits::is_string_like<typename Traits::decay_t<ContentsContainer>::value_type>::value,
                          "ContentsContainer::value_type must be convertible to std::string");

            auto& contents = get_contents();

            contents.clear();
            contents.insert(contents.end(),
                            std::make_move_iterator(new_contents.begin()),
                            std::make_move_iterator(new_contents.end()));

            /*
             * Resets the reader's mocked is_open() state:
             * - Returns false for open()'s initial check and true after having called the stream's open().
             * - Returns true for close()'s initial check.
            */
            set_is_open_return({true, true, false});
        }

    private:
        // Returns the static contents deque.
        static std::deque<std::string>& get_contents() {
            static std::deque<std::string> contents;

            return contents;
        }

        // Returns the mocked stream's peek() result value.
        static bool& peek_result() {
            static bool result{true};

            return result;
        }

        // Returns the mocked stream's fail() result value.
        static bool& fail_result() {
            static bool result{false};

            return result;
        }

        // Returns the mocked stream's is_open() result values. The last element in the vector will be consumed.
        static std::vector<bool>& is_open_results() {
            static std::vector<bool> results;

            return results;
        }
};

// Provide mocked getline() for ADL.
inline bool getline(MockIfStream& mock_ifs, std::string& line) noexcept {
    (void)mock_ifs;

    return Mocks::MockIfStream::getline(line);
}

using MockFileReader = FileReaderImplementation<MockIfStream>;

} // namespace Mocks
} // namespace PANGWES
