/*
 * Log.hpp - Thread-safe logging class.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <atomic>
#include <iostream>
#include <mutex>
#include <ostream>
#include <sstream>

#include "common/utils/format.hpp"
#include "common/utils/Timer.hpp"

namespace PANGWES {

// Generic output stream provider for LogImplementation.
template <typename OStream>
struct OStreamProvider {
    static OStream& get() {
        static OStream ostream;

        return ostream;
    }
};

// Output stream provider specialization for std::ostream, binding to std::cout.
template <>
struct OStreamProvider<std::ostream> {
    static std::ostream& get() {
        return std::cout;
    }
};

/*
 * Thread-safe logging class implementation.
 *
 * Each thread writes into a thread-local buffer. However, when verbose (default: false) is set to false, `operator<<()`
 * becomes a no-op.
 *
 * Output is written under a global mutex only when std::endl is inserted. That is, writing '\n' does not flush.
 *
 * Templated on OStream and Timer to allow testing with a MockOStream and MockTimer.
*/
template <typename OStream, typename Timer>
class LogImplementation {
private:
    // Internal stream buffer class.
    class LogStream {
    public:
        // Buffers values into the calling thread's local buffer. No-op when verbose is false.
        template <typename Value>
        LogStream& operator<<(const Value& value) {
            if (verbose().load()) {
                thread_buffer() << value;
            }

            return *this;
        }

        /*
         * Handles I/O manipulators. Inserting std::endl writes and flushes the buffer.
         *
         * No-op when verbose is false.
        */
        LogStream& operator<<(std::ostream& (*manipulator_cb)(std::ostream&)) {
            if (verbose().load()) {
                if (manipulator_cb == static_cast<std::ostream& (*)(std::ostream&)>(std::endl)) {
                    // Write and flush the buffer to stdout and clear it afterwards.
                    std::lock_guard<std::mutex> lock(write_mutex());

                    stream_out() << thread_buffer().str() << std::endl;

                    thread_buffer().str("");
                    thread_buffer().clear();
                } else {
                    thread_buffer() << manipulator_cb;
                }
            }

            return *this;
        }

    private:
        // Returns the output stream via an output stream provider.
        static OStream& stream_out() {
            return OStreamProvider<OStream>::get();
        }

        // Returns a thread-local buffer.
        static std::ostringstream& thread_buffer() {
            static thread_local std::ostringstream buffer{};

            return buffer;
        }
    };

public:
    // Returns the global log stream instance and prefixes the log output with the current time date block.
    static LogStream& out() noexcept {
        static Timer timer;

        return stream() << Format::date_block(timer.unix_time_ms()) << " ";
    }

    // Returns just the global log stream instance.
    static LogStream& out_without_date_block() noexcept {
        return stream();
    }


    /*
     * Set global verbose state. If false, logging is disabled.
     *
     * Note: setting verbose to false during ongoing logging potentially leaves the thread buffers in a corrupted state.
    */
    static void set_verbose(bool new_verbose) noexcept {
        verbose().store(new_verbose);
    }

private:
    LogImplementation() = default;

    // Returns the global log stream instance.
    static LogStream& stream() {
        static LogStream stream;

        return stream;
    }

    // Returns a global mutex to protect writes.
    static std::mutex& write_mutex() {
        static std::mutex write_mutex;

        return write_mutex;
    }

    // Returns a global verbose state variable.
    static std::atomic<bool>& verbose() {
        static std::atomic<bool> verbose{false};

        return verbose;
    }
};

using Log = LogImplementation<std::ostream, Timer>;

} // namespace PANGWES
