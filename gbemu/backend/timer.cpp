#include "gbemu/backend/timer.h"

namespace gbemu::backend
{

Timer::Timer(CPU *cpu)
    : initialized_(false), cyclesSinceLaunch_(0),
      divAccumulator_(std::make_unique<Accumulator<uint8_t>>(DIV_REGISTER_START_VALUE)), cpu_(cpu),
      tima_(std::make_unique<uint8_t>(0x00)), tma_(std::make_unique<uint8_t>(0x00)),
      tac_(std::make_unique<uint8_t>(0x00))
{
}

void Timer::init()
{
    addTimerListener(divAccumulator_.get(), DIV_REGISTER_MODULO);

    timer0_ = std::make_unique<EnabledTimer>(TIMER_0_TAC_ID, tima_.get(), tma_.get(), tac_.get(), cpu_);
    timer1_ = std::make_unique<EnabledTimer>(TIMER_1_TAC_ID, tima_.get(), tma_.get(), tac_.get(), cpu_);
    timer2_ = std::make_unique<EnabledTimer>(TIMER_2_TAC_ID, tima_.get(), tma_.get(), tac_.get(), cpu_);
    timer3_ = std::make_unique<EnabledTimer>(TIMER_3_TAC_ID, tima_.get(), tma_.get(), tac_.get(), cpu_);

    addTimerListener(timer0_.get(), TIMER_0_MODULO);
    addTimerListener(timer1_.get(), TIMER_1_MODULO);
    addTimerListener(timer2_.get(), TIMER_2_MODULO);
    addTimerListener(timer3_.get(), TIMER_3_MODULO);

    initialized_ = true;
}

void Timer::update(uint64_t deltaCycles)
{
    if (!initialized_)
        throw std::runtime_error("Cannot increment an uninitialized timer.");

    uint64_t targetCycle = cyclesSinceLaunch_ + deltaCycles;

    while (!timerListeners_.empty() && timerListeners_.top().nextCycleCountTrigger_ <= targetCycle)
    {
        auto info = timerListeners_.top();
        timerListeners_.pop();

        // Fast-forward to trigger point
        cyclesSinceLaunch_ = info.nextCycleCountTrigger_;
        info.listener_->trigger();
        info.nextCycleCountTrigger_ += info.listenerModulo_;

        timerListeners_.push(info);
    }

    cyclesSinceLaunch_ = targetCycle;
}

auto Timer::onReadOwnedByte(uint16_t address) -> uint8_t
{
    switch (address)
    {
    case RAM::DIV:
        return divAccumulator_->value();
        break;
    case RAM::TIMA:
        return *tima_;
    case RAM::TMA:
        return *tma_;
    case RAM::TAC:
        return *tac_;
    }
    // TODO: throw exception?
    return 0x00;
}

void Timer::onWriteOwnedByte(uint16_t address, uint8_t newValue, uint8_t currentValue)
{
    switch (address)
    {
    case RAM::DIV:
        divAccumulator_->resetAccumulator();
        break;
    case RAM::TIMA:
        *tima_ = newValue;
        break;
    case RAM::TMA:
        *tma_ = newValue;
        break;
    case RAM::TAC:
        *tac_ = newValue;
        break;
    }
    // TODO: throw exception?
}

} // namespace gbemu::backend
