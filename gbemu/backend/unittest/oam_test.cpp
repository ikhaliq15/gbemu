#include <gtest/gtest.h>

#include "gbemu/backend/interrupt_controller.h"
#include "gbemu/backend/oam.h"
#include "gbemu/backend/ram.h"
#include "gbemu/backend/timer.h"

namespace
{

using gbemu::backend::InterruptController;
using gbemu::backend::OAM;
using gbemu::backend::RAM;
using gbemu::backend::Timer;

constexpr int START_DELAY_CYCLES = 2;

class OAMTest : public testing::Test
{
  protected:
    void SetUp() override
    {
        ram_ = std::make_unique<RAM>(1 << 16);
        interruptController_ = std::make_unique<InterruptController>(ram_.get());
        timer_ = std::make_unique<Timer>(interruptController_.get());
        oam_ = std::make_unique<OAM>(ram_.get());

        ram_->addOwner(RAM::DMA, oam_.get());
        for (uint16_t addr = RAM::OAM; addr < RAM::OAM + OAM::OAM_SIZE; ++addr)
        {
            ram_->addOwner(addr, oam_.get());
        }

        timer_->addTimerListener(oam_.get(), 1);
        timer_->init();
    }

    void advanceCycles(int count)
    {
        for (int i = 0; i < count; ++i)
        {
            timer_->update(1);
        }
    }

    void fillSourcePage(uint8_t page, uint8_t value)
    {
        const uint16_t base = static_cast<uint16_t>(page) << 8;
        for (uint16_t i = 0; i < OAM::OAM_SIZE; ++i)
        {
            ram_->set(base + i, value);
        }
    }

    void beginDmaTransfer(uint8_t sourcePage) { ram_->set(RAM::DMA, sourcePage); }

    auto readOamByte(uint8_t index) -> uint8_t { return ram_->get(RAM::OAM + index); }

    std::unique_ptr<RAM> ram_;
    std::unique_ptr<InterruptController> interruptController_;
    std::unique_ptr<Timer> timer_;
    std::unique_ptr<OAM> oam_;
};

TEST_F(OAMTest, ReadsReturnActualDataBeforeAnyDma)
{
    ram_->set(RAM::OAM + 0, 0xAA);
    ram_->set(RAM::OAM + 159, 0xBB);

    EXPECT_EQ(readOamByte(0), 0xAA);
    EXPECT_EQ(readOamByte(159), 0xBB);
}

TEST_F(OAMTest, ReadsStayUnlockedDuringDmaStartDelay)
{
    ram_->set(RAM::OAM, 0xAA);
    fillSourcePage(0x80, 0xCC);

    beginDmaTransfer(0x80);
    EXPECT_EQ(readOamByte(0), 0xAA);

    advanceCycles(START_DELAY_CYCLES - 1);
    EXPECT_EQ(readOamByte(0), 0xAA);
}

TEST_F(OAMTest, ReadsLockedOnceDmaBecomesActive)
{
    ram_->set(RAM::OAM, 0xAA);
    fillSourcePage(0x80, 0xCC);

    beginDmaTransfer(0x80);
    advanceCycles(START_DELAY_CYCLES);

    EXPECT_EQ(readOamByte(0), 0xff);
}

TEST_F(OAMTest, ReadsStayLockedThroughLastTransferAndOneMoreCycle)
{
    fillSourcePage(0x80, 0xCC);

    beginDmaTransfer(0x80);
    advanceCycles(START_DELAY_CYCLES);
    advanceCycles(OAM::OAM_SIZE - 1);

    EXPECT_EQ(readOamByte(0), 0xff);
}

TEST_F(OAMTest, ReadsResumeNormalOnceDmaCompletes)
{
    fillSourcePage(0x80, 0xCC);

    beginDmaTransfer(0x80);
    advanceCycles(START_DELAY_CYCLES + OAM::OAM_SIZE);

    EXPECT_EQ(readOamByte(0), 0xCC);
    EXPECT_EQ(readOamByte(159), 0xCC);
}

TEST_F(OAMTest, WritesIgnoredWhileDmaActive)
{
    fillSourcePage(0x80, 0xCC);

    beginDmaTransfer(0x80);
    advanceCycles(START_DELAY_CYCLES);

    ram_->set(RAM::OAM + 0, 0xBB);

    advanceCycles(OAM::OAM_SIZE);
    EXPECT_EQ(readOamByte(0), 0xCC);
}

TEST_F(OAMTest, ReadingDmaRegisterReturnsLastWrittenSourcePage)
{
    ram_->set(RAM::DMA, 0x80);
    EXPECT_EQ(ram_->get(RAM::DMA), 0x80);

    advanceCycles(START_DELAY_CYCLES + 50);
    EXPECT_EQ(ram_->get(RAM::DMA), 0x80);
}

TEST_F(OAMTest, WritingDmaRegisterMidTransferRestartsFromNewSource)
{
    fillSourcePage(0x80, 0xCC);
    fillSourcePage(0x90, 0xDD);

    beginDmaTransfer(0x80);
    advanceCycles(START_DELAY_CYCLES + 50);

    beginDmaTransfer(0x90);
    advanceCycles(START_DELAY_CYCLES + OAM::OAM_SIZE);

    EXPECT_EQ(readOamByte(0), 0xDD);
    EXPECT_EQ(readOamByte(159), 0xDD);
}

} // namespace
