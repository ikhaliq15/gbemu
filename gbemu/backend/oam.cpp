#include "gbemu/backend/oam.h"

#include "gbemu/backend/bitutils.h"

namespace gbemu::backend
{

OAM::OAM(RAM *ram) : ram_(ram) {}

void OAM::trigger()
{
    switch (state_)
    {
    case State::Idle: return;
    case State::Starting:
        if (--startDelayCyclesRemaining_ > 0)
        {
            return;
        }
        state_ = State::Active;
        [[fallthrough]];
    case State::Active:
        transferOneByte();
        if (transferIndex_ >= OAM_SIZE)
        {
            state_ = State::Ending;
        }
        return;
    case State::Ending: state_ = State::Idle; return;
    }
}

void OAM::transferOneByte()
{
    const auto srcAddr = concatBytes(transferSourceAddress_, transferIndex_);
    oamData_[transferIndex_] = ram_->get(srcAddr);
    transferIndex_ += 1;
}

auto OAM::onReadOwnedByte(uint16_t address) -> uint8_t
{
    if (address == RAM::DMA)
    {
        return transferSourceAddress_;
    }
    if (busDriven())
    {
        return 0xff;
    }
    return oamData_[address - RAM::OAM];
}

void OAM::onWriteOwnedByte(uint16_t address, uint8_t newValue, uint8_t currentValue)
{
    if (address == RAM::DMA)
    {
        transferSourceAddress_ = newValue;
        transferIndex_ = 0;
        startDelayCyclesRemaining_ = DMA_START_DELAY_CYCLES;
        state_ = State::Starting;
        return;
    }
    if (busDriven())
    {
        return;
    }
    oamData_[address - RAM::OAM] = newValue;
}

} // namespace gbemu::backend
