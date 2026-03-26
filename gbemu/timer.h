#ifndef GBEMU_TIMER
#define GBEMU_TIMER

#include "gbemu/cpu.h"
#include "gbemu/ram.h"

#include <queue>

namespace gbemu
{

class Timer : public RAM::Owner
{
  public:
    class TimerListener
    {
      public:
        TimerListener()
        {
        }
        virtual ~TimerListener()
        {
        }
        virtual void timerTriggerHandler() = 0;
    };

    template <typename T> class Accumulator : public TimerListener
    {
      public:
        Accumulator(T startValue) : accumulator_(startValue)
        {
        }
        void timerTriggerHandler()
        {
            accumulator_ += 1;
        }
        T value() const
        {
            return accumulator_;
        }
        void reset()
        {
            accumulator_ = 0;
        }

      private:
        T accumulator_;
    };

    Timer(std::shared_ptr<CPU> cpu);

    void init();
    void update(uint64_t deltaCycles);

    uint8_t onReadOwnedByte(uint16_t address);
    void onWriteOwnedByte(uint16_t address, uint8_t newValue, uint8_t currentValue);

    void addTimerListener(const std::shared_ptr<TimerListener> listener, uint64_t cycleModulo)
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
        std::shared_ptr<TimerListener> listener_;
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

    class EnabledTimer : public TimerListener
    {
      public:
        EnabledTimer(uint8_t tacId, std::shared_ptr<uint8_t> tima, std::shared_ptr<uint8_t> tma,
                     std::shared_ptr<uint8_t> tac, std::shared_ptr<CPU> cpu)
            : tacId_(tacId), tima_(tima), tma_(tma), tac_(tac), cpu_(cpu)
        {
        }
        void timerTriggerHandler()
        {
            if ((*tac_ & 0x04) != 0x00 && (*tac_ & 0x03) == tacId_)
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
        const uint8_t tacId_;

        const std::shared_ptr<uint8_t> tima_;
        const std::shared_ptr<uint8_t> tma_;
        const std::shared_ptr<uint8_t> tac_;

        const std::shared_ptr<CPU> cpu_;
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

    std::shared_ptr<Accumulator<uint8_t>> divAccumulator_;

    // TODO: make these non-static?
    std::shared_ptr<CPU> cpu_;

    std::shared_ptr<uint8_t> tima_;
    std::shared_ptr<uint8_t> tma_;
    std::shared_ptr<uint8_t> tac_;

    std::priority_queue<TimerListenerInfo, std::vector<TimerListenerInfo>, TimerListenerInfoCompare> timerListeners_;
};

} // namespace gbemu

#endif // GBEMU_TIMER