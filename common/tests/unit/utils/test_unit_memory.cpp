/*
 * test_unit_memory.cpp - Unit tests for memory-related utility functions defined in utils/memory.hpp.
 *
 * MIT License (see LICENSE in the repository root).
 * Copyright (c) 2020-2026 Juri Kuronen
*/
#include <cstdint>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include "common/test_harness/Test.hpp"
#include "common/type_traits/type_traits.hpp"
#include "common/utils/memory.hpp"

namespace PANGWES {
namespace {

// Global counters to verify RAII behavior for `Memory::make_unique`.
auto make_unique_constructions = 0;
auto make_unique_destructions = 0;

// Structure that tracks constructions/destructions.
struct MakeUniqueRAII {
    int value;

    // Constructor that records that an instance was constructed.
    explicit MakeUniqueRAII(int val)
        : value{val}
    {
        ++make_unique_constructions;
    }

    // Destructor that records that an instance was destructed.
    virtual ~MakeUniqueRAII() {
        ++make_unique_destructions;
    }
};

// Structure used to check forwarding behavior with `Memory::make_unique`.
struct MakeUniqueForwarding : public MakeUniqueRAII {
    bool lvalue;
    bool rvalue;

    // Constructor for lvalues.
    MakeUniqueForwarding(const int& val)
        : MakeUniqueRAII(val),
          lvalue{true},
          rvalue{false}
    { }

    // Constructor for rvalues.
    MakeUniqueForwarding(int&& val)
        : MakeUniqueRAII(val),
          lvalue{false},
          rvalue{true}
    { }
};

bool test_unit_memory_make_unique() {
    const auto ptr1 = Memory::make_unique<int>(42);

    ASSERT_TRUE(static_cast<bool>(ptr1));
    ASSERT_EQUAL(*ptr1, 42);

    const auto ptr2 = Memory::make_unique<std::vector<int>>(1, 10);

    ASSERT_TRUE(static_cast<bool>(ptr2));
    ASSERT_EQUAL(ptr2->size(), 1);
    ASSERT_EQUAL(ptr2->front(), 10);

    return true;
}

bool test_unit_memory_make_unique_forwarding() {
    const int value = 1;
    const auto ptr1 = Memory::make_unique<MakeUniqueForwarding>(value);
    const auto ptr2 = Memory::make_unique<MakeUniqueForwarding>(2);

    ASSERT_TRUE(ptr1->lvalue);
    ASSERT_FALSE(ptr1->rvalue);
    ASSERT_FALSE(ptr2->lvalue);
    ASSERT_TRUE(ptr2->rvalue);

    const std::unique_ptr<MakeUniqueRAII> ptr = Memory::make_unique<MakeUniqueForwarding>(3);
    ASSERT_EQUAL(ptr->value, 3);

    return true;
}

bool test_unit_memory_make_unique_RAII() {
    // Reset global counters.
    make_unique_constructions = 0;
    make_unique_destructions = 0;

    {
        /*
         * Create an object and immediately mover ownership to another unique_ptr.
         *
         * After the move, the source pointer should be empty, the destination pointer should own the object, and when
         * the destination pointer goes out of scope the object should be destroyed exactly once.
        */
        auto ptr1 = Memory::make_unique<MakeUniqueRAII>(0);
        const auto ptr2 = std::move(ptr1);

        ASSERT_FALSE(static_cast<bool>(ptr1));
        ASSERT_TRUE(static_cast<bool>(ptr2));
    }

    ASSERT_EQUAL(make_unique_constructions, make_unique_destructions);
    ASSERT_EQUAL(make_unique_constructions, 1);

    {
        // Destruct through a base-class unique_ptr.
        const std::unique_ptr<MakeUniqueRAII> ptr = Memory::make_unique<MakeUniqueForwarding>(0);
    }

    ASSERT_EQUAL(make_unique_constructions, make_unique_destructions);
    ASSERT_EQUAL(make_unique_constructions, 2);

    return true;
}

// Structure for testing `Memory::container_reserved_bytes`.
struct TestStruct {
    int64_t a;
    std::size_t b;
    bool c;
};

// Global counter to verify `Memory::container_reserved_bytes` is called.
auto reserved_bytes_called = 0;

// Structure for testing `Memory::container_reserved_bytes` for containers that have `reserved_bytes()`.
struct TestStructWithReservedBytes {
    int64_t a;
    std::size_t b;
    bool c;

    std::size_t reserved_bytes() const noexcept {
        ++reserved_bytes_called;

        return sizeof(*this);
    }

    // Provided for `Traits::is_container` to fake that this is a container.
    std::size_t size() const;
    std::size_t capacity() const;
    int& operator[](std::size_t idx);
};

template <typename Container>
bool check_unit_memory_container_reserved_bytes(const Container& container, std::size_t expected_reserved_bytes) {
    static_assert(Traits::is_container<Container>::value, "must be a container");

    ASSERT_EQUAL(Memory::container_reserved_bytes(container), expected_reserved_bytes);

    return true;
}

bool test_unit_memory_container_reserved_bytes_vector_int() {
    const std::vector<int> vector{1, 2, 3, 4, 5};
    const auto expected_reserved_bytes = vector.capacity() * sizeof(int);

    return check_unit_memory_container_reserved_bytes(vector, expected_reserved_bytes);
}

bool test_unit_memory_container_reserved_bytes_vector_bool() {
    const auto bits_per_block = sizeof(std::size_t) * std::numeric_limits<unsigned char>::digits;
    // Set vector size to reserve two blocks worth of bits of capacity.
    const std::vector<bool> vector_bool(bits_per_block + 1);

    // Each block takes `std::size_t` memory, and we should have two blocks reserved.
    ASSERT_EQUAL(Memory::container_reserved_bytes(vector_bool), 2 * sizeof(std::size_t));

    return true;
}

bool test_unit_memory_container_reserved_bytes_vector_int_pair() {
    const std::vector<std::pair<int, int>> vector{{1, 2}, {3, 4}, {5, 6}};
    const auto expected_reserved_bytes = vector.capacity() * sizeof(std::pair<int, int>);

    return check_unit_memory_container_reserved_bytes(vector, expected_reserved_bytes);
}

bool test_unit_memory_container_reserved_bytes_vector_struct() {
    const std::vector<TestStruct> vector{TestStruct{1, 2, true}, TestStruct{3, 4, false}, TestStruct{5, 6, false}};
    const auto expected_reserved_bytes = vector.capacity() * sizeof(TestStruct);

    return check_unit_memory_container_reserved_bytes(vector, expected_reserved_bytes);
}

bool test_unit_memory_container_reserved_bytes_vector_struct_with_reserved_bytes() {
    const std::vector<TestStruct> vector{TestStruct{1, 2, true}, TestStruct{3, 4, false}, TestStruct{5, 6, false}};
    const std::vector<TestStructWithReservedBytes> vector2{
        TestStructWithReservedBytes{1, 2, true},
        TestStructWithReservedBytes{3, 4, false},
        TestStructWithReservedBytes{5, 6, false}
    };

    reserved_bytes_called = 0;

    ASSERT_EQUAL(Memory::container_reserved_bytes(vector), Memory::container_reserved_bytes(vector2));

    ASSERT_EQUAL(reserved_bytes_called, vector2.size());

    return true;
}

bool test_unit_memory_container_reserved_bytes_vector_int_reserved_only() {
    std::vector<int> vector;

    vector.reserve(100);

    const auto expected_reserved_bytes = vector.capacity() * sizeof(int);

    return check_unit_memory_container_reserved_bytes(vector, expected_reserved_bytes);
}

bool test_unit_memory_container_reserved_bytes_vector2d_int() {
    const std::vector<std::vector<int>> vector_2d{
        std::vector<int>{1, 2, 3},
        std::vector<int>{1, 2, 3, 4, 5},
        std::vector<int>{},
    };

    auto expected_reserved_bytes = vector_2d.capacity() * sizeof(std::vector<int>);
    for (std::size_t i = 0; i < vector_2d.size(); ++i) {
        expected_reserved_bytes += vector_2d[i].capacity() * sizeof(int);
    }

    return check_unit_memory_container_reserved_bytes(vector_2d, expected_reserved_bytes);
}

bool test_unit_memory_container_reserved_bytes_vector2d_struct_with_reserved_bytes() {
    using T = TestStructWithReservedBytes;
    const std::vector<std::vector<T>> vector{
        std::vector<T>{T{1, 2, true}},
        std::vector<T>{},
        std::vector<T>{T{1, 1, true}, T{2, 2, true}, T{3, 3, true}}
    };

    auto expected_reserved_bytes = vector.capacity() * sizeof(std::vector<T>);
    for (std::size_t i = 0; i < vector.size(); ++i) {
        expected_reserved_bytes += vector[i].capacity() * sizeof(T);
    }

    reserved_bytes_called = 0;

    ASSERT_EQUAL(Memory::container_reserved_bytes(vector), expected_reserved_bytes);

    // Called four times, for the four inner TestStructWithReservedBytes structures.
    ASSERT_EQUAL(reserved_bytes_called, 4);

    return true;
}

bool test_unit_memory_container_reserved_bytes_vector3d_int() {
    const std::vector<std::vector<std::vector<int>>> vector_3d{
        std::vector<std::vector<int>>{std::vector<int>{}, std::vector<int>{1}, std::vector<int>{2, 3}},
        std::vector<std::vector<int>>{std::vector<int>{1, 2, 3, 4, 5}},
        std::vector<std::vector<int>>{std::vector<int>{}, std::vector<int>{}},
    };

    auto expected_reserved_bytes = vector_3d.capacity() * sizeof(std::vector<std::vector<int>>);
    for (std::size_t i = 0; i < vector_3d.size(); ++i) {
        const auto& vector_2d = vector_3d[i];

        expected_reserved_bytes += vector_2d.capacity() * sizeof(std::vector<int>);
        for (std::size_t j = 0; j < vector_2d.size(); ++j) {
            expected_reserved_bytes += vector_2d[j].capacity() * sizeof(int);
        }
    }

    return check_unit_memory_container_reserved_bytes(vector_3d, expected_reserved_bytes);
}

bool test_unit_memory_clear_and_release_reserved_memory_vector_int() {
    std::vector<int> vector;

    vector.reserve(1000);
    vector.push_back(1);
    vector.push_back(2);

    ASSERT_EQUAL(vector.size(), 2);
    ASSERT_GREATER(Memory::container_reserved_bytes(vector), 0);

    Memory::clear_and_release_reserved_memory(vector);

    ASSERT_EQUAL(vector.size(), 0);
    ASSERT_EQUAL(Memory::container_reserved_bytes(vector), 0);

    return true;
}

bool test_unit_memory_clear_and_release_reserved_memory_string() {
    std::string str{};

    const auto default_reserved_bytes = Memory::container_reserved_bytes(str);

    str.reserve(1000);
    str += "test";

    ASSERT_EQUAL(str.size(), 4);
    ASSERT_GREATER(Memory::container_reserved_bytes(str), default_reserved_bytes);

    Memory::clear_and_release_reserved_memory(str);

    ASSERT_EQUAL(str.size(), 0);
    ASSERT_EQUAL(Memory::container_reserved_bytes(str), default_reserved_bytes);

    return true;
}

bool test_unit_memory_clear_and_release_reserved_memory_vector2d_int() {
    std::vector<std::vector<int>> vector_2d;

    vector_2d.reserve(1000);
    vector_2d.push_back({});
    vector_2d.push_back({});
    vector_2d[0].reserve(1000);
    vector_2d[1].reserve(1000);

    ASSERT_GREATER(Memory::container_reserved_bytes(vector_2d), 0);
    ASSERT_EQUAL(vector_2d.size(), 2);
    ASSERT_GREATER(Memory::container_reserved_bytes(vector_2d[0]), 0);
    ASSERT_GREATER(Memory::container_reserved_bytes(vector_2d[1]), 0);

    Memory::clear_and_release_reserved_memory(vector_2d[0]);

    ASSERT_EQUAL(Memory::container_reserved_bytes(vector_2d[0]), 0);
    ASSERT_GREATER(Memory::container_reserved_bytes(vector_2d[1]), 0);

    Memory::clear_and_release_reserved_memory(vector_2d);

    ASSERT_EQUAL(Memory::container_reserved_bytes(vector_2d), 0);
    ASSERT_EQUAL(vector_2d.size(), 0);

    return true;
}

bool test_unit_memory_bytes_to_mebibytes() {
    using Memory::bytes_to_mebibytes;

    const int64_t MB_bits = 1024LL * 1024LL;
    const int64_t GB_bits = MB_bits * 1024LL;

    for (auto n_megabytes = 1; n_megabytes < 10; ++n_megabytes) {
        ASSERT_EQUAL(bytes_to_mebibytes(n_megabytes * MB_bits - 1), n_megabytes - 1);
        ASSERT_EQUAL(bytes_to_mebibytes(n_megabytes * MB_bits), n_megabytes);
        ASSERT_EQUAL(bytes_to_mebibytes(n_megabytes * MB_bits + MB_bits - 1), n_megabytes);

        ASSERT_EQUAL(bytes_to_mebibytes(n_megabytes * GB_bits - MB_bits), n_megabytes * 1024 - 1);
        ASSERT_EQUAL(bytes_to_mebibytes(n_megabytes * GB_bits), n_megabytes * 1024);
        ASSERT_EQUAL(bytes_to_mebibytes(n_megabytes * GB_bits + MB_bits), n_megabytes * 1024 + 1);
    }

    return true;
}

} // namespace
} // namespace PANGWES

int main() {
    using namespace PANGWES;

    const auto tests = {
        TEST(test_unit_memory_make_unique),
        TEST(test_unit_memory_make_unique_forwarding),
        TEST(test_unit_memory_make_unique_RAII),
        TEST(test_unit_memory_container_reserved_bytes_vector_int),
        TEST(test_unit_memory_container_reserved_bytes_vector_bool),
        TEST(test_unit_memory_container_reserved_bytes_vector_int_pair),
        TEST(test_unit_memory_container_reserved_bytes_vector_struct),
        TEST(test_unit_memory_container_reserved_bytes_vector_struct_with_reserved_bytes),
        TEST(test_unit_memory_container_reserved_bytes_vector_int_reserved_only),
        TEST(test_unit_memory_container_reserved_bytes_vector2d_int),
        TEST(test_unit_memory_container_reserved_bytes_vector2d_struct_with_reserved_bytes),
        TEST(test_unit_memory_container_reserved_bytes_vector3d_int),
        TEST(test_unit_memory_clear_and_release_reserved_memory_vector_int),
        TEST(test_unit_memory_clear_and_release_reserved_memory_string),
        TEST(test_unit_memory_clear_and_release_reserved_memory_vector2d_int),
        TEST(test_unit_memory_bytes_to_mebibytes),
    };

    return Test::run_suite("test_unit_memory", tests);
}
