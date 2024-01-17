#ifndef GBEMU_GAMEBOY
#define GBEMU_GAMEBOY

#include "cpu.h"
#include "timer.h"
#include "ram.h"
#include "cartridge.h"
#include "ppu.h"
#include "joypad.h"

namespace gbemu {

    class Gameboy: public PPU::FrameCompleteListener, public std::enable_shared_from_this<Gameboy>
    {
    public:
        Gameboy(const std::string& opcodeDataFile);

        void loadCartridge(const Cartridge& cartridge);
        void start();
        void reset();

        void onFrameComplete();

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
    };

} // gbemu

#endif // GBEMU_GAMEBOY