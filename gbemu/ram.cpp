#include "ram.h"
#include "bitutils.h"

namespace gbemu {

    RAM::RAM(uint32_t ramSize, uint8_t defaultValue)
    {
        memory_ = std::vector<uint8_t>(ramSize, defaultValue);

        // TODO: remove when joypad is working correctly
        memory_[RAM::JOYP] = 0xff;
    }

    RAM::RAM(const RAM& ram)
    {
        memory_ = ram.memory_;
    }

    // uint8_t RAM::operator [](int i) const
    // {
    //     // TODO: catch out of bounds errors here and either throw custom error or handle how real hardware would.
    //     return memory_[i];
    // }

    // uint8_t& RAM::operator [](int i)
    // {
    //     // TODO: catch out of bounds errors here and either throw custom error or handle how real hardware would.
    //     return memory_[i];
    // }

    bool RAM::operator ==(const RAM& rhs) const
    {
        if (memory_.size() != rhs.memory_.size())
            return false;

        for (size_t i = 0; i < memory_.size(); i++)
        {
            if (get(i) != rhs.get(i))
                return false;
        }
        
        return true;
    }

    bool RAM::operator !=(const RAM& rhs) const
    {
        return !(*this == rhs);
    }

    std::ostream& operator<<(std::ostream& os, const RAM& ram)
    {
        for (int i = 0; i < ram.memory_.size(); i++)
        {
            const auto val = ram.get(i);
            if (val != 0)
            {
                // TODO: avoid having to cast address
                os << "[" << toHexString((uint16_t) i) << "] = " << toHexString(ram.get(i)) << "\n";
            }
        }
        return os;
    }

    void RAM::loadCartridge(const Cartridge& cartridge)
    {
        for (int i = 0; i < std::max(cartridge.size(), 0x8000ul); i++)
            memory_[i] = cartridge[i];
    }

    uint16_t RAM::getImmediate16(uint16_t i) const
    {
        const auto lower = get(i);
        const auto upper = get(i+1);
        return concatBytes(upper, lower);
    }
    
    void RAM::setImmediate16(uint16_t i, uint16_t newVal)
    {
        const auto lower = lowerByte(newVal);
        const auto upper = upperByte(newVal); 

        set(i, lower);
        set(i + 1, upper);
    }

    void RAM::set(uint16_t address, uint8_t value)
    {
        // TODO: temp: should we block all write attempts to ROM?
        if (address == 0x2000)
            return;

        if (address == RAM::JOYP)
        {
            // TODO: remove when joypad is working correctly
            memory_[address] = 0xff;
            return;
        }

        // TODO: OAM DMA transfer (should make rest of RAM inaccessible and take ~160 microseconds)
        if (address == RAM::DMA)
        {
            const uint16_t startAddress = concatBytes(value, 0x00);
            for (int i = 0; i < 160; i++)
                set(RAM::OAM + i, get(startAddress + i));
        }

        memory_[address] = value;
    }

    uint8_t RAM::get(uint16_t address) const
    {
        // temp: for GB Doctor log comparison testing.
        // if (address == RAM::LY)
        //     return 0x90;

        return memory_[address];
    }

} // gbemu