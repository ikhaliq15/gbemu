#ifndef GBEMU_TIMER
#define GBEMU_TIMER

#include "ram.h"

#include <chrono>
#include <queue>

namespace gbemu {

    class Timer: public RAM::Owner
    {
    public:
        class TimerListener
        {
        public:
            TimerListener(){}
            virtual ~TimerListener(){}
            virtual void timerTriggerHandler() = 0;
        };

        template<typename T>
        class Accumulator: public TimerListener
        {
        public:
            Accumulator<T>(T startValue): accumulator_(startValue) {}
            void timerTriggerHandler() { accumulator_ += 1; }
            T value() const { return accumulator_; }
            void reset() { accumulator_ = 0; }
        private:
            T accumulator_;
        };

        Timer();

        void init();
        void update();

        uint8_t onReadOwnedByte(uint16_t address);
        void onWriteOwnedByte(uint16_t address, uint8_t newValue, uint8_t currentValue);

        void addTimerListener(const std::shared_ptr<TimerListener> listener, uint64_t listenerFrequency)
        {
            if (initialized_)
                throw std::runtime_error("Cannot add timer listener after timer is initalized.");
            if (listenerFrequency == 0)
                throw std::runtime_error("Cannot add timer listener that is triggered at a frequency of 0Hz.");

            const auto gap = (1.0/(double)listenerFrequency) * NANOS_PER_SECOND;
            timerListeners_.push(TimerListenerInfo{listener, DurationNS(gap), DurationNS(gap)});
        }

    private:
        using Nanoseconds = std::chrono::nanoseconds;
        using Clock = std::chrono::high_resolution_clock;
        using Timepoint = std::chrono::steady_clock::time_point;
        using DurationNS = std::chrono::duration<double, std::nano>;

        class TimerListenerInfo
        {
        public:
            std::shared_ptr<TimerListener> listener_;
            DurationNS listenerInterval_;
            DurationNS nextTimepointTrigger_;
        };

        class TimerListenerInfoCompare
        {
        public:
            bool operator() (TimerListenerInfo a, TimerListenerInfo b)
            {
                return a.nextTimepointTrigger_ > b.nextTimepointTrigger_;
            }
        };

        static constexpr uint64_t NANOS_PER_SECOND = 1000000000;

        static constexpr uint64_t DIV_REGISTER_MODULO  = 64;
        static constexpr uint64_t DIV_REGISTER_FREQUENCY  = 16384;
        static constexpr uint8_t DIV_REGISTER_START_VALUE  = 0xab;

        bool initialized_;
        Timepoint launchTime_;

        std::shared_ptr<Accumulator<uint8_t>> divAccumulator_;

        std::priority_queue<TimerListenerInfo, std::vector<TimerListenerInfo>, TimerListenerInfoCompare> timerListeners_;
    };

} // gbemu

#endif // GBEMU_TIMER