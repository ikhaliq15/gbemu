#ifndef GBEMU_BACKEND_TIMER_H
#define GBEMU_BACKEND_TIMER_H

#include "gbemu/backend/bitutils.h"
#include "gbemu/backend/interrupt_controller.h"
#include "gbemu/backend/ram.h"

#include <vector>

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
        explicit Accumulator(T startValue) : accumulator_(startValue) {}

        void trigger() override { accumulator_ += 1; }

        T value() const { return accumulator_; }

        void reset() { accumulator_ = 0; }

      private:
        T accumulator_;
    };

    explicit Timer(InterruptController *interruptController);

    void init();
    void update(uint64_t deltaCycles);
    void addTimerListener(IListener *listener, uint64_t cycleModulo);

    // RAM::Owner
    uint8_t onReadOwnedByte(uint16_t address) override;
    void onWriteOwnedByte(uint16_t address, uint8_t newValue, uint8_t currentValue) override;

  private:
    struct TimerListenerInfo
    {
        IListener *listener;
        uint64_t modulo;
        uint64_t nextTriggerCycle;
    };

    class EnabledTimer : public IListener
    {
      public:
        EnabledTimer(uint8_t tacId, uint8_t *tima, const uint8_t *tma, const uint8_t *tac,
                     InterruptController *interruptController)
            : tacId_(tacId), tima_(tima), tma_(tma), tac_(tac), interruptController_(interruptController)
        {}

        void trigger() override
        {
            if (!enabled())
                return;

            if (*tima_ == 0xff)
            {
                *tima_ = *tma_;
                interruptController_->requestInterrupt(InterruptType::Timer);
            }
            else
            {
                *tima_ += 1;
            }
        }

      private:
        bool enabled() const { return getBit(*tac_, 2) && (*tac_ & 0b11) == tacId_; }

        const uint8_t tacId_;
        uint8_t *tima_;
        const uint8_t *tma_;
        const uint8_t *tac_;
        InterruptController *interruptController_;
    };

    // DIV register
    static constexpr uint64_t DIV_MODULO = 64;
    static constexpr uint8_t DIV_START_VALUE = 0xab;

    // TAC-controlled timers (id, modulo)
    static constexpr uint8_t TAC_ID_0 = 0x00;
    static constexpr uint8_t TAC_ID_1 = 0x01;
    static constexpr uint8_t TAC_ID_2 = 0x02;
    static constexpr uint8_t TAC_ID_3 = 0x03;

    static constexpr uint64_t TAC_MODULO_0 = 256;
    static constexpr uint64_t TAC_MODULO_1 = 4;
    static constexpr uint64_t TAC_MODULO_2 = 16;
    static constexpr uint64_t TAC_MODULO_3 = 64;

    bool initialized_;
    uint64_t cyclesSinceLaunch_;

    // IO registers
    uint8_t tima_;
    uint8_t tma_;
    uint8_t tac_;

    // Internal timer components
    Accumulator<uint8_t> divAccumulator_;
    EnabledTimer timer0_;
    EnabledTimer timer1_;
    EnabledTimer timer2_;
    EnabledTimer timer3_;

    std::vector<TimerListenerInfo> timerListeners_;
};

} // namespace gbemu::backend

#endif // GBEMU_BACKEND_TIMER_H
