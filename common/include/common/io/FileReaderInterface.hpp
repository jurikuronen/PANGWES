/*
 * FileReaderInterface.hpp - File reader interface.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cstddef>
#include <string>

namespace PANGWES {

// File reader interface.
class FileReaderInterface {
public:
    virtual ~FileReaderInterface() noexcept = default;

    // Opens the file identified by the stored filename for reading and resets the line number.
    virtual void open() = 0;

    // Closes the file opened by this reader.
    virtual void close() = 0;

    // Returns the name of the file currently opened by this reader.
    virtual std::string filename() const noexcept = 0;

    // Attempts to read the next line and increment line number. Returns true on success.
    virtual bool getline(std::string& line) = 0;

    // Returns the current line number.
    virtual std::size_t line_number() const noexcept = 0;
};

} // namespace PANGWES
