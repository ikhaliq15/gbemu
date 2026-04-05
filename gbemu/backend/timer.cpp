#include "gbemu/backend/timer.h"

#include <stdexcept>

namespace gbemu::backend
{

Timer::Timer(InterruptController *cpu)
    : initialized_(false), cyclesSinceLaunch_(0), tima_(0x00), tma_(0x00), tac_(0x00), divAccumulator_(DIV_START_VALUE),
      timer0_(TAC_ID_0, &tima_, &tma_, &tac_, cpu), timer1_(TAC_ID_1, &tima_, &tma_, &tac_, cpu),
      timer2_(TAC_ID_2, &tima_, &tma_, &tac_, cpu), timer3_(TAC_ID_3, &tima_, &tma_, &tac_, cpu)
{}

void Timer::init()
{
    addTimerListener(&divAccumulator_, DIV_MODULO);

    addTimerListener(&timer0_, TAC_MODULO_0);
    addTimerListener(&timer1_, TAC_MODULO_1);
    addTimerListener(&timer2_, TAC_MODULO_2);
    addTimerListener(&timer3_, TAC_MODULO_3);

    initialized_ = true;
}

void Timer::update(uint64_t deltaCycles)
{
    if (!initialized_)
        throw std::runtime_error("Cannot increment an uninitialized timer.");

    uint64_t targetCycle = cyclesSinceLaunch_ + deltaCycles;

    for (auto &info : timerListeners_)
    {
        while (info.nextTriggerCycle <= targetCycle)
        {
            info.listener->trigger();
            info.nextTriggerCycle += info.modulo;
        }
    }

    cyclesSinceLaunch_ = targetCycle;
}

void Timer::addTimerListener(IListener *listener, uint64_t cycleModulo)
{
    if (initialized_)
        throw std::runtime_error("Cannot add timer listener after timer is initialized.");
    if (cycleModulo == 0)
        throw std::runtime_error("Cycle modulo must be positive.");
    timerListeners_.push_back({listener, cycleModulo, cycleModulo});
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
    return 0x00;
}

void Timer::onWriteOwnedByte(uint16_t address, uint8_t newValue, uint8_t currentValue)
{
    switch (address)
    {
    case RAM::DIV:
        divAccumulator_.reset();
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
}

} // namespace gbemu::backend
