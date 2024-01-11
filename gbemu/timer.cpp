#include "timer.h"

namespace gbemu {

    Timer::Timer()
    : initialized_(false)
    , framesSinceLaunch_(0)
    , divAccumulator_(std::make_shared<Accumulator<uint8_t>>(DIV_REGISTER_START_VALUE))
    {
    }

    void Timer::init()
    {
        addCycleListener(divAccumulator_, DIV_REGISTER_MODULO);

        initialized_ = true;
    }

    void Timer::incrementTimer(uint64_t deltaCycles)
    {
        if (!initialized_)
            throw std::runtime_error("Cannot increment an uninitialized tiemr.");

        framesSinceLaunch_ += deltaCycles;

        if (cycleListeners_.empty())
            return;

        while (cycleListeners_.top().nextCycleCountTrigger_ <= framesSinceLaunch_)
        {
            auto info = cycleListeners_.top();
            cycleListeners_.pop();

            info.listener_->cycleTriggerHandler(framesSinceLaunch_);
            info.nextCycleCountTrigger_ = framesSinceLaunch_ + info.listenerModulo_;

            cycleListeners_.push(info);
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