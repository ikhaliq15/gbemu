#ifndef GBEMU_TIMER
#define GBEMU_TIMER

#include "ram.h"

#include <queue>

namespace gbemu {

    class Timer: public RAM::Owner
    {
    public:
        class CycleListener
        {
        public:
            CycleListener(){}
            virtual ~CycleListener(){}
            virtual void cycleTriggerHandler(uint64_t cycleCount) = 0;
        };

        template<typename T>
        class Accumulator: public CycleListener
        {
        public:
            Accumulator<T>(T startValue): accumulator_(startValue) {}
            void cycleTriggerHandler(uint64_t cycleCount) { accumulator_ += 1; }
            T value() const { return accumulator_; }
            void reset() { accumulator_ = 0; }
        private:
            T accumulator_;
        };

        Timer();

        void init();
        void incrementTimer(uint64_t deltaCycles);

        uint8_t onReadOwnedByte(uint16_t address);
        void onWriteOwnedByte(uint16_t address, uint8_t newValue, uint8_t currentValue);

        void addCycleListener(const std::shared_ptr<CycleListener> listener, uint64_t cycleModulo)
        {
            if (initialized_)
                throw std::runtime_error("Cannot add cycle listener after timer is initalized.");
            if (cycleModulo == 0)
                throw std::runtime_error("Cannot add cycle listener that is trigger every 0 cycles.");
            cycleListeners_.push({listener, cycleModulo, cycleModulo});
        }

    private:
        class CycleListenerInfo
        {
        public:
            std::shared_ptr<CycleListener> listener_;
            uint64_t listenerModulo_;
            uint64_t nextCycleCountTrigger_;
        };

        class CycleListenerInfoCompare
        {
        public:
            bool operator() (CycleListenerInfo a, CycleListenerInfo b)
            {
                return a.nextCycleCountTrigger_ > b.nextCycleCountTrigger_;
            }
        };

        static constexpr uint64_t DIV_REGISTER_MODULO  = 64;
        static constexpr uint8_t DIV_REGISTER_START_VALUE  = 0xab;

        bool initialized_;
        uint64_t framesSinceLaunch_;

        std::shared_ptr<Accumulator<uint8_t>> divAccumulator_;

        std::priority_queue<CycleListenerInfo, std::vector<CycleListenerInfo>, CycleListenerInfoCompare> cycleListeners_;
    };

} // gbemu

#endif // GBEMU_TIMER