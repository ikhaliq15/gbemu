#include "gbemu/backend/opcode.h"

#include "gbemu/backend/bitutils.h"
#include "gbemu/backend/opcode_data.h"

#include <nlohmann/json.hpp>

#include <unordered_map>
#include <utility>

using json = nlohmann::json;

namespace gbemu::backend
{

namespace
{

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

} // namespace

OPCode::OPCode(uint8_t opcode, std::string command, std::string mnemonic, std::vector<uint8_t> auxiliaryArguments,
               std::vector<Operand> operands, uint8_t bytes, uint8_t cycles, uint8_t additionalCycles,
               JumpCondition jumpCondition, Flags flags)
    : opcode_(opcode), command_(std::move(command)), mnemonic_(std::move(mnemonic)),
      auxiliaryArguments_(std::move(auxiliaryArguments)), operands_(std::move(operands)), bytes_(bytes),
      cycles_(cycles), additionalCycles_(additionalCycles), jumpCondition_(jumpCondition), flags_(flags)
{}

const OpcodeTable &OpcodeTable::instance()
{
    static const OpcodeTable table;
    return table;
}

OpcodeTable::OpcodeTable()
{
    const auto opcodeData = json::parse(opcodes::OPCODES_DATA).get<std::vector<json>>();

    for (const auto &entry : opcodeData)
    {
        const auto prefixed = entry.contains("prefix") && entry["prefix"].get<bool>();
        auto &map = prefixed ? prefixedOpcodes_ : opcodes_;

        const auto op = static_cast<uint8_t>(std::strtoul(entry["opcode"].get<std::string>().c_str(), nullptr, 16));

        if (map.at(op))
            throw std::runtime_error(std::string("Repeated opcode in opcode data: ") + toHexString(op));

        const auto command = entry["command"].get<std::string>();
        const auto mnemonic = entry["mnemonic"].get<std::string>();
        const auto bytes = entry["bytes"].get<uint8_t>();
        const auto cycles = entry["cycles"].get<uint8_t>();

        uint8_t additionalCycles = 0;
        if (entry.contains("additionalCycles"))
            additionalCycles = entry["additionalCycles"].get<uint8_t>();

        OPCode::JumpCondition jumpCondition = OPCode::JumpCondition::ALWAYS;
        if (entry.contains("jumpCondition"))
        {
            const auto str = entry["jumpCondition"].get<std::string>();
            const auto it = JUMP_CONDITIONS.find(str);
            if (it == JUMP_CONDITIONS.end())
                throw std::runtime_error(std::string("Unknown jump condition in opcode data: ") + str);
            jumpCondition = it->second;
        }

        std::vector<Operand> operands;
        for (const auto &operand : entry["operands"].get<std::vector<std::string>>())
        {
            const auto it = OPERANDS.find(operand);
            if (it == OPERANDS.end())
                throw std::runtime_error(std::string("Unknown operand in opcode data: ") + operand);
            operands.push_back(it->second);
        }

        std::vector<uint8_t> auxiliaryArguments;
        if (entry.contains("aux_args"))
            auxiliaryArguments = entry["aux_args"].get<std::vector<uint8_t>>();

        const auto opcodeFlags = entry["flagsZNHC"].get<std::vector<std::string>>();
        if (opcodeFlags.size() != 4)
            throw std::runtime_error("Invalid number of flags in opcode data.");

        OPCode::Flags flags;
        for (int i = 0; i < 4; i++)
        {
            const auto it = FLAGS.find(opcodeFlags[i]);
            if (it == FLAGS.end())
                throw std::runtime_error(std::string("Unknown flag in opcode data: ") + opcodeFlags[i]);
            flags[i] = it->second;
        }

        storage_.push_back(std::make_unique<OPCode>(op, command, mnemonic, auxiliaryArguments, operands, bytes, cycles,
                                                    additionalCycles, jumpCondition, flags));
        map.at(op) = storage_.back().get();
    }
}

} // namespace gbemu::backend
