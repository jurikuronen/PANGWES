/*
 * FileWriterInterface.hpp - File writer interface.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <ostream>
#include <string>

namespace PANGWES {

// File writer interface.
class FileWriterInterface {
public:
    virtual ~FileWriterInterface() noexcept = default;

    // Returns the name of the file that this writer will write to.
    virtual std::string filename() const noexcept = 0;

    // Returns access to the underlying output stream.
    virtual std::ostream& out() noexcept = 0;

};

} // namespace PANGWES
