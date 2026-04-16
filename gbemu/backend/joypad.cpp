#include "gbemu/backend/joypad.h"

#include "gbemu/backend/bitutils.h"

namespace gbemu::backend
{

auto Joypad::onReadOwnedByte(uint16_t address) -> uint8_t { return joypadRegister(); }

void Joypad::onWriteOwnedByte(uint16_t address, uint8_t newValue, uint8_t currentValue)
{
    selectBits_ = 0xf0 & newValue;
}

auto Joypad::joypadRegister() const -> uint8_t
{
    uint8_t lowerNibble = ALL_RELEASED;
    if (!getBit(selectBits_, DPAD_SELECT_BIT))
    {
        lowerNibble = dpadStates_;
    }
    if (!getBit(selectBits_, BUTTONS_SELECT_BIT))
    {
        lowerNibble = buttonStates_;
    }
    return interpolateNibbles(selectBits_, lowerNibble);
}

void Joypad::handleButtonEvent(Button button, bool pressed)
{
    const auto index = static_cast<uint8_t>(button);
    auto &states = index >= 4 ? dpadStates_ : buttonStates_;
    states = setBit(states, index & 0x03, !pressed);
}

} // namespace gbemu::backend
