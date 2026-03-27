#include "gbemu/joypad.h"
#include "gbemu/bitutils.h"

namespace gbemu
{

Joypad::Joypad() : selectedStates_(0x30), buttonStates_(0x0f), dpadStates_(0x0f)
{
}

void Joypad::handleKeyDownEvent(const SDL_KeyboardEvent &event)
{
    handleKeyEvent(event.keysym.sym, BUTTON_DOWN);
}

void Joypad::handleKeyUpEvent(const SDL_KeyboardEvent &event)
{
    handleKeyEvent(event.keysym.sym, BUTTON_UP);
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

void Joypad::handleKeyEvent(SDL_Keycode keyCode, uint8_t newButtonState)
{
    const auto it = BUTTON_TO_BIT_MAP.find(keyCode);
    if (it == BUTTON_TO_BIT_MAP.end())
        return;
    const auto keyBit = it->second;

    switch (keyCode)
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

} // namespace gbemu
