#ifndef GBEMU_BACKEND_BITUTILS_H
#define GBEMU_BACKEND_BITUTILS_H

#include <concepts>
#include <cstdint>
#include <string>

namespace gbemu::backend
{

constexpr auto upperByte(uint16_t n) noexcept -> uint8_t { return static_cast<uint8_t>(n >> 8); }

constexpr auto lowerByte(uint16_t n) noexcept -> uint8_t { return static_cast<uint8_t>(n); }

constexpr auto setUpperByte(uint16_t n, uint8_t b) noexcept -> uint16_t
{
    return static_cast<uint16_t>((n & 0x00FFu) | (static_cast<uint16_t>(b) << 8));
}

constexpr auto setLowerByte(uint16_t n, uint8_t b) noexcept -> uint16_t
{
    return static_cast<uint16_t>((n & 0xFF00u) | b);
}

constexpr auto concatBytes(uint8_t upper, uint8_t lower) noexcept -> uint16_t
{
    return static_cast<uint16_t>((static_cast<uint16_t>(upper) << 8) | lower);
}

constexpr auto swapNibbles(uint8_t n) noexcept -> uint8_t { return static_cast<uint8_t>((n << 4) | (n >> 4)); }

constexpr auto interpolateNibbles(uint8_t upper, uint8_t lower) noexcept -> uint8_t
{
    return static_cast<uint8_t>((upper & 0xF0u) | (lower & 0x0Fu));
}

template <std::unsigned_integral T> constexpr auto getBit(T val, uint8_t bit) noexcept -> bool
{
    return (val >> bit) & T{1};
}

template <std::unsigned_integral T> constexpr auto setBit(T val, uint8_t bit, bool newBit) noexcept -> T
{
    const T mask = T{1} << bit;
    return newBit ? (val | mask) : (val & ~mask);
}

inline auto toHexString(uint8_t n, bool prefixed = true) -> std::string
{
    constexpr char hex[] = "0123456789ABCDEF";

    std::string out;
    if (prefixed)
    {
        out += "0x";
    }

    out += hex[(n >> 4) & 0xF];
    out += hex[n & 0xF];

    return out;
}

inline auto toHexString(uint16_t n, bool prefixed = true) -> std::string
{
    constexpr char hex[] = "0123456789ABCDEF";

    std::string out;
    if (prefixed)
    {
        out += "0x";
    }

    out += hex[(n >> 12) & 0xF];
    out += hex[(n >> 8) & 0xF];
    out += hex[(n >> 4) & 0xF];
    out += hex[n & 0xF];

    return out;
}

constexpr auto addHadCarry(uint8_t a, uint8_t b, uint8_t c = 0) noexcept -> bool
{
    return static_cast<uint16_t>(a) + b + c > 0xFF;
}

constexpr auto addHadCarry(uint16_t a, uint16_t b) noexcept -> bool { return static_cast<uint32_t>(a) + b > 0xFFFF; }

constexpr auto addHadHalfCarry(uint8_t a, uint8_t b, uint8_t c = 0) noexcept -> bool
{
    return ((a & 0x0F) + (b & 0x0F) + (c & 0x0F)) > 0x0F;
}

constexpr auto addHadHalfCarry(uint16_t a, uint16_t b) noexcept -> bool
{
    return ((a & 0x0FFF) + (b & 0x0FFF)) > 0x0FFF;
}

template <std::unsigned_integral T> constexpr auto subHadCarry(T a, T b, T c = 0) noexcept -> bool
{
    using WideT = std::make_unsigned_t<decltype(a + b + c + 0U)>;
    return static_cast<WideT>(a) < static_cast<WideT>(b) + static_cast<WideT>(c);
}

constexpr auto subHadHalfCarry(uint8_t a, uint8_t b, uint8_t c = 0) noexcept -> bool
{
    return (a & 0x0F) < ((b & 0x0F) + (c & 0x0F));
}

constexpr auto subHadHalfCarry(uint16_t a, uint16_t b) noexcept -> bool { return (a & 0x0FFF) < (b & 0x0FFF); }

} // namespace gbemu::backend

#endif // GBEMU_BACKEND_BITUTILS_H
