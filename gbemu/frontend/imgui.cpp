#include "gbemu/frontend/imgui.hpp"

#include "gbemu/backend/cartridge.h"
#include "gbemu/backend/gameboy.h"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "imgui_internal.h"

#include <nfd.h>

namespace gbemu::frontend
{

bool ImguiFrontend::init(gbemu::backend::Gameboy *gameboy)
{
    gameboy_ = gameboy;

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER);
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");

    const auto main_scale = ImGui_ImplSDL2_GetContentScaleForDisplay(0);

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    window_ = SDL_CreateWindow("GBEmu v3", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, (int)(1280 * main_scale),
                               (int)(800 * main_scale), window_flags);

    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                 gbemu::backend::WINDOW_WIDTH, gbemu::backend::WINDOW_HEIGHT);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();

    ImGuiStyle &style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;

    ImGui_ImplSDL2_InitForSDLRenderer(window_, renderer_);
    ImGui_ImplSDLRenderer2_Init(renderer_);

    show_demo_window_ = true;
    show_another_window_ = false;
    done_ = false;

    return true;
}

bool ImguiFrontend::update()
{
    if (done_)
    {
        return false;
    }

    const auto frameReady = gameboy_->consumeCompletedFrame();

    if (!frameReady && gameboy_->cartridgeLoaded())
    {
        return true;
    }

    pollEvents();

    startRender();

    setupDockspace();

    renderScreen();
    renderDebug();

    // if (show_demo_window_)
    //     ImGui::ShowDemoWindow(&show_demo_window_);

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

    const ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    ImGui::Render();
    SDL_RenderSetScale(renderer_, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
    SDL_SetRenderDrawColor(renderer_, (Uint8)(clear_color.x * 255), (Uint8)(clear_color.y * 255),
                           (Uint8)(clear_color.z * 255), (Uint8)(clear_color.w * 255));
    SDL_RenderClear(renderer_);
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer_);

    SDL_RenderPresent(renderer_);
}

void ImguiFrontend::setupDockspace()
{
    const ImGuiID dockspace_id = ImGui::GetID("GBEmu DockSpace");
    const ImGuiViewport *viewport = ImGui::GetMainViewport();

    if (ImGui::DockBuilderGetNode(dockspace_id) == nullptr)
    {
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);
        ImGuiID dock_id_left = 0;
        ImGuiID dock_id_main = dockspace_id;
        ImGui::DockBuilderSplitNode(dock_id_main, ImGuiDir_Left, 0.20f, &dock_id_left, &dock_id_main);
        ImGuiID dock_id_left_top = 0;
        ImGuiID dock_id_left_bottom = 0;
        ImGui::DockBuilderSplitNode(dock_id_left, ImGuiDir_Up, 0.50f, &dock_id_left_top, &dock_id_left_bottom);
        ImGui::DockBuilderDockWindow("Screen", dock_id_main);
        ImGui::DockBuilderDockWindow("Debug", dock_id_left_top);
        ImGui::DockBuilderDockWindow("Scene", dock_id_left_bottom);
        ImGui::DockBuilderFinish(dockspace_id);
    }

    ImGui::DockSpaceOverViewport(dockspace_id, viewport,
                                 ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_NoUndocking);
}

void ImguiFrontend::renderScreen()
{
    ImGui::Begin("Screen", nullptr,
                 ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoTitleBar);

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open", "Ctrl+O"))
            {
                const auto romFile = selectRomFile();
                if (romFile)
                {
                    gameboy_->loadCartridge(backend::Cartridge(*romFile));
                }
            }
            ImGui::EndMenu();
        }

        gameboy_->getScreenPixels(screenPixels_);
        SDL_UpdateTexture(texture_, nullptr, screenPixels_.data(), gbemu::backend::WINDOW_WIDTH * sizeof(uint32_t));

        ImGui::EndMenuBar();
    }

    ImGui::Image(texture_, ImVec2((float)gbemu::backend::WINDOW_WIDTH, (float)gbemu::backend::WINDOW_HEIGHT));

    ImGui::End();
}

void ImguiFrontend::renderDebug()
{
    ImGuiIO &io = ImGui::GetIO();

    ImGui::Begin("Debug", nullptr,
                 ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove);

    ImGui::TextWrapped("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

    ImGui::End();
}

void ImguiFrontend::pollEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT)
            done_ = true;
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
            event.window.windowID == SDL_GetWindowID(window_))
            done_ = true;
        if (event.type == SDL_KEYDOWN)
            gameboy_->inputDown(event.key.keysym.sym);
        if (event.type == SDL_KEYUP)
            gameboy_->inputUp(event.key.keysym.sym);
    }
}

std::optional<std::string> ImguiFrontend::selectRomFile()
{
    NFD_Init();

    nfdchar_t *selectedPath;
    nfdfilteritem_t filterItem[1] = {{"Gameboy ROMs", "gb"}};
    nfdresult_t result = NFD_OpenDialog(&selectedPath, filterItem, 1, NULL);

    if (result != NFD_OKAY)
    {
        return std::nullopt;
    }

    const std::string path = selectedPath;
    NFD_FreePathU8(selectedPath);
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
