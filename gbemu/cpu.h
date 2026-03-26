#ifndef GBEMU_CPU
#define GBEMU_CPU

#include "alu.h"
#include "opcode.h"
#include "operand.h"
#include "ram.h"

#include <functional>
#include <memory>
#include <stdint.h>

namespace gbemu
{

class CPU
{
  public:
    enum class Interrupt
    {
        VBLANK,
        STAT,
        TIMER,
    };

    enum class Mode
    {
        NORMAL,
        HALT,
    };

    CPU(std::shared_ptr<RAM> ram, const std::string &opcodeDataFile);
    CPU(const CPU &cpu);

    bool IME() const;

    uint16_t PC() const;
    uint16_t SP() const;

    uint16_t AF() const; // TODO: remove?
    uint16_t BC() const;
    uint16_t DE() const;
    uint16_t HL() const;

    uint8_t A() const;
    uint8_t B() const;
    uint8_t C() const;
    uint8_t D() const;
    uint8_t E() const;
    uint8_t H() const;
    uint8_t L() const;

    uint8_t FlagZ() const;
    uint8_t FlagN() const;
    uint8_t FlagH() const;
    uint8_t FlagC() const;

    std::shared_ptr<RAM> ram() const;

    uint64_t cycles() const;
    Mode mode() const;
    void setMode(Mode mode);

    void setIME(bool newIME);

    void setPC(uint16_t newRegVal);
    void setSP(uint16_t newRegVal);

    void setAF(uint16_t newRegVal); // TODO: remove?
    void setBC(uint16_t newRegVal);
    void setDE(uint16_t newRegVal);
    void setHL(uint16_t newRegVal);

    void setA(uint8_t newRegVal);
    void setB(uint8_t newRegVal);
    void setC(uint8_t newRegVal);
    void setD(uint8_t newRegVal);
    void setE(uint8_t newRegVal);
    void setH(uint8_t newRegVal);
    void setL(uint8_t newRegVal);

    void setFlagZ(uint8_t newFlagVal);
    void setFlagN(uint8_t newFlagVal);
    void setFlagH(uint8_t newFlagVal);
    void setFlagC(uint8_t newFlagVal);
    void setFlags(uint8_t newZ, uint8_t newN, uint8_t newH, uint8_t newC);

    void advancePC(uint16_t inc);
    void offsetSP(int32_t offset);

    void pushToStack(uint16_t val);
    uint16_t popFromStack();

    uint8_t getRegister(Register reg) const;
    uint16_t getFullRegister(FullRegister reg) const;

    void setRegister(Register reg, uint8_t newRegVal);
    void setFullRegister(FullRegister reg, uint16_t newRegVal);

    void requestInterupt(Interrupt interrupt);

    void executeInstruction(bool verbose = false);

    bool operator==(const CPU &rhs) const;
    bool operator!=(const CPU &rhs) const;

    friend std::ostream &operator<<(std::ostream &os, const CPU &cpu);

  private:
    // TODO: find proper starting values for registers (and fix tests to be agnostic to starting values).
    static constexpr uint16_t STARTING_AF = 0x01B0;
    static constexpr uint16_t STARTING_BC = 0x0013;
    static constexpr uint16_t STARTING_DE = 0x00D8;
    static constexpr uint16_t STARTING_HL = 0x014D;

    static constexpr uint16_t STARTING_PC = 0x0100;
    static constexpr uint16_t STARTING_SP = 0xFFFE;

    static constexpr uint8_t FLAG_Z_BIT = 7;
    static constexpr uint8_t FLAG_N_BIT = 6;
    static constexpr uint8_t FLAG_H_BIT = 5;
    static constexpr uint8_t FLAG_C_BIT = 4;

    bool IME_;
    bool interuptsEnabledQueued_ = false;

    uint16_t PC_;
    uint16_t SP_;

    uint16_t AF_;
    uint16_t BC_;
    uint16_t DE_;
    uint16_t HL_;

    std::shared_ptr<RAM> ram_;

    uint64_t cycles_;
    Mode mode_;

    using OPCodeHandler = std::function<void(uint16_t, const OPCode &)>;
    std::map<uint8_t, OPCode> opcodes_;
    std::map<uint8_t, OPCode> prefixedOpcodes_;
    std::unordered_map<std::string, OPCodeHandler> opcodeFunctions_;

    OperandValue getOperand(Operand operand) const;
    void setOperand(Operand operand, OperandValue newValue);

    void setFlagsFromResult(const alu::AluFlagResult &flagResult, const OPCode &opcode);
    bool testJumpCondition(OPCode::JumpCondition jumpCondition) const;

    /*** OPCode Handlers ***/
    void NOP(uint16_t pc, const OPCode &opcode);
    void LD(uint16_t pc, const OPCode &opcode);
    void INC(uint16_t pc, const OPCode &opcode);
    void DEC(uint16_t pc, const OPCode &opcode);
    void RLCA(uint16_t pc, const OPCode &opcode);
    void LD_SP(uint16_t pc, const OPCode &opcode);
    void ADD(uint16_t pc, const OPCode &opcode);
    void RRCA(uint16_t pc, const OPCode &opcode);
    void RLA(uint16_t pc, const OPCode &opcode);
    void JR(uint16_t pc, const OPCode &opcode);
    void RRA(uint16_t pc, const OPCode &opcode);
    void LDI(uint16_t pc, const OPCode &opcode);
    void DAA(uint16_t pc, const OPCode &opcode);
    void CPL(uint16_t pc, const OPCode &opcode);
    void LDD(uint16_t pc, const OPCode &opcode);
    void SCF(uint16_t pc, const OPCode &opcode);
    void CCF(uint16_t pc, const OPCode &opcode);
    void HALT(uint16_t pc, const OPCode &opcode);
    void ADC(uint16_t pc, const OPCode &opcode);
    void SUB(uint16_t pc, const OPCode &opcode);
    void SBC(uint16_t pc, const OPCode &opcode);
    void AND(uint16_t pc, const OPCode &opcode);
    void XOR(uint16_t pc, const OPCode &opcode);
    void OR(uint16_t pc, const OPCode &opcode);
    void CP(uint16_t pc, const OPCode &opcode);
    void RET(uint16_t pc, const OPCode &opcode);
    void POP(uint16_t pc, const OPCode &opcode);
    void JP(uint16_t pc, const OPCode &opcode);
    void CALL(uint16_t pc, const OPCode &opcode);
    void PUSH(uint16_t pc, const OPCode &opcode);
    void RST(uint16_t pc, const OPCode &opcode);
    void RETI(uint16_t pc, const OPCode &opcode);
    void LDff8(uint16_t pc, const OPCode &opcode);
    void LDa16(uint16_t pc, const OPCode &opcode);
    void LDaff8(uint16_t pc, const OPCode &opcode);
    void DI(uint16_t pc, const OPCode &opcode);
    void LDaff16(uint16_t pc, const OPCode &opcode);
    void LDs8(uint16_t pc, const OPCode &opcode);
    void EI(uint16_t pc, const OPCode &opcode);

    void RLC(uint16_t pc, const OPCode &opcode);
    void RRC(uint16_t pc, const OPCode &opcode);
    void RL(uint16_t pc, const OPCode &opcode);
    void RR(uint16_t pc, const OPCode &opcode);
    void SLA(uint16_t pc, const OPCode &opcode);
    void SRA(uint16_t pc, const OPCode &opcode);
    void SWAP(uint16_t pc, const OPCode &opcode);
    void SRL(uint16_t pc, const OPCode &opcode);
    void BIT_GET(uint16_t pc, const OPCode &opcode);
    void BIT_SET(uint16_t pc, const OPCode &opcode);
};

} // namespace gbemu

#endif // GBEMU_CPU