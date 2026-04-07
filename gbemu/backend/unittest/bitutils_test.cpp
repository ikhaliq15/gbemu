#include "gbemu/backend/bitutils.h"

#include <gtest/gtest.h>

#include <cstdint>

using namespace gbemu::backend;

TEST(BitUtilsTest, UpperLowerByte)
{
    constexpr uint16_t v = 0xABCD;

    EXPECT_EQ(uint8_t{0xAB}, upperByte(v));
    EXPECT_EQ(uint8_t{0xCD}, lowerByte(v));
}

TEST(BitUtilsTest, SetUpperLowerByte)
{
    constexpr uint16_t v = 0x1234;

    EXPECT_EQ(uint16_t{0xAB34}, setUpperByte(v, uint8_t{0xAB}));
    EXPECT_EQ(uint16_t{0x12CD}, setLowerByte(v, uint8_t{0xCD}));
}

TEST(BitUtilsTest, ConcatBytes) { EXPECT_EQ(uint16_t{0xABCD}, concatBytes(uint8_t{0xAB}, uint8_t{0xCD})); }

TEST(BitUtilsTest, SwapNibbles)
{
    EXPECT_EQ(uint8_t{0xBA}, swapNibbles(uint8_t{0xAB}));
    EXPECT_EQ(uint8_t{0x00}, swapNibbles(uint8_t{0x00}));
    EXPECT_EQ(uint8_t{0xFF}, swapNibbles(uint8_t{0xFF}));
}

TEST(BitUtilsTest, InterpolateNibbles)
{
    EXPECT_EQ(uint8_t{0xA5}, interpolateNibbles(uint8_t{0xA0}, uint8_t{0x05}));
    EXPECT_EQ(uint8_t{0xFF}, interpolateNibbles(uint8_t{0xF0}, uint8_t{0x0F}));
    EXPECT_EQ(uint8_t{0xC4}, interpolateNibbles(uint8_t{0xC3}, uint8_t{0x24}));
}

TEST(BitUtilsTest, GetBit)
{
    constexpr uint8_t v = 0b11010010;

    EXPECT_FALSE(getBit(v, 0));
    EXPECT_TRUE(getBit(v, 1));
    EXPECT_FALSE(getBit(v, 2));
    EXPECT_FALSE(getBit(v, 3));
    EXPECT_TRUE(getBit(v, 4));
    EXPECT_FALSE(getBit(v, 5));
    EXPECT_TRUE(getBit(v, 6));
    EXPECT_TRUE(getBit(v, 7));
}

TEST(BitUtilsTest, SetBit_SetAndClear)
{
    uint8_t v = 0;

    for (uint8_t bit = 0; bit < 8; ++bit)
    {
        v = setBit(v, bit, true);
        EXPECT_TRUE(getBit(v, bit));

        v = setBit(v, bit, false);
        EXPECT_FALSE(getBit(v, bit));
    }
}

TEST(BitUtilsTest, SetBit_DoesNotAffectOthers)
{
    constexpr uint8_t original = 0b10101010;

    for (uint8_t bit = 0; bit < 8; ++bit)
    {
        uint8_t modified = setBit(original, bit, true);

        for (uint8_t i = 0; i < 8; ++i)
        {
            if (i == bit)
                EXPECT_TRUE(getBit(modified, i));
            else
                EXPECT_EQ(getBit(original, i), getBit(modified, i));
        }
    }
}

TEST(BitUtilsTest, BitOps_Uint16)
{
    uint16_t v = 0;

    v = setBit(v, 12, true);
    EXPECT_TRUE(getBit(v, 12));

    v = setBit(v, 12, false);
    EXPECT_FALSE(getBit(v, 12));
}

TEST(BitUtilsTest, HexString_Uint8)
{
    EXPECT_EQ("0x00", toHexString(uint8_t{0x00}));
    EXPECT_EQ("0x0A", toHexString(uint8_t{0x0A}));
    EXPECT_EQ("0xFF", toHexString(uint8_t{0xFF}));

    EXPECT_EQ("FF", toHexString(uint8_t{0xFF}, false));
}

TEST(BitUtilsTest, HexString_Uint16)
{
    EXPECT_EQ("0x0000", toHexString(uint16_t{0x0000}));
    EXPECT_EQ("0x00FF", toHexString(uint16_t{0x00FF}));
    EXPECT_EQ("0xABCD", toHexString(uint16_t{0xABCD}));

    EXPECT_EQ("ABCD", toHexString(uint16_t{0xABCD}, false));
}

TEST(BitUtilsTest, AddCarry_Uint8)
{
    EXPECT_FALSE(addHadCarry(uint8_t{0x00}, uint8_t{0x00}));
    EXPECT_FALSE(addHadCarry(uint8_t{0xFE}, uint8_t{0x01}));

    EXPECT_TRUE(addHadCarry(uint8_t{0xFF}, uint8_t{0x01}));
    EXPECT_TRUE(addHadCarry(uint8_t{0x80}, uint8_t{0x80}));
}

TEST(BitUtilsTest, AddCarry_WithCarry)
{
    EXPECT_TRUE(addHadCarry(uint8_t{0xFF}, uint8_t{0x00}, uint8_t{0x01}));
    EXPECT_TRUE(addHadCarry(uint8_t{0xFE}, uint8_t{0x01}, uint8_t{0x01}));
}

TEST(BitUtilsTest, AddHalfCarry_Uint8)
{
    EXPECT_FALSE(addHadHalfCarry(uint8_t{0x0E}, uint8_t{0x01}));
    EXPECT_TRUE(addHadHalfCarry(uint8_t{0x0F}, uint8_t{0x01}));
    EXPECT_TRUE(addHadHalfCarry(uint8_t{0x08}, uint8_t{0x08}));
}

TEST(BitUtilsTest, AddHalfCarry_WithCarry)
{
    EXPECT_TRUE(addHadHalfCarry(uint8_t{0x0F}, uint8_t{0x00}, uint8_t{0x01}));
    EXPECT_TRUE(addHadHalfCarry(uint8_t{0x0E}, uint8_t{0x01}, uint8_t{0x01}));
}

TEST(BitUtilsTest, AddCarry_Uint16)
{
    EXPECT_FALSE(addHadCarry(uint16_t{0x0000}, uint16_t{0x0001}));
    EXPECT_TRUE(addHadCarry(uint16_t{0xFFFF}, uint16_t{0x0001}));
}

TEST(BitUtilsTest, AddHalfCarry_Uint16)
{
    EXPECT_FALSE(addHadHalfCarry(uint16_t{0x0FFF}, uint16_t{0x0000}));
    EXPECT_TRUE(addHadHalfCarry(uint16_t{0x0FFF}, uint16_t{0x0001}));
}

TEST(BitUtilsTest, SubCarry_Uint8)
{
    EXPECT_FALSE(subHadCarry(uint8_t{0x10}, uint8_t{0x01}));
    EXPECT_TRUE(subHadCarry(uint8_t{0x00}, uint8_t{0x01}));
}

TEST(BitUtilsTest, SubCarry_WithBorrow)
{
    EXPECT_TRUE(subHadCarry(uint8_t{0x00}, uint8_t{0x00}, uint8_t{0x01}));
    EXPECT_TRUE(subHadCarry(uint8_t{0x10}, uint8_t{0x0F}, uint8_t{0x02}));
}

TEST(BitUtilsTest, SubHalfCarry_Uint8)
{
    EXPECT_FALSE(subHadHalfCarry(uint8_t{0x0F}, uint8_t{0x0F}));
    EXPECT_TRUE(subHadHalfCarry(uint8_t{0x10}, uint8_t{0x01}));
}

TEST(BitUtilsTest, SubHalfCarry_WithBorrow)
{
    EXPECT_TRUE(subHadHalfCarry(uint8_t{0x00}, uint8_t{0x00}, uint8_t{0x01}));
    EXPECT_TRUE(subHadHalfCarry(uint8_t{0x10}, uint8_t{0x0F}, uint8_t{0x02}));
}

TEST(BitUtilsTest, SubCarry_Uint16)
{
    EXPECT_FALSE(subHadCarry(uint16_t{0x1000}, uint16_t{0x0001}));
    EXPECT_TRUE(subHadCarry(uint16_t{0x0000}, uint16_t{0x0001}));
}

TEST(BitUtilsTest, SubHalfCarry_Uint16)
{
    EXPECT_FALSE(subHadHalfCarry(uint16_t{0x0FFF}, uint16_t{0x0FFF}));
    EXPECT_TRUE(subHadHalfCarry(uint16_t{0x1000}, uint16_t{0x0001}));
}
