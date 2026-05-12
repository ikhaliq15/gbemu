#ifndef GBEMU_FRONTEND_IMGUI_DEBUGGER_H
#define GBEMU_FRONTEND_IMGUI_DEBUGGER_H

#include "gbemu/frontend/frontend.h"
#include "gbemu/frontend/imgui/window.h"

namespace gbemu::backend
{

class Gameboy;

} // namespace gbemu::backend

namespace gbemu::frontend
{

class ImguiDebugger : public ImguiWindow
{
  public:
    ImguiDebugger();

    void init(backend::Gameboy *gameboy);

    void render();
    void renderMenuItem();

    bool inDebuggerMode() const { return in_debugger_mode_; }

  private:
    backend::Gameboy *gameboy_;

    bool in_debugger_mode_;
};

} // namespace gbemu::frontend

#endif // GBEMU_FRONTEND_IMGUI_DEBUGGER_H
