#include "timer.h"

namespace gbemu {

    Timer::Timer(std::shared_ptr<CPU> cpu)
    : initialized_(false)
    , cyclesSinceLaunch_(0)
    , divAccumulator_(std::make_shared<Accumulator<uint8_t>>(DIV_REGISTER_START_VALUE))
    , cpu_(cpu)
    , tima_(std::make_shared<uint8_t>(0x00))
    , tma_(std::make_shared<uint8_t>(0x00))
    , tac_(std::make_shared<uint8_t>(0x00))
    {
    }

    void Timer::init()
    {
        addTimerListener(divAccumulator_, DIV_REGISTER_MODULO);

        addTimerListener(std::make_shared<EnabledTimer>(TIMER_0_TAC_ID, tima_, tma_, tac_, cpu_), TIMER_0_MODULO);
        addTimerListener(std::make_shared<EnabledTimer>(TIMER_1_TAC_ID, tima_, tma_, tac_, cpu_), TIMER_1_MODULO);
        addTimerListener(std::make_shared<EnabledTimer>(TIMER_2_TAC_ID, tima_, tma_, tac_, cpu_), TIMER_2_MODULO);
        addTimerListener(std::make_shared<EnabledTimer>(TIMER_3_TAC_ID, tima_, tma_, tac_, cpu_), TIMER_3_MODULO);

        initialized_ = true;
    }

    void Timer::update(uint64_t deltaCycles)
    {
        if (!initialized_)
            throw std::runtime_error("Cannot increment an uninitialized tiemr.");

        cyclesSinceLaunch_ += deltaCycles;

        if (timerListeners_.empty())
            return;

        while (timerListeners_.top().nextCycleCountTrigger_ <= cyclesSinceLaunch_)
        {
            auto info = timerListeners_.top();
            timerListeners_.pop();

            info.listener_->timerTriggerHandler();
            info.nextCycleCountTrigger_ = cyclesSinceLaunch_ + info.listenerModulo_;

            timerListeners_.push(info);
        }
    }

    uint8_t Timer::onReadOwnedByte(uint16_t address)
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
                divAccumulator_.reset();
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

} // gbemu