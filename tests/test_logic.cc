#include <sstream>

#include "gtest/gtest.h"
#include "logic/logic.hh"

TEST(logic, size) {  // NOLINT
    EXPECT_EQ(sizeof(logic::logic<4 - 1, 0>), 1 * 2);
    EXPECT_EQ(sizeof(logic::logic<8 - 1, 0>), 1 * 2);
    EXPECT_EQ(sizeof(logic::logic<64 - 1, 0>), 8 * 2);
    EXPECT_EQ(sizeof(logic::logic<100 - 1, 0>), 16 * 2);
    EXPECT_EQ(sizeof(logic::logic<128 - 1, 0>), 16 * 2);
}

TEST(logic, init) {  // NOLINT
    {
        // uin8_t holder
        logic::logic<4 - 1, 0> l;
        auto s = l.str();
        EXPECT_EQ(s, "xxxx");
    }
    {
        // uint16_t holder
        logic::logic<15 - 1, 0> l;
        auto s = l.str();
        EXPECT_EQ(s, "xxxxxxxxxxxxxxx");
    }
    {
        // uint32_t holder
        logic::logic<30 - 1, 0> l(42);
        auto s = l.str();
        EXPECT_EQ(s, "000000000000000000000000101010");
    }

    {
        // uint64_t holder
        logic::logic<64 - 1, 0> l(std::numeric_limits<uint64_t>::max());
        auto s = l.str();
        std::stringstream ss;
        for (auto i = 0; i < 64; i++) ss << '1';
        EXPECT_EQ(s, ss.str());
    }

    {
        // uint64_t holder with str initializer
        constexpr std::string_view v = "'bxx1010";
        logic::logic<40 - 1, 0> l(v);
        auto s = l.str();
        std::stringstream ss;
        for (auto i = 0u; i < 40u - v.size() + 2; i++) ss << '0';
        ss << "xx1010";
        EXPECT_EQ(s, ss.str());
    }

    {
        // big number holder
        // uint64_t holder with str initializer
        constexpr std::string_view v = "'bxx1010";
        logic::logic<128 - 1, 0> l(v);
        auto s = l.str();
        std::stringstream ss;
        for (auto i = 0u; i < 128u - v.size() + 2; i++) ss << '0';
        ss << "xx1010";
        EXPECT_EQ(s, ss.str());
    }
}

TEST(logic, slice) {  // NOLINT
    {
        // integer holder
        logic::logic<40 - 1, 0> l(42);
        auto s = l.slice<3, 2>();
        EXPECT_EQ(s.str(), "10");
    }

    {
        // big number holder slice small
        std::stringstream ss;
        auto constexpr ref = "'b1011";
        ss << ref;
        for (auto i = 0; i < 100; i++) ss << '0';
        logic::logic<120> l(ss.str());
        auto s = l.slice<103, 100>();
        EXPECT_EQ("1011", s.str());
    }

    {
        // big number holder slice big
        std::stringstream ss;
        auto constexpr ref = "'b1011";
        ss << ref;
        for (auto i = 0; i < 100; i++) ss << '1';
        logic::logic<120 - 1, 0> l(ss.str());
        auto s = l.slice<103, 42>();
        ss = std::stringstream();
        ss << "1011";
        for (auto i = 0; i < 100 - 42; i++) ss << "1";
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

TEST(logic, concat) {  // NOLINT
    {
        logic::logic<4 - 1, 0> a(1);
        auto b = a.concat(logic::logic<4 - 1, 0>(2), logic::logic<4 - 1, 0>(3));
        EXPECT_EQ("000100100011", b.str());
    }
}

TEST(logic, mask) {  // NOLINT
    {
        logic::logic<12 - 1, 0> a("'bxx00zz");
        EXPECT_TRUE(a.xz_mask.any_set());
        EXPECT_NE(a.value.value, 0);
    }
}

TEST(logic, unpack) {  // NOLINT
    {
        logic::logic<4 - 1, 0> a, b, c;
        auto d = logic::logic<12 - 1, 0>("'b000100100011");
        d.unpack(a, b, c);
        EXPECT_EQ("0001", a.str());
        EXPECT_EQ("0010", b.str());
        EXPECT_EQ("0011", c.str());
    }
}

TEST(logic, bool_) {  // NOLINT
    {
        logic::logic<4 - 1, 0> a{"'b010x"};
        if (a) {
            FAIL();
        }
        bool b = static_cast<bool>(a);
        EXPECT_FALSE(b);
    }

    {
        logic::logic<4 - 1, 0> a{"'b0100"};
        if (!a) {
            FAIL();
        }
        bool b = static_cast<bool>(a);
        EXPECT_TRUE(b);
    }

    {
        logic::logic<4 - 1, 0> a{"'b0100"};
        logic::logic<4 - 1, 0> b{"'b010z"};
        if (a && b) {
            FAIL();
        }
        bool c = static_cast<bool>(a && b);
        EXPECT_FALSE(c);
    }
}

TEST(logic, and_) {  // NOLINT
    using namespace std::literals;
    {
        logic::logic a{"'b0"}, b{"'b1"};
        auto c = a & b;
        EXPECT_EQ("0", c.str());
    }

    {
        logic::logic a{"'b0"}, b{"'bx"};
        auto c = a & b;
        EXPECT_EQ("0", c.str());
    }

    {
        logic::logic a{"'bz"}, b{"'bx"};
        auto c = a & b;
        EXPECT_EQ("x", c.str());
    }

    {
        logic::logic a{"'b1"}, b{"'bx"};
        auto c = a & b;
        EXPECT_EQ("x", c.str());
    }

    {
        logic::logic a{"'b1"}, b{"'bz"};
        auto c = a & b;
        EXPECT_EQ("x", c.str());
    }

    // big number with big number
    {
        std::stringstream ss;
        ss << "'b";
        for (auto i = 0; i < 120; i++) ss << '1';
        logic::logic<120 - 1, 0> a{ss.str()};
        auto const s = "'b1010101010"sv;
        logic::logic<140 - 1, 0> b{s};
        auto c = a & b;
        ss = {};
        for (auto i = 0u; i < (140 - s.size() + 2); i++) ss << '0';
        ss << "1010101010";
        EXPECT_EQ(ss.str(), c.str());
        c = b & a;
        EXPECT_EQ(ss.str(), c.str());
    }

    // big number with native number
    {
        std::stringstream ss;
        ss << "'b";
        for (auto i = 0; i < 120; i++) ss << '1';
        logic::logic<120 - 1, 0> a{ss.str()};
        auto const s = "'b1010101010"sv;
        logic::logic<40 - 1, 0> b{s};
        auto c = a & b;
        ss = {};
        for (auto i = 0u; i < (120 - s.size() + 2); i++) ss << '0';
        ss << "1010101010";
        EXPECT_EQ(ss.str(), c.str());
        c = b & a;
        EXPECT_EQ(ss.str(), c.str());
    }
}

TEST(logic, or_) {  // NOLINT
    using namespace std::literals;
    {
        logic::logic a{"'b0"}, b{"'b1"};
        auto c = a | b;
        EXPECT_EQ("1", c.str());
    }

    {
        logic::logic a{"'b0"}, b{"'b0"};
        auto c = a | b;
        EXPECT_EQ("0", c.str());
    }

    {
        logic::logic a{"'b0"}, b{"'bx"};
        auto c = a | b;
        EXPECT_EQ("x", c.str());
    }

    {
        logic::logic a{"'b1"}, b{"'bx"};
        auto c = a | b;
        EXPECT_EQ("1", c.str());
    }

    {
        logic::logic a{"'bz"}, b{"'bx"};
        auto c = a | b;
        EXPECT_EQ("x", c.str());
    }

    // big number with big number
    {
        std::stringstream ss;
        ss << "'b";
        for (auto i = 0; i < 120; i++) ss << '0';
        logic::logic<120 - 1, 0> a{ss.str()};
        auto const s = "'b1010101010"sv;
        logic::logic<140 - 1, 0> b{s};
        auto c = a | b;
        ss = {};
        for (auto i = 0u; i < (140 - s.size() + 2); i++) ss << '0';
        ss << "1010101010";
        EXPECT_EQ(ss.str(), c.str());
        c = b | a;
        EXPECT_EQ(ss.str(), c.str());
    }

    // big number with native number
    {
        std::stringstream ss;
        ss << "'b";
        for (auto i = 0; i < 120; i++) ss << '0';
        logic::logic<120 - 1, 0> a{ss.str()};
        auto const s = "'b1010101010"sv;
        logic::logic<40 - 1, 0> b{s};
        auto c = a | b;
        ss = {};
        for (auto i = 0u; i < (120 - s.size() + 2); i++) ss << '0';
        ss << "1010101010";
        EXPECT_EQ(ss.str(), c.str());
        c = b | a;
        EXPECT_EQ(ss.str(), c.str());
    }
}

TEST(logic, xor_) {  // NOLINT
    using namespace std::literals;
    {
        logic::logic a{"'b0"}, b{"'b1"};
        auto c = a ^ b;
        EXPECT_EQ("1", c.str());
    }

    {
        logic::logic a{"'b0"}, b{"'b0"};
        auto c = a ^ b;
        EXPECT_EQ("0", c.str());
    }

    {
        logic::logic a{"'b0"}, b{"'bx"};
        auto c = a ^ b;
        EXPECT_EQ("x", c.str());
    }

    {
        logic::logic a{"'b1"}, b{"'bx"};
        auto c = a ^ b;
        EXPECT_EQ("x", c.str());
    }

    {
        logic::logic a{"'bz"}, b{"'bx"};
        auto c = a ^ b;
        EXPECT_EQ("x", c.str());
    }

    // big number with big number
    {
        std::stringstream ss;
        ss << "'b";
        for (auto i = 0; i < 120; i++) ss << '0';
        logic::logic<120 - 1, 0> a{ss.str()};
        auto const s = "'b1010101010"sv;
        logic::logic<140 - 1, 0> b{s};
        auto c = a ^ b;
        ss = {};
        for (auto i = 0u; i < (140 - s.size() + 2); i++) ss << '0';
        ss << "1010101010";
        EXPECT_EQ(ss.str(), c.str());
        c = b ^ a;
        EXPECT_EQ(ss.str(), c.str());
    }

    // big number with native number
    {
        std::stringstream ss;
        ss << "'b";
        for (auto i = 0; i < 120; i++) ss << '0';
        logic::logic<120 - 1, 0> a{ss.str()};
        auto const s = "'b1010101010"sv;
        logic::logic<40 - 1, 0> b{s};
        auto c = a ^ b;
        ss = {};
        for (auto i = 0u; i < (120 - s.size() + 2); i++) ss << '0';
        ss << "1010101010";
        EXPECT_EQ(ss.str(), c.str());
        c = b ^ a;
        EXPECT_EQ(ss.str(), c.str());
    }
}

TEST(logic, not_) {  // NOLINT
    {
        logic::logic a{"'b0"};
        auto b = ~a;
        EXPECT_EQ("1", b.str());
    }

    {
        logic::logic a{"'b1"};
        auto b = ~a;
        EXPECT_EQ("0", b.str());
    }

    {
        logic::logic a{"'bx"};
        auto b = ~a;
        EXPECT_EQ("x", b.str());
    }

    {
        logic::logic a{"'bz"};
        auto b = ~a;
        EXPECT_EQ("x", b.str());
    }
}

TEST(logic, r_and) {  // NOLINT
    {
        logic::logic<4 - 1, 0> a{"'b1001"};
        auto b = a.r_and();
        EXPECT_EQ("0", b.str());
    }

    {
        logic::logic<4 - 1, 0> a{"'b1111"};
        auto b = a.r_and();
        EXPECT_EQ("1", b.str());
    }

    {
        logic::logic<4 - 1, 0> a{"'bx111"};
        auto b = a.r_and();
        EXPECT_EQ("x", b.str());
    }

    {
        logic::logic<4 - 1, 0> a{"'bxz11"};
        auto b = a.r_and();
        EXPECT_EQ("x", b.str());
    }
}

TEST(logic, r_nand) {  // NOLINT
    {
        logic::logic<4 - 1, 0> a{"'b1001"};
        auto b = a.r_nand();
        EXPECT_EQ("1", b.str());
    }

    {
        logic::logic<4 - 1, 0> a{"'bx001"};
        auto b = a.r_nand();
        EXPECT_EQ("1", b.str());
    }

    {
        logic::logic<4 - 1, 0> a{"'bz001"};
        auto b = a.r_nand();
        EXPECT_EQ("1", b.str());
    }

    {
        logic::logic<4 - 1, 0> a{"'bxz11"};
        auto b = a.r_nand();
        EXPECT_EQ("x", b.str());
    }
}

TEST(logic, r_or) {  // NOLINT
    {
        logic::logic<4 - 1, 0> a{"'b1001"};
        auto b = a.r_or();
        EXPECT_EQ("1", b.str());
    }

    {
        logic::logic<4 - 1, 0> a{"'bz000"};
        auto b = a.r_or();
        EXPECT_EQ("x", b.str());
    }

    {
        logic::logic<4 - 1, 0> a{"'bx000"};
        auto b = a.r_or();
        EXPECT_EQ("x", b.str());
    }
}

TEST(logic, r_nor) {  // NOLINT
    {
        logic::logic<4 - 1, 0> a{"'b1001"};
        auto b = a.r_nor();
        EXPECT_EQ("0", b.str());
    }

    {
        logic::logic<4 - 1, 0> a{"'bz000"};
        auto b = a.r_nor();
        EXPECT_EQ("x", b.str());
    }

    {
        logic::logic<4 - 1, 0> a{"'bx001"};
        auto b = a.r_nor();
        EXPECT_EQ("0", b.str());
    }
}

TEST(logic, r_xor) {  // NOLINT
    {
        logic::logic<4 - 1, 0> a{"'b1001"};
        auto b = a.r_xor();
        EXPECT_EQ("0", b.str());
    }

    {
        logic::logic<4 - 1, 0> a{"'bz001"};
        auto b = a.r_xor();
        EXPECT_EQ("x", b.str());
    }

    {
        logic::logic<4 - 1, 0> a{"'bx001"};
        auto b = a.r_xor();
        EXPECT_EQ("x", b.str());
    }
}

TEST(logic, r_xnor) {  // NOLINT
    {
        logic::logic<4 - 1, 0> a{"'b1001"};
        auto b = a.r_xnor();
        EXPECT_EQ("1", b.str());
    }

    {
        logic::logic<4 - 1, 0> a{"'bz001"};
        auto b = a.r_xnor();
        EXPECT_EQ("x", b.str());
    }

    {
        logic::logic<4 - 1, 0> a{"'bx001"};
        auto b = a.r_xnor();
        EXPECT_EQ("x", b.str());
    }
}

TEST(logic, bitwise_shift_left) {  // NOLINT
    {
        // big number
        std::stringstream ss;
        ss << "'b";
        for (uint64_t i = 0; i < 120; i++) ss << '1';
        logic::logic<120 - 1, 0> a{ss.str()};
        logic::logic<100 - 1, 0> b{"'b1111111111111111"};

        auto c = a << b;
        ss = {};
        for (uint64_t i = 0; i < 120; i++) ss << '0';
        EXPECT_EQ(ss.str(), c.str());
        b = logic::logic<100 - 1, 0>{"'b111"};
        c = a << b;
        ss = {};
        for (uint64_t i = 0; i < (120 - 7); i++) ss << '1';
        for (auto i = 0; i < 7; i++) ss << '0';
        EXPECT_EQ(ss.str(), c.str());
    }

    {
        // shifts with signed
        logic::logic<32 - 1, 0, true> a{-120};
        logic::logic<2 - 1, 0> b{2};
        auto c = a << b;
        logic::logic<32 - 1, 0, true> ref{-120 * 4};
        EXPECT_EQ(c.str(), ref.str());
    }

    {
        // normal number shifting
        logic::logic<20 - 1, 0> a{42};
        logic::logic<60 - 1, 0> b{8};
        auto c = a << b;
        EXPECT_EQ(c.value.value, 42 << 8);
    }
}

TEST(logic, bitwise_shift_right) {  // NOLINT
    {
        // big number
        std::stringstream ss;
        ss << "'b";
        for (uint64_t i = 0; i < 120; i++) ss << '1';
        logic::logic<120 - 1, 0> a{ss.str()};
        logic::logic<100 - 1, 0> b{"'b1111111111111111"};

        auto c = a >> b;
        ss = {};
        for (uint64_t i = 0; i < 120; i++) ss << '0';
        EXPECT_EQ(ss.str(), c.str());
        b = logic::logic<100 - 1, 0>{"'b111"};
        c = a >> b;
        ss = {};
        for (auto i = 0; i < 7; i++) ss << '0';
        for (uint64_t i = 0; i < (120 - 7); i++) ss << '1';
        EXPECT_EQ(ss.str(), c.str());
    }

    {
        // shifts with signed
        logic::logic<32 - 1, 0, true> a{-120};
        logic::logic<2> b{2};
        auto c = a >> b;
        logic::logic<32 - 1, 0, false> ref{static_cast<uint32_t>(-120) >> 2};
        EXPECT_EQ(c.str(), ref.str());
    }

    {
        // normal number shifting
        logic::logic<20 - 1, 0> a{42};
        logic::logic<60 - 1, 0> b{8};
        auto c = a >> b;
        EXPECT_EQ(c.value.value, 42 >> 8);
    }
}

TEST(logic, arithmetic_shift_left) {  // NOLINT
    {
        // big number
        std::stringstream ss;
        ss << "'b";
        for (uint64_t i = 0; i < 120; i++) ss << '1';
        logic::logic<120 - 1, 0> a{ss.str()};
        logic::logic<100 - 1, 0> b{"'b1111111111111111"};

        auto c = a.ashl(b);
        ss = {};
        for (uint64_t i = 0; i < 120; i++) ss << '0';
        EXPECT_EQ(ss.str(), c.str());
        b = logic::logic<100 - 1, 0>{"'b111"};
        c = a.ashl(b);
        ss = {};
        for (uint64_t i = 0; i < (120 - 7); i++) ss << '1';
        for (auto i = 0; i < 7; i++) ss << '0';
        EXPECT_EQ(ss.str(), c.str());
    }

    {
        // shifts with signed
        logic::logic<32 - 1, 0, true> a{-120};
        logic::logic<2> b{2};
        auto c = a.ashl(b);
        logic::logic<32 - 1, 0, true> ref{-120 * 4};
        EXPECT_EQ(c.str(), ref.str());
    }

    {
        // normal number shifting
        logic::logic<20 - 1, 0> a{42};
        logic::logic<60 - 1, 0> b{8};
        auto c = a.ashl(b);
        EXPECT_EQ(c.value.value, 42 << 8);
    }
}

TEST(logic, arithmetic_shift_right) {  // NOLINT
    {
        // big number
        std::stringstream ss;
        ss << "'b";
        for (uint64_t i = 0; i < 120; i++) ss << '1';
        logic::logic<120 - 1, 0> a{ss.str()};
        logic::logic<100 - 1, 0> b{"'b1111111111111111"};

        auto c = a.ashr(b);
        ss = {};
        for (uint64_t i = 0; i < 120; i++) ss << '0';
        EXPECT_EQ(ss.str(), c.str());
        b = logic::logic<100 - 1, 0>{"'b111"};
        c = a >> b;
        ss = {};
        for (auto i = 0; i < 7; i++) ss << '0';
        for (uint64_t i = 0; i < (120 - 7); i++) ss << '1';
        EXPECT_EQ(ss.str(), c.str());
    }

    {
        // shifts with signed
        logic::logic<32 - 1, 0, true> a{-120};
        logic::logic<2 - 1, 0> b{2};
        auto c = a.ashr(b);
        logic::logic<32 - 1, 0, true> ref{-120 >> 2};
        EXPECT_EQ(c.str(), ref.str());
    }

    {
        // big number signed shifting
        std::stringstream ss;
        ss << "'b1";
        for (auto i = 0; i < (420 - 1); i++) ss << '0';
        logic::logic<420 - 1, 0, true> a{ss.str()};
        logic::logic<120 - 1, 0> b{42ul};
        auto c = a.ashr(b);
        ss = {};
        for (auto i = 0; i < 42; i++) ss << '1';
        for (auto i = 42; i < 420; i++) ss << '0';
        EXPECT_EQ(c.str(), ss.str());
    }
}

TEST(logic, equal) {  // NOLINT
    {
        logic::logic a{true};
        auto b = a == logic::logic<10>{1u};
        EXPECT_EQ(b.str(), "1");
    }

    {
        // with big number
        logic::logic<120 - 1, 0> a{std::numeric_limits<uint64_t>::max() - 1};
        logic::logic<420 - 1, 0> b{std::numeric_limits<uint64_t>::max()};
        auto c = a == b;
        EXPECT_EQ(c.str(), "0");
    }

    {
        // with x and z
        logic::logic<1024 - 1, 0> a{"'bx"};
        logic::logic<20 - 1, 0> b{12};
        auto c = a == b;
        EXPECT_EQ(c.str(), "x");
    }
}

TEST(logic, neq) {  // NOLINT
    {
        logic::logic a{true};
        auto b = a != logic::logic<10>{1u};
        EXPECT_EQ(b.str(), "0");
    }

    {
        // with big number
        logic::logic<120 - 1, 0> a{std::numeric_limits<uint64_t>::max() - 1};
        logic::logic<420 - 1, 0> b{std::numeric_limits<uint64_t>::max()};
        auto c = a != b;
        EXPECT_EQ(c.str(), "1");
    }

    {
        // with x and z
        logic::logic<1024 - 1, 0> a{"'bx"};
        logic::logic<20 - 1, 0> b{12};
        auto c = a != b;
        EXPECT_EQ(c.str(), "x");
    }
}

TEST(logic, add) {  // NOLINT
    {
        // big number
        std::stringstream ss;
        ss << "'b";
        for (uint64_t i = 0; i < 120; i++) ss << '1';
        logic::logic<120 - 1, 0> a{ss.str()};
        ss = {};
        ss << "'b";
        for (uint64_t i = 0; i < 100; i++) ss << '1';
        logic::logic<100 - 1, 0> b{ss.str()};

        auto c = a + b;
        ss = {};
        for (uint64_t i = 0; i < 20; i++) ss << '0';
        for (uint64_t i = 0; i < (100 - 1); i++) ss << '1';
        ss << '0';
        EXPECT_EQ(ss.str(), c.str());
        c = b + a;
        EXPECT_EQ(ss.str(), c.str());
    }

    {
        // big number with small number
        std::stringstream ss;
        ss << "'b";
        for (uint64_t i = 0; i < 120; i++) ss << '1';
        logic::logic<120 - 1, 0> a{ss.str()};
        ss = {};
        ss << "'b";
        for (uint64_t i = 0; i < 40; i++) ss << '1';
        logic::logic<40 - 1, 0> b{ss.str()};
        auto c = a + b;
        ss = {};

        for (auto i = 0; i < 80; i++) ss << '0';
        for (auto i = 0; i < 39; i++) ss << '1';
        ss << '0';
        EXPECT_EQ(c.str(), ss.str());
        c = b + a;
        EXPECT_EQ(c.str(), ss.str());
    }

    {
        // small numbers
        logic::logic<20 - 1, 0> a{100};
        logic::logic<30 - 1, 0> b{10000};

        auto c = a + b;
        std::string_view ref = "10011101110100";
        std::stringstream ss;
        for (auto i = 0u; i < (30 - ref.size()); i++) ss << '0';
        ss << ref;
        EXPECT_EQ(c.str(), ss.str());
    }

    {
        // signed big numbers
    }
}

TEST(logic, minus) {  // NOLINT
    using namespace logic::literals;
    {
        // big number
        std::stringstream ss;
        ss << "'b";
        for (uint64_t i = 0; i < 120; i++) ss << '1';
        logic::logic<120 - 1, 0> a{ss.str()};
        ss = {};
        ss << "'b";
        for (uint64_t i = 0; i < 100; i++) ss << '1';
        logic::logic<100 - 1, 0> b{ss.str()};

        auto c = a - b;
        ss = {};
        for (uint64_t i = 0; i < 20; i++) ss << '1';
        for (uint64_t i = 0; i < 100; i++) ss << '0';
        EXPECT_EQ(ss.str(), c.str());
        c = b - a;
        ss = {};
        for (uint64_t i = 0; i < 19; i++) ss << '0';
        ss << '1';
        for (uint64_t i = 0; i < 100; i++) ss << '0';
        EXPECT_EQ(ss.str(), c.str());
    }

    {
        // big number with small number
        std::stringstream ss;
        ss << "'b";
        for (uint64_t i = 0; i < 120; i++) ss << '1';
        logic::logic<120 - 1, 0> a{ss.str()};
        ss = {};
        ss << "'b";
        for (uint64_t i = 0; i < 40; i++) ss << '1';
        logic::logic<40 - 1, 0> b{ss.str()};
        auto c = a - b;
        ss = {};

        for (auto i = 0; i < 80; i++) ss << '1';
        for (auto i = 0; i < 40; i++) ss << '0';
        EXPECT_EQ(c.str(), ss.str());
    }

    {
        // small numbers
        logic::logic<40 - 1, 0> a{600000};
        logic::logic<30 - 1, 0> b{10000};

        auto c = a - b;
        std::string_view ref = "10010000000010110000";
        std::stringstream ss;
        for (auto i = 0u; i < (40 - ref.size()); i++) ss << '0';
        ss << ref;
        EXPECT_EQ(c.str(), ss.str());
    }

    {
        // signed big numbers
        logic::logic<100 - 1, 0> a{0u};
        logic::logic<30, 0, true> b{-42};
        auto c = b.extend<100>() << 20_logic;
        auto d = a - c;
        auto ref = logic::logic<100 - 1>(42u << 20);
        EXPECT_EQ(ref.str(), d.str());
    }
}

TEST(logic, multiply) {  // NOLINT
    using namespace logic::literals;
    {
        // small numbers
        auto a = -2_logic;
        auto b = 42_logic;
        auto c = a * b;
        auto d = -84_logic;
        EXPECT_TRUE(c == d);
    }

    {
        // big number multiply big numbers
        auto a = logic::logic<299, 0>(
            "'b100010101011111110001011000101011111111111111111111111111111111111111111111111111111"
            "00000000000000000");
        auto b = logic::logic<299, 0>(
            "'b100000011111000110011010111111111111111111111111111111111111111111111111110101010100"
            "10110101010100101");
        auto c = a * b;
        EXPECT_EQ(c.str(),
                  "00000000000000000000000000000000000000000000000000000000000000000000000000000000"
                  "00000000000000000001000110011011010111011101101001111111101110110001010001111111"
                  "11111111111110100011010010100010001100111010100100100011100000100101110000000000"
                  "000000000000000010101010110100101010101101100000000000000000");
    }

    {
        // big number multiply small numbers
        // big number is negative as well
        std::stringstream ss;
        ss << "'b";
        for (auto i = 0; i < 100; i++) ss << '1';
        auto a = logic::logic<100 - 1, 0, true>{ss.str()};
        auto b = logic::logic<20 - 1, 0>{42};
        auto c = a * b;
        EXPECT_EQ(c.str(),
                  "11111111111111111111111111111111111111111111111111111111111111111111111111111111"
                  "11111111111111010110");
    }

    {
        // small number and small number
        auto a = logic::logic<9, 0>(10);
        auto b = logic::logic<2, 0>(3);
        EXPECT_EQ(a * b, 30_logic);
    }
}

TEST(logic, divide) {  // NOLINT
    using namespace logic::literals;
    {
        // big numbers vs big numbers
        logic::logic<300 - 1, 0> a{0u};
        a = ~a;
        std::stringstream ss;
        ss << "'b";
        for (auto i = 0; i < 18; i++) ss << "10";
        logic::logic<100, 0> b{ss.str()};

        auto c = a / b;
        EXPECT_EQ(c.str(),
                  "00000000000000000000000000000000000110000000000000000000000000000000000110000000"
                  "00000000000000000000000000011000000000000000000000000000000000011000000000000000"
                  "00000000000000000001100000000000000000000000000000000001100000000000000000000000"
                  "000000000001100000000000000000000000000000000001100000000000");
    }

    {
        // big number vs small number
        logic::logic<300 - 1, 0> a{0u};
        a = ~a;
        auto b = 42_logic;
        auto c = a / b;
        EXPECT_EQ(c.str(),
                  "00000110000110000110000110000110000110000110000110000110000110000110000110000110"
                  "00011000011000011000011000011000011000011000011000011000011000011000011000011000"
                  "01100001100001100001100001100001100001100001100001100001100001100001100001100001"
                  "100001100001100001100001100001100001100001100001100001100001");
    }

    {
        // small numbers
        auto a = 100_logic;
        auto b = 42_logic;
        auto c = a / b;
        EXPECT_EQ(c.str(), (2_logic).str().c_str());
    }
}

TEST(logic, mod) {  // NOLINT
    using namespace logic::literals;
    {
        // big numbers vs big numbers
        logic::logic<300 - 1, 0> a{0u};
        a = ~a;
        std::stringstream ss;
        ss << "'b";
        for (auto i = 0; i < 18; i++) ss << "10";
        logic::logic<100, 0> b{ss.str()};

        auto c = a % b;
        EXPECT_EQ(c.str(),
                  "00000000000000000000000000000000000000000000000000000000000000000000000000000000"
                  "00000000000000000000000000000000000000000000000000000000000000000000000000000000"
                  "00000000000000000000000000000000000000000000000000000000000000000000000000000000"
                  "000000000000000000000000000000000000000000000000111111111111");
    }

    {
        // big number vs small number
        logic::logic<300 - 1, 0> a{0u};
        a = ~a;
        auto b = 42_logic;
        auto c = a % b;
        EXPECT_EQ(c.str(),
                  "00000000000000000000000000000000000000000000000000000000000000000000000000000000"
                  "00000000000000000000000000000000000000000000000000000000000000000000000000000000"
                  "00000000000000000000000000000000000000000000000000000000000000000000000000000000"
                  "000000000000000000000000000000000000000000000000000000010101");
    }

    {
        // small numbers
        auto a = 100_logic;
        auto b = 42_logic;
        auto c = a % b;
        EXPECT_EQ(c.str(), (16_logic).str().c_str());
    }
}

// we only need to implement > since the reset are based off that
TEST(logic, gt) {  // NOLINT
    using namespace logic::literals;
    {
        // big number one signed one unsigned
        // should be unsigned comparison
        logic::logic<100, 0, true> a{0u};
        a = (~a).to_signed();  // should be -1
        logic::logic<100> b{10u};
        EXPECT_TRUE(a > b);
    }

    {
        // big number one signed one unsigned
        // should be unsigned comparison
        logic::logic<100, 0, true> a{0u};
        a = (~a).to_signed();  // should be -1
        logic::logic<100, 0, true> b{10u};
        b = (~b).to_signed();
        EXPECT_TRUE(a > b);
        b = logic::logic<100, 0, true>{10u};
        EXPECT_FALSE(a > b);
    }

    {
        // two small numbers
        auto a = -2_logic;
        auto b = 3_logic;
        EXPECT_TRUE(b > a);
        logic::logic<31> c{1u};
        EXPECT_TRUE(a > c);
        auto d = -1_logic;
        EXPECT_TRUE(d > a);
    }

    {
        logic::logic<20> a{42u};
        logic::logic<10> b{20u};
        EXPECT_TRUE(a > b);
    }

    {
        // two big signed numbers
        logic::logic<100, 0, true> a{0u};
        a = (~a).to_signed();  // should be -1
        logic::logic<100, 0, true> b{10u};
        b = (~b).to_signed();  // should be -10;
        EXPECT_TRUE(a > b);
    }
}

TEST(logic, update) {  // NOLINT
    {
        // native number exact
        logic::logic<15, 0> a;
        logic::logic<3, 0> b{"'b1010"};
        a.update<3, 0>(b);
        EXPECT_EQ(a.str(), "xxxxxxxxxxxx1010");
    }

    {
        // native number smaller
        logic::logic<15, 0> a;
        logic::logic<3, 0> b{"'b1010"};
        a.update<1, 0>(b);
        EXPECT_EQ(a.str(), "xxxxxxxxxxxxxx10");
    }

    {
        // native number smaller
        logic::logic<15, 0> a;
        logic::logic<3, 0> b{"'b1010"};
        a.update<9, 0>(b);
        EXPECT_EQ(a.str(), "xxxxxx0000001010");
    }

    {
        // big number
        // should work since it's goes through the same function calls as the native number
        logic::logic<100 - 1, 0> a;
        auto constexpr ref = "'b1010";
        logic::logic<3, 0> b{ref};
        a.update<3, 0>(b);
        std::stringstream ss;
        for (auto i = 0; i < 96; i++) ss << 'x';
        ss << "1010";
        EXPECT_EQ(a.str(), ss.str());
    }

    {
        // little endian
        logic::logic<0, 99> a;
        auto constexpr ref = "'b1110";
        logic::logic<0, 3> b{ref};
        a.update<0, 3>(b);
        std::stringstream ss;
        ss << "1110";
        for (auto i = 0; i < 96; i++) ss << 'x';
        EXPECT_EQ(a.str(), ss.str());
    }

    {
        // little endian mixed
        logic::logic<0, 99> a;
        auto constexpr ref = "'b1110";
        logic::logic<3, 0> b{ref};
        a.update<0, 3>(b);
        std::stringstream ss;
        ss << "1110";
        for (auto i = 0; i < 96; i++) ss << 'x';
        EXPECT_EQ(a.str(), ss.str());
    }
}

TEST(logic, to_uint64) {  // NOLINT
    using namespace logic::literals;
    auto a = 42_logic;
    auto v = a.to_uint64();
    EXPECT_EQ(v, 42);
}

TEST(logic, string_init) {  // NOLINT
    auto a = logic::logic<31, 0>("16'hFFFF");
    EXPECT_EQ(a.to_uint64(), 0xFFFF);

    auto b = logic::logic<127, 0>("128'd123456");
    EXPECT_EQ(b.to_uint64(), 123456);

    auto c = logic::logic<9, 0>("32'h42");
    EXPECT_EQ(c.to_uint64(), 66);

    auto d = logic::logic<31, 0>("100'o12");
    EXPECT_EQ(d.to_uint64(), 0xA);

    auto e = logic::logic<31, 0>("20'b001_0101");
    EXPECT_EQ(e.to_uint64(), 21);
}

TEST(logic, format) {  // NOLINT
    {
        auto a = logic::logic<20 - 1, 0>("'b1010_1001");
        auto s = a.str("0d");
        EXPECT_EQ(s, "169");
    }
    {
        auto a = logic::logic<20 - 1, 0>("'b1010_1001");
        auto s = a.str("d");
        EXPECT_EQ(s, "    169");
    }

    {
        // negative numbers
        auto b = -logic::logic<20 - 1, 0, true>("'d42");
        auto s = b.str("0d");
        EXPECT_EQ(s, "-42");
    }

    {
        // negative numbers
        auto b = -logic::logic<20 - 1, 0, true>("'d42");
        auto s = b.str("10d");
        EXPECT_EQ(s, "        -42");
    }

    {
        // negative numbers
        auto b = -logic::logic<20 - 1, 0, true>("'d42");
        auto s = b.str("d");
        EXPECT_EQ(s, "     -42");
    }

    {
        // hex
        auto a = logic::logic<20 - 1, 0>("'d42");
        auto s = a.str("x");
        EXPECT_EQ(s, "0002a");
    }

    // big number oct
    {
        auto a = logic::logic<120 - 1, 0>("'hFFFFFFFFFFFF");
        auto s = a.str("o");
        EXPECT_EQ(s, "0000000000000000000000007777777777777777");
    }

    // big number hex
    {
        auto a = logic::logic<120 - 1, 0>("'hFFFFFFFFFFFF");
        auto s = a.str("X");
        EXPECT_EQ(s, "000000000000000000ffffffffffff");
    }

    {
        // big number raw str with overflow
        auto a = logic::logic<120 - 1, 0>("ABCDEFGHIJKLMNOPQRST");
        auto s = a.str("s");
        EXPECT_EQ(s, "FGHIJKLMNOPQRST");
    }
}