#include "gbemu/backend/ram.h"

#include "gbemu/backend/bitutils.h"

namespace gbemu::backend
{

RAM::RAM(uint32_t ramSize, uint8_t defaultValue)
{
    memory_ = std::vector<uint8_t>(ramSize, defaultValue);
    writeOwners_ = std::vector<WriteOwner *>(ramSize, nullptr);
    readOwners_ = std::vector<ReadOwner *>(ramSize, nullptr);
}

RAM::RAM(const RAM &ram)
{
    memory_ = ram.memory_;
    writeOwners_ = ram.writeOwners_;
    readOwners_ = ram.readOwners_;
}

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

auto RAM::operator!=(const RAM &rhs) const -> bool
{
    return !(*this == rhs);
}

auto operator<<(std::ostream &os, const RAM &ram) -> std::ostream &
{
    for (size_t i = 0; i < ram.memory_.size(); i++)
    {
        const auto val = ram.get(i);
        if (val != 0)
        {
            // TODO: avoid having to cast address
            os << "[" << toHexString(static_cast<uint16_t>(i)) << "] = " << toHexString(ram.get(i)) << "\n";
        }
    }
    return os;
}

void RAM::loadCartridge(const Cartridge &cartridge)
{
    for (size_t i = 0; i < std::max(cartridge.size(), 0x8000ul); i++)
        memory_[i] = cartridge[i];
}

auto RAM::getImmediate16(uint16_t i) const -> uint16_t
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

    const auto writeOwner = writeOwners_[address];
    if (writeOwner != nullptr)
    {
        writeOwner->onWriteOwnedByte(address, value, get(address));
        return;
    }

    memory_[address] = value;
}

auto RAM::get(uint16_t address) const -> uint8_t
{
    const auto readOwner = readOwners_[address];
    if (readOwner != nullptr)
        return readOwner->onReadOwnedByte(address);
    return memory_[address];
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
