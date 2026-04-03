#include <iostream>

#include "gbemu/backend/cartridge.h"
#include "gbemu/backend/gameboy.h"
#include "gbemu/frontend/imgui.hpp"

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
    auto frontend = std::make_unique<gbemu::frontend::ImguiFrontend>();

    if (args.is_used("--rom_file"))
    {
        const auto cartridgeFilename = args.get<std::string>("--rom_file");
        const gbemu::backend::Cartridge catridge(cartridgeFilename);
        gameboy->loadCartridge(catridge);
    }

    start(gameboy.get(), frontend.get());

    return 0;
}
