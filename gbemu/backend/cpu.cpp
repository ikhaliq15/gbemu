#include "gbemu/backend/cpu.h"

#include "gbemu/backend/bitutils.h"

#include <iostream>

namespace gbemu::backend
{

CPU::CPU(RAM *ram)
    : IME_(false), PC_(STARTING_PC), SP_(STARTING_SP), AF_(STARTING_AF), BC_(STARTING_BC), DE_(STARTING_DE),
      HL_(STARTING_HL), ram_(ram), cycles_(0ul), mode_(Mode::NORMAL)
{}

CPU::CPU(const CPU &cpu)
    : IME_(cpu.IME_), PC_(cpu.PC_), SP_(cpu.SP_), AF_(cpu.AF_), BC_(cpu.BC_), DE_(cpu.DE_), HL_(cpu.HL_),
      ram_(cpu.ram_), cycles_(cpu.cycles_), mode_(cpu.mode_)
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
    constexpr std::array<uint8_t, 3> INTERRUPT_BITS = {0, 1, 2}; // VBLANK, LCD_STAT, TIMER
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
    {
        std::cout << *this << std::endl;
    }

    const auto enableInterruptsAfterInstruction = interruptsEnabledQueued_;

    const auto firstByte = ram_->get(PC());
    const auto prefixed = firstByte == OPCode::PREFIX_OPCODE;
    const auto opcodeValue = prefixed ? ram_->get(PC() + 1) : firstByte;
    const auto &opcodeTable = prefixed ? PREFIXED_OPCODES : OPCODES;
    const auto &handlerTable = prefixed ? prefixedOpcodeFunctions_ : opcodeFunctions_;

    const auto &opcode = opcodeTable[opcodeValue];
    if (!opcode.valid)
    {
        const auto opcodeString = prefixed ? (toHexString(opcodeValue) + " (CB)") : toHexString(opcodeValue);
        throw std::runtime_error("Unknown opcode detected: " + opcodeString + " at PC=" + toHexString(PC()));
    }

    const auto oldPC = PC();
    const auto &opcodeHandler = handlerTable[opcodeValue];

    advancePC(opcode.bytes);
    (this->*opcodeHandler)(oldPC, &opcode);
    cycles_ += opcode.cycles;

    if (enableInterruptsAfterInstruction)
    {
        IME_ = true;
        interruptsEnabledQueued_ = false;
    }
}

auto CPU::operator==(const CPU &rhs) const -> bool
{
    return PC_ == rhs.PC_ && SP_ == rhs.SP_ && AF_ == rhs.AF_ && BC_ == rhs.BC_ && DE_ == rhs.DE_ && HL_ == rhs.HL_ &&
           cycles_ == rhs.cycles_ && IME_ == rhs.IME_ && (*ram_ == *(rhs.ram_));
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

auto CPU::getOperand(Operand operand) const -> OperandValue
{
    switch (operand.kind)
    {
    case Operand::Kind::REG:
        return getRegister(operand.asRegister());
    case Operand::Kind::FULL_REG:
        return getFullRegister(operand.asFullRegister());
    case Operand::Kind::SPECIAL_REG:
        return SP();
    case Operand::Kind::DEREF_FULL_REG:
        return ram_->get(getFullRegister(operand.asDereferencedFullRegister()));
    case Operand::Kind::NONE:
        throw std::runtime_error("Cannot get value of NONE operand.");
    }
}

void CPU::setOperand(Operand operand, OperandValue newValue)
{
    switch (operand.kind)
    {
    case Operand::Kind::REG:
        setRegister(operand.asRegister(), newValue.as8());
        return;
    case Operand::Kind::FULL_REG:
        setFullRegister(operand.asFullRegister(), newValue.as16());
        return;
    case Operand::Kind::SPECIAL_REG:
        setSP(newValue.as16());
        return;
    case Operand::Kind::DEREF_FULL_REG:
        ram_->set(getFullRegister(operand.asDereferencedFullRegister()), newValue.as8());
        return;
    case Operand::Kind::NONE:
        throw std::runtime_error("Cannot set value of NONE operand.");
    }
}

void CPU::setFlagsFromResult(const alu::AluFlagResult &flagResult, const OPCode *opcode)
{
    const auto opcodeFlags = opcode->flags;

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
            newFlags[i] = (flagResult.bit0Set) ? 1 : 0;
            break;
        case OPCode::Flag::A1:
            newFlags[i] = (flagResult.bit1Set) ? 1 : 0;
            break;
        case OPCode::Flag::A2:
            newFlags[i] = (flagResult.bit2Set) ? 1 : 0;
            break;
        case OPCode::Flag::A3:
            newFlags[i] = (flagResult.bit3Set) ? 1 : 0;
            break;
        case OPCode::Flag::A4:
            newFlags[i] = (flagResult.bit4Set) ? 1 : 0;
            break;
        case OPCode::Flag::A5:
            newFlags[i] = (flagResult.bit5Set) ? 1 : 0;
            break;
        case OPCode::Flag::A6:
            newFlags[i] = (flagResult.bit6Set) ? 1 : 0;
            break;
        case OPCode::Flag::A7:
            newFlags[i] = (flagResult.bit7Set) ? 1 : 0;
            break;
        case OPCode::Flag::NOT_A0:
            newFlags[i] = (flagResult.bit0Set) ? 0 : 1;
            break;
        case OPCode::Flag::NOT_A1:
            newFlags[i] = (flagResult.bit1Set) ? 0 : 1;
            break;
        case OPCode::Flag::NOT_A2:
            newFlags[i] = (flagResult.bit2Set) ? 0 : 1;
            break;
        case OPCode::Flag::NOT_A3:
            newFlags[i] = (flagResult.bit3Set) ? 0 : 1;
            break;
        case OPCode::Flag::NOT_A4:
            newFlags[i] = (flagResult.bit4Set) ? 0 : 1;
            break;
        case OPCode::Flag::NOT_A5:
            newFlags[i] = (flagResult.bit5Set) ? 0 : 1;
            break;
        case OPCode::Flag::NOT_A6:
            newFlags[i] = (flagResult.bit6Set) ? 0 : 1;
            break;
        case OPCode::Flag::NOT_A7:
            newFlags[i] = (flagResult.bit7Set) ? 0 : 1;
            break;
        }
    }

    setFlags(newFlags[0], newFlags[1], newFlags[2], newFlags[3]);
}

auto CPU::testJumpCondition(OPCode::JumpCondition jumpCondition) const -> bool
{
    switch (jumpCondition)
    {
    case OPCode::JumpCondition::ALWAYS:
        return true;
    case OPCode::JumpCondition::Z:
        return FlagZ() == 1;
    case OPCode::JumpCondition::C:
        return FlagC() == 1;
    case OPCode::JumpCondition::NZ:
        return FlagZ() == 0;
    case OPCode::JumpCondition::NC:
        return FlagC() == 0;
    }
}

/*** OPCode Handlers ***/

template <auto Operation> void CPU::unary_alu_operation(uint16_t pc, const OPCode *opcode)
{
    const auto operand = opcode->operands[0];

    const auto currentValue = getOperand(operand).as8();
    const auto result = Operation(currentValue);

    setOperand(operand, result.result);
    setFlagsFromResult(result.flags, opcode);
}

template <auto Operation> void CPU::binary_alu_operation(uint16_t pc, const OPCode *opcode)
{
    const auto destOperand = opcode->operands[0];

    const auto firstValue = getOperand(destOperand).as8();
    const auto secondValue = opcode->numOperands == 2 ? getOperand(opcode->operands[1]).as8() : ram_->get(pc + 1);
    const auto result = Operation(firstValue, secondValue);

    setOperand(destOperand, result.result);
    setFlagsFromResult(result.flags, opcode);
}

void CPU::UNIMPLEMENTED(uint16_t pc, const OPCode *opcode)
{
    throw std::runtime_error("Unimplemented opcode handler called for opcode " + toHexString(opcode->opcode));
}

void CPU::SUB(uint16_t pc, const OPCode *opcode)
{
    binary_alu_operation<&alu::sub<uint8_t, uint8_t>>(pc, opcode);
}

void CPU::AND(uint16_t pc, const OPCode *opcode)
{
    binary_alu_operation<&alu::bit_and>(pc, opcode);
}

void CPU::XOR(uint16_t pc, const OPCode *opcode)
{
    binary_alu_operation<&alu::bit_xor>(pc, opcode);
}

void CPU::OR(uint16_t pc, const OPCode *opcode)
{
    binary_alu_operation<&alu::bit_or>(pc, opcode);
}

void CPU::RLC(uint16_t pc, const OPCode *opcode)
{
    unary_alu_operation<&alu::rlc>(pc, opcode);
}

void CPU::RRC(uint16_t pc, const OPCode *opcode)
{
    unary_alu_operation<&alu::rrc>(pc, opcode);
}

void CPU::SLA(uint16_t pc, const OPCode *opcode)
{
    unary_alu_operation<&alu::bit_sla>(pc, opcode);
}

void CPU::SRA(uint16_t pc, const OPCode *opcode)
{
    unary_alu_operation<&alu::bit_sra>(pc, opcode);
}

void CPU::SWAP(uint16_t pc, const OPCode *opcode)
{
    unary_alu_operation<&alu::bit_swap>(pc, opcode);
}

void CPU::SRL(uint16_t pc, const OPCode *opcode)
{
    unary_alu_operation<&alu::bit_srl>(pc, opcode);
}

void CPU::NOP(uint16_t pc, const OPCode *opcode)
{}

void CPU::LD(uint16_t pc, const OPCode *opcode)
{
    if (opcode->numOperands == 1)
    {
        const auto destOperand = opcode->operands[0];
        if (destOperand.isRegister() || destOperand.isDereferencedFullRegister())
        {
            const auto immediate = ram_->get(pc + 1);
            setOperand(destOperand, immediate);
            return;
        }
        else if (destOperand.isFullRegister() || destOperand.isSpecialRegister())
        {
            const auto immediate = ram_->getImmediate16(pc + 1);
            setOperand(destOperand, immediate);
            return;
        }
    }
    else if (opcode->numOperands == 2)
    {
        const auto destOperand = opcode->operands[0];
        const auto srcOperand = opcode->operands[1];
        setOperand(destOperand, getOperand(srcOperand));
        return;
    }

    throw std::runtime_error("load not implemented for opcode " + toHexString(opcode->opcode));
}

void CPU::INC(uint16_t pc, const OPCode *opcode)
{
    const auto operand = opcode->operands[0];
    const auto operandValue = getOperand(operand);

    if (operandValue.is8bit())
    {
        const auto aluResult = alu::add(operandValue.as8(), static_cast<uint8_t>(1));
        setOperand(operand, aluResult.result);
        setFlagsFromResult(aluResult.flags, opcode);
    }
    else
    {
        const auto aluResult = alu::add(operandValue.as16(), static_cast<uint16_t>(1));
        setOperand(operand, aluResult.result);
        setFlagsFromResult(aluResult.flags, opcode);
    }
}

void CPU::DEC(uint16_t pc, const OPCode *opcode)
{
    const auto operand = opcode->operands[0];
    const auto operandValue = getOperand(operand);

    if (operandValue.is8bit())
    {
        const auto aluResult = alu::sub(operandValue.as8(), static_cast<uint8_t>(1));
        setOperand(operand, aluResult.result);
        setFlagsFromResult(aluResult.flags, opcode);
    }
    else
    {
        const auto aluResult = alu::sub(operandValue.as16(), static_cast<uint16_t>(1));
        setOperand(operand, aluResult.result);
        setFlagsFromResult(aluResult.flags, opcode);
    }
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
    if (opcode->numOperands == 1)
    {
        const auto destOperand = opcode->operands[0];
        const auto firstOperand = getOperand(opcode->operands[0]);

        if (firstOperand.is8bit())
        {
            const auto firstValue = firstOperand.as8();
            const auto secondValue = ram_->get(pc + 1);

            const auto result = alu::add(firstValue, secondValue);

            setOperand(destOperand, result.result);
            setFlagsFromResult(result.flags, opcode);

            return;
        }
        else
        {
            const auto firstValue = firstOperand.as16();
            const auto secondValue = static_cast<int8_t>(ram_->get(pc + 1));

            // TODO: check how half-carry and carry are working for mixed signed and unsigned addition.
            const auto result = alu::add(static_cast<uint8_t>(0x00FF & firstValue), static_cast<uint8_t>(secondValue));

            setOperand(destOperand, static_cast<uint16_t>(firstValue + secondValue));
            setFlagsFromResult(result.flags, opcode);

            return;
        }
    }
    else if (opcode->numOperands == 2)
    {
        const auto destOperand = opcode->operands[0];
        const auto firstOperand = getOperand(opcode->operands[0]);
        const auto secondOperand = getOperand(opcode->operands[1]);

        if (firstOperand.is8bit() && secondOperand.is8bit())
        {
            const auto result = alu::add(firstOperand.as8(), secondOperand.as8());

            setOperand(destOperand, result.result);
            setFlagsFromResult(result.flags, opcode);

            return;
        }
        else if (firstOperand.is16bit() && secondOperand.is16bit())
        {
            const auto result = alu::add(firstOperand.as16(), secondOperand.as16());

            setOperand(destOperand, result.result);
            setFlagsFromResult(result.flags, opcode);

            return;
        }
    }

    throw std::runtime_error("add not implemented for opcode " + toHexString(opcode->opcode));
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
    if (testJumpCondition(opcode->jumpCondition))
    {
        const auto immediate = static_cast<int8_t>(ram_->get(pc + 1));
        setPC(PC() + immediate);
        cycles_ += opcode->additionalCycles;
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
    const auto destOperand = opcode->operands[0];
    const auto sourceOperand = opcode->operands[1];

    const auto fullReg = destOperand.isDereferencedFullRegister()     ? destOperand.asDereferencedFullRegister()
                         : sourceOperand.isDereferencedFullRegister() ? sourceOperand.asDereferencedFullRegister()
                                                                      : std::optional<FullRegister>{};

    if (!fullReg.has_value())
    {
        throw std::runtime_error("load+decrement not implemented for opcode " + toHexString(opcode->opcode));
    }

    setOperand(destOperand, getOperand(sourceOperand));
    const auto currentFullRegisterValue = getFullRegister(*fullReg);
    setFullRegister(*fullReg, static_cast<uint16_t>(currentFullRegisterValue + 1));
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
    const auto destOperand = opcode->operands[0];
    const auto sourceOperand = opcode->operands[1];

    const auto fullReg = destOperand.isDereferencedFullRegister()     ? destOperand.asDereferencedFullRegister()
                         : sourceOperand.isDereferencedFullRegister() ? sourceOperand.asDereferencedFullRegister()
                                                                      : std::optional<FullRegister>{};

    if (!fullReg.has_value())
    {
        throw std::runtime_error("load+decrement not implemented for opcode " + toHexString(opcode->opcode));
    }

    setOperand(destOperand, getOperand(sourceOperand));
    const auto currentFullRegisterValue = getFullRegister(*fullReg);
    setFullRegister(*fullReg, static_cast<uint16_t>(currentFullRegisterValue - 1));
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
    const auto destOperand = opcode->operands[0];

    const auto firstValue = getOperand(destOperand);
    const auto secondValue = opcode->numOperands == 1 ? ram_->get(pc + 1) : getOperand(opcode->operands[1]);

    const auto result = alu::adc(firstValue.as8(), secondValue.as8(), FlagC());
    setOperand(destOperand, result.result);
    setFlagsFromResult(result.flags, opcode);
}

void CPU::SBC(uint16_t pc, const OPCode *opcode)
{
    const auto destOperand = opcode->operands[0];

    const auto firstValue = getOperand(destOperand);
    const auto secondValue = opcode->numOperands == 1 ? ram_->get(pc + 1) : getOperand(opcode->operands[1]);

    const auto result = alu::sbc(firstValue.as8(), secondValue.as8(), FlagC());
    setOperand(destOperand, result.result);
    setFlagsFromResult(result.flags, opcode);
}

void CPU::CP(uint16_t pc, const OPCode *opcode)
{
    const auto firstValue = getOperand(opcode->operands[0]);
    const auto secondValue = opcode->numOperands == 1 ? ram_->get(pc + 1) : getOperand(opcode->operands[1]);
    const auto result = alu::sub(firstValue.as8(), secondValue.as8());

    setFlagsFromResult(result.flags, opcode);
}

void CPU::RET(uint16_t pc, const OPCode *opcode)
{
    if (testJumpCondition(opcode->jumpCondition))
    {
        const auto immediate = popFromStack();
        setPC(immediate);
        cycles_ += opcode->additionalCycles;
    }
}

void CPU::POP(uint16_t pc, const OPCode *opcode)
{
    const auto destOperand = opcode->operands[0];
    auto stackValue = popFromStack();

    if (destOperand.asFullRegister() == FullRegister::AF)
        stackValue &= 0xFFF0;

    setOperand(destOperand, stackValue);
}

void CPU::JP(uint16_t pc, const OPCode *opcode)
{
    if (testJumpCondition(opcode->jumpCondition))
    {
        const auto newAddress =
            opcode->numOperands == 0 ? ram_->getImmediate16(pc + 1) : getOperand(opcode->operands[0]).as16();
        setPC(newAddress);
        cycles_ += opcode->additionalCycles;
    }
}

void CPU::CALL(uint16_t pc, const OPCode *opcode)
{
    if (testJumpCondition(opcode->jumpCondition))
    {
        const auto immediate = ram_->getImmediate16(pc + 1);
        pushToStack(PC());
        setPC(immediate);
        cycles_ += opcode->additionalCycles;
    }
}

void CPU::PUSH(uint16_t pc, const OPCode *opcode)
{
    const auto operandValue = getOperand(opcode->operands[0]);
    pushToStack(operandValue.as16());
}

void CPU::RST(uint16_t pc, const OPCode *opcode)
{
    const auto auxArg = opcode->auxiliaryArguments[0];
    pushToStack(PC());
    const uint16_t rstAddress = concatBytes(0x00, auxArg << 3);
    setPC(rstAddress);
}

void CPU::RETI(uint16_t pc, const OPCode *opcode)
{
    if (testJumpCondition(opcode->jumpCondition))
    {
        const auto immediate = popFromStack();
        setPC(immediate);
        IME_ = true;
        cycles_ += opcode->additionalCycles;
    }
}

void CPU::LDff8(uint16_t pc, const OPCode *opcode)
{
    const auto srcOperand = opcode->operands[opcode->numOperands - 1];
    const auto lowerByte = opcode->numOperands == 2 ? getOperand(opcode->operands[0]).as8() : ram_->get(pc + 1);
    const auto address = concatBytes(0xff, lowerByte);
    const auto value = getOperand(srcOperand).as8();
    ram_->set(address, value);
}

void CPU::LDa16(uint16_t pc, const OPCode *opcode)
{
    const auto srcOperand = opcode->operands[0];
    const auto address = ram_->getImmediate16(pc + 1);
    const auto value = getOperand(srcOperand).as8();
    ram_->set(address, value);
}

void CPU::LDaff8(uint16_t pc, const OPCode *opcode)
{
    const auto destOperand = opcode->operands[0];
    const auto lowerByte = opcode->numOperands == 2 ? getOperand(opcode->operands[1]).as8() : ram_->get(pc + 1);
    const auto address = concatBytes(0xff, lowerByte);
    const auto value = ram_->get(address);
    setOperand(destOperand, value);
}

void CPU::DI(uint16_t pc, const OPCode *opcode)
{
    IME_ = false;
}

void CPU::LDaff16(uint16_t pc, const OPCode *opcode)
{
    const auto destOperand = opcode->operands[0];
    const auto address = ram_->getImmediate16(pc + 1);
    const auto value = ram_->get(address);
    setOperand(destOperand, value);
}

void CPU::LDs8(uint16_t pc, const OPCode *opcode)
{
    const auto destOperand = opcode->operands[0];
    const auto srcOperand = opcode->operands[1];

    const auto srcValue = getOperand(srcOperand).as16();
    const auto offset = static_cast<int8_t>(ram_->get(pc + 1));

    const auto result = alu::add(static_cast<uint8_t>(0x00FF & srcValue), static_cast<uint8_t>(offset));

    setOperand(destOperand, static_cast<uint16_t>(srcValue + offset));
    setFlagsFromResult(result.flags, opcode);
}

void CPU::EI(uint16_t pc, const OPCode *opcode)
{
    interruptsEnabledQueued_ = true;
}

void CPU::RL(uint16_t pc, const OPCode *opcode)
{
    const auto operand = opcode->operands[0];

    const auto currentValue = getOperand(operand).as8();
    const auto result = alu::rl(currentValue, FlagC());

    setOperand(operand, result.result);
    setFlagsFromResult(result.flags, opcode);
}

void CPU::RR(uint16_t pc, const OPCode *opcode)
{
    const auto operand = opcode->operands[0];

    const auto currentValue = getOperand(operand).as8();
    const auto result = alu::rr(currentValue, FlagC());

    setOperand(operand, result.result);
    setFlagsFromResult(result.flags, opcode);
}

void CPU::BIT_GET(uint16_t pc, const OPCode *opcode)
{
    const auto operand = opcode->operands[0];
    const auto bitIndex = opcode->auxiliaryArguments[0];

    const auto currentValue = getOperand(operand).as8();
    const auto result = alu::bit_get(currentValue, bitIndex);

    setFlagsFromResult(result.flags, opcode);
}

void CPU::BIT_SET(uint16_t pc, const OPCode *opcode)
{
    const auto operand = opcode->operands[0];
    const auto bitIndex = opcode->auxiliaryArguments[0];
    const auto newBit = opcode->auxiliaryArguments[1];

    const auto currentValue = getOperand(operand).as8();
    const auto result = alu::bit_set(currentValue, bitIndex, newBit);

    setOperand(operand, result.result);
    setFlagsFromResult(result.flags, opcode);
}

} // namespace gbemu::backend
