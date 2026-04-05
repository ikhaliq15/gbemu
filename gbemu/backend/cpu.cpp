#include "gbemu/backend/cpu.h"

#include "gbemu/backend/bitutils.h"

#include <iomanip>
#include <iostream>
#include <utility>

namespace gbemu::backend
{

CPU::CPU(RAM *ram)
    : IME_(false), PC_(STARTING_PC), SP_(STARTING_SP), AF_(STARTING_AF), BC_(STARTING_BC), DE_(STARTING_DE),
      HL_(STARTING_HL), ram_(ram), cycles_(0ul), mode_(Mode::NORMAL), opcodes_(OpcodeTable::instance().opcodes()),
      prefixedOpcodes_(OpcodeTable::instance().prefixedOpcodes())
{

    const std::unordered_map<std::string, OPCodeHandler> opcodeFunctions{
        {"NOP", [this](uint16_t pc, const OPCode *opcode) -> void { NOP(pc, opcode); }},
        {"LD", [this](uint16_t pc, const OPCode *opcode) -> void { LD(pc, opcode); }},
        {"INC", [this](uint16_t pc, const OPCode *opcode) -> void { INC(pc, opcode); }},
        {"DEC", [this](uint16_t pc, const OPCode *opcode) -> void { DEC(pc, opcode); }},
        {"RLCA", [this](uint16_t pc, const OPCode *opcode) -> void { RLCA(pc, opcode); }},
        {"LD_SP", [this](uint16_t pc, const OPCode *opcode) -> void { LD_SP(pc, opcode); }},
        {"ADD", [this](uint16_t pc, const OPCode *opcode) -> void { ADD(pc, opcode); }},
        {"RRCA", [this](uint16_t pc, const OPCode *opcode) -> void { RRCA(pc, opcode); }},
        {"RLA", [this](uint16_t pc, const OPCode *opcode) -> void { RLA(pc, opcode); }},
        {"JR", [this](uint16_t pc, const OPCode *opcode) -> void { JR(pc, opcode); }},
        {"RRA", [this](uint16_t pc, const OPCode *opcode) -> void { RRA(pc, opcode); }},
        {"LDI", [this](uint16_t pc, const OPCode *opcode) -> void { LDI(pc, opcode); }},
        {"DAA", [this](uint16_t pc, const OPCode *opcode) -> void { DAA(pc, opcode); }},
        {"CPL", [this](uint16_t pc, const OPCode *opcode) -> void { CPL(pc, opcode); }},
        {"LDD", [this](uint16_t pc, const OPCode *opcode) -> void { LDD(pc, opcode); }},
        {"SCF", [this](uint16_t pc, const OPCode *opcode) -> void { SCF(pc, opcode); }},
        {"CCF", [this](uint16_t pc, const OPCode *opcode) -> void { CCF(pc, opcode); }},
        {"HALT", [this](uint16_t pc, const OPCode *opcode) -> void { HALT(pc, opcode); }},
        {"ADC", [this](uint16_t pc, const OPCode *opcode) -> void { ADC(pc, opcode); }},
        {"SUB",
         [this](uint16_t pc, const OPCode *opcode) -> void {
             binary_alu_operation(pc, opcode, alu::sub<uint8_t, uint8_t>);
         }},
        {"SBC", [this](uint16_t pc, const OPCode *opcode) -> void { SBC(pc, opcode); }},
        {"AND", [this](uint16_t pc, const OPCode *opcode) -> void { binary_alu_operation(pc, opcode, alu::bit_and); }},
        {"XOR", [this](uint16_t pc, const OPCode *opcode) -> void { binary_alu_operation(pc, opcode, alu::bit_xor); }},
        {"OR", [this](uint16_t pc, const OPCode *opcode) -> void { binary_alu_operation(pc, opcode, alu::bit_or); }},
        {"CP", [this](uint16_t pc, const OPCode *opcode) -> void { CP(pc, opcode); }},
        {"RET", [this](uint16_t pc, const OPCode *opcode) -> void { RET(pc, opcode); }},
        {"POP", [this](uint16_t pc, const OPCode *opcode) -> void { POP(pc, opcode); }},
        {"JP", [this](uint16_t pc, const OPCode *opcode) -> void { JP(pc, opcode); }},
        {"CALL", [this](uint16_t pc, const OPCode *opcode) -> void { CALL(pc, opcode); }},
        {"PUSH", [this](uint16_t pc, const OPCode *opcode) -> void { PUSH(pc, opcode); }},
        {"RST", [this](uint16_t pc, const OPCode *opcode) -> void { RST(pc, opcode); }},
        {"RETI", [this](uint16_t pc, const OPCode *opcode) -> void { RETI(pc, opcode); }},
        {"LDff8", [this](uint16_t pc, const OPCode *opcode) -> void { LDff8(pc, opcode); }},
        {"LDa16", [this](uint16_t pc, const OPCode *opcode) -> void { LDa16(pc, opcode); }},
        {"LDaff8", [this](uint16_t pc, const OPCode *opcode) -> void { LDaff8(pc, opcode); }},
        {"DI", [this](uint16_t pc, const OPCode *opcode) -> void { DI(pc, opcode); }},
        {"EI", [this](uint16_t pc, const OPCode *opcode) -> void { EI(pc, opcode); }},
        {"LDs8", [this](uint16_t pc, const OPCode *opcode) -> void { LDs8(pc, opcode); }},
        {"LDaff16", [this](uint16_t pc, const OPCode *opcode) -> void { LDaff16(pc, opcode); }},
        {"RLC", [this](uint16_t pc, const OPCode *opcode) -> void { unary_alu_operation(pc, opcode, alu::rlc); }},
        {"RRC", [this](uint16_t pc, const OPCode *opcode) -> void { unary_alu_operation(pc, opcode, alu::rrc); }},
        {"RL", [this](uint16_t pc, const OPCode *opcode) -> void { RL(pc, opcode); }},
        {"RR", [this](uint16_t pc, const OPCode *opcode) -> void { RR(pc, opcode); }},
        {"SLA", [this](uint16_t pc, const OPCode *opcode) -> void { unary_alu_operation(pc, opcode, alu::bit_sla); }},
        {"SRA", [this](uint16_t pc, const OPCode *opcode) -> void { unary_alu_operation(pc, opcode, alu::bit_sra); }},
        {"SWAP", [this](uint16_t pc, const OPCode *opcode) -> void { unary_alu_operation(pc, opcode, alu::bit_swap); }},
        {"SRL", [this](uint16_t pc, const OPCode *opcode) -> void { unary_alu_operation(pc, opcode, alu::bit_srl); }},
        {"BIT_GET", [this](uint16_t pc, const OPCode *opcode) -> void { BIT_GET(pc, opcode); }},
        {"BIT_SET", [this](uint16_t pc, const OPCode *opcode) -> void { BIT_SET(pc, opcode); }},
    };

    for (size_t i = 0; i < opcodeFunctions_.size(); i++)
    {
        opcodeFunctions_[i] = [](uint16_t, const OPCode *) -> void {
            throw std::runtime_error("Unimplemented opcode handler called.");
        };

        if (opcodes_[i] == nullptr)
        {
            continue;
        }

        const auto opcodeIt = opcodeFunctions.find(opcodes_[i]->mnemonic());
        if (opcodeIt != opcodeFunctions.end())
            opcodeFunctions_[i] = opcodeIt->second;
    }

    for (size_t i = 0; i < prefixedOpcodeFunctions_.size(); i++)
    {
        prefixedOpcodeFunctions_[i] = [](uint16_t, const OPCode *) -> void {
            throw std::runtime_error("Unimplemented opcode handler called.");
        };

        if (prefixedOpcodes_[i] == nullptr)
        {
            continue;
        }

        const auto opcodeIt = opcodeFunctions.find(prefixedOpcodes_[i]->mnemonic());
        if (opcodeIt != opcodeFunctions.end())
            prefixedOpcodeFunctions_[i] = opcodeIt->second;
    }
}

CPU::CPU(const CPU &cpu)
    : IME_(cpu.IME_), PC_(cpu.PC_), SP_(cpu.SP_), AF_(cpu.AF_), BC_(cpu.BC_), DE_(cpu.DE_), HL_(cpu.HL_),
      ram_(cpu.ram_), cycles_(cpu.cycles_), mode_(cpu.mode_), opcodes_(OpcodeTable::instance().opcodes()),
      prefixedOpcodes_(OpcodeTable::instance().prefixedOpcodes())
{}

auto CPU::IME() const -> bool
{
    return IME_;
}

auto CPU::PC() const -> uint16_t
{
    return PC_;
}
auto CPU::SP() const -> uint16_t
{
    return SP_;
}

auto CPU::AF() const -> uint16_t
{
    return AF_;
}
auto CPU::BC() const -> uint16_t
{
    return BC_;
}
auto CPU::DE() const -> uint16_t
{
    return DE_;
}
auto CPU::HL() const -> uint16_t
{
    return HL_;
}

auto CPU::A() const -> uint8_t
{
    return upperByte(AF_);
}
auto CPU::B() const -> uint8_t
{
    return upperByte(BC_);
}
auto CPU::C() const -> uint8_t
{
    return lowerByte(BC_);
}
auto CPU::D() const -> uint8_t
{
    return upperByte(DE_);
}
auto CPU::E() const -> uint8_t
{
    return lowerByte(DE_);
}
auto CPU::H() const -> uint8_t
{
    return upperByte(HL_);
}
auto CPU::L() const -> uint8_t
{
    return lowerByte(HL_);
}

auto CPU::FlagZ() const -> uint8_t
{
    return getBit(AF_, FLAG_Z_BIT);
}
auto CPU::FlagN() const -> uint8_t
{
    return getBit(AF_, FLAG_N_BIT);
}
auto CPU::FlagH() const -> uint8_t
{
    return getBit(AF_, FLAG_H_BIT);
}
auto CPU::FlagC() const -> uint8_t
{
    return getBit(AF_, FLAG_C_BIT);
}

auto CPU::ram() const -> RAM *
{
    return ram_;
}

auto CPU::cycles() const -> uint64_t
{
    return cycles_;
}
auto CPU::mode() const -> CPU::Mode
{
    return mode_;
}
void CPU::setMode(Mode mode)
{
    mode_ = mode;
}

void CPU::setIME(bool newIME)
{
    IME_ = newIME;
}

void CPU::setPC(uint16_t newRegVal)
{
    PC_ = newRegVal;
}
void CPU::setSP(uint16_t newRegVal)
{
    SP_ = newRegVal;
}

void CPU::setAF(uint16_t newRegVal)
{
    AF_ = newRegVal;
}
void CPU::setBC(uint16_t newRegVal)
{
    BC_ = newRegVal;
}
void CPU::setDE(uint16_t newRegVal)
{
    DE_ = newRegVal;
}
void CPU::setHL(uint16_t newRegVal)
{
    HL_ = newRegVal;
}

void CPU::setA(uint8_t newRegVal)
{
    AF_ = setUpperByte(AF_, newRegVal);
}
void CPU::setB(uint8_t newRegVal)
{
    BC_ = setUpperByte(BC_, newRegVal);
}
void CPU::setC(uint8_t newRegVal)
{
    BC_ = setLowerByte(BC_, newRegVal);
}
void CPU::setD(uint8_t newRegVal)
{
    DE_ = setUpperByte(DE_, newRegVal);
}
void CPU::setE(uint8_t newRegVal)
{
    DE_ = setLowerByte(DE_, newRegVal);
}
void CPU::setH(uint8_t newRegVal)
{
    HL_ = setUpperByte(HL_, newRegVal);
}
void CPU::setL(uint8_t newRegVal)
{
    HL_ = setLowerByte(HL_, newRegVal);
}

void CPU::setFlagZ(uint8_t newFlagVal)
{
    AF_ = setBit(AF_, FLAG_Z_BIT, newFlagVal);
}
void CPU::setFlagN(uint8_t newFlagVal)
{
    AF_ = setBit(AF_, FLAG_N_BIT, newFlagVal);
}
void CPU::setFlagH(uint8_t newFlagVal)
{
    AF_ = setBit(AF_, FLAG_H_BIT, newFlagVal);
}
void CPU::setFlagC(uint8_t newFlagVal)
{
    AF_ = setBit(AF_, FLAG_C_BIT, newFlagVal);
}
void CPU::setFlags(uint8_t newZ, uint8_t newN, uint8_t newH, uint8_t newC)
{
    setFlagZ(newZ);
    setFlagN(newN);
    setFlagH(newH);
    setFlagC(newC);
}

void CPU::advancePC(uint16_t inc)
{
    PC_ += inc;
}
void CPU::offsetSP(int32_t offset)
{
    SP_ += offset;
}

auto CPU::getRegister(Register reg) const -> uint8_t
{
    switch (reg)
    {
    case Register::A:
        return A();
    case Register::B:
        return B();
    case Register::C:
        return C();
    case Register::D:
        return D();
    case Register::E:
        return E();
    case Register::H:
        return H();
    case Register::L:
        return L();
    }
}

auto CPU::getFullRegister(FullRegister reg) const -> uint16_t
{
    switch (reg)
    {
    case FullRegister::BC:
        return BC();
    case FullRegister::DE:
        return DE();
    case FullRegister::HL:
        return HL();
    case FullRegister::AF:
        return AF();
    }
}

void CPU::setRegister(Register reg, uint8_t newRegVal)
{
    switch (reg)
    {
    case Register::A:
        setA(newRegVal);
        return;
    case Register::B:
        setB(newRegVal);
        return;
    case Register::C:
        setC(newRegVal);
        return;
    case Register::D:
        setD(newRegVal);
        return;
    case Register::E:
        setE(newRegVal);
        return;
    case Register::H:
        setH(newRegVal);
        return;
    case Register::L:
        setL(newRegVal);
        return;
    }
}

void CPU::setFullRegister(FullRegister reg, uint16_t newRegVal)
{
    switch (reg)
    {
    case FullRegister::BC:
        setBC(newRegVal);
        return;
    case FullRegister::DE:
        setDE(newRegVal);
        return;
    case FullRegister::HL:
        setHL(newRegVal);
        return;
    case FullRegister::AF:
        setAF(newRegVal);
        return;
    }
}

void CPU::serviceInterrupts()
{
    const uint8_t pending = ram_->get(RAM::IF) & ram_->get(RAM::IE);

    // Any pending interrupt wakes the CPU from HALT
    if (pending & 0x1f)
        setMode(Mode::NORMAL);

    if (!IME_)
        return;

    // Service the highest-priority pending interrupt
    constexpr uint8_t INTERRUPT_BITS[] = {0, 1, 2}; // VBLANK, LCD_STAT, TIMER
    for (const auto bit : INTERRUPT_BITS)
    {
        if (getBit(pending, bit))
        {
            IME_ = false;
            pushToStack(PC_);
            ram_->set(RAM::IF, setBit(ram_->get(RAM::IF), bit, 0));
            PC_ = 0x40 + 0x08 * bit;
            return;
        }
    }
}

void CPU::executeInstruction(bool verbose)
{
    if (verbose)
        std::cout << *this << std::endl;

    const auto enableInterruptsAfterInstruction = interruptsEnabledQueued_;

    auto opcodeValue = ram_->get(PC());
    auto opcodeMap = &opcodes_;
    auto opCodeHandlerMap = &opcodeFunctions_;
    auto prefixedOpcode = false;

    if (opcodeValue == OPCode::PREFIX_OPCODE)
    {
        opcodeValue = ram_->get(PC() + 1);
        opcodeMap = &prefixedOpcodes_;
        opCodeHandlerMap = &prefixedOpcodeFunctions_;
        prefixedOpcode = true;
    }

    const auto opcodeString = [&prefixedOpcode, &opcodeValue]() -> std::string {
        return (prefixedOpcode) ? (toHexString(opcodeValue) + " (CB)") : toHexString(opcodeValue);
    };

    const auto opcode = opcodeMap->at(opcodeValue);
    if (!opcode)
        throw std::runtime_error(std::string("Unknown opcode detected: ") + opcodeString() + std::string(" at PC=") +
                                 toHexString(PC()));

    const auto oldPC = PC();
    advancePC(opcode->bytes());

    const auto &opcodeHandler = opCodeHandlerMap->at(opcodeValue);
    opcodeHandler(oldPC, opcode);

    cycles_ += opcode->cycles();

    if (enableInterruptsAfterInstruction)
    {
        IME_ = true;
        interruptsEnabledQueued_ = false;
    }

    // if (verbose)
    // {
    // std::cout << std::setw(20) << std::left << opcode->command() << std::setw(17) << ("Opcode=" + opcodeString())
    //           << "   " << std::setw(9) << ("PC=" + toHexString(PC())) << "   " << std::setw(9)
    //           << ("SP=" + toHexString(SP())) << "   " << std::setw(9) << ("AF=" + toHexString(AF())) << "   "
    //           << std::setw(9) << ("BC=" + toHexString(BC())) << "   " << std::setw(9) << ("DE=" + toHexString(DE()))
    //           << "   " << std::setw(9) << ("HL=" + toHexString(HL())) << "   " << std::setw(9)
    //           << ("LY=" + std::to_string(ram_->get(RAM::LY))) << "   " << std::setw(11)
    //           << ("LCDC=" + toHexString(ram_->get(RAM::LCDC))) << "   " << std::setw(9)
    //           << ("IF=" + toHexString(ram_->get(RAM::IF))) << "   " << std::setw(9)
    //           << ("IE=" + toHexString(ram_->get(RAM::IE))) << "   " << std::setw(9)
    //           << ("IME=" + (IME_ ? std::string("true") : std::string("false"))) << "   " << std::endl;
    // }
}

auto CPU::operator==(const CPU &rhs) const -> bool
{
    return PC_ == rhs.PC_ && SP_ == rhs.SP_ && AF_ == rhs.AF_ && BC_ == rhs.BC_ && DE_ == rhs.DE_ && HL_ == rhs.HL_ &&
           cycles_ == rhs.cycles_ && IME_ == rhs.IME_ && (*ram_ == *(rhs.ram_));
}

auto CPU::operator!=(const CPU &rhs) const -> bool
{
    return !(*this == rhs);
}

auto operator<<(std::ostream &os, const CPU &cpu) -> std::ostream &
{
    os << "A:" << toHexString(cpu.A(), false) << " "
       << "F:" << toHexString(lowerByte(cpu.AF()), false) << " "
       << "B:" << toHexString(cpu.B(), false) << " "
       << "C:" << toHexString(cpu.C(), false) << " "
       << "D:" << toHexString(cpu.D(), false) << " "
       << "E:" << toHexString(cpu.E(), false) << " "
       << "H:" << toHexString(cpu.H(), false) << " "
       << "L:" << toHexString(cpu.L(), false) << " "
       << "SP:" << toHexString(cpu.SP(), false) << " "
       << "PC:" << toHexString(cpu.PC(), false) << " "
       << "PCMEM:" << toHexString(cpu.ram()->get(cpu.PC()), false) << ","
       << toHexString(cpu.ram()->get(cpu.PC() + 1), false) << "," << toHexString(cpu.ram()->get(cpu.PC() + 2), false)
       << "," << toHexString(cpu.ram()->get(cpu.PC() + 3), false);

    return os;
}

void CPU::pushToStack(uint16_t val)
{
    offsetSP(-1);
    ram_->set(SP(), upperByte(val));

    offsetSP(-1);
    ram_->set(SP(), lowerByte(val));
}

auto CPU::popFromStack() -> uint16_t
{
    const auto lower = ram_->get(SP());
    offsetSP(1);

    const auto upper = ram_->get(SP());
    offsetSP(1);

    return concatBytes(upper, lower);
}

// TODO: cleaner way to handle than if-statement galore?
auto CPU::getOperand(Operand operand) const -> OperandValue
{
    if (std::holds_alternative<Register>(operand))
    {
        return getRegister(std::get<Register>(operand));
    }
    else if (std::holds_alternative<FullRegister>(operand))
    {
        return getFullRegister(std::get<FullRegister>(operand));
    }
    else if (std::holds_alternative<SpecialRegister>(operand))
    {
        const auto specialRegister = std::get<SpecialRegister>(operand);
        if (specialRegister == SpecialRegister::SP)
            return SP();
        else
            throw std::runtime_error("Unknown special register.");
    }
    else if (std::holds_alternative<DereferencedFullRegister>(operand))
    {
        const auto fullRegister = std::get<DereferencedFullRegister>(operand).fullRegister;
        const auto address = getFullRegister(fullRegister);
        return ram_->get(address);
    }
    else
    {
        throw std::runtime_error("Unknown operand type.");
    }
}

void CPU::setOperand(Operand operand, OperandValue newValue)
{
    if (std::holds_alternative<Register>(operand))
    {
        if (!std::holds_alternative<uint8_t>(newValue))
            throw std::runtime_error("Cannot set operand type with this operand value type.");
        setRegister(std::get<Register>(operand), std::get<uint8_t>(newValue));
        return;
    }
    else if (std::holds_alternative<FullRegister>(operand))
    {
        if (!std::holds_alternative<uint16_t>(newValue))
            throw std::runtime_error("Cannot set operand type with this operand value type.");
        setFullRegister(std::get<FullRegister>(operand), std::get<uint16_t>(newValue));
        return;
    }
    else if (std::holds_alternative<SpecialRegister>(operand))
    {
        if (!std::holds_alternative<uint16_t>(newValue))
            throw std::runtime_error("Cannot set operand type with this operand value type.");
        const auto specialRegister = std::get<SpecialRegister>(operand);
        if (specialRegister == SpecialRegister::SP)
        {
            setSP(std::get<uint16_t>(newValue));
            return;
        }
        else
            throw std::runtime_error("Unknown special register.");
    }
    else if (std::holds_alternative<DereferencedFullRegister>(operand))
    {
        if (!std::holds_alternative<uint8_t>(newValue))
            throw std::runtime_error("Cannot set operand type with this operand value type.");
        const auto fullRegister = std::get<DereferencedFullRegister>(operand).fullRegister;
        const auto address = getFullRegister(fullRegister);
        ram_->set(address, std::get<uint8_t>(newValue));
        return;
    }
    else
    {
        throw std::runtime_error("Unknown operand type.");
    }
}

void CPU::setFlagsFromResult(const alu::AluFlagResult &flagResult, const OPCode *opcode)
{
    const auto opcodeFlags = opcode->flags();

    std::array<uint8_t, 4> newFlags = {FlagZ(), FlagN(), FlagH(), FlagC()};

    for (int i = 0; i < 4; i++)
    {
        const auto opcodeFlag = opcodeFlags[i];
        switch (opcodeFlag)
        {
        case OPCode::Flag::UNTOUCHED:
            break;
        case OPCode::Flag::ONE:
            newFlags[i] = 1;
            break;
        case OPCode::Flag::ZERO:
            newFlags[i] = 0;
            break;
        case OPCode::Flag::Z:
            newFlags[i] = (flagResult.isZero) ? 1 : 0;
            break;
        case OPCode::Flag::H:
            newFlags[i] = (flagResult.hadHalfCarry) ? 1 : 0;
            break;
        case OPCode::Flag::CY:
            newFlags[i] = (flagResult.hadCarry) ? 1 : 0;
            break;
        case OPCode::Flag::NOT_CY:
            newFlags[i] = (FlagC() == 1) ? 0 : 1;
            break;
        case OPCode::Flag::A0:
            newFlags[i] = ((flagResult.bit0Set)) ? 1 : 0;
            break;
        case OPCode::Flag::A1:
            newFlags[i] = ((flagResult.bit1Set)) ? 1 : 0;
            break;
        case OPCode::Flag::A2:
            newFlags[i] = ((flagResult.bit2Set)) ? 1 : 0;
            break;
        case OPCode::Flag::A3:
            newFlags[i] = ((flagResult.bit3Set)) ? 1 : 0;
            break;
        case OPCode::Flag::A4:
            newFlags[i] = ((flagResult.bit4Set)) ? 1 : 0;
            break;
        case OPCode::Flag::A5:
            newFlags[i] = ((flagResult.bit5Set)) ? 1 : 0;
            break;
        case OPCode::Flag::A6:
            newFlags[i] = ((flagResult.bit6Set)) ? 1 : 0;
            break;
        case OPCode::Flag::A7:
            newFlags[i] = ((flagResult.bit7Set)) ? 1 : 0;
            break;
        case OPCode::Flag::NOT_A0:
            newFlags[i] = ((flagResult.bit0Set)) ? 0 : 1;
            break;
        case OPCode::Flag::NOT_A1:
            newFlags[i] = ((flagResult.bit1Set)) ? 0 : 1;
            break;
        case OPCode::Flag::NOT_A2:
            newFlags[i] = ((flagResult.bit2Set)) ? 0 : 1;
            break;
        case OPCode::Flag::NOT_A3:
            newFlags[i] = ((flagResult.bit3Set)) ? 0 : 1;
            break;
        case OPCode::Flag::NOT_A4:
            newFlags[i] = ((flagResult.bit4Set)) ? 0 : 1;
            break;
        case OPCode::Flag::NOT_A5:
            newFlags[i] = ((flagResult.bit5Set)) ? 0 : 1;
            break;
        case OPCode::Flag::NOT_A6:
            newFlags[i] = ((flagResult.bit6Set)) ? 0 : 1;
            break;
        case OPCode::Flag::NOT_A7:
            newFlags[i] = ((flagResult.bit7Set)) ? 0 : 1;
            break;
        }
    }

    setFlags(newFlags[0], newFlags[1], newFlags[2], newFlags[3]);
}

auto CPU::testJumpCondition(OPCode::JumpCondition jumpCondition) const -> bool
{
    if (jumpCondition == OPCode::JumpCondition::ALWAYS)
        return true;
    else if (jumpCondition == OPCode::JumpCondition::Z)
        return FlagZ() == 1;
    else if (jumpCondition == OPCode::JumpCondition::C)
        return FlagC() == 1;
    else if (jumpCondition == OPCode::JumpCondition::NZ)
        return FlagZ() == 0;
    else if (jumpCondition == OPCode::JumpCondition::NC)
        return FlagC() == 0;
    throw std::runtime_error("asked to test unknown jump condition.");
}

/*** OPCode Handlers ***/

template <typename F> void CPU::unary_alu_operation(uint16_t pc, const OPCode *opcode, F operation)
{
    const auto operand = opcode->operands()[0];

    if (std::holds_alternative<Register>(operand) || std::holds_alternative<DereferencedFullRegister>(operand))
    {
        const auto currentValue = std::get<uint8_t>(getOperand(operand));
        const auto result = operation(currentValue);

        setOperand(operand, result.result);
        setFlagsFromResult(result.flags, opcode);

        return;
    }

    throw std::runtime_error("sra not implemented for opcode " + toHexString(opcode->opcode()));
}

template <typename F> void CPU::binary_alu_operation(uint16_t pc, const OPCode *opcode, F operation)
{
    if (opcode->operands().size() == 1)
    {
        const auto destOperand = opcode->operands()[0];

        const auto firstOperand = getOperand(opcode->operands()[0]);

        const auto firstValue = std::get<uint8_t>(firstOperand);
        const auto secondValue = ram_->get(pc + 1);
        const auto result = operation(firstValue, secondValue);

        setOperand(destOperand, result.result);
        setFlagsFromResult(result.flags, opcode);

        return;
    }
    else if (opcode->operands().size() == 2)
    {
        const auto destOperand = opcode->operands()[0];
        const auto firstOperand = getOperand(opcode->operands()[0]);
        const auto secondOperand = getOperand(opcode->operands()[1]);

        if (std::holds_alternative<uint8_t>(firstOperand) && std::holds_alternative<uint8_t>(secondOperand))
        {
            const auto firstValue = std::get<uint8_t>(firstOperand);
            const auto secondValue = std::get<uint8_t>(secondOperand);

            const auto result = operation(firstValue, secondValue);

            setOperand(destOperand, result.result);
            setFlagsFromResult(result.flags, opcode);

            return;
        }
    }

    throw std::runtime_error("and not implemented for opcode " + toHexString(opcode->opcode()));
}

void CPU::NOP(uint16_t pc, const OPCode *opcode)
{
    return;
}

void CPU::LD(uint16_t pc, const OPCode *opcode)
{
    if (opcode->operands().size() == 1)
    {
        const auto destOperand = opcode->operands()[0];
        if (std::holds_alternative<Register>(destOperand) ||
            std::holds_alternative<DereferencedFullRegister>(destOperand))
        {
            const auto immediate = ram_->get(pc + 1);
            setOperand(destOperand, immediate);
            return;
        }
        else if (std::holds_alternative<FullRegister>(destOperand) ||
                 std::holds_alternative<SpecialRegister>(destOperand))
        {
            const auto immediate = ram_->getImmediate16(pc + 1);
            setOperand(destOperand, immediate);
            return;
        }
    }
    else if (opcode->operands().size() == 2)
    {
        const auto destOperand = opcode->operands()[0];
        const auto srcOperand = opcode->operands()[1];
        setOperand(destOperand, getOperand(srcOperand));
        return;
    }

    throw std::runtime_error("load not implemented for opcode " + toHexString(opcode->opcode()));
}

void CPU::INC(uint16_t pc, const OPCode *opcode)
{
    const auto operand = opcode->operands()[0];
    const auto operandValue = getOperand(operand);

    if (std::holds_alternative<uint8_t>(operandValue))
    {
        const auto currentValue = std::get<uint8_t>(operandValue);
        const auto aluResult = alu::add(currentValue, static_cast<uint8_t>(1));
        setOperand(operand, aluResult.result);
        setFlagsFromResult(aluResult.flags, opcode);
    }
    else if (std::holds_alternative<uint16_t>(operandValue))
    {
        const auto currentValue = std::get<uint16_t>(operandValue);
        const auto aluResult = alu::add(currentValue, static_cast<uint16_t>(1));
        setOperand(operand, aluResult.result);
        setFlagsFromResult(aluResult.flags, opcode);
    }
    else
        throw std::runtime_error("inc not implemented for opcode " + toHexString(opcode->opcode()));
}

void CPU::DEC(uint16_t pc, const OPCode *opcode)
{
    const auto operand = opcode->operands()[0];
    const auto operandValue = getOperand(operand);

    if (std::holds_alternative<uint8_t>(operandValue))
    {
        const auto currentValue = std::get<uint8_t>(operandValue);
        const auto aluResult = alu::sub(currentValue, static_cast<uint8_t>(1));
        setOperand(operand, aluResult.result);
        setFlagsFromResult(aluResult.flags, opcode);
    }
    else if (std::holds_alternative<uint16_t>(operandValue))
    {
        const auto currentValue = std::get<uint16_t>(operandValue);
        const auto aluResult = alu::sub(currentValue, static_cast<uint16_t>(1));
        setOperand(operand, aluResult.result);
        setFlagsFromResult(aluResult.flags, opcode);
    }
    else
        throw std::runtime_error("dec not implemented for opcode " + toHexString(opcode->opcode()));
}

void CPU::RLCA(uint16_t pc, const OPCode *opcode)
{
    const auto currentValue = A();
    const auto result = alu::rlc(currentValue);

    setA(result.result);
    setFlagsFromResult(result.flags, opcode);
}

void CPU::LD_SP(uint16_t pc, const OPCode *opcode)
{
    const auto immediate = ram_->getImmediate16(pc + 1);
    const auto sp = SP();

    ram_->setImmediate16(immediate, sp);
}

void CPU::ADD(uint16_t pc, const OPCode *opcode)
{
    if (opcode->operands().size() == 1)
    {
        const auto destOperand = opcode->operands()[0];
        const auto firstOperand = getOperand(opcode->operands()[0]);

        if (std::holds_alternative<uint8_t>(firstOperand))
        {
            const auto firstValue = std::get<uint8_t>(firstOperand);
            const auto secondValue = ram_->get(pc + 1);

            const auto result = alu::add(firstValue, secondValue);

            setOperand(destOperand, result.result);
            setFlagsFromResult(result.flags, opcode);

            return;
        }
        else if (std::holds_alternative<uint16_t>(firstOperand))
        {
            const auto firstValue = std::get<uint16_t>(firstOperand);
            const auto secondValue = static_cast<int8_t>(ram_->get(pc + 1));

            // TODO: check how half-carry and carry are working for mixed signed and unsigned addition.
            const auto result = alu::add(static_cast<uint8_t>(0x00FF & firstValue), static_cast<uint8_t>(secondValue));

            setOperand(destOperand, static_cast<uint16_t>(firstValue + secondValue));
            setFlagsFromResult(result.flags, opcode);

            return;
        }
    }
    else if (opcode->operands().size() == 2)
    {
        const auto destOperand = opcode->operands()[0];
        const auto firstOperand = getOperand(opcode->operands()[0]);
        const auto secondOperand = getOperand(opcode->operands()[1]);

        if (std::holds_alternative<uint8_t>(firstOperand) && std::holds_alternative<uint8_t>(secondOperand))
        {
            const auto firstValue = std::get<uint8_t>(firstOperand);
            const auto secondValue = std::get<uint8_t>(secondOperand);

            const auto result = alu::add(firstValue, secondValue);

            setOperand(destOperand, result.result);
            setFlagsFromResult(result.flags, opcode);

            return;
        }
        else if (std::holds_alternative<uint16_t>(firstOperand) && std::holds_alternative<uint16_t>(secondOperand))
        {
            const auto firstValue = std::get<uint16_t>(firstOperand);
            const auto secondValue = std::get<uint16_t>(secondOperand);

            const auto result = alu::add(firstValue, secondValue);

            setOperand(destOperand, result.result);
            setFlagsFromResult(result.flags, opcode);

            return;
        }
    }

    throw std::runtime_error("add not implemented for opcode " + toHexString(opcode->opcode()));
}

void CPU::RRCA(uint16_t pc, const OPCode *opcode)
{
    const auto currentValue = A();
    const auto result = alu::rrc(currentValue);

    setA(result.result);
    setFlagsFromResult(result.flags, opcode);
}

void CPU::RLA(uint16_t pc, const OPCode *opcode)
{
    const auto currentValue = A();
    const auto result = alu::rl(currentValue, FlagC());

    setA(result.result);
    setFlagsFromResult(result.flags, opcode);
}

void CPU::JR(uint16_t pc, const OPCode *opcode)
{
    if (testJumpCondition(opcode->jumpCondition()))
    {
        const auto immediate = static_cast<int8_t>(ram_->get(pc + 1));
        setPC(PC() + immediate);
        cycles_ += opcode->additionalCycles();
    }
}

void CPU::RRA(uint16_t pc, const OPCode *opcode)
{
    const auto currentValue = A();
    const auto result = alu::rr(currentValue, FlagC());

    setA(result.result);
    setFlagsFromResult(result.flags, opcode);
}

void CPU::LDI(uint16_t pc, const OPCode *opcode)
{
    if (opcode->operands().size() == 2)
    {
        const auto destOperand = opcode->operands()[0];
        const auto sourceOperand = opcode->operands()[1];
        if (std::holds_alternative<DereferencedFullRegister>(destOperand))
        {
            const auto dereferencedFullRegister = std::get<DereferencedFullRegister>(destOperand);
            setOperand(destOperand, getOperand(sourceOperand));

            const auto currentFullRegisterValue = std::get<uint16_t>(getOperand(dereferencedFullRegister.fullRegister));
            setOperand(dereferencedFullRegister.fullRegister, static_cast<uint16_t>(currentFullRegisterValue + 1));
            return;
        }
        else if (std::holds_alternative<DereferencedFullRegister>(sourceOperand))
        {
            const auto dereferencedFullRegister = std::get<DereferencedFullRegister>(sourceOperand);
            setOperand(destOperand, getOperand(sourceOperand));

            const auto currentFullRegisterValue = std::get<uint16_t>(getOperand(dereferencedFullRegister.fullRegister));
            setOperand(dereferencedFullRegister.fullRegister, static_cast<uint16_t>(currentFullRegisterValue + 1));
            return;
        }
    }

    throw std::runtime_error("load+increment not implemented for opcode " + toHexString(opcode->opcode()));
}

void CPU::DAA(uint16_t pc, const OPCode *opcode)
{
    // TODO: move adjustment logic to ALU and respect Flag values from opcode data.

    auto value = A();

    auto correction = 0;

    uint8_t newFlagC = 0;

    if (FlagH() == 1 || (FlagN() != 1 && (value & 0x0f) > 0x09))
        correction |= 0x06;

    if (FlagC() == 1 || (FlagN() != 1 && value > 0x99))
    {
        correction |= 0x60;
        newFlagC = 1;
    }

    if (FlagN() == 1)
        value -= correction;
    else
        value += correction;

    const uint8_t newFlagZ = (value == 0) ? 1 : 0;

    setFlags(newFlagZ, FlagN(), 0, newFlagC);
    setA(value);

    // throw std::runtime_error("DAA not implemented for opcode " + toHexString(opcode.opcode()));
}

void CPU::CPL(uint16_t pc, const OPCode *opcode)
{
    const auto accumulateValue = A();
    const auto result = alu::bit_cpl(accumulateValue);
    setA(result.result);
    setFlagsFromResult(result.flags, opcode);
}

void CPU::LDD(uint16_t pc, const OPCode *opcode)
{
    if (opcode->operands().size() == 2)
    {
        const auto destOperand = opcode->operands()[0];
        const auto sourceOperand = opcode->operands()[1];
        if (std::holds_alternative<DereferencedFullRegister>(destOperand))
        {
            const auto dereferencedFullRegister = std::get<DereferencedFullRegister>(destOperand);
            setOperand(destOperand, getOperand(sourceOperand));

            const auto currentFullRegisterValue = std::get<uint16_t>(getOperand(dereferencedFullRegister.fullRegister));
            setOperand(dereferencedFullRegister.fullRegister, static_cast<uint16_t>(currentFullRegisterValue - 1));
            return;
        }
        else if (std::holds_alternative<DereferencedFullRegister>(sourceOperand))
        {
            const auto dereferencedFullRegister = std::get<DereferencedFullRegister>(sourceOperand);
            setOperand(destOperand, getOperand(sourceOperand));

            const auto currentFullRegisterValue = std::get<uint16_t>(getOperand(dereferencedFullRegister.fullRegister));
            setOperand(dereferencedFullRegister.fullRegister, static_cast<uint16_t>(currentFullRegisterValue - 1));
            return;
        }
    }

    throw std::runtime_error("load+decrement not implemented for opcode " + toHexString(opcode->opcode()));
}

void CPU::SCF(uint16_t pc, const OPCode *opcode)
{
    setFlagsFromResult(alu::AluFlagResult{}, opcode);
}

void CPU::CCF(uint16_t pc, const OPCode *opcode)
{
    setFlagsFromResult(alu::AluFlagResult{}, opcode);
}

void CPU::HALT(uint16_t pc, const OPCode *opcode)
{
    mode_ = Mode::HALT;
}

void CPU::ADC(uint16_t pc, const OPCode *opcode)
{
    if (opcode->operands().size() == 1)
    {
        const auto destOperand = opcode->operands()[0];
        const auto firstOperand = getOperand(opcode->operands()[0]);

        const auto firstValue = std::get<uint8_t>(firstOperand);
        const auto secondValue = ram_->get(pc + 1);

        const auto result = alu::adc(firstValue, secondValue, FlagC());

        setOperand(destOperand, result.result);
        setFlagsFromResult(result.flags, opcode);

        return;
    }
    else if (opcode->operands().size() == 2)
    {
        const auto destOperand = opcode->operands()[0];
        const auto firstOperand = getOperand(opcode->operands()[0]);
        const auto secondOperand = getOperand(opcode->operands()[1]);

        if (std::holds_alternative<uint8_t>(firstOperand) && std::holds_alternative<uint8_t>(secondOperand))
        {
            const auto firstValue = std::get<uint8_t>(firstOperand);
            const auto secondValue = std::get<uint8_t>(secondOperand);

            const auto result = alu::adc(firstValue, secondValue, FlagC());

            setOperand(destOperand, result.result);
            setFlagsFromResult(result.flags, opcode);

            return;
        }
    }

    throw std::runtime_error("adc not implemented for opcode " + toHexString(opcode->opcode()));
}

void CPU::SBC(uint16_t pc, const OPCode *opcode)
{
    if (opcode->operands().size() == 1)
    {
        const auto destOperand = opcode->operands()[0];
        const auto firstOperand = getOperand(opcode->operands()[0]);

        const auto firstValue = std::get<uint8_t>(firstOperand);
        const auto secondValue = ram_->get(pc + 1);

        const auto result = alu::sbc(firstValue, secondValue, FlagC());

        setOperand(destOperand, result.result);
        setFlagsFromResult(result.flags, opcode);

        return;
    }
    else if (opcode->operands().size() == 2)
    {
        const auto destOperand = opcode->operands()[0];
        const auto firstOperand = getOperand(opcode->operands()[0]);
        const auto secondOperand = getOperand(opcode->operands()[1]);

        if (std::holds_alternative<uint8_t>(firstOperand) && std::holds_alternative<uint8_t>(secondOperand))
        {
            const auto firstValue = std::get<uint8_t>(firstOperand);
            const auto secondValue = std::get<uint8_t>(secondOperand);

            const auto result = alu::sbc(firstValue, secondValue, FlagC());

            setOperand(destOperand, result.result);
            setFlagsFromResult(result.flags, opcode);

            return;
        }
    }

    throw std::runtime_error("sbc not implemented for opcode " + toHexString(opcode->opcode()));
}

void CPU::CP(uint16_t pc, const OPCode *opcode)
{
    if (opcode->operands().size() == 1)
    {
        const auto firstOperand = getOperand(opcode->operands()[0]);

        const auto firstValue = std::get<uint8_t>(firstOperand);
        const auto secondValue = ram_->get(pc + 1);

        const auto result = alu::sub(firstValue, secondValue);

        setFlagsFromResult(result.flags, opcode);

        return;
    }
    else if (opcode->operands().size() == 2)
    {
        const auto firstOperand = getOperand(opcode->operands()[0]);
        const auto secondOperand = getOperand(opcode->operands()[1]);

        if (std::holds_alternative<uint8_t>(firstOperand) && std::holds_alternative<uint8_t>(secondOperand))
        {
            const auto firstValue = std::get<uint8_t>(firstOperand);
            const auto secondValue = std::get<uint8_t>(secondOperand);

            const auto result = alu::sub(firstValue, secondValue);

            setFlagsFromResult(result.flags, opcode);

            return;
        }
    }

    throw std::runtime_error("cp not implemented for opcode " + toHexString(opcode->opcode()));
}

void CPU::RET(uint16_t pc, const OPCode *opcode)
{
    if (testJumpCondition(opcode->jumpCondition()))
    {
        const auto immediate = popFromStack();
        setPC(immediate);
        cycles_ += opcode->additionalCycles();
    }
}

void CPU::POP(uint16_t pc, const OPCode *opcode)
{
    const auto destOperand = opcode->operands()[0];

    if (std::holds_alternative<FullRegister>(destOperand))
    {
        auto stackValue = popFromStack();

        if (std::get<FullRegister>(destOperand) == FullRegister::AF)
            stackValue &= 0xFFF0;

        setOperand(destOperand, stackValue);
        return;
    }

    throw std::runtime_error("pop not implemented for opcode " + toHexString(opcode->opcode()));
}

void CPU::JP(uint16_t pc, const OPCode *opcode)
{
    if (opcode->operands().size() == 0)
    {
        if (testJumpCondition(opcode->jumpCondition()))
        {
            const auto immediate = ram_->getImmediate16(pc + 1);
            setPC(immediate);
            cycles_ += opcode->additionalCycles();
        }
        return;
    }
    else if (opcode->operands().size() == 1)
    {
        if (testJumpCondition(opcode->jumpCondition()))
        {
            const auto addressOperand = opcode->operands()[0];
            const auto newAddress = std::get<uint16_t>(getOperand(addressOperand));
            setPC(newAddress);
            cycles_ += opcode->additionalCycles();
        }
        return;
    }

    throw std::runtime_error("jump not implemented for opcode " + toHexString(opcode->opcode()));
}

void CPU::CALL(uint16_t pc, const OPCode *opcode)
{
    if (testJumpCondition(opcode->jumpCondition()))
    {
        const auto immediate = ram_->getImmediate16(pc + 1);
        pushToStack(PC());
        setPC(immediate);
        cycles_ += opcode->additionalCycles();
    }
}

void CPU::PUSH(uint16_t pc, const OPCode *opcode)
{
    const auto operand = opcode->operands()[0];
    const auto operandValue = getOperand(operand);

    if (std::holds_alternative<uint16_t>(operandValue))
    {
        const auto stackValue = std::get<uint16_t>(operandValue);
        pushToStack(stackValue);
        return;
    }

    throw std::runtime_error("push not implemented for opcode " + toHexString(opcode->opcode()));
}

void CPU::RST(uint16_t pc, const OPCode *opcode)
{
    const auto auxArg = opcode->auxiliaryArguments()[0];
    if (auxArg > 7)
        throw std::runtime_error("rst not implemented for opcode " + toHexString(opcode->opcode()));

    pushToStack(PC());
    const uint16_t rstAddress = concatBytes(0x00, auxArg << 3);
    setPC(rstAddress);
}

void CPU::RETI(uint16_t pc, const OPCode *opcode)
{
    if (testJumpCondition(opcode->jumpCondition()))
    {
        const auto immediate = popFromStack();
        setPC(immediate);
        IME_ = true;
        cycles_ += opcode->additionalCycles();
    }
}

void CPU::LDff8(uint16_t pc, const OPCode *opcode)
{
    if (opcode->operands().size() == 1)
    {
        const auto srcOperand = opcode->operands()[0];
        if (std::holds_alternative<Register>(srcOperand))
        {
            const auto lowerByte = ram_->get(pc + 1);
            const auto address = concatBytes(0xff, lowerByte);
            const auto value = std::get<uint8_t>(getOperand(srcOperand));
            ram_->set(address, value);
            return;
        }
    }
    else if (opcode->operands().size() == 2)
    {
        const auto addressOperand = opcode->operands()[0];
        const auto srcOperand = opcode->operands()[1];
        if (std::holds_alternative<Register>(addressOperand) && std::holds_alternative<Register>(srcOperand))
        {
            const auto lowerByte = std::get<uint8_t>(getOperand(addressOperand));
            const auto address = concatBytes(0xff, lowerByte);
            const auto value = std::get<uint8_t>(getOperand(srcOperand));
            ram_->set(address, value);
            return;
        }
    }

    throw std::runtime_error("ldff8 not implemented for opcode " + toHexString(opcode->opcode()));
}

void CPU::LDa16(uint16_t pc, const OPCode *opcode)
{
    if (opcode->operands().size() == 1)
    {
        const auto srcOperand = opcode->operands()[0];
        if (std::holds_alternative<Register>(srcOperand))
        {
            const auto address = ram_->getImmediate16(pc + 1);
            const auto value = std::get<uint8_t>(getOperand(srcOperand));
            ram_->set(address, value);
            return;
        }
    }

    throw std::runtime_error("lda16 not implemented for opcode " + toHexString(opcode->opcode()));
}

void CPU::LDaff8(uint16_t pc, const OPCode *opcode)
{
    if (opcode->operands().size() == 1)
    {
        const auto destOperand = opcode->operands()[0];
        if (std::holds_alternative<Register>(destOperand))
        {
            const auto lowerByte = ram_->get(pc + 1);
            const auto address = concatBytes(0xff, lowerByte);
            const auto value = ram_->get(address);
            setOperand(destOperand, value);
            return;
        }
    }
    else if (opcode->operands().size() == 2)
    {
        const auto destOperand = opcode->operands()[0];
        const auto addressOperand = opcode->operands()[1];
        if (std::holds_alternative<Register>(destOperand) && std::holds_alternative<Register>(addressOperand))
        {
            const auto lowerByte = std::get<uint8_t>(getOperand(addressOperand));
            const auto address = concatBytes(0xff, lowerByte);
            const auto value = ram_->get(address);
            setOperand(destOperand, value);
            return;
        }
    }

    throw std::runtime_error("ldaff8 not implemented for opcode " + toHexString(opcode->opcode()));
}

void CPU::DI(uint16_t pc, const OPCode *opcode)
{
    IME_ = false;
}

void CPU::LDaff16(uint16_t pc, const OPCode *opcode)
{
    if (opcode->operands().size() == 1)
    {
        const auto destOperand = opcode->operands()[0];
        if (std::holds_alternative<Register>(destOperand))
        {
            const auto address = ram_->getImmediate16(pc + 1);
            const auto value = ram_->get(address);
            setOperand(destOperand, value);
            return;
        }
    }

    throw std::runtime_error("ldaff16 not implemented for opcode " + toHexString(opcode->opcode()));
}

void CPU::LDs8(uint16_t pc, const OPCode *opcode)
{
    if (opcode->operands().size() == 2)
    {
        const auto destOperand = opcode->operands()[0];
        const auto srcOperand = opcode->operands()[1];

        if ((std::holds_alternative<FullRegister>(destOperand) ||
             std::holds_alternative<SpecialRegister>(destOperand)) &&
            (std::holds_alternative<FullRegister>(srcOperand) || std::holds_alternative<SpecialRegister>(srcOperand)))
        {
            const auto srcValue = std::get<uint16_t>(getOperand(srcOperand));
            const auto offset = static_cast<int8_t>(ram_->get(pc + 1));

            const auto result = alu::add(static_cast<uint8_t>(0x00FF & srcValue), static_cast<uint8_t>(offset));

            setOperand(destOperand, static_cast<uint16_t>(srcValue + offset));
            setFlagsFromResult(result.flags, opcode);

            return;
        }
    }

    throw std::runtime_error("LDs8 not implemented for opcode " + toHexString(opcode->opcode()));
}

void CPU::EI(uint16_t pc, const OPCode *opcode)
{
    interruptsEnabledQueued_ = true;
}

void CPU::RL(uint16_t pc, const OPCode *opcode)
{
    const auto operand = opcode->operands()[0];

    if (std::holds_alternative<Register>(operand) || std::holds_alternative<DereferencedFullRegister>(operand))
    {
        const auto currentValue = std::get<uint8_t>(getOperand(operand));
        const auto result = alu::rl(currentValue, FlagC());

        setOperand(operand, result.result);
        setFlagsFromResult(result.flags, opcode);

        return;
    }

    throw std::runtime_error("rl not implemented for opcode " + toHexString(opcode->opcode()));
}

void CPU::RR(uint16_t pc, const OPCode *opcode)
{
    const auto operand = opcode->operands()[0];

    if (std::holds_alternative<Register>(operand) || std::holds_alternative<DereferencedFullRegister>(operand))
    {
        const auto currentValue = std::get<uint8_t>(getOperand(operand));
        const auto result = alu::rr(currentValue, FlagC());

        setOperand(operand, result.result);
        setFlagsFromResult(result.flags, opcode);

        return;
    }

    throw std::runtime_error("sla not implemented for opcode " + toHexString(opcode->opcode()));
}

void CPU::BIT_GET(uint16_t pc, const OPCode *opcode)
{
    const auto operand = opcode->operands()[0];
    const auto bitIndex = opcode->auxiliaryArguments()[0];

    if (std::holds_alternative<Register>(operand) || std::holds_alternative<DereferencedFullRegister>(operand))
    {
        const auto currentValue = std::get<uint8_t>(getOperand(operand));
        const auto result = alu::bit_get(currentValue, bitIndex);

        setFlagsFromResult(result.flags, opcode);

        return;
    }

    throw std::runtime_error("bit_set not implemented for opcode " + toHexString(opcode->opcode()));
}

void CPU::BIT_SET(uint16_t pc, const OPCode *opcode)
{
    const auto operand = opcode->operands()[0];
    const auto bitIndex = opcode->auxiliaryArguments()[0];
    const auto newBit = opcode->auxiliaryArguments()[1];

    if (std::holds_alternative<Register>(operand) || std::holds_alternative<DereferencedFullRegister>(operand))
    {
        const auto currentValue = std::get<uint8_t>(getOperand(operand));
        const auto result = alu::bit_set(currentValue, bitIndex, newBit);

        setOperand(operand, result.result);
        setFlagsFromResult(result.flags, opcode);

        return;
    }

    throw std::runtime_error("bit_set not implemented for opcode " + toHexString(opcode->opcode()));
}

} // namespace gbemu::backend
