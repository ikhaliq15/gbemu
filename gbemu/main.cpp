#include <iostream>

#include "gameboy.h"
#include "cartridge.h"

#include <nfd.h>

int main(int argc, char** argv) {
    std::string cartridgeFilename;
    std::string opcodeDataFilename;

    if (argc <= 1)
    {
        std::cerr << "Too few arguments provided. Args: [rom_file] opcode_data_file" << std::endl;
        return EXIT_FAILURE;
    }
    else if (argc == 2)
    {
        // TODO: cleaner handling of NFD_Init/NFD_Quit
        NFD_Init();

        opcodeDataFilename = argv[1];

        nfdchar_t *selectedPath;
        nfdfilteritem_t filterItem[1] = { { "Gameboy ROMs", "gb" } };
        nfdresult_t result = NFD_OpenDialog(&selectedPath, filterItem, 1, NULL);
        if (result == NFD_OKAY)
        {
            cartridgeFilename = selectedPath;
            NFD_FreePath(selectedPath);
        }
        else if (result == NFD_CANCEL)
        {
            std::cerr << "Too few arguments provided. Args: [rom_file] opcode_data_file" << std::endl;
            return EXIT_FAILURE;
        }
        else
        {
            std::cerr << "File Selection Error:" << NFD_GetError() << std::endl;
            return EXIT_FAILURE;
        }
    }
    else if (argc == 3)
    {
        cartridgeFilename = argv[1];
        opcodeDataFilename = argv[2];
    }
    else
    {
        std::cerr << "Too many arguments provided. Args: [rom_file] opcode_data_file" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Selected cartridge file: " << cartridgeFilename << std::endl;

    const auto gameboy = std::make_shared<gbemu::Gameboy>(opcodeDataFilename);

    gbemu::Cartridge catridge(cartridgeFilename);
    gameboy->loadCartridge(catridge);

    gameboy->start();

    // TODO: cleaner handling of NFD_Init/NFD_Quit
    NFD_Quit();

    return 0;
}