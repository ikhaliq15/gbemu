#ifndef GBEMU_FRONTEND_HEADLESS_H
#define GBEMU_FRONTEND_HEADLESS_H

#include "gbemu/backend/gameboy.h"
#include "gbemu/frontend/frontend.hpp"

#include <SDL2/SDL.h>

#include <cstdio>

namespace gbemu::frontend
{

class HeadlessFrontend : public IFrontend
{
  public:
    explicit HeadlessFrontend(bool logSerial = false) : logSerial_(logSerial) {}

    bool init(gbemu::backend::Gameboy *gameboy) override
    {
        gameboy_ = gameboy;
        SDL_Init(SDL_INIT_GAMECONTROLLER);
        return true;
    }

    bool update() override
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                return false;
            }
        }

        if (logSerial_)
        {
            while (const auto byte = gameboy_->consumeSerialByte())
            {
                putc(*byte, stdout);
                fflush(stdout);
            }
        }

        return true;
    }

    void done() override { SDL_Quit(); }

  private:
    bool logSerial_;
    gbemu::backend::Gameboy *gameboy_ = nullptr;
};

} // namespace gbemu::frontend

#endif // GBEMU_FRONTEND_HEADLESS_H
