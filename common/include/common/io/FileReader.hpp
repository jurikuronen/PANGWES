/*
 * FileReader.hpp - Simple file reader class.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cstddef>
#include <fstream>
#include <string>

#include "common/io/FileReaderInterface.hpp"
#include "common/utils/Exception.hpp"

namespace PANGWES {

/*
 * File reader implementation.
 *
 * Templated on IfStream to allow testing with MockIfStream.
*/
template <typename IfStream>
class FileReaderImplementation final : public FileReaderInterface {
public:
    // Constructs a file reader and stores the filename to open with `open()`. Throws if the filename is empty.
    explicit FileReaderImplementation(const std::string& filename)
        : m_ifs{},
          m_filename{filename},
          m_line_number{}
    {
        if (m_filename.empty()) {
            throw Exception(ErrorCode::EMPTY_FILENAME);
        }
    }

    FileReaderImplementation(const FileReaderImplementation&) = delete;
    FileReaderImplementation& operator=(const FileReaderImplementation&) = delete;
    FileReaderImplementation(FileReaderImplementation&&) noexcept = default;
    FileReaderImplementation& operator=(FileReaderImplementation&&) noexcept = default;

    // Opens the file identified by the stored filename for reading and resets the line number. Throws on any errors.
    void open() override final {
        if (m_ifs.is_open()) {
            throw Exception(ErrorCode::FAILED_TO_OPEN_FILE, m_filename, " is already open for reading");
        }

        m_ifs.open(m_filename);

        if (!m_ifs.is_open() || !m_ifs.good()) {
            throw Exception(ErrorCode::FAILED_TO_OPEN_FILE, m_filename);
        }

        if (m_ifs.peek() == IfStream::traits_type::eof()) {
            throw Exception(ErrorCode::FILE_EMPTY, m_filename);
        }

        m_line_number = 0;
    }

    // Closes the file opened by this reader. Throws if the file stream was not open or if closing fails.
    void close() override final {
        if (!m_ifs.is_open()) {
            throw Exception(ErrorCode::FAILED_TO_CLOSE_FILE, m_filename, " is not open");
        }

        // Isolate errors caused specifically by close().
        m_ifs.clear();

        m_ifs.close();

        if (m_ifs.fail()) {
            throw Exception(ErrorCode::FAILED_TO_CLOSE_FILE, m_filename);
        }
    }

    // Returns the name of the file currently opened by this reader.
    std::string filename() const noexcept override final {
        return m_filename;
    }

    // Attempts to read the next line and increment line number. Returns true on success.
    bool getline(std::string& line) override final {
        // Enable ADL to find a mocked overload for unit testing.
        using std::getline;

        if (getline(m_ifs, line)) {
            ++m_line_number;
            return true;
        }

        return false;
    }

    // Returns the 1-based count of lines read so far.
    std::size_t line_number() const noexcept override final {
        return m_line_number;
    }

private:
    IfStream m_ifs;
    std::string m_filename;
    std::size_t m_line_number;
};

using FileReader = FileReaderImplementation<std::ifstream>;

} // namespace PANGWES
