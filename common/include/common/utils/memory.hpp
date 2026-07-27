/*
 * memory.hpp - Memory-related utility functions.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cassert>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "common/type_traits/type_traits.hpp"

namespace PANGWES {
namespace Memory {

constexpr std::size_t MiB = 1024ULL * 1024;
constexpr std::size_t GiB = 1024ULL * 1024 * 1024;

// Makes a type T unique_ptr. C++11 replacement.
template <typename T, typename... Args>
inline std::unique_ptr<T> make_unique(Args&&... args) {
    static_assert(!std::is_array<T>::value, "Memory::make_unique does not support arrays");

    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

// Unreachable overload of `container_reserved_bytes()` to satisfy the compiler.
template <typename T>
inline std::size_t
container_reserved_bytes(const T& type,
    Traits::enable_if_t<!Traits::is_container<T>::value && !Traits::has_reserved_bytes<T>::value>* = nullptr) noexcept
{
    assert(0 && "unreachable");

    (void)type;

    return 0;
}

// Specialization of `container_reserved_bytes()` for non-container elements that provide `reserved_bytes()`.
template <typename T>
inline std::size_t
container_reserved_bytes(const T& type,
    Traits::enable_if_t<!Traits::is_container<T>::value && Traits::has_reserved_bytes<T>::value>* = nullptr) noexcept
{
    return type.reserved_bytes();
}

// Specialization of `container_reserved_bytes()` for std::vector<bool> (capacity() is in bits).
inline std::size_t container_reserved_bytes(const std::vector<bool>& container) noexcept {
    const auto bits = container.capacity();
    const auto size_per_block = sizeof(std::size_t) * std::numeric_limits<unsigned char>::digits;
    const auto blocks = (bits + size_per_block - 1) / size_per_block;

    return blocks * sizeof(std::size_t);
}

// Specialization of `container_reserved_bytes()` for containers that have `reserved_bytes()`.
template <typename Container>
inline std::size_t
container_reserved_bytes(const Container& container,
                         Traits::enable_if_t<Traits::is_container<Container>::value &&
                                             Traits::has_reserved_bytes<Container>::value>* = nullptr) noexcept
{
    return container.reserved_bytes();
}

// Returns the number of bytes of dynamic storage reserved by this container (excludes allocator overhead).
template <typename Container>
inline std::size_t
container_reserved_bytes(const Container& container,
                         Traits::enable_if_t<Traits::is_container<Container>::value &&
                                             !Traits::has_reserved_bytes<Container>::value>* = nullptr) noexcept
{
    using Element = Traits::element_type_t<Container>;

    // Count this container's reserved storage.
    std::size_t reserved_bytes = container.capacity() * sizeof(Element);

    // If elements themselves are containers, add their reserved storage recursively.
    if (Traits::is_container<Element>::value || Traits::has_reserved_bytes<Element>::value) {
        for (std::size_t i = 0; i < container.size(); ++i) {
            // Get element's reserved bytes via the correct overload.
            const auto element_reserved_bytes = container_reserved_bytes(container[i]);

            /*
             * If the element implemented `reserved_bytes()`, that also counts `sizeof(Element)`.
             * Since this is already included in the outer container storage, subtract it to avoid double counting.
            */
            const auto object_size_correction = Traits::has_reserved_bytes<Element>::value ? sizeof(Element) : 0;

            assert(element_reserved_bytes >= object_size_correction);

            reserved_bytes += element_reserved_bytes - object_size_correction;
        }
    }

    return reserved_bytes;
}

// Converts the given amount of bytes into megabytes.
inline std::size_t bytes_to_mebibytes(std::size_t bytes) noexcept {
    return bytes / 1024 / 1024;
}

/*
 * Clears `container` and releases its reserved memory with the swap trick.
 *
 * Swapping with a default-constructed container causes immediate destruction of the old contents.
 */
template <typename T>
void clear_and_release_reserved_memory(T& container) {
    T().swap(container);
}

} // namespace Memory
} // namespace PANGWES
