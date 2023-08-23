/*
    A time-keeping class.
*/
#pragma once

#include <chrono>
#include <ratio>
#include <string>
#include <sstream>

#include "types.hpp"

class Timer {
public:
    using clock = std::chrono::high_resolution_clock;

    Timer() : m_start(now()), m_mark(m_start), m_stopwatch(clock::duration()), m_lap(m_stopwatch) { }

    void set_mark() { m_mark = now(); }

    void set_lap() { m_lap = m_stopwatch; }

    void add_time_since_mark() { m_stopwatch += time_elapsed(m_mark); }

    std::string get_stopwatch_time() const { return get_time_str(m_stopwatch); }

    std::string get_stopwatch_time_since_lap_and_set_lap() { auto time_str = get_time_str(m_stopwatch - m_lap); set_lap(); return time_str; }

    std::string get_time_since_start() const { return get_time_str(time_elapsed(m_start)); }

    std::string get_time_since_mark() const { return get_time_str(time_elapsed(m_mark)); }

    std::string get_time_since_start_and_set_mark() { set_mark(); return get_time_since_start(); }

    std::string get_time_since_mark_and_set_mark() { auto time_str = get_time_since_mark(); set_mark(); return time_str; }

    std::string get_time_block_since_start() const { return get_time_block(time_elapsed(m_start)); }

    std::string get_time_block_since_mark() const { return get_time_block(time_elapsed(m_mark)); }

    std::string get_time_block_since_start_and_set_mark() { set_mark(); return get_time_block_since_start(); }

    std::string get_time_block_since_mark_and_set_mark() { auto time_block = get_time_block_since_mark(); set_mark(); return time_block; }

private:
    clock::time_point m_start; // Set when Timer is created.
    clock::time_point m_mark; // Custom time point mark.
    clock::duration m_stopwatch; // Accumulates time.
    clock::duration m_lap; // Stores time at last lap.

    clock::time_point now() const { return clock::now(); }

    clock::duration time_elapsed(const clock::time_point& tp) const { return now() - tp; }

    std::string get_time_block(const clock::duration& t) const {
        std::ostringstream oss;

        int_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(t).count();
        int_t s = ms / 1000 % 60;
        int_t m = ms / 1000 / 60 % 60;
        int_t h = ms / 1000 / 60 / 60 % 24;
        int_t d = ms / 1000 / 60 / 60 / 24;
        ms %= 1000;

        oss << "[";
        if (d) oss << pad(d) << "d " << pad(h) << "h";
        else if (h) oss << pad(h) << "h " << pad(m) << "m";
        else if (m) oss << pad(m) << "m " << pad(s) << "s";
        else oss << pad(s) << "." << zeropad(ms) << "s";
        oss << "]";

        return oss.str();
    }

    std::string get_time_str(const clock::duration& t) const {
        std::ostringstream oss;

        int_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(t).count();
        int_t s = ms / 1000 % 60;
        int_t m = ms / 1000 / 60 % 60;
        int_t h = ms / 1000 / 60 / 60 % 24;
        int_t d = ms / 1000 / 60 / 60 / 24;
        ms %= 1000;

        if (d) oss << d << "d " << h << "h";
        else if (h) oss << h << "h " << m << "m";
        else if (m) oss << m << "m " << s << "s";
        else oss << s << "." << zeropad(ms) << "s";

        return oss.str();
    }

    std::string pad(int_t t) const { return t < 10 ? " " + std::to_string(t) : std::to_string(t); }

    std::string zeropad(int_t t) const { return t < 10 ? "00" + std::to_string(t) : (t < 100 ? "0" + std::to_string(t) : std::to_string(t)); }

};

