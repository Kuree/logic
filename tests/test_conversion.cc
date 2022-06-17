#include <sstream>

#include "gtest/gtest.h"
#include "logic/logic.hh"

TEST(bit2logic, lt) {
    logic::bit<31, 0> a(0);
    logic::logic<31, 0> b(10);
    EXPECT_TRUE(a < b);
}

TEST(bit2logic, gt) {
    logic::bit<31, 0> a(0);
    logic::logic<31, 0> b(10);
    EXPECT_FALSE(a > b);
}

TEST(bit2logic, le) {
    logic::bit<31, 0> a(0);
    logic::logic<31, 0> b(10);
    EXPECT_TRUE(a <= b);
}

TEST(bit2logic, ge) {
    logic::bit<31, 0> a(0);
    logic::logic<31, 0> b(10);
    EXPECT_FALSE(a >= b);
}
