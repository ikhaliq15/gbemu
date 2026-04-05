#include "gbemu/backend/gameboy.h"

#include <utility>

namespace gbemu::backend
{

Gameboy::Gameboy()
{
    createHardware();
}

void Gameboy::loadCartridge(const Cartridge &cartridge)
{
    createHardware();
    ram_->loadCartridge(cartridge);
    cartridgeLoaded_ = true;

    if (initialized_)
        initSubsystems();
}

void Gameboy::init()
{
    if (initialized_)
        return;

    initSubsystems();
}

void Gameboy::update()
{
    if (!cartridgeLoaded_)
        return;

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
    pollSerialPort();
}

void Gameboy::done()
{}

void Gameboy::inputDown(int32_t keyCode)
{
    joypad_->handleKeyDownEvent(keyCode);
}

void Gameboy::inputUp(int32_t keyCode)
{
    joypad_->handleKeyUpEvent(keyCode);
}

bool Gameboy::consumeCompletedFrame()
{
    return ppu_->consumeCompletedFrame();
}

void Gameboy::createHardware()
{
    joypad_ = std::make_unique<Joypad>();
    ram_ = std::make_unique<RAM>(RAM_SIZE);
    cpu_ = std::make_unique<CPU>(ram_.get());
    ppu_ = std::make_unique<PPU>(cpu_.get(), ram_.get());
    timer_ = std::make_unique<Timer>(cpu_.get());

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
}

void Gameboy::initSubsystems()
{
    timer_->addTimerListener(ppu_.get(), PPU::CYCLES_PER_SCANLINE);
    ppu_->init();
    timer_->init();
    initialized_ = true;
}

std::optional<uint8_t> Gameboy::consumeSerialByte()
{
    return std::exchange(pendingSerialByte_, std::nullopt);
}

void Gameboy::pollSerialPort()
{
    if (ram_->get(RAM::SC) != 0x81)
        return;

    pendingSerialByte_ = ram_->get(RAM::SB);
    ram_->set(RAM::SC, 0x00);
}

} // namespace gbemu::backend
