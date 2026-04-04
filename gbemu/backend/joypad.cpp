#include "gbemu/backend/joypad.h"
#include "gbemu/backend/bitutils.h"

namespace gbemu::backend
{

Joypad::Joypad() : selectedStates_(0x30), buttonStates_(0x0f), dpadStates_(0x0f)
{
}

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
    if (getBit(selectedStates_, DPAD_STATE_BIT) == SELECTED)
        lowerNibble = dpadStates_;
    if (getBit(selectedStates_, BUTTONS_STATE_BIT) == SELECTED)
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
    const auto it = BUTTON_TO_BIT_MAP.find(key);
    if (it == BUTTON_TO_BIT_MAP.end())
        return;
    const auto keyBit = it->second;

    switch (key)
    {
    case START_BUTTON:
    case SELECT_BUTTON:
    case B_BUTTON:
    case A_BUTTON:
        buttonStates_ = setBit(buttonStates_, keyBit, newButtonState);
        break;
    case DOWN_BUTTON:
    case UP_BUTTON:
    case LEFT_BUTTON:
    case RIGHT_BUTTON:
        dpadStates_ = setBit(dpadStates_, keyBit, newButtonState);
        break;
    }
}

} // namespace gbemu::backend
