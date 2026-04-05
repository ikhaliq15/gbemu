#include "gbemu/backend/joypad.h"

#include "gbemu/backend/bitutils.h"

namespace gbemu::backend
{

Joypad::Joypad() : selectedStates_(0x30), buttonStates_(0x0f), dpadStates_(0x0f)
{}

void Joypad::handleKeyDownEvent(uint32_t key)
{
    handleKeyEvent(key, BUTTON_DOWN);
}

void Joypad::handleKeyUpEvent(uint32_t key)
{
    handleKeyEvent(key, BUTTON_UP);
}

auto Joypad::getJoypadRegister() const -> uint8_t
{
    uint8_t lowerNibble = 0x0f;
    if (getBit(selectedStates_, DPAD_SELECT_BIT) == SELECTED)
        lowerNibble = dpadStates_;
    if (getBit(selectedStates_, BUTTONS_SELECT_BIT) == SELECTED)
        lowerNibble = buttonStates_;
    return interpolateNibbles(selectedStates_, lowerNibble);
}

auto Joypad::onReadOwnedByte(uint16_t address) -> uint8_t
{
    return getJoypadRegister();
}

void Joypad::onWriteOwnedByte(uint16_t address, uint8_t newValue, uint8_t currentValue)
{
    selectedStates_ = 0xf0 & newValue;
}

void Joypad::handleKeyEvent(uint32_t key, uint8_t newButtonState)
{
    for (const auto &mapping : KEY_MAPPINGS)
    {
        if (mapping.key != key)
            continue;

        if (mapping.isDpad)
            dpadStates_ = setBit(dpadStates_, mapping.bit, newButtonState);
        else
            buttonStates_ = setBit(buttonStates_, mapping.bit, newButtonState);
        return;
    }
}

} // namespace gbemu::backend
