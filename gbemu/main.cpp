#include <iostream>

#include "gbemu/backend/cartridge.h"
#include "gbemu/backend/gameboy.h"
#include "gbemu/frontend/headless.hpp"
#include "gbemu/frontend/imgui.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <argparse/argparse.hpp>

void start(gbemu::backend::Gameboy *gameboy, gbemu::frontend::IFrontend *frontend)
{
    gameboy->init();
    frontend->init(gameboy);

    bool running = true;
    do
    {
        running = frontend->update();
        gameboy->update();
    } while (running);

    frontend->done();
    gameboy->done();
}

void dumpDisplay(gbemu::backend::Gameboy *gameboy, const std::string &path)
{
    if (path.empty())
    {
        return;
    }

    const auto width = gbemu::backend::LCD_WIDTH;
    const auto height = gbemu::backend::LCD_HEIGHT;

    const auto &pixels = gameboy->ppu()->getPixels();

    constexpr auto channels = 4; // RGBA
    auto *image_data = static_cast<unsigned char *>(malloc(width * height * channels));
    if (!image_data)
    {
        std::cerr << "Failed to allocate memory for image data." << std::endl;
        return;
    }

    for (int i = 0; i < width * height; i++)
    {
        const auto pixel = pixels[i];
        image_data[i * 4 + 0] = (pixel >> 16) & 0xFF; // Red
        image_data[i * 4 + 1] = (pixel >> 8) & 0xFF;  // Green
        image_data[i * 4 + 2] = pixel & 0xFF;         // Blue
        image_data[i * 4 + 3] = (pixel >> 24) & 0xFF; // Alpha
    }

    const auto stride_bytes = width * channels * sizeof(unsigned char);
    stbi_write_png(path.c_str(), width, height, channels, image_data, stride_bytes);

    free(image_data);
}
auto main(int argc, char **argv) -> int
{
    argparse::ArgumentParser args("GBEmu v3");
    args.add_argument("--blarg_console")
        .help("Print output from serial port (useful for Blargg testing)")
        .default_value(false)
        .implicit_value(true);
    args.add_argument("--headless")
        .help("Run GBEmu without a window (useful for integration testing)")
        .default_value(false)
        .implicit_value(true);
    args.add_argument("--dump_display_path")
        .help("Dump the display output to the file at this path on exit as a PNG (useful for testing)");
    args.add_argument("--rom_file").help("ROM file to load into the GBEmu").help("ROM file to load into the GBEmu");

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

    const auto enableBlarggConsole = args.get<bool>("--blarg_console");
    const auto enableHeadlessMode = args.get<bool>("--headless");
    const auto dumpDisplayOnExitPath =
        (args.is_used("--dump_display_path")) ? args.get<std::string>("--dump_display_path") : "";

    const gbemu::config::Config gameboyCfg{enableBlarggConsole, enableHeadlessMode, dumpDisplayOnExitPath};

    auto gameboy = std::make_unique<gbemu::backend::Gameboy>(gameboyCfg);

    auto frontend = [enableHeadlessMode]() -> std::unique_ptr<gbemu::frontend::IFrontend> {
        if (enableHeadlessMode)
        {
            return std::make_unique<gbemu::frontend::HeadlessFrontend>();
        }
        else
        {
            return std::make_unique<gbemu::frontend::ImguiFrontend>();
        }
    }();

    if (args.is_used("--rom_file"))
    {
        const auto cartridgeFilename = args.get<std::string>("--rom_file");
        const gbemu::backend::Cartridge catridge(cartridgeFilename);
        gameboy->loadCartridge(catridge);
    }

    start(gameboy.get(), frontend.get());

    if (!dumpDisplayOnExitPath.empty())
    {
        std::cout << "Dumping display to " << dumpDisplayOnExitPath << "..." << std::endl;

        dumpDisplay(gameboy.get(), dumpDisplayOnExitPath);
    }

    return 0;
}
