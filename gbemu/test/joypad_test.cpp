#include <gtest/gtest.h>

#include "../joypad.h"

class JoypadTest : public testing::Test
{
  protected:
    void SetUp() override
    {
        joypad_ = std::make_shared<gbemu::Joypad>();
    }

    SDL_KeyboardEvent getKeyboardEvent(const SDL_Keycode keyCode)
    {
        auto event = SDL_KeyboardEvent();
        event.keysym.sym = keyCode;
        return event;
    }

    uint8_t getButtonsNibble() const
    {
        joypad_->onWriteOwnedByte(gbemu::RAM::JOYP, 0x10, 0x00);
        const auto joyp = joypad_->onReadOwnedByte(gbemu::RAM::JOYP);
        EXPECT_EQ(joyp & 0xf0, 0x10);
        return joyp & 0x0f;
    }

    uint8_t getDpadNibble() const
    {
        joypad_->onWriteOwnedByte(gbemu::RAM::JOYP, 0x20, 0x00);
        const auto joyp = joypad_->onReadOwnedByte(gbemu::RAM::JOYP);
        EXPECT_EQ(joyp & 0xf0, 0x20);
        return joyp & 0x0f;
    }

    std::shared_ptr<gbemu::Joypad> joypad_;
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
            joypad_->handleKeyDownEvent(getKeyboardEvent(Button));                                                     \
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
            joypad_->handleKeyUpEvent(getKeyboardEvent(Button));                                                       \
                                                                                                                       \
            ASSERT_EQ(getButtonsNibble(), 0x0f);                                                                       \
            ASSERT_EQ(getDpadNibble(), 0x0f);                                                                          \
        }                                                                                                              \
    }

TEST_F(JoypadTest, NoButtonsPressed)
{
    // Neither dpad or buttons selected defaults to all buttons not pressed.
    ASSERT_EQ(joypad_->onReadOwnedByte(gbemu::RAM::JOYP), 0x3f);

    joypad_->onWriteOwnedByte(gbemu::RAM::JOYP, 0x10, 0x00);
    ASSERT_EQ(joypad_->onReadOwnedByte(gbemu::RAM::JOYP), 0x1f);

    joypad_->onWriteOwnedByte(gbemu::RAM::JOYP, 0x20, 0x00);
    ASSERT_EQ(joypad_->onReadOwnedByte(gbemu::RAM::JOYP), 0x2f);
}

SINGLE_BUTTON_TEST(Down, gbemu::Joypad::DOWN_BUTTON, 3, true);
SINGLE_BUTTON_TEST(Up, gbemu::Joypad::UP_BUTTON, 2, true);
SINGLE_BUTTON_TEST(Left, gbemu::Joypad::LEFT_BUTTON, 1, true);
SINGLE_BUTTON_TEST(Right, gbemu::Joypad::RIGHT_BUTTON, 0, true);

SINGLE_BUTTON_TEST(Start, gbemu::Joypad::START_BUTTON, 3, false);
SINGLE_BUTTON_TEST(Select, gbemu::Joypad::SELECT_BUTTON, 2, false);
SINGLE_BUTTON_TEST(B, gbemu::Joypad::B_BUTTON, 1, false);
SINGLE_BUTTON_TEST(A, gbemu::Joypad::A_BUTTON, 0, false);
