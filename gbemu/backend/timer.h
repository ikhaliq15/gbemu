#ifndef GBEMU_BACKEND_TIMER_H
#define GBEMU_BACKEND_TIMER_H

#include "gbemu/backend/cpu.h"
#include "gbemu/backend/ram.h"

#include <memory>
#include <queue>

namespace gbemu::backend
{

class Timer : public RAM::Owner
{
  public:
    class IListener
    {
      public:
        IListener() = default;
        virtual ~IListener() = default;

        virtual void trigger() = 0;
    };

    template <typename T> class Accumulator : public IListener
    {
      public:
        Accumulator(T startValue) : accumulator_(startValue)
        {
        }

        void trigger()
        {
            accumulator_ += 1;
        }

        T value() const
        {
            return accumulator_;
        }

        void resetAccumulator()
        {
            accumulator_ = 0;
        }

      private:
        T accumulator_;
    };

    Timer(CPU *cpu);

    void init();
    void update(uint64_t deltaCycles);

    uint8_t onReadOwnedByte(uint16_t address);
    void onWriteOwnedByte(uint16_t address, uint8_t newValue, uint8_t currentValue);

    void addTimerListener(IListener *listener, uint64_t cycleModulo)
    {
        if (initialized_)
            throw std::runtime_error("Cannot add timer listener after timer is initalized.");
        if (cycleModulo == 0)
            throw std::runtime_error("Cycle modulo most be positive.");
        timerListeners_.push(TimerListenerInfo{listener, cycleModulo, cycleModulo});
    }

  private:
    class TimerListenerInfo
    {
      public:
        IListener *listener_;
        uint64_t listenerModulo_;
        uint64_t nextCycleCountTrigger_;
    };

    class TimerListenerInfoCompare
    {
      public:
        bool operator()(TimerListenerInfo a, TimerListenerInfo b)
        {
            return a.nextCycleCountTrigger_ > b.nextCycleCountTrigger_;
        }
    };

    class EnabledTimer : public IListener
    {
      public:
        EnabledTimer(uint8_t tacId, uint8_t *tima, uint8_t *tma, uint8_t *tac, CPU *cpu)
            : tacId_(tacId), tima_(tima), tma_(tma), tac_(tac), cpu_(cpu)
        {
        }
        void trigger()
        {
            if (enabled())
            {
                if (*tima_ == 0xff)
                {
                    *tima_ = *tma_;
                    cpu_->requestInterupt(CPU::Interrupt::TIMER);
                    return;
                }
                *tima_ = *tima_ + 1;
            }
        }

      private:
        bool enabled() const
        {
            const auto masterTACEnabled = getBit(*tac_, 2);
            if (!masterTACEnabled)
            {
                return false;
            }

            const auto enabledTACId = *tac_ & 0b11;
            return enabledTACId == tacId_;
        }

        const uint8_t tacId_;

        uint8_t *tima_;
        const uint8_t *tma_;
        const uint8_t *tac_;

        CPU *cpu_;
    };

    static constexpr uint64_t DIV_REGISTER_MODULO = 64;
    static constexpr uint8_t DIV_REGISTER_START_VALUE = 0xab;

    static constexpr uint64_t TIMER_0_MODULO = 256;
    static constexpr uint8_t TIMER_0_TAC_ID = 0x00;
    static constexpr uint64_t TIMER_1_MODULO = 4;
    static constexpr uint8_t TIMER_1_TAC_ID = 0x01;
    static constexpr uint64_t TIMER_2_MODULO = 16;
    static constexpr uint8_t TIMER_2_TAC_ID = 0x02;
    static constexpr uint64_t TIMER_3_MODULO = 64;
    static constexpr uint8_t TIMER_3_TAC_ID = 0x03;

    bool initialized_;
    uint64_t cyclesSinceLaunch_;

    std::unique_ptr<Accumulator<uint8_t>> divAccumulator_;

    CPU *cpu_;

    std::unique_ptr<EnabledTimer> timer0_;
    std::unique_ptr<EnabledTimer> timer1_;
    std::unique_ptr<EnabledTimer> timer2_;
    std::unique_ptr<EnabledTimer> timer3_;

    std::unique_ptr<uint8_t> tima_;
    std::unique_ptr<uint8_t> tma_;
    std::unique_ptr<uint8_t> tac_;

    std::priority_queue<TimerListenerInfo, std::vector<TimerListenerInfo>, TimerListenerInfoCompare> timerListeners_;
};

} // namespace gbemu::backend

#endif // GBEMU_BACKEND_TIMER_H
