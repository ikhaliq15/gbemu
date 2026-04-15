#ifndef GBEMU_BACKEND_GAMEBOY_H
#define GBEMU_BACKEND_GAMEBOY_H

#include "gbemu/backend/cartridge.h"
#include "gbemu/backend/cpu.h"
#include "gbemu/backend/joypad.h"
#include "gbemu/backend/ppu.h"
#include "gbemu/backend/ram.h"
#include "gbemu/backend/serial.h"
#include "gbemu/backend/timer.h"

#include <memory>
#include <optional>

namespace gbemu::backend
{

class Gameboy
{
  public:
    Gameboy();

    void loadCartridge(const Cartridge &cartridge);

    void init();
    void update();
    void done() {};

    void buttonPressed(Joypad::Button button) { joypad_->buttonPressed(button); }
    void buttonReleased(Joypad::Button button) { joypad_->buttonReleased(button); }

    [[nodiscard]] constexpr auto targetFPS() const -> double { return 59.7275; }

    [[nodiscard]] auto cartridgeLoaded() const -> bool { return cartridgeLoaded_; }
    [[nodiscard]] auto consumeCompletedFrame() -> bool { return ppu_->consumeCompletedFrame(); }
    [[nodiscard]] auto consumeSerialByte() -> std::optional<uint8_t> { return serial_->read(); };

    [[nodiscard]] auto cpu() const -> const CPU *const { return cpu_.get(); }
    [[nodiscard]] auto ppu() const -> const PPU *const { return ppu_.get(); }
    [[nodiscard]] auto ram() const -> const RAM *const { return ram_.get(); }

  private:
    static constexpr uint32_t RAM_SIZE = 0x10000;

    void createHardware();
    void configureMemoryOwners();
    void initSubsystems();

    bool cartridgeLoaded_ = false;
    bool initialized_ = false;

    std::unique_ptr<Joypad> joypad_;
    std::unique_ptr<RAM> ram_;
    std::unique_ptr<InterruptController> interruptController_;
    std::unique_ptr<CPU> cpu_;
    std::unique_ptr<PPU> ppu_;
    std::unique_ptr<Timer> timer_;
    std::unique_ptr<Serial> serial_;
};

} // namespace gbemu::backend

#endif // GBEMU_BACKEND_GAMEBOY_H
