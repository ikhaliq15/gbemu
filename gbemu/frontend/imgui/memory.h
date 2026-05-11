#ifndef GBEMU_FRONTEND_IMGUI_MEMORY_H
#define GBEMU_FRONTEND_IMGUI_MEMORY_H

#include "gbemu/frontend/imgui/window.h"

#include <cstdint>
#include <cstdio>

namespace gbemu::backend
{

class Gameboy;

} // namespace gbemu::backend

namespace gbemu::frontend
{

class ImguiMemoryWindow : public ImguiWindow
{
  public:
    ImguiMemoryWindow();

    void init(backend::Gameboy *gameboy);

    void render();
    void renderMenuItem();

    void setMemoryViewBase(uint16_t address)
    {
        memory_view_base_ = static_cast<uint16_t>(address & 0xfff0);
        std::snprintf(memory_address_buffer_, sizeof(memory_address_buffer_), "%04X", memory_view_base_);
    }

  private:
    static constexpr uint16_t MEMORY_ROWS = 16;
    static constexpr uint16_t MEMORY_COLUMNS = 16;

    backend::Gameboy *gameboy_;

    uint16_t memory_view_base_ = 0xff00;
    char memory_address_buffer_[5] = "FF00";
};

} // namespace gbemu::frontend

#endif // GBEMU_FRONTEND_IMGUI_MEMORY_H
