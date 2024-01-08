#include "gameboy.h"
#include "bitutils.h"

#include <iostream>

namespace gbemu {

    Gameboy::Gameboy(const std::string& opcodeDataFile)
    : cartridgeLoaded_(false)
    , ram_(std::make_shared<RAM>(GAMEBOY_RAM_SIZE))
    , cpu_(std::make_shared<CPU>(ram_, opcodeDataFile))
    , ppu_(PPU(cpu_))
    {}

    void Gameboy::loadCartridge(const Cartridge& cartridge)
    {
        // TODO
        for (int i = 0; i < std::max(cartridge.size(), 0x8000ul); i++)
            ram_->set(i, cartridge[i]);

        cartridgeLoaded_ = true;
    }

    void Gameboy::start()
    {
        // TODO
        if (!cartridgeLoaded_) 
        {
            std::cerr << "No cartridge loaded in gameboy." << std::endl;
            exit(EXIT_FAILURE);
        }

        ppu_.init();

        bool quit = false;
        bool runForever = true;
        bool runTillBreak = false;
        uint16_t breakInstruction = 0xffff;
        size_t numSteps = 0;
        while (!quit)
        {
            std::cout << "> ";

            std::string command;
            // std::getline(std::cin, command);

            if (command == "q")
                quit = true;
            else if (command == "n")
                numSteps = 1;
            else if (command.starts_with("n "))
                numSteps = std::stol(command.substr(2));
            else if (command.starts_with("b "))
            {
                runTillBreak = true;
                breakInstruction = std::stoul(command.substr(2), nullptr, 0);
            }
            else if (command == "c")
                runForever = true;
            else
                std::cout << "Unrecognized command." << std::endl;

            for (size_t i = 0; !quit && (i < numSteps || runForever || runTillBreak); i++)
            {
                cpu_->executeInstruction(false);
                ppu_.update();
                quit = ppu_.hasQuit();

                if (cpu_->IME())
                {
                    const auto interruptsFired = ram_->get(RAM::IF) & ram_->get(RAM::IE);
                    if ((interruptsFired & 0x01) != 0x00)
                    {
                        cpu_->setIME(false);
                        cpu_->pushToStack(cpu_->PC());
                        ram_->set(RAM::IF, setBit(ram_->get(RAM::IF), 0, 0));
                        cpu_->setPC(0x0040);
                    }
                }

                if (cpu_->PC() == breakInstruction)
                    runTillBreak = false;
            }

            runTillBreak = false;
            runForever = false;
            numSteps = 0;
        }
    }

    void Gameboy::reset()
    {
        // TODO
    }

} // gbemu