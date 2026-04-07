#ifndef GBEMU_BACKEND_OPCODE_H
#define GBEMU_BACKEND_OPCODE_H

#include "gbemu/backend/operand.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace gbemu::backend
{

struct OPCode
{
    enum class Flag
    {
        UNTOUCHED,
        ONE,
        ZERO,
        Z,
        H,
        CY,
        NOT_CY,
        A0,
        A1,
        A2,
        A3,
        A4,
        A5,
        A6,
        A7,
        NOT_A0,
        NOT_A1,
        NOT_A2,
        NOT_A3,
        NOT_A4,
        NOT_A5,
        NOT_A6,
        NOT_A7,
    };

    enum class JumpCondition
    {
        ALWAYS,
        Z,
        C,
        NZ,
        NC,
    };

    using Flags = std::array<Flag, 4>;

    static constexpr uint8_t PREFIX_OPCODE = 0xcb;
    static constexpr uint8_t MAX_OPERANDS = 2;
    static constexpr uint8_t MAX_AUX_ARGS = 2;

    uint8_t opcode{};
    std::string_view command{};
    std::string_view mnemonic{};
    std::array<Operand, MAX_OPERANDS> operands{};
    uint8_t numOperands{};
    std::array<uint8_t, MAX_AUX_ARGS> auxiliaryArguments{};
    uint8_t numAuxArgs{};
    uint8_t bytes{};
    uint8_t cycles{};
    uint8_t additionalCycles{};
    JumpCondition jumpCondition{JumpCondition::ALWAYS};
    Flags flags{};
    bool valid{false};
};

} // namespace gbemu::backend

#endif // GBEMU_BACKEND_OPCODE_H
