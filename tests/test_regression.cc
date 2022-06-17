#include <sstream>

#include "gtest/gtest.h"
#include "logic/logic.hh"

TEST(regression, boolean) {  // NOLINT
    logic::logic<5, 0, 0> a = logic::bit<5>(63);
    logic::logic<> b = a.get<3>();
    bool correct;
    if ((b == (logic::bit<0>(1)))) {  // NOLINT
        correct = true;
    } else {
        correct = false;
    }
    EXPECT_TRUE(correct);
}