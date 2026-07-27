/*
 * test_unit_type_traits.cpp - Unit tests for type_traits/type_traits.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "common/test_harness/Test.hpp"
#include "common/test_harness/test_type_traits.hpp"
#include "common/type_traits/type_traits.hpp"

namespace PANGWES {
namespace {

// Helper structure for testing `Traits::enable_if_t`.
template <typename T>
struct TestEnableIfTStruct {
    bool bool_constructor_called = false;

    // Constructor selected when `T` is a Boolean.
    template <typename U = T>
    TestEnableIfTStruct(Traits::enable_if_t<TestTraits::is_bool<U>::value>* = nullptr) : bool_constructor_called{true}
    { }

    // Constructor selected when `T` is not a Boolean.
    template <typename U = T>
    TestEnableIfTStruct(Traits::enable_if_t<!TestTraits::is_bool<U>::value>* = nullptr) : bool_constructor_called{false}
    { }

};

bool test_unit_type_traits_enable_if_t() {
    const TestEnableIfTStruct<bool> bool_struct{};
    const TestEnableIfTStruct<int> non_bool_struct{};

    ASSERT_TRUE(bool_struct.bool_constructor_called);
    ASSERT_FALSE(non_bool_struct.bool_constructor_called);

    return true;
}

bool test_unit_type_traits_decay_t() {
    int int_var = 0;
    const int const_int_var = 1;
    int& int_ref_var = int_var;
    const int& const_int_ref_var = const_int_var;

    ASSERT_TRUE((std::is_same<Traits::decay_t<decltype(int_var)>, int>::value));
    ASSERT_TRUE((std::is_same<Traits::decay_t<decltype(const_int_var)>, int>::value));
    ASSERT_TRUE((std::is_same<Traits::decay_t<decltype(int_ref_var)>, int>::value));
    ASSERT_TRUE((std::is_same<Traits::decay_t<decltype(const_int_ref_var)>, int>::value));

    return true;
}

bool test_unit_type_traits_underlying_type() {
    enum class Test : std::uint8_t { A = 0, B = 1 };

    ASSERT_TRUE((std::is_same<Traits::underlying_type_t<Test>, std::uint8_t>::value));

    Test test{Test::B};

    ASSERT_EQUAL(Traits::to_underlying(test), 1);

    return true;
}

bool test_unit_type_traits_element_type_t() {
    std::vector<int> vector_int;
    const std::vector<int> const_vector_int;
    std::vector<int>& vector_int_ref = vector_int;
    const std::vector<int>& const_vector_int_ref = const_vector_int;

    ASSERT_TRUE((std::is_same<Traits::element_type_t<decltype(vector_int)>, int>::value));
    ASSERT_TRUE((std::is_same<Traits::element_type_t<decltype(const_vector_int)>, int>::value));
    ASSERT_TRUE((std::is_same<Traits::element_type_t<decltype(vector_int_ref)>, int>::value));
    ASSERT_TRUE((std::is_same<Traits::element_type_t<decltype(const_vector_int_ref)>, int>::value));

    return true;
}

// Macros for generating structures for testing `Traits::is_container`.
#define SIZE_NC std::size_t size();
#define SIZE_C std::size_t size() const;
#define SIZE_BOTH SIZE_NC SIZE_C
#define CAPACITY_NC std::size_t capacity();
#define CAPACITY_C std::size_t capacity() const;
#define CAPACITY_BOTH CAPACITY_NC CAPACITY_C
#define OPERATOR_NC int operator[](std::size_t);
#define OPERATOR_C int operator[](std::size_t) const;
#define OPERATOR_BOTH OPERATOR_NC OPERATOR_C
#define TESTSTRUCT(name_, size_macro_, capacity_macro_, operator_macro_) \
    struct name_ { size_macro_ capacity_macro_ operator_macro_ }

#define IS_CONTAINER_TESTS(struct_, verdict_) \
    ASSERT_##verdict_(!!Traits::is_container<const struct_>::value); \
    ASSERT_##verdict_(!!Traits::is_container<const struct_&>::value); \
    ASSERT_##verdict_(!!Traits::is_container<const struct_&&>::value); \
    ASSERT_##verdict_(!!Traits::is_container<struct_>::value); \
    ASSERT_##verdict_(!!Traits::is_container<struct_&>::value); \
    ASSERT_##verdict_(!!Traits::is_container<struct_&&>::value)

#define EMPTY

bool test_unit_type_traits_is_container() {
    TESTSTRUCT(nc_nc_nc, SIZE_NC, CAPACITY_NC, OPERATOR_NC);
    TESTSTRUCT(c_nc_nc, SIZE_C, CAPACITY_NC, OPERATOR_NC);
    TESTSTRUCT(nc_c_nc, SIZE_NC, CAPACITY_C, OPERATOR_NC);
    TESTSTRUCT(nc_nc_c, SIZE_NC, CAPACITY_NC, OPERATOR_C);
    TESTSTRUCT(c_c_nc, SIZE_C, CAPACITY_C, OPERATOR_NC);
    TESTSTRUCT(nc_c_c, SIZE_NC, CAPACITY_C, OPERATOR_C);
    TESTSTRUCT(c_nc_c, SIZE_C, CAPACITY_NC, OPERATOR_C);
    TESTSTRUCT(c_c_c, SIZE_C, CAPACITY_C, OPERATOR_C);
    TESTSTRUCT(b_b_b, SIZE_BOTH, CAPACITY_BOTH, OPERATOR_BOTH);

    // Cases where all required functions are provided (`is_container` true).
    IS_CONTAINER_TESTS(nc_nc_nc, TRUE);
    IS_CONTAINER_TESTS(c_nc_nc, TRUE);
    IS_CONTAINER_TESTS(nc_c_nc, TRUE);
    IS_CONTAINER_TESTS(nc_nc_c, TRUE);
    IS_CONTAINER_TESTS(c_c_nc, TRUE);
    IS_CONTAINER_TESTS(nc_c_c, TRUE);
    IS_CONTAINER_TESTS(c_nc_c, TRUE);
    IS_CONTAINER_TESTS(c_c_c, TRUE);
    IS_CONTAINER_TESTS(b_b_b, TRUE);

    // Cases where one function is missing (`is_container` false).
    TESTSTRUCT(no_nc_nc, EMPTY, CAPACITY_NC, OPERATOR_NC);
    TESTSTRUCT(nc_no_nc, SIZE_NC, EMPTY, OPERATOR_NC);
    TESTSTRUCT(nc_nc_no, SIZE_NC, CAPACITY_NC, EMPTY);

    IS_CONTAINER_TESTS(no_nc_nc, FALSE);
    IS_CONTAINER_TESTS(nc_no_nc, FALSE);
    IS_CONTAINER_TESTS(nc_nc_no, FALSE);

    // Case where the struct provides none of the required functions (`is_container` false).
    TESTSTRUCT(nothing, EMPTY, EMPTY, EMPTY);

    IS_CONTAINER_TESTS(nothing, FALSE);

    return true;
}

#undef IS_CONTAINER_TESTS
#undef TESTSTRUCT

#define RESERVED_BYTES_NC std::size_t reserved_bytes();
#define RESERVED_BYTES_C std::size_t reserved_bytes() const;
#define RESERVED_BYTES_BOTH RESERVED_BYTES_NC RESERVED_BYTES_C
#define TESTSTRUCT(name_, reserved_bytes_macro_) \
    struct name_ { reserved_bytes_macro_ }

#define HAS_RESERVED_BYTES_TESTS(struct_, verdict_) \
    ASSERT_##verdict_(!!Traits::has_reserved_bytes<const struct_>::value); \
    ASSERT_##verdict_(!!Traits::has_reserved_bytes<const struct_&>::value); \
    ASSERT_##verdict_(!!Traits::has_reserved_bytes<const struct_&&>::value); \
    ASSERT_##verdict_(!!Traits::has_reserved_bytes<struct_>::value); \
    ASSERT_##verdict_(!!Traits::has_reserved_bytes<struct_&>::value); \
    ASSERT_##verdict_(!!Traits::has_reserved_bytes<struct_&&>::value)

bool test_unit_type_traits_has_reserved_bytes() {
    TESTSTRUCT(nc, RESERVED_BYTES_NC);
    TESTSTRUCT(c, RESERVED_BYTES_C);
    TESTSTRUCT(both, RESERVED_BYTES_BOTH);
    TESTSTRUCT(nothing, EMPTY);

    HAS_RESERVED_BYTES_TESTS(nc, TRUE);
    HAS_RESERVED_BYTES_TESTS(c, TRUE);
    HAS_RESERVED_BYTES_TESTS(both, TRUE);
    HAS_RESERVED_BYTES_TESTS(nothing, FALSE);

    return true;
}

#undef HAS_RESERVED_BYTES_TESTS
#undef TESTSTRUCT

#define HAS_HASH_OPERATOR_TESTS(type_, hash_, verdict_) \
    ASSERT_##verdict_((!!Traits::has_hash_operator<const type_, const hash_>::value)); \
    ASSERT_##verdict_((!!Traits::has_hash_operator<const type_&, const hash_&>::value)); \
    ASSERT_##verdict_((!!Traits::has_hash_operator<const type_&&, const hash_&&>::value)); \
    ASSERT_##verdict_((!!Traits::has_hash_operator<type_, hash_>::value)); \
    ASSERT_##verdict_((!!Traits::has_hash_operator<type_&, hash_&>::value)); \
    ASSERT_##verdict_((!!Traits::has_hash_operator<type_&&, hash_&&>::value))

bool test_unit_type_traits_has_hash_operator() {
    struct test_struct { };
    struct valid_hash {
        std::size_t operator()(const test_struct&) const { return 0; }
    };
    struct valid_int_hash {
        int operator()(const test_struct&) const { return 0; }
    };
    struct invalid_hash {
        std::size_t operator()(int) const { return 0; }
    };
    struct invalid_void_hash {
        void operator()(const test_struct&) const { }
    };
    struct invalid_non_default_constructible_hash {
        explicit invalid_non_default_constructible_hash(int) { }
        std::size_t operator()(const test_struct&) const { return 0; }
    };

    HAS_HASH_OPERATOR_TESTS(test_struct, valid_hash, TRUE);
    HAS_HASH_OPERATOR_TESTS(test_struct, valid_int_hash, TRUE);
    HAS_HASH_OPERATOR_TESTS(test_struct, test_struct, FALSE);
    HAS_HASH_OPERATOR_TESTS(test_struct, invalid_hash, FALSE);
    HAS_HASH_OPERATOR_TESTS(test_struct, invalid_void_hash, FALSE);
    HAS_HASH_OPERATOR_TESTS(test_struct, invalid_non_default_constructible_hash, FALSE);

    return true;
}

#undef HAS_HASH_OPERATOR_TESTS

#define HAS_EQUAL_OPERATOR_TESTS(type_, verdict_) \
    ASSERT_##verdict_(!!Traits::has_equal_operator<const type_>::value); \
    ASSERT_##verdict_(!!Traits::has_equal_operator<const type_&>::value); \
    ASSERT_##verdict_(!!Traits::has_equal_operator<const type_&&>::value); \
    ASSERT_##verdict_(!!Traits::has_equal_operator<type_>::value); \
    ASSERT_##verdict_(!!Traits::has_equal_operator<type_&>::value); \
    ASSERT_##verdict_(!!Traits::has_equal_operator<type_&&>::value)

bool test_unit_type_traits_has_equal_operator() {
    struct struct_has_bool_equal {
        bool operator==(const struct_has_bool_equal&) const { return true; }
    };
    struct struct_has_int_equal {
        int operator==(const struct_has_int_equal&) const { return 1; }
    };
    struct test_struct { };
    struct struct_with_non_bool_convertible_equal {
        test_struct operator==(const struct_with_non_bool_convertible_equal&) const {
            return test_struct{};
        }
    };

    HAS_EQUAL_OPERATOR_TESTS(struct_has_bool_equal, TRUE);
    HAS_EQUAL_OPERATOR_TESTS(struct_has_int_equal, TRUE);
    HAS_EQUAL_OPERATOR_TESTS(test_struct, FALSE);
    HAS_EQUAL_OPERATOR_TESTS(struct_with_non_bool_convertible_equal, FALSE);

    return true;
}

#undef HAS_EQUAL_OPERATOR_TESTS
#undef EMPTY

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_unit_type_traits_enable_if_t),
        TEST(test_unit_type_traits_decay_t),
        TEST(test_unit_type_traits_underlying_type),
        TEST(test_unit_type_traits_element_type_t),
        TEST(test_unit_type_traits_is_container),
        TEST(test_unit_type_traits_has_reserved_bytes),
        TEST(test_unit_type_traits_has_hash_operator),
        TEST(test_unit_type_traits_has_equal_operator),
    };

    return Test::run_suite("test_unit_type_traits", tests);
}
