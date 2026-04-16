#ifndef GBEMU_BACKEND_JOYPAD_H
#define GBEMU_BACKEND_JOYPAD_H

#include "gbemu/backend/ram.h"

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

    void buttonPressed(Button button) { handleButtonEvent(button, true); }
    void buttonReleased(Button button) { handleButtonEvent(button, false); }

    // RAM::Owner
    auto onReadOwnedByte(uint16_t address) -> uint8_t override;
    void onWriteOwnedByte(uint16_t address, uint8_t newValue, uint8_t currentValue) override;

  private:
    static constexpr uint8_t BUTTONS_SELECT_BIT = 5;
    static constexpr uint8_t DPAD_SELECT_BIT = 4;
    static constexpr uint8_t ALL_RELEASED = 0x0f;

    [[nodiscard]] auto joypadRegister() const -> uint8_t;
    void handleButtonEvent(Button button, bool pressed);

    uint8_t selectBits_ = 0x30;
    uint8_t buttonStates_ = ALL_RELEASED;
    uint8_t dpadStates_ = ALL_RELEASED;
};

} // namespace gbemu::backend

#endif // GBEMU_BACKEND_JOYPAD_H
