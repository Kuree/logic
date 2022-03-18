#include <sstream>

#include "gtest/gtest.h"
#include "logic/logic.hh"

TEST(regression, boolean) {  // NOLINT
    auto a = logic::bit<5>(63);
    auto b = a.get<3>();
    bool correct;
    if ((b == (logic::bit<0>(1)))) {    // NOLINT
        correct = true;
    } else {
        correct = false;
    }
    EXPECT_TRUE(correct);
}