#include "gbemu/backend/opcode.h"

#include "gbemu/backend/bitutils.h"
#include "gbemu/backend/opcode_data.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace gbemu::backend
{

std::deque<std::unique_ptr<OPCode>> OPCode::opcodesStorage_;

OPCode::OPCode(uint8_t opcode, const std::string command, const std::string mnemonic,
               const std::vector<uint8_t> auxiliaryArguments, const std::vector<Operand> operands, uint8_t bytes,
               uint8_t cycles, uint8_t additionalCycles, JumpCondition jumpCondition, Flags flags)
    : opcode_(opcode), command_(command), mnemonic_(mnemonic), auxiliaryArguments_(auxiliaryArguments),
      operands_(operands), bytes_(bytes), cycles_(cycles), additionalCycles_(additionalCycles),
      jumpCondition_(jumpCondition), flags_(flags)
{
}

auto OPCode::constructOpcodes() -> std::pair<OPCode::OpCodeMap, OPCode::OpCodeMap>
{
    const auto opcodeData = json::parse(opcodes::OPCODES_DATA).get<std::vector<json>>();

    OpCodeMap opcodesMap{};
    OpCodeMap prefixedOpcodesMap{};
    for (const auto &opcode : opcodeData)
    {
        const auto prefixed = opcode.contains("prefix") && opcode["prefix"].get<bool>();
        const auto opcodeMap = prefixed ? &prefixedOpcodesMap : &opcodesMap;

        const auto op = static_cast<uint8_t>(std::strtoul(opcode["opcode"].get<std::string>().c_str(), nullptr, 16));

        if (opcodeMap->at(op))
            throw std::runtime_error(std::string("Repeated opcode in opcode data: ") + toHexString(op));

        const auto command = opcode["command"].get<std::string>();
        const auto mnemonic = opcode["mnemonic"].get<std::string>();
        const auto bytes = opcode["bytes"].get<uint8_t>();
        const auto cycles = opcode["cycles"].get<uint8_t>();

        uint8_t additionalCycles = 0;
        if (opcode.contains("additionalCycles"))
            additionalCycles = opcode["additionalCycles"].get<uint8_t>();

        JumpCondition jumpCondition = JumpCondition::ALWAYS;
        if (opcode.contains("jumpCondition"))
        {
            const auto jumpConditionStr = opcode["jumpCondition"].get<std::string>();
            const auto it = JUMP_CONDITIONS.find(jumpConditionStr);
            if (it == JUMP_CONDITIONS.end())
                throw std::runtime_error(std::string("Unknown jump condition in opcode data: ") + jumpConditionStr);
            jumpCondition = it->second;
        }

        std::vector<Operand> operands;
        for (const auto &operand : opcode["operands"].get<std::vector<std::string>>())
        {
            const auto it = OPERANDS.find(operand);
            if (it == OPERANDS.end())
                throw std::runtime_error(std::string("Unknown operand in opcode data: ") + operand);

            operands.push_back(it->second);
        }

        std::vector<uint8_t> auxiliaryArguments;
        if (opcode.contains("aux_args"))
            auxiliaryArguments = opcode["aux_args"].get<std::vector<uint8_t>>();

        const auto opcodeFlags = opcode["flagsZNHC"].get<std::vector<std::string>>();
        if (opcodeFlags.size() != 4)
            throw std::runtime_error(std::string("Invalid number of flags in opcode data."));

        Flags flags;
        for (int i = 0; i < 4; i++)
        {
            const auto flag = opcodeFlags[i];
            const auto it = FLAGS.find(flag);
            if (it == FLAGS.end())
                throw std::runtime_error(std::string("Unknown flag in opcode data: ") + flag);

            flags[i] = it->second;
        }

        auto opcodePtr = std::make_unique<OPCode>(op, command, mnemonic, auxiliaryArguments, operands, bytes, cycles,
                                                  additionalCycles, jumpCondition, flags);
        opcodeMap->at(op) = opcodePtr.get();
        opcodesStorage_.push_back(std::move(opcodePtr));
    }

    return std::make_pair(opcodesMap, prefixedOpcodesMap);
}

auto OPCode::opcode() const -> uint8_t
{
    return opcode_;
}
auto OPCode::command() const -> const std::string &
{
    return command_;
}
auto OPCode::mnemonic() const -> const std::string &
{
    return mnemonic_;
}
auto OPCode::auxiliaryArguments() const -> const std::vector<uint8_t> &
{
    return auxiliaryArguments_;
}
auto OPCode::operands() const -> const std::vector<Operand> &
{
    return operands_;
}
auto OPCode::bytes() const -> uint8_t
{
    return bytes_;
}
auto OPCode::cycles() const -> uint8_t
{
    return cycles_;
}
auto OPCode::additionalCycles() const -> uint8_t
{
    return additionalCycles_;
}
auto OPCode::jumpCondition() const -> OPCode::JumpCondition
{
    return jumpCondition_;
}

auto OPCode::flags() const -> OPCode::Flags
{
    return flags_;
}
auto OPCode::flagZ() const -> OPCode::Flag
{
    return flags_[0];
}
auto OPCode::flagN() const -> OPCode::Flag
{
    return flags_[1];
}
auto OPCode::flagH() const -> OPCode::Flag
{
    return flags_[2];
}
auto OPCode::flagC() const -> OPCode::Flag
{
    return flags_[3];
}
} // namespace gbemu::backend
