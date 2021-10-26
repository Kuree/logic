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


TEST(logic, slice) {    // NOLINT
    {
        // integer holder
        logic::logic<40> l(42);
        auto s = l.slice<3, 2>();
        EXPECT_EQ(s.str(), "10");
    }

    {
        // big number holder slice small
        std::stringstream ss;
        auto constexpr ref = "1011";
        ss << ref;
        for (auto i = 0; i < 100; i++) ss << '0';
        logic::logic<120> l(ss.str());
        auto s = l.slice<103, 100>();
        EXPECT_EQ(ref, s.str());
    }

    {
        // big number holder slice big
        std::stringstream ss;
        auto constexpr ref = "1011";
        ss << ref;
        for (auto i = 0; i < 100; i++) ss << '1';
        logic::logic<120> l(ss.str());
        auto s = l.slice<103, 42>();
        ss = std::stringstream();
        ss << ref;
        for (auto i = 0; i < 100 -42; i++) ss << "1";
        auto result = ss.str();
        EXPECT_EQ(result, s.str());
    }
}


TEST(logic, literal) {  // NOLINT
    using namespace logic::literals;
    {
        auto v = 42_logic;
        EXPECT_EQ(v.value.value, 42ul);
    }
}

TEST(logic, concat) { // NOLINT
    {
        logic::logic<4> a(1);
        auto b = a.concat(logic::logic<4>(2), logic::logic<4>(3));
        EXPECT_EQ("000100100011", b.str());
    }
}

TEST(logic, mask) { // NOLINT
    {
        logic::logic<12> a("xx00zz");
        EXPECT_TRUE(a.x_mask.any_set());

    }
}