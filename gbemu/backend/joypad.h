#ifndef GBEMU_BACKEND_JOYPAD_H
#define GBEMU_BACKEND_JOYPAD_H

#include "gbemu/backend/ram.h"

#include <map>

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

    void handleKeyDownEvent(uint32_t event);
    void handleKeyUpEvent(uint32_t event);

    [[nodiscard]] uint8_t getJoypadRegister() const;

    [[nodiscard]] uint8_t onReadOwnedByte(uint16_t address) override;
    void onWriteOwnedByte(uint16_t address, uint8_t newValue, uint8_t currentValue) override;

  private:
    static constexpr uint8_t BUTTON_DOWN = 0;
    static constexpr uint8_t BUTTON_UP = 1;

    static constexpr uint8_t SELECTED = 0;
    static constexpr uint8_t UNSELECTED = 1;

    static constexpr uint8_t START_BUTTON_BIT = 3;
    static constexpr uint8_t SELECT_BUTTON_BIT = 2;
    static constexpr uint8_t B_BUTTON_BIT = 1;
    static constexpr uint8_t A_BUTTON_BIT = 0;
    static constexpr uint8_t DOWN_BUTTON_BIT = 3;
    static constexpr uint8_t UP_BUTTON_BIT = 2;
    static constexpr uint8_t LEFT_BUTTON_BIT = 1;
    static constexpr uint8_t RIGHT_BUTTON_BIT = 0;

    static constexpr uint8_t BUTTONS_STATE_BIT = 5;
    static constexpr uint8_t DPAD_STATE_BIT = 4;

    const std::map<uint32_t, uint8_t> BUTTON_TO_BIT_MAP = {
        {START_BUTTON, START_BUTTON_BIT}, {SELECT_BUTTON, SELECT_BUTTON_BIT}, {B_BUTTON, B_BUTTON_BIT},
        {A_BUTTON, A_BUTTON_BIT},         {DOWN_BUTTON, DOWN_BUTTON_BIT},     {UP_BUTTON, UP_BUTTON_BIT},
        {LEFT_BUTTON, LEFT_BUTTON_BIT},   {RIGHT_BUTTON, RIGHT_BUTTON_BIT},
    };

    uint8_t selectedStates_;

    uint8_t buttonStates_;
    uint8_t dpadStates_;

    void handleKeyEvent(uint32_t key, uint8_t newButtonState);
};

} // namespace gbemu::backend

#endif // GBEMU_BACKEND_JOYPAD_H
