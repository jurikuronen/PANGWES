/*
 * MockLog.hpp - Mock implementations of Log and LogStream in io/Log.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <sstream>
#include <string>

#include "common/io/Log.hpp"
#include "mocks/MockData.hpp"
#include "mocks/MockTimer.hpp"

namespace PANGWES {
namespace Mocks {

/*
 * Mock output stream implementation for LogImplementation::LogStream.
 *
 * Should be cleared between tests.
*/
class MockOStream {
    public:
        // Buffers values into this mock stream's buffer.
        template <typename Value>
        MockOStream& operator<<(const Value& value) {
            buffer() << value;

            return *this;
        }

        // Buffers I/O manipulators into this mock stream's buffer.
        MockOStream& operator<<(std::ostream& (*manipulator_cb)(std::ostream&)) {
            buffer() << manipulator_cb;

            return *this;
        }

        // Returns the contents of this mock stream's buffer as a single string.
        static std::string contents() {
            return buffer().str();
        }

        // Clears the contents of this mock stream's buffer.
        static void clear() {
            buffer().str("");
            buffer().clear();
        }

    private:
        // Returns a global buffer instance.
        static std::ostringstream& buffer() {
            static std::ostringstream buffer;

            return buffer;
        };
};

using MockLog = LogImplementation<MockOStream, MockTimer>;

} // namespace Mocks
} // namespace PANGWES
