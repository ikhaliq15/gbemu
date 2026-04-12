#ifndef GBEMU_BACKEND_GAMEBOY_H
#define GBEMU_BACKEND_GAMEBOY_H

#include "gbemu/backend/cartridge.h"
#include "gbemu/backend/cpu.h"
#include "gbemu/backend/joypad.h"
#include "gbemu/backend/ppu.h"
#include "gbemu/backend/ram.h"
#include "gbemu/backend/timer.h"

#include <memory>
#include <optional>
#include <queue>

namespace gbemu::backend
{

class Gameboy
{
  public:
    Gameboy();

    void loadCartridge(const Cartridge &cartridge);

    void init();
    void update();
    void done();

    [[nodiscard]] constexpr auto targetFPS() const -> double { return 59.7275; }

    void buttonPressed(Joypad::Button button);
    void buttonReleased(Joypad::Button button);

    [[nodiscard]] bool cartridgeLoaded() const { return cartridgeLoaded_; }
    bool consumeCompletedFrame();
    [[nodiscard]] std::optional<uint8_t> consumeSerialByte();

    [[nodiscard]] const CPU *cpu() const { return cpu_.get(); }
    [[nodiscard]] const PPU *ppu() const { return ppu_.get(); }
    [[nodiscard]] const RAM *ram() const { return ram_.get(); }

  private:
    static constexpr uint32_t RAM_SIZE = 0x10000;

    void createHardware();
    void configureMemoryOwners();
    void initSubsystems();

    void pollSerialPort();

    bool cartridgeLoaded_ = false;
    bool initialized_ = false;
    std::queue<uint8_t> pendingSerialBytes_;

    std::unique_ptr<Joypad> joypad_;
    std::unique_ptr<RAM> ram_;
    std::unique_ptr<InterruptController> interruptController_;
    std::unique_ptr<CPU> cpu_;
    std::unique_ptr<PPU> ppu_;
    std::unique_ptr<Timer> timer_;
};

} // namespace gbemu::backend

#endif // GBEMU_BACKEND_GAMEBOY_H
