/*
 * filesystem.cpp - Small helpers for filesystem operations.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <cerrno>
#include <cstdio>
#include <string>
#ifdef _WIN32
#include <direct.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>

#include "common/io/filesystem.hpp"
#include "common/io/Log.hpp"
#include "common/utils/Exception.hpp"

namespace PANGWES {
namespace Filesystem {
namespace {

#ifdef _WIN32
bool directory_exists_impl(const std::string& directory_path) {
    struct _stat64 info;

    if (_stat64(directory_path.c_str(), &info) != 0) {
        return false;
    }

    return (info.st_mode & _S_IFMT) == _S_IFDIR;
}
#else
bool directory_exists_impl(const std::string& directory_path) {
    struct stat info;

    if (stat(directory_path.c_str(), &info) != 0) {
        return false;
    }

    return S_ISDIR(info.st_mode);
}
#endif

} // namespace

std::size_t file_size(const std::string& filename) {
    if (filename.empty()) {
        throw Exception(ErrorCode::EMPTY_FILENAME);
    }

#ifdef _WIN32
    struct _stat64 info;

    if (_stat64(filename.c_str(), &info) != 0 || (info.st_mode & _S_IFMT) != _S_IFREG) {
        throw Exception(ErrorCode::FAILED_TO_GET_FILE_SIZE, filename);
    }
#else
    struct stat info;

    if (stat(filename.c_str(), &info) != 0 || !S_ISREG(info.st_mode)) {
        throw Exception(ErrorCode::FAILED_TO_GET_FILE_SIZE, filename);
    }
#endif

    return static_cast<std::size_t>(info.st_size);
}

bool directory_exists(const std::string& directory_path) {
    return directory_exists_impl(directory_path);
}

bool create_directory(const std::string& directory_path) {
#ifdef _WIN32
    int result_code = _mkdir(directory_path.c_str());
#else
    int result_code = mkdir(directory_path.c_str(), 0755);
#endif

    if (result_code == 0) {
        return true;
    }

    if (errno == EEXIST) {
        return directory_exists(directory_path);
    }

    return false;
}

void remove_file_if_exists(const std::string& filename) {
    if (filename.empty()) {
        return;
    }

    if (std::remove(filename.c_str()) == 0) {
        Log::out_without_date_block() << "INFO: Removed file \"" << filename << "\"." << std::endl;
    }
}

void remove_empty_directory_if_exists(const std::string& directory_path) {
    if (directory_path.empty()) {
        return;
    }

#ifdef _WIN32
    const auto result_code = _rmdir(directory_path.c_str());
#else
    const auto result_code = std::remove(directory_path.c_str());
#endif

    if (result_code == 0) {
        Log::out_without_date_block() << "INFO: Removed empty directory \"" << directory_path << "\"." << std::endl;
    }
}

} // namespace Filesystem
} // namespace PANGWES
