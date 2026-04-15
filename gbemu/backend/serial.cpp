#include "gbemu/backend/serial.h"

namespace gbemu::backend
{

Serial::Serial(RAM *ram) : ram_(ram) {}

auto Serial::onReadOwnedByte(uint16_t address) -> uint8_t
{
    // Serial register is write-only, so always return 0xFF
    return 0xFF;
}

void Serial::onWriteOwnedByte(uint16_t address, uint8_t newValue, uint8_t currentValue)
{
    // SC written with transfer start bit (0x81 = internal clock + start): read SB and buffer it
    if (newValue == 0x81)
    {
        bufferedSerialBytes_.push(ram_->get(RAM::SB));
    }
}

auto Serial::read() -> std::optional<uint8_t>
{
    if (bufferedSerialBytes_.empty())
    {
        return std::nullopt;
    }

    const auto byte = bufferedSerialBytes_.front();
    bufferedSerialBytes_.pop();
    return byte;
}

} // namespace gbemu::backend
