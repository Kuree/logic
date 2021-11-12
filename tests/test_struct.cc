#include <sstream>

#include "gtest/gtest.h"
#include "logic/struct.hh"

// packed array will be used as static slicing and updating values
struct test_a : logic::packed_struct<10> {
    template <typename T>
    static auto a(const T &v) {
        return v.template slice<4, 0>();
    }

    template <typename T, typename K = T>
    static auto a(const T &v, const K &k) {
        return v.template update<4, 0>(k);
    }

    template <typename T>
    static auto b(const T &v) {
        return v.template slice<9, 5>();
    }

    template <typename T, typename K = T>
    static auto b(const T &v, const K &k) {
        return v.template update<9, 5>(k);
    }
};

struct test_b : logic::unpacked_struct {
    logic::logic<12, 0> a;
    logic::logic<4, 0> b;
};

TEST(packed_struct_, ctor) {  // NOLINT
    using namespace logic::literals;
    test_a::type value;
    EXPECT_EQ(value.str(), "xxxxxxxxxx");
    value = (42_logic).to_unsigned();

    auto a_value = test_a::a(value);
    EXPECT_TRUE(a_value == 10_logic);
}

TEST(unpacked_struct_, ctor) {  // NOLINT
    using namespace logic::literals;
    test_b value;
    value.a = (42_logic).to_unsigned();

    EXPECT_TRUE(value.a == 42_logic);
}