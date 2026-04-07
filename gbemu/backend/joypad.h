#ifndef GBEMU_BACKEND_JOYPAD_H
#define GBEMU_BACKEND_JOYPAD_H

#include "gbemu/backend/ram.h"

#include <array>
#include <cstdint>

namespace gbemu::backend
{

class Joypad : public RAM::Owner
{
  public:
    enum class Button : uint8_t
    {
        A,
        B,
        SELECT,
        START,
        RIGHT,
        LEFT,
        UP,
        DOWN
    };

    Joypad();

    void buttonPressed(Button key) { handleButtonEvent(key, true); }
    void buttonReleased(Button key) { handleButtonEvent(key, false); }

    // RAM::Owner
    uint8_t onReadOwnedByte(uint16_t address) override;
    void onWriteOwnedByte(uint16_t address, uint8_t newValue, uint8_t currentValue) override;

  private:
    // Bit positions within the button/dpad state bytes
    static constexpr uint8_t BIT_A = 0;
    static constexpr uint8_t BIT_B = 1;
    static constexpr uint8_t BIT_SELECT = 2;
    static constexpr uint8_t BIT_START = 3;
    static constexpr uint8_t BIT_RIGHT = 0;
    static constexpr uint8_t BIT_LEFT = 1;
    static constexpr uint8_t BIT_UP = 2;
    static constexpr uint8_t BIT_DOWN = 3;

    // JOYP register select bits
    static constexpr uint8_t BUTTONS_SELECT_BIT = 5;
    static constexpr uint8_t DPAD_SELECT_BIT = 4;

    struct KeyData final
    {
        Button key_;
        uint8_t bit_;
        bool isDpad_;
    };

    static constexpr std::array<KeyData, 8> KEY_MAPPINGS = {{
        {Button::A, BIT_A, false},
        {Button::B, BIT_B, false},
        {Button::SELECT, BIT_SELECT, false},
        {Button::START, BIT_START, false},
        {Button::RIGHT, BIT_RIGHT, true},
        {Button::LEFT, BIT_LEFT, true},
        {Button::UP, BIT_UP, true},
        {Button::DOWN, BIT_DOWN, true},
    }};

    [[nodiscard]] uint8_t joypadRegister() const;
    void handleButtonEvent(Button key, bool pressed);

    uint8_t selectedStates_;
    uint8_t buttonStates_;
    uint8_t dpadStates_;
};

} // namespace gbemu::backend

#endif // GBEMU_BACKEND_JOYPAD_H
