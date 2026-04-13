#include "gbemu/backend/cpu.h"

#include "gbemu/backend/bitutils.h"

#include <algorithm>
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

void CPU::setMode(Mode mode) { mode_ = mode; }

void CPU::setIME(bool newIME) { IME_ = newIME; }

void CPU::setPC(uint16_t newRegVal) { PC_ = newRegVal; }
void CPU::setSP(uint16_t newRegVal) { SP_ = newRegVal; }

void CPU::setAF(uint16_t newRegVal) { AF_ = newRegVal; }
void CPU::setBC(uint16_t newRegVal) { BC_ = newRegVal; }
void CPU::setDE(uint16_t newRegVal) { DE_ = newRegVal; }
void CPU::setHL(uint16_t newRegVal) { HL_ = newRegVal; }

void CPU::setA(uint8_t newRegVal) { AF_ = setUpperByte(AF_, newRegVal); }
void CPU::setB(uint8_t newRegVal) { BC_ = setUpperByte(BC_, newRegVal); }
void CPU::setC(uint8_t newRegVal) { BC_ = setLowerByte(BC_, newRegVal); }
void CPU::setD(uint8_t newRegVal) { DE_ = setUpperByte(DE_, newRegVal); }
void CPU::setE(uint8_t newRegVal) { DE_ = setLowerByte(DE_, newRegVal); }
void CPU::setH(uint8_t newRegVal) { HL_ = setUpperByte(HL_, newRegVal); }
void CPU::setL(uint8_t newRegVal) { HL_ = setLowerByte(HL_, newRegVal); }

void CPU::setFlagZ(uint8_t newFlagVal) { AF_ = setBit(AF_, FLAG_Z_BIT, newFlagVal); }
void CPU::setFlagN(uint8_t newFlagVal) { AF_ = setBit(AF_, FLAG_N_BIT, newFlagVal); }
void CPU::setFlagH(uint8_t newFlagVal) { AF_ = setBit(AF_, FLAG_H_BIT, newFlagVal); }
void CPU::setFlagC(uint8_t newFlagVal) { AF_ = setBit(AF_, FLAG_C_BIT, newFlagVal); }
void CPU::setFlags(uint8_t newZ, uint8_t newN, uint8_t newH, uint8_t newC)
{
    setFlagZ(newZ);
    setFlagN(newN);
    setFlagH(newH);
    setFlagC(newC);
}

void CPU::advancePC(uint16_t inc) { PC_ += inc; }
void CPU::offsetSP(int32_t offset) { SP_ += offset; }

void CPU::setRegister(Register reg, uint8_t newRegVal)
{
    switch (reg)
    {
    case Register::A: setA(newRegVal); return;
    case Register::B: setB(newRegVal); return;
    case Register::C: setC(newRegVal); return;
    case Register::D: setD(newRegVal); return;
    case Register::E: setE(newRegVal); return;
    case Register::H: setH(newRegVal); return;
    case Register::L: setL(newRegVal); return;
    }
}

void CPU::setFullRegister(FullRegister reg, uint16_t newRegVal)
{
    switch (reg)
    {
    case FullRegister::BC: setBC(newRegVal); return;
    case FullRegister::DE: setDE(newRegVal); return;
    case FullRegister::HL: setHL(newRegVal); return;
    case FullRegister::AF: setAF(newRegVal); return;
    }
}

void CPU::serviceInterrupts()
{
    if (!IME_ && mode_ != Mode::HALT)
    {
        return;
    }

    // Any pending interrupt wakes the CPU from HALT
    const auto interruptFlags = ram_->get(RAM::IF);
    const auto interruptEnable = ram_->get(RAM::IE);
    const uint8_t pendingInterrupts = interruptFlags & interruptEnable & 0x1f;

    if (pendingInterrupts)
    {
        setMode(Mode::NORMAL);
    }

    if (!IME_)
    {
        return;
    }

    // Service the highest-priority pending interrupt
    constexpr std::array<uint8_t, 3> INTERRUPT_BITS = {0, 1, 2}; // VBLANK, LCD_STAT, TIMER

    const auto pendingInterruptIt =
        std::ranges::find_if(INTERRUPT_BITS, [&pendingInterrupts](auto bit) { return getBit(pendingInterrupts, bit); });
    if (pendingInterruptIt == INTERRUPT_BITS.end())
    {
        return;
    }
    const auto bit = *pendingInterruptIt;

    IME_ = false;
    pushToStack(PC_);
    ram_->set(RAM::IF, setBit(interruptFlags, bit, 0));
    PC_ = 0x40 + 0x08 * bit;
    return;
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
    case Operand::Kind::REG: return getRegister(operand.asRegister());
    case Operand::Kind::FULL_REG: return getFullRegister(operand.asFullRegister());
    case Operand::Kind::SPECIAL_REG: return SP();
    case Operand::Kind::DEREF_FULL_REG: return ram_->get(getFullRegister(operand.asDereferencedFullRegister()));
    case Operand::Kind::NONE: throw std::runtime_error("Cannot get value of NONE operand.");
    }
}

void CPU::setOperand(Operand operand, OperandValue newValue)
{
    switch (operand.kind)
    {
    case Operand::Kind::REG: setRegister(operand.asRegister(), newValue.as8()); return;
    case Operand::Kind::FULL_REG: setFullRegister(operand.asFullRegister(), newValue.as16()); return;
    case Operand::Kind::SPECIAL_REG: setSP(newValue.as16()); return;
    case Operand::Kind::DEREF_FULL_REG:
        ram_->set(getFullRegister(operand.asDereferencedFullRegister()), newValue.as8());
        return;
    case Operand::Kind::NONE: throw std::runtime_error("Cannot set value of NONE operand.");
    }
}

void CPU::setFlagsFromResult(const alu::AluFlagResult &flagResult, const OPCode *opcode)
{
    const auto opcodeFlags = opcode->flags;

    std::array<uint8_t, 4> newFlags = {FlagZ(), FlagN(), FlagH(), FlagC()};

    for (int i = 0; i < 4; i++)
    {
        const auto flag = opcodeFlags[i];
        switch (flag)
        {
        case OPCode::Flag::UNTOUCHED: break;
        case OPCode::Flag::ONE: newFlags[i] = 1; break;
        case OPCode::Flag::ZERO: newFlags[i] = 0; break;
        case OPCode::Flag::Z: newFlags[i] = flagResult.isZero ? 1 : 0; break;
        case OPCode::Flag::H: newFlags[i] = flagResult.hadHalfCarry ? 1 : 0; break;
        case OPCode::Flag::CY: newFlags[i] = flagResult.hadCarry ? 1 : 0; break;
        case OPCode::Flag::NOT_CY: newFlags[i] = (FlagC() == 1) ? 0 : 1; break;
        default: {
            // A0..A7 and NOT_A0..NOT_A7 — extract from flagBits
            const auto flagInt = static_cast<int>(flag);
            const auto a0Int = static_cast<int>(OPCode::Flag::A0);
            const auto notA0Int = static_cast<int>(OPCode::Flag::NOT_A0);
            if (flagInt >= a0Int && flagInt <= a0Int + 7)
            {
                newFlags[i] = getBit(flagResult.flagBits, flagInt - a0Int);
            }
            else if (flagInt >= notA0Int && flagInt <= notA0Int + 7)
            {
                newFlags[i] = getBit(flagResult.flagBits, flagInt - notA0Int) ? 0 : 1;
            }
            break;
        }
        }
    }

    setFlags(newFlags[0], newFlags[1], newFlags[2], newFlags[3]);
}

auto CPU::testJumpCondition(OPCode::JumpCondition jumpCondition) const -> bool
{
    switch (jumpCondition)
    {
    case OPCode::JumpCondition::ALWAYS: return true;
    case OPCode::JumpCondition::Z: return FlagZ() == 1;
    case OPCode::JumpCondition::C: return FlagC() == 1;
    case OPCode::JumpCondition::NZ: return FlagZ() == 0;
    case OPCode::JumpCondition::NC: return FlagC() == 0;
    }
}

/*** OPCode Handlers ***/

void CPU::UNIMPLEMENTED(uint16_t pc, const OPCode *opcode)
{
    throw std::runtime_error("Unimplemented opcode handler called for opcode " + toHexString(opcode->opcode));
}

void CPU::NOP(uint16_t pc, const OPCode *opcode) {}

void CPU::LD(uint16_t pc, const OPCode *opcode)
{
    if (opcode->numOperands == 1)
    {
        const auto dest = opcode->operands[0];
        if (dest.isRegister() || dest.isDereferencedFullRegister())
        {
            setOperand(dest, ram_->get(pc + 1));
            return;
        }
        else if (dest.isFullRegister() || dest.isSpecialRegister())
        {
            setOperand(dest, ram_->getImmediate16(pc + 1));
            return;
        }
    }
    else if (opcode->numOperands == 2)
    {
        setOperand(opcode->operands[0], getOperand(opcode->operands[1]));
        return;
    }

    throw std::runtime_error("load not implemented for opcode " + toHexString(opcode->opcode));
}

void CPU::LD_SP(uint16_t pc, const OPCode *opcode) { ram_->setImmediate16(ram_->getImmediate16(pc + 1), SP()); }

void CPU::ADD(uint16_t pc, const OPCode *opcode)
{
    if (opcode->numOperands == 1)
    {
        const auto dest = opcode->operands[0];
        const auto first = getOperand(dest);

        if (first.is8bit())
        {
            const auto result = alu::add(first.as8(), ram_->get(pc + 1));
            setOperand(dest, result.result);
            setFlagsFromResult(result.flags, opcode);
        }
        else
        {
            const auto firstValue = first.as16();
            const auto offset = static_cast<int8_t>(ram_->get(pc + 1));
            const auto result = alu::add(static_cast<uint8_t>(0x00FF & firstValue), static_cast<uint8_t>(offset));
            setOperand(dest, static_cast<uint16_t>(firstValue + offset));
            setFlagsFromResult(result.flags, opcode);
        }
    }
    else if (opcode->numOperands == 2)
    {
        const auto dest = opcode->operands[0];
        const auto first = getOperand(dest);
        const auto second = getOperand(opcode->operands[1]);

        if (first.is8bit() && second.is8bit())
        {
            const auto result = alu::add(first.as8(), second.as8());
            setOperand(dest, result.result);
            setFlagsFromResult(result.flags, opcode);
        }
        else if (first.is16bit() && second.is16bit())
        {
            const auto result = alu::add(first.as16(), second.as16());
            setOperand(dest, result.result);
            setFlagsFromResult(result.flags, opcode);
        }
        else
        {
            throw std::runtime_error("add not implemented for opcode " + toHexString(opcode->opcode));
        }
    }
    else
    {
        throw std::runtime_error("add not implemented for opcode " + toHexString(opcode->opcode));
    }
}

void CPU::DAA(uint16_t pc, const OPCode *opcode)
{
    auto value = A();
    auto correction = 0;
    uint8_t newFlagC = 0;

    if (FlagH() == 1 || (FlagN() != 1 && (value & 0x0f) > 0x09))
    {
        correction |= 0x06;
    }

    if (FlagC() == 1 || (FlagN() != 1 && value > 0x99))
    {
        correction |= 0x60;
        newFlagC = 1;
    }

    value = FlagN() == 1 ? value - correction : value + correction;

    setFlags((value == 0) ? 1 : 0, FlagN(), 0, newFlagC);
    setA(value);
}

void CPU::HALT(uint16_t pc, const OPCode *opcode) { mode_ = Mode::HALT; }

void CPU::RET(uint16_t pc, const OPCode *opcode)
{
    if (testJumpCondition(opcode->jumpCondition))
    {
        setPC(popFromStack());
        cycles_ += opcode->additionalCycles;
    }
}

void CPU::POP(uint16_t pc, const OPCode *opcode)
{
    auto stackValue = popFromStack();
    if (opcode->operands[0].asFullRegister() == FullRegister::AF)
    {
        stackValue &= 0xFFF0;
    }
    setOperand(opcode->operands[0], stackValue);
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

void CPU::JR(uint16_t pc, const OPCode *opcode)
{
    if (testJumpCondition(opcode->jumpCondition))
    {
        setPC(PC() + static_cast<int8_t>(ram_->get(pc + 1)));
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

void CPU::PUSH(uint16_t pc, const OPCode *opcode) { pushToStack(getOperand(opcode->operands[0]).as16()); }

void CPU::RST(uint16_t pc, const OPCode *opcode)
{
    pushToStack(PC());
    setPC(concatBytes(0x00, opcode->auxiliaryArguments[0] << 3));
}

void CPU::RETI(uint16_t pc, const OPCode *opcode)
{
    if (testJumpCondition(opcode->jumpCondition))
    {
        setPC(popFromStack());
        IME_ = true;
        cycles_ += opcode->additionalCycles;
    }
}

void CPU::LDff8(uint16_t pc, const OPCode *opcode)
{
    const auto lowerByte = opcode->numOperands == 2 ? getOperand(opcode->operands[0]).as8() : ram_->get(pc + 1);
    ram_->set(concatBytes(0xff, lowerByte), getOperand(opcode->operands[opcode->numOperands - 1]).as8());
}

void CPU::LDa16(uint16_t pc, const OPCode *opcode)
{
    ram_->set(ram_->getImmediate16(pc + 1), getOperand(opcode->operands[0]).as8());
}

void CPU::LDaff8(uint16_t pc, const OPCode *opcode)
{
    const auto lowerByte = opcode->numOperands == 2 ? getOperand(opcode->operands[1]).as8() : ram_->get(pc + 1);
    setOperand(opcode->operands[0], ram_->get(concatBytes(0xff, lowerByte)));
}

void CPU::LDaff16(uint16_t pc, const OPCode *opcode)
{
    setOperand(opcode->operands[0], ram_->get(ram_->getImmediate16(pc + 1)));
}

void CPU::LDs8(uint16_t pc, const OPCode *opcode)
{
    const auto srcValue = getOperand(opcode->operands[1]).as16();
    const auto offset = static_cast<int8_t>(ram_->get(pc + 1));
    const auto result = alu::add(static_cast<uint8_t>(0x00FF & srcValue), static_cast<uint8_t>(offset));
    setOperand(opcode->operands[0], static_cast<uint16_t>(srcValue + offset));
    setFlagsFromResult(result.flags, opcode);
}

void CPU::DI(uint16_t pc, const OPCode *opcode) { IME_ = false; }

void CPU::EI(uint16_t pc, const OPCode *opcode) { interruptsEnabledQueued_ = true; }

void CPU::BIT_GET(uint16_t pc, const OPCode *opcode)
{
    const auto result = alu::bit_get(getOperand(opcode->operands[0]).as8(), opcode->auxiliaryArguments[0]);
    setFlagsFromResult(result.flags, opcode);
}

void CPU::BIT_SET(uint16_t pc, const OPCode *opcode)
{
    const auto operand = opcode->operands[0];
    const auto result =
        alu::bit_set(getOperand(operand).as8(), opcode->auxiliaryArguments[0], opcode->auxiliaryArguments[1]);
    setOperand(operand, result.result);
    setFlagsFromResult(result.flags, opcode);
}

} // namespace gbemu::backend
