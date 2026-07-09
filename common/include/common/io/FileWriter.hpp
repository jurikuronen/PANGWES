/*
 * FileWriter.hpp - Simple file writer class.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <fstream>
#include <string>

#include "common/io/FileWriterInterface.hpp"
#include "common/io/Log.hpp"
#include "common/utils/Exception.hpp"

namespace PANGWES {
namespace FileWriterUtils {

// Real filesystem check for file existence.
template <typename OfStream>
struct FileExists {
    static bool check(const std::string& filename) {
        return std::ifstream(filename).good();
    }
};

} // namespace FileWriterUtils

/*
 * File writer implementation.
 *
 * Templated on OfStream to allow testing with MockOfStream.
*/
template <typename OfStream>
class FileWriterImplementation final : public FileWriterInterface {
public:
    /*
     * Constructs a file writer and opens the output file immediately. Throws on any errors.
     *
     * If the specified file already exists, the constructor calls `make_unique_filename()` that tries to choose a
     * unique filename by appending ".1", ".2", ..., to the filename.
    */
    explicit FileWriterImplementation(const std::string& filename)
        : m_ofs{},
          m_filename{}
    {
        if (filename.empty()) {
            throw Exception(ErrorCode::EMPTY_FILENAME);
        }

        m_filename = make_unique_filename(filename);

        m_ofs.open(m_filename);

        if (!m_ofs.is_open()) {
            throw Exception(ErrorCode::FAILED_TO_OPEN_FILE, m_filename);
        }
    }

    FileWriterImplementation(const FileWriterImplementation&) = delete;
    FileWriterImplementation& operator=(const FileWriterImplementation&) = delete;
    FileWriterImplementation(FileWriterImplementation&&) noexcept = default;
    FileWriterImplementation& operator=(FileWriterImplementation&&) noexcept = default;

    // Returns the name of the file that this writer will write to. Guaranteed to not overwrite existing files.
    std::string filename() const noexcept override final {
        return m_filename;
    }

    // Returns access to the underlying output stream.
    std::ostream& out() noexcept override final {
        return m_ofs;
    }

private:
    OfStream m_ofs;
    std::string m_filename;

    // If a file with the given filename already exists, append ".1", ".2", ..., until we get a unique filename.
    static std::string make_unique_filename(const std::string& filename) {
        constexpr auto MAX_UNIQUE_FILENAME_TRIES = 256;

        if (FileWriterUtils::FileExists<OfStream>::check(filename)) {
            for (auto i = 1; i <= MAX_UNIQUE_FILENAME_TRIES; ++i) {
                const auto filename_candidate = filename + "." + std::to_string(i);

                if (!FileWriterUtils::FileExists<OfStream>::check(filename_candidate)) {
                    return filename_candidate;
                }
            }

            throw Exception(ErrorCode::FAILED_TO_GENERATE_UNIQUE_NAME, " for ", filename);
        }

        return filename;
    }
};

using FileWriter = FileWriterImplementation<std::ofstream>;

} // namespace PANGWES
