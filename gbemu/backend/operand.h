#ifndef GBEMU_BACKEND_OPERAND_H
#define GBEMU_BACKEND_OPERAND_H

#include <cstdint>

namespace gbemu::backend
{

enum class Register : uint8_t
{
    A,
    B,
    C,
    D,
    E,
    H,
    L
};

enum class FullRegister : uint8_t
{
    BC,
    DE,
    HL,
    AF
};

enum class SpecialRegister : uint8_t
{
    SP
};

struct DereferencedFullRegister
{
    FullRegister fullRegister;
};

struct Operand
{
    enum class Kind : uint8_t
    {
        NONE,
        REG,
        FULL_REG,
        SPECIAL_REG,
        DEREF_FULL_REG
    };

    Kind kind = Kind::NONE;
    uint8_t value = 0;

    constexpr Operand() = default;
    constexpr Operand(Register r) : kind(Kind::REG), value(static_cast<uint8_t>(r)) {}
    constexpr Operand(FullRegister r) : kind(Kind::FULL_REG), value(static_cast<uint8_t>(r)) {}
    constexpr Operand(SpecialRegister r) : kind(Kind::SPECIAL_REG), value(static_cast<uint8_t>(r)) {}
    constexpr Operand(DereferencedFullRegister r)
        : kind(Kind::DEREF_FULL_REG), value(static_cast<uint8_t>(r.fullRegister))
    {}

    [[nodiscard]] constexpr auto isNone() const -> bool { return kind == Kind::NONE; }
    [[nodiscard]] constexpr auto isRegister() const -> bool { return kind == Kind::REG; }
    [[nodiscard]] constexpr auto isFullRegister() const -> bool { return kind == Kind::FULL_REG; }
    [[nodiscard]] constexpr auto isSpecialRegister() const -> bool { return kind == Kind::SPECIAL_REG; }
    [[nodiscard]] constexpr auto isDereferencedFullRegister() const -> bool { return kind == Kind::DEREF_FULL_REG; }

    [[nodiscard]] constexpr auto asRegister() const -> Register { return static_cast<Register>(value); }
    [[nodiscard]] constexpr auto asFullRegister() const -> FullRegister { return static_cast<FullRegister>(value); }
    [[nodiscard]] constexpr auto asSpecialRegister() const -> SpecialRegister
    {
        return static_cast<SpecialRegister>(value);
    }
    [[nodiscard]] constexpr auto asDereferencedFullRegister() const -> FullRegister
    {
        return static_cast<FullRegister>(value);
    }

    /// Returns true if this operand resolves to an 8-bit value.
    [[nodiscard]] constexpr auto is8bit() const -> bool { return kind == Kind::REG || kind == Kind::DEREF_FULL_REG; }
};

struct OperandValue
{
    uint16_t value = 0;
    bool wide = false;

    constexpr OperandValue() = default;
    constexpr OperandValue(uint8_t v) : value(v), wide(false) {}
    constexpr OperandValue(uint16_t v) : value(v), wide(true) {}

    [[nodiscard]] constexpr auto is8bit() const -> bool { return !wide; }
    [[nodiscard]] constexpr auto is16bit() const -> bool { return wide; }
    [[nodiscard]] constexpr auto as8() const -> uint8_t { return static_cast<uint8_t>(value); }
    [[nodiscard]] constexpr auto as16() const -> uint16_t { return value; }
};

} // namespace gbemu::backend

#endif // GBEMU_BACKEND_OPERAND_H
