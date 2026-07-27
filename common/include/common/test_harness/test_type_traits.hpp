/*
 * test_type_traits.hpp - Additional compile-time traits used by the test harness.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>

#include "common/type_traits/type_traits.hpp"

namespace PANGWES {
namespace TestTraits {

// Type alias that is true for Boolean types.
template <typename T>
using is_bool = std::is_same<T, bool>;

// Type alias that is true for string-like types.
template <typename T>
using is_string_like = std::is_convertible<T, std::string>;

// Enable ADL to find std::to_string.
using std::to_string;

// Checks for the existence of a free function to_string(T).
template <typename T>
class has_to_string {
private:
    // Selected when `to_string(T)` is a valid expression.
    template <typename U = T>
    static decltype((void)to_string(std::declval<const U&>()), std::true_type{}) test(U);

    // Fallback overload when the above substitution fails.
    template <typename>
    static std::false_type test(...);

public:
    // True if `to_string(T)` is a valid expression, false otherwise.
    static constexpr bool value = decltype(test<T>(std::declval<T>()))::value;
};

} // namespace TestTraits
} // namespace PANGWES
