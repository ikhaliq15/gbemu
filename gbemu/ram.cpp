#include "gbemu/ram.h"

#include "gbemu/bitutils.h"

namespace gbemu
{

RAM::RAM(uint32_t ramSize, uint8_t defaultValue)
{
    memory_ = std::vector<uint8_t>(ramSize, defaultValue);
}

RAM::RAM(const RAM &ram)
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

bool RAM::operator==(const RAM &rhs) const
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

bool RAM::operator!=(const RAM &rhs) const
{
    return !(*this == rhs);
}

std::ostream &operator<<(std::ostream &os, const RAM &ram)
{
    for (int i = 0; i < ram.memory_.size(); i++)
    {
        const auto val = ram.get(i);
        if (val != 0)
        {
            // TODO: avoid having to cast address
            os << "[" << toHexString((uint16_t)i) << "] = " << toHexString(ram.get(i)) << "\n";
        }
    }
    return os;
}

void RAM::loadCartridge(const Cartridge &cartridge)
{
    for (int i = 0; i < std::max(cartridge.size(), 0x8000ul); i++)
        memory_[i] = cartridge[i];
}

uint16_t RAM::getImmediate16(uint16_t i) const
{
    const auto lower = get(i);
    const auto upper = get(i + 1);
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

    // TODO: OAM DMA transfer (should make rest of RAM inaccessible and take ~160 microseconds)
    // TODO: turn into a R/W Owner?
    if (address == RAM::DMA)
    {
        const uint16_t startAddress = concatBytes(value, 0x00);
        for (int i = 0; i < 160; i++)
            set(RAM::OAM + i, get(startAddress + i));
    }

    const auto it = writeOwners_.find(address);
    if (it != writeOwners_.end())
    {
        it->second->onWriteOwnedByte(address, value, get(address));
        return;
    }

    memory_[address] = value;
}

uint8_t RAM::get(uint16_t address) const
{
    // temp: for GB Doctor log comparison testing.
    // if (address == RAM::LY)
    //     return 0x90;

    const auto it = readOwners_.find(address);
    if (it != readOwners_.end())
        return it->second->onReadOwnedByte(address);
    return memory_[address];
}

void RAM::addReadOwner(uint16_t address, std::shared_ptr<ReadOwner> owner)
{
    if (readOwners_.find(address) != readOwners_.end())
        throw std::runtime_error("The address " + toHexString(address) + " cannot have multiple read owners.");
    readOwners_.insert({address, owner});
}

void RAM::addWriteOwner(uint16_t address, std::shared_ptr<WriteOwner> owner)
{
    if (writeOwners_.find(address) != writeOwners_.end())
        throw std::runtime_error("The address " + toHexString(address) + " cannot have multiple write owners.");
    writeOwners_.insert({address, owner});
}

void RAM::addOwner(uint16_t address, std::shared_ptr<Owner> owner)
{
    addReadOwner(address, owner);
    addWriteOwner(address, owner);
}

} // namespace gbemu