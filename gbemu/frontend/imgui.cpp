#include "gbemu/frontend/imgui.hpp"

#include "gbemu/backend/cartridge.h"
#include "gbemu/backend/ppu.h"
#include "gbemu/backend/ram.h"
#include "gbemu/config/version.h"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "imgui_internal.h"

#include <nfd.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <string>

namespace gbemu::frontend
{
namespace
{

constexpr ImVec4 kClearColor = ImVec4(0.05f, 0.07f, 0.09f, 1.00f);
constexpr size_t kFrameHistorySize = 180;
constexpr uint16_t kMemoryRows = 16;
constexpr uint16_t kMemoryColumns = 16;

#ifdef __APPLE__
constexpr const char *kOpenShortcut = "Cmd+O";
constexpr const char *kQuitShortcut = "Cmd+Q";
constexpr SDL_Keymod kPrimaryShortcutModifier = KMOD_GUI;
#else
constexpr const char *kOpenShortcut = "Ctrl+O";
constexpr const char *kQuitShortcut = "Ctrl+Q";
constexpr SDL_Keymod kPrimaryShortcutModifier = KMOD_CTRL;
#endif

auto hex8(uint8_t value) -> std::string
{
    char buffer[3];
    std::snprintf(buffer, sizeof(buffer), "%02X", value);
    return buffer;
}

auto hex16(uint16_t value) -> std::string
{
    char buffer[5];
    std::snprintf(buffer, sizeof(buffer), "%04X", value);
    return buffer;
}

void renderBadge(const char *label, const ImVec4 &color)
{
    ImGui::PushStyleColor(ImGuiCol_Button, color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.98f, 0.99f, 0.99f, 1.00f));
    ImGui::Button(label);
    ImGui::PopStyleColor(4);
}

void renderRegisterRow(const char *name, const std::string &value)
{
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextDisabled("%s", name);
    ImGui::TableNextColumn();
    ImGui::Text("%s", value.c_str());
}

auto fileDisplayName(const std::string &path) -> std::string
{
    if (path.empty())
    {
        return "Cartridge loaded";
    }

    return std::filesystem::path(path).filename().string();
}

auto keycodeToButton(SDL_Keycode key) -> std::optional<gbemu::backend::Joypad::Button>
{
    switch (key)
    {
    case SDLK_a: return gbemu::backend::Joypad::Button::A;
    case SDLK_b: return gbemu::backend::Joypad::Button::B;
    case SDLK_RETURN: return gbemu::backend::Joypad::Button::START;
    case SDLK_SPACE: return gbemu::backend::Joypad::Button::SELECT;
    case SDLK_UP: return gbemu::backend::Joypad::Button::UP;
    case SDLK_DOWN: return gbemu::backend::Joypad::Button::DOWN;
    case SDLK_LEFT: return gbemu::backend::Joypad::Button::LEFT;
    case SDLK_RIGHT: return gbemu::backend::Joypad::Button::RIGHT;
    default: return std::nullopt;
    }
}

void lockDockNode(ImGuiID node_id)
{
    if (ImGuiDockNode *node = ImGui::DockBuilderGetNode(node_id))
    {
        node->LocalFlags |= ImGuiDockNodeFlags_NoDockingSplit;
        node->LocalFlags |= ImGuiDockNodeFlags_NoResize;
        node->LocalFlags |= ImGuiDockNodeFlags_NoUndocking;
        node->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;

        if (node->ChildNodes[0] != nullptr)
        {
            lockDockNode(node->ChildNodes[0]->ID);
        }
        if (node->ChildNodes[1] != nullptr)
        {
            lockDockNode(node->ChildNodes[1]->ID);
        }
    }
}

} // namespace

bool ImguiFrontend::init(gbemu::backend::Gameboy *gameboy)
{
    gameboy_ = gameboy;

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER);
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");

    const auto main_scale = ImGui_ImplSDL2_GetContentScaleForDisplay(0);

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    window_ = SDL_CreateWindow(gbemu::config::kDisplayName, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               (int)(1440 * main_scale), (int)(920 * main_scale), window_flags);

    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
    texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                 gbemu::backend::LCD_WIDTH, gbemu::backend::LCD_HEIGHT);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();

    ImGuiStyle &style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;

    applyTheme();

    ImGui_ImplSDL2_InitForSDLRenderer(window_, renderer_);
    ImGui_ImplSDLRenderer2_Init(renderer_);

    show_cpu_window_ = true;
    show_memory_window_ = true;
    show_performance_window_ = true;
    show_about_window_ = false;
    dockspace_initialized_ = false;
    done_ = false;

    if (gameboy_->cartridgeLoaded())
    {
        status_text_ = "Cartridge ready. Open another ROM at any time from the File menu.";
        setMemoryViewBase(0x0100);
    }

    updateWindowTitle();

    return true;
}

bool ImguiFrontend::update()
{
    if (done_)
    {
        return false;
    }

    pollEvents();

    startRender();
    renderMenuBar();
    setupDockspace();

    const ImGuiIO &io = ImGui::GetIO();
    if (io.Framerate > 0.0f)
    {
        pushFrameTimeSample(1000.0f / io.Framerate);
    }

    renderScreen();

    if (show_cpu_window_)
    {
        renderCpuWindow();
    }
    if (show_memory_window_)
    {
        renderMemoryWindow();
    }
    if (show_performance_window_)
    {
        renderPerformanceWindow();
    }
    if (show_about_window_)
    {
        renderAboutWindow();
    }

    finishRender();

    return true;
}

void ImguiFrontend::startRender()
{
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

void ImguiFrontend::finishRender()
{
    ImGuiIO &io = ImGui::GetIO();

    ImGui::Render();
    SDL_RenderSetScale(renderer_, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
    SDL_SetRenderDrawColor(renderer_, (Uint8)(kClearColor.x * 255), (Uint8)(kClearColor.y * 255),
                           (Uint8)(kClearColor.z * 255), (Uint8)(kClearColor.w * 255));
    SDL_RenderClear(renderer_);
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer_);

    SDL_RenderPresent(renderer_);
}

void ImguiFrontend::applyTheme()
{
    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(14.0f, 12.0f);
    style.FramePadding = ImVec2(10.0f, 7.0f);
    style.ItemSpacing = ImVec2(10.0f, 8.0f);
    style.ItemInnerSpacing = ImVec2(8.0f, 6.0f);
    style.WindowRounding = 14.0f;
    style.ChildRounding = 12.0f;
    style.FrameRounding = 9.0f;
    style.PopupRounding = 10.0f;
    style.ScrollbarRounding = 10.0f;
    style.GrabRounding = 10.0f;
    style.TabRounding = 10.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.TabBorderSize = 0.0f;

    ImVec4 *colors = style.Colors;
    colors[ImGuiCol_Text] = ImVec4(0.93f, 0.96f, 0.97f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.58f, 0.62f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.08f, 0.10f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.08f, 0.10f, 0.13f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.09f, 0.12f, 0.15f, 0.98f);
    colors[ImGuiCol_Border] = ImVec4(0.18f, 0.24f, 0.28f, 1.00f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.14f, 0.18f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.15f, 0.22f, 0.27f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.17f, 0.26f, 0.31f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.06f, 0.09f, 0.11f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.13f, 0.16f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.05f, 0.07f, 0.09f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.07f, 0.09f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.18f, 0.28f, 0.33f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.23f, 0.36f, 0.40f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.27f, 0.42f, 0.46f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.39f, 0.83f, 0.69f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.30f, 0.72f, 0.61f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.39f, 0.83f, 0.69f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.13f, 0.27f, 0.25f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.18f, 0.37f, 0.33f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.22f, 0.47f, 0.41f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.12f, 0.26f, 0.24f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.17f, 0.35f, 0.31f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.21f, 0.43f, 0.38f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.18f, 0.24f, 0.28f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.16f, 0.27f, 0.30f, 0.35f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.24f, 0.42f, 0.44f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.30f, 0.52f, 0.56f, 0.95f);
    colors[ImGuiCol_Tab] = ImVec4(0.09f, 0.14f, 0.17f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.18f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.14f, 0.23f, 0.24f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.07f, 0.10f, 0.13f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.10f, 0.17f, 0.18f, 1.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.39f, 0.83f, 0.69f, 0.28f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.04f, 0.05f, 0.06f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.09f, 0.13f, 0.17f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.16f, 0.21f, 0.24f, 1.00f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.11f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.02f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.30f, 0.72f, 0.61f, 0.35f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.39f, 0.83f, 0.69f, 1.00f);
}

void ImguiFrontend::setupDockspace()
{
    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    const ImGuiID dockspace_id = ImGui::GetID("GBEmu DockSpace");

    ImGui::DockSpaceOverViewport(dockspace_id, viewport, ImGuiDockNodeFlags_None);

    if (!dockspace_initialized_)
    {
        dockspace_initialized_ = true;
        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

        ImGuiID dock_main = dockspace_id;
        ImGuiID dock_right = 0;
        ImGuiID dock_bottom = 0;
        ImGuiID dock_right_bottom = 0;

        ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Right, 0.29f, &dock_right, &dock_main);
        ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Down, 0.32f, &dock_bottom, &dock_main);
        ImGui::DockBuilderSplitNode(dock_right, ImGuiDir_Down, 0.40f, &dock_right_bottom, &dock_right);

        ImGui::DockBuilderDockWindow("Screen", dock_main);
        ImGui::DockBuilderDockWindow("Memory", dock_bottom);
        ImGui::DockBuilderDockWindow("CPU", dock_right);
        ImGui::DockBuilderDockWindow("Performance", dock_right_bottom);
        ImGui::DockBuilderFinish(dockspace_id);
    }

    lockDockNode(dockspace_id);
}

void ImguiFrontend::renderMenuBar()
{
    if (!ImGui::BeginMainMenuBar())
    {
        return;
    }

    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("Open ROM...", kOpenShortcut))
        {
            openRom();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Quit", kQuitShortcut))
        {
            done_ = true;
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View"))
    {
        ImGui::MenuItem("CPU", nullptr, &show_cpu_window_);
        ImGui::MenuItem("Memory", nullptr, &show_memory_window_);
        ImGui::MenuItem("Performance", nullptr, &show_performance_window_);
        ImGui::Separator();
        if (ImGui::MenuItem("Restore Default Layout"))
        {
            dockspace_initialized_ = false;
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Help"))
    {
        ImGui::MenuItem("About GBEmu", nullptr, &show_about_window_);
        ImGui::EndMenu();
    }

    const std::string rom_name =
        gameboy_->cartridgeLoaded() ? fileDisplayName(loaded_rom_path_) : std::string("No ROM Loaded");
    const float rom_name_width = ImGui::CalcTextSize(rom_name.c_str()).x;
    const float remaining_space = ImGui::GetContentRegionAvail().x;
    if (remaining_space > rom_name_width)
    {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + remaining_space - rom_name_width);
    }
    ImGui::TextDisabled("%s", rom_name.c_str());
    ImGui::EndMainMenuBar();
}

void ImguiFrontend::renderScreen()
{
    ImGui::Begin("Screen", nullptr,
                 ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse);

    const ImVec2 available = ImGui::GetContentRegionAvail();
    ImGui::Dummy(available);
    const ImVec2 canvas_min = ImGui::GetItemRectMin();
    const ImVec2 canvas_max = ImGui::GetItemRectMax();

    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(canvas_min, canvas_max, IM_COL32(15, 20, 25, 255), 16.0f);
    draw_list->AddRect(canvas_min, canvas_max, IM_COL32(43, 61, 70, 255), 16.0f, 0, 1.5f);

    if (!gameboy_->cartridgeLoaded())
    {
        renderWelcomeState(canvas_min, canvas_max);
        ImGui::End();
        return;
    }

    const auto &pixels = gameboy_->ppu()->getPixels();
    SDL_UpdateTexture(texture_, nullptr, pixels.data(), gbemu::backend::LCD_WIDTH * sizeof(uint32_t));

    const float horizontal_padding = 28.0f;
    const float vertical_padding = 32.0f;
    const float max_width = std::max(available.x - horizontal_padding * 2.0f, 1.0f);
    const float max_height = std::max(available.y - vertical_padding * 2.0f, 1.0f);
    const float scale =
        std::max(0.1f, std::min(max_width / gbemu::backend::LCD_WIDTH, max_height / gbemu::backend::LCD_HEIGHT));

    const ImVec2 image_size(gbemu::backend::LCD_WIDTH * scale, gbemu::backend::LCD_HEIGHT * scale);
    const ImVec2 image_pos(canvas_min.x + (available.x - image_size.x) * 0.5f,
                           canvas_min.y + (available.y - image_size.y) * 0.5f);
    const ImVec2 frame_padding(12.0f, 12.0f);

    draw_list->AddRectFilled(
        ImVec2(image_pos.x - frame_padding.x, image_pos.y - frame_padding.y),
        ImVec2(image_pos.x + image_size.x + frame_padding.x, image_pos.y + image_size.y + frame_padding.y),
        IM_COL32(8, 10, 13, 255), 14.0f);
    draw_list->AddRect(
        ImVec2(image_pos.x - frame_padding.x, image_pos.y - frame_padding.y),
        ImVec2(image_pos.x + image_size.x + frame_padding.x, image_pos.y + image_size.y + frame_padding.y),
        IM_COL32(67, 94, 107, 255), 14.0f, 0, 2.0f);

    ImGui::SetCursorScreenPos(image_pos);
    ImGui::Image(texture_, image_size);
    ImGui::End();
}

void ImguiFrontend::renderCpuWindow()
{
    const auto *cpu = gameboy_->cpu();
    const auto *ram = gameboy_->ram();

    ImGui::Begin("CPU", &show_cpu_window_, ImGuiWindowFlags_NoCollapse);

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
        renderRegisterRow("PC", hex16(cpu->PC()));
        renderRegisterRow("SP", hex16(cpu->SP()));
        renderRegisterRow("AF", hex16(cpu->AF()));
        renderRegisterRow("BC", hex16(cpu->BC()));
        renderRegisterRow("DE", hex16(cpu->DE()));
        renderRegisterRow("HL", hex16(cpu->HL()));
        ImGui::EndTable();
    }

    ImGui::SeparatorText("Flags");
    renderBadge(cpu->FlagZ() ? "Z" : "z",
                cpu->FlagZ() ? ImVec4(0.24f, 0.42f, 0.56f, 1.0f) : ImVec4(0.17f, 0.19f, 0.22f, 1.0f));
    ImGui::SameLine();
    renderBadge(cpu->FlagN() ? "N" : "n",
                cpu->FlagN() ? ImVec4(0.56f, 0.34f, 0.21f, 1.0f) : ImVec4(0.17f, 0.19f, 0.22f, 1.0f));
    ImGui::SameLine();
    renderBadge(cpu->FlagH() ? "H" : "h",
                cpu->FlagH() ? ImVec4(0.46f, 0.37f, 0.17f, 1.0f) : ImVec4(0.17f, 0.19f, 0.22f, 1.0f));
    ImGui::SameLine();
    renderBadge(cpu->FlagC() ? "C" : "c",
                cpu->FlagC() ? ImVec4(0.19f, 0.46f, 0.41f, 1.0f) : ImVec4(0.17f, 0.19f, 0.22f, 1.0f));

    ImGui::SeparatorText("Next Bytes");
    ImGui::Text("PC: %s  %s %s %s %s", hex16(cpu->PC()).c_str(), hex8(ram->get(cpu->PC())).c_str(),
                hex8(ram->get(cpu->PC() + 1)).c_str(), hex8(ram->get(cpu->PC() + 2)).c_str(),
                hex8(ram->get(cpu->PC() + 3)).c_str());
    ImGui::Text("Cycles: %llu", static_cast<unsigned long long>(cpu->cycles()));

    ImGui::SeparatorText("Hardware");
    if (ImGui::BeginTable("cpu_hw", 2, ImGuiTableFlags_SizingStretchSame))
    {
        renderRegisterRow("LY", hex8(ram->get(backend::RAM::LY)));
        renderRegisterRow("SCX", hex8(ram->get(backend::RAM::SCX)));
        renderRegisterRow("SCY", hex8(ram->get(backend::RAM::SCY)));
        renderRegisterRow("STAT", hex8(ram->get(backend::RAM::STAT)));
        renderRegisterRow("IF", hex8(ram->get(backend::RAM::IF)));
        renderRegisterRow("IE", hex8(ram->get(backend::RAM::IE)));
        ImGui::EndTable();
    }

    ImGui::End();
}

void ImguiFrontend::renderMemoryWindow()
{
    const auto *ram = gameboy_->ram();
    const uint16_t base = static_cast<uint16_t>(memory_view_base_ & 0xfff0);
    const uint16_t pc = gameboy_->cpu()->PC();
    const uint16_t sp = gameboy_->cpu()->SP();

    ImGui::Begin("Memory", &show_memory_window_, ImGuiWindowFlags_NoCollapse);

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
    if (ImGui::BeginTable("memory_table", 18, flags, ImVec2(0.0f, 0.0f)))
    {
        ImGui::TableSetupColumn("Addr");
        for (uint16_t column = 0; column < kMemoryColumns; ++column)
        {
            const std::string label = hex8(static_cast<uint8_t>(column));
            ImGui::TableSetupColumn(label.c_str());
        }
        ImGui::TableSetupColumn("ASCII");
        ImGui::TableHeadersRow();

        for (uint16_t row = 0; row < kMemoryRows; ++row)
        {
            const uint16_t row_address = static_cast<uint16_t>(base + row * kMemoryColumns);
            std::string ascii;
            ascii.reserve(kMemoryColumns);

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextDisabled("%s", hex16(row_address).c_str());

            for (uint16_t column = 0; column < kMemoryColumns; ++column)
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

                ImGui::Text("%s", hex8(value).c_str());
            }

            ImGui::TableNextColumn();
            ImGui::TextDisabled("%s", ascii.c_str());
        }

        ImGui::EndTable();
    }

    ImGui::End();
}

void ImguiFrontend::renderPerformanceWindow()
{
    const ImGuiIO &io = ImGui::GetIO();
    const auto *cpu = gameboy_->cpu();
    const auto *ram = gameboy_->ram();

    std::array<float, kFrameHistorySize> plot_values{};
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

    ImGui::Begin("Performance", &show_performance_window_, ImGuiWindowFlags_NoCollapse);

    ImGui::SeparatorText("Realtime");
    if (ImGui::BeginTable("perf_metrics", 2, ImGuiTableFlags_SizingStretchSame))
    {
        const float frame_time_ms = (io.Framerate > 0.0f) ? (1000.0f / io.Framerate) : 0.0f;
        char frame_time_buffer[16];
        std::snprintf(frame_time_buffer, sizeof(frame_time_buffer), "%.2f ms", frame_time_ms);
        renderRegisterRow("Frame Time", sample_count > 0 ? std::string(frame_time_buffer) : std::string("--"));
        renderRegisterRow("FPS", sample_count > 0 ? std::to_string((int)io.Framerate) : std::string("--"));
        renderRegisterRow("CPU Cycles", std::to_string(static_cast<unsigned long long>(cpu->cycles())));
        renderRegisterRow("DIV", hex8(ram->get(backend::RAM::DIV)));
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

void ImguiFrontend::renderAboutWindow()
{
    ImGui::SetNextWindowSize(ImVec2(430.0f, 0.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("About GBEmu", &show_about_window_, ImGuiWindowFlags_NoCollapse);

    ImGui::TextUnformatted(gbemu::config::kAppName);
    ImGui::Text("Version %s", gbemu::config::kVersion);

    ImGui::SeparatorText("Controls");
    ImGui::BulletText("Open ROM: %s", kOpenShortcut);
    ImGui::BulletText("D-pad: Arrow keys");
    ImGui::BulletText("A / B: A and B keys");
    ImGui::BulletText("Start / Select: Enter and Space");

    ImGui::End();
}

void ImguiFrontend::renderWelcomeState(const ImVec2 &canvasMin, const ImVec2 &canvasMax)
{
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    const ImVec2 center((canvasMin.x + canvasMax.x) * 0.5f, (canvasMin.y + canvasMax.y) * 0.5f);

    const char *headline = "Open a ROM to start emulation";
    const char *supporting = "Use File > Open ROM... to load a cartridge and begin inspecting the emulator state.";

    const ImVec2 headline_size = ImGui::CalcTextSize(headline);
    const ImVec2 supporting_size = ImGui::CalcTextSize(supporting);

    draw_list->AddCircleFilled(ImVec2(center.x, center.y - 46.0f), 34.0f, IM_COL32(38, 90, 78, 255), 32);
    draw_list->AddRectFilled(ImVec2(center.x - 12.0f, center.y - 62.0f), ImVec2(center.x + 12.0f, center.y - 30.0f),
                             IM_COL32(228, 241, 237, 255), 4.0f);
    draw_list->AddText(ImVec2(center.x - headline_size.x * 0.5f, center.y + 6.0f), IM_COL32(233, 240, 243, 255),
                       headline);
    draw_list->AddText(ImVec2(center.x - supporting_size.x * 0.5f, center.y + 30.0f), IM_COL32(135, 150, 159, 255),
                       supporting);

    const ImVec2 button_size(170.0f, 0.0f);
    ImGui::SetCursorScreenPos(ImVec2(center.x - button_size.x * 0.5f, center.y + 62.0f));
    if (ImGui::Button("Open ROM...", button_size))
    {
        openRom();
    }
}

void ImguiFrontend::pushFrameTimeSample(float frameTimeMs)
{
    frame_time_history_[frame_time_history_offset_] = frameTimeMs;
    frame_time_history_offset_ = (frame_time_history_offset_ + 1) % frame_time_history_.size();
    frame_time_history_full_ = frame_time_history_full_ || frame_time_history_offset_ == 0;
}

void ImguiFrontend::setMemoryViewBase(uint16_t address)
{
    memory_view_base_ = static_cast<uint16_t>(address & 0xfff0);
    std::snprintf(memory_address_buffer_, sizeof(memory_address_buffer_), "%04X", memory_view_base_);
}

void ImguiFrontend::updateWindowTitle()
{
    std::string title = gbemu::config::kDisplayName;
    if (gameboy_ != nullptr && gameboy_->cartridgeLoaded())
    {
        title += " - ";
        title += fileDisplayName(loaded_rom_path_);
    }
    SDL_SetWindowTitle(window_, title.c_str());
}

void ImguiFrontend::openRom()
{
    const auto rom_file = selectRomFile();
    if (!rom_file.has_value())
    {
        status_text_ = "ROM selection cancelled.";
        return;
    }

    try
    {
        gameboy_->loadCartridge(backend::Cartridge(*rom_file));
        loaded_rom_path_ = *rom_file;
        status_text_ = "Loaded " + fileDisplayName(loaded_rom_path_) + ".";
        setMemoryViewBase(0x0100);
        updateWindowTitle();
    }
    catch (const std::exception &)
    {
        status_text_ = "Failed to load the selected ROM.";
    }
}

void ImguiFrontend::renderStatusBar() {}

void ImguiFrontend::pollEvents()
{
    SDL_Event event;
    ImGuiIO &io = ImGui::GetIO();

    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL2_ProcessEvent(&event);

        if (event.type == SDL_QUIT)
        {
            done_ = true;
            continue;
        }

        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
            event.window.windowID == SDL_GetWindowID(window_))
        {
            done_ = true;
            continue;
        }

        if (event.type == SDL_KEYDOWN)
        {
            const bool has_primary_shortcut = (event.key.keysym.mod & kPrimaryShortcutModifier) != 0;
            if (has_primary_shortcut && event.key.repeat == 0)
            {
                switch (event.key.keysym.sym)
                {
                case SDLK_o: openRom(); continue;
                case SDLK_q: done_ = true; continue;
                case SDLK_1: show_cpu_window_ = !show_cpu_window_; continue;
                case SDLK_2: show_memory_window_ = !show_memory_window_; continue;
                case SDLK_3: show_performance_window_ = !show_performance_window_; continue;
                default: break;
                }
            }

            const auto joypadButton = keycodeToButton(event.key.keysym.sym);
            if (joypadButton.has_value() && !io.WantCaptureKeyboard)
            {
                gameboy_->buttonPressed(joypadButton.value());
            }
            continue;
        }

        if (event.type == SDL_KEYUP)
        {
            const auto joypadButton = keycodeToButton(event.key.keysym.sym);
            if (joypadButton.has_value() && !io.WantCaptureKeyboard)
            {
                gameboy_->buttonReleased(joypadButton.value());
            }
        }
    }
}

std::optional<std::string> ImguiFrontend::selectRomFile()
{
    NFD_Init();

    nfdchar_t *selected_path = nullptr;
    nfdfilteritem_t filter_item[1] = {{"Game Boy ROMs", "gb"}};
    const nfdresult_t result = NFD_OpenDialog(&selected_path, filter_item, 1, nullptr);

    std::optional<std::string> path;
    if (result == NFD_OKAY && selected_path != nullptr)
    {
        path = std::string(selected_path);
    }

    if (selected_path != nullptr)
    {
        NFD_FreePathU8(selected_path);
    }
    NFD_Quit();

    return path;
}

void ImguiFrontend::done()
{
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyTexture(texture_);
    SDL_DestroyRenderer(renderer_);
    SDL_DestroyWindow(window_);
    SDL_Quit();
}

} // namespace gbemu::frontend
