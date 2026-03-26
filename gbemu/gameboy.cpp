#include "gbemu/gameboy.h"
#include "gbemu/bitutils.h"

#include <iostream>

namespace gbemu
{

Gameboy::Gameboy(const config::Config &cfg)
    : cartridgeLoaded_(false), quit_(false), joypad_(std::make_shared<Joypad>()),
      ram_(std::make_shared<RAM>(GAMEBOY_RAM_SIZE)), cpu_(std::make_shared<CPU>(ram_)),
      ppu_(std::make_shared<PPU>(cpu_, cfg.runHeadless(), cfg.dumpDisplayOnExitPath())),
      timer_(std::make_shared<Timer>(cpu_)), enableBlarggSerialLogging_(cfg.enableBlarggSerialLogging())
{
}

void Gameboy::loadCartridge(const Cartridge &cartridge)
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
    ram_->addOwner(RAM::DIV, timer_);
    ram_->addOwner(RAM::TIMA, timer_);
    ram_->addOwner(RAM::TMA, timer_);
    ram_->addOwner(RAM::TAC, timer_);
    ram_->addOwner(RAM::STAT, ppu_);
    ram_->addOwner(RAM::SCY, ppu_);
    ram_->addOwner(RAM::SCX, ppu_);
    ram_->addOwner(RAM::LY, ppu_);
    ram_->addOwner(RAM::LYC, ppu_);
    ram_->addOwner(RAM::WY, ppu_);
    ram_->addOwner(RAM::WX, ppu_);

    /* Setup timer cycle listeners. */
    timer_->addTimerListener(ppu_, PPU::CYCLES_PER_SCANLINE);

    /* Setup up PPU frame completion listeners. */
    ppu_->subscribeToCompleteFrames(shared_from_this());
    subscribeToShutDown(ppu_);

    /* Initialize subcomponents of the gameboy. */
    ppu_->init();
    timer_->init();

    while (!quit_)
    {
        uint64_t deltaCycles = 1;
        if (cpu_->mode() == CPU::Mode::NORMAL)
        {
            const auto beforeCycleCount = cpu_->cycles();
            cpu_->executeInstruction(false);
            const auto afterCycleCount = cpu_->cycles();
            deltaCycles = afterCycleCount - beforeCycleCount;
        }

        timer_->update(deltaCycles);
        ppu_->update();

        if ((ram_->get(RAM::IF) & ram_->get(RAM::IE) & 0x1f) != 0x00)
            cpu_->setMode(CPU::Mode::NORMAL);

        if (cpu_->IME())
        {
            // TODO: what happens if mulitple interupts fired at same time?
            const uint8_t interruptsFired = ram_->get(RAM::IF) & ram_->get(RAM::IE);
            if ((interruptsFired & 0x01) != 0x00)
            {
                cpu_->setIME(false);
                cpu_->pushToStack(cpu_->PC());
                ram_->set(RAM::IF, setBit(ram_->get(RAM::IF), 0, 0));
                cpu_->setPC(0x0040);
            }
            else if ((interruptsFired & 0x02) != 0x00)
            {
                cpu_->setIME(false);
                cpu_->pushToStack(cpu_->PC());
                ram_->set(RAM::IF, setBit(ram_->get(RAM::IF), 1, 0));
                cpu_->setPC(0x0048);
            }
            else if ((interruptsFired & 0x04) != 0x00)
            {
                cpu_->setIME(false);
                cpu_->pushToStack(cpu_->PC());
                ram_->set(RAM::IF, setBit(ram_->get(RAM::IF), 2, 0));
                cpu_->setPC(0x0050);
            }
        }

        if (enableBlarggSerialLogging_)
        {
            const auto currentSB = ram_->get(RAM::SB);
            const auto currentSC = ram_->get(RAM::SC);
            if (currentSC == 0x81)
            {
                putc(currentSB, stdout);
                fflush(stdout);
                ram_->set(RAM::SC, 0x00);
            }
        }
    }

    shutdown();
}

void Gameboy::reset()
{
    // TODO
}

void Gameboy::onFrameComplete()
{
    while (SDL_PollEvent(&event_) == 1)
    {
        switch (event_.type)
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

} // namespace gbemu
