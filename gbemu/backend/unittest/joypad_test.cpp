#include <gtest/gtest.h>

#include "gbemu/backend/joypad.h"

class JoypadTest : public testing::Test
{
  protected:
    void SetUp() override { joypad_ = std::make_unique<gbemu::backend::Joypad>(); }

    uint8_t getButtonsNibble() const
    {
        joypad_->onWriteOwnedByte(gbemu::backend::RAM::JOYP, 0x10, 0x00);
        const auto joyp = joypad_->onReadOwnedByte(gbemu::backend::RAM::JOYP);
        EXPECT_EQ(joyp & 0xf0, 0x10);
        return joyp & 0x0f;
    }

    uint8_t getDpadNibble() const
    {
        joypad_->onWriteOwnedByte(gbemu::backend::RAM::JOYP, 0x20, 0x00);
        const auto joyp = joypad_->onReadOwnedByte(gbemu::backend::RAM::JOYP);
        EXPECT_EQ(joyp & 0xf0, 0x20);
        return joyp & 0x0f;
    }

    std::unique_ptr<gbemu::backend::Joypad> joypad_;
};

#define SINGLE_BUTTON_TEST(TestName, Button, ButtonBitIndex, isDpad)                                                   \
    TEST_F(JoypadTest, SingleButton_##TestName)                                                                        \
    {                                                                                                                  \
        const uint8_t expectedNibble = (~(1 << ButtonBitIndex)) & 0x0f;                                                \
        ASSERT_EQ(getButtonsNibble(), 0x0f);                                                                           \
        ASSERT_EQ(getDpadNibble(), 0x0f);                                                                              \
                                                                                                                       \
        for (int i = 0; i < 3; i++)                                                                                    \
        {                                                                                                              \
            joypad_->handleKeyDownEvent(Button);                                                                       \
                                                                                                                       \
            if (isDpad)                                                                                                \
            {                                                                                                          \
                ASSERT_EQ(getButtonsNibble(), 0x0f);                                                                   \
                ASSERT_EQ(getDpadNibble(), expectedNibble);                                                            \
            }                                                                                                          \
            else                                                                                                       \
            {                                                                                                          \
                ASSERT_EQ(getButtonsNibble(), expectedNibble);                                                         \
                ASSERT_EQ(getDpadNibble(), 0x0f);                                                                      \
            }                                                                                                          \
                                                                                                                       \
            joypad_->handleKeyUpEvent(Button);                                                                         \
                                                                                                                       \
            ASSERT_EQ(getButtonsNibble(), 0x0f);                                                                       \
            ASSERT_EQ(getDpadNibble(), 0x0f);                                                                          \
        }                                                                                                              \
    }

TEST_F(JoypadTest, NoButtonsPressed)
{
    // Neither dpad or buttons selected defaults to all buttons not pressed.
    ASSERT_EQ(joypad_->onReadOwnedByte(gbemu::backend::RAM::JOYP), 0x3f);

    joypad_->onWriteOwnedByte(gbemu::backend::RAM::JOYP, 0x10, 0x00);
    ASSERT_EQ(joypad_->onReadOwnedByte(gbemu::backend::RAM::JOYP), 0x1f);

    joypad_->onWriteOwnedByte(gbemu::backend::RAM::JOYP, 0x20, 0x00);
    ASSERT_EQ(joypad_->onReadOwnedByte(gbemu::backend::RAM::JOYP), 0x2f);
}

SINGLE_BUTTON_TEST(Down, gbemu::backend::Joypad::DOWN_BUTTON, 3, true);
SINGLE_BUTTON_TEST(Up, gbemu::backend::Joypad::UP_BUTTON, 2, true);
SINGLE_BUTTON_TEST(Left, gbemu::backend::Joypad::LEFT_BUTTON, 1, true);
SINGLE_BUTTON_TEST(Right, gbemu::backend::Joypad::RIGHT_BUTTON, 0, true);

SINGLE_BUTTON_TEST(Start, gbemu::backend::Joypad::START_BUTTON, 3, false);
SINGLE_BUTTON_TEST(Select, gbemu::backend::Joypad::SELECT_BUTTON, 2, false);
SINGLE_BUTTON_TEST(B, gbemu::backend::Joypad::B_BUTTON, 1, false);
SINGLE_BUTTON_TEST(A, gbemu::backend::Joypad::A_BUTTON, 0, false);
