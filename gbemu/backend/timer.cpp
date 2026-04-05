#include "gbemu/backend/timer.h"

namespace gbemu::backend
{

Timer::Timer(CPU *cpu)
    : initialized_(false), cyclesSinceLaunch_(0), divAccumulator_(DIV_REGISTER_START_VALUE), cpu_(cpu), tima_(0x00),
      tma_(0x00), tac_(0x00), timer0_(TIMER_0_TAC_ID, &tima_, &tma_, &tac_, cpu),
      timer1_(TIMER_1_TAC_ID, &tima_, &tma_, &tac_, cpu), timer2_(TIMER_2_TAC_ID, &tima_, &tma_, &tac_, cpu),
      timer3_(TIMER_3_TAC_ID, &tima_, &tma_, &tac_, cpu)
{
}

void Timer::init()
{
    addTimerListener(&divAccumulator_, DIV_REGISTER_MODULO);

    addTimerListener(&timer0_, TIMER_0_MODULO);
    addTimerListener(&timer1_, TIMER_1_MODULO);
    addTimerListener(&timer2_, TIMER_2_MODULO);
    addTimerListener(&timer3_, TIMER_3_MODULO);

    initialized_ = true;
}

void Timer::update(uint64_t deltaCycles)
{
    if (!initialized_)
        throw std::runtime_error("Cannot increment an uninitialized timer.");

    uint64_t targetCycle = cyclesSinceLaunch_ + deltaCycles;

    for (auto &info : timerListeners_)
    {
        while (info.nextCycleCountTrigger_ <= targetCycle)
        {
            info.listener_->trigger();
            info.nextCycleCountTrigger_ += info.listenerModulo_;
        }
    }

    cyclesSinceLaunch_ = targetCycle;
}

auto Timer::onReadOwnedByte(uint16_t address) -> uint8_t
{
    switch (address)
    {
    case RAM::DIV:
        return divAccumulator_.value();
    case RAM::TIMA:
        return tima_;
    case RAM::TMA:
        return tma_;
    case RAM::TAC:
        return tac_;
    }
    // TODO: throw exception?
    return 0x00;
}

void Timer::onWriteOwnedByte(uint16_t address, uint8_t newValue, uint8_t currentValue)
{
    switch (address)
    {
    case RAM::DIV:
        divAccumulator_.resetAccumulator();
        break;
    case RAM::TIMA:
        tima_ = newValue;
        break;
    case RAM::TMA:
        tma_ = newValue;
        break;
    case RAM::TAC:
        tac_ = newValue;
        break;
    }
    // TODO: throw exception?
}

} // namespace gbemu::backend
