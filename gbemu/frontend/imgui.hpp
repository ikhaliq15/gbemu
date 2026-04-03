#ifndef GBEMU_FRONTEND_IMGUI_H
#define GBEMU_FRONTEND_IMGUI_H

#include "gbemu/frontend/frontend.hpp"

#include "gbemu/backend/gameboy.h"

#include <SDL2/SDL.h>

namespace gbemu::backend
{
class Gameboy;
}

namespace gbemu::frontend
{

class ImguiFrontend : public IFrontend
{
  public:
    bool init(gbemu::backend::Gameboy *gameboy) override;
    bool update() override;
    void done() override;

  private:
    void startRender();
    void finishRender();

    void setupDockspace();

    void renderScreen();
    void renderDebug();

  private:
    void pollEvents();

  private:
    [[nodiscard]] std::optional<std::string> selectRomFile();

  private:
    bool show_demo_window_ = true;
    bool show_another_window_ = false;

    bool done_ = false;

  private:
    SDL_Window *window_ = nullptr;
    SDL_Renderer *renderer_ = nullptr;
    SDL_Texture *texture_ = nullptr;

  private:
    std::array<uint32_t, gbemu::backend::WINDOW_WIDTH * gbemu::backend::WINDOW_HEIGHT> screenPixels_;

  private:
    gbemu::backend::Gameboy *gameboy_;
};

} // namespace gbemu::frontend

#endif // GBEMU_FRONTEND_IMGUI_H
