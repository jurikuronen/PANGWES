/*
 * program_options_test_helpers.hpp - Shared helpers for testing ProgramOptions-derived option parsers.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>

#include "common/program_options/program_options_option_names.hpp"
#include "common/program_options/ProgramOptions.hpp"
#include "common/test_harness/Test.hpp"
#include "common/test_harness/test_type_traits.hpp"
#include "common/type_traits/type_traits.hpp"
#include "common/utils/Exception.hpp"
#include "mocks/MockArgumentsBuilder.hpp"

/*
 * Helper macro for creating an accessor lambda for a ProgramOptions getter.
 *
 * Before use, define:
 * - `ProgramOptionsT`: the concrete options type under test, deriving from ProgramOptions.
*/
#define OPTION_ACCESSOR(option_) \
    [](const ProgramOptionsT& options) -> decltype(std::declval<const ProgramOptionsT&>().option_()) { \
        return options.option_(); \
    }

/*
 * Helper macro for adding single-option ProgramOptions test cases.
 *
 * Before use, define:
 * - `ProgramOptionsT`: the concrete options type under test, deriving from ProgramOptions.
 * - `suite_name`: test name prefix.
*/
#define PROGRAM_OPTIONS_TEST(option_, OPTION_) \
    Test(std::string{suite_name} + "_" #option_, \
         generate_program_options_test<ProgramOptionsT, decltype(std::declval<const ProgramOptionsT&>().option_())>( \
             OPTION_##_OPTION, \
             OPTION_##_LONG_OPTION, \
             OPTION_ACCESSOR(option_)))

namespace PANGWES {

// Asserts that a tested Boolean option became enabled.
template <typename OptionType>
typename std::enable_if<TestTraits::is_bool<OptionType>::value, bool>::type
assert_program_options_option_value(const OptionType& option_value) {
    ASSERT_TRUE(option_value);
    return true;
}

// Asserts that a tested arithmetic option was assigned the mock arithmetic option value.
template <typename OptionType>
typename std::enable_if<!TestTraits::is_bool<OptionType>::value &&
                        std::is_arithmetic<OptionType>::value, bool>::type
assert_program_options_option_value(const OptionType& option_value) {
    ASSERT_EQUAL(option_value, Mocks::test_arithmetic_option_value);
    return true;
}

// Asserts that a tested string-like option was assigned the mock string option value.
template <typename OptionType>
typename std::enable_if<TestTraits::is_string_like<OptionType>::value, bool>::type
assert_program_options_option_value(const OptionType& option_value) {
    ASSERT_EQUAL(option_value, Mocks::test_string_option_value);
    return true;
}

// Fails the test due to a tested option having an unsupported type for `generate_program_options_test` helper.
template <typename OptionType>
typename std::enable_if<!TestTraits::is_bool<OptionType>::value &&
                        !std::is_arithmetic<OptionType>::value &&
                        !TestTraits::is_string_like<OptionType>::value, bool>::type
assert_program_options_option_value(const OptionType& option_value) {
    (void)option_value;
    EXIT_TEST_WITH_FAILURE("Unknown option value type");
    return false;
}

// Generates a test function to verify that an option is parsed correctly from both its short and long name.
template <typename OptionsType, typename OptionType>
TestFuncT generate_program_options_test(const std::string& option,
                                        const std::string& long_option,
                                        std::function<OptionType(const OptionsType&)> accessor)
{
    return [option, long_option, accessor]() -> bool {
        const auto check_option = [&accessor](const std::string& option_name) -> bool
        {
            auto test_arguments = Mocks::MockArgumentsBuilder<OptionType>(option_name);
            OptionsType options(test_arguments.argc(), test_arguments.argv());

            return assert_program_options_option_value(accessor(options));
        };

        return check_option(option) && check_option(long_option);
    };
}

// Test-only derived class that exposes `ProgramOptions` protected helpers.
class TestProgramOptions final : public ProgramOptions {
public:
    TestProgramOptions(int argc, char** argv)
    : ProgramOptions(argc, argv)
    { }

    char** argv_begin() const noexcept {
        return ProgramOptions::argv_begin();
    }

    char** argv_end() const noexcept {
        return ProgramOptions::argv_end();
    }

    char** argv_find(const std::string& program_option) const noexcept {
        return ProgramOptions::argv_find(program_option);
    }

    bool find_arg(const std::string& program_option, const std::string& program_long_option) const noexcept {
        return ProgramOptions::find_arg(program_option, program_long_option);
    }

    std::uint64_t read_unsigned_value(const std::string& program_option, const std::string& program_long_option) {
        return ProgramOptions::read_unsigned_value(program_option, program_long_option);
    }

    std::string read_string_value(const std::string& program_option, const std::string& program_long_option) {
        return ProgramOptions::read_string_value(program_option, program_long_option);
    }

    // Unused virtual function implementations.
    std::uint64_t k() const noexcept override final { return 0; }
    std::size_t n_threads() const noexcept override final { return 0; }
    std::size_t n_workers() const noexcept override final { return 0; }
    bool verbose() const noexcept override final { return true; }
    bool help_requested() const noexcept override final { return false; }
    bool version_requested() const noexcept override final { return false; }
    void print_run_details() const noexcept override final { }
    void print_help() const override final { }
};

// Generates a test to verify verbose is true by default.
template <typename ProgramOptionsT>
bool test_unit_program_options_verbose_by_default() {
    // Provide some empty argument in order for the constructor to not return early.
    Mocks::MockArgumentsBuilder<> test_arguments({ "" });
    ProgramOptionsT options(test_arguments.argc(), test_arguments.argv());

    ASSERT_TRUE(options.verbose());

    return true;
}

// Generates a test to verify that the quiet option sets verbose to false.
template <typename ProgramOptionsT>
bool test_unit_program_options_quiet_sets_verbose_false() {
    const auto check_quiet_option = [](const std::string& quiet_option) {
        Mocks::MockArgumentsBuilder<> test_arguments({ quiet_option.c_str() });
        ProgramOptionsT options(test_arguments.argc(), test_arguments.argv());

        ASSERT_FALSE(options.verbose());

        return true;
    };

    return check_quiet_option(QUIET_OPTION) && check_quiet_option(QUIET_LONG_OPTION);
}

/*
 * Generates a test to verify that passing `argument` for the given program option defaults to `expected_default_value`.
 *
 * Use like:
 *     test_unit_<project>_options_<option>_<argument>_defaults_to_<value> =
 *         test_unit_program_options_option_argument_defaults_to_value<ProgramOptionsT>(
 *             <option>_OPTION,
 *             <option>_LONG_OPTION,
 *             "<argument>",
 *             <expected_default_value>,
 *             OPTION_ACCESSOR(<option>)
 *         );
*/
template <typename ProgramOptionsT, typename DefaultValueT, typename AccessorT>
TestFuncT test_unit_program_options_option_argument_defaults_to_value(const std::string& option,
                                                                      const std::string& long_option,
                                                                      const std::string& argument,
                                                                      const DefaultValueT& expected_default_value,
                                                                      AccessorT accessor)
{
    return [option, long_option, argument, expected_default_value, accessor]() -> bool {
        const auto check_default_value =
            [&argument, &expected_default_value, &accessor](const std::string& option_name) -> bool {
                Mocks::MockArgumentsBuilder<> test_arguments({ option_name.c_str(), argument.c_str() });
                ProgramOptionsT options(test_arguments.argc(), test_arguments.argv());

                ASSERT_EQUAL(accessor(options), expected_default_value);

                return true;
            };

        return check_default_value(option) && check_default_value(long_option);
    };
}

/*
 * Generates a test to verify that passing a non-unsigned argument for the given unsigned program option throws.
 *
 * Use like:
 *     const auto test_unit_<project>_options_bad_arg_value_to_<option> =
 *         test_unit_program_options_bad_arg_value<ProgramOptionsT>(<option>_OPTION, <option>_LONG_OPTION);
*/
template <typename ProgramOptionsT>
TestFuncT test_unit_program_options_bad_arg_value(const std::string& option, const std::string& long_option) {
    return [option, long_option]() -> bool {
        const auto check_non_unsigned_arg = [](const std::string& option_name) -> bool {
            Mocks::MockArgumentsBuilder<> string_argument({ option_name.c_str(), "a" });
            EXPECT_THROW(ProgramOptionsT(string_argument.argc(),
                                         string_argument.argv()),
                         ErrorCode::INVALID_PROGRAM_OPTION);

            Mocks::MockArgumentsBuilder<> signed_argument({ option_name.c_str(), "-5" });
            EXPECT_THROW(ProgramOptionsT(signed_argument.argc(),
                                         signed_argument.argv()),
                         ErrorCode::INVALID_PROGRAM_OPTION);

            return true;
        };

        return check_non_unsigned_arg(option) && check_non_unsigned_arg(long_option);
    };
}

} // namespace PANGWES
