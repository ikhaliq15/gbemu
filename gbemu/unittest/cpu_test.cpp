#include <gtest/gtest.h>

#include "gbemu/cpu.h"

class CPUTest : public testing::Test
{
  protected:
    using Instruction = std::vector<uint8_t>;
    using Program = std::vector<Instruction>;
    using ExpectedCPUs = std::vector<std::optional<gbemu::CPU>>;

    void SetUp() override
    {
        ram_ = std::make_shared<gbemu::RAM>(1 << 16);
        cpu_ = std::make_unique<gbemu::CPU>(ram_);
    }

    void loadSimpleProgram(const Program &program)
    {
        int ramIdx = 0x0100;
        for (const auto &instruction : program)
        {
            for (const auto &instructionByte : instruction)
            {
                ram_->set(ramIdx, instructionByte);
                ramIdx++;
            }
        }
    }

    void executeInstructions(size_t numInstructions)
    {
        for (int i = 0; i < numInstructions; i++)
            cpu_->executeInstruction();
    }

    void assertCPURegularRegistersAndRamEqual(const gbemu::CPU &cpu1, const gbemu::CPU &cpu2)
    {
        ASSERT_EQ(cpu1.AF(), cpu2.AF());
        ASSERT_EQ(cpu1.BC(), cpu2.BC());
        ASSERT_EQ(cpu1.DE(), cpu2.DE());
        ASSERT_EQ(cpu1.HL(), cpu2.HL());
        ASSERT_EQ(cpu1.A(), cpu2.A());
        ASSERT_EQ(cpu1.B(), cpu2.B());
        ASSERT_EQ(cpu1.C(), cpu2.C());
        ASSERT_EQ(cpu1.D(), cpu2.D());
        ASSERT_EQ(cpu1.E(), cpu2.E());
        ASSERT_EQ(*cpu1.ram(), *cpu2.ram());
    }

    void assertCPUPCEqual(const gbemu::CPU &cpu1, const gbemu::CPU &cpu2)
    {
        ASSERT_EQ(cpu1.PC(), cpu2.PC());
    }

    void runProgramAndCompareRegistersAndRam(const Program &program, const ExpectedCPUs &expectedCPUs,
                                             bool includePC = false)
    {
        ASSERT_EQ(program.size(), expectedCPUs.size())
            << "Length of expectedCPUs must equal number of instructions in program.";

        loadSimpleProgram(program);
        for (int i = 0; i < program.size(); i++)
        {
            cpu_->executeInstruction(true);
            const auto expectedCPU = expectedCPUs[i];
            if (expectedCPU)
            {
                ASSERT_EQ(cpu_->IME(), expectedCPU->IME());
                assertCPURegularRegistersAndRamEqual(*cpu_, expectedCPU.value());
                if (includePC)
                    assertCPUPCEqual(*cpu_, expectedCPU.value());
            }
        }
    }

    std::shared_ptr<gbemu::RAM> ram_;
    std::unique_ptr<gbemu::CPU> cpu_;
};

TEST_F(CPUTest, DefaultRegisterValues)
{
    ASSERT_EQ(cpu_->PC(), 0x0100);
    ASSERT_EQ(cpu_->SP(), 0xFFFE);
}

TEST_F(CPUTest, SettingAndGettingFullRegisters)
{
    const uint16_t AF_VALUE = 0x1234;
    const uint16_t BC_VALUE = 0x5678;
    const uint16_t DE_VALUE = 0x5249;
    const uint16_t HL_VALUE = 0x7314;

    cpu_->setAF(AF_VALUE);
    cpu_->setBC(BC_VALUE);
    cpu_->setDE(DE_VALUE);
    cpu_->setHL(HL_VALUE);

    ASSERT_EQ(cpu_->AF(), AF_VALUE);
    ASSERT_EQ(cpu_->BC(), BC_VALUE);
    ASSERT_EQ(cpu_->DE(), DE_VALUE);
    ASSERT_EQ(cpu_->HL(), HL_VALUE);
}

TEST_F(CPUTest, SettingAndGettingHalfRegisters)
{
    const uint8_t A_VALUE = 0x12;
    const uint8_t B_VALUE = 0x34;
    const uint8_t C_VALUE = 0x56;
    const uint8_t D_VALUE = 0x78;
    const uint8_t E_VALUE = 0x90;
    const uint8_t H_VALUE = 0x74;
    const uint8_t L_VALUE = 0xb2;

    cpu_->setA(A_VALUE);
    cpu_->setB(B_VALUE);
    cpu_->setC(C_VALUE);
    cpu_->setD(D_VALUE);
    cpu_->setE(E_VALUE);
    cpu_->setH(H_VALUE);
    cpu_->setL(L_VALUE);

    ASSERT_EQ(cpu_->A(), A_VALUE);
    ASSERT_EQ(cpu_->B(), B_VALUE);
    ASSERT_EQ(cpu_->C(), C_VALUE);
    ASSERT_EQ(cpu_->D(), D_VALUE);
    ASSERT_EQ(cpu_->E(), E_VALUE);
    ASSERT_EQ(cpu_->H(), H_VALUE);
    ASSERT_EQ(cpu_->L(), L_VALUE);
}

TEST_F(CPUTest, SettingAndGettingCombinationOfFullAndRegisters)
{
    cpu_->setA(0x31);
    cpu_->setB(0x41);
    cpu_->setC(0x15);
    cpu_->setD(0x19);
    cpu_->setE(0x26);
    cpu_->setH(0x25);
    cpu_->setL(0xe0);

    ASSERT_EQ(cpu_->AF() >> 8, 0x31);
    ASSERT_EQ(cpu_->BC(), 0x4115);
    ASSERT_EQ(cpu_->DE(), 0x1926);
    ASSERT_EQ(cpu_->HL(), 0x25e0);

    cpu_->setAF(0x1234);
    cpu_->setBC(0x5678);
    cpu_->setDE(0x3141);
    cpu_->setHL(0x6ba2);

    ASSERT_EQ(cpu_->A(), 0x12);
    ASSERT_EQ(cpu_->B(), 0x56);
    ASSERT_EQ(cpu_->C(), 0x78);
    ASSERT_EQ(cpu_->D(), 0x31);
    ASSERT_EQ(cpu_->E(), 0x41);
    ASSERT_EQ(cpu_->H(), 0x6b);
    ASSERT_EQ(cpu_->L(), 0xa2);
}

TEST_F(CPUTest, CPUEquality)
{
    auto cpu1 = *cpu_;
    auto cpu2 = *cpu_;

    ASSERT_EQ(cpu1, cpu2);

    cpu1.setA(0x50);
    cpu2.setA(0x60);
    ASSERT_NE(cpu1, cpu2);

    cpu1.setA(0x00);
    cpu2.setA(0x00);
    ASSERT_EQ(cpu1, cpu2);

    cpu1.advancePC(1);
    cpu2.advancePC(2);
    ASSERT_NE(cpu1, cpu2);

    cpu1.advancePC(1);
    ASSERT_EQ(cpu1, cpu2);

    // TODO: test that ram equality also is checked.
}

/*** Individual OPCode Tests ***/

// 0x00 - NOP
TEST_F(CPUTest, OpcodeTest_0x00_NOP)
{
    runProgramAndCompareRegistersAndRam(Program(4, {0x00}),    // DO 4 NOPs.
                                        ExpectedCPUs(4, *cpu_) // Expect CPU to remain unchanged after each NOP.
    );
}

// 0x01 - LD BC, d16
TEST_F(CPUTest, OpcodeTest_0x01_LD_BC_d16)
{
    Program program({
        {0x01, 0x05, 0x01}, // LD BC, 0x0105
        {0x01, 0x25, 0x3e}, // LD BC, 0x3e25
        {0x01, 0x00, 0x00}, // LD BC, 0x0000
        {0x01, 0x34, 0xab}, // LD BC, 0xab34
        {0x01, 0xff, 0xff}, // LD BC, 0xffff
        {0x01, 0x34, 0x1a}, // LD BC, 0x1a34
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);
    expectedCPUs[0]->setBC(0x0105);
    expectedCPUs[1]->setBC(0x3e25);
    expectedCPUs[2]->setBC(0x0000);
    expectedCPUs[3]->setBC(0xab34);
    expectedCPUs[4]->setBC(0xffff);
    expectedCPUs[5]->setBC(0x1a34);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x02 - LD (BC), A
TEST_F(CPUTest, OpcodeTest_0x02_LD_DEREF_BC_A)
{
    Program program({
        {0x01, 0x07, 0xc3}, // LD BC, 0xc307
        {0x3e, 0x5a},       // LD A, 0x5a
        {0x02},             // LD (BC), A
        {0x01, 0x25, 0xc7}, // LD BC, 0xc725
        {0x3e, 0x2b},       // LD A, 0x2b
        {0x02},             // LD (BC), A
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setBC(0xc307);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setBC(0xc725);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x2b);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc725, 0x2b);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x03 - INC BC
TEST_F(CPUTest, OpcodeTest_0x03_INC_BC)
{
    // TODO: test overflow?
    Program program({
        {0x01, 0x34, 0x12}, // LD BC, 0x1234
        {0x03},             // INC BC
        {0x03},             // INC BC
        {0x01, 0xfe, 0xab}, // LD BC, 0xabfe
        {0x03},             // INC BC
        {0x03},             // INC BC
        {0x03},             // INC BC
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);
    expectedCPUs[0]->setBC(0x1234);
    expectedCPUs[1]->setBC(0x1235);
    expectedCPUs[2]->setBC(0x1236);
    expectedCPUs[3]->setBC(0xabfe);
    expectedCPUs[4]->setBC(0xabff);
    expectedCPUs[5]->setBC(0xac00);
    expectedCPUs[6]->setBC(0xac01);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x04 - INC B
TEST_F(CPUTest, OpcodeTest_0x04_INC_B)
{
    // TODO: test overflow?
    Program program({
        {0x06, 0x3b}, // LD B, 0x3b
        {0x04},       // INC B
        {0x04},       // INC B
        {0x04},       // INC B
        {0x04},       // INC B
        {0x04},       // INC B
        {0x06, 0xfe}, // LD B, 0xfe
        {0x04},       // INC B
        {0x04},       // INC B
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);
    expectedCPUs[0]->setB(0x3b);
    expectedCPUs[1]->setFlags(cpu_->FlagZ(), cpu_->FlagN(), cpu_->FlagH(), cpu_->FlagC());

    expectedCPUs[1]->setB(0x3c);
    expectedCPUs[1]->setFlags(0, 0, 0, expectedCPUs[0]->FlagC());

    expectedCPUs[2]->setB(0x3d);
    expectedCPUs[2]->setFlags(0, 0, 0, expectedCPUs[1]->FlagC());

    expectedCPUs[3]->setB(0x3e);
    expectedCPUs[3]->setFlags(0, 0, 0, expectedCPUs[2]->FlagC());

    expectedCPUs[4]->setB(0x3f);
    expectedCPUs[4]->setFlags(0, 0, 0, expectedCPUs[3]->FlagC());

    expectedCPUs[5]->setB(0x40);
    expectedCPUs[5]->setFlags(0, 0, 1, expectedCPUs[4]->FlagC());

    expectedCPUs[6]->setB(0xfe);
    expectedCPUs[6]->setFlags(expectedCPUs[5]->FlagZ(), expectedCPUs[5]->FlagN(), expectedCPUs[5]->FlagH(),
                              expectedCPUs[5]->FlagC());

    expectedCPUs[7]->setB(0xff);
    expectedCPUs[7]->setFlags(0, 0, 0, expectedCPUs[6]->FlagC());

    expectedCPUs[8]->setB(0x00);
    expectedCPUs[8]->setFlags(1, 0, 1, expectedCPUs[7]->FlagC());

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x05 - DEC B
TEST_F(CPUTest, OpcodeTest_0x05_DEC_B)
{
    // TODO: test overflow?
    Program program({
        {0x06, 0xb3}, // LD B, 0xb3
        {0x05},       // DEC B
        {0x05},       // DEC B
        {0x05},       // DEC B
        {0x05},       // DEC B
        {0x05},       // DEC B
        {0x06, 0x01}, // LD B, 0x01
        {0x05},       // DEC B
        {0x05},       // DEC B
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);
    expectedCPUs[0]->setB(0xb3);
    expectedCPUs[1]->setFlags(cpu_->FlagZ(), cpu_->FlagN(), cpu_->FlagH(), cpu_->FlagC());

    expectedCPUs[1]->setB(0xb2);
    expectedCPUs[1]->setFlags(0, 1, 0, expectedCPUs[0]->FlagC());

    expectedCPUs[2]->setB(0xb1);
    expectedCPUs[2]->setFlags(0, 1, 0, expectedCPUs[1]->FlagC());

    expectedCPUs[3]->setB(0xb0);
    expectedCPUs[3]->setFlags(0, 1, 0, expectedCPUs[2]->FlagC());

    expectedCPUs[4]->setB(0xaf);
    expectedCPUs[4]->setFlags(0, 1, 1, expectedCPUs[3]->FlagC());

    expectedCPUs[5]->setB(0xae);
    expectedCPUs[5]->setFlags(0, 1, 0, expectedCPUs[4]->FlagC());

    expectedCPUs[6]->setB(0x01);
    expectedCPUs[6]->setFlags(expectedCPUs[5]->FlagZ(), expectedCPUs[5]->FlagN(), expectedCPUs[5]->FlagH(),
                              expectedCPUs[5]->FlagC());

    expectedCPUs[7]->setB(0x00);
    expectedCPUs[7]->setFlags(1, 1, 0, expectedCPUs[6]->FlagC());

    expectedCPUs[8]->setB(0xff);
    expectedCPUs[8]->setFlags(0, 1, 1, expectedCPUs[7]->FlagC());

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x06 - LD B, d8
TEST_F(CPUTest, OpcodeTest_0x06_LD_B_d8)
{
    Program program({
        {0x06, 0x01}, // LD B, 0x01
        {0x06, 0x3e}, // LD B, 0x3e
        {0x06, 0x00}, // LD B, 0x00
        {0x06, 0xab}, // LD B, 0xab
        {0x06, 0xff}, // LD B, 0xff
        {0x06, 0x1a}, // LD B, 0x1a
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);
    expectedCPUs[0]->setB(0x01);
    expectedCPUs[1]->setB(0x3e);
    expectedCPUs[2]->setB(0x00);
    expectedCPUs[3]->setB(0xab);
    expectedCPUs[4]->setB(0xff);
    expectedCPUs[5]->setB(0x1a);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x07 - RLCA
TEST_F(CPUTest, OpcodeTest_0x07_RLCA)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x07},       // RLCA
        {0x07},       // RLCA
        {0x07},       // RLCA
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);
    expectedCPUs[0]->setA(0x5a);

    expectedCPUs[1]->setA(0xb4);
    expectedCPUs[1]->setFlags(0, 0, 0, 0);

    expectedCPUs[2]->setA(0x69);
    expectedCPUs[2]->setFlags(0, 0, 0, 1);

    expectedCPUs[3]->setA(0xd2);
    expectedCPUs[3]->setFlags(0, 0, 0, 0);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x08 - LD (a16), SP
TEST_F(CPUTest, OpcodeTest_0x08_LD_DEREF_a16_SP)
{
    Program program({
        {0x31, 0x05, 0x01}, // LD SP, 0x0105
        {0x08, 0x07, 0xc3}, // LD (0xc307), SP
    });

    // TODO: test overwriting SP with its own value

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);
    expectedCPUs[0]->setSP(0x0105);

    expectedCPUs[1]->setSP(0x0105);
    expectedCPUs[1]->ram()->set(0xc307, 0x05);
    expectedCPUs[1]->ram()->set(0xc308, 0x01);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x09 - ADD HL, BC
TEST_F(CPUTest, OpcodeTest_0x09_ADD_HL_BC)
{
    Program program({
        {0x21, 0x05, 0x01}, // LD HL, 0x0105
        {0x01, 0x65, 0xa3}, // LD BC, 0xa365
        {0x09},             // ADD HL, BC
        {0x09},             // ADD HL, BC
        {0x21, 0xe4, 0x15}, // LD HL, 0x15e4
        {0x01, 0xe5, 0xa3}, // LD BC, 0xa3e5
        {0x09},             // ADD HL, BC
        {0x21, 0x00, 0x80}, // LD HL, 0x8000
        {0x01, 0x00, 0x80}, // LD BC, 0x8000
        {0x09},             // ADD HL, BC
        {0x21, 0x02, 0x4c}, // LD HL, 0x4c02
        {0x01, 0x05, 0x5b}, // LD BC, 0x5b05
        {0x09},             // ADD HL, BC
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setHL(0x0105);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setBC(0xa365);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0xa46a);
    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0x47cf);
    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0x15e4);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setBC(0xa3e5);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0xb9c9);
    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0x8000);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setBC(0x8000);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0x0000);
    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0x4c02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setBC(0x5b05);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0xa707);
    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x0A - LD A, (BC)
TEST_F(CPUTest, OpcodeTest_0x0A_LD_A_DEREF_BC)
{
    Program program({
        {0x01, 0x07, 0xc3}, // LD BC, 0xc307
        {0x3e, 0x5a},       // LD A, 0x5a
        {0x02},             // LD (BC), A
        {0x3e, 0x2b},       // LD A, 0x2b
        {0x0a},             // LD A, (BC)
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setBC(0xc307);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x2b);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x0B - DEC BC
TEST_F(CPUTest, OpcodeTest_0x0B_DEC_BC)
{
    // TODO: test overflow?
    Program program({
        {0x01, 0x34, 0x12}, // LD BC, 0x1234
        {0x0b},             // DEC BC
        {0x0b},             // DEC BC
        {0x01, 0x01, 0xab}, // LD BC, 0xab01
        {0x0b},             // DEC BC
        {0x0b},             // DEC BC
        {0x0b},             // DEC BC
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);
    expectedCPUs[0]->setBC(0x1234);
    expectedCPUs[1]->setBC(0x1233);
    expectedCPUs[2]->setBC(0x1232);
    expectedCPUs[3]->setBC(0xab01);
    expectedCPUs[4]->setBC(0xab00);
    expectedCPUs[5]->setBC(0xaaff);
    expectedCPUs[6]->setBC(0xaafe);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x0C - INC C
TEST_F(CPUTest, OpcodeTest_0x0C_INC_C)
{
    // TODO: test overflow?
    Program program({
        {0x0e, 0x3b}, // LD C, 0x3b
        {0x0c},       // INC C
        {0x0c},       // INC C
        {0x0c},       // INC C
        {0x0c},       // INC C
        {0x0c},       // INC C
        {0x0e, 0xfe}, // LD C, 0xfe
        {0x0c},       // INC C
        {0x0c},       // INC C
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setC(0x3b);
    expectedCPU.setFlags(cpu_->FlagZ(), cpu_->FlagN(), cpu_->FlagH(), cpu_->FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x3c);
    expectedCPU.setFlags(0, 0, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x3d);
    expectedCPU.setFlags(0, 0, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x3e);
    expectedCPU.setFlags(0, 0, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x3f);
    expectedCPU.setFlags(0, 0, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x40);
    expectedCPU.setFlags(0, 0, 1, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0xfe);
    expectedCPU.setFlags(expectedCPU.FlagZ(), expectedCPU.FlagN(), expectedCPU.FlagH(), expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0xff);
    expectedCPU.setFlags(0, 0, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x00);
    expectedCPU.setFlags(1, 0, 1, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x0D - DEC C
TEST_F(CPUTest, OpcodeTest_0x0D_DEC_C)
{
    // TODO: test overflow?
    Program program({
        {0x0e, 0xb3}, // LD C, 0xb3
        {0x0d},       // DEC C
        {0x0d},       // DEC C
        {0x0d},       // DEC C
        {0x0d},       // DEC C
        {0x0d},       // DEC C
        {0x0e, 0x01}, // LD C, 0x01
        {0x0d},       // DEC C
        {0x0d},       // DEC C
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setC(0xb3);
    expectedCPU.setFlags(cpu_->FlagZ(), cpu_->FlagN(), cpu_->FlagH(), cpu_->FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0xb2);
    expectedCPU.setFlags(0, 1, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0xb1);
    expectedCPU.setFlags(0, 1, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0xb0);
    expectedCPU.setFlags(0, 1, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0xaf);
    expectedCPU.setFlags(0, 1, 1, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0xae);
    expectedCPU.setFlags(0, 1, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x01);
    expectedCPU.setFlags(expectedCPU.FlagZ(), expectedCPU.FlagN(), expectedCPU.FlagH(), expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x00);
    expectedCPU.setFlags(1, 1, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0xff);
    expectedCPU.setFlags(0, 1, 1, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x0E - LD C, d8
TEST_F(CPUTest, OpcodeTest_0x0E_LD_C_d8)
{
    Program program({
        {0x0E, 0x01}, // LD C, 0x01
        {0x0E, 0x3e}, // LD C, 0x3e
        {0x0E, 0x00}, // LD C, 0x00
        {0x0E, 0xab}, // LD C, 0xab
        {0x0E, 0xff}, // LD C, 0xff
        {0x0E, 0x1a}, // LD C, 0x1a
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);
    expectedCPUs[0]->setC(0x01);
    expectedCPUs[1]->setC(0x3e);
    expectedCPUs[2]->setC(0x00);
    expectedCPUs[3]->setC(0xab);
    expectedCPUs[4]->setC(0xff);
    expectedCPUs[5]->setC(0x1a);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x0F - RRCA
TEST_F(CPUTest, OpcodeTest_0x0F_RRCA)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x0f},       // RRCA
        {0x0f},       // RRCA
        {0x0f},       // RRCA
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);
    expectedCPUs[0]->setA(0x5a);

    expectedCPUs[1]->setA(0x2d);
    expectedCPUs[1]->setFlags(0, 0, 0, 0);

    expectedCPUs[2]->setA(0x96);
    expectedCPUs[2]->setFlags(0, 0, 0, 1);

    expectedCPUs[3]->setA(0x4b);
    expectedCPUs[3]->setFlags(0, 0, 0, 0);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x10 - STOP
TEST_F(CPUTest, OpcodeTest_0x10_STOP)
{
    // TODO: implement STOP instruction and its tests.
    // throw std::runtime_error("STOP tests not implemented");
}

// 0x11 - LD DE, d16
TEST_F(CPUTest, OpcodeTest_0x11_LD_DE_d16)
{
    Program program({
        {0x11, 0x05, 0x01}, // LD DE, 0x0105
        {0x11, 0x25, 0x3e}, // LD DE, 0x3e25
        {0x11, 0x00, 0x00}, // LD DE, 0x0000
        {0x11, 0x34, 0xab}, // LD DE, 0xab34
        {0x11, 0xff, 0xff}, // LD DE, 0xffff
        {0x11, 0x34, 0x1a}, // LD DE, 0x1a34
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);
    expectedCPUs[0]->setDE(0x0105);
    expectedCPUs[1]->setDE(0x3e25);
    expectedCPUs[2]->setDE(0x0000);
    expectedCPUs[3]->setDE(0xab34);
    expectedCPUs[4]->setDE(0xffff);
    expectedCPUs[5]->setDE(0x1a34);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x12 - LD (DE), A
TEST_F(CPUTest, OpcodeTest_0x12_LD_DEREF_DE_A)
{
    Program program({
        {0x11, 0x06, 0xc3}, // LD DE, 0xc306
        {0x3e, 0x5a},       // LD A, 0x5a
        {0x12},             // LD (DE), A
        {0x11, 0x26, 0xc7}, // LD DE, 0xc726
        {0x3e, 0x2b},       // LD A, 0x2b
        {0x12},             // LD (DE), A
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setDE(0xc306);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc306, 0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setDE(0xc726);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x2b);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc726, 0x2b);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x13 - INC DE
TEST_F(CPUTest, OpcodeTest_0x13_INC_DE)
{
    // TODO: test overflow?
    Program program({
        {0x11, 0x34, 0x12}, // LD DE, 0x1234
        {0x13},             // INC DE
        {0x13},             // INC DE
        {0x11, 0xfe, 0xab}, // LD DE, 0xabfe
        {0x13},             // INC DE
        {0x13},             // INC DE
        {0x13},             // INC DE
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);
    expectedCPUs[0]->setDE(0x1234);
    expectedCPUs[1]->setDE(0x1235);
    expectedCPUs[2]->setDE(0x1236);
    expectedCPUs[3]->setDE(0xabfe);
    expectedCPUs[4]->setDE(0xabff);
    expectedCPUs[5]->setDE(0xac00);
    expectedCPUs[6]->setDE(0xac01);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x14 - INC D
TEST_F(CPUTest, OpcodeTest_0x14_INC_D)
{
    // TODO: test overflow?
    Program program({
        {0x16, 0x3b}, // LD D, 0x3b
        {0x14},       // INC D
        {0x14},       // INC D
        {0x14},       // INC D
        {0x14},       // INC D
        {0x14},       // INC D
        {0x16, 0xfe}, // LD D, 0xfe
        {0x14},       // INC D
        {0x14},       // INC D
        {0x14},       // INC D
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);
    expectedCPUs[0]->setD(0x3b);
    expectedCPUs[1]->setFlags(cpu_->FlagZ(), cpu_->FlagN(), cpu_->FlagH(), cpu_->FlagC());

    expectedCPUs[1]->setD(0x3c);
    expectedCPUs[1]->setFlags(0, 0, 0, expectedCPUs[0]->FlagC());

    expectedCPUs[2]->setD(0x3d);
    expectedCPUs[2]->setFlags(0, 0, 0, expectedCPUs[1]->FlagC());

    expectedCPUs[3]->setD(0x3e);
    expectedCPUs[3]->setFlags(0, 0, 0, expectedCPUs[2]->FlagC());

    expectedCPUs[4]->setD(0x3f);
    expectedCPUs[4]->setFlags(0, 0, 0, expectedCPUs[3]->FlagC());

    expectedCPUs[5]->setD(0x40);
    expectedCPUs[5]->setFlags(0, 0, 1, expectedCPUs[4]->FlagC());

    expectedCPUs[6]->setD(0xfe);
    expectedCPUs[6]->setFlags(expectedCPUs[5]->FlagZ(), expectedCPUs[5]->FlagN(), expectedCPUs[5]->FlagH(),
                              expectedCPUs[5]->FlagC());

    expectedCPUs[7]->setD(0xff);
    expectedCPUs[7]->setFlags(0, 0, 0, expectedCPUs[6]->FlagC());

    expectedCPUs[8]->setD(0x00);
    expectedCPUs[8]->setFlags(1, 0, 1, expectedCPUs[7]->FlagC());

    expectedCPUs[9]->setD(0x01);
    expectedCPUs[9]->setFlags(0, 0, 0, expectedCPUs[8]->FlagC());

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x15 - DEC D
TEST_F(CPUTest, OpcodeTest_0x15_DEC_D)
{
    // TODO: test overflow?
    Program program({
        {0x16, 0xb3}, // LD D, 0xb3
        {0x15},       // DEC D
        {0x15},       // DEC D
        {0x15},       // DEC D
        {0x15},       // DEC D
        {0x15},       // DEC D
        {0x16, 0x01}, // LD D, 0x01
        {0x15},       // DEC D
        {0x15},       // DEC D
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);
    expectedCPUs[0]->setD(0xb3);
    expectedCPUs[1]->setFlags(cpu_->FlagZ(), cpu_->FlagN(), cpu_->FlagH(), cpu_->FlagC());

    expectedCPUs[1]->setD(0xb2);
    expectedCPUs[1]->setFlags(0, 1, 0, expectedCPUs[0]->FlagC());

    expectedCPUs[2]->setD(0xb1);
    expectedCPUs[2]->setFlags(0, 1, 0, expectedCPUs[1]->FlagC());

    expectedCPUs[3]->setD(0xb0);
    expectedCPUs[3]->setFlags(0, 1, 0, expectedCPUs[2]->FlagC());

    expectedCPUs[4]->setD(0xaf);
    expectedCPUs[4]->setFlags(0, 1, 1, expectedCPUs[3]->FlagC());

    expectedCPUs[5]->setD(0xae);
    expectedCPUs[5]->setFlags(0, 1, 0, expectedCPUs[4]->FlagC());

    expectedCPUs[6]->setD(0x01);
    expectedCPUs[6]->setFlags(expectedCPUs[5]->FlagZ(), expectedCPUs[5]->FlagN(), expectedCPUs[5]->FlagH(),
                              expectedCPUs[5]->FlagC());

    expectedCPUs[7]->setD(0x00);
    expectedCPUs[7]->setFlags(1, 1, 0, expectedCPUs[6]->FlagC());

    expectedCPUs[8]->setD(0xff);
    expectedCPUs[8]->setFlags(0, 1, 1, expectedCPUs[7]->FlagC());

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x16 - LD D, d8
TEST_F(CPUTest, OpcodeTest_0x16_LD_D_d8)
{
    Program program({
        {0x16, 0x01}, // LD D, 0x01
        {0x16, 0x3e}, // LD D, 0x3e
        {0x16, 0x00}, // LD D, 0x00
        {0x16, 0xab}, // LD D, 0xab
        {0x16, 0xff}, // LD D, 0xff
        {0x16, 0x1a}, // LD D, 0x1a
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);
    expectedCPUs[0]->setD(0x01);
    expectedCPUs[1]->setD(0x3e);
    expectedCPUs[2]->setD(0x00);
    expectedCPUs[3]->setD(0xab);
    expectedCPUs[4]->setD(0xff);
    expectedCPUs[5]->setD(0x1a);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x17 - RLA
TEST_F(CPUTest, OpcodeTest_0x17_RLA)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x17},       // RLA
        {0x17},       // RLA
        {0x17},       // RLA
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);
    expectedCPUs[0]->setA(0x5a);

    expectedCPUs[1]->setA(0xb5); // default value of C=1
    expectedCPUs[1]->setFlags(0, 0, 0, 0);

    expectedCPUs[2]->setA(0x6a);
    expectedCPUs[2]->setFlags(0, 0, 0, 1);

    expectedCPUs[3]->setA(0xd5);
    expectedCPUs[3]->setFlags(0, 0, 0, 0);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x18 - JR s8
TEST_F(CPUTest, OpcodeTest_0x18_JR_s8_FWD)
{
    Program program({
        {0x18, 0x25}, // JR 0x25
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);
    expectedCPUs[0]->setPC(0x0127);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

TEST_F(CPUTest, OpcodeTest_0x18_JR_s8_BKWD)
{
    Program program({
        {0x00},       // NOP
        {0x18, 0xf6}, // JR 0xf6
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);
    expectedCPUs[0]->setPC(0x0101);
    expectedCPUs[1]->setPC(0x00f9);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

// 0x19 - ADD HL, DE
TEST_F(CPUTest, OpcodeTest_0x19_ADD_HL_DE)
{
    Program program({
        {0x21, 0x05, 0x01}, // LD HL, 0x0105
        {0x11, 0x65, 0xa3}, // LD DE, 0xa365
        {0x19},             // ADD HL, DE
        {0x19},             // ADD HL, DE
        {0x21, 0xe4, 0x15}, // LD HL, 0x15e4
        {0x11, 0xe5, 0xa3}, // LD DE, 0xa3e5
        {0x19},             // ADD HL, DE
        {0x21, 0x00, 0x80}, // LD HL, 0x8000
        {0x11, 0x00, 0x80}, // LD DE, 0x8000
        {0x19},             // ADD HL, DE
        {0x21, 0x02, 0x4c}, // LD HL, 0x4c02
        {0x11, 0x05, 0x5b}, // LD DE, 0x5b05
        {0x19},             // ADD HL, DE
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setHL(0x0105);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setDE(0xa365);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0xa46a);
    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0x47cf);
    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0x15e4);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setDE(0xa3e5);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0xb9c9);
    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0x8000);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setDE(0x8000);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0x0000);
    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0x4c02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setDE(0x5b05);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0xa707);
    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x1A - LD A, (DE)
TEST_F(CPUTest, OpcodeTest_0x1A_LD_A_DEREF_DE)
{
    Program program({
        {0x11, 0x07, 0xc3}, // LD DE, 0xc307
        {0x3e, 0x5a},       // LD A, 0x5a
        {0x12},             // LD (DE), A
        {0x3e, 0x2b},       // LD A, 0x2b
        {0x1a},             // LD A, (DE)
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setDE(0xc307);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x2b);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x1B - DEC DE
TEST_F(CPUTest, OpcodeTest_0x1B_DEC_DE)
{
    // TODO: test overflow?
    Program program({
        {0x11, 0x34, 0x12}, // LD DE, 0x1234
        {0x1b},             // DEC DE
        {0x1b},             // DEC DE
        {0x11, 0x01, 0xab}, // LD DE, 0xab01
        {0x1b},             // DEC DE
        {0x1b},             // DEC DE
        {0x1b},             // DEC DE
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);
    expectedCPUs[0]->setDE(0x1234);
    expectedCPUs[1]->setDE(0x1233);
    expectedCPUs[2]->setDE(0x1232);
    expectedCPUs[3]->setDE(0xab01);
    expectedCPUs[4]->setDE(0xab00);
    expectedCPUs[5]->setDE(0xaaff);
    expectedCPUs[6]->setDE(0xaafe);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x1C - INC E
TEST_F(CPUTest, OpcodeTest_0x1C_INC_E)
{
    // TODO: test overflow?
    Program program({
        {0x1e, 0x3b}, // LD E, 0x3b
        {0x1c},       // INC E
        {0x1c},       // INC E
        {0x1c},       // INC E
        {0x1c},       // INC E
        {0x1c},       // INC E
        {0x1e, 0xfe}, // LD E, 0xfe
        {0x1c},       // INC E
        {0x1c},       // INC E
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setE(0x3b);
    expectedCPU.setFlags(cpu_->FlagZ(), cpu_->FlagN(), cpu_->FlagH(), cpu_->FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x3c);
    expectedCPU.setFlags(0, 0, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x3d);
    expectedCPU.setFlags(0, 0, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x3e);
    expectedCPU.setFlags(0, 0, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x3f);
    expectedCPU.setFlags(0, 0, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x40);
    expectedCPU.setFlags(0, 0, 1, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0xfe);
    expectedCPU.setFlags(expectedCPU.FlagZ(), expectedCPU.FlagN(), expectedCPU.FlagH(), expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0xff);
    expectedCPU.setFlags(0, 0, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x00);
    expectedCPU.setFlags(1, 0, 1, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x1D - DEC E
TEST_F(CPUTest, OpcodeTest_0x1D_DEC_E)
{
    // TODO: test overflow?
    Program program({
        {0x1e, 0xb3}, // LD E, 0xb3
        {0x1d},       // DEC E
        {0x1d},       // DEC E
        {0x1d},       // DEC E
        {0x1d},       // DEC E
        {0x1d},       // DEC E
        {0x1e, 0x01}, // LD E, 0x01
        {0x1d},       // DEC E
        {0x1d},       // DEC E
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setE(0xb3);
    expectedCPU.setFlags(cpu_->FlagZ(), cpu_->FlagN(), cpu_->FlagH(), cpu_->FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0xb2);
    expectedCPU.setFlags(0, 1, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0xb1);
    expectedCPU.setFlags(0, 1, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0xb0);
    expectedCPU.setFlags(0, 1, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0xaf);
    expectedCPU.setFlags(0, 1, 1, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0xae);
    expectedCPU.setFlags(0, 1, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x01);
    expectedCPU.setFlags(expectedCPU.FlagZ(), expectedCPU.FlagN(), expectedCPU.FlagH(), expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x00);
    expectedCPU.setFlags(1, 1, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0xff);
    expectedCPU.setFlags(0, 1, 1, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x1E - LD E, d8
TEST_F(CPUTest, OpcodeTest_0x1E_LD_E_d8)
{
    Program program({
        {0x1e, 0x01}, // LD E, 0x01
        {0x1e, 0x3e}, // LD E, 0x3e
        {0x1e, 0x00}, // LD E, 0x00
        {0x1e, 0xab}, // LD E, 0xab
        {0x1e, 0xff}, // LD E, 0xff
        {0x1e, 0x1a}, // LD E, 0x1a
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);
    expectedCPUs[0]->setE(0x01);
    expectedCPUs[1]->setE(0x3e);
    expectedCPUs[2]->setE(0x00);
    expectedCPUs[3]->setE(0xab);
    expectedCPUs[4]->setE(0xff);
    expectedCPUs[5]->setE(0x1a);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x1F - RRA
TEST_F(CPUTest, OpcodeTest_0x1F_RRA)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x1f},       // RRA
        {0x1f},       // RRA
        {0x1f},       // RRA
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);
    expectedCPUs[0]->setA(0x5a);

    expectedCPUs[1]->setA(0xad);
    expectedCPUs[1]->setFlags(0, 0, 0, 0);

    expectedCPUs[2]->setA(0x56);
    expectedCPUs[2]->setFlags(0, 0, 0, 1);

    expectedCPUs[3]->setA(0xab);
    expectedCPUs[3]->setFlags(0, 0, 0, 0);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x20 - JR NZ, s8
TEST_F(CPUTest, OpcodeTest_0x20_JR_NZ_s8_FWD)
{
    Program program({
        {0xaf},       // XOR A, A
        {0x0f},       // RRCA
        {0x06, 0x00}, // LD B, 0x00
        {0x04},       // INC B
        {0x20, 0x25}, // JR NZ, 0x25
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);

    expectedCPUs[0]->setPC(0x0101);
    expectedCPUs[0]->setA(0);
    expectedCPUs[0]->setFlags(1, 0, 0, 0);

    expectedCPUs[1]->setPC(0x0102);
    expectedCPUs[1]->setA(0);
    expectedCPUs[1]->setFlags(0, 0, 0, 0);

    expectedCPUs[2]->setPC(0x0104);
    expectedCPUs[2]->setA(0);
    expectedCPUs[2]->setFlags(0, 0, 0, 0);
    expectedCPUs[2]->setB(0x00);

    expectedCPUs[3]->setPC(0x0105);
    expectedCPUs[3]->setA(0);
    expectedCPUs[3]->setB(0x01);
    expectedCPUs[3]->setFlags(0, 0, 0, 0);

    expectedCPUs[4]->setPC(0x012C);
    expectedCPUs[4]->setA(0);
    expectedCPUs[4]->setB(0x01);
    expectedCPUs[4]->setFlags(0, 0, 0, 0);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

TEST_F(CPUTest, OpcodeTest_0x20_JR_NZ_s8_BKWD)
{
    Program program({
        {0xaf},       // XOR A, A
        {0x0f},       // RRCA
        {0x06, 0x00}, // LD B, 0x00
        {0x04},       // INC B
        {0x20, 0xf6}, // JR NZ, 0xf6
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);

    expectedCPUs[0]->setPC(0x0101);
    expectedCPUs[0]->setA(0);
    expectedCPUs[0]->setFlags(1, 0, 0, 0);

    expectedCPUs[1]->setPC(0x0102);
    expectedCPUs[1]->setA(0);
    expectedCPUs[1]->setFlags(0, 0, 0, 0);

    expectedCPUs[2]->setPC(0x0104);
    expectedCPUs[2]->setA(0);
    expectedCPUs[2]->setFlags(0, 0, 0, 0);
    expectedCPUs[2]->setB(0x00);

    expectedCPUs[3]->setPC(0x0105);
    expectedCPUs[3]->setA(0);
    expectedCPUs[3]->setB(0x01);
    expectedCPUs[3]->setFlags(0, 0, 0, 0);

    expectedCPUs[4]->setPC(0x00fd);
    expectedCPUs[4]->setA(0);
    expectedCPUs[4]->setB(0x01);
    expectedCPUs[4]->setFlags(0, 0, 0, 0);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

TEST_F(CPUTest, OpcodeTest_0x20_JR_NZ_s8_NO_JUMP)
{
    Program program({
        {0xaf},       // XOR A
        {0x20, 0x25}, // JR NZ, 0x25
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);
    expectedCPUs[0]->setPC(0x0101);
    expectedCPUs[0]->setA(0x00);
    expectedCPUs[0]->setFlags(1, 0, 0, 0);

    expectedCPUs[1]->setPC(0x0103);
    expectedCPUs[1]->setA(0x00);
    expectedCPUs[1]->setFlags(1, 0, 0, 0);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

// 0x21 - LD HL, d16
TEST_F(CPUTest, OpcodeTest_0x21_LD_HL_d16)
{
    Program program({
        {0x21, 0x05, 0x01}, // LD HL, 0x0105
        {0x21, 0x25, 0x3e}, // LD HL, 0x3e25
        {0x21, 0x00, 0x00}, // LD HL, 0x0000
        {0x21, 0x34, 0xab}, // LD HL, 0xab34
        {0x21, 0xff, 0xff}, // LD HL, 0xffff
        {0x21, 0x34, 0x1a}, // LD HL, 0x1a34
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);
    expectedCPUs[0]->setHL(0x0105);
    expectedCPUs[1]->setHL(0x3e25);
    expectedCPUs[2]->setHL(0x0000);
    expectedCPUs[3]->setHL(0xab34);
    expectedCPUs[4]->setHL(0xffff);
    expectedCPUs[5]->setHL(0x1a34);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x22 - LD (HL+), A
TEST_F(CPUTest, OpcodeTest_0x22_LDI_DEREF_HL_A)
{
    Program program({
        {0x21, 0x06, 0xc3}, // LD HL, 0xc306
        {0x3e, 0x5a},       // LD A, 0x5a
        {0x22},             // LD (HL+), A
        {0x3e, 0x14},       // LD A, 0x14
        {0x22},             // LD (HL+), A
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setHL(0xc306);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc306, 0x5a);
    expectedCPU.setHL(0xc307);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x14);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x14);
    expectedCPU.setHL(0xc308);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x23 - INC HL
TEST_F(CPUTest, OpcodeTest_0x23_INC_HL)
{
    // TODO: test overflow?
    Program program({
        {0x21, 0x34, 0x12}, // LD HL, 0x1234
        {0x23},             // INC HL
        {0x23},             // INC HL
        {0x21, 0xfe, 0xab}, // LD HL, 0xabfe
        {0x23},             // INC HL
        {0x23},             // INC HL
        {0x23},             // INC HL
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);
    expectedCPUs[0]->setHL(0x1234);
    expectedCPUs[1]->setHL(0x1235);
    expectedCPUs[2]->setHL(0x1236);
    expectedCPUs[3]->setHL(0xabfe);
    expectedCPUs[4]->setHL(0xabff);
    expectedCPUs[5]->setHL(0xac00);
    expectedCPUs[6]->setHL(0xac01);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x24 - INC H
TEST_F(CPUTest, OpcodeTest_0x24_INC_H)
{
    // TODO: test overflow?
    Program program({
        {0x26, 0x3b}, // LD H, 0x3b
        {0x24},       // INC H
        {0x24},       // INC H
        {0x24},       // INC H
        {0x24},       // INC H
        {0x24},       // INC H
        {0x26, 0xfe}, // LD H, 0xfe
        {0x24},       // INC H
        {0x24},       // INC H
        {0x24},       // INC H
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);
    expectedCPUs[0]->setH(0x3b);
    expectedCPUs[1]->setFlags(cpu_->FlagZ(), cpu_->FlagN(), cpu_->FlagH(), cpu_->FlagC());

    expectedCPUs[1]->setH(0x3c);
    expectedCPUs[1]->setFlags(0, 0, 0, expectedCPUs[0]->FlagC());

    expectedCPUs[2]->setH(0x3d);
    expectedCPUs[2]->setFlags(0, 0, 0, expectedCPUs[1]->FlagC());

    expectedCPUs[3]->setH(0x3e);
    expectedCPUs[3]->setFlags(0, 0, 0, expectedCPUs[2]->FlagC());

    expectedCPUs[4]->setH(0x3f);
    expectedCPUs[4]->setFlags(0, 0, 0, expectedCPUs[3]->FlagC());

    expectedCPUs[5]->setH(0x40);
    expectedCPUs[5]->setFlags(0, 0, 1, expectedCPUs[4]->FlagC());

    expectedCPUs[6]->setH(0xfe);
    expectedCPUs[6]->setFlags(expectedCPUs[5]->FlagZ(), expectedCPUs[5]->FlagN(), expectedCPUs[5]->FlagH(),
                              expectedCPUs[5]->FlagC());

    expectedCPUs[7]->setH(0xff);
    expectedCPUs[7]->setFlags(0, 0, 0, expectedCPUs[6]->FlagC());

    expectedCPUs[8]->setH(0x00);
    expectedCPUs[8]->setFlags(1, 0, 1, expectedCPUs[7]->FlagC());

    expectedCPUs[9]->setH(0x01);
    expectedCPUs[9]->setFlags(0, 0, 0, expectedCPUs[8]->FlagC());

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x25 - DEC H
TEST_F(CPUTest, OpcodeTest_0x25_DEC_H)
{
    // TODO: test overflow?
    Program program({
        {0x26, 0xb3}, // LD H, 0xb3
        {0x25},       // DEC H
        {0x25},       // DEC H
        {0x25},       // DEC H
        {0x25},       // DEC H
        {0x25},       // DEC H
        {0x26, 0x01}, // LD H, 0x01
        {0x25},       // DEC H
        {0x25},       // DEC H
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);
    expectedCPUs[0]->setH(0xb3);
    expectedCPUs[1]->setFlags(cpu_->FlagZ(), cpu_->FlagN(), cpu_->FlagH(), cpu_->FlagC());

    expectedCPUs[1]->setH(0xb2);
    expectedCPUs[1]->setFlags(0, 1, 0, expectedCPUs[0]->FlagC());

    expectedCPUs[2]->setH(0xb1);
    expectedCPUs[2]->setFlags(0, 1, 0, expectedCPUs[1]->FlagC());

    expectedCPUs[3]->setH(0xb0);
    expectedCPUs[3]->setFlags(0, 1, 0, expectedCPUs[2]->FlagC());

    expectedCPUs[4]->setH(0xaf);
    expectedCPUs[4]->setFlags(0, 1, 1, expectedCPUs[3]->FlagC());

    expectedCPUs[5]->setH(0xae);
    expectedCPUs[5]->setFlags(0, 1, 0, expectedCPUs[4]->FlagC());

    expectedCPUs[6]->setH(0x01);
    expectedCPUs[6]->setFlags(expectedCPUs[5]->FlagZ(), expectedCPUs[5]->FlagN(), expectedCPUs[5]->FlagH(),
                              expectedCPUs[5]->FlagC());

    expectedCPUs[7]->setH(0x00);
    expectedCPUs[7]->setFlags(1, 1, 0, expectedCPUs[6]->FlagC());

    expectedCPUs[8]->setH(0xff);
    expectedCPUs[8]->setFlags(0, 1, 1, expectedCPUs[7]->FlagC());

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x26 - LD H, d8
TEST_F(CPUTest, OpcodeTest_0x26_LD_H_d8)
{
    Program program({
        {0x26, 0x01}, // LD H, 0x01
        {0x26, 0x3e}, // LD H, 0x3e
        {0x26, 0x00}, // LD H, 0x00
        {0x26, 0xab}, // LD H, 0xab
        {0x26, 0xff}, // LD H, 0xff
        {0x26, 0x1a}, // LD H, 0x1a
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);
    expectedCPUs[0]->setH(0x01);
    expectedCPUs[1]->setH(0x3e);
    expectedCPUs[2]->setH(0x00);
    expectedCPUs[3]->setH(0xab);
    expectedCPUs[4]->setH(0xff);
    expectedCPUs[5]->setH(0x1a);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x27 - DAA
TEST_F(CPUTest, OpcodeTest_0x27_DAA)
{
    // TODO: implement DAA instruction and its tests.
    // throw std::runtime_error("DAA tests not implemented");
}

// 0x28 - JR Z, s8
TEST_F(CPUTest, OpcodeTest_0x28_JR_Z_s8_FWD)
{
    Program program({
        {0xaf},       // XOR A
        {0x28, 0x25}, // JR Z, 0x25
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);
    expectedCPUs[0]->setPC(0x0101);
    expectedCPUs[0]->setA(0x00);
    expectedCPUs[0]->setFlags(1, 0, 0, 0);

    expectedCPUs[1]->setPC(0x0128);
    expectedCPUs[1]->setA(0x00);
    expectedCPUs[1]->setFlags(1, 0, 0, 0);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

TEST_F(CPUTest, OpcodeTest_0x28_JR_Z_s8_BKWD)
{
    Program program({
        {0xaf},       // XOR A
        {0x28, 0xf6}, // JR Z, 0xf6
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);
    expectedCPUs[0]->setPC(0x0101);
    expectedCPUs[0]->setA(0x00);
    expectedCPUs[0]->setFlags(1, 0, 0, 0);

    expectedCPUs[1]->setPC(0x0f9);
    expectedCPUs[1]->setA(0x00);
    expectedCPUs[1]->setFlags(1, 0, 0, 0);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

TEST_F(CPUTest, OpcodeTest_0x28_JR_Z_s8_NO_JUMP)
{
    Program program({
        {0xaf},       // XOR A, A
        {0x0f},       // RRCA
        {0x06, 0x00}, // LD B, 0x00
        {0x04},       // INC B
        {0x28, 0x25}, // JR Z, 0x25
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);

    expectedCPUs[0]->setPC(0x0101);
    expectedCPUs[0]->setA(0);
    expectedCPUs[0]->setFlags(1, 0, 0, 0);

    expectedCPUs[1]->setPC(0x0102);
    expectedCPUs[1]->setA(0);
    expectedCPUs[1]->setFlags(0, 0, 0, 0);

    expectedCPUs[2]->setPC(0x0104);
    expectedCPUs[2]->setA(0);
    expectedCPUs[2]->setFlags(0, 0, 0, 0);
    expectedCPUs[2]->setB(0x00);

    expectedCPUs[3]->setPC(0x0105);
    expectedCPUs[3]->setA(0);
    expectedCPUs[3]->setB(0x01);
    expectedCPUs[3]->setFlags(0, 0, 0, 0);

    expectedCPUs[4]->setPC(0x0107);
    expectedCPUs[4]->setA(0);
    expectedCPUs[4]->setB(0x01);
    expectedCPUs[4]->setFlags(0, 0, 0, 0);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

// 0x29 - ADD HL, HL
TEST_F(CPUTest, OpcodeTest_0x29_ADD_HL_HL)
{
    Program program({
        {0x21, 0x05, 0x01}, // LD HL, 0x0105
        {0x29},             // ADD HL, DL
        {0x29},             // ADD HL, DL
        {0x21, 0xe4, 0x15}, // LD HL, 0x15e4
        {0x29},             // ADD HL, HL
        {0x21, 0xff, 0xff}, // LD HL, 0xffff
        {0x29},             // ADD HL, HL
        {0x21, 0x00, 0x80}, // LD HL, 0x8000
        {0x29},             // ADD HL, HL
        {0x21, 0x00, 0x4c}, // LD HL, 0x4c00
        {0x29},             // ADD HL
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);
    expectedCPUs[0]->setHL(0x0105);

    expectedCPUs[1]->setHL(0x020a);
    expectedCPUs[1]->setFlags(expectedCPUs[0]->FlagZ(), 0, 0, 0);

    expectedCPUs[2]->setHL(0x0414);
    expectedCPUs[2]->setFlags(expectedCPUs[1]->FlagZ(), 0, 0, 0);

    expectedCPUs[3]->setHL(0x15e4);
    expectedCPUs[3]->setFlags(expectedCPUs[2]->FlagZ(), expectedCPUs[2]->FlagN(), expectedCPUs[2]->FlagH(),
                              expectedCPUs[2]->FlagC());

    expectedCPUs[4]->setHL(0x2bc8);
    expectedCPUs[4]->setFlags(expectedCPUs[3]->FlagZ(), 0, 0, 0);

    expectedCPUs[5]->setHL(0xffff);
    expectedCPUs[5]->setFlags(expectedCPUs[4]->FlagZ(), expectedCPUs[4]->FlagN(), expectedCPUs[4]->FlagH(),
                              expectedCPUs[4]->FlagC());

    expectedCPUs[6]->setHL(0xfffe);
    expectedCPUs[6]->setFlags(expectedCPUs[5]->FlagZ(), 0, 1, 1);

    expectedCPUs[7]->setHL(0x8000);
    expectedCPUs[7]->setFlags(expectedCPUs[6]->FlagZ(), expectedCPUs[6]->FlagN(), expectedCPUs[6]->FlagH(),
                              expectedCPUs[6]->FlagC());

    expectedCPUs[8]->setHL(0x0000);
    expectedCPUs[8]->setFlags(expectedCPUs[7]->FlagZ(), 0, 0, 1);

    expectedCPUs[9]->setHL(0x4c00);
    expectedCPUs[9]->setFlags(expectedCPUs[8]->FlagZ(), expectedCPUs[8]->FlagN(), expectedCPUs[8]->FlagH(),
                              expectedCPUs[8]->FlagC());

    expectedCPUs[10]->setHL(0x9800);
    expectedCPUs[10]->setFlags(expectedCPUs[7]->FlagZ(), 0, 1, 0);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x2A - LD A, (HL+)
TEST_F(CPUTest, OpcodeTest_0x2A_LDI_A_DEREF_HL)
{
    Program program({
        {0x21, 0x07, 0xc3}, // LD HL, 0xc307
        {0x3e, 0x5a},       // LD A, 0x5a
        {0x22},             // LD (HL+), A
        {0x2B},             // DEC HL
        {0x3e, 0x2b},       // LD A, 0x2b
        {0x2a},             // LD A, (HL+)
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setHL(0xc307);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x5a);
    expectedCPU.setHL(0xc308);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0xc307);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x2b);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPU.setHL(0xc308);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x2B - DEC HL
TEST_F(CPUTest, OpcodeTest_0x2B_DEC_HL)
{
    // TODO: test overflow?
    Program program({
        {0x21, 0x34, 0x12}, // LD HL, 0x1234
        {0x2b},             // DEC HL
        {0x2b},             // DEC HL
        {0x21, 0x01, 0xab}, // LD HL, 0xab01
        {0x2b},             // DEC HL
        {0x2b},             // DEC HL
        {0x2b},             // DEC HL
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);
    expectedCPUs[0]->setHL(0x1234);
    expectedCPUs[1]->setHL(0x1233);
    expectedCPUs[2]->setHL(0x1232);
    expectedCPUs[3]->setHL(0xab01);
    expectedCPUs[4]->setHL(0xab00);
    expectedCPUs[5]->setHL(0xaaff);
    expectedCPUs[6]->setHL(0xaafe);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x2C - INC L
TEST_F(CPUTest, OpcodeTest_0x2C_INC_L)
{
    // TODO: test overflow?
    Program program({
        {0x2e, 0x3b}, // LD L, 0x3b
        {0x2c},       // INC L
        {0x2c},       // INC L
        {0x2c},       // INC L
        {0x2c},       // INC L
        {0x2c},       // INC L
        {0x2e, 0xfe}, // LD L, 0xfe
        {0x2c},       // INC L
        {0x2c},       // INC L
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setL(0x3b);
    expectedCPU.setFlags(cpu_->FlagZ(), cpu_->FlagN(), cpu_->FlagH(), cpu_->FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x3c);
    expectedCPU.setFlags(0, 0, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x3d);
    expectedCPU.setFlags(0, 0, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x3e);
    expectedCPU.setFlags(0, 0, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x3f);
    expectedCPU.setFlags(0, 0, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x40);
    expectedCPU.setFlags(0, 0, 1, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0xfe);
    expectedCPU.setFlags(expectedCPU.FlagZ(), expectedCPU.FlagN(), expectedCPU.FlagH(), expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0xff);
    expectedCPU.setFlags(0, 0, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x00);
    expectedCPU.setFlags(1, 0, 1, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x2D - DEC L
TEST_F(CPUTest, OpcodeTest_0x2D_DEC_L)
{
    // TODO: test overflow?
    Program program({
        {0x2e, 0xb3}, // LD L, 0xb3
        {0x2d},       // DEC L
        {0x2d},       // DEC L
        {0x2d},       // DEC L
        {0x2d},       // DEC L
        {0x2d},       // DEC L
        {0x2e, 0x01}, // LD L, 0x01
        {0x2d},       // DEC L
        {0x2d},       // DEC L
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setL(0xb3);
    expectedCPU.setFlags(cpu_->FlagZ(), cpu_->FlagN(), cpu_->FlagH(), cpu_->FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0xb2);
    expectedCPU.setFlags(0, 1, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0xb1);
    expectedCPU.setFlags(0, 1, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0xb0);
    expectedCPU.setFlags(0, 1, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0xaf);
    expectedCPU.setFlags(0, 1, 1, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0xae);
    expectedCPU.setFlags(0, 1, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x01);
    expectedCPU.setFlags(expectedCPU.FlagZ(), expectedCPU.FlagN(), expectedCPU.FlagH(), expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x00);
    expectedCPU.setFlags(1, 1, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0xff);
    expectedCPU.setFlags(0, 1, 1, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x2E - LD L, d8
TEST_F(CPUTest, OpcodeTest_0x2E_LD_L_d8)
{
    Program program({
        {0x2e, 0x01}, // LD L, 0x01
        {0x2e, 0x3e}, // LD L, 0x3e
        {0x2e, 0x00}, // LD L, 0x00
        {0x2e, 0xab}, // LD L, 0xab
        {0x2e, 0xff}, // LD L, 0xff
        {0x2e, 0x1a}, // LD L, 0x1a
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);
    expectedCPUs[0]->setL(0x01);
    expectedCPUs[1]->setL(0x3e);
    expectedCPUs[2]->setL(0x00);
    expectedCPUs[3]->setL(0xab);
    expectedCPUs[4]->setL(0xff);
    expectedCPUs[5]->setL(0x1a);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x2F - CPL
TEST_F(CPUTest, OpcodeTest_0x2F_CPL)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x2f},       // CPL
        {0x3e, 0x00}, // LD A, 0x00
        {0x2f},       // CPL
        {0x3e, 0xff}, // LD A, 0xff
        {0x2f},       // CPL
        {0x3e, 0xb7}, // LD A, 0xff
        {0x2f},       // CPL
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xa5);
    expectedCPU.setFlags(expectedCPU.FlagZ(), 1, 1, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xff);
    expectedCPU.setFlags(expectedCPU.FlagZ(), 1, 1, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xff);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(expectedCPU.FlagZ(), 1, 1, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xb7);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x48);
    expectedCPU.setFlags(expectedCPU.FlagZ(), 1, 1, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x30 - JR NC, s8
TEST_F(CPUTest, OpcodeTest_0x30_JR_NC_s8_FWD)
{
    Program program({
        {0xaf},       // XOR A
        {0x30, 0x25}, // JR NC, 0x25
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);
    expectedCPUs[0]->setPC(0x0101);
    expectedCPUs[0]->setA(0x00);
    expectedCPUs[0]->setFlags(1, 0, 0, 0);

    expectedCPUs[1]->setPC(0x0128);
    expectedCPUs[1]->setA(0x00);
    expectedCPUs[1]->setFlags(1, 0, 0, 0);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

TEST_F(CPUTest, OpcodeTest_0x30_JR_NC_s8_BKWD)
{
    Program program({
        {0xaf},       // XOR A
        {0x30, 0xf6}, // JR NC, 0xf6
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);
    expectedCPUs[0]->setPC(0x0101);
    expectedCPUs[0]->setA(0x00);
    expectedCPUs[0]->setFlags(1, 0, 0, 0);

    expectedCPUs[1]->setPC(0x00f9);
    expectedCPUs[1]->setA(0x00);
    expectedCPUs[1]->setFlags(1, 0, 0, 0);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

TEST_F(CPUTest, OpcodeTest_0x30_JR_NC_s8_NO_JUMP)
{
    Program program({
        {0x3e, 0x80}, // LD A, 0x80
        {0x07},       // RLCA
        {0x30, 0x25}, // JR NC, 0x25
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);
    expectedCPUs[0]->setPC(0x0102);
    expectedCPUs[0]->setA(0x80);

    expectedCPUs[1]->setPC(0x0103);
    expectedCPUs[1]->setA(0x01);
    expectedCPUs[1]->setFlags(0, 0, 0, 1);

    expectedCPUs[2]->setPC(0x0105);
    expectedCPUs[2]->setA(0x01);
    expectedCPUs[2]->setFlags(0, 0, 0, 1);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

// 0x31 - LD SP, d16
TEST_F(CPUTest, OpcodeTest_0x31_LD_SP_d16)
{
    Program program({
        {0x31, 0x05, 0x01}, // LD SP, 0x0105
        {0x31, 0x25, 0x3e}, // LD SP, 0x3e25
        {0x31, 0x00, 0x00}, // LD SP, 0x0000
        {0x31, 0x34, 0xab}, // LD SP, 0xab34
        {0x31, 0xff, 0xff}, // LD SP, 0xffff
        {0x31, 0x34, 0x1a}, // LD SP, 0x1a34
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);
    expectedCPUs[0]->setSP(0x0105);
    expectedCPUs[1]->setSP(0x3e25);
    expectedCPUs[2]->setSP(0x0000);
    expectedCPUs[3]->setSP(0xab34);
    expectedCPUs[4]->setSP(0xffff);
    expectedCPUs[5]->setSP(0x1a34);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x32 - LD (HL-), A
TEST_F(CPUTest, OpcodeTest_0x32_LDD_DEREF_HL_A)
{
    Program program({
        {0x21, 0x06, 0xc3}, // LD HL, 0xc306
        {0x3e, 0x5a},       // LD A, 0x5a
        {0x32},             // LD (HL-), A
        {0x3e, 0x14},       // LD A, 0x14
        {0x32},             // LD (HL-), A
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setHL(0xc306);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc306, 0x5a);
    expectedCPU.setHL(0xc305);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x14);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc305, 0x14);
    expectedCPU.setHL(0xc304);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x33 - INC SP
TEST_F(CPUTest, OpcodeTest_0x33_INC_SP)
{
    // TODO: test overflow?
    Program program({
        {0x31, 0x34, 0x12}, // LD SP, 0x1234
        {0x33},             // INC SP
        {0x33},             // INC SP
        {0x31, 0xfe, 0xab}, // LD SP, 0xabfe
        {0x33},             // INC SP
        {0x33},             // INC SP
        {0x33},             // INC SP
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);
    expectedCPUs[0]->setSP(0x1234);
    expectedCPUs[1]->setSP(0x1235);
    expectedCPUs[2]->setSP(0x1236);
    expectedCPUs[3]->setSP(0xabfe);
    expectedCPUs[4]->setSP(0xabff);
    expectedCPUs[5]->setSP(0xac00);
    expectedCPUs[6]->setSP(0xac01);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x34 - INC (HL)
TEST_F(CPUTest, OpcodeTest_0x34_INC_DEREF_HL)
{
    // TODO: test overflow?
    Program program({
        {0x21, 0x07, 0xc3}, // LD HL, 0xc307
        {0x36, 0x3b},       // LD (HL), 0x3b
        {0x34},             // INC (HL)
        {0x34},             // INC (HL)
        {0x34},             // INC (HL)
        {0x34},             // INC (HL)
        {0x34},             // INC (HL)
        {0x36, 0xfe},       // LD (HL), 0xfe
        {0x34},             // INC (HL)
        {0x34},             // INC (HL)
        {0x34},             // INC (HL)
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setHL(0xc307);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x3b);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x3c);
    expectedCPU.setFlags(0, 0, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x3d);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x3e);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x3f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x40);
    expectedCPU.setFlags(0, 0, 1, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0xfe);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0xff);
    expectedCPU.setFlags(0, 0, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x00);
    expectedCPU.setFlags(1, 0, 1, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x01);
    expectedCPU.setFlags(0, 0, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x35 - DEC (HL)
TEST_F(CPUTest, OpcodeTest_0x35_DEC_DEREF_HL)
{
    // TODO: test overflow?
    Program program({
        {0x21, 0x07, 0xc3}, // LD HL, 0xc307
        {0x36, 0xb3},       // LD (HL), 0xb3
        {0x35},             // DEC (HL)
        {0x35},             // DEC (HL)
        {0x35},             // DEC (HL)
        {0x35},             // DEC (HL)
        {0x35},             // DEC (HL)
        {0x36, 0x01},       // LD (HL), 0x01
        {0x35},             // DEC (HL)
        {0x35},             // DEC (HL)
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setHL(0xc307);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0xb3);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0xb2);
    expectedCPU.setFlags(0, 1, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0xb1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0xb0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0xaf);
    expectedCPU.setFlags(0, 1, 1, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0xae);
    expectedCPU.setFlags(0, 1, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x01);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x00);
    expectedCPU.setFlags(1, 1, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0xff);
    expectedCPU.setFlags(0, 1, 1, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x36 - LD (HL), d8
TEST_F(CPUTest, OpcodeTest_0x36_LD_DEREF_HL_d8)
{
    Program program({
        {0x21, 0x07, 0xc3}, // LD HL, 0xc307
        {0x36, 0x01},       // LD (HL), 0x01
        {0x36, 0x3e},       // LD (HL), 0x3e
        {0x36, 0x00},       // LD (HL), 0x00
        {0x36, 0xab},       // LD (HL), 0xab
        {0x36, 0xff},       // LD (HL), 0xff
        {0x36, 0x1a},       // LD (HL), 0x1a
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setHL(0xc307);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x01);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x3e);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0xab);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0xff);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x1a);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x37 - SCF
TEST_F(CPUTest, OpcodeTest_0x37_SCF)
{
    Program program({
        {0x37}, // SCF
        {0xaf}, // XOR A
        {0x37}, // SCF
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x38 - JR C, s8
TEST_F(CPUTest, OpcodeTest_0x38_JR_C_s8_FWD)
{
    Program program({
        {0x3e, 0x80}, // LD A, 0x80
        {0x07},       // RLCA
        {0x38, 0x25}, // JR C, 0x25
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);
    expectedCPUs[0]->setPC(0x0102);
    expectedCPUs[0]->setA(0x80);

    expectedCPUs[1]->setPC(0x0103);
    expectedCPUs[1]->setA(0x01);
    expectedCPUs[1]->setFlags(0, 0, 0, 1);

    expectedCPUs[2]->setPC(0x012a);
    expectedCPUs[2]->setA(0x01);
    expectedCPUs[2]->setFlags(0, 0, 0, 1);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

TEST_F(CPUTest, OpcodeTest_0x38_JR_C_s8_BKWD)
{
    Program program({
        {0x3e, 0x80}, // LD A, 0x80
        {0x07},       // RLCA
        {0x38, 0xf6}, // JR C, 0xf6
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);
    expectedCPUs[0]->setPC(0x0102);
    expectedCPUs[0]->setA(0x80);

    expectedCPUs[1]->setPC(0x0103);
    expectedCPUs[1]->setA(0x01);
    expectedCPUs[1]->setFlags(0, 0, 0, 1);

    expectedCPUs[2]->setPC(0x00fb);
    expectedCPUs[2]->setA(0x01);
    expectedCPUs[2]->setFlags(0, 0, 0, 1);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

TEST_F(CPUTest, OpcodeTest_0x38_JR_C_s8_NO_JUMP)
{
    Program program({
        {0xaf},       // XOR A
        {0x38, 0x25}, // JR C, 0x25
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);
    expectedCPUs[0]->setPC(0x0101);
    expectedCPUs[0]->setA(0x00);
    expectedCPUs[0]->setFlags(1, 0, 0, 0);

    expectedCPUs[1]->setPC(0x0103);
    expectedCPUs[1]->setA(0x00);
    expectedCPUs[1]->setFlags(1, 0, 0, 0);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

// 0x39 - ADD HL, SP
TEST_F(CPUTest, OpcodeTest_0x39_ADD_HL_SP)
{
    Program program({
        {0x21, 0x05, 0x01}, // LD HL, 0x0105
        {0x31, 0x65, 0xa3}, // LD SP, 0xa365
        {0x39},             // ADD HL, SP
        {0x39},             // ADD HL, SP
        {0x21, 0xe4, 0x15}, // LD HL, 0x15e4
        {0x31, 0xe5, 0xa3}, // LD SP, 0xa3e5
        {0x39},             // ADD HL, SP
        {0x21, 0x00, 0x80}, // LD HL, 0x8000
        {0x31, 0x00, 0x80}, // LD SP, 0x8000
        {0x39},             // ADD HL, SP
        {0x21, 0x02, 0x4c}, // LD HL, 0x4c02
        {0x31, 0x05, 0x5b}, // LD SP, 0x5b05
        {0x39},             // ADD HL, SP
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setHL(0x0105);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xa365);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0xa46a);
    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0x47cf);
    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0x15e4);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xa3e5);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0xb9c9);
    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0x8000);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0x8000);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0x0000);
    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0x4c02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0x5b05);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0xa707);
    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x3A - LD A, (HL-)
TEST_F(CPUTest, OpcodeTest_0x3A_LDD_A_DEREF_HL)
{
    Program program({
        {0x21, 0x07, 0xc3}, // LD HL, 0xc307
        {0x3e, 0x5a},       // LD A, 0x5a
        {0x32},             // LD (HL-), A
        {0x23},             // INC HL
        {0x3e, 0x2b},       // LD A, 0x2b
        {0x3a},             // LD A, (HL-)
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setHL(0xc307);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x5a);
    expectedCPU.setHL(0xc306);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0xc307);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x2b);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPU.setHL(0xc306);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x3B - DEC SP
TEST_F(CPUTest, OpcodeTest_0x3B_DEC_SP)
{
    // TODO: test overflow?
    Program program({
        {0x31, 0x34, 0x12}, // LD SP, 0x1234
        {0x3b},             // DEC SP
        {0x3b},             // DEC SP
        {0x31, 0x01, 0xab}, // LD SP, 0xab01
        {0x3b},             // DEC SP
        {0x3b},             // DEC SP
        {0x3b},             // DEC SP
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);
    expectedCPUs[0]->setSP(0x1234);
    expectedCPUs[1]->setSP(0x1233);
    expectedCPUs[2]->setSP(0x1232);
    expectedCPUs[3]->setSP(0xab01);
    expectedCPUs[4]->setSP(0xab00);
    expectedCPUs[5]->setSP(0xaaff);
    expectedCPUs[6]->setSP(0xaafe);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x3C - INC A
TEST_F(CPUTest, OpcodeTest_0x3C_INC_A)
{
    // TODO: test overflow?
    Program program({
        {0x3e, 0x3b}, // LD A, 0x3b
        {0x3c},       // INC A
        {0x3c},       // INC A
        {0x3c},       // INC A
        {0x3c},       // INC A
        {0x3c},       // INC A
        {0x3e, 0xfe}, // LD A, 0xfe
        {0x3c},       // INC A
        {0x3c},       // INC A
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x3b);
    expectedCPU.setFlags(cpu_->FlagZ(), cpu_->FlagN(), cpu_->FlagH(), cpu_->FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPU.setFlags(0, 0, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3d);
    expectedCPU.setFlags(0, 0, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3e);
    expectedCPU.setFlags(0, 0, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3f);
    expectedCPU.setFlags(0, 0, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x40);
    expectedCPU.setFlags(0, 0, 1, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xfe);
    expectedCPU.setFlags(expectedCPU.FlagZ(), expectedCPU.FlagN(), expectedCPU.FlagH(), expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xff);
    expectedCPU.setFlags(0, 0, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 1, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x3D - DEC A
TEST_F(CPUTest, OpcodeTest_0x3D_DEC_A)
{
    // TODO: test overflow?
    Program program({
        {0x3e, 0xb3}, // LD A, 0xb3
        {0x3d},       // DEC A
        {0x3d},       // DEC A
        {0x3d},       // DEC A
        {0x3d},       // DEC A
        {0x3d},       // DEC A
        {0x3e, 0x01}, // LD A, 0x01
        {0x3d},       // DEC A
        {0x3d},       // DEC A
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0xb3);
    expectedCPU.setFlags(cpu_->FlagZ(), cpu_->FlagN(), cpu_->FlagH(), cpu_->FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xb2);
    expectedCPU.setFlags(0, 1, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xb1);
    expectedCPU.setFlags(0, 1, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xb0);
    expectedCPU.setFlags(0, 1, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xaf);
    expectedCPU.setFlags(0, 1, 1, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xae);
    expectedCPU.setFlags(0, 1, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x01);
    expectedCPU.setFlags(expectedCPU.FlagZ(), expectedCPU.FlagN(), expectedCPU.FlagH(), expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 1, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xff);
    expectedCPU.setFlags(0, 1, 1, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x3E - LD A, d8
TEST_F(CPUTest, OpcodeTest_0x3E_LD_A_d8)
{
    Program program({
        {0x3e, 0x01}, // LD A, 0x01
        {0x3e, 0x3e}, // LD A, 0x3e
        {0x3e, 0x00}, // LD A, 0x00
        {0x3e, 0xab}, // LD A, 0xab
        {0x3e, 0xff}, // LD A, 0xff
        {0x3e, 0x1a}, // LD A, 0x1a
    });

    loadSimpleProgram(program);

    ExpectedCPUs expectedCPUs(program.size(), *cpu_);
    expectedCPUs[0]->setA(0x01);
    expectedCPUs[1]->setA(0x3e);
    expectedCPUs[2]->setA(0x00);
    expectedCPUs[3]->setA(0xab);
    expectedCPUs[4]->setA(0xff);
    expectedCPUs[5]->setA(0x1a);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x3F - CCF
TEST_F(CPUTest, OpcodeTest_0x3F_CCF)
{
    Program program({
        {0x3F},       // CCF
        {0x3e, 0x80}, // LD A, 0x80
        {0x07},       // RLCA
        {0x3F},       // CCF
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, (expectedCPU.FlagC() == 1) ? 0 : 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x01);
    expectedCPU.setFlags(0, 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x40 - LD B, B
TEST_F(CPUTest, OpcodeTest_0x40_LD_B_B)
{
    Program program({
        {0x06, 0x5a}, // LD B, 0x5a
        {0x40},       // LD B, B
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setB(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x5a);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x41 - LD B, C
TEST_F(CPUTest, OpcodeTest_0x41_LD_B_C)
{
    Program program({
        {0x06, 0x5a}, // LD B, 0x5a
        {0x0e, 0x42}, // LD C, 0x42
        {0x41},       // LD B, C
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setB(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x42 - LD B, D
TEST_F(CPUTest, OpcodeTest_0x42_LD_B_D)
{
    Program program({
        {0x06, 0x5a}, // LD B, 0x5a
        {0x16, 0x42}, // LD D, 0x42
        {0x42},       // LD B, D
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setB(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x43 - LD B, E
TEST_F(CPUTest, OpcodeTest_0x43_LD_B_E)
{
    Program program({
        {0x06, 0x5a}, // LD B, 0x5a
        {0x1e, 0x42}, // LD E, 0x42
        {0x43},       // LD B, E
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setB(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x44 - LD B, H
TEST_F(CPUTest, OpcodeTest_0x44_LD_B_H)
{
    Program program({
        {0x06, 0x5a}, // LD B, 0x5a
        {0x26, 0x42}, // LD H, 0x42
        {0x44},       // LD B, H
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setB(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x45 - LD B, L
TEST_F(CPUTest, OpcodeTest_0x45_LD_B_L)
{
    Program program({
        {0x06, 0x5a}, // LD B, 0x5a
        {0x2e, 0x42}, // LD L, 0x42
        {0x45},       // LD B, L
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setB(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x46 - LD B, (HL)
TEST_F(CPUTest, OpcodeTest_0x46_LD_B_DEREF_HL)
{
    Program program({
        {0x06, 0x5a},       // LD B, 0x5a
        {0x21, 0x07, 0xc3}, // LD HL, 0xc307
        {0x36, 0x42},       // LD (HL), 0x42
        {0x46},             // LD B, (HL)
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setB(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0xc307);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x47 - LD B, A
TEST_F(CPUTest, OpcodeTest_0x47_LD_B_A)
{
    Program program({
        {0x06, 0x5a}, // LD B, 0x5a
        {0x3e, 0x42}, // LD A, 0x42
        {0x47},       // LD B, A
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setB(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x48 - LD C, B
TEST_F(CPUTest, OpcodeTest_0x48_LD_C_B)
{
    Program program({
        {0x0e, 0x5a}, // LD C, 0x5a
        {0x06, 0x42}, // LD B, 0x42
        {0x48},       // LD C, B
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setC(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x49 - LD C, C
TEST_F(CPUTest, OpcodeTest_0x49_LD_C_C)
{
    Program program({
        {0x0e, 0x5a}, // LD C, 0x5a
        {0x49},       // LD C, C
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setC(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x5a);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x4A - LD C, D
TEST_F(CPUTest, OpcodeTest_0x4A_LD_C_D)
{
    Program program({
        {0x0e, 0x5a}, // LD C, 0x5a
        {0x16, 0x42}, // LD D, 0x42
        {0x4a},       // LD C, D
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setC(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x4B - LD C, E
TEST_F(CPUTest, OpcodeTest_0x4B_LD_C_E)
{
    Program program({
        {0x0e, 0x5a}, // LD C, 0x5a
        {0x1e, 0x42}, // LD E, 0x42
        {0x4b},       // LD C, E
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setC(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x4C - LD C, H
TEST_F(CPUTest, OpcodeTest_0x4C_LD_C_H)
{
    Program program({
        {0x0e, 0x5a}, // LD C, 0x5a
        {0x26, 0x42}, // LD H, 0x42
        {0x4c},       // LD C, H
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setC(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x4D - LD C, L
TEST_F(CPUTest, OpcodeTest_0x4D_LD_C_L)
{
    Program program({
        {0x0e, 0x5a}, // LD C, 0x5a
        {0x2e, 0x42}, // LD L, 0x42
        {0x4d},       // LD C, L
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setC(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x4E - LD C, (HL)
TEST_F(CPUTest, OpcodeTest_0x4E_LD_C_DEREF_HL)
{
    Program program({
        {0x0e, 0x5a},       // LD C, 0x5a
        {0x21, 0x07, 0xc3}, // LD HL, 0xc307
        {0x36, 0x42},       // LD (HL), 0x42
        {0x4e},             // LD C, (HL)
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setC(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0xc307);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x4F - LD C, A
TEST_F(CPUTest, OpcodeTest_0x4F_LD_C_A)
{
    Program program({
        {0x0e, 0x5a}, // LD C, 0x5a
        {0x3e, 0x42}, // LD A, 0x42
        {0x4f},       // LD C, A
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setC(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x50 - LD D, B
TEST_F(CPUTest, OpcodeTest_0x50_LD_D_B)
{
    Program program({
        {0x16, 0x5a}, // LD D, 0x5a
        {0x06, 0x42}, // LD B, 0x42
        {0x50},       // LD D, B
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setD(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x51 - LD D, C
TEST_F(CPUTest, OpcodeTest_0x51_LD_D_C)
{
    Program program({
        {0x16, 0x5a}, // LD D, 0x5a
        {0x0e, 0x42}, // LD C, 0x42
        {0x51},       // LD D, C
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setD(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x52 - LD D, D
TEST_F(CPUTest, OpcodeTest_0x52_LD_D_D)
{
    Program program({
        {0x16, 0x5a}, // LD D, 0x5a
        {0x52},       // LD D, D
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setD(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x5a);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x53 - LD D, E
TEST_F(CPUTest, OpcodeTest_0x53_LD_D_E)
{
    Program program({
        {0x16, 0x5a}, // LD D, 0x5a
        {0x1e, 0x42}, // LD E, 0x42
        {0x53},       // LD D, E
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setD(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x54 - LD D, H
TEST_F(CPUTest, OpcodeTest_0x54_LD_D_H)
{
    Program program({
        {0x16, 0x5a}, // LD D, 0x5a
        {0x26, 0x42}, // LD H, 0x42
        {0x54},       // LD D, H
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setD(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x55 - LD D, L
TEST_F(CPUTest, OpcodeTest_0x55_LD_D_L)
{
    Program program({
        {0x16, 0x5a}, // LD D, 0x5a
        {0x2e, 0x42}, // LD L, 0x42
        {0x55},       // LD D, L
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setD(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x56 - LD D, (HL)
TEST_F(CPUTest, OpcodeTest_0x56_LD_D_DEREF_HL)
{
    Program program({
        {0x16, 0x5a},       // LD D, 0x5a
        {0x21, 0x07, 0xc3}, // LD HL, 0xc307
        {0x36, 0x42},       // LD (HL), 0x42
        {0x56},             // LD D, (HL)
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setD(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0xc307);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x57 - LD D, A
TEST_F(CPUTest, OpcodeTest_0x57_LD_D_A)
{
    Program program({
        {0x16, 0x5a}, // LD D, 0x5a
        {0x3e, 0x42}, // LD A, 0x42
        {0x57},       // LD D, A
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setD(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x58 - LD E, B
TEST_F(CPUTest, OpcodeTest_0x58_LD_E_B)
{
    Program program({
        {0x1e, 0x5a}, // LD E, 0x5a
        {0x06, 0x42}, // LD B, 0x42
        {0x58},       // LD E, B
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setE(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x59 - LD E, C
TEST_F(CPUTest, OpcodeTest_0x59_LD_E_C)
{
    Program program({
        {0x1e, 0x5a}, // LD E, 0x5a
        {0x0e, 0x42}, // LD C, 0x42
        {0x59},       // LD E, C
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setE(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x5A - LD E, D
TEST_F(CPUTest, OpcodeTest_0x5A_LD_E_D)
{
    Program program({
        {0x1e, 0x5a}, // LD E, 0x5a
        {0x16, 0x42}, // LD D, 0x42
        {0x5a},       // LD E, D
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setE(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x5B - LD E, E
TEST_F(CPUTest, OpcodeTest_0x5B_LD_E_E)
{
    Program program({
        {0x1e, 0x5a}, // LD E, 0x5a
        {0x5b},       // LD E, E
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setE(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x5a);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x5C - LD E, H
TEST_F(CPUTest, OpcodeTest_0x5C_LD_E_H)
{
    Program program({
        {0x1e, 0x5a}, // LD E, 0x5a
        {0x26, 0x42}, // LD H, 0x42
        {0x5c},       // LD E, H
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setE(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x5D - LD E, L
TEST_F(CPUTest, OpcodeTest_0x5D_LD_E_L)
{
    Program program({
        {0x1e, 0x5a}, // LD E, 0x5a
        {0x2e, 0x42}, // LD L, 0x42
        {0x5d},       // LD E, L
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setE(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x5E - LD E, (HL)
TEST_F(CPUTest, OpcodeTest_0x5E_LD_E_DEREF_HL)
{
    Program program({
        {0x1e, 0x5a},       // LD E, 0x5a
        {0x21, 0x07, 0xc3}, // LD HL, 0xc307
        {0x36, 0x42},       // LD (HL), 0x42
        {0x5e},             // LD E, (HL)
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setE(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0xc307);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x5F - LD E, A
TEST_F(CPUTest, OpcodeTest_0x5F_LD_E_A)
{
    Program program({
        {0x1e, 0x5a}, // LD E, 0x5a
        {0x3e, 0x42}, // LD A, 0x42
        {0x5f},       // LD E, A
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setE(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x60 - LD H, B
TEST_F(CPUTest, OpcodeTest_0x60_LD_H_B)
{
    Program program({
        {0x26, 0x5a}, // LD H, 0x5a
        {0x06, 0x42}, // LD B, 0x42
        {0x60},       // LD H, B
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setH(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x61 - LD H, C
TEST_F(CPUTest, OpcodeTest_0x61_LD_H_C)
{
    Program program({
        {0x26, 0x5a}, // LD H, 0x5a
        {0x0e, 0x42}, // LD C, 0x42
        {0x61},       // LD H, C
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setH(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x62 - LD H, D
TEST_F(CPUTest, OpcodeTest_0x62_LD_H_D)
{
    Program program({
        {0x26, 0x5a}, // LD H, 0x5a
        {0x16, 0x42}, // LD D, 0x42
        {0x62},       // LD H, D
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setH(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x63 - LD H, E
TEST_F(CPUTest, OpcodeTest_0x63_LD_H_E)
{
    Program program({
        {0x26, 0x5a}, // LD H, 0x5a
        {0x1e, 0x42}, // LD E, 0x42
        {0x63},       // LD H, E
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setH(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x64 - LD H, H
TEST_F(CPUTest, OpcodeTest_0x64_LD_H_H)
{
    Program program({
        {0x26, 0x5a}, // LD H, 0x5a
        {0x64},       // LD H, H
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setH(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x5a);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x65 - LD H, L
TEST_F(CPUTest, OpcodeTest_0x65_LD_H_L)
{
    Program program({
        {0x26, 0x5a}, // LD H, 0x5a
        {0x2e, 0x42}, // LD E, 0x42
        {0x65},       // LD H, E
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setH(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x66 - LD H, (HL)
TEST_F(CPUTest, OpcodeTest_0x66_LD_H_DEREF_HL)
{
    Program program({
        {0x26, 0x5a},       // LD H, 0x5a
        {0x21, 0x07, 0xc3}, // LD HL, 0xc307
        {0x36, 0x42},       // LD (HL), 0x42
        {0x66},             // LD H, (HL)
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setH(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0xc307);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x67 - LD H, A
TEST_F(CPUTest, OpcodeTest_0x67_LD_H_A)
{
    Program program({
        {0x26, 0x5a}, // LD H, 0x5a
        {0x3e, 0x42}, // LD A, 0x42
        {0x67},       // LD H, A
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setH(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x68 - LD L, B
TEST_F(CPUTest, OpcodeTest_0x68_LD_L_B)
{
    Program program({
        {0x2e, 0x5a}, // LD L, 0x5a
        {0x06, 0x42}, // LD B, 0x42
        {0x68},       // LD L, B
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setL(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x69 - LD L, C
TEST_F(CPUTest, OpcodeTest_0x69_LD_L_C)
{
    Program program({
        {0x2e, 0x5a}, // LD L, 0x5a
        {0x0e, 0x42}, // LD C, 0x42
        {0x69},       // LD L, C
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setL(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x6A - LD L, D
TEST_F(CPUTest, OpcodeTest_0x6A_LD_L_D)
{
    Program program({
        {0x2e, 0x5a}, // LD L, 0x5a
        {0x16, 0x42}, // LD D, 0x42
        {0x6a},       // LD L, D
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setL(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x6B - LD L, E
TEST_F(CPUTest, OpcodeTest_0x6B_LD_L_E)
{
    Program program({
        {0x2e, 0x5a}, // LD L, 0x5a
        {0x1e, 0x42}, // LD E, 0x42
        {0x6b},       // LD L, E
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setL(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x6C - LD L, H
TEST_F(CPUTest, OpcodeTest_0x6C_LD_L_H)
{
    Program program({
        {0x2e, 0x5a}, // LD L, 0x5a
        {0x26, 0x42}, // LD H, 0x42
        {0x6c},       // LD L, H
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setL(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x6D - LD L, L
TEST_F(CPUTest, OpcodeTest_0x6D_LD_L_L)
{
    Program program({
        {0x2e, 0x5a}, // LD L, 0x5a
        {0x6d},       // LD L, L
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setL(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x5a);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x6E - LD L, (HL)
TEST_F(CPUTest, OpcodeTest_0x6E_LD_L_DEREF_HL)
{
    Program program({
        {0x2e, 0x5a},       // LD L, 0x5a
        {0x21, 0x07, 0xc3}, // LD HL, 0xc307
        {0x36, 0x42},       // LD (HL), 0x42
        {0x6e},             // LD L, (HL)
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setL(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0xc307);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x6F - LD L, A
TEST_F(CPUTest, OpcodeTest_0x6F_LD_L_A)
{
    Program program({
        {0x2e, 0x5a}, // LD L, 0x5a
        {0x3e, 0x42}, // LD A, 0x42
        {0x6f},       // LD L, A
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setL(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x70 - LD (HL), B
TEST_F(CPUTest, OpcodeTest_0x70_LD_DEREF_HL_B)
{
    Program program({
        {0x21, 0x07, 0xc3}, // LD HL, 0xc307
        {0x36, 0x5a},       // LD (HL), 0x5a
        {0x06, 0x42},       // LD B, 0x42
        {0x70},             // LD (HL), B
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setHL(0xc307);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x71 - LD (HL), C
TEST_F(CPUTest, OpcodeTest_0x71_LD_DEREF_HL_C)
{
    Program program({
        {0x21, 0x07, 0xc3}, // LD HL, 0xc307
        {0x36, 0x5a},       // LD (HL), 0x5a
        {0x0e, 0x42},       // LD C, 0x42
        {0x71},             // LD (HL), C
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setHL(0xc307);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x72 - LD (HL), D
TEST_F(CPUTest, OpcodeTest_0x72_LD_DEREF_HL_D)
{
    Program program({
        {0x21, 0x07, 0xc3}, // LD HL, 0xc307
        {0x36, 0x5a},       // LD (HL), 0x5a
        {0x16, 0x42},       // LD D, 0x42
        {0x72},             // LD (HL), D
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setHL(0xc307);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x73 - LD (HL), E
TEST_F(CPUTest, OpcodeTest_0x73_LD_DEREF_HL_E)
{
    Program program({
        {0x21, 0x07, 0xc3}, // LD HL, 0xc307
        {0x36, 0x5a},       // LD (HL), 0x5a
        {0x1e, 0x42},       // LD E, 0x42
        {0x73},             // LD (HL), E
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setHL(0xc307);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x74 - LD (HL), H
TEST_F(CPUTest, OpcodeTest_0x74_LD_DEREF_HL_H)
{
    Program program({
        {0x21, 0x07, 0xc3}, // LD HL, 0xc307
        {0x36, 0x5a},       // LD (HL), 0x5a
        {0x74},             // LD (HL), H
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setHL(0xc307);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0xc3);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x75 - LD (HL), L
TEST_F(CPUTest, OpcodeTest_0x75_LD_DEREF_HL_L)
{
    Program program({
        {0x21, 0x07, 0xc3}, // LD HL, 0xc307
        {0x36, 0x5a},       // LD (HL), 0x5a
        {0x75},             // LD (HL), L
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setHL(0xc307);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x07);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x76 - HALT
TEST_F(CPUTest, OpcodeTest_0x76_HALT)
{
    // TODO: implement HALT instruction and its tests.
    // throw std::runtime_error("HALT tests not implemented");
}

// 0x77 - LD (HL), A
TEST_F(CPUTest, OpcodeTest_0x77_LD_DEREF_HL_A)
{
    Program program({
        {0x21, 0x07, 0xc3}, // LD HL, 0xc307
        {0x36, 0x5a},       // LD (HL), 0x5a
        {0x3e, 0x42},       // LD A, 0x42
        {0x77},             // LD (HL), A
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setHL(0xc307);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x78 - LD A, B
TEST_F(CPUTest, OpcodeTest_0x78_LD_A_B)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x06, 0x42}, // LD B, 0x42
        {0x78},       // LD A, B
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x79 - LD A, C
TEST_F(CPUTest, OpcodeTest_0x79_LD_A_C)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x0e, 0x42}, // LD C, 0x42
        {0x79},       // LD A, C
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x7A - LD A, D
TEST_F(CPUTest, OpcodeTest_0x7A_LD_A_D)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x16, 0x42}, // LD D, 0x42
        {0x7a},       // LD A, D
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x7B - LD A, E
TEST_F(CPUTest, OpcodeTest_0x7B_LD_A_E)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x1e, 0x42}, // LD E, 0x42
        {0x7b},       // LD A, E
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x7C - LD A, H
TEST_F(CPUTest, OpcodeTest_0x7C_LD_A_H)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x26, 0x42}, // LD H, 0x42
        {0x7c},       // LD A, H
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x7D - LD A, L
TEST_F(CPUTest, OpcodeTest_0x7D_LD_A_L)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x2e, 0x42}, // LD L, 0x42
        {0x7d},       // LD A, L
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x7E - LD A, (HL)
TEST_F(CPUTest, OpcodeTest_0x7E_LD_A_DEREF_HL)
{
    Program program({
        {0x3e, 0x5a},       // LD A, 0x5a
        {0x21, 0x07, 0xc3}, // LD HL, 0xc307
        {0x36, 0x42},       // LD (HL), 0x42
        {0x7e},             // LD A, (HL)
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0xc307);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x7F - LD A, A
TEST_F(CPUTest, OpcodeTest_0x7F_LD_A_A)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x7f},       // LD A, A
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x80 - ADD A, B
TEST_F(CPUTest, OpcodeTest_0x80_ADD_A_B)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x06, 0x25}, // LD B, 0x25
        {0x80},       // ADD A, B
        {0x3e, 0x80}, // LD A, 0x80
        {0x06, 0x40}, // LD B, 0x40
        {0x80},       // ADD A, B
        {0x80},       // ADD A, B
        {0x3e, 0x8f}, // LD A, 0x8f
        {0x06, 0x02}, // LD B, 0x02
        {0x80},       // ADD A, B
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x7f);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x40);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xc0);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x91);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x81 - ADD A, C
TEST_F(CPUTest, OpcodeTest_0x81_ADD_A_C)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x0e, 0x25}, // LD C, 0x25
        {0x81},       // ADD A, C
        {0x3e, 0x80}, // LD A, 0x80
        {0x0e, 0x40}, // LD C, 0x40
        {0x81},       // ADD A, C
        {0x81},       // ADD A, C
        {0x3e, 0x8f}, // LD A, 0x8f
        {0x0e, 0x02}, // LD C, 0x02
        {0x81},       // ADD A, C
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x7f);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x40);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xc0);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x91);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x82 - ADD A, D
TEST_F(CPUTest, OpcodeTest_0x82_ADD_A_D)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x16, 0x25}, // LD D, 0x25
        {0x82},       // ADD A, D
        {0x3e, 0x80}, // LD A, 0x80
        {0x16, 0x40}, // LD D, 0x40
        {0x82},       // ADD A, D
        {0x82},       // ADD A, D
        {0x3e, 0x8f}, // LD A, 0x8f
        {0x16, 0x02}, // LD D, 0x02
        {0x82},       // ADD A, D
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x7f);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x40);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xc0);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x91);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x83 - ADD A, E
TEST_F(CPUTest, OpcodeTest_0x83_ADD_A_E)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x1e, 0x25}, // LD E, 0x25
        {0x83},       // ADD A, E
        {0x3e, 0x80}, // LD A, 0x80
        {0x1e, 0x40}, // LD E, 0x40
        {0x83},       // ADD A, E
        {0x83},       // ADD A, E
        {0x3e, 0x8f}, // LD A, 0x8f
        {0x1e, 0x02}, // LD E, 0x02
        {0x83},       // ADD A, E
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x7f);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x40);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xc0);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x91);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x84 - ADD A, H
TEST_F(CPUTest, OpcodeTest_0x84_ADD_A_H)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x26, 0x25}, // LD H, 0x25
        {0x84},       // ADD A, H
        {0x3e, 0x80}, // LD A, 0x80
        {0x26, 0x40}, // LD H, 0x40
        {0x84},       // ADD A, H
        {0x84},       // ADD A, H
        {0x3e, 0x8f}, // LD A, 0x8f
        {0x26, 0x02}, // LD H, 0x02
        {0x84},       // ADD A, H
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x7f);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x40);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xc0);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x91);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x85 - ADD A, L
TEST_F(CPUTest, OpcodeTest_0x85_ADD_A_L)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x2e, 0x25}, // LD L, 0x25
        {0x85},       // ADD A, L
        {0x3e, 0x80}, // LD A, 0x80
        {0x2e, 0x40}, // LD L, 0x40
        {0x85},       // ADD A, L
        {0x85},       // ADD A, L
        {0x3e, 0x8f}, // LD A, 0x8f
        {0x2e, 0x02}, // LD L, 0x02
        {0x85},       // ADD A, L
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x7f);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x40);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xc0);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x91);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x86 - ADD A, (HL)
TEST_F(CPUTest, OpcodeTest_0x86_ADD_A_DEREF_HL)
{
    Program program({
        {0x21, 0x07, 0xc3}, // LD HL, 0xc307
        {0x3e, 0x5a},       // LD A, 0x5a
        {0x36, 0x25},       // LD (HL), 0x25
        {0x86},             // ADD A, (HL)
        {0x3e, 0x80},       // LD A, 0x80
        {0x36, 0x40},       // LD (HL), 0x40
        {0x86},             // ADD A, (HL)
        {0x86},             // ADD A, (HL)
        {0x3e, 0x8f},       // LD A, 0x8f
        {0x36, 0x02},       // LD (HL), 0x02
        {0x86},             // ADD A, (HL)
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setHL(0xc307);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x7f);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x40);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xc0);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x91);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x87 - ADD A, A
TEST_F(CPUTest, OpcodeTest_0x87_ADD_A_A)
{
    Program program({
        {0x3e, 0x07}, // LD A, 0x07
        {0x87},       // ADD A, A
        {0x3e, 0x80}, // LD A, 0x80
        {0x87},       // ADD A, A
        {0x3e, 0x0f}, // LD A, 0x0f
        {0x87},       // ADD A, A
        {0x3e, 0x92}, // LD A, 0x92
        {0x87},       // ADD A, A
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x07);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x0e);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x0f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x1e);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x92);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x24);
    expectedCPU.setFlags(0, 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x88 - ADC A, B
TEST_F(CPUTest, OpcodeTest_0x88_ADC_A_B)
{
    Program program({
        {0x37},       // SCF
        {0x3f},       // CCF
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x06, 0x25}, // LD B, 0x25
        {0x88},       // ADC A, B
        {0x3e, 0x80}, // LD A, 0x80
        {0x06, 0x40}, // LD B, 0x40
        {0x88},       // ADC A, B
        {0x88},       // ADC A, B
        {0x3e, 0x8f}, // LD A, 0x8f
        {0x06, 0x02}, // LD B, 0x02
        {0x88},       // ADC A, B
        {0x3e, 0xfd}, // LD A, 0xfd
        {0x06, 0x02}, // LD B, 0x02
        {0x37},       // SCF
        {0x88},       // ADC A, B
        {0x3e, 0x00}, // LD A, 0x00
        {0x06, 0x0f}, // LD B, 0x0f
        {0x37},       // SCF
        {0x88}        // ADC A, B
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x7f);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x40);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xc0);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x92);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xfd);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 1, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x0f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x10);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x89 - ADC A, C
TEST_F(CPUTest, OpcodeTest_0x89_ADC_A_C)
{
    Program program({
        {0x37},       // SCF
        {0x3f},       // CCF
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x0e, 0x25}, // LD C, 0x25
        {0x89},       // ADC A, C
        {0x3e, 0x80}, // LD A, 0x80
        {0x0e, 0x40}, // LD C, 0x40
        {0x89},       // ADC A, C
        {0x89},       // ADC A, C
        {0x3e, 0x8f}, // LD A, 0x8f
        {0x0e, 0x02}, // LD C, 0x02
        {0x89},       // ADC A, C
        {0x3e, 0xfd}, // LD A, 0xfd
        {0x0e, 0x02}, // LD C, 0x02
        {0x37},       // SCF
        {0x89},       // ADC A, C
        {0x3e, 0x00}, // LD A, 0x00
        {0x0e, 0x0f}, // LD C, 0x0f
        {0x37},       // SCF
        {0x89}        // ADC A, C
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x7f);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x40);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xc0);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x92);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xfd);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 1, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x0f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x10);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x8A - ADC A, D
TEST_F(CPUTest, OpcodeTest_0x8A_ADC_A_D)
{
    Program program({
        {0x37},       // SCF
        {0x3f},       // CCF
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x16, 0x25}, // LD D, 0x25
        {0x8a},       // ADC A, D
        {0x3e, 0x80}, // LD A, 0x80
        {0x16, 0x40}, // LD D, 0x40
        {0x8a},       // ADC A, D
        {0x8a},       // ADC A, D
        {0x3e, 0x8f}, // LD A, 0x8f
        {0x16, 0x02}, // LD D, 0x02
        {0x8a},       // ADC A, D
        {0x3e, 0xfd}, // LD A, 0xfd
        {0x16, 0x02}, // LD D, 0x02
        {0x37},       // SCF
        {0x8a},       // ADC A, D
        {0x3e, 0x00}, // LD A, 0x00
        {0x16, 0x0f}, // LD D, 0x0f
        {0x37},       // SCF
        {0x8a}        // ADC A, D
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x7f);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x40);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xc0);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x92);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xfd);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 1, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x0f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x10);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x8B - ADC A, E
TEST_F(CPUTest, OpcodeTest_0x8B_ADC_A_E)
{
    Program program({
        {0x37},       // SCF
        {0x3f},       // CCF
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x1e, 0x25}, // LD E, 0x25
        {0x8b},       // ADC A, E
        {0x3e, 0x80}, // LD A, 0x80
        {0x1e, 0x40}, // LD E, 0x40
        {0x8b},       // ADC A, E
        {0x8b},       // ADC A, E
        {0x3e, 0x8f}, // LD A, 0x8f
        {0x1e, 0x02}, // LD E, 0x02
        {0x8b},       // ADC A, E
        {0x3e, 0xfd}, // LD A, 0xfd
        {0x1e, 0x02}, // LD E, 0x02
        {0x37},       // SCF
        {0x8b},       // ADC A, E
        {0x3e, 0x00}, // LD A, 0x00
        {0x1e, 0x0f}, // LD E, 0x0f
        {0x37},       // SCF
        {0x8b}        // ADC A, E
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x7f);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x40);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xc0);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x92);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xfd);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 1, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x0f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x10);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x8C - ADC A, H
TEST_F(CPUTest, OpcodeTest_0x8C_ADC_A_H)
{
    Program program({
        {0x37},       // SCF
        {0x3f},       // CCF
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x26, 0x25}, // LD H, 0x25
        {0x8c},       // ADC A, H
        {0x3e, 0x80}, // LD A, 0x80
        {0x26, 0x40}, // LD H, 0x40
        {0x8c},       // ADC A, H
        {0x8c},       // ADC A, H
        {0x3e, 0x8f}, // LD A, 0x8f
        {0x26, 0x02}, // LD H, 0x02
        {0x8c},       // ADC A, H
        {0x3e, 0xfd}, // LD A, 0xfd
        {0x26, 0x02}, // LD H, 0x02
        {0x37},       // SCF
        {0x8c},       // ADC A, H
        {0x3e, 0x00}, // LD A, 0x00
        {0x26, 0x0f}, // LD H, 0x0f
        {0x37},       // SCF
        {0x8c}        // ADC A, H
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x7f);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x40);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xc0);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x92);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xfd);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 1, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x0f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x10);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x8D - ADC A, L
TEST_F(CPUTest, OpcodeTest_0x8D_ADC_A_L)
{
    Program program({
        {0x37},       // SCF
        {0x3f},       // CCF
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x2e, 0x25}, // LD L, 0x25
        {0x8d},       // ADC A, L
        {0x3e, 0x80}, // LD A, 0x80
        {0x2e, 0x40}, // LD L, 0x40
        {0x8d},       // ADC A, L
        {0x8d},       // ADC A, L
        {0x3e, 0x8f}, // LD A, 0x8f
        {0x2e, 0x02}, // LD L, 0x02
        {0x8d},       // ADC A, L
        {0x3e, 0xfd}, // LD A, 0xfd
        {0x2e, 0x02}, // LD L, 0x02
        {0x37},       // SCF
        {0x8d},       // ADC A, L
        {0x3e, 0x00}, // LD A, 0x00
        {0x2e, 0x0f}, // LD L, 0x0f
        {0x37},       // SCF
        {0x8d}        // ADC A, L
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x7f);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x40);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xc0);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x92);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xfd);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 1, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x0f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x10);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x8E - ADC A, (HL)
TEST_F(CPUTest, OpcodeTest_0x8E_ADC_A_DEREF_HL)
{
    Program program({
        {0x37},             // SCF
        {0x3f},             // CCF
        {0x21, 0x07, 0xc3}, // LD HL, 0xc307
        {0x3e, 0x5a},       // LD A, 0x5a
        {0x36, 0x25},       // LD (HL), 0x25
        {0x8e},             // ADC A, (HL)
        {0x3e, 0x80},       // LD A, 0x80
        {0x36, 0x40},       // LD (HL), 0x40
        {0x8e},             // ADC A, (HL)
        {0x8e},             // ADC A, (HL)
        {0x3e, 0x8f},       // LD A, 0x8f
        {0x36, 0x02},       // LD (HL), 0x02
        {0x8e},             // ADC A, (HL)
        {0x3e, 0xfd},       // LD A, 0xfd
        {0x36, 0x02},       // LD (HL), 0x02
        {0x37},             // SCF
        {0x8e},             // ADC A, (HL)
        {0x3e, 0x00},       // LD A, 0x00
        {0x36, 0x0f},       // LD (HL), 0x0f
        {0x37},             // SCF
        {0x8e}              // ADC A, (HL)
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0xc307);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x7f);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x40);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xc0);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x92);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xfd);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 1, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x0f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x10);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x8F - ADC A, A
TEST_F(CPUTest, OpcodeTest_0x8F_ADC_A_A)
{
    Program program({
        {0x37},       // SCF
        {0x3f},       // CCF
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x8f},       // ADC A, A
        {0x3e, 0x80}, // LD A, 0x80
        {0x8f},       // ADC A, A
        {0x8f},       // ADC A, A
        {0x3e, 0x8f}, // LD A, 0x8f
        {0x8f},       // ADC A, A
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xb4);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x01);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x1e);
    expectedCPU.setFlags(0, 0, 1, 1);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x90 - SUB A, B
TEST_F(CPUTest, OpcodeTest_0x90_SUB_A_B)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x06, 0x25}, // LD B, 0x25
        {0x90},       // SUB A, B
        {0x3e, 0x80}, // LD A, 0x80
        {0x06, 0x40}, // LD B, 0x40
        {0x90},       // SUB A, B
        {0x90},       // SUB A, B
        {0x90},       // SUB A, B
        {0x3e, 0x56}, // LD A, 0x56
        {0x06, 0x3e}, // LD B, 0x3e
        {0x90},       // SUB A, B
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x35);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x40);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x40);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xc0);
    expectedCPU.setFlags(0, 1, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x3e);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x18);
    expectedCPU.setFlags(0, 1, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x91 - SUB A, C
TEST_F(CPUTest, OpcodeTest_0x91_SUB_A_C)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x0e, 0x25}, // LD C, 0x25
        {0x91},       // SUB A, C
        {0x3e, 0x80}, // LD A, 0x80
        {0x0e, 0x40}, // LD C, 0x40
        {0x91},       // SUB A, C
        {0x91},       // SUB A, C
        {0x91},       // SUB A, C
        {0x3e, 0x56}, // LD A, 0x56
        {0x0e, 0x3e}, // LD C, 0x3e
        {0x91},       // SUB A, C
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x35);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x40);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x40);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xc0);
    expectedCPU.setFlags(0, 1, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x3e);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x18);
    expectedCPU.setFlags(0, 1, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x92 - SUB A, D
TEST_F(CPUTest, OpcodeTest_0x92_SUB_A_D)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x16, 0x25}, // LD D, 0x25
        {0x92},       // SUB A, D
        {0x3e, 0x80}, // LD A, 0x80
        {0x16, 0x40}, // LD D, 0x40
        {0x92},       // SUB A, D
        {0x92},       // SUB A, D
        {0x92},       // SUB A, D
        {0x3e, 0x56}, // LD A, 0x56
        {0x16, 0x3e}, // LD D, 0x3e
        {0x92},       // SUB A, D
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x35);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x40);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x40);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xc0);
    expectedCPU.setFlags(0, 1, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x3e);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x18);
    expectedCPU.setFlags(0, 1, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x93 - SUB A, E
TEST_F(CPUTest, OpcodeTest_0x93_SUB_A_E)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x1e, 0x25}, // LD E, 0x25
        {0x93},       // SUB A, E
        {0x3e, 0x80}, // LD A, 0x80
        {0x1e, 0x40}, // LD E, 0x40
        {0x93},       // SUB A, E
        {0x93},       // SUB A, E
        {0x93},       // SUB A, E
        {0x3e, 0x56}, // LD A, 0x56
        {0x1e, 0x3e}, // LD E, 0x3e
        {0x93},       // SUB A, E
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x35);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x40);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x40);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xc0);
    expectedCPU.setFlags(0, 1, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x3e);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x18);
    expectedCPU.setFlags(0, 1, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x94 - SUB A, H
TEST_F(CPUTest, OpcodeTest_0x94_SUB_A_H)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x26, 0x25}, // LD H, 0x25
        {0x94},       // SUB A, H
        {0x3e, 0x80}, // LD A, 0x80
        {0x26, 0x40}, // LD H, 0x40
        {0x94},       // SUB A, H
        {0x94},       // SUB A, H
        {0x94},       // SUB A, H
        {0x3e, 0x56}, // LD A, 0x56
        {0x26, 0x3e}, // LD H, 0x3e
        {0x94},       // SUB A, H
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x35);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x40);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x40);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xc0);
    expectedCPU.setFlags(0, 1, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x3e);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x18);
    expectedCPU.setFlags(0, 1, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x95 - SUB A, L
TEST_F(CPUTest, OpcodeTest_0x95_SUB_A_L)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x2e, 0x25}, // LD L, 0x25
        {0x95},       // SUB A, L
        {0x3e, 0x80}, // LD A, 0x80
        {0x2e, 0x40}, // LD L, 0x40
        {0x95},       // SUB A, L
        {0x95},       // SUB A, L
        {0x95},       // SUB A, L
        {0x3e, 0x56}, // LD A, 0x56
        {0x2e, 0x3e}, // LD L, 0x3e
        {0x95},       // SUB A, L
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x35);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x40);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x40);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xc0);
    expectedCPU.setFlags(0, 1, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x3e);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x18);
    expectedCPU.setFlags(0, 1, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x96 - SUB A, (HL)
TEST_F(CPUTest, OpcodeTest_0x96_SUB_A_DEREF_HL)
{
    Program program({
        {0x21, 0x07, 0xc3}, // LD HL, 0xc307
        {0x3e, 0x5a},       // LD A, 0x5a
        {0x36, 0x25},       // LD (HL), 0x25
        {0x96},             // SUB A, (HL)
        {0x3e, 0x80},       // LD A, 0x80
        {0x36, 0x40},       // LD (HL), 0x40
        {0x96},             // SUB A, (HL)
        {0x96},             // SUB A, (HL)
        {0x96},             // SUB A, (HL)
        {0x3e, 0x56},       // LD A, 0x56
        {0x36, 0x3e},       // LD (HL), 0x3e
        {0x96},             // SUB A, (HL)
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setHL(0xc307);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x35);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x40);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x40);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xc0);
    expectedCPU.setFlags(0, 1, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x3e);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x18);
    expectedCPU.setFlags(0, 1, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x97 - SUB A, A
TEST_F(CPUTest, OpcodeTest_0x97_SUB_A_A)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x97},       // SUB A, A
        {0x3e, 0x80}, // LD A, 0x80
        {0x97},       // SUB A, A
        {0x97},       // SUB A, A
        {0x3e, 0x56}, // LD A, 0x56
        {0x97},       // SUB A, A
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x98 - SBC A, B
TEST_F(CPUTest, OpcodeTest_0x98_SBC_A_B)
{
    Program program({
        {0x37},       // SCF
        {0x3f},       // CCF
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x06, 0x25}, // LD B, 0x25
        {0x98},       // SBC A, B
        {0x3e, 0x80}, // LD A, 0x80
        {0x06, 0x40}, // LD B, 0x40
        {0x98},       // SBC A, B
        {0x98},       // SBC A, B
        {0x98},       // SBC A, B
        {0x3e, 0x56}, // LD A, 0x56
        {0x06, 0x3e}, // LD B, 0x3e
        {0x98},       // SBC A, B
        {0x3e, 0x56}, // LD A, 0x56
        {0x06, 0x55}, // LD B, 0x55
        {0x37},       // SCF
        {0x98},       // SBC A, B
        {0x3e, 0x05}, // LD A, 0x05
        {0x06, 0x05}, // LD B, 0x05
        {0x37},       // SCF
        {0x98},       // SBC A, B
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x35);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x40);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x40);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xc0);
    expectedCPU.setFlags(0, 1, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x3e);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x17);
    expectedCPU.setFlags(0, 1, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x55);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x05);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x05);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xff);
    expectedCPU.setFlags(0, 1, 1, 1);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x99 - SBC A, C
TEST_F(CPUTest, OpcodeTest_0x99_SBC_A_C)
{
    Program program({
        {0x37},       // SCF
        {0x3f},       // CCF
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x0e, 0x25}, // LD C, 0x25
        {0x99},       // SBC A, C
        {0x3e, 0x80}, // LD A, 0x80
        {0x0e, 0x40}, // LD C, 0x40
        {0x99},       // SBC A, C
        {0x99},       // SBC A, C
        {0x99},       // SBC A, C
        {0x3e, 0x56}, // LD A, 0x56
        {0x0e, 0x3e}, // LD C, 0x3e
        {0x99},       // SBC A, C
        {0x3e, 0x56}, // LD A, 0x56
        {0x0e, 0x55}, // LD C, 0x55
        {0x37},       // SCF
        {0x99},       // SBC A, C
        {0x3e, 0x05}, // LD A, 0x05
        {0x0e, 0x05}, // LD C, 0x05
        {0x37},       // SCF
        {0x99},       // SBC A, C
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x35);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x40);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x40);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xc0);
    expectedCPU.setFlags(0, 1, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x3e);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x17);
    expectedCPU.setFlags(0, 1, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x55);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x05);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x05);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xff);
    expectedCPU.setFlags(0, 1, 1, 1);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x9A - SBC A, D
TEST_F(CPUTest, OpcodeTest_0x9A_SBC_A_D)
{
    Program program({
        {0x37},       // SCF
        {0x3f},       // CCF
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x16, 0x25}, // LD D, 0x25
        {0x9a},       // SBC A, D
        {0x3e, 0x80}, // LD A, 0x80
        {0x16, 0x40}, // LD D, 0x40
        {0x9a},       // SBC A, D
        {0x9a},       // SBC A, D
        {0x9a},       // SBC A, D
        {0x3e, 0x56}, // LD A, 0x56
        {0x16, 0x3e}, // LD D, 0x3e
        {0x9a},       // SBC A, D
        {0x3e, 0x56}, // LD A, 0x56
        {0x16, 0x55}, // LD D, 0x55
        {0x37},       // SCF
        {0x9a},       // SBC A, D
        {0x3e, 0x05}, // LD A, 0x05
        {0x16, 0x05}, // LD D, 0x05
        {0x37},       // SCF
        {0x9a},       // SBC A, D
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x35);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x40);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x40);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xc0);
    expectedCPU.setFlags(0, 1, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x3e);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x17);
    expectedCPU.setFlags(0, 1, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x55);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x05);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x05);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xff);
    expectedCPU.setFlags(0, 1, 1, 1);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x9B - SBC A, E
TEST_F(CPUTest, OpcodeTest_0x9B_SBC_A_E)
{
    Program program({
        {0x37},       // SCF
        {0x3f},       // CCF
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x1e, 0x25}, // LD E, 0x25
        {0x9b},       // SBC A, E
        {0x3e, 0x80}, // LD A, 0x80
        {0x1e, 0x40}, // LD E, 0x40
        {0x9b},       // SBC A, E
        {0x9b},       // SBC A, E
        {0x9b},       // SBC A, E
        {0x3e, 0x56}, // LD A, 0x56
        {0x1e, 0x3e}, // LD E, 0x3e
        {0x9b},       // SBC A, E
        {0x3e, 0x56}, // LD A, 0x56
        {0x1e, 0x55}, // LD E, 0x55
        {0x37},       // SCF
        {0x9b},       // SBC A, E
        {0x3e, 0x05}, // LD A, 0x05
        {0x1e, 0x05}, // LD E, 0x05
        {0x37},       // SCF
        {0x9b},       // SBC A, E
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x35);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x40);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x40);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xc0);
    expectedCPU.setFlags(0, 1, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x3e);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x17);
    expectedCPU.setFlags(0, 1, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x55);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x05);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x05);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xff);
    expectedCPU.setFlags(0, 1, 1, 1);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x9C - SBC A, H
TEST_F(CPUTest, OpcodeTest_0x9C_SBC_A_H)
{
    Program program({
        {0x37},       // SCF
        {0x3f},       // CCF
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x26, 0x25}, // LD H, 0x25
        {0x9c},       // SBC A, H
        {0x3e, 0x80}, // LD A, 0x80
        {0x26, 0x40}, // LD H, 0x40
        {0x9c},       // SBC A, H
        {0x9c},       // SBC A, H
        {0x9c},       // SBC A, H
        {0x3e, 0x56}, // LD A, 0x56
        {0x26, 0x3e}, // LD H, 0x3e
        {0x9c},       // SBC A, H
        {0x3e, 0x56}, // LD A, 0x56
        {0x26, 0x55}, // LD H, 0x55
        {0x37},       // SCF
        {0x9c},       // SBC A, H
        {0x3e, 0x05}, // LD A, 0x05
        {0x26, 0x05}, // LD H, 0x05
        {0x37},       // SCF
        {0x9c},       // SBC A, H
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x35);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x40);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x40);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xc0);
    expectedCPU.setFlags(0, 1, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x3e);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x17);
    expectedCPU.setFlags(0, 1, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x55);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x05);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x05);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xff);
    expectedCPU.setFlags(0, 1, 1, 1);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x9D - SBC A, L
TEST_F(CPUTest, OpcodeTest_0x9D_SBC_A_L)
{
    Program program({
        {0x37},       // SCF
        {0x3f},       // CCF
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x2e, 0x25}, // LD L, 0x25
        {0x9d},       // SBC A, L
        {0x3e, 0x80}, // LD A, 0x80
        {0x2e, 0x40}, // LD L, 0x40
        {0x9d},       // SBC A, L
        {0x9d},       // SBC A, L
        {0x9d},       // SBC A, L
        {0x3e, 0x56}, // LD A, 0x56
        {0x2e, 0x3e}, // LD L, 0x3e
        {0x9d},       // SBC A, L
        {0x3e, 0x56}, // LD A, 0x56
        {0x2e, 0x55}, // LD L, 0x55
        {0x37},       // SCF
        {0x9d},       // SBC A, L
        {0x3e, 0x05}, // LD A, 0x05
        {0x2e, 0x05}, // LD L, 0x05
        {0x37},       // SCF
        {0x9d},       // SBC A, L
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x35);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x40);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x40);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xc0);
    expectedCPU.setFlags(0, 1, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x3e);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x17);
    expectedCPU.setFlags(0, 1, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x55);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x05);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x05);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xff);
    expectedCPU.setFlags(0, 1, 1, 1);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x9E - SBC A, (HL)
TEST_F(CPUTest, OpcodeTest_0x9E_SBC_A_DEREF_HL)
{
    Program program({
        {0x37},             // SCF
        {0x3f},             // CCF
        {0x21, 0x07, 0xc3}, // LD HL, 0xc307
        {0x3e, 0x5a},       // LD A, 0x5a
        {0x36, 0x25},       // LD (HL), 0x25
        {0x9e},             // SBC A, (HL)
        {0x3e, 0x80},       // LD A, 0x80
        {0x36, 0x40},       // LD (HL), 0x40
        {0x9e},             // SBC A, (HL)
        {0x9e},             // SBC A, (HL)
        {0x9e},             // SBC A, (HL)
        {0x3e, 0x56},       // LD A, 0x56
        {0x36, 0x3e},       // LD (HL), 0x3e
        {0x9e},             // SBC A, (HL)
        {0x3e, 0x56},       // LD A, 0x56
        {0x36, 0x55},       // LD (HL), 0x55
        {0x37},             // SCF
        {0x9e},             // SBC A, (HL)
        {0x3e, 0x05},       // LD A, 0x05
        {0x36, 0x05},       // LD (HL), 0x05
        {0x37},             // SCF
        {0x9e},             // SBC A, (HL)
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0xc307);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x35);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x40);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x40);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xc0);
    expectedCPU.setFlags(0, 1, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x3e);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x17);
    expectedCPU.setFlags(0, 1, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x55);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x05);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x05);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xff);
    expectedCPU.setFlags(0, 1, 1, 1);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0x9F - SBC A, A
TEST_F(CPUTest, OpcodeTest_0x9F_SBC_A_A)
{
    Program program({
        {0x37},       // SCF
        {0x3f},       // CCF
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x9f},       // SBC A, A
        {0x3e, 0x80}, // LD A, 0x80
        {0x9f},       // SBC A, A
        {0x9f},       // SBC A, A
        {0x3e, 0x56}, // LD A, 0x56
        {0x9f},       // SBC A, A
        {0x3e, 0x56}, // LD A, 0x56
        {0x37},       // SCF
        {0x9f},       // SBC A, A
        {0x3e, 0x05}, // LD A, 0x05
        {0x37},       // SCF
        {0x9f},       // SBC A, A
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xff);
    expectedCPU.setFlags(0, 1, 1, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x05);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xff);
    expectedCPU.setFlags(0, 1, 1, 1);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xA0 - AND A, B
TEST_F(CPUTest, OpcodeTest_0xA0_AND_A_B)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x06, 0x25}, // LD B, 0x25
        {0xa0},       // AND A, B
        {0x3e, 0xf5}, // LD A, 0xf5
        {0x06, 0x73}, // LD B, 0x73
        {0xa0},       // AND A, B
        {0xa0},       // AND A, B
        {0x3e, 0x8f}, // LD A, 0x8f
        {0x06, 0x02}, // LD B, 0x02
        {0xa0},       // AND A, B
        {0x3e, 0x3c}, // LD A, 0x3c
        {0x06, 0x00}, // LD B, 0x00
        {0xa0},       // AND A, B
        {0x3e, 0x69}, // LD A, 0x69
        {0x06, 0xff}, // LD B, 0xff
        {0xa0},       // AND A, B
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf5);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x73);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x71);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x71);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x02);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x69);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0xff);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x69);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xA1 - AND A, C
TEST_F(CPUTest, OpcodeTest_0xA1_AND_A_C)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x0e, 0x25}, // LD C, 0x25
        {0xa1},       // AND A, C
        {0x3e, 0xf5}, // LD A, 0xf5
        {0x0e, 0x73}, // LD C, 0x73
        {0xa1},       // AND A, C
        {0xa1},       // AND A, C
        {0x3e, 0x8f}, // LD A, 0x8f
        {0x0e, 0x02}, // LD C, 0x02
        {0xa1},       // AND A, C
        {0x3e, 0x3c}, // LD A, 0x3c
        {0x0e, 0x00}, // LD C, 0x00
        {0xa1},       // AND A, C
        {0x3e, 0x69}, // LD A, 0x69
        {0x0e, 0xff}, // LD C, 0xff
        {0xa1},       // AND A, C
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf5);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x73);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x71);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x71);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x02);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x69);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0xff);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x69);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xA2 - AND A, D
TEST_F(CPUTest, OpcodeTest_0xA2_AND_A_D)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x16, 0x25}, // LD D, 0x25
        {0xa2},       // AND A, D
        {0x3e, 0xf5}, // LD A, 0xf5
        {0x16, 0x73}, // LD D, 0x73
        {0xa2},       // AND A, D
        {0xa2},       // AND A, D
        {0x3e, 0x8f}, // LD A, 0x8f
        {0x16, 0x02}, // LD D, 0x02
        {0xa2},       // AND A, D
        {0x3e, 0x3c}, // LD A, 0x3c
        {0x16, 0x00}, // LD D, 0x00
        {0xa2},       // AND A, D
        {0x3e, 0x69}, // LD A, 0x69
        {0x16, 0xff}, // LD D, 0xff
        {0xa2},       // AND A, D
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf5);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x73);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x71);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x71);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x02);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x69);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0xff);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x69);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xA3 - AND A, E
TEST_F(CPUTest, OpcodeTest_0xA3_AND_A_E)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x1e, 0x25}, // LD E, 0x25
        {0xa3},       // AND A, E
        {0x3e, 0xf5}, // LD A, 0xf5
        {0x1e, 0x73}, // LD E, 0x73
        {0xa3},       // AND A, E
        {0xa3},       // AND A, E
        {0x3e, 0x8f}, // LD A, 0x8f
        {0x1e, 0x02}, // LD E, 0x02
        {0xa3},       // AND A, E
        {0x3e, 0x3c}, // LD A, 0x3c
        {0x1e, 0x00}, // LD E, 0x00
        {0xa3},       // AND A, E
        {0x3e, 0x69}, // LD A, 0x69
        {0x1e, 0xff}, // LD E, 0xff
        {0xa3},       // AND A, E
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf5);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x73);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x71);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x71);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x02);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x69);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0xff);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x69);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xA4 - AND A, H
TEST_F(CPUTest, OpcodeTest_0xA4_AND_A_H)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x26, 0x25}, // LD H, 0x25
        {0xa4},       // AND A, H
        {0x3e, 0xf5}, // LD A, 0xf5
        {0x26, 0x73}, // LD H, 0x73
        {0xa4},       // AND A, H
        {0xa4},       // AND A, H
        {0x3e, 0x8f}, // LD A, 0x8f
        {0x26, 0x02}, // LD H, 0x02
        {0xa4},       // AND A, H
        {0x3e, 0x3c}, // LD A, 0x3c
        {0x26, 0x00}, // LD H, 0x00
        {0xa4},       // AND A, H
        {0x3e, 0x69}, // LD A, 0x69
        {0x26, 0xff}, // LD H, 0xff
        {0xa4},       // AND A, H
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf5);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x73);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x71);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x71);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x02);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x69);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0xff);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x69);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xA5 - AND A, L
TEST_F(CPUTest, OpcodeTest_0xA5_AND_A_L)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x2e, 0x25}, // LD L, 0x25
        {0xa5},       // AND A, L
        {0x3e, 0xf5}, // LD A, 0xf5
        {0x2e, 0x73}, // LD L, 0x73
        {0xa5},       // AND A, L
        {0xa5},       // AND A, L
        {0x3e, 0x8f}, // LD A, 0x8f
        {0x2e, 0x02}, // LD L, 0x02
        {0xa5},       // AND A, L
        {0x3e, 0x3c}, // LD A, 0x3c
        {0x2e, 0x00}, // LD L, 0x00
        {0xa5},       // AND A, L
        {0x3e, 0x69}, // LD A, 0x69
        {0x2e, 0xff}, // LD L, 0xff
        {0xa5},       // AND A, L
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf5);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x73);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x71);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x71);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x02);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x69);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0xff);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x69);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xA6 - AND A, (HL)
TEST_F(CPUTest, OpcodeTest_0xA6_AND_A_DEREF_HL)
{
    Program program({
        {0x21, 0x07, 0xc3}, // LD HL, 0xc307
        {0x3e, 0x5a},       // LD A, 0x5a
        {0x36, 0x25},       // LD (HL), 0x25
        {0xa6},             // AND A, (HL)
        {0x3e, 0xf5},       // LD A, 0xf5
        {0x36, 0x73},       // LD (HL), 0x73
        {0xa6},             // AND A, (HL)
        {0xa6},             // AND A, (HL)
        {0x3e, 0x8f},       // LD A, 0x8f
        {0x36, 0x02},       // LD (HL), 0x02
        {0xa6},             // AND A, (HL)
        {0x3e, 0x3c},       // LD A, 0x3c
        {0x36, 0x00},       // LD (HL), 0x00
        {0xa6},             // AND A, (HL)
        {0x3e, 0x69},       // LD A, 0x69
        {0x36, 0xff},       // LD (HL), 0xff
        {0xa6},             // AND A, (HL)
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setHL(0xc307);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf5);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x73);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x71);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x71);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x02);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x69);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0xff);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x69);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xA7 - AND A, A
TEST_F(CPUTest, OpcodeTest_0xA7_AND_A_A)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0xa7},       // AND A, A
        {0x3e, 0xf5}, // LD A, 0xf5
        {0xa7},       // AND A, A
        {0xa7},       // AND A, A
        {0x3e, 0x8f}, // LD A, 0x8f
        {0xa7},       // AND A, A
        {0x3e, 0x3c}, // LD A, 0x3c
        {0xa7},       // AND A, A
        {0x3e, 0x00}, // LD A, 0x00
        {0xa7},       // AND A, A
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf5);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf5);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf5);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xA8 - XOR A, B
TEST_F(CPUTest, OpcodeTest_0xA8_XOR_A_B)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x06, 0x25}, // LD B, 0x25
        {0xa8},       // XOR A, B
        {0x3e, 0xf5}, // LD A, 0xf5
        {0x06, 0x73}, // LD B, 0x73
        {0xa8},       // XOR A, B
        {0xa8},       // XOR A, B
        {0x3e, 0x8f}, // LD A, 0x8f
        {0x06, 0x02}, // LD B, 0x02
        {0xa8},       // XOR A, B
        {0x3e, 0x3c}, // LD A, 0x3c
        {0x06, 0x00}, // LD B, 0x00
        {0xa8},       // XOR A, B
        {0x3e, 0x69}, // LD A, 0x69
        {0x06, 0xff}, // LD B, 0xff
        {0xa8},       // XOR A, B
        {0x3e, 0x75}, // LD A, 0x75
        {0x06, 0x75}, // LD B, 0x75
        {0xa8},       // XOR A, B
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x7f);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf5);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x73);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x86);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf5);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8d);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x69);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0xff);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x96);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x75);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x75);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xA9 - XOR A, C
TEST_F(CPUTest, OpcodeTest_0xA9_XOR_A_C)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x0e, 0x25}, // LD C, 0x25
        {0xa9},       // XOR A, C
        {0x3e, 0xf5}, // LD A, 0xf5
        {0x0e, 0x73}, // LD C, 0x73
        {0xa9},       // XOR A, C
        {0xa9},       // XOR A, C
        {0x3e, 0x8f}, // LD A, 0x8f
        {0x0e, 0x02}, // LD C, 0x02
        {0xa9},       // XOR A, C
        {0x3e, 0x3c}, // LD A, 0x3c
        {0x0e, 0x00}, // LD C, 0x00
        {0xa9},       // XOR A, C
        {0x3e, 0x69}, // LD A, 0x69
        {0x0e, 0xff}, // LD C, 0xff
        {0xa9},       // XOR A, C
        {0x3e, 0x75}, // LD A, 0x75
        {0x0e, 0x75}, // LD B, 0x75
        {0xa9},       // XOR A, C
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x7f);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf5);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x73);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x86);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf5);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8d);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x69);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0xff);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x96);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x75);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x75);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xAA - XOR A, D
TEST_F(CPUTest, OpcodeTest_0xAA_XOR_A_D)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x16, 0x25}, // LD D, 0x25
        {0xaa},       // XOR A, D
        {0x3e, 0xf5}, // LD A, 0xf5
        {0x16, 0x73}, // LD D, 0x73
        {0xaa},       // XOR A, D
        {0xaa},       // XOR A, D
        {0x3e, 0x8f}, // LD A, 0x8f
        {0x16, 0x02}, // LD D, 0x02
        {0xaa},       // XOR A, D
        {0x3e, 0x3c}, // LD A, 0x3c
        {0x16, 0x00}, // LD D, 0x00
        {0xaa},       // XOR A, D
        {0x3e, 0x69}, // LD A, 0x69
        {0x16, 0xff}, // LD D, 0xff
        {0xaa},       // XOR A, D
        {0x3e, 0x75}, // LD A, 0x75
        {0x16, 0x75}, // LD D, 0x75
        {0xaa},       // XOR A, D
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x7f);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf5);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x73);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x86);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf5);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8d);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x69);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0xff);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x96);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x75);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x75);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xAB - XOR A, E
TEST_F(CPUTest, OpcodeTest_0xAB_XOR_A_E)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x1e, 0x25}, // LD E, 0x25
        {0xab},       // XOR A, E
        {0x3e, 0xf5}, // LD A, 0xf5
        {0x1e, 0x73}, // LD E, 0x73
        {0xab},       // XOR A, E
        {0xab},       // XOR A, E
        {0x3e, 0x8f}, // LD A, 0x8f
        {0x1e, 0x02}, // LD E, 0x02
        {0xab},       // XOR A, E
        {0x3e, 0x3c}, // LD A, 0x3c
        {0x1e, 0x00}, // LD E, 0x00
        {0xab},       // XOR A, E
        {0x3e, 0x69}, // LD A, 0x69
        {0x1e, 0xff}, // LD E, 0xff
        {0xab},       // XOR A, E
        {0x3e, 0x75}, // LD A, 0x75
        {0x1e, 0x75}, // LD E, 0x75
        {0xab},       // XOR A, E
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x7f);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf5);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x73);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x86);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf5);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8d);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x69);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0xff);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x96);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x75);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x75);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xAC - XOR A, H
TEST_F(CPUTest, OpcodeTest_0xAC_XOR_A_H)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x26, 0x25}, // LD H, 0x25
        {0xac},       // XOR A, H
        {0x3e, 0xf5}, // LD A, 0xf5
        {0x26, 0x73}, // LD H, 0x73
        {0xac},       // XOR A, H
        {0xac},       // XOR A, H
        {0x3e, 0x8f}, // LD A, 0x8f
        {0x26, 0x02}, // LD H, 0x02
        {0xac},       // XOR A, H
        {0x3e, 0x3c}, // LD A, 0x3c
        {0x26, 0x00}, // LD H, 0x00
        {0xac},       // XOR A, H
        {0x3e, 0x69}, // LD A, 0x69
        {0x26, 0xff}, // LD H, 0xff
        {0xac},       // XOR A, H
        {0x3e, 0x75}, // LD A, 0x75
        {0x26, 0x75}, // LD H, 0x75
        {0xac},       // XOR A, H
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x7f);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf5);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x73);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x86);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf5);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8d);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x69);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0xff);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x96);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x75);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x75);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xAD - XOR A, L
TEST_F(CPUTest, OpcodeTest_0xAD_XOR_A_L)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x2e, 0x25}, // LD L, 0x25
        {0xad},       // XOR A, L
        {0x3e, 0xf5}, // LD A, 0xf5
        {0x2e, 0x73}, // LD L, 0x73
        {0xad},       // XOR A, L
        {0xad},       // XOR A, L
        {0x3e, 0x8f}, // LD A, 0x8f
        {0x2e, 0x02}, // LD L, 0x02
        {0xad},       // XOR A, L
        {0x3e, 0x3c}, // LD A, 0x3c
        {0x2e, 0x00}, // LD L, 0x00
        {0xad},       // XOR A, L
        {0x3e, 0x69}, // LD A, 0x69
        {0x2e, 0xff}, // LD L, 0xff
        {0xad},       // XOR A, L
        {0x3e, 0x75}, // LD A, 0x75
        {0x2e, 0x75}, // LD L, 0x75
        {0xad},       // XOR A, L
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x7f);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf5);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x73);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x86);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf5);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8d);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x69);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0xff);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x96);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x75);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x75);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xAE - XOR A, (HL)
TEST_F(CPUTest, OpcodeTest_0xAE_XOR_A_DEREF_HL)
{
    Program program({
        {0x21, 0x07, 0xc3}, // LD HL, 0xc307
        {0x3e, 0x5a},       // LD A, 0x5a
        {0x36, 0x25},       // LD (HL), 0x25
        {0xae},             // XOR A, (HL)
        {0x3e, 0xf5},       // LD A, 0xf5
        {0x36, 0x73},       // LD (HL), 0x73
        {0xae},             // XOR A, (HL)
        {0xae},             // XOR A, (HL)
        {0x3e, 0x8f},       // LD A, 0x8f
        {0x36, 0x02},       // LD (HL), 0x02
        {0xae},             // XOR A, (HL)
        {0x3e, 0x3c},       // LD A, 0x3c
        {0x36, 0x00},       // LD (HL), 0x00
        {0xae},             // XOR A, (HL)
        {0x3e, 0x69},       // LD A, 0x69
        {0x36, 0xff},       // LD (HL), 0xff
        {0xae},             // XOR A, (HL)
        {0x3e, 0x75},       // LD A, 0x75
        {0x36, 0x75},       // LD (HL), 0x75
        {0xae},             // XOR A, (HL)
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setHL(0xc307);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x7f);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf5);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x73);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x86);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf5);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8d);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x69);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0xff);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x96);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x75);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x75);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xAF - XOR A, A
TEST_F(CPUTest, OpcodeTest_0xAF_XOR_A_A)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0xaf},       // XOR A, A
        {0x3e, 0xf5}, // LD A, 0xf5
        {0xaf},       // XOR A, A
        {0xaf},       // XOR A, A
        {0x3e, 0x8f}, // LD A, 0x8f
        {0xaf},       // XOR A, A
        {0x3e, 0x3c}, // LD A, 0x3c
        {0xaf},       // XOR A, A
        {0x3e, 0x69}, // LD A, 0x69
        {0xaf},       // XOR A, A
        {0x3e, 0x75}, // LD A, 0x75
        {0xaf},       // XOR A, A
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf5);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x69);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x75);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xB0 - OR A, B
TEST_F(CPUTest, OpcodeTest_0xB0_OR_A_B)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x06, 0x25}, // LD B, 0x25
        {0xb0},       // OR A, B
        {0x3e, 0xf5}, // LD A, 0xf5
        {0x06, 0x73}, // LD B, 0x73
        {0xb0},       // OR A, B
        {0xb0},       // OR A, B
        {0x3e, 0x8f}, // LD A, 0x8f
        {0x06, 0x02}, // LD B, 0x02
        {0xb0},       // OR A, B
        {0x3e, 0x3c}, // LD A, 0x3c
        {0x06, 0x00}, // LD B, 0x00
        {0xb0},       // OR A, B
        {0x3e, 0x69}, // LD A, 0x69
        {0x06, 0xff}, // LD B, 0xff
        {0xb0},       // OR A, B
        {0x3e, 0x75}, // LD A, 0x75
        {0x06, 0x75}, // LD B, 0x75
        {0xb0},       // OR A, B
        {0x3e, 0x00}, // LD A, 0x00
        {0x06, 0x00}, // LD B, 0x00
        {0xb0},       // OR A, B
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x7f);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf5);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x73);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf7);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf7);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x69);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0xff);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xff);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x75);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x75);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x75);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xB1 - OR A, C
TEST_F(CPUTest, OpcodeTest_0xB1_OR_A_C)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x0e, 0x25}, // LD C, 0x25
        {0xb1},       // OR A, C
        {0x3e, 0xf5}, // LD A, 0xf5
        {0x0e, 0x73}, // LD C, 0x73
        {0xb1},       // OR A, C
        {0xb1},       // OR A, C
        {0x3e, 0x8f}, // LD A, 0x8f
        {0x0e, 0x02}, // LD C, 0x02
        {0xb1},       // OR A, C
        {0x3e, 0x3c}, // LD A, 0x3c
        {0x0e, 0x00}, // LD C, 0x00
        {0xb1},       // OR A, C
        {0x3e, 0x69}, // LD A, 0x69
        {0x0e, 0xff}, // LD C, 0xff
        {0xb1},       // OR A, C
        {0x3e, 0x75}, // LD A, 0x75
        {0x0e, 0x75}, // LD C, 0x75
        {0xb1},       // OR A, C
        {0x3e, 0x00}, // LD A, 0x00
        {0x0e, 0x00}, // LD C, 0x00
        {0xb1},       // OR A, C
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x7f);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf5);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x73);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf7);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf7);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x69);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0xff);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xff);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x75);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x75);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x75);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xB2 - OR A, D
TEST_F(CPUTest, OpcodeTest_0xB2_OR_A_D)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x16, 0x25}, // LD D, 0x25
        {0xb2},       // OR A, D
        {0x3e, 0xf5}, // LD A, 0xf5
        {0x16, 0x73}, // LD D, 0x73
        {0xb2},       // OR A, D
        {0xb2},       // OR A, D
        {0x3e, 0x8f}, // LD A, 0x8f
        {0x16, 0x02}, // LD D, 0x02
        {0xb2},       // OR A, D
        {0x3e, 0x3c}, // LD A, 0x3c
        {0x16, 0x00}, // LD D, 0x00
        {0xb2},       // OR A, D
        {0x3e, 0x69}, // LD A, 0x69
        {0x16, 0xff}, // LD D, 0xff
        {0xb2},       // OR A, D
        {0x3e, 0x75}, // LD A, 0x75
        {0x16, 0x75}, // LD D, 0x75
        {0xb2},       // OR A, D
        {0x3e, 0x00}, // LD A, 0x00
        {0x16, 0x00}, // LD D, 0x00
        {0xb2},       // OR A, D
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x7f);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf5);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x73);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf7);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf7);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x69);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0xff);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xff);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x75);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x75);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x75);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xB3 - OR A, E
TEST_F(CPUTest, OpcodeTest_0xB3_OR_A_E)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x1e, 0x25}, // LD E, 0x25
        {0xb3},       // OR A, E
        {0x3e, 0xf5}, // LD A, 0xf5
        {0x1e, 0x73}, // LD E, 0x73
        {0xb3},       // OR A, E
        {0xb3},       // OR A, E
        {0x3e, 0x8f}, // LD A, 0x8f
        {0x1e, 0x02}, // LD E, 0x02
        {0xb3},       // OR A, E
        {0x3e, 0x3c}, // LD A, 0x3c
        {0x1e, 0x00}, // LD E, 0x00
        {0xb3},       // OR A, E
        {0x3e, 0x69}, // LD A, 0x69
        {0x1e, 0xff}, // LD E, 0xff
        {0xb3},       // OR A, E
        {0x3e, 0x75}, // LD A, 0x75
        {0x1e, 0x75}, // LD E, 0x75
        {0xb3},       // OR A, E
        {0x3e, 0x00}, // LD A, 0x00
        {0x1e, 0x00}, // LD E, 0x00
        {0xb3},       // OR A, E
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x7f);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf5);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x73);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf7);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf7);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x69);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0xff);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xff);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x75);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x75);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x75);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xB4 - OR A, H
TEST_F(CPUTest, OpcodeTest_0xB4_OR_A_H)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x26, 0x25}, // LD H, 0x25
        {0xb4},       // OR A, H
        {0x3e, 0xf5}, // LD A, 0xf5
        {0x26, 0x73}, // LD H, 0x73
        {0xb4},       // OR A, H
        {0xb4},       // OR A, H
        {0x3e, 0x8f}, // LD A, 0x8f
        {0x26, 0x02}, // LD H, 0x02
        {0xb4},       // OR A, H
        {0x3e, 0x3c}, // LD A, 0x3c
        {0x26, 0x00}, // LD H, 0x00
        {0xb4},       // OR A, H
        {0x3e, 0x69}, // LD A, 0x69
        {0x26, 0xff}, // LD H, 0xff
        {0xb4},       // OR A, H
        {0x3e, 0x75}, // LD A, 0x75
        {0x26, 0x75}, // LD H, 0x75
        {0xb4},       // OR A, H
        {0x3e, 0x00}, // LD A, 0x00
        {0x26, 0x00}, // LD H, 0x00
        {0xb4},       // OR A, H
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x7f);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf5);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x73);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf7);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf7);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x69);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0xff);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xff);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x75);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x75);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x75);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xB5 - OR A, L
TEST_F(CPUTest, OpcodeTest_0xB5_OR_A_L)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x2e, 0x25}, // LD L, 0x25
        {0xb5},       // OR A, L
        {0x3e, 0xf5}, // LD A, 0xf5
        {0x2e, 0x73}, // LD L, 0x73
        {0xb5},       // OR A, L
        {0xb5},       // OR A, L
        {0x3e, 0x8f}, // LD A, 0x8f
        {0x2e, 0x02}, // LD L, 0x02
        {0xb5},       // OR A, L
        {0x3e, 0x3c}, // LD A, 0x3c
        {0x2e, 0x00}, // LD L, 0x00
        {0xb5},       // OR A, L
        {0x3e, 0x69}, // LD A, 0x69
        {0x2e, 0xff}, // LD L, 0xff
        {0xb5},       // OR A, L
        {0x3e, 0x75}, // LD A, 0x75
        {0x2e, 0x75}, // LD L, 0x75
        {0xb5},       // OR A, L
        {0x3e, 0x00}, // LD A, 0x00
        {0x2e, 0x00}, // LD L, 0x00
        {0xb5},       // OR A, L
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x7f);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf5);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x73);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf7);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf7);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x69);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0xff);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xff);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x75);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x75);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x75);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xB6 - OR A, (HL)
TEST_F(CPUTest, OpcodeTest_0xB6_OR_A_DEREF_HL)
{
    Program program({
        {0x21, 0x07, 0xc3}, // LD HL, 0xc307
        {0x3e, 0x5a},       // LD A, 0x5a
        {0x36, 0x25},       // LD (HL), 0x25
        {0xb6},             // OR A, (HL)
        {0x3e, 0xf5},       // LD A, 0xf5
        {0x36, 0x73},       // LD (HL), 0x73
        {0xb6},             // OR A, (HL)
        {0xb6},             // OR A, (HL)
        {0x3e, 0x8f},       // LD A, 0x8f
        {0x36, 0x02},       // LD (HL), 0x02
        {0xb6},             // OR A, (HL)
        {0x3e, 0x3c},       // LD A, 0x3c
        {0x36, 0x00},       // LD (HL), 0x00
        {0xb6},             // OR A, (HL)
        {0x3e, 0x69},       // LD A, 0x69
        {0x36, 0xff},       // LD (HL), 0xff
        {0xb6},             // OR A, (HL)
        {0x3e, 0x75},       // LD A, 0x75
        {0x36, 0x75},       // LD (HL), 0x75
        {0xb6},             // OR A, (HL)
        {0x3e, 0x00},       // LD A, 0x00
        {0x36, 0x00},       // LD (HL), 0x00
        {0xb6},             // OR A, (HL)
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setHL(0xc307);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x7f);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf5);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x73);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf7);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf7);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x02);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x69);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0xff);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xff);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x75);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x75);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x75);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xB7 - OR A, A
TEST_F(CPUTest, OpcodeTest_0xB7_OR_A_A)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0xb7},       // OR A, A
        {0x3e, 0xf5}, // LD A, 0xf5
        {0xb7},       // OR A, A
        {0xb7},       // OR A, A
        {0x3e, 0x8f}, // LD A, 0x8f
        {0xb7},       // OR A, A
        {0x3e, 0x3c}, // LD A, 0x3c
        {0xb7},       // OR A, A
        {0x3e, 0x69}, // LD A, 0x69
        {0xb7},       // OR A, A
        {0x3e, 0x75}, // LD A, 0x75
        {0xb7},       // OR A, A
        {0x3e, 0x00}, // LD A, 0x00
        {0xb7},       // OR A, A
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf5);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf5);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf5);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x69);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x69);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x75);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x75);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xB8 - CP A, B
TEST_F(CPUTest, OpcodeTest_0xB8_CP_A_B)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x06, 0x25}, // LD B, 0x25
        {0xb8},       // CP A, B
        {0x3e, 0x80}, // LD A, 0x80
        {0x06, 0x40}, // LD B, 0x40
        {0xb8},       // CP A, B
        {0xb8},       // CP A, B
        {0xb8},       // CP A, B
        {0x3e, 0x67}, // LD A, 0x67
        {0x06, 0x67}, // LD B, 0x67
        {0xb8},       // CP A, B
        {0x3e, 0x56}, // LD A, 0x56
        {0x06, 0x3e}, // LD B, 0x3e
        {0xb8},       // CP A, B
        {0x3e, 0x56}, // LD A, 0x56
        {0x06, 0x58}, // LD B, 0x58
        {0xb8},       // CP A, B
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x40);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x67);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x67);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x67);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x3e);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPU.setFlags(0, 1, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x58);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPU.setFlags(0, 1, 1, 1);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xB9 - CP A, C
TEST_F(CPUTest, OpcodeTest_0xB9_CP_A_C)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x0e, 0x25}, // LD C, 0x25
        {0xb9},       // CP A, C
        {0x3e, 0x80}, // LD A, 0x80
        {0x0e, 0x40}, // LD C, 0x40
        {0xb9},       // CP A, C
        {0xb9},       // CP A, C
        {0xb9},       // CP A, C
        {0x3e, 0x67}, // LD A, 0x67
        {0x0e, 0x67}, // LD C, 0x67
        {0xb9},       // CP A, C
        {0x3e, 0x56}, // LD A, 0x56
        {0x0e, 0x3e}, // LD C, 0x3e
        {0xb9},       // CP A, C
        {0x3e, 0x56}, // LD A, 0x56
        {0x0e, 0x58}, // LD C, 0x58
        {0xb9},       // CP A, C
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x40);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x67);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x67);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x67);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x3e);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPU.setFlags(0, 1, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x58);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPU.setFlags(0, 1, 1, 1);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xBA - CP A, D
TEST_F(CPUTest, OpcodeTest_0xBA_CP_A_D)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x16, 0x25}, // LD D, 0x25
        {0xba},       // CP A, D
        {0x3e, 0x80}, // LD A, 0x80
        {0x16, 0x40}, // LD D, 0x40
        {0xba},       // CP A, D
        {0xba},       // CP A, D
        {0xba},       // CP A, D
        {0x3e, 0x67}, // LD A, 0x67
        {0x16, 0x67}, // LD D, 0x67
        {0xba},       // CP A, D
        {0x3e, 0x56}, // LD A, 0x56
        {0x16, 0x3e}, // LD D, 0x3e
        {0xba},       // CP A, D
        {0x3e, 0x56}, // LD A, 0x56
        {0x16, 0x58}, // LD D, 0x58
        {0xba},       // CP A, D
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x40);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x67);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x67);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x67);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x3e);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPU.setFlags(0, 1, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setD(0x58);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPU.setFlags(0, 1, 1, 1);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xBB - CP A, E
TEST_F(CPUTest, OpcodeTest_0xBB_CP_A_E)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x1e, 0x25}, // LD E, 0x25
        {0xbb},       // CP A, E
        {0x3e, 0x80}, // LD A, 0x80
        {0x1e, 0x40}, // LD E, 0x40
        {0xbb},       // CP A, E
        {0xbb},       // CP A, E
        {0xbb},       // CP A, E
        {0x3e, 0x67}, // LD A, 0x67
        {0x1e, 0x67}, // LD E, 0x67
        {0xbb},       // CP A, E
        {0x3e, 0x56}, // LD A, 0x56
        {0x1e, 0x3e}, // LD E, 0x3e
        {0xbb},       // CP A, E
        {0x3e, 0x56}, // LD A, 0x56
        {0x1e, 0x58}, // LD E, 0x58
        {0xbb},       // CP A, E
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x40);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x67);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x67);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x67);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x3e);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPU.setFlags(0, 1, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setE(0x58);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPU.setFlags(0, 1, 1, 1);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xBC - CP A, H
TEST_F(CPUTest, OpcodeTest_0xBC_CP_A_H)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x26, 0x25}, // LD H, 0x25
        {0xbc},       // CP A, H
        {0x3e, 0x80}, // LD A, 0x80
        {0x26, 0x40}, // LD H, 0x40
        {0xbc},       // CP A, H
        {0xbc},       // CP A, H
        {0xbc},       // CP A, H
        {0x3e, 0x67}, // LD A, 0x67
        {0x26, 0x67}, // LD H, 0x67
        {0xbc},       // CP A, H
        {0x3e, 0x56}, // LD A, 0x56
        {0x26, 0x3e}, // LD H, 0x3e
        {0xbc},       // CP A, H
        {0x3e, 0x56}, // LD A, 0x56
        {0x26, 0x58}, // LD H, 0x58
        {0xbc},       // CP A, H
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x40);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x67);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x67);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x67);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x3e);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPU.setFlags(0, 1, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setH(0x58);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPU.setFlags(0, 1, 1, 1);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xBD - CP A, L
TEST_F(CPUTest, OpcodeTest_0xBD_CP_A_L)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0x2e, 0x25}, // LD L, 0x25
        {0xbd},       // CP A, L
        {0x3e, 0x80}, // LD A, 0x80
        {0x2e, 0x40}, // LD L, 0x40
        {0xbd},       // CP A, L
        {0xbd},       // CP A, L
        {0xbd},       // CP A, L
        {0x3e, 0x67}, // LD A, 0x67
        {0x2e, 0x67}, // LD L, 0x67
        {0xbd},       // CP A, L
        {0x3e, 0x56}, // LD A, 0x56
        {0x2e, 0x3e}, // LD L, 0x3e
        {0xbd},       // CP A, L
        {0x3e, 0x56}, // LD A, 0x56
        {0x2e, 0x58}, // LD L, 0x58
        {0xbd},       // CP A, L
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x40);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x67);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x67);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x67);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x3e);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPU.setFlags(0, 1, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setL(0x58);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPU.setFlags(0, 1, 1, 1);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xBE - CP A, (HL)
TEST_F(CPUTest, OpcodeTest_0xBE_CP_A_DEREF_HL)
{
    Program program({
        {0x21, 0x07, 0xc3}, // LD HL, 0xc307
        {0x3e, 0x5a},       // LD A, 0x5a
        {0x36, 0x25},       // LD (HL), 0x25
        {0xbe},             // CP A, (HL)
        {0x3e, 0x80},       // LD A, 0x80
        {0x36, 0x40},       // LD (HL), 0x40
        {0xbe},             // CP A, (HL)
        {0xbe},             // CP A, (HL)
        {0xbe},             // CP A, (HL)
        {0x3e, 0x67},       // LD A, 0x67
        {0x36, 0x67},       // LD (HL), 0x67
        {0xbe},             // CP A, (HL)
        {0x3e, 0x56},       // LD A, 0x56
        {0x36, 0x3e},       // LD (HL), 0x3e
        {0xbe},             // CP A, (HL)
        {0x3e, 0x56},       // LD A, 0x56
        {0x36, 0x58},       // LD (HL), 0x58
        {0xbe},             // CP A, (HL)
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setHL(0xc307);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x40);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x67);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x67);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x67);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x3e);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPU.setFlags(0, 1, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x58);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPU.setFlags(0, 1, 1, 1);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xBF - CP A, A
TEST_F(CPUTest, OpcodeTest_0xBF_CP_A_A)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0xbf},       // CP A, A
        {0x3e, 0x80}, // LD A, 0x80
        {0xbf},       // CP A, A
        {0xbf},       // CP A, A
        {0xbf},       // CP A, A
        {0x3e, 0x67}, // LD A, 0x67
        {0xbf},       // CP A, A
        {0x3e, 0x56}, // LD A, 0x56
        {0xbf},       // CP A, A
        {0x3e, 0x56}, // LD A, 0x56
        {0xbf},       // CP A, A
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x67);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x67);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xC0 - RET NZ
TEST_F(CPUTest, OpcodeTest_0xC0_RET_NZ_JUMP)
{
    Program program({
        {0x31, 0xfe, 0xff}, // LD SP, 0xfffe
        {0x01, 0x25, 0x01}, // LD BC, 0x0125
        {0xc5},             // PUSH BC
        {0x06, 0x00},       // LD B, 0x00
        {0x04},             // INC B
        {0xc0},             // RET NZ
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setSP(0xfffe);
    expectedCPU.setPC(0x0103);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setBC(0x0125);
    expectedCPU.setPC(0x0106);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffc);
    expectedCPU.ram()->set(0xfffd, 0x01);
    expectedCPU.ram()->set(0xfffc, 0x25);
    expectedCPU.setPC(0x0107);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x00);
    expectedCPU.setPC(0x0109);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x01);
    expectedCPU.setPC(0x010a);
    expectedCPU.setFlags(0, 0, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffe);
    expectedCPU.setPC(0x0125);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

TEST_F(CPUTest, OpcodeTest_0xC0_RET_NZ_NO_JUMP)
{
    Program program({
        {0x31, 0xfe, 0xff}, // LD SP, 0xfffe
        {0x01, 0x25, 0x01}, // LD BC, 0x0125
        {0xc5},             // PUSH BC
        {0xaf},             // XOR A
        {0xc0},             // RET NZ
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setSP(0xfffe);
    expectedCPU.setPC(0x0103);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setBC(0x0125);
    expectedCPU.setPC(0x0106);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffc);
    expectedCPU.ram()->set(0xfffd, 0x01);
    expectedCPU.ram()->set(0xfffc, 0x25);
    expectedCPU.setPC(0x0107);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setPC(0x0108);
    expectedCPU.setFlags(1, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffe);
    expectedCPU.setPC(0x0109);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

// 0xC1 - POP BC
TEST_F(CPUTest, OpcodeTest_0xC1_POP_BC)
{
    Program program({
        {0x31, 0xfe, 0xff}, // LD SP, 0xfffe
        {0x01, 0x7a, 0x32}, // LD BC, 0x327a
        {0xc5},             // PUSH BC
        {0x01, 0x34, 0x12}, // LD BC, 0x1234
        {0xc5},             // PUSH BC
        {0xc1},             // POP BC
        {0x01, 0x76, 0x98}, // LD BC, 0x9876
        {0xc5},             // PUSH BC
        {0x01, 0x00, 0x00}, // LD BC, 0x0000
        {0xc1},             // POP BC
        {0xc1},             // POP BC
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setSP(0xfffe);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setBC(0x327a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffc);
    expectedCPU.ram()->set(0xfffd, 0x32);
    expectedCPU.ram()->set(0xfffc, 0x7a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setBC(0x1234);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffa);
    expectedCPU.ram()->set(0xfffb, 0x12);
    expectedCPU.ram()->set(0xfffa, 0x34);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setBC(0x1234);
    expectedCPU.setSP(0xfffc);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setBC(0x9876);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffa);
    expectedCPU.ram()->set(0xfffb, 0x98);
    expectedCPU.ram()->set(0xfffa, 0x76);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setBC(0x0000);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setBC(0x9876);
    expectedCPU.setSP(0xfffc);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setBC(0x327a);
    expectedCPU.setSP(0xfffe);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xC2 - JP NZ, a16
TEST_F(CPUTest, OpcodeTest_0xC2_JP_NZ_a16_JUMP)
{
    Program program({
        {0x06, 0x00},       // LD B, 0x00
        {0x04},             // INC B
        {0xc2, 0x25, 0x01}, // JP NZ, 0x0125
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setB(0x00);
    expectedCPU.setPC(0x0102);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x01);
    expectedCPU.setFlags(0, 0, 0, expectedCPU.FlagC());
    expectedCPU.setPC(0x0103);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0125);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

TEST_F(CPUTest, OpcodeTest_0xC2_JP_NZ_a16_NO_JUMP)
{
    Program program({
        {0xaf},             // XOR A
        {0xc2, 0x25, 0x01}, // JP NZ, 0x0125
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 0);
    expectedCPU.setPC(0x0101);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0104);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

// 0xC3 - JP a16
TEST_F(CPUTest, OpcodeTest_0xC3_JP_a16_JUMP)
{
    Program program({
        {0xc3, 0x25, 0x01}, // JP 0x0125
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setPC(0x0125);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

// 0xC4 - CALL NZ, a16
TEST_F(CPUTest, OpcodeTest_0xC4_CALL_NZ_a16_JUMP)
{
    Program program({
        {0x06, 0x00},       // LD B, 0x00
        {0x04},             // INC B
        {0xc4, 0x25, 0x01}, // CALL NZ, 0x0125
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setB(0x00);
    expectedCPU.setPC(0x0102);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x01);
    expectedCPU.setFlags(0, 0, 0, expectedCPU.FlagC());
    expectedCPU.setPC(0x0103);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffc);
    expectedCPU.ram()->set(0xfffd, 0x01);
    expectedCPU.ram()->set(0xfffc, 0x06);
    expectedCPU.setPC(0x0125);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

TEST_F(CPUTest, OpcodeTest_0xC4_CALL_NZ_a16_NO_JUMP)
{
    Program program({
        {0xaf},             // XOR A
        {0xc4, 0x25, 0x01}, // CALL NZ, 0x0125
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 0);
    expectedCPU.setPC(0x0101);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0104);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

// 0xC5 - PUSH BC
TEST_F(CPUTest, OpcodeTest_0xC5_PUSH_BC)
{
    Program program({
        {0x31, 0xfe, 0xff}, // LD SP, 0xfffe
        {0x01, 0x7a, 0x32}, // LD BC, 0x327a
        {0xc5},             // PUSH BC
        {0x01, 0x34, 0x12}, // LD BC, 0x1234
        {0xc5},             // PUSH BC
        {0xc1},             // POP BC
        {0x01, 0x76, 0x98}, // LD BC, 0x9876
        {0xc5},             // PUSH BC
        {0x01, 0x00, 0x00}, // LD BC, 0x0000
        {0xc1},             // POP BC
        {0xc1},             // POP BC
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setSP(0xfffe);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setBC(0x327a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffc);
    expectedCPU.ram()->set(0xfffd, 0x32);
    expectedCPU.ram()->set(0xfffc, 0x7a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setBC(0x1234);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffa);
    expectedCPU.ram()->set(0xfffb, 0x12);
    expectedCPU.ram()->set(0xfffa, 0x34);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setBC(0x1234);
    expectedCPU.setSP(0xfffc);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setBC(0x9876);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffa);
    expectedCPU.ram()->set(0xfffb, 0x98);
    expectedCPU.ram()->set(0xfffa, 0x76);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setBC(0x0000);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setBC(0x9876);
    expectedCPU.setSP(0xfffc);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setBC(0x327a);
    expectedCPU.setSP(0xfffe);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xC6 - ADD A, d8
TEST_F(CPUTest, OpcodeTest_0xC6_ADD_A_d8)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0xc6, 0x25}, // ADD A, 0x25
        {0x3e, 0x80}, // LD A, 0x80
        {0xc6, 0x40}, // ADD A, 0x40
        {0xc6, 0x40}, // ADD A, 0x40
        {0x3e, 0x8f}, // LD A, 0x8f
        {0xc6, 0x02}, // ADD A, 0x02
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x7f);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xc0);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x91);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xC7 - RST 0
TEST_F(CPUTest, OpcodeTest_0xC7_RST_0)
{
    Program program({
        {0x31, 0xfe, 0xff}, // LD SP, 0xfffe
        {0x00},             // NOP
        {0x00},             // NOP
        {0x00},             // NOP
        {0x00},             // NOP
        {0xc7},             // RST 0
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setSP(0xfffe);
    expectedCPU.setPC(0x0103);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0104);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0105);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0106);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0107);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffc);
    expectedCPU.ram()->set(0xfffd, 0x01);
    expectedCPU.ram()->set(0xfffc, 0x08);
    expectedCPU.setPC(0x0000);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

// 0xC8 - RET Z
TEST_F(CPUTest, OpcodeTest_0xC8_RET_Z_JUMP)
{
    Program program({
        {0x31, 0xfe, 0xff}, // LD SP, 0xfffe
        {0x01, 0x25, 0x01}, // LD BC, 0x0125
        {0xc5},             // PUSH BC
        {0xaf},             // XOR A
        {0xc8},             // RET Z
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setSP(0xfffe);
    expectedCPU.setPC(0x0103);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setBC(0x0125);
    expectedCPU.setPC(0x0106);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffc);
    expectedCPU.ram()->set(0xfffd, 0x01);
    expectedCPU.ram()->set(0xfffc, 0x25);
    expectedCPU.setPC(0x0107);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setPC(0x0108);
    expectedCPU.setFlags(1, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffe);
    expectedCPU.setPC(0x0125);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

TEST_F(CPUTest, OpcodeTest_0xC8_RET_Z_NO_JUMP)
{
    Program program({
        {0x31, 0xfe, 0xff}, // LD SP, 0xfffe
        {0x01, 0x25, 0x01}, // LD BC, 0x0125
        {0xc5},             // PUSH BC
        {0x06, 0x00},       // LD B, 0x00
        {0x04},             // INC B
        {0xc8},             // RET Z
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setSP(0xfffe);
    expectedCPU.setPC(0x0103);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setBC(0x0125);
    expectedCPU.setPC(0x0106);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffc);
    expectedCPU.ram()->set(0xfffd, 0x01);
    expectedCPU.ram()->set(0xfffc, 0x25);
    expectedCPU.setPC(0x0107);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x00);
    expectedCPU.setPC(0x0109);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x01);
    expectedCPU.setPC(0x010a);
    expectedCPU.setFlags(0, 0, 0, expectedCPU.FlagC());
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffe);
    expectedCPU.setPC(0x010b);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

// 0xC9 - RET
TEST_F(CPUTest, OpcodeTest_0xC9_RET_JUMP)
{
    Program program({
        {0x31, 0xfe, 0xff}, // LD SP, 0xfffe
        {0x01, 0x25, 0x01}, // LD BC, 0x0125
        {0xc5},             // PUSH BC
        {0xaf},             // XOR A
        {0xc9},             // RET
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setSP(0xfffe);
    expectedCPU.setPC(0x0103);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setBC(0x0125);
    expectedCPU.setPC(0x0106);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffc);
    expectedCPU.ram()->set(0xfffd, 0x01);
    expectedCPU.ram()->set(0xfffc, 0x25);
    expectedCPU.setPC(0x0107);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setPC(0x0108);
    expectedCPU.setFlags(1, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffe);
    expectedCPU.setPC(0x0125);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

// 0xCA - JP Z, a16
TEST_F(CPUTest, OpcodeTest_0xCA_JP_Z_a16_JUMP)
{
    Program program({
        {0xaf},             // XOR A
        {0xca, 0x25, 0x01}, // JP Z, 0x0125
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 0);
    expectedCPU.setPC(0x0101);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0125);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

TEST_F(CPUTest, OpcodeTest_0xCA_JP_Z_a16_NO_JUMP)
{
    Program program({
        {0x06, 0x00},       // LD B, 0x00
        {0x04},             // INC B
        {0xca, 0x25, 0x01}, // JP Z, 0x0125
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setB(0x00);
    expectedCPU.setPC(0x0102);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x01);
    expectedCPU.setFlags(0, 0, 0, expectedCPU.FlagC());
    expectedCPU.setPC(0x0103);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0106);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

// 0xCC - CALL Z, a16
TEST_F(CPUTest, OpcodeTest_0xCC_CALL_Z_a16_JUMP)
{
    Program program({
        {0xaf},             // XOR A
        {0xcc, 0x25, 0x01}, // CALL Z, 0x0125
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 0);
    expectedCPU.setPC(0x0101);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffc);
    expectedCPU.ram()->set(0xfffd, 0x01);
    expectedCPU.ram()->set(0xfffc, 0x04);
    expectedCPU.setPC(0x0125);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

TEST_F(CPUTest, OpcodeTest_0xCC_CALL_Z_a16_NO_JUMP)
{
    Program program({
        {0x06, 0x00},       // LD B, 0x00
        {0x04},             // INC B
        {0xcc, 0x25, 0x01}, // CALL Z, 0x0125
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setB(0x00);
    expectedCPU.setPC(0x0102);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setB(0x01);
    expectedCPU.setFlags(0, 0, 0, expectedCPU.FlagC());
    expectedCPU.setPC(0x0103);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0106);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

// 0xCD - CALL a16
TEST_F(CPUTest, OpcodeTest_0xCD_CALL_a16_JUMP)
{
    Program program({
        {0xcd, 0x25, 0x01}, // CALL 0x0125
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setSP(0xfffc);
    expectedCPU.ram()->set(0xfffd, 0x01);
    expectedCPU.ram()->set(0xfffc, 0x03);
    expectedCPU.setPC(0x0125);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

// 0xCE - ADC A, d8
TEST_F(CPUTest, OpcodeTest_0xCE_ADC_A_d8)
{
    Program program({
        {0x37},       // SCF
        {0x3f},       // CCF
        {0x3e, 0x5a}, // LD A, 0x5a
        {0xce, 0x25}, // ADC A, 0x25
        {0x3e, 0x80}, // LD A, 0x80
        {0xce, 0x40}, // ADC A, 0x40
        {0xce, 0x40}, // ADC A, 0x40
        {0x3e, 0x8f}, // LD A, 0x8f
        {0xce, 0x02}, // ADC A, 0x02
        {0x3e, 0xfd}, // LD A, 0xfd
        {0x37},       // SCF
        {0xce, 0x02}, // ADC A, 0x02
        {0x3e, 0x00}, // LD A, 0x00
        {0x37},       // SCF
        {0xce, 0x0f}  // ADC A, 0x0f
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x7f);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xc0);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x92);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xfd);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 1, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x10);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xCF - RST 1
TEST_F(CPUTest, OpcodeTest_0xCF_RST_1)
{
    Program program({
        {0x31, 0xfe, 0xff}, // LD SP, 0xfffe
        {0x00},             // NOP
        {0x00},             // NOP
        {0x00},             // NOP
        {0x00},             // NOP
        {0xcf},             // RST 1
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setSP(0xfffe);
    expectedCPU.setPC(0x0103);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0104);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0105);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0106);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0107);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffc);
    expectedCPU.ram()->set(0xfffd, 0x01);
    expectedCPU.ram()->set(0xfffc, 0x08);
    expectedCPU.setPC(0x0008);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

// 0xD0 - RET NC
TEST_F(CPUTest, OpcodeTest_0xD0_RET_NC_JUMP)
{
    Program program({
        {0x31, 0xfe, 0xff}, // LD SP, 0xfffe
        {0x01, 0x25, 0x01}, // LD BC, 0x0125
        {0xc5},             // PUSH BC
        {0x37},             // SCF
        {0x3f},             // CCF
        {0xd0},             // RET NC
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setSP(0xfffe);
    expectedCPU.setPC(0x0103);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setBC(0x0125);
    expectedCPU.setPC(0x0106);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffc);
    expectedCPU.ram()->set(0xfffd, 0x01);
    expectedCPU.ram()->set(0xfffc, 0x25);
    expectedCPU.setPC(0x0107);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPU.setPC(0x0108);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 0);
    expectedCPU.setPC(0x0109);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffe);
    expectedCPU.setPC(0x0125);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

TEST_F(CPUTest, OpcodeTest_0xD0_RET_NC_NO_JUMP)
{
    Program program({
        {0x31, 0xfe, 0xff}, // LD SP, 0xfffe
        {0x01, 0x25, 0x01}, // LD BC, 0x0125
        {0xc5},             // PUSH BC
        {0x37},             // SCF
        {0xd0},             // RET NC
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setSP(0xfffe);
    expectedCPU.setPC(0x0103);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setBC(0x0125);
    expectedCPU.setPC(0x0106);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffc);
    expectedCPU.ram()->set(0xfffd, 0x01);
    expectedCPU.ram()->set(0xfffc, 0x25);
    expectedCPU.setPC(0x0107);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPU.setPC(0x0108);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffe);
    expectedCPU.setPC(0x0109);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

// 0xD1 - POP DE
TEST_F(CPUTest, OpcodeTest_0xD1_POP_DE)
{
    Program program({
        {0x31, 0xfe, 0xff}, // LD SP, 0xfffe
        {0x11, 0x7a, 0x32}, // LD DE, 0x327a
        {0xd5},             // PUSH DE
        {0x11, 0x34, 0x12}, // LD DE, 0x1234
        {0xd5},             // PUSH DE
        {0xd1},             // POP DE
        {0x11, 0x76, 0x98}, // LD DE, 0x9876
        {0xd5},             // PUSH DE
        {0x11, 0x00, 0x00}, // LD DE, 0x0000
        {0xd1},             // POP DE
        {0xd1},             // POP DE
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setSP(0xfffe);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setDE(0x327a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffc);
    expectedCPU.ram()->set(0xfffd, 0x32);
    expectedCPU.ram()->set(0xfffc, 0x7a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setDE(0x1234);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffa);
    expectedCPU.ram()->set(0xfffb, 0x12);
    expectedCPU.ram()->set(0xfffa, 0x34);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setDE(0x1234);
    expectedCPU.setSP(0xfffc);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setDE(0x9876);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffa);
    expectedCPU.ram()->set(0xfffb, 0x98);
    expectedCPU.ram()->set(0xfffa, 0x76);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setDE(0x0000);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setDE(0x9876);
    expectedCPU.setSP(0xfffc);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setDE(0x327a);
    expectedCPU.setSP(0xfffe);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xD2 - JP NC, a16
TEST_F(CPUTest, OpcodeTest_0xD2_JP_NC_a16_JUMP)
{
    Program program({
        {0x37},             // SCF
        {0x3f},             // CCF
        {0xd2, 0x25, 0x01}, // JP NC, 0x0125
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPU.setPC(0x0101);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 0);
    expectedCPU.setPC(0x0102);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0125);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

TEST_F(CPUTest, OpcodeTest_0xD2_JP_NC_a16_NO_JUMP)
{
    Program program({
        {0x37},             // SCF
        {0xd2, 0x25, 0x01}, // JP NC, 0x0125
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPU.setPC(0x0101);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0104);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

// 0xD4 - CALL NC, a16
TEST_F(CPUTest, OpcodeTest_0xD4_CALL_NC_a16_JUMP)
{
    Program program({
        {0x37},             // SCF
        {0x3f},             // CCF
        {0xd4, 0x25, 0x01}, // CALL NC, 0x0125
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPU.setPC(0x0101);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 0);
    expectedCPU.setPC(0x0102);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffc);
    expectedCPU.ram()->set(0xfffd, 0x01);
    expectedCPU.ram()->set(0xfffc, 0x05);
    expectedCPU.setPC(0x0125);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

TEST_F(CPUTest, OpcodeTest_0xD4_CALL_NC_a16_NO_JUMP)
{
    Program program({
        {0x37},             // SCF
        {0xd4, 0x25, 0x01}, // CALL NC, 0x0125
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPU.setPC(0x0101);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0104);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

// 0xD5 - PUSH DE
TEST_F(CPUTest, OpcodeTest_0xD5_PUSH_DE)
{
    Program program({
        {0x31, 0xfe, 0xff}, // LD SP, 0xfffe
        {0x11, 0x7a, 0x32}, // LD DE, 0x327a
        {0xd5},             // PUSH DE
        {0x11, 0x34, 0x12}, // LD DE, 0x1234
        {0xd5},             // PUSH DE
        {0xd1},             // POP DE
        {0x11, 0x76, 0x98}, // LD DE, 0x9876
        {0xd5},             // PUSH DE
        {0x11, 0x00, 0x00}, // LD DE, 0x0000
        {0xd1},             // POP DE
        {0xd1},             // POP DE
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setSP(0xfffe);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setDE(0x327a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffc);
    expectedCPU.ram()->set(0xfffd, 0x32);
    expectedCPU.ram()->set(0xfffc, 0x7a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setDE(0x1234);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffa);
    expectedCPU.ram()->set(0xfffb, 0x12);
    expectedCPU.ram()->set(0xfffa, 0x34);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setDE(0x1234);
    expectedCPU.setSP(0xfffc);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setDE(0x9876);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffa);
    expectedCPU.ram()->set(0xfffb, 0x98);
    expectedCPU.ram()->set(0xfffa, 0x76);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setDE(0x0000);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setDE(0x9876);
    expectedCPU.setSP(0xfffc);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setDE(0x327a);
    expectedCPU.setSP(0xfffe);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xD6 - SUB A, d8
TEST_F(CPUTest, OpcodeTest_0xD6_SUB_A_d8)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0xd6, 0x25}, // SUB A, 0x25
        {0x3e, 0x80}, // LD A, 0x80
        {0xd6, 0x40}, // SUB A, 0x40
        {0xd6, 0x40}, // SUB A, 0x40
        {0xd6, 0x40}, // SUB A, 0x40
        {0x3e, 0x56}, // LD A, 0x56
        {0xd6, 0x3e}, // SUB A, 0x3e
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x35);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x40);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xc0);
    expectedCPU.setFlags(0, 1, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x18);
    expectedCPU.setFlags(0, 1, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xD7 - RST 2
TEST_F(CPUTest, OpcodeTest_0xD7_RST_2)
{
    Program program({
        {0x31, 0xfe, 0xff}, // LD SP, 0xfffe
        {0x00},             // NOP
        {0x00},             // NOP
        {0x00},             // NOP
        {0x00},             // NOP
        {0xd7},             // RST 2
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setSP(0xfffe);
    expectedCPU.setPC(0x0103);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0104);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0105);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0106);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0107);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffc);
    expectedCPU.ram()->set(0xfffd, 0x01);
    expectedCPU.ram()->set(0xfffc, 0x08);
    expectedCPU.setPC(0x0010);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

// 0xD8 - RET C
TEST_F(CPUTest, OpcodeTest_0xD8_RET_C_JUMP)
{
    Program program({
        {0x31, 0xfe, 0xff}, // LD SP, 0xfffe
        {0x01, 0x25, 0x01}, // LD BC, 0x0125
        {0xc5},             // PUSH BC
        {0x37},             // SCF
        {0xd8},             // RET C
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setSP(0xfffe);
    expectedCPU.setPC(0x0103);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setBC(0x0125);
    expectedCPU.setPC(0x0106);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffc);
    expectedCPU.ram()->set(0xfffd, 0x01);
    expectedCPU.ram()->set(0xfffc, 0x25);
    expectedCPU.setPC(0x0107);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPU.setPC(0x0108);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffe);
    expectedCPU.setPC(0x0125);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

TEST_F(CPUTest, OpcodeTest_0xD8_RET_C_NO_JUMP)
{
    Program program({
        {0x31, 0xfe, 0xff}, // LD SP, 0xfffe
        {0x01, 0x25, 0x01}, // LD BC, 0x0125
        {0xc5},             // PUSH BC
        {0x37},             // SCF
        {0x3f},             // CCF
        {0xd8},             // RET C
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setSP(0xfffe);
    expectedCPU.setPC(0x0103);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setBC(0x0125);
    expectedCPU.setPC(0x0106);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffc);
    expectedCPU.ram()->set(0xfffd, 0x01);
    expectedCPU.ram()->set(0xfffc, 0x25);
    expectedCPU.setPC(0x0107);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPU.setPC(0x0108);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 0);
    expectedCPU.setPC(0x0109);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffe);
    expectedCPU.setPC(0x010a);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

// 0xD9 - RETI
TEST_F(CPUTest, OpcodeTest_0xD9_RETI_JUMP)
{
    Program program({
        {0x31, 0xfe, 0xff}, // LD SP, 0xfffe
        {0x01, 0x25, 0x01}, // LD BC, 0x0125
        {0xc5},             // PUSH BC
        {0xd9},             // RETI
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setSP(0xfffe);
    expectedCPU.setPC(0x0103);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setBC(0x0125);
    expectedCPU.setPC(0x0106);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffc);
    expectedCPU.ram()->set(0xfffd, 0x01);
    expectedCPU.ram()->set(0xfffc, 0x25);
    expectedCPU.setPC(0x0107);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffe);
    expectedCPU.setIME(true);
    expectedCPU.setPC(0x0125);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

// 0xDA - JP C, a16
TEST_F(CPUTest, OpcodeTest_0xDA_JP_C_a16_JUMP)
{
    Program program({
        {0x37},             // SCF
        {0xda, 0x25, 0x01}, // JP C, 0x0125
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPU.setPC(0x0101);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0125);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

TEST_F(CPUTest, OpcodeTest_0xDA_JP_C_a16_NO_JUMP)
{
    Program program({
        {0x37},             // SCF
        {0x3f},             // CCF
        {0xda, 0x25, 0x01}, // JP C, 0x0125
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPU.setPC(0x0101);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 0);
    expectedCPU.setPC(0x0102);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0105);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

// 0xDC - CALL C, a16
TEST_F(CPUTest, OpcodeTest_0xDC_CALL_C_a16_JUMP)
{
    Program program({
        {0x37},             // SCF
        {0xdc, 0x25, 0x01}, // CALL C, 0x0125
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPU.setPC(0x0101);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffc);
    expectedCPU.ram()->set(0xfffd, 0x01);
    expectedCPU.ram()->set(0xfffc, 0x04);
    expectedCPU.setPC(0x0125);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

TEST_F(CPUTest, OpcodeTest_0xDC_CALL_C_a16_NO_JUMP)
{
    Program program({
        {0x37},             // SCF
        {0x3f},             // CCF
        {0xdc, 0x25, 0x01}, // CALL C, 0x0125
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPU.setPC(0x0101);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 0);
    expectedCPU.setPC(0x0102);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0105);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

// 0xDE - SBC A, d8
TEST_F(CPUTest, OpcodeTest_0xDE_SBC_A_d8)
{
    Program program({
        {0x37},       // SCF
        {0x3f},       // CCF
        {0x3e, 0x5a}, // LD A, 0x5a
        {0xde, 0x25}, // SBC A, 0x25
        {0x3e, 0x80}, // LD A, 0x80
        {0xde, 0x40}, // SBC A, 0x40
        {0xde, 0x40}, // SBC A, 0x40
        {0xde, 0x40}, // SBC A, 0x40
        {0x3e, 0x56}, // LD A, 0x56
        {0xde, 0x3e}, // SBC A, 0x3e
        {0x3e, 0x56}, // LD A, 0x56
        {0x37},       // SCF
        {0xde, 0x55}, // SBC A, 0x55
        {0x3e, 0x05}, // LD A, 0x05
        {0x37},       // SCF
        {0xde, 0x05}, // SBC A, 0x05
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x35);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x40);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xc0);
    expectedCPU.setFlags(0, 1, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x17);
    expectedCPU.setFlags(0, 1, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x05);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xff);
    expectedCPU.setFlags(0, 1, 1, 1);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xDF - RST 3
TEST_F(CPUTest, OpcodeTest_0xDF_RST_3)
{
    Program program({
        {0x31, 0xfe, 0xff}, // LD SP, 0xfffe
        {0x00},             // NOP
        {0x00},             // NOP
        {0x00},             // NOP
        {0x00},             // NOP
        {0xdf},             // RST 3
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setSP(0xfffe);
    expectedCPU.setPC(0x0103);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0104);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0105);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0106);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0107);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffc);
    expectedCPU.ram()->set(0xfffd, 0x01);
    expectedCPU.ram()->set(0xfffc, 0x08);
    expectedCPU.setPC(0x0018);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

// 0xE0 - LD (a8), A
TEST_F(CPUTest, OpcodeTest_0xE0_LD_DEREF_ffa8_A)
{
    Program program({
        {0x3e, 0x42}, // LD A, 0x42
        {0xe0, 0x85}, // LD (0xff00 + 0x85), A
        {0xe0, 0x05}, // LD (0xff00 + 0x05), A
        {0xe0, 0xff}, // LD (0xff00 + 0xff), A
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xff85, 0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xff05, 0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xffff, 0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xE1 - POP HL
TEST_F(CPUTest, OpcodeTest_0xE1_POP_HL)
{
    Program program({
        {0x31, 0xfe, 0xff}, // LD SP, 0xfffe
        {0x21, 0x7a, 0x32}, // LD HL, 0x327a
        {0xe5},             // PUSH HL
        {0x21, 0x34, 0x12}, // LD HL, 0x1234
        {0xe5},             // PUSH HL
        {0xe1},             // POP HL
        {0x21, 0x76, 0x98}, // LD HL, 0x9876
        {0xe5},             // PUSH HL
        {0x21, 0x00, 0x00}, // LD HL, 0x0000
        {0xe1},             // POP HL
        {0xe1},             // POP HL
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setSP(0xfffe);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0x327a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffc);
    expectedCPU.ram()->set(0xfffd, 0x32);
    expectedCPU.ram()->set(0xfffc, 0x7a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0x1234);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffa);
    expectedCPU.ram()->set(0xfffb, 0x12);
    expectedCPU.ram()->set(0xfffa, 0x34);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0x1234);
    expectedCPU.setSP(0xfffc);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0x9876);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffa);
    expectedCPU.ram()->set(0xfffb, 0x98);
    expectedCPU.ram()->set(0xfffa, 0x76);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0x0000);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0x9876);
    expectedCPU.setSP(0xfffc);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0x327a);
    expectedCPU.setSP(0xfffe);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xE2 - LD (C), A
TEST_F(CPUTest, OpcodeTest_0xE2_LD_DEREF_ffC_A)
{
    Program program({
        {0x3e, 0x42}, // LD A, 0x42
        {0x0e, 0x85}, // LD C, 0x85
        {0xe2},       // LD (0xff00 + C), A
        {0x0e, 0x05}, // LD C, 0x05
        {0xe2},       // LD (0xff00 + C), A
        {0x0e, 0xff}, // LD C, 0xff
        {0xe2},       // LD (0xff00 + C), A
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x85);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xff85, 0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x05);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xff05, 0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0xff);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xffff, 0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xE5 - PUSH HL
TEST_F(CPUTest, OpcodeTest_0xE5_PUSH_HL)
{
    Program program({
        {0x31, 0xfe, 0xff}, // LD SP, 0xfffe
        {0x21, 0x7a, 0x32}, // LD HL, 0x327a
        {0xe5},             // PUSH HL
        {0x21, 0x34, 0x12}, // LD HL, 0x1234
        {0xe5},             // PUSH HL
        {0xe1},             // POP HL
        {0x21, 0x76, 0x98}, // LD HL, 0x9876
        {0xe5},             // PUSH HL
        {0x21, 0x00, 0x00}, // LD HL, 0x0000
        {0xe1},             // POP HL
        {0xe1},             // POP HL
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setSP(0xfffe);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0x327a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffc);
    expectedCPU.ram()->set(0xfffd, 0x32);
    expectedCPU.ram()->set(0xfffc, 0x7a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0x1234);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffa);
    expectedCPU.ram()->set(0xfffb, 0x12);
    expectedCPU.ram()->set(0xfffa, 0x34);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0x1234);
    expectedCPU.setSP(0xfffc);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0x9876);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffa);
    expectedCPU.ram()->set(0xfffb, 0x98);
    expectedCPU.ram()->set(0xfffa, 0x76);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0x0000);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0x9876);
    expectedCPU.setSP(0xfffc);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setHL(0x327a);
    expectedCPU.setSP(0xfffe);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xE6 - AND A, d8
TEST_F(CPUTest, OpcodeTest_0xE6_AND_A_d8)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0xe6, 0x25}, // AND A, 0x25
        {0x3e, 0xf5}, // LD A, 0xf5
        {0xe6, 0x73}, // AND A, 0x73
        {0xe6, 0x73}, // AND A, 0x73
        {0x3e, 0x8f}, // LD A, 0x8f
        {0xe6, 0x02}, // AND A, 0x02
        {0x3e, 0x3c}, // LD A, 0x3c
        {0xe6, 0x00}, // AND A, 0x00
        {0x3e, 0x69}, // LD A, 0x69
        {0xe6, 0xff}, // AND A, 0xff
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf5);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x71);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x71);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x02);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x69);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x69);
    expectedCPU.setFlags(0, 0, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xE7 - RST 4
TEST_F(CPUTest, OpcodeTest_0xE7_RST_4)
{
    Program program({
        {0x31, 0xfe, 0xff}, // LD SP, 0xfffe
        {0x00},             // NOP
        {0x00},             // NOP
        {0x00},             // NOP
        {0x00},             // NOP
        {0xe7},             // RST 4
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setSP(0xfffe);
    expectedCPU.setPC(0x0103);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0104);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0105);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0106);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0107);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffc);
    expectedCPU.ram()->set(0xfffd, 0x01);
    expectedCPU.ram()->set(0xfffc, 0x08);
    expectedCPU.setPC(0x0020);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

// 0xE8 - ADD SP, s8
TEST_F(CPUTest, OpcodeTest_0xE8_ADD_SP_s8)
{
    Program program({
        {0x37},             // SCF
        {0x3f},             // CCF
        {0x31, 0x05, 0x01}, // LD SP, 0x0105
        {0xe8, 0x10},       // ADD SP, 16
        {0xe8, 0xf0},       // ADD SP, -16
        {0x31, 0xf0, 0xff}, // LD SP, 0xfff0
        {0xe8, 0x20},       // ADD SP, 32
        {0xe8, 0xe0},       // ADD SP, -32
        {0xe8, 0x10},       // ADD SP, 16
        {0xe8, 0x00},       // ADD SP, 0
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0x0105);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0x0115);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0x0105);
    expectedCPU.setFlags(0, 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfff0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0x0010);
    expectedCPU.setFlags(0, 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfff0);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0x0000);
    expectedCPU.setFlags(0, 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0x0000);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xE9 - JP HL
TEST_F(CPUTest, OpcodeTest_0xE9_JP_HL_JUMP)
{
    Program program({
        {0x21, 0x25, 0x01}, // LD HL, 0x0125
        {0xe9},             // JP HL
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setHL(0x0125);
    expectedCPU.setPC(0x0103);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0125);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

// 0xEA - LD (a16), A
TEST_F(CPUTest, OpcodeTest_0xEA_LD_DEREF_a16_A)
{
    Program program({
        {0x3e, 0x42},       // LD A, 0x42
        {0xea, 0x07, 0xc3}, // LD (0xc307), A
        {0xea, 0x00, 0xc1}, // LD (0xc100), A
        {0xea, 0xff, 0xc5}, // LD (0xc5ff), A
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc100, 0x42);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc5ff, 0x42);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xEE - XOR A, d8
TEST_F(CPUTest, OpcodeTest_0xEE_XOR_A_d8)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0xee, 0x25}, // XOR A, 0x25
        {0x3e, 0xf5}, // LD A, 0xf5
        {0xee, 0x73}, // XOR A, 0x73
        {0xee, 0x73}, // XOR A, 0x73
        {0x3e, 0x8f}, // LD A, 0x8f
        {0xee, 0x02}, // XOR A, 0x02
        {0x3e, 0x3c}, // LD A, 0x3c
        {0xee, 0x00}, // XOR A, 0x00
        {0x3e, 0x69}, // LD A, 0x69
        {0xee, 0xff}, // XOR A, 0xff
        {0x3e, 0x75}, // LD A, 0x75
        {0xee, 0x75}, // XOR A, 0x75
        {0x3e, 0x00}, // LD A, 0x00
        {0xee, 0x6c}, // XOR A, 0x6c
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x7f);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf5);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x86);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf5);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8d);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x69);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x96);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x75);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x6c);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xEF - RST 5
TEST_F(CPUTest, OpcodeTest_0xEF_RST_5)
{
    Program program({
        {0x31, 0xfe, 0xff}, // LD SP, 0xfffe
        {0x00},             // NOP
        {0x00},             // NOP
        {0x00},             // NOP
        {0x00},             // NOP
        {0xef},             // RST 5
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setSP(0xfffe);
    expectedCPU.setPC(0x0103);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0104);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0105);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0106);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0107);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffc);
    expectedCPU.ram()->set(0xfffd, 0x01);
    expectedCPU.ram()->set(0xfffc, 0x08);
    expectedCPU.setPC(0x0028);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

// 0xF0 - LD A, (0xff00+a8)
TEST_F(CPUTest, OpcodeTest_0xF0_LD_A_DEREF_ffa8)
{
    Program program({
        {0x3e, 0x25}, // LD A, 0x25
        {0xe0, 0xc3}, // LD (0xff00 + 0xc3), A
        {0x3e, 0x36}, // LD A, 0x36
        {0xe0, 0x05}, // LD (0xff00 + 0x05), A
        {0x3e, 0x47}, // LD A, 0x47
        {0xe0, 0xff}, // LD (0xff00 + 0xff), A
        {0x3e, 0x00}, // LD A, 0x00
        {0xf0, 0xc3}, // LD A, (0xff00 + 0xc3)
        {0xf0, 0x05}, // LD A, (0xff00 + 0x05)
        {0xf0, 0xff}, // LD A, (0xff00 + 0xff)
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xffc3, 0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x36);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xff05, 0x36);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x47);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xffff, 0x47);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x36);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x47);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xF1 - POP AF
TEST_F(CPUTest, OpcodeTest_0xF1_POP_AF)
{
    Program program({
        {0xaf},             // XOR A, A
        {0x0f},             // RRCA
        {0x31, 0xfe, 0xff}, // LD SP, 0xfffe
        {0x3e, 0x7a},       // LD A, 0x7a
        {0x37},             // SCF
        {0xf5},             // PUSH AF
        {0x3e, 0x12},       // LD A, 0x12
        {0xf5},             // PUSH AF
        {0xf1},             // POP AF
        {0x3e, 0x98},       // LD A, 0x98
        {0xf5},             // PUSH AF
        {0x3e, 0x00},       // LD A, 0x00
        {0x3f},             // CCF
        {0xf1},             // POP AF
        {0xf1},             // POP AF
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0);
    expectedCPU.setFlags(1, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffe);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x7a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffc);
    expectedCPU.ram()->set(0xfffd, 0x7a);
    expectedCPU.ram()->set(0xfffc, 0x10);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x12);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffa);
    expectedCPU.ram()->set(0xfffb, 0x12);
    expectedCPU.ram()->set(0xfffa, 0x10);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setAF(0x1210);
    expectedCPU.setSP(0xfffc);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x98);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffa);
    expectedCPU.ram()->set(0xfffb, 0x98);
    expectedCPU.ram()->set(0xfffa, 0x10);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setAF(0x9810);
    expectedCPU.setSP(0xfffc);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setAF(0x7a10);
    expectedCPU.setSP(0xfffe);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xF2 - LD A, (0xff00+C)
TEST_F(CPUTest, OpcodeTest_0xF2_LD_A_DEREF_ffC)
{
    Program program({
        {0x3e, 0x25}, // LD A, 0x25
        {0x0e, 0xc3}, // LD C, 0xc3
        {0xe2},       // LD (0xff00 + C), A
        {0x3e, 0x36}, // LD A, 0x36
        {0x0e, 0x05}, // LD C, 0x05
        {0xe2},       // LD (0xff00 + C), A
        {0x3e, 0x47}, // LD A, 0x47
        {0x0e, 0xff}, // LD C, 0xff
        {0xe2},       // LD (0xff00 + C), A
        {0x3e, 0x00}, // LD A, 0x00
        {0x0e, 0xc3}, // LD C, 0xc3
        {0xf2},       // LD A, (0xff00 + C)
        {0x0e, 0x05}, // LD C, 0x05
        {0xf2},       // LD A, (0xff00 + C)
        {0x0e, 0xff}, // LD C, 0xff
        {0xf2},       // LD A, (0xff00 + C)
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0xc3);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xffc3, 0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x36);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x05);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xff05, 0x36);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x47);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0xff);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xffff, 0x47);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0xc3);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0x05);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x36);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setC(0xff);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x47);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xF3 - DI
TEST_F(CPUTest, OpcodeTest_0xF3_DI)
{
    Program program({
        {0xFB}, // EI
        {0x00}, // NOP
        {0xF3}, // DI
        {0xFB}, // EI
        {0x00}, // NOP
        {0xFB}, // EI
        {0x00}, // NOP
        {0xF3}, // DI
        {0xF3}, // DI
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setIME(true);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setIME(false);
    expectedCPUs.push_back(expectedCPU);

    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setIME(true);
    expectedCPUs.push_back(expectedCPU);

    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setIME(true);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setIME(false);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setIME(false);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xF5 - PUSH AF
TEST_F(CPUTest, OpcodeTest_0xF5_PUSH_AF)
{
    Program program({
        {0xaf},             // XOR A, A
        {0x0f},             // RRCA
        {0x31, 0xfe, 0xff}, // LD SP, 0xfffe
        {0x3e, 0x7a},       // LD A, 0x7a
        {0x37},             // SCF
        {0xf5},             // PUSH AF
        {0x3e, 0x12},       // LD A, 0x12
        {0xf5},             // PUSH AF
        {0xf1},             // POP AF
        {0x3e, 0x98},       // LD A, 0x98
        {0xf5},             // PUSH AF
        {0x3e, 0x00},       // LD A, 0x00
        {0x3f},             // CCF
        {0xf1},             // POP AF
        {0xf1},             // POP AF
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0);
    expectedCPU.setFlags(1, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffe);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x7a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 1);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffc);
    expectedCPU.ram()->set(0xfffd, 0x7a);
    expectedCPU.ram()->set(0xfffc, 0x10);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x12);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffa);
    expectedCPU.ram()->set(0xfffb, 0x12);
    expectedCPU.ram()->set(0xfffa, 0x10);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setAF(0x1210);
    expectedCPU.setSP(0xfffc);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x98);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffa);
    expectedCPU.ram()->set(0xfffb, 0x98);
    expectedCPU.ram()->set(0xfffa, 0x10);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setFlags(expectedCPU.FlagZ(), 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setAF(0x9810);
    expectedCPU.setSP(0xfffc);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setAF(0x7a10);
    expectedCPU.setSP(0xfffe);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xF6 - OR A, d8
TEST_F(CPUTest, OpcodeTest_0xF6_OR_A_d8)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0xf6, 0x25}, // OR A, 0x25
        {0x3e, 0xf5}, // LD A, 0xf5
        {0xf6, 0x73}, // OR A, 0x73
        {0xf6, 0x73}, // OR A, 0x73
        {0x3e, 0x8f}, // LD A, 0x8f
        {0xf6, 0x02}, // OR A, 0x02
        {0x3e, 0x3c}, // LD A, 0x3c
        {0xf6, 0x00}, // OR A, 0x00
        {0x3e, 0x69}, // LD A, 0x69
        {0xf6, 0xff}, // OR A, 0xff
        {0x3e, 0x75}, // LD A, 0x75
        {0xf6, 0x75}, // OR A, 0x75
        {0x3e, 0x00}, // LD A, 0x00
        {0xf6, 0x00}, // OR A, 0x00
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x7f);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf5);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf7);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xf7);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x8f);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x3c);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x69);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0xff);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x75);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x75);
    expectedCPU.setFlags(0, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPU.setFlags(1, 0, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xF7 - RST 6
TEST_F(CPUTest, OpcodeTest_0xF7_RST_6)
{
    Program program({
        {0x31, 0xfe, 0xff}, // LD SP, 0xfffe
        {0x00},             // NOP
        {0x00},             // NOP
        {0x00},             // NOP
        {0x00},             // NOP
        {0xf7},             // RST 6
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setSP(0xfffe);
    expectedCPU.setPC(0x0103);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0104);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0105);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0106);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0107);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffc);
    expectedCPU.ram()->set(0xfffd, 0x01);
    expectedCPU.ram()->set(0xfffc, 0x08);
    expectedCPU.setPC(0x30);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}

// 0xF8 - LD HL, SP+s8
TEST_F(CPUTest, OpcodeTest_0xF8_LD_HL_SP_s8)
{
    // TODO: implement LD HL, SP+s8 instruction and its tests.
    // throw std::runtime_error("LD HL, SP+s8 tests not implemented");
}

// 0xF9 - LD SP, HL
TEST_F(CPUTest, OpcodeTest_0xF9_LD_SP_HL)
{
    // TODO: implement LD SP, HL instruction and its tests.
    // throw std::runtime_error("LD SP, HL tests not implemented");
}

// 0xFA - LD A, (a16)
TEST_F(CPUTest, OpcodeTest_0xFA_LD_A_DEREF_a16)
{
    Program program({
        {0x3e, 0x25},       // LD A, 0x25
        {0xea, 0xc3, 0xff}, // LD (0xffc3), A
        {0x3e, 0x36},       // LD A, 0x36
        {0xea, 0x07, 0xc3}, // LD (0xc307), A
        {0x3e, 0x47},       // LD A, 0x47
        {0xea, 0xff, 0xcd}, // LD (0xcdff), A
        {0x3e, 0x00},       // LD A, 0x00
        {0xfa, 0xc3, 0xff}, // LD A, (0xffc3)
        {0xfa, 0x07, 0xc3}, // LD A, (0xc307)
        {0xfa, 0xff, 0xcd}, // LD A, (0xcdff)
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xffc3, 0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x36);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xc307, 0x36);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x47);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.ram()->set(0xcdff, 0x47);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x00);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x25);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x36);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x47);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xFB - EI
TEST_F(CPUTest, OpcodeTest_0xFB_EI)
{
    Program program({
        {0xFB}, // EI
        {0x00}, // NOP
        {0xF3}, // DI
        {0xFB}, // EI
        {0x00}, // NOP
        {0xFB}, // EI
        {0x00}, // NOP
        {0xF3}, // DI
        {0xF3}, // DI
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setIME(true);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setIME(false);
    expectedCPUs.push_back(expectedCPU);

    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setIME(true);
    expectedCPUs.push_back(expectedCPU);

    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setIME(true);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setIME(false);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setIME(false);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xFE - CP A, d8
TEST_F(CPUTest, OpcodeTest_0xFE_CP_A_d8)
{
    Program program({
        {0x3e, 0x5a}, // LD A, 0x5a
        {0xfe, 0x25}, // CP A, 0x25
        {0x3e, 0x80}, // LD A, 0x80
        {0xfe, 0x40}, // CP A, 0x40
        {0xfe, 0x40}, // CP A, 0x40
        {0xfe, 0x40}, // CP A, 0x40
        {0x3e, 0x67}, // LD A, 0x67
        {0xfe, 0x67}, // CP A, 0x67
        {0x3e, 0x56}, // LD A, 0x56
        {0xfe, 0x3e}, // CP A, 0x3e
        {0x3e, 0x56}, // LD A, 0x56
        {0xfe, 0x58}, // CP A, 0x58
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setA(0x5a);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x5a);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x80);
    expectedCPU.setFlags(0, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x67);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x67);
    expectedCPU.setFlags(1, 1, 0, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPU.setFlags(0, 1, 1, 0);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setA(0x56);
    expectedCPU.setFlags(0, 1, 1, 1);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs);
}

// 0xFF - RST 7
TEST_F(CPUTest, OpcodeTest_0xFF_RST_7)
{
    Program program({
        {0x31, 0xfe, 0xff}, // LD SP, 0xfffe
        {0x00},             // NOP
        {0x00},             // NOP
        {0x00},             // NOP
        {0x00},             // NOP
        {0xff},             // RST 7
    });

    loadSimpleProgram(program);

    auto expectedCPU = *cpu_;
    ExpectedCPUs expectedCPUs;

    expectedCPU.setSP(0xfffe);
    expectedCPU.setPC(0x0103);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0104);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0105);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0106);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setPC(0x0107);
    expectedCPUs.push_back(expectedCPU);

    expectedCPU.setSP(0xfffc);
    expectedCPU.ram()->set(0xfffd, 0x01);
    expectedCPU.ram()->set(0xfffc, 0x08);
    expectedCPU.setPC(0x0038);
    expectedCPUs.push_back(expectedCPU);

    runProgramAndCompareRegistersAndRam(program, expectedCPUs, true);
}