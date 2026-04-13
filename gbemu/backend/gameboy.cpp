#include "gbemu/backend/gameboy.h"

namespace gbemu::backend
{

Gameboy::Gameboy() { createHardware(); }

void Gameboy::loadCartridge(const Cartridge &cartridge)
{
    createHardware();
    ram_->loadCartridge(cartridge);
    cartridgeLoaded_ = true;

    if (initialized_)
    {
        initSubsystems();
    }
}

void Gameboy::init()
{
    if (initialized_)
    {
        return;
    }

    initSubsystems();
}

void Gameboy::update()
{
    if (!cartridgeLoaded_)
    {
        return;
    }

    uint64_t deltaCycles = 1;
    if (cpu_->mode() == CPU::Mode::NORMAL)
    {
        const auto before = cpu_->cycles();
        cpu_->executeInstruction(false);
        deltaCycles = cpu_->cycles() - before;
    }

    timer_->update(deltaCycles);
    ppu_->update();

    cpu_->serviceInterrupts();
}

void Gameboy::done() {}

void Gameboy::buttonPressed(Joypad::Button button) { joypad_->buttonPressed(button); }

void Gameboy::buttonReleased(Joypad::Button button) { joypad_->buttonReleased(button); }

bool Gameboy::consumeCompletedFrame() { return ppu_->consumeCompletedFrame(); }

void Gameboy::createHardware()
{
    joypad_ = std::make_unique<Joypad>();
    ram_ = std::make_unique<RAM>(RAM_SIZE);
    cpu_ = std::make_unique<CPU>(ram_.get());
    interruptController_ = std::make_unique<InterruptController>(ram_.get());
    ppu_ = std::make_unique<PPU>(ram_.get(), interruptController_.get());
    timer_ = std::make_unique<Timer>(interruptController_.get());
    serial_ = std::make_unique<Serial>(ram_.get());

    configureMemoryOwners();
    cartridgeLoaded_ = false;
}

void Gameboy::configureMemoryOwners()
{
    // Joypad
    ram_->addOwner(RAM::JOYP, joypad_.get());

    // Timer
    ram_->addOwner(RAM::DIV, timer_.get());
    ram_->addOwner(RAM::TIMA, timer_.get());
    ram_->addOwner(RAM::TMA, timer_.get());
    ram_->addOwner(RAM::TAC, timer_.get());

    // PPU
    ram_->addOwner(RAM::STAT, ppu_.get());
    ram_->addOwner(RAM::SCY, ppu_.get());
    ram_->addOwner(RAM::SCX, ppu_.get());
    ram_->addOwner(RAM::LY, ppu_.get());
    ram_->addOwner(RAM::LYC, ppu_.get());
    ram_->addOwner(RAM::WY, ppu_.get());
    ram_->addOwner(RAM::WX, ppu_.get());

    // Serial
    ram_->addOwner(RAM::SC, serial_.get());
}

void Gameboy::initSubsystems()
{
    timer_->addTimerListener(ppu_.get(), PPU::CYCLES_PER_SCANLINE);
    ppu_->init();
    timer_->init();
    initialized_ = true;
}

} // namespace gbemu::backend
