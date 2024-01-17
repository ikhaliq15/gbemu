#include "timer.h"

namespace gbemu {

    Timer::Timer()
    : initialized_(false)
    , launchTime_(Clock::now())
    , divAccumulator_(std::make_shared<Accumulator<uint8_t>>(DIV_REGISTER_START_VALUE))
    {
    }

    void Timer::init()
    {
        addTimerListener(divAccumulator_, DIV_REGISTER_FREQUENCY);
        launchTime_ = Clock::now();
        initialized_ = true;
    }

    void Timer::update()
    {
        if (!initialized_)
            throw std::runtime_error("Cannot increment an uninitialized tiemr.");

        const auto timeSinceLaunch = Clock::now() - launchTime_;

        if (timerListeners_.empty())
            return;

        while (timerListeners_.top().nextTimepointTrigger_ <= timeSinceLaunch)
        {
            auto info = timerListeners_.top();
            timerListeners_.pop();

            info.listener_->timerTriggerHandler();
            info.nextTimepointTrigger_ = timeSinceLaunch + info.listenerInterval_;

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
        }
        // TODO: throw exception?
    }

} // gbemu