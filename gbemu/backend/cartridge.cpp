#include "gbemu/backend/cartridge.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace gbemu::backend
{

Cartridge::Cartridge(const std::string &romFileName)
{
    std::ifstream source_file{romFileName, std::ios::binary};

    if (!source_file.good())
    {
        std::cerr << "Unable to read file " << romFileName << std::endl;
        throw std::runtime_error{"Unable to read file"};
    }

    const auto length = std::filesystem::file_size(romFileName);
    cartridgeData_ = std::vector<uint8_t>(length);
    source_file.read(reinterpret_cast<char *>(cartridgeData_.data()), static_cast<long>(length));
}

Cartridge::Cartridge(const std::vector<uint8_t> &cartridgeData) : cartridgeData_(cartridgeData) {}

} // namespace gbemu::backend
