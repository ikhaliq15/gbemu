#ifndef GBEMU_OPCODE
#define GBEMU_OPCODE

#include "gbemu/operand.h"

#include <array>
#include <map>
#include <stdint.h>
#include <string>
#include <vector>

namespace gbemu
{

class OPCode
{
  public:
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

    static constexpr uint8_t PREFIX_OPCODE = 0xcb;

    using Flags = std::array<Flag, 4>;

    OPCode(uint8_t opcode, const std::string command, const std::string mnemonic,
           const std::vector<uint8_t> auxiliaryArguments, const std::vector<Operand> operands, uint8_t bytes,
           uint8_t cycles, uint8_t additionalCycles, JumpCondition jumpCondition, Flags flags);

    static std::pair<std::map<uint8_t, OPCode>, std::map<uint8_t, OPCode>> constructOpcodes();

    uint8_t opcode() const;
    std::string command() const;
    std::string mnemonic() const;
    std::vector<uint8_t> auxiliaryArguments() const;
    std::vector<Operand> operands() const;
    uint8_t bytes() const;
    uint8_t cycles() const;
    uint8_t additionalCycles() const;
    JumpCondition jumpCondition() const;
    Flags flags() const;
    Flag flagZ() const;
    Flag flagN() const;
    Flag flagH() const;
    Flag flagC() const;

  private:
    const uint8_t opcode_;
    const std::string command_;
    const std::string mnemonic_;
    const std::vector<uint8_t> auxiliaryArguments_;
    const std::vector<Operand> operands_;
    const uint8_t bytes_;
    const uint8_t cycles_;
    const uint8_t additionalCycles_;
    const JumpCondition jumpCondition_;
    const Flags flags_;
};

const std::unordered_map<std::string, OPCode::Flag> FLAGS{
    {"-", OPCode::Flag::UNTOUCHED}, {"1", OPCode::Flag::ONE},      {"0", OPCode::Flag::ZERO},
    {"Z", OPCode::Flag::Z},         {"H", OPCode::Flag::H},        {"CY", OPCode::Flag::CY},
    {"!CY", OPCode::Flag::NOT_CY},  {"A0", OPCode::Flag::A0},      {"A1", OPCode::Flag::A1},
    {"A2", OPCode::Flag::A2},       {"A3", OPCode::Flag::A3},      {"A4", OPCode::Flag::A4},
    {"A5", OPCode::Flag::A5},       {"A6", OPCode::Flag::A6},      {"A7", OPCode::Flag::A7},
    {"!A0", OPCode::Flag::NOT_A0},  {"!A1", OPCode::Flag::NOT_A1}, {"!A2", OPCode::Flag::NOT_A2},
    {"!A3", OPCode::Flag::NOT_A3},  {"!A4", OPCode::Flag::NOT_A4}, {"!A5", OPCode::Flag::NOT_A5},
    {"!A6", OPCode::Flag::NOT_A6},  {"!A7", OPCode::Flag::NOT_A7},
};

const std::unordered_map<std::string, OPCode::JumpCondition> JUMP_CONDITIONS{
    {"ALWAYS", OPCode::JumpCondition::ALWAYS}, {"Z", OPCode::JumpCondition::Z},   {"C", OPCode::JumpCondition::C},
    {"NZ", OPCode::JumpCondition::NZ},         {"NC", OPCode::JumpCondition::NC},
};

} // namespace gbemu

#endif // GBEMU_OPCODE
