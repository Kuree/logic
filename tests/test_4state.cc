#include "logic/logic.hh"
#include "gtest/gtest.h"
#include <sstream>

TEST(logic, size) { // NOLINT
    EXPECT_EQ(sizeof(logic::logic<4>), 1 * 2);
    EXPECT_EQ(sizeof(logic::logic<8>), 1 * 2);
    EXPECT_EQ(sizeof(logic::logic<64>), 8 * 2);
    EXPECT_EQ(sizeof(logic::logic<100>), 16 * 2);
    EXPECT_EQ(sizeof(logic::logic<128>), 16 * 2);
}

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
        for (auto i = 0u; i < 40u - v.size(); i++) ss << '0';
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
        for (auto i = 0u; i < 128u - v.size(); i++) ss << '0';
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
        EXPECT_TRUE(a.xz_mask.any_set());
        EXPECT_NE(a.value.value, 0);
    }
}

TEST(logic, unpack) {   // NOLINT
    {
        logic::logic<4> a, b, c;
        auto d = logic::logic<12>("000100100011");
        d.unpack(a, b, c);
        EXPECT_EQ("0001", a.str());
        EXPECT_EQ("0010", b.str());
        EXPECT_EQ("0011", c.str());
    }
}

TEST(logic, bool_) {    // NOLINT
    {
        logic::logic<4> a{"010x"};
        if (a) {
            FAIL();
        }
        bool b = static_cast<bool>(a);
        EXPECT_FALSE(b);
    }

    {
        logic::logic<4> a{"0100"};
        if (!a) {
            FAIL();
        }
        bool b = static_cast<bool>(a);
        EXPECT_TRUE(b);
    }

    {
        logic::logic<4> a{"0100"};
        logic::logic<4> b{"010z"};
        if (a && b) {
            FAIL();
        }
        bool c = static_cast<bool>(a && b);
        EXPECT_FALSE(c);
    }
}

TEST(logic, and_) { // NOLINT
    {
        logic::logic<1> a{"0"}, b{"1"};
        auto c = a & b;
        EXPECT_EQ("0", c.str());
    }

    {
        logic::logic<1> a{"0"}, b{"x"};
        auto c = a & b;
        EXPECT_EQ("0", c.str());
    }

    {
        logic::logic<1> a{"z"}, b{"x"};
        auto c = a & b;
        EXPECT_EQ("x", c.str());
    }

    {
        logic::logic<1> a{"1"}, b{"x"};
        auto c = a & b;
        EXPECT_EQ("x", c.str());
    }

    {
        logic::logic<1> a{"1"}, b{"z"};
        auto c = a & b;
        EXPECT_EQ("x", c.str());
    }
}

TEST(logic, or_) {  // NOLINT
    {
        logic::logic<1> a{"0"}, b{"1"};
        auto c = a | b;
        EXPECT_EQ("1", c.str());
    }

    {
        logic::logic<1> a{"0"}, b{"0"};
        auto c = a | b;
        EXPECT_EQ("0", c.str());
    }

    {
        logic::logic<1> a{"0"}, b{"x"};
        auto c = a | b;
        EXPECT_EQ("x", c.str());
    }

    {
        logic::logic<1> a{"1"}, b{"x"};
        auto c = a | b;
        EXPECT_EQ("1", c.str());
    }

    {
        logic::logic<1> a{"z"}, b{"x"};
        auto c = a | b;
        EXPECT_EQ("x", c.str());
    }
}

TEST(logic, xor_) {  // NOLINT
    {
        logic::logic<1> a{"0"}, b{"1"};
        auto c = a ^ b;
        EXPECT_EQ("1", c.str());
    }

    {
        logic::logic<1> a{"0"}, b{"0"};
        auto c = a ^ b;
        EXPECT_EQ("0", c.str());
    }

    {
        logic::logic<1> a{"0"}, b{"x"};
        auto c = a ^ b;
        EXPECT_EQ("x", c.str());
    }

    {
        logic::logic<1> a{"1"}, b{"x"};
        auto c = a ^ b;
        EXPECT_EQ("x", c.str());
    }

    {
        logic::logic<1> a{"z"}, b{"x"};
        auto c = a ^ b;
        EXPECT_EQ("x", c.str());
    }
}

TEST(logic, not_) {  // NOLINT
    {
        logic::logic<1> a{"0"};
        auto b = ~a;
        EXPECT_EQ("1", b.str());
    }

    {
        logic::logic<1> a{"1"};
        auto b = ~a;
        EXPECT_EQ("0", b.str());
    }

    {
        logic::logic<1> a{"x"};
        auto b = ~a;
        EXPECT_EQ("x", b.str());
    }

    {
        logic::logic<1> a{"z"};
        auto b = ~a;
        EXPECT_EQ("x", b.str());
    }

}

TEST(logic, r_and) {    // NOLINT
    {
        logic::logic<4> a{"1001"};
        auto b = a.r_and();
        EXPECT_EQ("0", b.str());
    }

    {
        logic::logic<4> a{"1111"};
        auto b = a.r_and();
        EXPECT_EQ("1", b.str());
    }

    {
        logic::logic<4> a{"x111"};
        auto b = a.r_and();
        EXPECT_EQ("x", b.str());
    }

    {
        logic::logic<4> a{"xz11"};
        auto b = a.r_and();
        EXPECT_EQ("x", b.str());
    }
}

TEST(logic, r_nand) {    // NOLINT
    {
        logic::logic<4> a{"1001"};
        auto b = a.r_nand();
        EXPECT_EQ("1", b.str());
    }

    {
        logic::logic<4> a{"x001"};
        auto b = a.r_nand();
        EXPECT_EQ("1", b.str());
    }

    {
        logic::logic<4> a{"z001"};
        auto b = a.r_nand();
        EXPECT_EQ("1", b.str());
    }

    {
        logic::logic<4> a{"xz11"};
        auto b = a.r_nand();
        EXPECT_EQ("x", b.str());
    }
}
