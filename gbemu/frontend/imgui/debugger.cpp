#include "gbemu/frontend/imgui/debugger.h"

#include "gbemu/backend/gameboy.h"
#include "gbemu/backend/opcode.h"
#include "gbemu/backend/ram.h"
#include "gbemu/frontend/imgui/util.h"

#include <imgui.h>

#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace gbemu::frontend
{
namespace
{

auto resolvePlaceholder(std::string_view token, uint16_t dataAddress, const backend::RAM *ram)
    -> std::optional<std::pair<std::string, uint8_t>>
{
    if (token == "a16" || token == "d16")
    {
        return std::pair{"$" + util::hex16(ram->getImmediate16(dataAddress)), uint8_t{2}};
    }
    if (token == "a8" || token == "d8" || token == "s8")
    {
        return std::pair{"$" + util::hex8(ram->get(dataAddress)), uint8_t{1}};
    }
    return std::nullopt;
}

auto formatOpcode(const backend::OPCode &opCode, uint16_t dataAddress, const backend::RAM *ram) -> std::string
{
    std::string out;
    out.reserve(opCode.command.size() + 8);

    std::string_view remaining = opCode.command;
    while (!remaining.empty())
    {
        const auto open = remaining.find('{');
        const auto close = (open == std::string_view::npos) ? std::string_view::npos : remaining.find('}', open + 1);
        if (close == std::string_view::npos)
        {
            out.append(remaining);
            break;
        }

        out.append(remaining.substr(0, open));

        const auto token = remaining.substr(open + 1, close - open - 1);
        if (const auto resolved = resolvePlaceholder(token, dataAddress, ram))
        {
            out.append(resolved->first);
            dataAddress += resolved->second;
        }
        else
        {
            out.append(remaining.substr(open, close - open + 1));
        }

        remaining.remove_prefix(close + 1);
    }

    return out;
}

} // namespace

ImguiDebugger::ImguiDebugger() : gameboy_(nullptr), in_debugger_mode_(false) {}

void ImguiDebugger::init(backend::Gameboy *gameboy) { gameboy_ = gameboy; }

void ImguiDebugger::render()
{
    if (!visible_)
    {
        return;
    }

    ImGui::Begin("Debugger", &visible_, ImGuiWindowFlags_NoCollapse);

    if (ImGui::Button(in_debugger_mode_ ? "Resume" : "Pause"))
    {
        in_debugger_mode_ = !in_debugger_mode_;
    }

    ImGui::SameLine();
    if (ImGui::Button("Step"))
    {
        gameboy_->update();
    }

    ImGui::Separator();

    const ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
                                  ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoSavedSettings;
    if (ImGui::BeginTable("debugger_table", 2, flags, ImVec2(0.0f, 0.0f)))
    {
        ImGui::TableSetupColumn("Address");
        ImGui::TableSetupColumn("Instruction");
        ImGui::TableHeadersRow();

        auto pc = gameboy_->cpu()->PC();
        const auto ram = gameboy_->ram();

        for (uint16_t row = 0; row < 100; ++row)
        {
            const auto instrPC = pc;
            auto opCodeByte = ram->get(pc);
            auto dataAddress = static_cast<uint16_t>(pc + 1);

            auto opCodesMap = backend::OPCODES;
            if (opCodeByte == backend::OPCode::PREFIX_OPCODE)
            {
                opCodeByte = ram->get(pc + 1);
                dataAddress = static_cast<uint16_t>(pc + 2);
                opCodesMap = backend::PREFIXED_OPCODES;
            }
            const auto &opCode = opCodesMap[opCodeByte];
            const auto formattedOpcode = formatOpcode(opCode, dataAddress, ram);

            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::TextDisabled("%s", util::hex16(instrPC).c_str());

            ImGui::TableNextColumn();
            ImGui::Text("%s", formattedOpcode.c_str());

            pc += opCode.bytes;
        }

        ImGui::EndTable();
    }

    ImGui::End();
}

void ImguiDebugger::renderMenuItem() { ImGui::MenuItem("Debugger", nullptr, &visible_); }

} // namespace gbemu::frontend
