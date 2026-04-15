#ifndef GBEMU_BACKEND_SERIAL_H
#define GBEMU_BACKEND_SERIAL_H

#include "gbemu/backend/ram.h"

#include <cstdint>
#include <optional>
#include <queue>

namespace gbemu::backend
{

class Serial : public RAM::Owner
{
  public:
    explicit Serial(RAM *ram);

    auto read() -> std::optional<uint8_t>;

    // RAM::Owner
    auto onReadOwnedByte(uint16_t address) -> uint8_t override;
    void onWriteOwnedByte(uint16_t address, uint8_t newValue, uint8_t currentValue) override;

  private:
    RAM *ram_;
    std::queue<uint8_t> bufferedSerialBytes_;
};

} // namespace gbemu::backend

#endif // GBEMU_BACKEND_SERIAL_H
