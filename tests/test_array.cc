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
}