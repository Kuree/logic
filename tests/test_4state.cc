#include "logic/logic.hh"
#include "gtest/gtest.h"
#include <sstream>

TEST(logic, init) { // NOLINT
    {
        // uin8_t holder
        logic::logic<4> l;
        auto s = l.str();
        EXPECT_EQ(s, "xxxx");
    }
    {
        // uint16_t holder
        logic::logic<15> l;
        auto s = l.str();
        EXPECT_EQ(s, "xxxxxxxxxxxxxxx");
    }
    {
        // uint32_t holder
        logic::logic<30> l(42);
        auto s = l.str();
        EXPECT_EQ(s, "000000000000000000000000101010");
    }

    {
        // uint64_t holder
        logic::logic<64> l(std::numeric_limits<uint64_t>::max());
        auto s = l.str();
        std::stringstream ss;
        for (auto i = 0; i < 64; i++) ss << '1';
        EXPECT_EQ(s, ss.str());
    }

    {
        // uint64_t holder with str initializer
        constexpr std::string_view v = "xx1010";
        logic::logic<40> l(v);
        auto s = l.str();
        std::stringstream ss;
        for (auto i = 0; i < 40 - v.size(); i++) ss << '0';
        ss << v;
        EXPECT_EQ(s, ss.str());
    }

    {
        // big number holder
        // uint64_t holder with str initializer
        constexpr std::string_view v = "xx1010";
        logic::logic<128> l(v);
        auto s = l.str();
        std::stringstream ss;
        for (auto i = 0; i < 128 - v.size(); i++) ss << '0';
        ss << v;
        EXPECT_EQ(s, ss.str());
    }
}
