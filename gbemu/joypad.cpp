#include "joypad.h"
#include "bitutils.h"

#include <iostream>

namespace gbemu {

    Joypad::Joypad()
    : buttonStates_(0x0f)
    , dpadStates_(0x0f)
    {
    }

    void Joypad::handleKeyDownEvent(const SDL_KeyboardEvent& event)
    {
        handleKeyEvent(event.keysym.sym, BUTTON_DOWN);
    }

    void Joypad::handleKeyUpEvent(const SDL_KeyboardEvent& event)
    {
        handleKeyEvent(event.keysym.sym, BUTTON_UP);
    }

    uint8_t Joypad::getJoypadRegister(uint8_t joyp) const
    {
        const auto buttonState = getBit(joyp, BUTTONS_STATE_BIT);
        const auto dpadState = getBit(joyp, DPAD_STATE_BIT);

        if (buttonState == 0)
            return interpolateNibbles(joyp, buttonStates_);

        if (dpadState == 0)
            return interpolateNibbles(joyp, dpadStates_);

        return interpolateNibbles(joyp, 0x0F);
    }

    void Joypad::handleKeyEvent(SDL_Keycode keyCode, uint8_t newButtonState)
    {
        const auto it = BUTTON_TO_BIT_MAP.find(keyCode);
        if (it == BUTTON_TO_BIT_MAP.end())
            return;
        const auto keyBit = it->second;            

        switch (keyCode) {
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

} // gbemu