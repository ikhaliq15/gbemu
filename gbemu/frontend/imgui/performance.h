#ifndef GBEMU_FRONTEND_IMGUI_PERFORMANCE_H
#define GBEMU_FRONTEND_IMGUI_PERFORMANCE_H

#include "gbemu/frontend/imgui/window.h"

#include <array>
#include <cstddef>

namespace gbemu::backend
{

class Gameboy;

} // namespace gbemu::backend

namespace gbemu::frontend
{

class ImguiPerformanceWindow : public ImguiWindow
{
  public:
    ImguiPerformanceWindow();

    void init(backend::Gameboy *gameboy);

    void render();
    void renderMenuItem();

    void pushFrameTimeSample(float frameTimeMs);

  private:
    static constexpr size_t FRAME_HISTORY_SIZE = 180;

    backend::Gameboy *gameboy_;

    std::array<float, FRAME_HISTORY_SIZE> frame_time_history_{};

    size_t frame_time_history_offset_ = 0;
    bool frame_time_history_full_ = false;
};

} // namespace gbemu::frontend

#endif // GBEMU_FRONTEND_IMGUI_PERFORMANCE_H
