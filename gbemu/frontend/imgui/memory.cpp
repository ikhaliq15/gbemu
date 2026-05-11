#include "gbemu/frontend/imgui/memory.h"

#include "gbemu/backend/gameboy.h"
#include "gbemu/frontend/imgui/util.h"

#include <imgui.h>

namespace gbemu::frontend
{

ImguiMemoryWindow::ImguiMemoryWindow() : gameboy_(nullptr) {}

void ImguiMemoryWindow::init(backend::Gameboy *gameboy) { gameboy_ = gameboy; }

void ImguiMemoryWindow::render()
{
    if (!visible_)
    {
        return;
    }

    const auto *ram = gameboy_->ram();
    const uint16_t base = static_cast<uint16_t>(memory_view_base_ & 0xfff0);
    const uint16_t pc = gameboy_->cpu()->PC();
    const uint16_t sp = gameboy_->cpu()->SP();

    ImGui::Begin("Memory", &visible_, ImGuiWindowFlags_NoCollapse);

    if (ImGui::InputText("Base", memory_address_buffer_, IM_ARRAYSIZE(memory_address_buffer_),
                         ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase |
                             ImGuiInputTextFlags_EnterReturnsTrue))
    {
        const unsigned long address = std::strtoul(memory_address_buffer_, nullptr, 16);
        setMemoryViewBase(static_cast<uint16_t>(address));
    }
    ImGui::SameLine();
    if (ImGui::Button("Go"))
    {
        const unsigned long address = std::strtoul(memory_address_buffer_, nullptr, 16);
        setMemoryViewBase(static_cast<uint16_t>(address));
    }
    ImGui::SameLine();
    if (ImGui::Button("-0x100"))
    {
        setMemoryViewBase(static_cast<uint16_t>(base - 0x0100));
    }
    ImGui::SameLine();
    if (ImGui::Button("+0x100"))
    {
        setMemoryViewBase(static_cast<uint16_t>(base + 0x0100));
    }

    if (ImGui::Button("Follow PC"))
    {
        setMemoryViewBase(pc);
    }
    ImGui::SameLine();
    if (ImGui::Button("Follow SP"))
    {
        setMemoryViewBase(sp);
    }
    ImGui::SameLine();
    if (ImGui::Button("IO"))
    {
        setMemoryViewBase(0xff00);
    }
    ImGui::SameLine();
    if (ImGui::Button("VRAM"))
    {
        setMemoryViewBase(0x8000);
    }
    ImGui::SameLine();
    if (ImGui::Button("OAM"))
    {
        setMemoryViewBase(backend::RAM::OAM);
    }

    ImGui::Separator();

    const ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
                                  ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoSavedSettings;
    if (ImGui::BeginTable("memory_table", MEMORY_COLUMNS + 2, flags, ImVec2(0.0f, 0.0f)))
    {
        ImGui::TableSetupColumn("Addr");
        for (uint16_t column = 0; column < MEMORY_COLUMNS; ++column)
        {
            const std::string label = util::hex8(static_cast<uint8_t>(column));
            ImGui::TableSetupColumn(label.c_str());
        }
        ImGui::TableSetupColumn("ASCII");
        ImGui::TableHeadersRow();

        for (uint16_t row = 0; row < MEMORY_ROWS; ++row)
        {
            const uint16_t row_address = static_cast<uint16_t>(base + row * MEMORY_COLUMNS);
            std::string ascii;
            ascii.reserve(MEMORY_COLUMNS);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextDisabled("%s", util::hex16(row_address).c_str());

            for (uint16_t column = 0; column < MEMORY_COLUMNS; ++column)
            {
                const uint16_t address = static_cast<uint16_t>(row_address + column);
                const uint8_t value = ram->get(address);
                const bool is_pc = address == pc;
                const bool is_sp = address == sp;
                ascii.push_back((value >= 32 && value <= 126) ? static_cast<char>(value) : '.');

                ImGui::TableNextColumn();
                if (is_pc)
                {
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, IM_COL32(44, 112, 150, 90));
                }
                else if (is_sp)
                {
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, IM_COL32(52, 148, 119, 90));
                }

                ImGui::Text("%s", util::hex8(value).c_str());
            }

            ImGui::TableNextColumn();
            ImGui::TextDisabled("%s", ascii.c_str());
        }

        ImGui::EndTable();
    }

    ImGui::End();
}

void ImguiMemoryWindow::renderMenuItem() { ImGui::MenuItem("Memory", nullptr, &visible_); }

} // namespace gbemu::frontend
