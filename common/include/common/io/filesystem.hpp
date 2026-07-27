/*
 * filesystem.hpp - Helper functions for filesystem operations.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cstddef>
#include <string>

namespace PANGWES {
namespace Filesystem {

// Returns the size in bytes of the file identified by `filename`. Throws if its size cannot be determined.
std::size_t file_size(const std::string& filename);

// Returns whether the directory already exists.
bool directory_exists(const std::string& directory_path);

// Creates the directory if needed.
bool create_directory(const std::string& directory_path);

/*
 * Attempts to remove the file identified by `filename`.
 *
 * Platform-specific file-removal semantics apply.
 *
 * Does nothing if `filename` is empty. Removal failures are ignored.
*/
void remove_file_if_exists(const std::string& filename);

/*
 * Attempts to remove the empty directory identified by `directory_path`.
 *
 * Platform-specific directory-removal semantics apply.
 *
 * Does nothing if `directory_path` is empty. Removal failures are ignored.
*/
void remove_empty_directory_if_exists(const std::string& directory_path);

} // namespace Filesystem
} // namespace PANGWES
