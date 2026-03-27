#ifndef GBEMU_CARTRIDGE
#define GBEMU_CARTRIDGE

#include <cstdint>
#include <string>
#include <vector>

namespace gbemu
{

class Cartridge
{
  public:
    Cartridge(const std::string &romFileName);
    Cartridge(const std::vector<uint8_t> &cartridgeData);

    [[nodiscard]] uint8_t operator[](int i) const;
    [[nodiscard]] size_t size() const;

  private:
    std::vector<uint8_t> cartridgeData_;
};

} // namespace gbemu

#endif // GBEMU_CARTRIDGE
