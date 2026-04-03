#ifndef GBEMU_FRONTEND_IMGUI_H
#define GBEMU_FRONTEND_IMGUI_H

#include "gbemu/frontend/frontend.hpp"

#include "gbemu/backend/gameboy.h"

#include <SDL2/SDL.h>

#include <array>
#include <optional>
#include <string>

struct ImVec2;

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

    void applyTheme();
    void setupDockspace();
    void renderMenuBar();
    void renderStatusBar();

    void renderScreen();
    void renderCpuWindow();
    void renderMemoryWindow();
    void renderPerformanceWindow();
    void renderAboutWindow();
    void renderWelcomeState(const ImVec2 &canvasMin, const ImVec2 &canvasMax);

    void pushFrameTimeSample(float frameTimeMs);
    void setMemoryViewBase(uint16_t address);
    void updateWindowTitle();
    void openRom();

  private:
    void pollEvents();

  private:
    [[nodiscard]] std::optional<std::string> selectRomFile();

  private:
    bool show_cpu_window_ = true;
    bool show_memory_window_ = true;
    bool show_performance_window_ = true;
    bool show_about_window_ = false;
    bool dockspace_initialized_ = false;

    bool done_ = false;

  private:
    SDL_Window *window_ = nullptr;
    SDL_Renderer *renderer_ = nullptr;
    SDL_Texture *texture_ = nullptr;

  private:
    std::array<uint32_t, gbemu::backend::WINDOW_WIDTH * gbemu::backend::WINDOW_HEIGHT> screenPixels_;
    std::array<float, 180> frame_time_history_{};

    size_t frame_time_history_offset_ = 0;
    bool frame_time_history_full_ = false;

    uint16_t memory_view_base_ = 0xff00;
    char memory_address_buffer_[5] = "FF00";

    std::string loaded_rom_path_;
    std::string status_text_ = "Open a Game Boy ROM to start emulation.";

  private:
    gbemu::backend::Gameboy *gameboy_;
};

} // namespace gbemu::frontend

#endif // GBEMU_FRONTEND_IMGUI_H
