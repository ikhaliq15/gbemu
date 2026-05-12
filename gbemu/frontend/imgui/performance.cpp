#include "gbemu/frontend/imgui/performance.h"

#include "gbemu/backend/gameboy.h"
#include "gbemu/frontend/imgui/util.h"

#include <imgui.h>

namespace gbemu::frontend
{

ImguiPerformanceWindow::ImguiPerformanceWindow() : gameboy_(nullptr) {}

void ImguiPerformanceWindow::init(backend::Gameboy *gameboy) { gameboy_ = gameboy; }

void ImguiPerformanceWindow::render()
{
    if (!visible_)
    {
        return;
    }

    const ImGuiIO &io = ImGui::GetIO();
    const auto *ram = gameboy_->ram();

    std::array<float, FRAME_HISTORY_SIZE> plot_values{};
    size_t sample_count = frame_time_history_full_ ? frame_time_history_.size() : frame_time_history_offset_;
    if (frame_time_history_full_)
    {
        for (size_t i = 0; i < frame_time_history_.size(); ++i)
        {
            plot_values[i] = frame_time_history_[(frame_time_history_offset_ + i) % frame_time_history_.size()];
        }
    }
    else
    {
        std::copy(frame_time_history_.begin(), frame_time_history_.begin() + sample_count, plot_values.begin());
    }

    float max_frame_time = 16.0f;
    for (size_t i = 0; i < sample_count; ++i)
    {
        max_frame_time = std::max(max_frame_time, plot_values[i]);
    }

    ImGui::Begin("Performance", &visible_, ImGuiWindowFlags_NoCollapse);

    ImGui::SeparatorText("Realtime");
    if (ImGui::BeginTable("perf_metrics", 2, ImGuiTableFlags_SizingStretchSame))
    {
        const float frame_time_ms = (io.Framerate > 0.0f) ? (1000.0f / io.Framerate) : 0.0f;
        char frame_time_buffer[16];
        std::snprintf(frame_time_buffer, sizeof(frame_time_buffer), "%.2f ms", frame_time_ms);
        util::renderRegisterRow("Frame Time", sample_count > 0 ? std::string(frame_time_buffer) : std::string("--"));
        util::renderRegisterRow("FPS", sample_count > 0 ? std::to_string((int)io.Framerate) : std::string("--"));
        util::renderRegisterRow("DIV", util::hex8(ram->get(backend::RAM::DIV)));
        ImGui::EndTable();
    }

    ImGui::SeparatorText("Frame History");
    if (sample_count > 0)
    {
        ImGui::PlotLines("##frame_history", plot_values.data(), (int)sample_count, 0, nullptr, 0.0f, max_frame_time,
                         ImVec2(-1.0f, 90.0f));
    }
    else
    {
        ImGui::TextDisabled("Frame timing will appear once the renderer has accumulated a few frames.");
    }

    ImGui::SeparatorText("Notes");
    ImGui::TextWrapped("VSync is enabled, and the display panel scales the LCD output to fit without distorting the "
                       "Game Boy aspect ratio.");

    ImGui::End();
}

void ImguiPerformanceWindow::renderMenuItem() { ImGui::MenuItem("Performance", nullptr, &visible_); }

void ImguiPerformanceWindow::pushFrameTimeSample(float frameTimeMs)
{
    frame_time_history_[frame_time_history_offset_] = frameTimeMs;
    frame_time_history_offset_ = (frame_time_history_offset_ + 1) % frame_time_history_.size();
    frame_time_history_full_ = frame_time_history_full_ || frame_time_history_offset_ == 0;
}

} // namespace gbemu::frontend
