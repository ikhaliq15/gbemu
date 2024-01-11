#include "timer.h"

namespace gbemu {

    Timer::Timer()
    : initialized_(false)
    , framesSinceLaunch_(0)
    {
    }

    void Timer::init()
    {
        initialized_ = true;
    }

    void Timer::incrementTimer(uint64_t deltaCycles)
    {
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
        return 0x00;
    }

    void Timer::onWriteOwnedByte(uint16_t address, uint8_t newValue, uint8_t currentValue)
    {
        return;
    }

} // gbemu