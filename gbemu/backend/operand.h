#ifndef GBEMU_BACKEND_OPERAND_H
#define GBEMU_BACKEND_OPERAND_H

#include <cstdint>
#include <string>
#include <unordered_map>
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

const std::unordered_map<std::string, Operand> OPERANDS{
    {"A", Register::A},
    {"B", Register::B},
    {"C", Register::C},
    {"D", Register::D},
    {"E", Register::E},
    {"H", Register::H},
    {"L", Register::L},
    {"BC", FullRegister::BC},
    {"DE", FullRegister::DE},
    {"HL", FullRegister::HL},
    {"AF", FullRegister::AF},
    {"SP", SpecialRegister::SP},
    {"(BC)", DereferencedFullRegister{FullRegister::BC}},
    {"(DE)", DereferencedFullRegister{FullRegister::DE}},
    {"(HL)", DereferencedFullRegister{FullRegister::HL}},
};

} // namespace gbemu::backend

#endif // GBEMU_BACKEND_OPERAND_H
