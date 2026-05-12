#ifndef GBEMU_FRONTEND_IMGUI_CPU_H
#define GBEMU_FRONTEND_IMGUI_CPU_H

#include "gbemu/frontend/imgui/window.h"

struct ImVec4;

namespace gbemu::backend
{

class Gameboy;

} // namespace gbemu::backend

namespace gbemu::frontend
{

class ImguiCpuWindow : public ImguiWindow
{
  public:
    ImguiCpuWindow();

    void init(backend::Gameboy *gameboy);

    void render();
    void renderMenuItem();

  private:
    void renderBadge(const char *label, const ImVec4 &color);

  private:
    backend::Gameboy *gameboy_;
};

} // namespace gbemu::frontend

#endif // GBEMU_FRONTEND_IMGUI_CPU_H
