#ifndef GBEMU_GAMEBOY
#define GBEMU_GAMEBOY

#include "gbemu/cartridge.h"
#include "gbemu/config.h"
#include "gbemu/cpu.h"
#include "gbemu/joypad.h"
#include "gbemu/ppu.h"
#include "gbemu/ram.h"
#include "gbemu/shutdown_listener.h"
#include "gbemu/timer.h"

namespace gbemu
{

class Gameboy : public PPU::FrameCompleteListener, public std::enable_shared_from_this<Gameboy>
{
  public:
    Gameboy(const config::Config &cfg);

    void loadCartridge(const Cartridge &cartridge);
    void start();
    void reset();

    void onFrameComplete();

    void subscribeToShutDown(const std::shared_ptr<ShutDownListener> shutDownListener)
    {
        shutDownListeners_.push_back(shutDownListener);
    }

  private:
    void shutdown()
    {
        for (const auto &shutDownListener : shutDownListeners_)
            shutDownListener->onShutDown();
    }

  private:
    static constexpr uint32_t GAMEBOY_RAM_SIZE = 0x10000;

    bool cartridgeLoaded_;
    bool quit_;

    std::shared_ptr<Joypad> joypad_;
    std::shared_ptr<RAM> ram_;
    std::shared_ptr<CPU> cpu_;
    std::shared_ptr<PPU> ppu_;
    std::shared_ptr<Timer> timer_;

    SDL_Event event_;

    const bool enableBlarggSerialLogging_;
    std::vector<std::shared_ptr<ShutDownListener>> shutDownListeners_;
};

} // namespace gbemu

#endif // GBEMU_GAMEBOY