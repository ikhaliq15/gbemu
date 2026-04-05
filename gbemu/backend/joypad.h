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
    static constexpr uint32_t START_BUTTON = '\r';
    static constexpr uint32_t SELECT_BUTTON = ' ';
    static constexpr uint32_t B_BUTTON = 'b';
    static constexpr uint32_t A_BUTTON = 'a';
    static constexpr uint32_t DOWN_BUTTON = 81 | (1 << 30);
    static constexpr uint32_t UP_BUTTON = 82 | (1 << 30);
    static constexpr uint32_t LEFT_BUTTON = 80 | (1 << 30);
    static constexpr uint32_t RIGHT_BUTTON = 79 | (1 << 30);

    Joypad();

    void handleKeyDownEvent(uint32_t key);
    void handleKeyUpEvent(uint32_t key);

    [[nodiscard]] uint8_t getJoypadRegister() const;

    // RAM::Owner
    uint8_t onReadOwnedByte(uint16_t address) override;
    void onWriteOwnedByte(uint16_t address, uint8_t newValue, uint8_t currentValue) override;

  private:
    static constexpr uint8_t BUTTON_DOWN = 0;
    static constexpr uint8_t BUTTON_UP = 1;

    static constexpr uint8_t SELECTED = 0;
    static constexpr uint8_t UNSELECTED = 1;

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

    struct KeyMapping
    {
        uint32_t key;
        uint8_t bit;
        bool isDpad;
    };

    static constexpr std::array<KeyMapping, 8> KEY_MAPPINGS = {{
        {A_BUTTON, BIT_A, false},
        {B_BUTTON, BIT_B, false},
        {SELECT_BUTTON, BIT_SELECT, false},
        {START_BUTTON, BIT_START, false},
        {RIGHT_BUTTON, BIT_RIGHT, true},
        {LEFT_BUTTON, BIT_LEFT, true},
        {UP_BUTTON, BIT_UP, true},
        {DOWN_BUTTON, BIT_DOWN, true},
    }};

    void handleKeyEvent(uint32_t key, uint8_t newButtonState);

    uint8_t selectedStates_;
    uint8_t buttonStates_;
    uint8_t dpadStates_;
};

} // namespace gbemu::backend

#endif // GBEMU_BACKEND_JOYPAD_H
