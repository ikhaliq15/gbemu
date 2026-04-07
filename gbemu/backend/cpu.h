#ifndef GBEMU_BACKEND_CPU_H
#define GBEMU_BACKEND_CPU_H

#include "gbemu/backend/alu.h"
#include "gbemu/backend/opcode.h"
#include "gbemu/backend/opcode_data.h"
#include "gbemu/backend/operand.h"
#include "gbemu/backend/ram.h"

#include <cstdint>

namespace gbemu::backend
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

    CPU(RAM *ram);
    CPU(const CPU &cpu);

    [[nodiscard]] bool IME() const;

    [[nodiscard]] uint16_t PC() const;
    [[nodiscard]] uint16_t SP() const;

    [[nodiscard]] uint16_t AF() const; // TODO: remove?
    [[nodiscard]] uint16_t BC() const;
    [[nodiscard]] uint16_t DE() const;
    [[nodiscard]] uint16_t HL() const;

    [[nodiscard]] uint8_t A() const;
    [[nodiscard]] uint8_t B() const;
    [[nodiscard]] uint8_t C() const;
    [[nodiscard]] uint8_t D() const;
    [[nodiscard]] uint8_t E() const;
    [[nodiscard]] uint8_t H() const;
    [[nodiscard]] uint8_t L() const;

    [[nodiscard]] uint8_t FlagZ() const;
    [[nodiscard]] uint8_t FlagN() const;
    [[nodiscard]] uint8_t FlagH() const;
    [[nodiscard]] uint8_t FlagC() const;

    [[nodiscard]] RAM *ram() const;

    [[nodiscard]] uint64_t cycles() const;
    [[nodiscard]] Mode mode() const;
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

    [[nodiscard]] uint8_t getRegister(Register reg) const;
    [[nodiscard]] uint16_t getFullRegister(FullRegister reg) const;

    void setRegister(Register reg, uint8_t newRegVal);
    void setFullRegister(FullRegister reg, uint16_t newRegVal);

    void serviceInterrupts();

    void executeInstruction(bool verbose = false);

    bool operator==(const CPU &rhs) const;

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
    bool interruptsEnabledQueued_ = false;

    uint16_t PC_;
    uint16_t SP_;

    uint16_t AF_;
    uint16_t BC_;
    uint16_t DE_;
    uint16_t HL_;

    RAM *ram_;

    uint64_t cycles_;
    Mode mode_;

    /*** Constexpr handler dispatch ***/
    using OPCodeHandler = void (CPU::*)(uint16_t, const OPCode *);
    using OPCodeHandlerMap = std::array<OPCodeHandler, 256>;

    [[nodiscard]] OperandValue getOperand(Operand operand) const;
    void setOperand(Operand operand, OperandValue newValue);

    void setFlagsFromResult(const alu::AluFlagResult &flagResult, const OPCode *opcode);
    [[nodiscard]] bool testJumpCondition(OPCode::JumpCondition jumpCondition) const;

    /*** Templated ALU dispatch — replaces 23 individual handler methods ***/

    // Unary: op(val) -> result, store to operand (RLC, RRC, SLA, SRA, SWAP, SRL, CPL-as-accum)
    template <auto Operation> void alu_unary(uint16_t pc, const OPCode *opcode)
    {
        const auto operand = opcode->operands[0];
        const auto result = Operation(getOperand(operand).as8());
        setOperand(operand, result.result);
        setFlagsFromResult(result.flags, opcode);
    }

    // Unary with carry: op(val, FlagC()) -> result, store to operand (RL, RR)
    template <auto Operation> void alu_unary_carry(uint16_t pc, const OPCode *opcode)
    {
        const auto operand = opcode->operands[0];
        const auto result = Operation(getOperand(operand).as8(), FlagC());
        setOperand(operand, result.result);
        setFlagsFromResult(result.flags, opcode);
    }

    // Binary: op(a, b) -> result, store to dest. Handles 1-operand (immediate) and 2-operand forms.
    template <auto Operation, bool StoreResult = true> void alu_binary(uint16_t pc, const OPCode *opcode)
    {
        const auto dest = opcode->operands[0];
        const auto a = getOperand(dest).as8();
        const auto b = opcode->numOperands == 2 ? getOperand(opcode->operands[1]).as8() : ram_->get(pc + 1);
        const auto result = Operation(a, b);
        if constexpr (StoreResult)
            setOperand(dest, result.result);
        setFlagsFromResult(result.flags, opcode);
    }

    // Binary with carry: op(a, b, FlagC()) -> result (ADC, SBC)
    template <auto Operation> void alu_binary_carry(uint16_t pc, const OPCode *opcode)
    {
        const auto dest = opcode->operands[0];
        const auto a = getOperand(dest).as8();
        const auto b = opcode->numOperands == 2 ? getOperand(opcode->operands[1]).as8() : ram_->get(pc + 1);
        const auto result = Operation(a, b, FlagC());
        setOperand(dest, result.result);
        setFlagsFromResult(result.flags, opcode);
    }

    // Accumulator unary: op(A()) -> store to A (RLCA, RRCA, CPL)
    template <auto Operation> void alu_accum(uint16_t pc, const OPCode *opcode)
    {
        const auto result = Operation(A());
        setA(result.result);
        setFlagsFromResult(result.flags, opcode);
    }

    // Accumulator unary with carry: op(A(), FlagC()) -> store to A (RLA, RRA)
    template <auto Operation> void alu_accum_carry(uint16_t pc, const OPCode *opcode)
    {
        const auto result = Operation(A(), FlagC());
        setA(result.result);
        setFlagsFromResult(result.flags, opcode);
    }

    // INC/DEC: handles both 8-bit and 16-bit operands
    enum class IncDecDir
    {
        Inc,
        Dec
    };
    template <IncDecDir Dir> void inc_dec(uint16_t pc, const OPCode *opcode)
    {
        const auto operand = opcode->operands[0];
        const auto val = getOperand(operand);
        if (val.is8bit())
        {
            const auto r = Dir == IncDecDir::Inc ? alu::add(val.as8(), static_cast<uint8_t>(1))
                                                 : alu::sub(val.as8(), static_cast<uint8_t>(1));
            setOperand(operand, r.result);
            setFlagsFromResult(r.flags, opcode);
        }
        else
        {
            const auto r = Dir == IncDecDir::Inc ? alu::add(val.as16(), static_cast<uint16_t>(1))
                                                 : alu::sub(val.as16(), static_cast<uint16_t>(1));
            setOperand(operand, r.result);
            setFlagsFromResult(r.flags, opcode);
        }
    }

    // LDI/LDD: load then adjust the dereferenced register by Offset
    template <int16_t Offset> void load_and_adjust(uint16_t pc, const OPCode *opcode)
    {
        const auto dest = opcode->operands[0];
        const auto src = opcode->operands[1];
        setOperand(dest, getOperand(src));
        const auto derefOp = dest.isDereferencedFullRegister() ? dest : src;
        const auto fullReg = derefOp.asDereferencedFullRegister();
        setFullRegister(fullReg, static_cast<uint16_t>(getFullRegister(fullReg) + Offset));
    }

    // SCF/CCF: only set flags, no ALU computation
    void flags_only(uint16_t pc, const OPCode *opcode) { setFlagsFromResult(alu::AluFlagResult{}, opcode); }

    /*** Remaining named handlers (unique logic, not worth templating) ***/
    void UNIMPLEMENTED(uint16_t pc, const OPCode *opcode);
    void NOP(uint16_t pc, const OPCode *opcode);
    void LD(uint16_t pc, const OPCode *opcode);
    void LD_SP(uint16_t pc, const OPCode *opcode);
    void ADD(uint16_t pc, const OPCode *opcode);
    void DAA(uint16_t pc, const OPCode *opcode);
    void HALT(uint16_t pc, const OPCode *opcode);
    void RET(uint16_t pc, const OPCode *opcode);
    void POP(uint16_t pc, const OPCode *opcode);
    void JP(uint16_t pc, const OPCode *opcode);
    void JR(uint16_t pc, const OPCode *opcode);
    void CALL(uint16_t pc, const OPCode *opcode);
    void PUSH(uint16_t pc, const OPCode *opcode);
    void RST(uint16_t pc, const OPCode *opcode);
    void RETI(uint16_t pc, const OPCode *opcode);
    void LDff8(uint16_t pc, const OPCode *opcode);
    void LDa16(uint16_t pc, const OPCode *opcode);
    void LDaff8(uint16_t pc, const OPCode *opcode);
    void LDaff16(uint16_t pc, const OPCode *opcode);
    void LDs8(uint16_t pc, const OPCode *opcode);
    void DI(uint16_t pc, const OPCode *opcode);
    void EI(uint16_t pc, const OPCode *opcode);
    void BIT_GET(uint16_t pc, const OPCode *opcode);
    void BIT_SET(uint16_t pc, const OPCode *opcode);

    static constexpr std::pair<std::string_view, OPCodeHandler> kHandlers[] = {
        // ALU: templated dispatch
        {"SUB", &CPU::alu_binary<&alu::sub<uint8_t, uint8_t>>},
        {"AND", &CPU::alu_binary<&alu::bit_and>},
        {"XOR", &CPU::alu_binary<&alu::bit_xor>},
        {"OR", &CPU::alu_binary<&alu::bit_or>},
        {"CP", &CPU::alu_binary<&alu::sub<uint8_t, uint8_t>, false>},
        {"ADC", &CPU::alu_binary_carry<&alu::adc>},
        {"SBC", &CPU::alu_binary_carry<&alu::sbc>},
        {"RLC", &CPU::alu_unary<&alu::rlc>},
        {"RRC", &CPU::alu_unary<&alu::rrc>},
        {"SLA", &CPU::alu_unary<&alu::bit_sla>},
        {"SRA", &CPU::alu_unary<&alu::bit_sra>},
        {"SRL", &CPU::alu_unary<&alu::bit_srl>},
        {"SWAP", &CPU::alu_unary<&alu::bit_swap>},
        {"RL", &CPU::alu_unary_carry<&alu::rl>},
        {"RR", &CPU::alu_unary_carry<&alu::rr>},
        {"RLCA", &CPU::alu_accum<&alu::rlc>},
        {"RRCA", &CPU::alu_accum<&alu::rrc>},
        {"CPL", &CPU::alu_accum<&alu::bit_cpl>},
        {"RLA", &CPU::alu_accum_carry<&alu::rl>},
        {"RRA", &CPU::alu_accum_carry<&alu::rr>},
        {"INC", &CPU::inc_dec<IncDecDir::Inc>},
        {"DEC", &CPU::inc_dec<IncDecDir::Dec>},
        {"LDI", &CPU::load_and_adjust<+1>},
        {"LDD", &CPU::load_and_adjust<-1>},
        {"SCF", &CPU::flags_only},
        {"CCF", &CPU::flags_only},

        // Named handlers
        {"ADD", &CPU::ADD},
        {"BIT_GET", &CPU::BIT_GET},
        {"BIT_SET", &CPU::BIT_SET},
        {"CALL", &CPU::CALL},
        {"DAA", &CPU::DAA},
        {"DI", &CPU::DI},
        {"EI", &CPU::EI},
        {"HALT", &CPU::HALT},
        {"JP", &CPU::JP},
        {"JR", &CPU::JR},
        {"LD", &CPU::LD},
        {"LDa16", &CPU::LDa16},
        {"LDaff16", &CPU::LDaff16},
        {"LDaff8", &CPU::LDaff8},
        {"LDff8", &CPU::LDff8},
        {"LDs8", &CPU::LDs8},
        {"LD_SP", &CPU::LD_SP},
        {"NOP", &CPU::NOP},
        {"POP", &CPU::POP},
        {"PUSH", &CPU::PUSH},
        {"RET", &CPU::RET},
        {"RETI", &CPU::RETI},
        {"RST", &CPU::RST},
    };

    static constexpr OPCodeHandler lookupHandler(std::string_view mnemonic)
    {
        for (const auto &[name, handler] : kHandlers)
        {
            if (name == mnemonic)
                return handler;
        }
        return &CPU::UNIMPLEMENTED;
    }

    static constexpr OPCodeHandlerMap buildHandlerMap(const std::array<OPCode, 256> &opcodes)
    {
        OPCodeHandlerMap map{};
        for (size_t i = 0; i < 256; i++)
        {
            map[i] = opcodes[i].valid ? lookupHandler(opcodes[i].mnemonic) : &CPU::UNIMPLEMENTED;
        }
        return map;
    }

    static const OPCodeHandlerMap opcodeFunctions_;
    static const OPCodeHandlerMap prefixedOpcodeFunctions_;
};

inline constexpr CPU::OPCodeHandlerMap CPU::opcodeFunctions_ = CPU::buildHandlerMap(OPCODES);
inline constexpr CPU::OPCodeHandlerMap CPU::prefixedOpcodeFunctions_ = CPU::buildHandlerMap(PREFIXED_OPCODES);

} // namespace gbemu::backend

#endif // GBEMU_BACKEND_CPU_H
