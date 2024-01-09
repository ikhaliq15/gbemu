#ifndef GBEMU_RAM
#define GBEMU_RAM

#include "cartridge.h"

#include <vector>
#include <iostream>

namespace gbemu {

    class RAM
    {
    public:
        static constexpr uint16_t OAM  = 0xfe00;
        static constexpr uint16_t JOYP = 0xff00;
        static constexpr uint16_t IF   = 0xff0f;
        static constexpr uint16_t LCDC = 0xff40;
        static constexpr uint16_t LY   = 0xff44;
        static constexpr uint16_t DMA  = 0xff46;
        static constexpr uint16_t BGP  = 0xff47;
        static constexpr uint16_t OBP0 = 0xff48;
        static constexpr uint16_t OBP1 = 0xff49;
        static constexpr uint16_t IE   = 0xffff;

        RAM(uint32_t ramSize, uint8_t defaultValue = 0);
        RAM(const RAM& ram);

        // uint8_t operator [](int i) const;
        // uint8_t& operator [](int i);

        bool operator ==(const RAM& rhs) const;
        bool operator !=(const RAM& rhs) const;

        friend std::ostream& operator<<(std::ostream& os, const RAM& ram);

        void loadCartridge(const Cartridge& cartridge);

        uint16_t getImmediate16(uint16_t i) const;
        void setImmediate16(uint16_t i, uint16_t newVal);

        void set(uint16_t address, uint8_t value);
        uint8_t get(uint16_t address) const;

    private:
        std::vector<uint8_t> memory_;
    };

} // gbemu

#endif // GBEMU_RAM