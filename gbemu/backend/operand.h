#ifndef GBEMU_BACKEND_OPERAND_H
#define GBEMU_BACKEND_OPERAND_H

#include <cstdint>
#include <variant>

namespace gbemu::backend
{

enum class Register
{
    A,
    B,
    C,
    D,
    E,
    H,
    L
};

enum class FullRegister
{
    BC,
    DE,
    HL,
    AF
};

enum class SpecialRegister
{
    SP
};

struct DereferencedFullRegister
{
    FullRegister fullRegister;
};

using Operand = std::variant<Register, FullRegister, SpecialRegister, DereferencedFullRegister>;
using OperandValue = std::variant<uint8_t, uint16_t>;

} // namespace gbemu::backend

#endif // GBEMU_BACKEND_OPERAND_H
