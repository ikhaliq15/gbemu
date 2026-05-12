#ifndef GBEMU_FRONTEND_IMGUI_FRONTEND_H
#define GBEMU_FRONTEND_IMGUI_FRONTEND_H

#include "gbemu/frontend/frontend.h"

#include <SDL2/SDL.h>

#include <memory>
#include <optional>
#include <string>

struct ImVec2;

namespace gbemu::backend
{
class Gameboy;
}

namespace gbemu::frontend
{

class ImguiAboutWindow;
class ImguiCpuWindow;
class ImguiDebugger;
class ImguiMemoryWindow;
class ImguiPerformanceWindow;

class ImguiFrontend : public IFrontend
{
  public:
    ImguiFrontend();
    ~ImguiFrontend() override;

    auto init(gbemu::backend::Gameboy *gameboy) -> bool override;
    auto update() -> FrontEndMode override;
    void done() override;

  private:
    void startRender();
    void finishRender();

    void applyTheme();
    void setupDockspace();
    void renderMenuBar();

    void renderScreen();
    void renderWelcomeState(const ImVec2 &canvasMin, const ImVec2 &canvasMax);

    void updateWindowTitle();
    void openRom();

    void pollEvents();

    [[nodiscard]] auto selectRomFile() -> std::optional<std::string>;

    bool dockspace_initialized_ = false;
    bool done_ = false;

    std::unique_ptr<gbemu::frontend::ImguiCpuWindow> cpuWindow_;
    std::unique_ptr<gbemu::frontend::ImguiPerformanceWindow> performanceWindow_;
    std::unique_ptr<gbemu::frontend::ImguiMemoryWindow> memoryWindow_;
    std::unique_ptr<gbemu::frontend::ImguiDebugger> debugger_;
    std::unique_ptr<gbemu::frontend::ImguiAboutWindow> aboutWindow_;

    SDL_Window *window_ = nullptr;
    SDL_Renderer *renderer_ = nullptr;
    SDL_Texture *texture_ = nullptr;

    std::string loaded_rom_path_;
    std::string status_text_ = "Open a Game Boy ROM to start emulation.";

    gbemu::backend::Gameboy *gameboy_ = nullptr;
};

} // namespace gbemu::frontend

#endif // GBEMU_FRONTEND_IMGUI_FRONTEND_H
