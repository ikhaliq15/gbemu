#ifndef GBEMU_GAMEBOY
#define GBEMU_GAMEBOY

#include "cpu.h"
#include "operand.h"
#include "ram.h"
#include "cartridge.h"
#include "opcode.h"
#include "ppu.h"

#include <map>
#include <unordered_map>

namespace gbemu {

    class Gameboy
    {
    public:
        Gameboy(const std::string& opcodeDataFile);

        void loadCartridge(const Cartridge& cartridge);
        void start();
        void reset();

    private:
        static constexpr uint32_t GAMEBOY_RAM_SIZE = 0x10000;

        bool cartridgeLoaded_;

        std::shared_ptr<RAM> ram_;
        std::shared_ptr<CPU> cpu_;
        PPU ppu_;
    };

} // gbemu

#endif // GBEMU_GAMEBOY