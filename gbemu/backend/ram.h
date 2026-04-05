#ifndef GBEMU_BACKEND_RAM_H
#define GBEMU_BACKEND_RAM_H

#include "gbemu/backend/cartridge.h"

#include <iostream>
#include <vector>

namespace gbemu::backend
{

class RAM
{
  public:
    class ReadOwner
    {
      public:
        ReadOwner() = default;
        virtual ~ReadOwner() = default;
        virtual uint8_t onReadOwnedByte(uint16_t address) = 0;
    };

    class WriteOwner
    {
      public:
        WriteOwner() = default;
        virtual ~WriteOwner() = default;
        virtual void onWriteOwnedByte(uint16_t address, uint8_t newValue, uint8_t currentValue) = 0;
    };

    class Owner : public WriteOwner, public ReadOwner
    {
    };

    static constexpr uint16_t OAM = 0xfe00;
    static constexpr uint16_t JOYP = 0xff00;
    static constexpr uint16_t SB = 0xff01;
    static constexpr uint16_t SC = 0xff02;
    static constexpr uint16_t DIV = 0xff04;
    static constexpr uint16_t TIMA = 0xff05;
    static constexpr uint16_t TMA = 0xff06;
    static constexpr uint16_t TAC = 0xff07;
    static constexpr uint16_t IF = 0xff0f;
    static constexpr uint16_t LCDC = 0xff40;
    static constexpr uint16_t STAT = 0xff41;
    static constexpr uint16_t SCY = 0xff42;
    static constexpr uint16_t SCX = 0xff43;
    static constexpr uint16_t LY = 0xff44;
    static constexpr uint16_t LYC = 0xff45;
    static constexpr uint16_t DMA = 0xff46;
    static constexpr uint16_t BGP = 0xff47;
    static constexpr uint16_t OBP0 = 0xff48;
    static constexpr uint16_t OBP1 = 0xff49;
    static constexpr uint16_t WY = 0xff4a;
    static constexpr uint16_t WX = 0xff4b;
    static constexpr uint16_t IE = 0xffff;

    RAM(uint32_t ramSize, uint8_t defaultValue = 0);
    RAM(const RAM &ram);

    bool operator==(const RAM &rhs) const;

    friend std::ostream &operator<<(std::ostream &os, const RAM &ram);

    void loadCartridge(const Cartridge &cartridge);

    [[nodiscard]] uint16_t getImmediate16(uint16_t i) const;
    void setImmediate16(uint16_t i, uint16_t newVal);

    [[nodiscard]] uint8_t get(uint16_t address) const;
    void set(uint16_t address, uint8_t value);

    void addReadOwner(uint16_t address, ReadOwner *owner);
    void addWriteOwner(uint16_t address, WriteOwner *owner);
    void addOwner(uint16_t address, Owner *owner);

  private:
    std::vector<uint8_t> memory_;
    std::vector<ReadOwner *> readOwners_;
    std::vector<WriteOwner *> writeOwners_;
};

} // namespace gbemu::backend

#endif // GBEMU_BACKEND_RAM_H
