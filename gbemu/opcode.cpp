#include "gbemu/opcode.h"

#include "gbemu/bitutils.h"
#include "gbemu/opcode_data.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace gbemu
{

OPCode::OPCode(uint8_t opcode, const std::string command, const std::string mnemonic,
               const std::vector<uint8_t> auxiliaryArguments, const std::vector<Operand> operands, uint8_t bytes,
               uint8_t cycles, uint8_t additionalCycles, JumpCondition jumpCondition, Flags flags)
    : opcode_(opcode), command_(command), mnemonic_(mnemonic), auxiliaryArguments_(auxiliaryArguments),
      operands_(operands), bytes_(bytes), cycles_(cycles), additionalCycles_(additionalCycles),
      jumpCondition_(jumpCondition), flags_(flags)
{
}

std::pair<std::map<uint8_t, OPCode>, std::map<uint8_t, OPCode>> OPCode::constructOpcodes()
{
    const auto opcodeData = json::parse(opcodes::OPCODES_DATA).get<std::vector<json>>();

    std::map<uint8_t, OPCode> opcodesMap;
    std::map<uint8_t, OPCode> prefixedOpcodesMap;
    for (const auto &opcode : opcodeData)
    {
        const auto prefixed = opcode.contains("prefix") && opcode["prefix"].get<bool>();
        const auto opcodeMap = prefixed ? &prefixedOpcodesMap : &opcodesMap;

        const auto op = (uint8_t)std::strtoul(opcode["opcode"].get<std::string>().c_str(), nullptr, 16);

        if (opcodeMap->find(op) != opcodeMap->end())
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

        opcodeMap->insert({op, OPCode(op, command, mnemonic, auxiliaryArguments, operands, bytes, cycles,
                                      additionalCycles, jumpCondition, flags)});
    }

    return std::make_pair(opcodesMap, prefixedOpcodesMap);
}

uint8_t OPCode::opcode() const
{
    return opcode_;
}
std::string OPCode::command() const
{
    return command_;
}
std::string OPCode::mnemonic() const
{
    return mnemonic_;
}
std::vector<uint8_t> OPCode::auxiliaryArguments() const
{
    return auxiliaryArguments_;
}
std::vector<Operand> OPCode::operands() const
{
    return operands_;
}
uint8_t OPCode::bytes() const
{
    return bytes_;
}
uint8_t OPCode::cycles() const
{
    return cycles_;
}
uint8_t OPCode::additionalCycles() const
{
    return additionalCycles_;
}
OPCode::JumpCondition OPCode::jumpCondition() const
{
    return jumpCondition_;
}

OPCode::Flags OPCode::flags() const
{
    return flags_;
}
OPCode::Flag OPCode::flagZ() const
{
    return flags_[0];
}
OPCode::Flag OPCode::flagN() const
{
    return flags_[1];
}
OPCode::Flag OPCode::flagH() const
{
    return flags_[2];
}
OPCode::Flag OPCode::flagC() const
{
    return flags_[3];
}
} // namespace gbemu
