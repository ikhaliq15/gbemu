#ifndef GBEMU_BITUTILS
#define GBEMU_BITUTILS

#include <stdint.h>
#include <string>
#include <sstream>
#include <iomanip>

namespace gbemu {
    // TODO: test these methods!

    // TODO: makes a lot of assumptions on endinaness? can we remove such an assumption?
    inline uint8_t upperByte(uint16_t n) { return n >> 8; }
    inline uint8_t lowerByte(uint16_t n) { return n & 0x00FF; }
    inline uint16_t setUpperByte(uint16_t n, uint8_t b) { return (n & 0x00FF) | (b << 8); }
    inline uint16_t setLowerByte(uint16_t n, uint8_t b) { return (n & 0xFF00) | b; }
    inline uint16_t concatBytes(uint8_t upper, uint8_t lower) { return (upper << 8) | lower; }
    inline uint8_t swapNibbles(uint8_t n) { return ((n & 0x0f) << 4) | ((n & 0xf0) >> 4); }
    inline uint8_t interpolateNibbles(uint8_t upper, uint8_t lower) { return (upper & 0xF0) | (lower & 0x0F); }

    // Note: least significant bit is the zero-th bit
    template <typename T>
    inline T getBit(T val, uint8_t bit) { return (val >> (bit)) & 0x01; }

    template <typename T> 
    inline T setBit(T val, uint8_t bit, uint8_t newBit) {
        return (val & ~(1 << bit)) | (newBit << bit);
    }

    inline std::string toHexString(uint8_t n, bool prefixed = true) {
        std::stringstream os;
        if (prefixed)
            os << "0x";
        os << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int) n;
        return os.str();
    }

    inline std::string toHexString(uint16_t n, bool prefixed = true) {
        std::stringstream os;
        if (prefixed)
             os << "0x";
        os << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << (int) n;
        return os.str();
    }

    inline bool addHadCarry(uint8_t a, uint8_t b)
    {
        uint32_t promotedA = (uint32_t) a;
        uint32_t promotedB = (uint32_t) b;
        return ((promotedA + promotedB) & 0x00000100) == 0x00000100;
    }

    inline bool addHadCarry(uint8_t a, uint8_t b, uint8_t c)
    {
        uint32_t promotedA = (uint32_t) a;
        uint32_t promotedB = (uint32_t) b;
        uint32_t promotedC = (uint32_t) c;
        return ((promotedA + promotedB + promotedC) & 0x00000100) == 0x00000100;
    }

    inline bool addHadCarry(uint16_t a, uint16_t b)
    {
        uint32_t promotedA = (uint32_t) a;
        uint32_t promotedB = (uint32_t) b;
        return ((promotedA + promotedB) & 0x00010000) == 0x00010000;
    }

    inline bool addHadHalfCarry(uint8_t a, uint8_t b)
    {
        return (((0x0F & a) + (0x0F & b)) & 0x10) == 0x10;
    }

    inline bool addHadHalfCarry(uint8_t a, uint8_t b, uint8_t c)
    {
        return (((0x0F & a) + (0x0F & b)) + (0x0F & c) & 0x10) == 0x10;
    }

    inline bool addHadHalfCarry(uint16_t a, uint16_t b)
    {
        return (((0x0FFF & a) + (0x0FFF & b)) & 0x1000) == 0x1000;
    }

    inline bool subHadCarry(uint8_t a, uint8_t b)
    {
        // TODO: test correctness
        return (int32_t)(a & 0xFF) - (int32_t)(b & 0xFF) < 0;
    }

    inline bool subHadCarry(uint8_t a, uint8_t b, uint8_t c)
    {
        // TODO: test correctness
        return (int32_t)(a & 0xFF) - (int32_t)(b & 0xFF) - (int32_t)(c & 0xFF) < 0;
    }

    inline bool subHadCarry(uint16_t a, uint16_t b)
    {
        // TODO: test correctness
        return (int32_t)(a & 0xFFFF) - (int32_t)(b & 0xFFFF) < 0;
    }

    inline bool subHadHalfCarry(uint8_t a, uint8_t b)
    {
        return (int32_t)(a & 0x0F) - (int32_t)(b & 0x0F) < 0;
    }

    inline bool subHadHalfCarry(uint8_t a, uint8_t b, uint8_t c)
    {
        return (int32_t)(a & 0x0F) - (int32_t)(b & 0x0F) - (int32_t)(c & 0x0F) < 0;
    }

    inline bool subHadHalfCarry(uint16_t a, uint16_t b)
    {
        return (int32_t)(a & 0x0FFF) - (int32_t)(b & 0x0FFF) < 0;
    }

} // gbemu

#endif // GBEMU_BITUTILS