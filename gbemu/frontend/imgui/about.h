#ifndef GBEMU_FRONTEND_IMGUI_ABOUT_H
#define GBEMU_FRONTEND_IMGUI_ABOUT_H

#include "gbemu/frontend/imgui/window.h"

namespace gbemu::frontend
{

class ImguiAboutWindow : public ImguiWindow
{
  public:
    ImguiAboutWindow();

    void render();
    void renderMenuItem();
};

} // namespace gbemu::frontend

#endif // GBEMU_FRONTEND_IMGUI_ABOUT_H
