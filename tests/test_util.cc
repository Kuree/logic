#include <sstream>

#include "gtest/gtest.h"
#include "logic/util.hh"

TEST(util, int_parsing) {  // NOLINT
    {
        // TODO: remove this once the parsing migrates to the SV syntax
        auto v = logic::util::parse_raw_str("01010");
        EXPECT_EQ(v, 0b1010);
    }
    {
        auto v = logic::util::parse_raw_str("4'b1010");
        EXPECT_EQ(v, 0b1010);
        v = logic::util::parse_raw_str("4'b10x0");
        EXPECT_EQ(v, 0b1000);
        v = logic::util::parse_raw_str("4'b10z0");
        EXPECT_EQ(v, 0b1010);
        v = logic::util::parse_xz_raw_str("4'b10z0");
        EXPECT_EQ(v, 0b0010);
        v = logic::util::parse_xz_raw_str("4'b10x0");
        EXPECT_EQ(v, 0b0010);
    }
    {
        auto v = logic::util::parse_raw_str("100'd42");
        EXPECT_EQ(v, 42);
    }
    {
        auto v = logic::util::parse_raw_str("100'o12");
        EXPECT_EQ(v, 10);
        v = logic::util::parse_raw_str("100'ox2");
        EXPECT_EQ(v, 2);
        v = logic::util::parse_raw_str("100'oz2");
        EXPECT_EQ(v, 0b111'010);
        v = logic::util::parse_xz_raw_str("100'oz2");
        EXPECT_EQ(v, 0b111'000);
    }
    {
        auto v = logic::util::parse_raw_str("100'hFFFF");
        EXPECT_EQ(v, 0xFFFF);
        v = logic::util::parse_raw_str("100'hFXFF");
        EXPECT_EQ(v, 0xF0FF);
        v = logic::util::parse_raw_str("100'hFZFF");
        EXPECT_EQ(v, 0xFFFF);
        v = logic::util::parse_xz_raw_str("100'hFZFF");
        EXPECT_EQ(v, 0x0F00);
    }
}

TEST(util, ints_parsing) {  // NOLINT
    {
        std::stringstream ss;
        ss << "'b";
        for (auto i = 0; i < 50; i++) {
            ss << "10";
        }
        std::array<uint64_t, 2> data = {};
        logic::util::parse_raw_str(ss.str(), 2 * 64, data.data());
        EXPECT_EQ(data[0], 0xAAAAAAAAAAAAAAAA);
        EXPECT_EQ(data[1], 0xAAAAAAAAA);
    }

    {
        std::stringstream ss;
        ss << "'h";
        for (auto i = 0; i < 20; i++) {
            ss << "1X";
        }
        std::array<uint64_t, 3> data = {};
        logic::util::parse_raw_str(ss.str(), 3 * 64, data.data());
        EXPECT_EQ(data[0], 0x1010101010101010);
        EXPECT_EQ(data[1], 0x1010101010101010);
        EXPECT_EQ(data[2], 0x10101010);

        logic::util::parse_xz_raw_str(ss.str(), 3 * 64, data.data());
        EXPECT_EQ(data[0], 0x0F0F0F0F0F0F0F0F);
        EXPECT_EQ(data[1], 0x0F0F0F0F0F0F0F0F);
        EXPECT_EQ(data[2], 0x0F0F0F0F);
    }
}