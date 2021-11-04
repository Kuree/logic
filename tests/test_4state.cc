#include <sstream>

#include "gtest/gtest.h"
#include "logic/logic.hh"

TEST(logic, size) {  // NOLINT
    EXPECT_EQ(sizeof(logic::logic<4>), 1 * 2);
    EXPECT_EQ(sizeof(logic::logic<8>), 1 * 2);
    EXPECT_EQ(sizeof(logic::logic<64>), 8 * 2);
    EXPECT_EQ(sizeof(logic::logic<100>), 16 * 2);
    EXPECT_EQ(sizeof(logic::logic<128>), 16 * 2);
}

TEST(logic, init) {  // NOLINT
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

TEST(logic, slice) {  // NOLINT
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
        logic::logic<4> a(1);
        auto b = a.concat(logic::logic<4>(2), logic::logic<4>(3));
        EXPECT_EQ("000100100011", b.str());
    }
}

TEST(logic, mask) {  // NOLINT
    {
        logic::logic<12> a("xx00zz");
        EXPECT_TRUE(a.xz_mask.any_set());
        EXPECT_NE(a.value.value, 0);
    }
}

TEST(logic, unpack) {  // NOLINT
    {
        logic::logic<4> a, b, c;
        auto d = logic::logic<12>("000100100011");
        d.unpack(a, b, c);
        EXPECT_EQ("0001", a.str());
        EXPECT_EQ("0010", b.str());
        EXPECT_EQ("0011", c.str());
    }
}

TEST(logic, bool_) {  // NOLINT
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

TEST(logic, and_) {  // NOLINT
    using namespace std::literals;
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

    // big number with big number
    {
        std::stringstream ss;
        for (auto i = 0; i < 120; i++) ss << '1';
        logic::logic<120> a{ss.str()};
        auto const s = "1010101010"sv;
        logic::logic<140> b{s};
        auto c = a & b;
        ss = {};
        for (auto i = 0u; i < (140 - s.size()); i++) ss << '0';
        ss << s;
        EXPECT_EQ(ss.str(), c.str());
        c = b & a;
        EXPECT_EQ(ss.str(), c.str());
    }

    // big number with native number
    {
        std::stringstream ss;
        for (auto i = 0; i < 120; i++) ss << '1';
        logic::logic<120> a{ss.str()};
        auto const s = "1010101010"sv;
        logic::logic<40> b{s};
        auto c = a & b;
        ss = {};
        for (auto i = 0u; i < (120 - s.size()); i++) ss << '0';
        ss << s;
        EXPECT_EQ(ss.str(), c.str());
        c = b & a;
        EXPECT_EQ(ss.str(), c.str());
    }
}

TEST(logic, or_) {  // NOLINT
    using namespace std::literals;
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

    // big number with big number
    {
        std::stringstream ss;
        for (auto i = 0; i < 120; i++) ss << '0';
        logic::logic<120> a{ss.str()};
        auto const s = "1010101010"sv;
        logic::logic<140> b{s};
        auto c = a | b;
        ss = {};
        for (auto i = 0u; i < (140 - s.size()); i++) ss << '0';
        ss << s;
        EXPECT_EQ(ss.str(), c.str());
        c = b | a;
        EXPECT_EQ(ss.str(), c.str());
    }

    // big number with native number
    {
        std::stringstream ss;
        for (auto i = 0; i < 120; i++) ss << '0';
        logic::logic<120> a{ss.str()};
        auto const s = "1010101010"sv;
        logic::logic<40> b{s};
        auto c = a | b;
        ss = {};
        for (auto i = 0u; i < (120 - s.size()); i++) ss << '0';
        ss << s;
        EXPECT_EQ(ss.str(), c.str());
        c = b | a;
        EXPECT_EQ(ss.str(), c.str());
    }
}

TEST(logic, xor_) {  // NOLINT
    using namespace std::literals;
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

    // big number with big number
    {
        std::stringstream ss;
        for (auto i = 0; i < 120; i++) ss << '0';
        logic::logic<120> a{ss.str()};
        auto const s = "1010101010"sv;
        logic::logic<140> b{s};
        auto c = a ^ b;
        ss = {};
        for (auto i = 0u; i < (140 - s.size()); i++) ss << '0';
        ss << s;
        EXPECT_EQ(ss.str(), c.str());
        c = b ^ a;
        EXPECT_EQ(ss.str(), c.str());
    }

    // big number with native number
    {
        std::stringstream ss;
        for (auto i = 0; i < 120; i++) ss << '0';
        logic::logic<120> a{ss.str()};
        auto const s = "1010101010"sv;
        logic::logic<40> b{s};
        auto c = a ^ b;
        ss = {};
        for (auto i = 0u; i < (120 - s.size()); i++) ss << '0';
        ss << s;
        EXPECT_EQ(ss.str(), c.str());
        c = b ^ a;
        EXPECT_EQ(ss.str(), c.str());
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

TEST(logic, r_and) {  // NOLINT
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

TEST(logic, r_nand) {  // NOLINT
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

TEST(logic, r_or) {  // NOLINT
    {
        logic::logic<4> a{"1001"};
        auto b = a.r_or();
        EXPECT_EQ("1", b.str());
    }

    {
        logic::logic<4> a{"z000"};
        auto b = a.r_or();
        EXPECT_EQ("x", b.str());
    }

    {
        logic::logic<4> a{"x000"};
        auto b = a.r_or();
        EXPECT_EQ("x", b.str());
    }
}

TEST(logic, r_nor) {  // NOLINT
    {
        logic::logic<4> a{"1001"};
        auto b = a.r_nor();
        EXPECT_EQ("0", b.str());
    }

    {
        logic::logic<4> a{"z000"};
        auto b = a.r_nor();
        EXPECT_EQ("x", b.str());
    }

    {
        logic::logic<4> a{"x001"};
        auto b = a.r_nor();
        EXPECT_EQ("0", b.str());
    }
}

TEST(logic, r_xor) {  // NOLINT
    {
        logic::logic<4> a{"1001"};
        auto b = a.r_xor();
        EXPECT_EQ("0", b.str());
    }

    {
        logic::logic<4> a{"z001"};
        auto b = a.r_xor();
        EXPECT_EQ("x", b.str());
    }

    {
        logic::logic<4> a{"x001"};
        auto b = a.r_xor();
        EXPECT_EQ("x", b.str());
    }
}

TEST(logic, r_xnor) {  // NOLINT
    {
        logic::logic<4> a{"1001"};
        auto b = a.r_xnor();
        EXPECT_EQ("1", b.str());
    }

    {
        logic::logic<4> a{"z001"};
        auto b = a.r_xnor();
        EXPECT_EQ("x", b.str());
    }

    {
        logic::logic<4> a{"x001"};
        auto b = a.r_xnor();
        EXPECT_EQ("x", b.str());
    }
}

TEST(logic, bitwise_shift_left) {  // NOLINT
    {
        // big number
        std::stringstream ss;
        for (uint64_t i = 0; i < 120; i++) ss << '1';
        logic::logic<120> a{ss.str()};
        logic::logic<100> b{"1111111111111111"};

        auto c = a << b;
        ss = {};
        for (uint64_t i = 0; i < 120; i++) ss << '0';
        EXPECT_EQ(ss.str(), c.str());
        b = logic::logic<100>{"111"};
        c = a << b;
        ss = {};
        for (uint64_t i = 0; i < (120 - 7); i++) ss << '1';
        for (auto i = 0; i < 7; i++) ss << '0';
        EXPECT_EQ(ss.str(), c.str());
    }

    {
        // shifts with signed
        logic::logic<32, true> a{-120};
        logic::logic<2> b{2};
        auto c = a << b;
        logic::logic<32, true> ref{-120 * 4};
        EXPECT_EQ(c.str(), ref.str());
    }

    {
        // normal number shifting
        logic::logic<20> a{42};
        logic::logic<60> b{8};
        auto c = a << b;
        EXPECT_EQ(c.value.value, 42 << 8);
    }
}

TEST(logic, bitwise_shift_right) {  // NOLINT
    {
        // big number
        std::stringstream ss;
        for (uint64_t i = 0; i < 120; i++) ss << '1';
        logic::logic<120> a{ss.str()};
        logic::logic<100> b{"1111111111111111"};

        auto c = a >> b;
        ss = {};
        for (uint64_t i = 0; i < 120; i++) ss << '0';
        EXPECT_EQ(ss.str(), c.str());
        b = logic::logic<100>{"111"};
        c = a >> b;
        ss = {};
        for (auto i = 0; i < 7; i++) ss << '0';
        for (uint64_t i = 0; i < (120 - 7); i++) ss << '1';
        EXPECT_EQ(ss.str(), c.str());
    }

    {
        // shifts with signed
        logic::logic<32, true> a{-120};
        logic::logic<2> b{2};
        auto c = a >> b;
        logic::logic<32, false> ref{static_cast<uint32_t>(-120) >> 2};
        EXPECT_EQ(c.str(), ref.str());
    }

    {
        // normal number shifting
        logic::logic<20> a{42};
        logic::logic<60> b{8};
        auto c = a >> b;
        EXPECT_EQ(c.value.value, 42 >> 8);
    }
}

TEST(logic, arithmetic_shift_left) {  // NOLINT
    {
        // big number
        std::stringstream ss;
        for (uint64_t i = 0; i < 120; i++) ss << '1';
        logic::logic<120> a{ss.str()};
        logic::logic<100> b{"1111111111111111"};

        auto c = a.ashl(b);
        ss = {};
        for (uint64_t i = 0; i < 120; i++) ss << '0';
        EXPECT_EQ(ss.str(), c.str());
        b = logic::logic<100>{"111"};
        c = a.ashl(b);
        ss = {};
        for (uint64_t i = 0; i < (120 - 7); i++) ss << '1';
        for (auto i = 0; i < 7; i++) ss << '0';
        EXPECT_EQ(ss.str(), c.str());
    }

    {
        // shifts with signed
        logic::logic<32, true> a{-120};
        logic::logic<2> b{2};
        auto c = a.ashl(b);
        logic::logic<32, true> ref{-120 * 4};
        EXPECT_EQ(c.str(), ref.str());
    }

    {
        // normal number shifting
        logic::logic<20> a{42};
        logic::logic<60> b{8};
        auto c = a.ashl(b);
        EXPECT_EQ(c.value.value, 42 << 8);
    }
}

TEST(logic, arithmetic_shift_right) {  // NOLINT
    {
        // big number
        std::stringstream ss;
        for (uint64_t i = 0; i < 120; i++) ss << '1';
        logic::logic<120> a{ss.str()};
        logic::logic<100> b{"1111111111111111"};

        auto c = a.ashr(b);
        ss = {};
        for (uint64_t i = 0; i < 120; i++) ss << '0';
        EXPECT_EQ(ss.str(), c.str());
        b = logic::logic<100>{"111"};
        c = a >> b;
        ss = {};
        for (auto i = 0; i < 7; i++) ss << '0';
        for (uint64_t i = 0; i < (120 - 7); i++) ss << '1';
        EXPECT_EQ(ss.str(), c.str());
    }

    {
        // shifts with signed
        logic::logic<32, true> a{-120};
        logic::logic<2> b{2};
        auto c = a.ashr(b);
        logic::logic<32, true> ref{-120 >> 2};
        EXPECT_EQ(c.str(), ref.str());
    }

    {
        // big number signed shifting
        std::stringstream ss;
        ss << '1';
        for (auto i = 0; i < (420 - 1); i++) ss << '0';
        logic::logic<420, true> a{ss.str()};
        logic::logic<120> b{42ul};
        auto c = a.ashr(b);
        ss = {};
        for (auto i = 0; i < 42; i++) ss << '1';
        for (auto i = 42; i < 420; i++) ss << '0';
        EXPECT_EQ(c.str(), ss.str());
    }
}

TEST(logic, equal) {  // NOLINT
    {
        logic::logic<1> a{true};
        auto b = a == logic::logic<10>{1u};
        EXPECT_EQ(b.str(), "1");
    }

    {
        // with big number
        logic::logic<120> a{std::numeric_limits<uint64_t>::max() - 1};
        logic::logic<420> b{std::numeric_limits<uint64_t>::max()};
        auto c = a == b;
        EXPECT_EQ(c.str(), "0");
    }

    {
        // with x and z
        logic::logic<1024> a{"x"};
        logic::logic<20> b{12};
        auto c = a == b;
        EXPECT_EQ(c.str(), "x");
    }
}
