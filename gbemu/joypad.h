#ifndef GBEMU_JOYPAD
#define GBEMU_JOYPAD

#include "gbemu/ram.h"

#include <SDL2/SDL.h>
#include <map>

namespace gbemu
{

class Joypad : public RAM::Owner
{
  public:
    static constexpr SDL_Keycode START_BUTTON = SDLK_RETURN;
    static constexpr SDL_Keycode SELECT_BUTTON = SDLK_SPACE;
    static constexpr SDL_Keycode B_BUTTON = SDLK_b;
    static constexpr SDL_Keycode A_BUTTON = SDLK_a;
    static constexpr SDL_Keycode DOWN_BUTTON = SDLK_DOWN;
    static constexpr SDL_Keycode UP_BUTTON = SDLK_UP;
    static constexpr SDL_Keycode LEFT_BUTTON = SDLK_LEFT;
    static constexpr SDL_Keycode RIGHT_BUTTON = SDLK_RIGHT;

    Joypad();

    void handleKeyDownEvent(const SDL_KeyboardEvent &event);
    void handleKeyUpEvent(const SDL_KeyboardEvent &event);

    uint8_t getJoypadRegister() const;

    uint8_t onReadOwnedByte(uint16_t address);
    void onWriteOwnedByte(uint16_t address, uint8_t newValue, uint8_t currentValue);

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

    const std::map<SDL_Keycode, uint8_t> BUTTON_TO_BIT_MAP = {
        {START_BUTTON, START_BUTTON_BIT}, {SELECT_BUTTON, SELECT_BUTTON_BIT}, {B_BUTTON, B_BUTTON_BIT},
        {A_BUTTON, A_BUTTON_BIT},         {DOWN_BUTTON, DOWN_BUTTON_BIT},     {UP_BUTTON, UP_BUTTON_BIT},
        {LEFT_BUTTON, LEFT_BUTTON_BIT},   {RIGHT_BUTTON, RIGHT_BUTTON_BIT},
    };

    uint8_t selectedStates_;

    uint8_t buttonStates_;
    uint8_t dpadStates_;

    void handleKeyEvent(SDL_Keycode keyCode, uint8_t newButtonState);
};

} // namespace gbemu

#endif // GBEMU_JOYPAD
