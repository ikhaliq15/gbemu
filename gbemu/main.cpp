#include <iostream>

#include "gbemu/cartridge.h"
#include "gbemu/gameboy.h"

#include <argparse/argparse.hpp>

int main(int argc, char **argv)
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
    args.add_argument("rom_file").help("ROM file to load into the GBEmu");

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

    const auto cartridgeFilename = args.get<std::string>("rom_file");
    const auto gameboy = std::make_shared<gbemu::Gameboy>(gameboyCfg);

    const gbemu::Cartridge catridge(cartridgeFilename);

    gameboy->loadCartridge(catridge);
    gameboy->start();

    return 0;
}