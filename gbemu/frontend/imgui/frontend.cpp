#include "gbemu/frontend/imgui/frontend.h"

#include "gbemu/backend/cartridge.h"
#include "gbemu/backend/gameboy.h"
#include "gbemu/backend/ppu.h"
#include "gbemu/config/version.h"
#include "gbemu/frontend/frontend.h"
#include "gbemu/frontend/imgui/about.h"
#include "gbemu/frontend/imgui/cpu.h"
#include "gbemu/frontend/imgui/debugger.h"
#include "gbemu/frontend/imgui/memory.h"
#include "gbemu/frontend/imgui/performance.h"
#include "gbemu/frontend/imgui/util.h"

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <imgui_internal.h>

#include <memory>
#include <nfd.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <string>

namespace gbemu::frontend
{
namespace
{

constexpr ImVec4 kClearColor = ImVec4(0.05f, 0.07f, 0.09f, 1.00f);

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

ImguiFrontend::ImguiFrontend()
    : cpuWindow_(std::make_unique<ImguiCpuWindow>()), performanceWindow_(std::make_unique<ImguiPerformanceWindow>()),
      memoryWindow_(std::make_unique<ImguiMemoryWindow>()), debugger_(std::make_unique<ImguiDebugger>()),
      aboutWindow_(std::make_unique<ImguiAboutWindow>())
{}

ImguiFrontend::~ImguiFrontend() = default;

auto ImguiFrontend::init(gbemu::backend::Gameboy *gameboy) -> bool
{
    gameboy_ = gameboy;

    cpuWindow_->init(gameboy);
    performanceWindow_->init(gameboy);
    memoryWindow_->init(gameboy);
    debugger_->init(gameboy);

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

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();

    ImGuiStyle &style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;

    applyTheme();

    ImGui_ImplSDL2_InitForSDLRenderer(window_, renderer_);
    ImGui_ImplSDLRenderer2_Init(renderer_);

    cpuWindow_->setVisible(true);
    memoryWindow_->setVisible(true);
    debugger_->setVisible(true);
    performanceWindow_->setVisible(true);
    aboutWindow_->setVisible(false);
    dockspace_initialized_ = false;
    done_ = false;

    if (gameboy_->cartridgeLoaded())
    {
        status_text_ = "Cartridge ready. Open another ROM at any time from the File menu.";
        memoryWindow_->setMemoryViewBase(0x0100);
    }

    updateWindowTitle();

    return true;
}

auto ImguiFrontend::update() -> FrontEndMode
{
    if (done_)
    {
        return FrontEndMode::EXIT;
    }

    pollEvents();

    startRender();
    renderMenuBar();
    setupDockspace();

    const ImGuiIO &io = ImGui::GetIO();
    if (io.Framerate > 0.0f)
    {
        performanceWindow_->pushFrameTimeSample(1000.0f / io.Framerate);
    }

    renderScreen();

    cpuWindow_->render();
    memoryWindow_->render();
    debugger_->render();
    performanceWindow_->render();

    aboutWindow_->render();

    finishRender();

    return debugger_->inDebuggerMode() ? FrontEndMode::DEBUGGER : FrontEndMode::NORMAL;
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
        ImGuiID dock_bottom_left = 0;
        ImGuiID dock_bottom_right = 0;
        ImGuiID dock_right_bottom = 0;

        ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Right, 0.29f, &dock_right, &dock_main);
        ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Down, 0.32f, &dock_bottom, &dock_main);
        ImGui::DockBuilderSplitNode(dock_bottom, ImGuiDir_Right, 0.5f, &dock_bottom_right, &dock_bottom_left);
        ImGui::DockBuilderSplitNode(dock_right, ImGuiDir_Down, 0.40f, &dock_right_bottom, &dock_right);

        ImGui::DockBuilderDockWindow("Screen", dock_main);
        ImGui::DockBuilderDockWindow("Memory", dock_bottom_left);
        ImGui::DockBuilderDockWindow("Debugger", dock_bottom_right);
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
        if (ImGui::MenuItem("Open ROM...", util::OPEN_SHORTCUT))
        {
            openRom();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Quit", util::QUIT_SHORTCUT))
        {
            done_ = true;
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View"))
    {
        cpuWindow_->renderMenuItem();
        memoryWindow_->renderMenuItem();
        debugger_->renderMenuItem();
        performanceWindow_->renderMenuItem();
        ImGui::Separator();
        if (ImGui::MenuItem("Restore Default Layout"))
        {
            dockspace_initialized_ = false;
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Help"))
    {
        aboutWindow_->renderMenuItem();
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
        memoryWindow_->setMemoryViewBase(0x0100);
        updateWindowTitle();
    }
    catch (const std::exception &)
    {
        status_text_ = "Failed to load the selected ROM.";
    }
}

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
            const bool has_primary_shortcut = (event.key.keysym.mod & util::PRIMARY_SHORTCUT_MODIFIER) != 0;
            if (has_primary_shortcut && event.key.repeat == 0)
            {
                switch (event.key.keysym.sym)
                {
                case SDLK_o: openRom(); continue;
                case SDLK_q: done_ = true; continue;
                case SDLK_1: cpuWindow_->toggleVisibility(); continue;
                case SDLK_2: memoryWindow_->toggleVisibility(); continue;
                case SDLK_3: debugger_->toggleVisibility(); continue;
                case SDLK_4: performanceWindow_->toggleVisibility(); continue;
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

auto ImguiFrontend::selectRomFile() -> std::optional<std::string>
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
