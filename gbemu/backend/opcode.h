#ifndef GBEMU_BACKEND_OPCODE_H
#define GBEMU_BACKEND_OPCODE_H

#include "gbemu/backend/operand.h"

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace gbemu::backend
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

    using Flags = std::array<Flag, 4>;

    static constexpr uint8_t PREFIX_OPCODE = 0xcb;

    OPCode(uint8_t opcode, std::string command, std::string mnemonic, std::vector<uint8_t> auxiliaryArguments,
           std::vector<Operand> operands, uint8_t bytes, uint8_t cycles, uint8_t additionalCycles,
           JumpCondition jumpCondition, Flags flags);

    [[nodiscard]] uint8_t opcode() const { return opcode_; }
    [[nodiscard]] const std::string &command() const { return command_; }
    [[nodiscard]] const std::string &mnemonic() const { return mnemonic_; }
    [[nodiscard]] const std::vector<uint8_t> &auxiliaryArguments() const { return auxiliaryArguments_; }
    [[nodiscard]] const std::vector<Operand> &operands() const { return operands_; }
    [[nodiscard]] uint8_t bytes() const { return bytes_; }
    [[nodiscard]] uint8_t cycles() const { return cycles_; }
    [[nodiscard]] uint8_t additionalCycles() const { return additionalCycles_; }
    [[nodiscard]] JumpCondition jumpCondition() const { return jumpCondition_; }
    [[nodiscard]] Flags flags() const { return flags_; }
    [[nodiscard]] Flag flagZ() const { return flags_[0]; }
    [[nodiscard]] Flag flagN() const { return flags_[1]; }
    [[nodiscard]] Flag flagH() const { return flags_[2]; }
    [[nodiscard]] Flag flagC() const { return flags_[3]; }

  private:
    uint8_t opcode_;
    std::string command_;
    std::string mnemonic_;
    std::vector<uint8_t> auxiliaryArguments_;
    std::vector<Operand> operands_;
    uint8_t bytes_;
    uint8_t cycles_;
    uint8_t additionalCycles_;
    JumpCondition jumpCondition_;
    Flags flags_;
};

// Singleton table of all opcodes, constructed once from JSON data.
class OpcodeTable
{
  public:
    using OpCodeMap = std::array<const OPCode *, 256>;

    static const OpcodeTable &instance();

    [[nodiscard]] const OpCodeMap &opcodes() const { return opcodes_; }
    [[nodiscard]] const OpCodeMap &prefixedOpcodes() const { return prefixedOpcodes_; }

  private:
    OpcodeTable();

    std::vector<std::unique_ptr<OPCode>> storage_;
    OpCodeMap opcodes_{};
    OpCodeMap prefixedOpcodes_{};
};

} // namespace gbemu::backend

#endif // GBEMU_BACKEND_OPCODE_H
