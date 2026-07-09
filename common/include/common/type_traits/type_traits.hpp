/*
 * type_traits.hpp - Compile-time traits.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cstddef>
#include <type_traits>
#include <utility>

namespace PANGWES {
namespace Traits {

// C++11 replacement for `std::enable_if_t`.
template <bool B, typename T = void>
using enable_if_t = typename std::enable_if<B, T>::type;

// C++11 replacement for `std::decay_t`.
template <typename T>
using decay_t = typename std::decay<T>::type;

// C++11 replacement for `std::underlying_type_t`.
template <typename Enum>
using underlying_type_t = typename std::underlying_type<Enum>::type;

// Converts an enum value to its underlying integer type. C++11 replacement for `std::to_underlying`.
template <typename Enum>
constexpr underlying_type_t<Enum> to_underlying(Enum enum_value) noexcept {
    static_assert(std::is_enum<Enum>::value, "to_underlying() must be called with an enum type");

    return static_cast<underlying_type_t<Enum>>(enum_value);
}

// Gives the element type of a container of type `T` based on `operator[]`.
template <typename T>
using element_type_t = decay_t<decltype(std::declval<T&>()[std::declval<std::size_t>()])>;

// Deduces if `T` is a container by the existence of functions `T::size`, `T::capacity` and `T::operator[]`.
template <typename T>
class is_container {
private:
    // Selected when `T::size` and `T::operator[]` exist.
    template <typename U>
    static decltype((void)std::declval<U&>().size(),
                    (void)std::declval<U&>().capacity(),
                    (void)std::declval<element_type_t<U>&>(),
                    std::true_type{}) test(int);

    // Fallback overload when the above substitution fails.
    template <typename>
    static std::false_type test(...);


public:
    // True if `T::size` and `T::operator[]` exist for `T`, false otherwise.
    static constexpr bool value = decltype(test<decay_t<T>>(0))::value;
};

// Checks for existence of a member function `T::reserved_bytes`.
template <typename T>
class has_reserved_bytes {
private:
    // Selected when `T::reserved_bytes` exists.
    template <typename U = decay_t<T>>
    static decltype((void)std::declval<U&>().reserved_bytes(), std::true_type{}) test(int);

    // Fallback overload when the above substitution fails.
    template <typename>
    static std::false_type test(...);


public:
    // True if `T::reserved_bytes` exists for `T`, false otherwise.
    static constexpr bool value = decltype(test<decay_t<T>>(0))::value;
};

// Checks if `T` has a valid hash operator.
template <typename T, typename Hash>
class has_hash_operator {
private:
    template <typename U = decay_t<T>, typename H = decay_t<Hash>>
    static decltype((void)static_cast<std::size_t>(std::declval<H&>()(std::declval<const U&>())),
                    std::true_type{}) test(int);

    template <typename, typename>
    static std::false_type test(...);

public:
    static constexpr bool value = std::is_default_constructible<decay_t<Hash>>::value &&
                                  decltype(test<decay_t<T>, decay_t<Hash>>(0))::value;
};

// Checks if `T` has equality operator.
template <typename T>
class has_equal_operator {
private:
    // Selected when `operator==` exists for `T` and the result is convertible to bool.
    template <typename U = decay_t<T>>
    static decltype((void)static_cast<bool>(std::declval<U&>() == std::declval<U&>()), std::true_type{}) test(int);

    // Fallback overload when the above substitution fails.
    template <typename>
    static std::false_type test(...);

public:
    // True if `T` has equality operator, false otherwise.
    static constexpr bool value = decltype(test<decay_t<T>>(0))::value;
};

} // namespace Traits
} // namespace PANGWES
