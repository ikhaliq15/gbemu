#ifndef GBEMU_BITUTILS
#define GBEMU_BITUTILS

#include <concepts>
#include <cstdint>
#include <string>

namespace gbemu
{

constexpr uint8_t upperByte(uint16_t n) noexcept
{
    return static_cast<uint8_t>(n >> 8);
}

constexpr uint8_t lowerByte(uint16_t n) noexcept
{
    return static_cast<uint8_t>(n);
}

constexpr uint16_t setUpperByte(uint16_t n, uint8_t b) noexcept
{
    return static_cast<uint16_t>((n & 0x00FFu) | (static_cast<uint16_t>(b) << 8));
}

constexpr uint16_t setLowerByte(uint16_t n, uint8_t b) noexcept
{
    return static_cast<uint16_t>((n & 0xFF00u) | b);
}

constexpr uint16_t concatBytes(uint8_t upper, uint8_t lower) noexcept
{
    return static_cast<uint16_t>((static_cast<uint16_t>(upper) << 8) | lower);
}

constexpr uint8_t swapNibbles(uint8_t n) noexcept
{
    return static_cast<uint8_t>((n << 4) | (n >> 4));
}

constexpr uint8_t interpolateNibbles(uint8_t upper, uint8_t lower) noexcept
{
    return static_cast<uint8_t>((upper & 0xF0u) | (lower & 0x0Fu));
}

template <std::unsigned_integral T> constexpr bool getBit(T val, uint8_t bit) noexcept
{
    return (val >> bit) & T{1};
}

template <std::unsigned_integral T> constexpr T setBit(T val, uint8_t bit, bool newBit) noexcept
{
    const T mask = T{1} << bit;
    return newBit ? (val | mask) : (val & ~mask);
}

inline std::string toHexString(uint8_t n, bool prefixed = true)
{
    constexpr char hex[] = "0123456789ABCDEF";

    std::string out;
    if (prefixed)
        out += "0x";

    out += hex[(n >> 4) & 0xF];
    out += hex[n & 0xF];

    return out;
}

inline std::string toHexString(uint16_t n, bool prefixed = true)
{
    constexpr char hex[] = "0123456789ABCDEF";

    std::string out;
    if (prefixed)
        out += "0x";

    out += hex[(n >> 12) & 0xF];
    out += hex[(n >> 8) & 0xF];
    out += hex[(n >> 4) & 0xF];
    out += hex[n & 0xF];

    return out;
}

constexpr bool addHadCarry(uint8_t a, uint8_t b, uint8_t c = 0) noexcept
{
    return static_cast<uint16_t>(a) + b + c > 0xFF;
}

constexpr bool addHadCarry(uint16_t a, uint16_t b) noexcept
{
    return static_cast<uint32_t>(a) + b > 0xFFFF;
}

constexpr bool addHadHalfCarry(uint8_t a, uint8_t b, uint8_t c = 0) noexcept
{
    return ((a & 0x0F) + (b & 0x0F) + (c & 0x0F)) > 0x0F;
}

constexpr bool addHadHalfCarry(uint16_t a, uint16_t b) noexcept
{
    return ((a & 0x0FFF) + (b & 0x0FFF)) > 0x0FFF;
}

template <std::unsigned_integral T> constexpr bool subHadCarry(T a, T b, T c = 0) noexcept
{
    using WideT = std::make_unsigned_t<decltype(a + b + c + 0U)>;
    return static_cast<WideT>(a) < static_cast<WideT>(b) + static_cast<WideT>(c);
}

constexpr bool subHadHalfCarry(uint8_t a, uint8_t b, uint8_t c = 0) noexcept
{
    return (a & 0x0F) < ((b & 0x0F) + (c & 0x0F));
}

constexpr bool subHadHalfCarry(uint16_t a, uint16_t b) noexcept
{
    return (a & 0x0FFF) < (b & 0x0FFF);
}

} // namespace gbemu

#endif // GBEMU_BITUTILS
