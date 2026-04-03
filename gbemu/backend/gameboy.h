#ifndef GBEMU_BACKEND_GAMEBOY_H
#define GBEMU_BACKEND_GAMEBOY_H

#include "gbemu/backend/cartridge.h"
#include "gbemu/backend/cpu.h"
#include "gbemu/backend/joypad.h"
#include "gbemu/backend/ppu.h"
#include "gbemu/backend/ram.h"
#include "gbemu/backend/shutdown_listener.h"
#include "gbemu/backend/timer.h"
#include "gbemu/config/config.h"

namespace gbemu::backend
{

class Gameboy
{
  public:
    Gameboy(const config::Config &cfg);

    void loadCartridge(const Cartridge &cartridge);

    void init();
    void update();
    void done();

    void inputDown(int32_t keyCode);
    void inputUp(int32_t keyCode);

    bool consumeCompletedFrame()
    {
        return ppu_->consumeCompletedFrame();
    }
    void getScreenPixels(std::array<uint32_t, WINDOW_WIDTH * WINDOW_HEIGHT> &outPixels);

    void subscribeToShutDown(ShutDownListener *shutDownListener)
    {
        shutDownListeners_.push_back(shutDownListener);
    }

    bool cartridgeLoaded() const
    {
        return cartridgeLoaded_;
    }

  private:
    void shutdown()
    {
        for (const auto &shutDownListener : shutDownListeners_)
            shutDownListener->onShutDown();
    }

  private:
    void handleInterrupts();
    void handleSerialPorts();

  private:
    static constexpr uint32_t GAMEBOY_RAM_SIZE = 0x10000;

    bool cartridgeLoaded_;
    bool quit_;

    std::unique_ptr<Joypad> joypad_;
    std::unique_ptr<RAM> ram_;
    std::unique_ptr<CPU> cpu_;
    std::unique_ptr<PPU> ppu_;
    std::unique_ptr<Timer> timer_;

    const bool enableBlarggSerialLogging_;
    std::vector<ShutDownListener *> shutDownListeners_;
};

} // namespace gbemu::backend

#endif // GBEMU_BACKEND_GAMEBOY_H
