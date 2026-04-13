#include "gbemu/backend/joypad.h"

#include "gbemu/backend/bitutils.h"

#include <algorithm>

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
    {
        lowerNibble = dpadStates_;
    }
    if (!getBit(selectedStates_, BUTTONS_SELECT_BIT))
    {
        lowerNibble = buttonStates_;
    }
    return interpolateNibbles(selectedStates_, lowerNibble);
}

void Joypad::handleButtonEvent(Button key, bool pressed)
{
    if (const auto it = std::ranges::find(KEY_MAPPINGS, key, &KeyData::key_); it != KEY_MAPPINGS.end())
    {
        const auto keyData = *it;
        const auto newButtonState = pressed ? 0 : 1;
        if (keyData.isDpad_)
        {
            dpadStates_ = setBit(dpadStates_, keyData.bit_, newButtonState);
        }
        else
        {
            buttonStates_ = setBit(buttonStates_, keyData.bit_, newButtonState);
        }
    }
}

} // namespace gbemu::backend
