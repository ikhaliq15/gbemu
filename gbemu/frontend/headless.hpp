#ifndef GBEMU_FRONTEND_HEADLESS_H
#define GBEMU_FRONTEND_HEADLESS_H

#include "gbemu/frontend/frontend.hpp"

#include <SDL2/SDL.h>

namespace gbemu::backend
{

class Gameboy;

} // namespace gbemu::backend

namespace gbemu::frontend
{

class HeadlessFrontend : public IFrontend
{
  public:
    bool init(gbemu::backend::Gameboy *gameboy) override
    {
        SDL_Init(SDL_INIT_GAMECONTROLLER);

        return true;
    }

    bool update() override
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                return false;
        }

        return true;
    }

    void done() override
    {
        SDL_Quit();
    }
};

} // namespace gbemu::frontend

#endif // GBEMU_FRONTEND_HEADLESS_H
