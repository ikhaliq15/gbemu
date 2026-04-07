#include "gbemu/backend/joypad.h"

#include "gbemu/backend/bitutils.h"

namespace gbemu::backend
{

Joypad::Joypad() : selectedStates_(0x30), buttonStates_(0x0f), dpadStates_(0x0f) {}

auto Joypad::onReadOwnedByte(uint16_t address) -> uint8_t { return joypadRegister(); }

void Joypad::onWriteOwnedByte(uint16_t address, uint8_t newValue, uint8_t currentValue)
{
    selectedStates_ = 0xf0 & newValue;
}

auto Joypad::joypadRegister() const -> uint8_t
{
    uint8_t lowerNibble = 0x0f;
    if (!getBit(selectedStates_, DPAD_SELECT_BIT))
        lowerNibble = dpadStates_;
    if (!getBit(selectedStates_, BUTTONS_SELECT_BIT))
        lowerNibble = buttonStates_;
    return interpolateNibbles(selectedStates_, lowerNibble);
}

void Joypad::handleButtonEvent(Button key, bool pressed)
{
    const auto newButtonState = pressed ? 0 : 1;

    for (const auto &mapping : KEY_MAPPINGS)
    {
        if (mapping.key_ != key)
            continue;

        if (mapping.isDpad_)
            dpadStates_ = setBit(dpadStates_, mapping.bit_, newButtonState);
        else
            buttonStates_ = setBit(buttonStates_, mapping.bit_, newButtonState);
        return;
    }
}

} // namespace gbemu::backend
