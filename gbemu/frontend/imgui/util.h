#ifndef GBEMU_FRONTEND_IMGUI_UTIL_H
#define GBEMU_FRONTEND_IMGUI_UTIL_H

#include <imgui.h>

#include <SDL2/SDL.h>

#include <cstdint>
#include <string>

namespace gbemu::frontend::util
{

inline auto hex8(uint8_t value) -> std::string
{
    char buffer[3];
    std::snprintf(buffer, sizeof(buffer), "%02X", value);
    return buffer;
}

inline auto hex16(uint16_t value) -> std::string
{
    char buffer[5];
    std::snprintf(buffer, sizeof(buffer), "%04X", value);
    return buffer;
}

inline void renderRegisterRow(const char *name, const std::string &value)
{
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextDisabled("%s", name);
    ImGui::TableNextColumn();
    ImGui::Text("%s", value.c_str());
}

#ifdef __APPLE__
constexpr auto *OPEN_SHORTCUT = "Cmd+O";
constexpr auto *QUIT_SHORTCUT = "Cmd+Q";
constexpr auto PRIMARY_SHORTCUT_MODIFIER = KMOD_GUI;
#else
constexpr auto *OPEN_SHORTCUT = "Ctrl+O";
constexpr auto *QUIT_SHORTCUT = "Ctrl+Q";
constexpr auto PRIMARY_SHORTCUT_MODIFIER = KMOD_CTRL;
#endif

} // namespace gbemu::frontend::util

#endif // GBEMU_FRONTEND_IMGUI_UTIL_H
