#include <gtest/gtest.h>

#include "gbemu/bitutils.h"

TEST(BitUtilsTest, GetAndSetBit)
{
    EXPECT_EQ(0b00000001, gbemu::setBit(0b00000000, 0, 1));
    EXPECT_EQ(0b00000010, gbemu::setBit(0b00000000, 1, 1));
    EXPECT_EQ(0b00000100, gbemu::setBit(0b00000000, 2, 1));
    EXPECT_EQ(0b00001000, gbemu::setBit(0b00000000, 3, 1));
    EXPECT_EQ(0b00010000, gbemu::setBit(0b00000000, 4, 1));
    EXPECT_EQ(0b00100000, gbemu::setBit(0b00000000, 5, 1));
    EXPECT_EQ(0b01000000, gbemu::setBit(0b00000000, 6, 1));
    EXPECT_EQ(0b10000000, gbemu::setBit(0b00000000, 7, 1));

    EXPECT_EQ(0, gbemu::getBit(0b11010010, 0));
    EXPECT_EQ(1, gbemu::getBit(0b11010010, 1));
    EXPECT_EQ(0, gbemu::getBit(0b11010010, 2));
    EXPECT_EQ(0, gbemu::getBit(0b11010010, 3));
    EXPECT_EQ(1, gbemu::getBit(0b11010010, 4));
    EXPECT_EQ(0, gbemu::getBit(0b11010010, 5));
    EXPECT_EQ(1, gbemu::getBit(0b11010010, 6));
    EXPECT_EQ(1, gbemu::getBit(0b11010010, 7));
}