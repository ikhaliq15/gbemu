#ifndef GBEMU_BACKEND_CARTRIDGE_H
#define GBEMU_BACKEND_CARTRIDGE_H

#include <cstdint>
#include <string>
#include <vector>

namespace gbemu::backend
{

class Cartridge
{
  public:
    explicit Cartridge(const std::string &romFileName);
    explicit Cartridge(const std::vector<uint8_t> &cartridgeData);

    [[nodiscard]] auto operator[](size_t i) const -> uint8_t { return cartridgeData_[i]; };
    [[nodiscard]] auto size() const -> size_t { return cartridgeData_.size(); };

  private:
    std::vector<uint8_t> cartridgeData_;
};

} // namespace gbemu::backend

#endif // GBEMU_BACKEND_CARTRIDGE_H
