#include "gameboy.h"
#include "bitutils.h"

#include <iostream>

namespace gbemu {

    Gameboy::Gameboy(const std::string& opcodeDataFile)
    : cartridgeLoaded_(false)
    , quit_(false)
    , timer_(std::make_shared<Timer>())
    , joypad_(std::make_shared<Joypad>())
    , ram_(std::make_shared<RAM>(GAMEBOY_RAM_SIZE))
    , cpu_(std::make_shared<CPU>(ram_, opcodeDataFile))
    , ppu_(std::make_shared<PPU>(cpu_))
    {}

    void Gameboy::loadCartridge(const Cartridge& cartridge)
    {
        ram_->loadCartridge(cartridge);
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

        /* Setup RAM address owners. */
        ram_->addOwner(RAM::JOYP, joypad_);

        /* Setup timer cycle listeners. */
        timer_->addCycleListener(ppu_, PPU::CYCLES_PER_SCANLINE);

        /* Setup up PPU frame completion listeners. */
        ppu_->subscribeToCompleteFrames(shared_from_this());

        /* Initialize subcomponents of the gameboy. */
        ppu_->init();
        timer_->init();

        while (!quit_)
        {
            const auto beforeCycleCount = cpu_->cycles();
            cpu_->executeInstruction(false);
            const auto afterCycleCount = cpu_->cycles();

            timer_->incrementTimer(afterCycleCount - beforeCycleCount);

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
        }
    }

    void Gameboy::reset()
    {
        // TODO
    }

    void Gameboy::onFrameComplete()
    {
        while (SDL_PollEvent(&event_) == 1) {
            switch(event_.type)
            {
                case SDL_QUIT:
                    quit_ = true;
                    break;
                case SDL_KEYDOWN:
                    joypad_->handleKeyDownEvent(event_.key);
                    break;
                case SDL_KEYUP:
                    joypad_->handleKeyUpEvent(event_.key);
                    break;
            }
        }
    }

} // gbemu