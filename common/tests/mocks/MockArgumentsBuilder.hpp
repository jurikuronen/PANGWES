/*
 * MockArgumentsBuilder.hpp - Constructs argc/argv for testing program_options/ProgramOptions.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <initializer_list>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "common/test_harness/test_type_traits.hpp"
#include "common/type_traits/type_traits.hpp"

namespace PANGWES {
namespace Mocks {

constexpr auto test_string_option_value = "test_string";
constexpr auto test_arithmetic_option_string = "42";
constexpr auto test_arithmetic_option_value = 42;

// Helper class for constructing argc/argv for ProgramOptions.
template <typename OptionType = int>
class MockArgumentsBuilder {
public:
    // Constructor for a single Boolean argument (option).
    template <typename T = OptionType>
    MockArgumentsBuilder(std::string option, Traits::enable_if_t<TestTraits::is_bool<T>::value>* = nullptr)
        : m_str_args{{ std::move(option) }},
          m_args{}
    { }

    // Constructor for a single arithmetic argument (option + value).
    template <typename T = OptionType>
    MockArgumentsBuilder(std::string option, Traits::enable_if_t<!TestTraits::is_bool<T>::value &&
                                                              std::is_arithmetic<T>::value>* = nullptr)
        : m_str_args{{ std::move(option), test_arithmetic_option_string }},
          m_args{}
    { }

    // Constructor for a single string argument (option + value).
    template <typename T = OptionType>
    MockArgumentsBuilder(std::string option, Traits::enable_if_t<TestTraits::is_string_like<T>::value>* = nullptr)
        : m_str_args{{ std::move(option), test_string_option_value }},
          m_args{}
    { }

    // General constructor for a list of arguments.
    MockArgumentsBuilder(const std::initializer_list<const char*>& args)
        : m_str_args{args.begin(), args.end()},
          m_args{}
    { }

    MockArgumentsBuilder() = delete;

    // Returns the `argc` value corresponding to the constructed argument list.
    int argc() const noexcept {
        return static_cast<int>(m_str_args.size() + 1);
    }

    /*
     * Returns the `argv` array corresponding to the constructed argument list. The pointers remain valid until this
     * object is destroyed.
    */
    char** argv() {
        if (m_args.empty()) {
            // +1 to reserve space for argv[0] (program name).
            m_args.resize(m_str_args.size() + 1);

            for (std::size_t i = 0; i < m_str_args.size(); ++i) {
                m_args[i + 1] = const_cast<char*>(m_str_args[i].c_str());
            }
        }

        return m_args.data();
    }

private:
    std::vector<std::string> m_str_args;
    std::vector<char*> m_args;
};

} // namespace Mocks
} // namespace PANGWES
