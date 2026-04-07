#include "gbemu/backend/ram.h"

#include "gbemu/backend/bitutils.h"

#include <ostream>

namespace gbemu::backend
{

RAM::RAM(uint32_t ramSize, uint8_t defaultValue)
    : memory_(ramSize, defaultValue), readOwners_(ramSize, nullptr), writeOwners_(ramSize, nullptr)
{}

auto RAM::operator==(const RAM &rhs) const -> bool
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

auto operator<<(std::ostream &os, const RAM &ram) -> std::ostream &
{
    for (size_t i = 0; i < ram.memory_.size(); i++)
    {
        const auto val = ram.get(i);
        if (val != 0)
            os << "[" << toHexString(static_cast<uint16_t>(i)) << "] = " << toHexString(val) << "\n";
    }
    return os;
}

void RAM::loadCartridge(const Cartridge &cartridge)
{
    for (size_t i = 0; i < std::min(cartridge.size(), static_cast<size_t>(0x8000)); i++)
        memory_[i] = cartridge[i];
}

auto RAM::get(uint16_t address) const -> uint8_t
{
    if (const auto readOwner = readOwners_[address]; readOwner != nullptr)
    {
        return readOwner->onReadOwnedByte(address);
    }
    return memory_[address];
}

void RAM::set(uint16_t address, uint8_t value)
{
    if (address == 0x2000)
        return;

    // TODO: OAM DMA transfer (should make rest of RAM inaccessible and take ~160 microseconds)
    if (address == RAM::DMA)
    {
        const uint16_t startAddress = concatBytes(value, 0x00);
        for (int i = 0; i < 160; i++)
            set(RAM::OAM + i, get(startAddress + i));
    }

    if (const auto writeOwner = writeOwners_[address]; writeOwner != nullptr)
    {
        writeOwner->onWriteOwnedByte(address, value, get(address));
        return;
    }

    memory_[address] = value;
}

auto RAM::getImmediate16(uint16_t address) const -> uint16_t { return concatBytes(get(address + 1), get(address)); }

void RAM::setImmediate16(uint16_t address, uint16_t value)
{
    set(address, lowerByte(value));
    set(address + 1, upperByte(value));
}

void RAM::addReadOwner(uint16_t address, ReadOwner *owner)
{
    if (readOwners_[address] != nullptr)
        throw std::runtime_error("The address " + toHexString(address) + " cannot have multiple read owners.");
    readOwners_[address] = owner;
}

void RAM::addWriteOwner(uint16_t address, WriteOwner *owner)
{
    if (writeOwners_[address] != nullptr)
        throw std::runtime_error("The address " + toHexString(address) + " cannot have multiple write owners.");
    writeOwners_[address] = owner;
}

void RAM::addOwner(uint16_t address, Owner *owner)
{
    addReadOwner(address, owner);
    addWriteOwner(address, owner);
}

} // namespace gbemu::backend
