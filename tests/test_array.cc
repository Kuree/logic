#include <sstream>

#include "gtest/gtest.h"
#include "logic/array.hh"

TEST(array, ctor) {  // NOLINT
    logic::packed_array<logic::logic<15, 0>, 3, 0> logic_array;
    logic::packed_array<logic::bit<15, 0>, 3, 0> bit_array;
    EXPECT_EQ(sizeof(bit_array), 4 * 2);
    EXPECT_EQ(sizeof(logic_array), 4 * 4);
    std::stringstream ss;
    for (auto i = 0; i < 16 * 4; i++) ss << 'x';
    EXPECT_EQ(logic_array.str(), ss.str());
    ss = {};
    for (auto i = 0; i < 16 * 4; i++) ss << '0';
    EXPECT_EQ(bit_array.str(), ss.str());

    // multi-dim array
    logic::packed_array<logic::packed_array<logic::logic<15, 0>, 3, 0>, 1, 0> logic_arrays;
    EXPECT_EQ(sizeof(logic_arrays), 4 * 4 * 2);
}

TEST(array, slice) {    // NOLINT
    {
        // big number returns big number
        std::stringstream ss;
        for (auto i = 0; i < 16 * 10; i++) ss << '1';
        logic::packed_array<logic::logic<15, 0>, 9, 0> logic_array{ss.str()};
        auto s = logic_array.slice<4, 0>();
        EXPECT_EQ(s.size, 16 * 5);
        EXPECT_EQ(s.str(), ss.str().substr(16 * 10 - 16 * 5));
    }

    {
        // big number returns small number
        std::stringstream ss;
        for (auto i = 0; i < 16 * 10; i++) ss << '1';
        logic::packed_array<logic::logic<15, 0>, 9, 0> logic_array{ss.str()};
        auto s = logic_array.slice<2, 0>();
        EXPECT_EQ(s.size, 16 * 3);
        EXPECT_EQ(s.str(), ss.str().substr(16 * 10 - 16 * 3));
    }
}

TEST(array, update) {   // NOLINT
    using namespace logic::literals;
    {
        logic::packed_array<logic::logic<15, 0>, 9, 0> logic_array;
        logic_array.update<1, 0>(0_logic);
        std::stringstream ss;
        for (auto i = 0; i < 16 * (10 - 2); i++) ss << 'x';
        for (auto i = 0; i < 16 * 2; i++) ss << '0';
        EXPECT_EQ(ss.str(), logic_array.str());
    }
}