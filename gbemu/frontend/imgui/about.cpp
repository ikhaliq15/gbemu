#include "gbemu/frontend/imgui/about.h"

#include "gbemu/config/version.h"
#include "gbemu/frontend/imgui/util.h"

#include <imgui.h>

namespace gbemu::frontend
{

ImguiAboutWindow::ImguiAboutWindow() {}

void ImguiAboutWindow::render()
{
    if (!visible_)
    {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(430.0f, 0.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("About GBEmu", &visible_, ImGuiWindowFlags_NoCollapse);

    ImGui::TextUnformatted(gbemu::config::kAppName);
    ImGui::Text("Version %s", gbemu::config::kVersion);

    ImGui::SeparatorText("Controls");
    ImGui::BulletText("Open ROM: %s", util::OPEN_SHORTCUT);
    ImGui::BulletText("D-pad: Arrow keys");
    ImGui::BulletText("A / B: A and B keys");
    ImGui::BulletText("Start / Select: Enter and Space");

    ImGui::End();
}

void ImguiAboutWindow::renderMenuItem() { ImGui::MenuItem("About GBEmu", nullptr, &visible_); }

} // namespace gbemu::frontend
