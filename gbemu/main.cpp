#include <iostream>

#include "gameboy.h"
#include "cartridge.h"

int main(int argc, char** argv) {
    if (argc <= 1)
    {
        std::cerr << "No ROM file provided." << std::endl;
        return EXIT_FAILURE;
    }

    if (argc <= 2)
    {
        std::cerr << "No opcode data file provided." << std::endl;
        return EXIT_FAILURE;
    }

    gbemu::Gameboy gameboy(argv[2]);

    gbemu::Cartridge catridge(argv[1]);
    gameboy.loadCartridge(catridge);

    gameboy.start();

    return 0;
}