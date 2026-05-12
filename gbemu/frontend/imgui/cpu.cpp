#include "gbemu/frontend/imgui/cpu.h"

#include "gbemu/backend/gameboy.h"
#include "gbemu/frontend/imgui/util.h"

#include <imgui.h>

namespace gbemu::frontend
{

ImguiCpuWindow::ImguiCpuWindow() : gameboy_(nullptr) {}

void ImguiCpuWindow::init(backend::Gameboy *gameboy) { gameboy_ = gameboy; }

void ImguiCpuWindow::render()
{
    if (!visible_)
    {
        return;
    }

    const auto *cpu = gameboy_->cpu();
    const auto *ram = gameboy_->ram();

    ImGui::Begin("CPU", &visible_, ImGuiWindowFlags_NoCollapse);

    ImGui::TextDisabled("Execution");
    renderBadge(cpu->mode() == backend::CPU::Mode::NORMAL ? "RUNNING" : "HALTED",
                cpu->mode() == backend::CPU::Mode::NORMAL ? ImVec4(0.17f, 0.45f, 0.30f, 1.0f)
                                                          : ImVec4(0.43f, 0.22f, 0.15f, 1.0f));
    ImGui::SameLine();
    renderBadge(cpu->IME() ? "IME ON" : "IME OFF",
                cpu->IME() ? ImVec4(0.20f, 0.38f, 0.52f, 1.0f) : ImVec4(0.22f, 0.23f, 0.27f, 1.0f));

    ImGui::SeparatorText("Registers");
    if (ImGui::BeginTable("cpu_registers", 2, ImGuiTableFlags_SizingStretchSame))
    {
        util::renderRegisterRow("PC", util::hex16(cpu->PC()));
        util::renderRegisterRow("SP", util::hex16(cpu->SP()));
        util::renderRegisterRow("AF", util::hex16(cpu->AF()));
        util::renderRegisterRow("BC", util::hex16(cpu->BC()));
        util::renderRegisterRow("DE", util::hex16(cpu->DE()));
        util::renderRegisterRow("HL", util::hex16(cpu->HL()));
        ImGui::EndTable();
    }

    const auto disabledControlFlagColor = ImVec4(0.17f, 0.19f, 0.22f, 1.0f);
    ImGui::SeparatorText("Flags");
    renderBadge(cpu->FlagZ() ? "Z" : "z", cpu->FlagZ() ? ImVec4(0.24f, 0.42f, 0.56f, 1.0f) : disabledControlFlagColor);
    ImGui::SameLine();
    renderBadge(cpu->FlagN() ? "N" : "n", cpu->FlagN() ? ImVec4(0.56f, 0.34f, 0.21f, 1.0f) : disabledControlFlagColor);
    ImGui::SameLine();
    renderBadge(cpu->FlagH() ? "H" : "h", cpu->FlagH() ? ImVec4(0.46f, 0.37f, 0.17f, 1.0f) : disabledControlFlagColor);
    ImGui::SameLine();
    renderBadge(cpu->FlagC() ? "C" : "c", cpu->FlagC() ? ImVec4(0.19f, 0.46f, 0.41f, 1.0f) : disabledControlFlagColor);

    ImGui::SeparatorText("Hardware");
    if (ImGui::BeginTable("cpu_hw", 2, ImGuiTableFlags_SizingStretchSame))
    {
        util::renderRegisterRow("LY", util::hex8(ram->get(backend::RAM::LY)));
        util::renderRegisterRow("SCX", util::hex8(ram->get(backend::RAM::SCX)));
        util::renderRegisterRow("SCY", util::hex8(ram->get(backend::RAM::SCY)));
        util::renderRegisterRow("STAT", util::hex8(ram->get(backend::RAM::STAT)));
        util::renderRegisterRow("IF", util::hex8(ram->get(backend::RAM::IF)));
        util::renderRegisterRow("IE", util::hex8(ram->get(backend::RAM::IE)));
        ImGui::EndTable();
    }

    ImGui::End();
}

void ImguiCpuWindow::renderMenuItem() { ImGui::MenuItem("CPU", nullptr, &visible_); }

void ImguiCpuWindow::renderBadge(const char *label, const ImVec4 &color)
{
    ImGui::PushStyleColor(ImGuiCol_Button, color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.98f, 0.99f, 0.99f, 1.00f));
    ImGui::Button(label);
    ImGui::PopStyleColor(4);
}

} // namespace gbemu::frontend
