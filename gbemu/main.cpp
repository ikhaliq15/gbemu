#include "gbemu/backend/cartridge.h"
#include "gbemu/backend/gameboy.h"
#include "gbemu/backend/ppu.h"
#include "gbemu/config/version.h"
#include "gbemu/frontend/fps_regulator.h"
#include "gbemu/frontend/headless.hpp"
#include "gbemu/frontend/imgui.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <argparse/argparse.hpp>

#include <array>
#include <iostream>
#include <memory>
#include <string>

namespace
{

void runEmulationLoop(gbemu::backend::Gameboy &gameboy, gbemu::frontend::IFrontend &frontend,
                      gbemu::frontend::FPSRegulator &fpsRegulator)
{
    gameboy.init();
    frontend.init(&gameboy);

    bool running = true;
    while (running)
    {
        if (gameboy.cartridgeLoaded())
        {
            while (!gameboy.consumeCompletedFrame())
            {
                gameboy.update();
            }
        }

        running = frontend.update();

        fpsRegulator.waitForNextFrame();
    }

    frontend.done();
    gameboy.done();
}

void dumpDisplay(const gbemu::backend::Gameboy &gameboy, const std::string &path)
{
    constexpr auto width = gbemu::backend::LCD_WIDTH;
    constexpr auto height = gbemu::backend::LCD_HEIGHT;
    constexpr auto channels = 4; // RGBA

    const auto &pixels = gameboy.ppu()->getPixels();
    std::array<unsigned char, width * height * channels> imageData{};

    for (size_t i = 0; i < pixels.size(); i++)
    {
        const auto pixel = pixels[i];
        imageData[i * 4 + 0] = (pixel >> 16) & 0xFF; // R
        imageData[i * 4 + 1] = (pixel >> 8) & 0xFF;  // G
        imageData[i * 4 + 2] = pixel & 0xFF;         // B
        imageData[i * 4 + 3] = (pixel >> 24) & 0xFF; // A
    }

    const auto strideBytes = width * channels * sizeof(unsigned char);
    stbi_write_png(path.c_str(), width, height, channels, imageData.data(), strideBytes);
}

std::unique_ptr<gbemu::frontend::IFrontend> createFrontend(bool headless, bool logSerial)
{
    if (headless)
    {
        return std::make_unique<gbemu::frontend::HeadlessFrontend>(logSerial);
    }
    return std::make_unique<gbemu::frontend::ImguiFrontend>();
}

} // namespace

auto main(int argc, char **argv) -> int
{
    argparse::ArgumentParser args(gbemu::config::kDisplayName);

    args.add_argument("--dump_serial")
        .help("Print output from serial port (useful for Blargg testing)")
        .default_value(false)
        .implicit_value(true);

    args.add_argument("--headless")
        .help("Run GBEmu without a window (useful for integration testing)")
        .default_value(false)
        .implicit_value(true);

    args.add_argument("--dump_display_path").help("Dump the display output to a PNG at this path on exit");

    args.add_argument("--rom_file").help("ROM file to load into the GBEmu");

    try
    {
        args.parse_args(argc, argv);
    }
    catch (const std::exception &err)
    {
        std::cerr << err.what() << std::endl;
        std::cerr << args.usage() << std::endl;
        return EXIT_FAILURE;
    }

    const auto headless = args.get<bool>("--headless");
    const auto dumpSerial = args.get<bool>("--dump_serial");
    const auto dumpPath = args.is_used("--dump_display_path") ? args.get<std::string>("--dump_display_path") : "";

    auto gameboy = std::make_unique<gbemu::backend::Gameboy>();
    auto frontend = createFrontend(headless, dumpSerial);

    if (args.is_used("--rom_file"))
    {
        const gbemu::backend::Cartridge cartridge(args.get<std::string>("--rom_file"));
        gameboy->loadCartridge(cartridge);
    }

    gbemu::frontend::FPSRegulator fpsRegulator(gameboy->targetFPS());
    if (headless)
    {
        fpsRegulator.disable();
    }

    runEmulationLoop(*gameboy, *frontend, fpsRegulator);

    if (!dumpPath.empty())
    {
        dumpDisplay(*gameboy, dumpPath);
    }

    return 0;
}
