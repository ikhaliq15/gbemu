#include "cpu.h"
#include "bitutils.h"

#include <iostream>

namespace gbemu {

    CPU::CPU(std::shared_ptr<RAM> ram, const std::string& opcodeDataFile)
    : IME_(false)
    , PC_(STARTING_PC)
    , SP_(STARTING_SP)
    , AF_(STARTING_AF)
    , BC_(STARTING_BC)
    , DE_(STARTING_DE)
    , HL_(STARTING_HL)
    , ram_(ram)
    , cycles_(0ul)
    {
        std::tie(opcodes_, prefixedOpcodes_) = OPCode::constructOpcodes(opcodeDataFile);

        opcodeFunctions_.insert(std::make_pair<std::string, OPCodeHandler>(
            std::string("NOP"), 
            [this](uint16_t pc, const OPCode& opcode) { NOP(pc, opcode); }
        ));

        opcodeFunctions_.insert(std::make_pair<std::string, OPCodeHandler>(
            std::string("LD"), 
            [this](uint16_t pc, const OPCode& opcode) { LD(pc, opcode); }
        ));

        opcodeFunctions_.insert(std::make_pair<std::string, OPCodeHandler>(
            std::string("INC"), 
            [this](uint16_t pc, const OPCode& opcode) { INC(pc, opcode); }
        ));

        opcodeFunctions_.insert(std::make_pair<std::string, OPCodeHandler>(
            std::string("DEC"), 
            [this](uint16_t pc, const OPCode& opcode) { DEC(pc, opcode); }
        ));

        opcodeFunctions_.insert(std::make_pair<std::string, OPCodeHandler>(
            std::string("RLCA"), 
            [this](uint16_t pc, const OPCode& opcode) { RLCA(pc, opcode); }
        ));

        opcodeFunctions_.insert(std::make_pair<std::string, OPCodeHandler>(
            std::string("LD_SP"), 
            [this](uint16_t pc, const OPCode& opcode) { LD_SP(pc, opcode); }
        ));

        opcodeFunctions_.insert(std::make_pair<std::string, OPCodeHandler>(
            std::string("ADD"), 
            [this](uint16_t pc, const OPCode& opcode) { ADD(pc, opcode); }
        ));

        opcodeFunctions_.insert(std::make_pair<std::string, OPCodeHandler>(
            std::string("RRCA"), 
            [this](uint16_t pc, const OPCode& opcode) { RRCA(pc, opcode); }
        ));

        opcodeFunctions_.insert(std::make_pair<std::string, OPCodeHandler>(
            std::string("RLA"), 
            [this](uint16_t pc, const OPCode& opcode) { RLA(pc, opcode); }
        ));

        opcodeFunctions_.insert(std::make_pair<std::string, OPCodeHandler>(
            std::string("JR"), 
            [this](uint16_t pc, const OPCode& opcode) { JR(pc, opcode); }
        ));

        opcodeFunctions_.insert(std::make_pair<std::string, OPCodeHandler>(
            std::string("RRA"), 
            [this](uint16_t pc, const OPCode& opcode) { RRA(pc, opcode); }
        ));

        opcodeFunctions_.insert(std::make_pair<std::string, OPCodeHandler>(
            std::string("LDI"), 
            [this](uint16_t pc, const OPCode& opcode) { LDI(pc, opcode); }
        ));

        opcodeFunctions_.insert(std::make_pair<std::string, OPCodeHandler>(
            std::string("DAA"), 
            [this](uint16_t pc, const OPCode& opcode) { DAA(pc, opcode); }
        ));

        opcodeFunctions_.insert(std::make_pair<std::string, OPCodeHandler>(
            std::string("CPL"), 
            [this](uint16_t pc, const OPCode& opcode) { CPL(pc, opcode); }
        ));

        opcodeFunctions_.insert(std::make_pair<std::string, OPCodeHandler>(
            std::string("LDD"), 
            [this](uint16_t pc, const OPCode& opcode) { LDD(pc, opcode); }
        ));

        opcodeFunctions_.insert(std::make_pair<std::string, OPCodeHandler>(
            std::string("SCF"), 
            [this](uint16_t pc, const OPCode& opcode) { SCF(pc, opcode); }
        ));

        opcodeFunctions_.insert(std::make_pair<std::string, OPCodeHandler>(
            std::string("CCF"), 
            [this](uint16_t pc, const OPCode& opcode) { CCF(pc, opcode); }
        ));

        opcodeFunctions_.insert(std::make_pair<std::string, OPCodeHandler>(
            std::string("ADC"), 
            [this](uint16_t pc, const OPCode& opcode) { ADC(pc, opcode); }
        ));

        opcodeFunctions_.insert(std::make_pair<std::string, OPCodeHandler>(
            std::string("SUB"), 
            [this](uint16_t pc, const OPCode& opcode) { SUB(pc, opcode); }
        ));

        opcodeFunctions_.insert(std::make_pair<std::string, OPCodeHandler>(
            std::string("SBC"), 
            [this](uint16_t pc, const OPCode& opcode) { SBC(pc, opcode); }
        ));

        opcodeFunctions_.insert(std::make_pair<std::string, OPCodeHandler>(
            std::string("AND"), 
            [this](uint16_t pc, const OPCode& opcode) { AND(pc, opcode); }
        ));

        opcodeFunctions_.insert(std::make_pair<std::string, OPCodeHandler>(
            std::string("XOR"), 
            [this](uint16_t pc, const OPCode& opcode) { XOR(pc, opcode); }
        ));

        opcodeFunctions_.insert(std::make_pair<std::string, OPCodeHandler>(
            std::string("OR"), 
            [this](uint16_t pc, const OPCode& opcode) { OR(pc, opcode); }
        ));

        opcodeFunctions_.insert(std::make_pair<std::string, OPCodeHandler>(
            std::string("CP"), 
            [this](uint16_t pc, const OPCode& opcode) { CP(pc, opcode); }
        ));

        opcodeFunctions_.insert(std::make_pair<std::string, OPCodeHandler>(
            std::string("RET"), 
            [this](uint16_t pc, const OPCode& opcode) { RET(pc, opcode); }
        ));

        opcodeFunctions_.insert(std::make_pair<std::string, OPCodeHandler>(
            std::string("POP"), 
            [this](uint16_t pc, const OPCode& opcode) { POP(pc, opcode); }
        ));

        opcodeFunctions_.insert(std::make_pair<std::string, OPCodeHandler>(
            std::string("JP"), 
            [this](uint16_t pc, const OPCode& opcode) { JP(pc, opcode); }
        ));

        opcodeFunctions_.insert(std::make_pair<std::string, OPCodeHandler>(
            std::string("CALL"), 
            [this](uint16_t pc, const OPCode& opcode) { CALL(pc, opcode); }
        ));

        opcodeFunctions_.insert(std::make_pair<std::string, OPCodeHandler>(
            std::string("PUSH"), 
            [this](uint16_t pc, const OPCode& opcode) { PUSH(pc, opcode); }
        ));

        opcodeFunctions_.insert(std::make_pair<std::string, OPCodeHandler>(
            std::string("RST"), 
            [this](uint16_t pc, const OPCode& opcode) { RST(pc, opcode); }
        ));

        opcodeFunctions_.insert(std::make_pair<std::string, OPCodeHandler>(
            std::string("RETI"), 
            [this](uint16_t pc, const OPCode& opcode) { RETI(pc, opcode); }
        ));

        opcodeFunctions_.insert(std::make_pair<std::string, OPCodeHandler>(
            std::string("LDff8"), 
            [this](uint16_t pc, const OPCode& opcode) { LDff8(pc, opcode); }
        ));

        opcodeFunctions_.insert(std::make_pair<std::string, OPCodeHandler>(
            std::string("LDa16"), 
            [this](uint16_t pc, const OPCode& opcode) { LDa16(pc, opcode); }
        ));

        opcodeFunctions_.insert(std::make_pair<std::string, OPCodeHandler>(
            std::string("LDaff8"), 
            [this](uint16_t pc, const OPCode& opcode) { LDaff8(pc, opcode); }
        ));

        opcodeFunctions_.insert(std::make_pair<std::string, OPCodeHandler>(
            std::string("DI"), 
            [this](uint16_t pc, const OPCode& opcode) { DI(pc, opcode); }
        ));

        opcodeFunctions_.insert(std::make_pair<std::string, OPCodeHandler>(
            std::string("EI"), 
            [this](uint16_t pc, const OPCode& opcode) { EI(pc, opcode); }
        ));

        opcodeFunctions_.insert(std::make_pair<std::string, OPCodeHandler>(
            std::string("LDaff16"), 
            [this](uint16_t pc, const OPCode& opcode) { LDaff16(pc, opcode); }
        ));

        opcodeFunctions_.insert(std::make_pair<std::string, OPCodeHandler>(
            std::string("RR"), 
            [this](uint16_t pc, const OPCode& opcode) { RR(pc, opcode); }
        ));

        opcodeFunctions_.insert(std::make_pair<std::string, OPCodeHandler>(
            std::string("SLA"), 
            [this](uint16_t pc, const OPCode& opcode) { SLA(pc, opcode); }
        ));

        opcodeFunctions_.insert(std::make_pair<std::string, OPCodeHandler>(
            std::string("SWAP"), 
            [this](uint16_t pc, const OPCode& opcode) { SWAP(pc, opcode); }
        ));

        opcodeFunctions_.insert(std::make_pair<std::string, OPCodeHandler>(
            std::string("SRL"), 
            [this](uint16_t pc, const OPCode& opcode) { SRL(pc, opcode); }
        ));

        opcodeFunctions_.insert(std::make_pair<std::string, OPCodeHandler>(
            std::string("BIT_GET"), 
            [this](uint16_t pc, const OPCode& opcode) { BIT_SET(pc, opcode); }
        ));

        opcodeFunctions_.insert(std::make_pair<std::string, OPCodeHandler>(
            std::string("BIT_SET"), 
            [this](uint16_t pc, const OPCode& opcode) { BIT_SET(pc, opcode); }
        ));
    }

    CPU::CPU(const CPU& cpu)
    : IME_(cpu.IME_)
    , PC_(cpu.PC_)
    , SP_(cpu.SP_)
    , AF_(cpu.AF_)
    , BC_(cpu.BC_)
    , DE_(cpu.DE_)
    , HL_(cpu.HL_)
    , ram_(std::make_shared<RAM>(*cpu.ram_))
    , cycles_(cpu.cycles_)
    , opcodes_(cpu.opcodes_)
    {}

    bool CPU::IME() const { return IME_; }

    uint16_t CPU::PC() const { return PC_; }
    uint16_t CPU::SP() const { return SP_; }

    uint16_t CPU::AF() const { return AF_; }
    uint16_t CPU::BC() const { return BC_; }
    uint16_t CPU::DE() const { return DE_; }
    uint16_t CPU::HL() const { return HL_; }

    uint8_t CPU::A() const { return upperByte(AF_); }
    uint8_t CPU::B() const { return upperByte(BC_); }
    uint8_t CPU::C() const { return lowerByte(BC_); }
    uint8_t CPU::D() const { return upperByte(DE_); }
    uint8_t CPU::E() const { return lowerByte(DE_); }
    uint8_t CPU::H() const { return upperByte(HL_); }
    uint8_t CPU::L() const { return lowerByte(HL_); }

    uint8_t CPU::FlagZ() const { return getBit(AF_, FLAG_Z_BIT); }
    uint8_t CPU::FlagN() const { return getBit(AF_, FLAG_N_BIT); }
    uint8_t CPU::FlagH() const { return getBit(AF_, FLAG_H_BIT); }
    uint8_t CPU::FlagC() const { return getBit(AF_, FLAG_C_BIT); }

    std::shared_ptr<RAM> CPU::ram() const { return ram_; }

    size_t CPU::cycles() const { return cycles_; }

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

    uint8_t CPU::getRegister(Register reg) const
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

    uint16_t CPU::getFullRegister(FullRegister reg) const
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

    void CPU::requestInterupt(Interrupt interrupt)
    {
        switch (interrupt) {
            case Interrupt::VBLANK:
                const auto currentIF = ram_->get(RAM::IF);
                const auto newIF = setBit(currentIF, 0, 1);
                ram_->set(RAM::IF, newIF);
                return;
        }
    }

    void CPU::executeInstruction(bool verbose)
    {
        if (verbose)
            std::cout << *this << std::endl;

        auto opcodeValue = ram_->get(PC());
        auto opcodeMap = &opcodes_;
        auto prefixedOpcode = false;
        
        if (opcodeValue == OPCode::PREFIX_OPCODE)
        {
            opcodeValue = ram_->get(PC() + 1);
            opcodeMap = &prefixedOpcodes_;
            prefixedOpcode = true;
        }

        const auto opcodeString = (prefixedOpcode) ? (toHexString(opcodeValue) + " (CB)") : toHexString(opcodeValue);

        const auto it = opcodeMap->find(opcodeValue);
        if (it == opcodeMap->end())
            throw std::runtime_error(std::string("Unknown opcode detected: ") + opcodeString + std::string(" at PC=") + toHexString(PC()));
        const auto opcode = it->second;

        const auto oldPC = PC();
        advancePC(opcode.bytes());

        const auto it2 = opcodeFunctions_.find(opcode.mnemonic());
        if (it2 == opcodeFunctions_.end())
            throw std::runtime_error(
                std::string("Unknown mnemonic ") + opcode.mnemonic() + 
                std::string(" detected for opcode: ") + opcodeString
            );
        const auto opcodeHandler = it2->second;

        opcodeHandler(oldPC, opcode);

        cycles_ += opcode.cycles();

        // if (verbose)
        // {
        //     std::cout
        //             << std::setw(20) << std::left << opcode.command()
        //             << std::setw(17) << ("Opcode=" + opcodeString) << "   "
        //             << std::setw(9) << ("PC=" + toHexString(PC())) << "   "
        //             << std::setw(9) << ("SP=" + toHexString(SP())) << "   "
        //             << std::setw(9) << ("AF=" + toHexString(AF())) << "   "
        //             << std::setw(9)<<  ("BC=" + toHexString(BC())) << "   "
        //             << std::setw(9) << ("DE=" + toHexString(DE())) << "   "
        //             << std::setw(9) << ("HL=" + toHexString(HL())) << "   "
        //             << std::setw(9) << ("LY=" + std::to_string(ram_->get(RAM::LY))) << "   "
        //             << std::setw(11) << ("LCDC=" + toHexString(ram_->get(RAM::LCDC))) << "   "
        //             << std::setw(9) << ("IF=" + toHexString(ram_->get(RAM::IF))) << "   "
        //             << std::setw(9) << ("IE=" + toHexString (ram_->get(RAM::IE))) << "   "
        //             << std::setw(9) << ("IME=" + (IME_ ? std::string("true") : std::string("false"))) << "   "
        //             << std::endl;
        // }
    }

    bool CPU::operator ==(const CPU& rhs) const
    {
        return PC_ == rhs.PC_ &&
                SP_ == rhs.SP_ &&
                AF_ == rhs.AF_ &&
                BC_ == rhs.BC_ &&
                DE_ == rhs.DE_ &&
                HL_ == rhs.HL_ &&
                cycles_ == rhs.cycles_ &&
                IME_ == rhs.IME_ &&
                (*ram_ == *(rhs.ram_));
    }

    bool CPU::operator !=(const CPU& rhs) const
    {
        return !(*this == rhs);
    }

    std::ostream& operator<<(std::ostream& os, const CPU& cpu)
    {
        os
            << "A:" << toHexString(cpu.A(), false) << " "
            << "F:" << toHexString(lowerByte(cpu.AF()), false) << " "
            << "B:" << toHexString(cpu.B(), false) << " "
            << "C:" << toHexString(cpu.C(), false) << " "
            << "D:" << toHexString(cpu.D(), false) << " "
            << "E:" << toHexString(cpu.E(), false) << " "
            << "H:" << toHexString(cpu.H(), false) << " "
            << "L:" << toHexString(cpu.L(), false) << " "
            << "SP:" << toHexString(cpu.SP(), false) << " "
            << "PC:" << toHexString(cpu.PC(), false) << " "
            << "PCMEM:"
                << toHexString(cpu.ram()->get(cpu.PC()), false) << ","
                << toHexString(cpu.ram()->get(cpu.PC() + 1), false) << ","
                << toHexString(cpu.ram()->get(cpu.PC() + 2), false) << ","
                << toHexString(cpu.ram()->get(cpu.PC() + 3), false);

        return os;
    }

    void CPU::pushToStack(uint16_t val)
    {
        offsetSP(-1);
        ram_->set(SP(), upperByte(val));

        offsetSP(-1);
        ram_->set(SP(), lowerByte(val));
    }

    uint16_t CPU::popFromStack()
    {
        const auto lower = ram_->get(SP());
        offsetSP(1);

        const auto upper = ram_->get(SP());
        offsetSP(1);

        return concatBytes(upper, lower);
    }

    // TODO: cleaner way to handle than if-statement galore?
    OperandValue CPU::getOperand(Operand operand) const
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
        else {
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
        else {
            throw std::runtime_error("Unknown operand type.");
        }
    }

    void CPU::setFlagsFromResult(const alu::AluFlagResult& flagResult, const OPCode& opcode)
    {
        const auto opcodeFlags = opcode.flags();

        uint8_t newFlags[4] = {FlagZ(), FlagN(), FlagH(), FlagC()};

        for (int i = 0; i < 4; i ++)
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

    bool CPU::testJumpCondition(OPCode::JumpCondition jumpCondition) const
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

    void CPU::NOP(uint16_t pc, const OPCode& opcode)
    {
        return;
    }

    void CPU::LD(uint16_t pc, const OPCode& opcode)
    {
        if (opcode.operands().size() == 1)
        {
            const auto destOperand = opcode.operands()[0];
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
        else if (opcode.operands().size() == 2)
        {
            const auto destOperand = opcode.operands()[0];
            const auto srcOperand = opcode.operands()[1];
            setOperand(destOperand, getOperand(srcOperand));
            return;
        }

        throw std::runtime_error("load not implemented for opcode " + toHexString(opcode.opcode()));
    }

    void CPU::INC(uint16_t pc, const OPCode& opcode)
    {
        const auto operand = opcode.operands()[0];
        const auto operandValue = getOperand(operand);

        if (std::holds_alternative<uint8_t>(operandValue))
        {
            const auto currentValue = std::get<uint8_t>(operandValue);
            const auto aluResult = alu::add(currentValue, (uint8_t) 1);
            setOperand(operand, aluResult.result);
            setFlagsFromResult(aluResult.flags, opcode);
        }
        else if (std::holds_alternative<uint16_t>(operandValue))
        {
            const auto currentValue = std::get<uint16_t>(operandValue);
            const auto aluResult = alu::add(currentValue, (uint16_t) 1);
            setOperand(operand, aluResult.result);
            setFlagsFromResult(aluResult.flags, opcode);
        }
        else
            throw std::runtime_error("inc not implemented for opcode " + toHexString(opcode.opcode()));
    }

    void CPU::DEC(uint16_t pc, const OPCode& opcode)
    {
        const auto operand = opcode.operands()[0];
        const auto operandValue = getOperand(operand);

        if (std::holds_alternative<uint8_t>(operandValue))
        {
            const auto currentValue = std::get<uint8_t>(operandValue);
            const auto aluResult = alu::sub(currentValue, (uint8_t) 1);
            setOperand(operand, aluResult.result);
            setFlagsFromResult(aluResult.flags, opcode);
        }
        else if (std::holds_alternative<uint16_t>(operandValue))
        {
            const auto currentValue = std::get<uint16_t>(operandValue);
            const auto aluResult = alu::sub(currentValue, (uint16_t) 1);
            setOperand(operand, aluResult.result);
            setFlagsFromResult(aluResult.flags, opcode);
        }
        else
            throw std::runtime_error("dec not implemented for opcode " + toHexString(opcode.opcode()));
    }

    void CPU::RLCA(uint16_t pc, const OPCode& opcode)
    {
        const auto currentValue = A();
        const auto result = alu::rlc(currentValue);

        setA(result.result);
        setFlagsFromResult(result.flags, opcode);
    }

    void CPU::LD_SP(uint16_t pc, const OPCode& opcode)
    {
        const auto immediate = ram_->getImmediate16(pc + 1);
        const auto sp = SP();

        ram_->setImmediate16(immediate, sp);
    }

    void CPU::ADD(uint16_t pc, const OPCode& opcode)
    {
        if (opcode.operands().size() == 1)
        {
            const auto destOperand = opcode.operands()[0];
            const auto firstOperand = getOperand(opcode.operands()[0]);

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
                const auto secondValue = (int8_t) ram_->get(pc + 1);

                // TODO: check how half-carry and carry are working for mixed signed and unsigned addition.
                alu::AluResult<uint16_t> result;
                if (secondValue >= 0)
                    result = alu::add(firstValue, secondValue);
                else
                    result = alu::sub(firstValue, -secondValue);

                setOperand(destOperand, result.result);
                setFlagsFromResult(result.flags, opcode);
            
                return;
            }
        }
        else if (opcode.operands().size() == 2)
        {
            const auto destOperand = opcode.operands()[0];
            const auto firstOperand = getOperand(opcode.operands()[0]);
            const auto secondOperand = getOperand(opcode.operands()[1]);
            
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

        throw std::runtime_error("add not implemented for opcode " + toHexString(opcode.opcode()));
    }

    void CPU::RRCA(uint16_t pc, const OPCode& opcode)
    {
        const auto currentValue = A();
        const auto result = alu::rrc(currentValue);

        setA(result.result);
        setFlagsFromResult(result.flags, opcode);
    }

    void CPU::RLA(uint16_t pc, const OPCode& opcode)
    {
        const auto currentValue = A();
        const auto result = alu::rl(currentValue, FlagC());

        setA(result.result);
        setFlagsFromResult(result.flags, opcode);
    }

    void CPU::JR(uint16_t pc, const OPCode& opcode)
    {
        if (testJumpCondition(opcode.jumpCondition()))
        {
            const auto immediate = (int8_t) ram_->get(pc + 1);
            setPC(PC() + immediate);
            cycles_ += opcode.additionalCycles();
        }
    }

    void CPU::RRA(uint16_t pc, const OPCode& opcode)
    {
        const auto currentValue = A();
        const auto result = alu::rr(currentValue, FlagC());

        setA(result.result);
        setFlagsFromResult(result.flags, opcode);
    }

    void CPU::LDI(uint16_t pc, const OPCode& opcode)
    {
        if (opcode.operands().size() == 2)
        {
            const auto destOperand = opcode.operands()[0];
            const auto sourceOperand = opcode.operands()[1];
            if (std::holds_alternative<DereferencedFullRegister>(destOperand))
            {
                const auto dereferencedFullRegister = std::get<DereferencedFullRegister>(destOperand);
                setOperand(destOperand, getOperand(sourceOperand));

                const auto currentFullRegisterValue = std::get<uint16_t>(getOperand(dereferencedFullRegister.fullRegister));
                setOperand(dereferencedFullRegister.fullRegister, (uint16_t) (currentFullRegisterValue + 1));
                return;
            } 
            else if (std::holds_alternative<DereferencedFullRegister>(sourceOperand))
            {
                const auto dereferencedFullRegister = std::get<DereferencedFullRegister>(sourceOperand);
                setOperand(destOperand, getOperand(sourceOperand));

                const auto currentFullRegisterValue = std::get<uint16_t>(getOperand(dereferencedFullRegister.fullRegister));
                setOperand(dereferencedFullRegister.fullRegister, (uint16_t) (currentFullRegisterValue + 1));
                return;
            }
        }

        throw std::runtime_error("load+increment not implemented for opcode " + toHexString(opcode.opcode()));
    }

    void CPU::DAA(uint16_t pc, const OPCode& opcode)
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

    void CPU::CPL(uint16_t pc, const OPCode& opcode)
    {
        const auto accumulateValue = A();
        const auto result = alu::bit_cpl(accumulateValue);
        setA(result.result);
        setFlagsFromResult(result.flags, opcode);
    }

    void CPU::LDD(uint16_t pc, const OPCode& opcode)
    {
        if (opcode.operands().size() == 2)
        {
            const auto destOperand = opcode.operands()[0];
            const auto sourceOperand = opcode.operands()[1];
            if (std::holds_alternative<DereferencedFullRegister>(destOperand))
            {
                const auto dereferencedFullRegister = std::get<DereferencedFullRegister>(destOperand);
                setOperand(destOperand, getOperand(sourceOperand));

                const auto currentFullRegisterValue = std::get<uint16_t>(getOperand(dereferencedFullRegister.fullRegister));
                setOperand(dereferencedFullRegister.fullRegister, (uint16_t) (currentFullRegisterValue - 1));
                return;
            } 
            else if (std::holds_alternative<DereferencedFullRegister>(sourceOperand))
            {
                const auto dereferencedFullRegister = std::get<DereferencedFullRegister>(sourceOperand);
                setOperand(destOperand, getOperand(sourceOperand));

                const auto currentFullRegisterValue = std::get<uint16_t>(getOperand(dereferencedFullRegister.fullRegister));
                setOperand(dereferencedFullRegister.fullRegister, (uint16_t) (currentFullRegisterValue - 1));
                return;
            }
        }

        throw std::runtime_error("load+decrement not implemented for opcode " + toHexString(opcode.opcode()));
    }

    void CPU::SCF(uint16_t pc, const OPCode& opcode)
    {
        setFlagsFromResult(alu::AluFlagResult{}, opcode);
    }

    void CPU::CCF(uint16_t pc, const OPCode& opcode)
    {
        setFlagsFromResult(alu::AluFlagResult{}, opcode);
    }

    void CPU::ADC(uint16_t pc, const OPCode& opcode)
    {
        if (opcode.operands().size() == 1)
        {
            const auto destOperand = opcode.operands()[0];
            const auto firstOperand = getOperand(opcode.operands()[0]);

            const auto firstValue = std::get<uint8_t>(firstOperand);
            const auto secondValue = ram_->get(pc + 1);

            const auto result = alu::adc(firstValue, secondValue, FlagC());

            setOperand(destOperand, result.result);
            setFlagsFromResult(result.flags, opcode);
            
            return;
        }
        else if (opcode.operands().size() == 2)
        {
            const auto destOperand = opcode.operands()[0];
            const auto firstOperand = getOperand(opcode.operands()[0]);
            const auto secondOperand = getOperand(opcode.operands()[1]);
            
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

        throw std::runtime_error("adc not implemented for opcode " + toHexString(opcode.opcode()));
    }

    void CPU::SUB(uint16_t pc, const OPCode& opcode)
    {
        if (opcode.operands().size() == 1)
        {
            const auto destOperand = opcode.operands()[0];
            const auto firstOperand = getOperand(opcode.operands()[0]);

            const auto firstValue = std::get<uint8_t>(firstOperand);
            const auto secondValue = ram_->get(pc + 1);

            const auto result = alu::sub(firstValue, secondValue);

            setOperand(destOperand, result.result);
            setFlagsFromResult(result.flags, opcode);
            
            return;
        }
        else if (opcode.operands().size() == 2)
        {
            const auto destOperand = opcode.operands()[0];
            const auto firstOperand = getOperand(opcode.operands()[0]);
            const auto secondOperand = getOperand(opcode.operands()[1]);
            
            if (std::holds_alternative<uint8_t>(firstOperand) && std::holds_alternative<uint8_t>(secondOperand))
            {
                const auto firstValue = std::get<uint8_t>(firstOperand);
                const auto secondValue = std::get<uint8_t>(secondOperand);

                const auto result = alu::sub(firstValue, secondValue);

                setOperand(destOperand, result.result);
                setFlagsFromResult(result.flags, opcode);
                
                return;
            }
        }

        throw std::runtime_error("sub not implemented for opcode " + toHexString(opcode.opcode()));
    }

    void CPU::SBC(uint16_t pc, const OPCode& opcode)
    {
        if (opcode.operands().size() == 1)
        {
            const auto destOperand = opcode.operands()[0];
            const auto firstOperand = getOperand(opcode.operands()[0]);

            const auto firstValue = std::get<uint8_t>(firstOperand);
            const auto secondValue = ram_->get(pc + 1);

            const auto result = alu::sbc(firstValue, secondValue, FlagC());

            setOperand(destOperand, result.result);
            setFlagsFromResult(result.flags, opcode);
            
            return;
        }
        else if (opcode.operands().size() == 2)
        {
            const auto destOperand = opcode.operands()[0];
            const auto firstOperand = getOperand(opcode.operands()[0]);
            const auto secondOperand = getOperand(opcode.operands()[1]);
            
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

        throw std::runtime_error("sbc not implemented for opcode " + toHexString(opcode.opcode()));
    }
    
    void CPU::AND(uint16_t pc, const OPCode& opcode)
    {
        if (opcode.operands().size() == 1)
        {
            const auto destOperand = opcode.operands()[0];
            const auto firstOperand = getOperand(opcode.operands()[0]);

            const auto firstValue = std::get<uint8_t>(firstOperand);
            const auto secondValue = ram_->get(pc + 1);

            const auto result = alu::bit_and(firstValue, secondValue);

            setOperand(destOperand, result.result);
            setFlagsFromResult(result.flags, opcode);
            
            return;
        }
        else if (opcode.operands().size() == 2)
        {
            const auto destOperand = opcode.operands()[0];
            const auto firstOperand = getOperand(opcode.operands()[0]);
            const auto secondOperand = getOperand(opcode.operands()[1]);
            
            if (std::holds_alternative<uint8_t>(firstOperand) && std::holds_alternative<uint8_t>(secondOperand))
            {
                const auto firstValue = std::get<uint8_t>(firstOperand);
                const auto secondValue = std::get<uint8_t>(secondOperand);

                const auto result = alu::bit_and(firstValue, secondValue);

                setOperand(destOperand, result.result);
                setFlagsFromResult(result.flags, opcode);
                
                return;
            }
        }

        throw std::runtime_error("and not implemented for opcode " + toHexString(opcode.opcode()));
    }

    void CPU::XOR(uint16_t pc, const OPCode& opcode)
    {
        if (opcode.operands().size() == 1)
        {
            const auto destOperand = opcode.operands()[0];
            const auto firstOperand = getOperand(opcode.operands()[0]);

            const auto firstValue = std::get<uint8_t>(firstOperand);
            const auto secondValue = ram_->get(pc + 1);

            const auto result = alu::bit_xor(firstValue, secondValue);

            setOperand(destOperand, result.result);
            setFlagsFromResult(result.flags, opcode);
            
            return;
        }
        else if (opcode.operands().size() == 2)
        {
            const auto destOperand = opcode.operands()[0];
            const auto firstOperand = getOperand(opcode.operands()[0]);
            const auto secondOperand = getOperand(opcode.operands()[1]);
            
            if (std::holds_alternative<uint8_t>(firstOperand) && std::holds_alternative<uint8_t>(secondOperand))
            {
                const auto firstValue = std::get<uint8_t>(firstOperand);
                const auto secondValue = std::get<uint8_t>(secondOperand);

                const auto result = alu::bit_xor(firstValue, secondValue);

                setOperand(destOperand, result.result);
                setFlagsFromResult(result.flags, opcode);
                
                return;
            }
        }

        throw std::runtime_error("and not implemented for opcode " + toHexString(opcode.opcode()));
    }

    void CPU::OR(uint16_t pc, const OPCode& opcode)
    {
        if (opcode.operands().size() == 1)
        {
            const auto destOperand = opcode.operands()[0];
            const auto firstOperand = getOperand(opcode.operands()[0]);

            const auto firstValue = std::get<uint8_t>(firstOperand);
            const auto secondValue = ram_->get(pc + 1);

            const auto result = alu::bit_or(firstValue, secondValue);

            setOperand(destOperand, result.result);
            setFlagsFromResult(result.flags, opcode);
            
            return;
        }
        else if (opcode.operands().size() == 2)
        {
            const auto destOperand = opcode.operands()[0];
            const auto firstOperand = getOperand(opcode.operands()[0]);
            const auto secondOperand = getOperand(opcode.operands()[1]);
            
            if (std::holds_alternative<uint8_t>(firstOperand) && std::holds_alternative<uint8_t>(secondOperand))
            {
                const auto firstValue = std::get<uint8_t>(firstOperand);
                const auto secondValue = std::get<uint8_t>(secondOperand);

                const auto result = alu::bit_or(firstValue, secondValue);

                setOperand(destOperand, result.result);
                setFlagsFromResult(result.flags, opcode);
                
                return;
            }
        }

        throw std::runtime_error("or not implemented for opcode " + toHexString(opcode.opcode()));
    }

    void CPU::CP(uint16_t pc, const OPCode& opcode)
    {
        if (opcode.operands().size() == 1)
        {
            const auto firstOperand = getOperand(opcode.operands()[0]);

            const auto firstValue = std::get<uint8_t>(firstOperand);
            const auto secondValue = ram_->get(pc + 1);

            const auto result = alu::sub(firstValue, secondValue);

            setFlagsFromResult(result.flags, opcode);
            
            return;
        }
        else if (opcode.operands().size() == 2)
        {
            const auto firstOperand = getOperand(opcode.operands()[0]);
            const auto secondOperand = getOperand(opcode.operands()[1]);
            
            if (std::holds_alternative<uint8_t>(firstOperand) && std::holds_alternative<uint8_t>(secondOperand))
            {
                const auto firstValue = std::get<uint8_t>(firstOperand);
                const auto secondValue = std::get<uint8_t>(secondOperand);

                const auto result = alu::sub(firstValue, secondValue);

                setFlagsFromResult(result.flags, opcode);
                
                return;
            }
        }

        throw std::runtime_error("cp not implemented for opcode " + toHexString(opcode.opcode()));
    }

    void CPU::RET(uint16_t pc, const OPCode& opcode)
    {
        if (testJumpCondition(opcode.jumpCondition()))
        {
            const auto immediate = popFromStack();
            setPC(immediate);
            cycles_ += opcode.additionalCycles();
        }
    }

    void CPU::POP(uint16_t pc, const OPCode& opcode)
    {
        const auto destOperand = opcode.operands()[0];

        if (std::holds_alternative<FullRegister>(destOperand))
        {
            const auto stackValue = popFromStack();
            setOperand(destOperand, stackValue);
            return;
        }

        throw std::runtime_error("pop not implemented for opcode " + toHexString(opcode.opcode()));
    }

    void CPU::JP(uint16_t pc, const OPCode& opcode)
    {
        if (opcode.operands().size() == 0)
        {
            if (testJumpCondition(opcode.jumpCondition()))
            {
                const auto immediate = ram_->getImmediate16(pc + 1);
                setPC(immediate);
                cycles_ += opcode.additionalCycles();
            }
            return;
        }
        else if (opcode.operands().size() == 1)
        {
            if (testJumpCondition(opcode.jumpCondition()))
            {
                const auto addressOperand = opcode.operands()[0];
                const auto newAddress = std::get<uint16_t>(getOperand(addressOperand));
                setPC(newAddress);
                cycles_ += opcode.additionalCycles();
            }
            return;
        }

        throw std::runtime_error("jump not implemented for opcode " + toHexString(opcode.opcode()));
    }

    void CPU::CALL(uint16_t pc, const OPCode& opcode)
    {
        if (testJumpCondition(opcode.jumpCondition()))
        {
            const auto immediate = ram_->getImmediate16(pc + 1);
            pushToStack(PC());
            setPC(immediate);
            cycles_ += opcode.additionalCycles();
        }
    }

     void CPU::PUSH(uint16_t pc, const OPCode& opcode)
    {
        const auto operand = opcode.operands()[0];
        const auto operandValue = getOperand(operand);

        if (std::holds_alternative<uint16_t>(operandValue))
        {
            const auto stackValue = std::get<uint16_t>(operandValue);
            pushToStack(stackValue);
            return;
        }

        throw std::runtime_error("push not implemented for opcode " + toHexString(opcode.opcode()));
    }

    void CPU::RST(uint16_t pc, const OPCode& opcode)
    {
        const auto auxArg = opcode.auxiliaryArguments()[0];
        if (auxArg > 7)
            throw std::runtime_error("rst not implemented for opcode " + toHexString(opcode.opcode()));

        pushToStack(PC());
        const uint16_t rstAddress = concatBytes(0x00, auxArg << 3);
        setPC(rstAddress);
    }

    void CPU::RETI(uint16_t pc, const OPCode& opcode)
    {
        if (testJumpCondition(opcode.jumpCondition()))
        {
            const auto immediate = popFromStack();
            setPC(immediate);
            IME_ = true;
            cycles_ += opcode.additionalCycles();
        }
    }

    void CPU::LDff8(uint16_t pc, const OPCode& opcode)
    {
        if (opcode.operands().size() == 1)
        {
            const auto srcOperand = opcode.operands()[0];
            if (std::holds_alternative<Register>(srcOperand))
            {
                const auto lowerByte = ram_->get(pc + 1);
                const auto address = concatBytes(0xff, lowerByte);
                const auto value = std::get<uint8_t>(getOperand(srcOperand));
                ram_->set(address, value);
                return;
            }
        }
        else if (opcode.operands().size() == 2)
        {
            const auto addressOperand = opcode.operands()[0];
            const auto srcOperand = opcode.operands()[1];
            if (std::holds_alternative<Register>(addressOperand) && std::holds_alternative<Register>(srcOperand))
            {
                const auto lowerByte = std::get<uint8_t>(getOperand(addressOperand));
                const auto address = concatBytes(0xff, lowerByte);
                const auto value = std::get<uint8_t>(getOperand(srcOperand));
                ram_->set(address, value);
                return;
            }
        }

        throw std::runtime_error("ldff8 not implemented for opcode " + toHexString(opcode.opcode()));
    }

    void CPU::LDa16(uint16_t pc, const OPCode& opcode)
    {
        if (opcode.operands().size() == 1)
        {
            const auto srcOperand = opcode.operands()[0];
            if (std::holds_alternative<Register>(srcOperand))
            {
                const auto address = ram_->getImmediate16(pc + 1);
                const auto value = std::get<uint8_t>(getOperand(srcOperand));
                ram_->set(address, value);
                return;
            }
        }

        throw std::runtime_error("lda16 not implemented for opcode " + toHexString(opcode.opcode()));
    }

    void CPU::LDaff8(uint16_t pc, const OPCode& opcode)
    {
        if (opcode.operands().size() == 1)
        {
            const auto destOperand = opcode.operands()[0];
            if (std::holds_alternative<Register>(destOperand))
            {
                const auto lowerByte = ram_->get(pc + 1);
                const auto address = concatBytes(0xff, lowerByte);
                const auto value = ram_->get(address);
                setOperand(destOperand, value);
                return;
            }
        }
        else if (opcode.operands().size() == 2)
        {
            const auto destOperand = opcode.operands()[0];
            const auto addressOperand = opcode.operands()[1];
            if (std::holds_alternative<Register>(destOperand) && std::holds_alternative<Register>(addressOperand))
            {
                const auto lowerByte = std::get<uint8_t>(getOperand(addressOperand));
                const auto address = concatBytes(0xff, lowerByte);
                const auto value = ram_->get(address);
                setOperand(destOperand, value);
                return;
            }
        }

        throw std::runtime_error("ldaff8 not implemented for opcode " + toHexString(opcode.opcode()));
    }

    void CPU::DI(uint16_t pc, const OPCode& opcode)
    {
        IME_ = false;
    }

    void CPU::LDaff16(uint16_t pc, const OPCode& opcode)
    {
        if (opcode.operands().size() == 1)
        {
            const auto destOperand = opcode.operands()[0];
            if (std::holds_alternative<Register>(destOperand))
            {
                const auto address = ram_->getImmediate16(pc + 1);
                const auto value = ram_->get(address);
                setOperand(destOperand, value);
                return;
            }
        }

        throw std::runtime_error("ldaff16 not implemented for opcode " + toHexString(opcode.opcode()));
    }

    void CPU::EI(uint16_t pc, const OPCode& opcode)
    {
        IME_ = true;
    }

    void CPU::RR(uint16_t pc, const OPCode& opcode)
    {
        const auto operand = opcode.operands()[0];

        if (std::holds_alternative<Register>(operand))
        {
            const auto currentValue = std::get<uint8_t>(getOperand(operand));
            const auto result = alu::rr(currentValue, FlagC());

            setOperand(operand, result.result);
            setFlagsFromResult(result.flags, opcode);

            return;
        }

        throw std::runtime_error("sla not implemented for opcode " + toHexString(opcode.opcode()));
    }

    void CPU::SLA(uint16_t pc, const OPCode& opcode)
    {
        const auto operand = opcode.operands()[0];

        if (std::holds_alternative<Register>(operand))
        {
            const auto currentValue = std::get<uint8_t>(getOperand(operand));
            const auto result = alu::bit_sla(currentValue);

            setOperand(operand, result.result);
            setFlagsFromResult(result.flags, opcode);

            return;
        }

        throw std::runtime_error("sla not implemented for opcode " + toHexString(opcode.opcode()));
    }

    void CPU::SWAP(uint16_t pc, const OPCode& opcode)
    {
        const auto operand = opcode.operands()[0];

        if (std::holds_alternative<Register>(operand))
        {
            const auto currentValue = std::get<uint8_t>(getOperand(operand));
            const auto result = alu::bit_swap(currentValue);

            setOperand(operand, result.result);
            setFlagsFromResult(result.flags, opcode);

            return;
        }

        throw std::runtime_error("swap not implemented for opcode " + toHexString(opcode.opcode()));
    }

    void CPU::SRL(uint16_t pc, const OPCode& opcode)
    {
        const auto operand = opcode.operands()[0];

        if (std::holds_alternative<Register>(operand))
        {
            const auto currentValue = std::get<uint8_t>(getOperand(operand));
            const auto result = alu::bit_srl(currentValue);

            setOperand(operand, result.result);
            setFlagsFromResult(result.flags, opcode);

            return;
        }

        throw std::runtime_error("sla not implemented for opcode " + toHexString(opcode.opcode()));
    }

    void CPU::BIT_GET(uint16_t pc, const OPCode& opcode)
    {
        const auto operand = opcode.operands()[0];
        const auto bitIndex = opcode.auxiliaryArguments()[0];

        if (std::holds_alternative<Register>(operand))
        {
            const auto currentValue = std::get<uint8_t>(getOperand(operand));
            const auto result = alu::bit_get(currentValue, bitIndex);

            setFlagsFromResult(result.flags, opcode);

            return;
        }

        throw std::runtime_error("bit_set not implemented for opcode " + toHexString(opcode.opcode()));
    }

    void CPU::BIT_SET(uint16_t pc, const OPCode& opcode)
    {
        const auto operand = opcode.operands()[0];
        const auto bitIndex = opcode.auxiliaryArguments()[0];
        const auto newBit = opcode.auxiliaryArguments()[1];

        if (std::holds_alternative<Register>(operand) || std::holds_alternative<DereferencedFullRegister>(operand))
        {
            const auto currentValue = std::get<uint8_t>(getOperand(operand));
            const auto result = alu::bit_set(currentValue, bitIndex, newBit);

            setOperand(operand, result.result);
            setFlagsFromResult(result.flags, opcode);

            return;
        }

        throw std::runtime_error("bit_set not implemented for opcode " + toHexString(opcode.opcode()));
    }

} // gbemu