#ifndef GBEMU_CARTRIDGE
#define GBEMU_CARTRIDGE

#include "cpu.h"
#include "ram.h"

namespace gbemu {

    class Cartridge
    {
    public:
        Cartridge(const std::string& romFileName);
        Cartridge(const std::vector<uint8_t>& catridgeData);

        uint8_t operator [](int i) const;
        size_t size() const;

    private:
        std::vector<uint8_t> cartridgeData_;
    };

} // gbemu

#endif // GBEMU_CARTRIDGE