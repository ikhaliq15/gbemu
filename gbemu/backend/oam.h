#ifndef GBEMU_BACKEND_OAM_H
#define GBEMU_BACKEND_OAM_H

#include "gbemu/backend/ram.h"
#include "gbemu/backend/timer.h"

#include <array>
#include <cstdint>

namespace gbemu::backend
{

class OAM : public Timer::IListener, public RAM::Owner
{
  public:
    static constexpr uint8_t OAM_SIZE = 160;

    explicit OAM(RAM *ram);

    // Timer::IListener
    void trigger() override;

    // RAM::Owner
    auto onReadOwnedByte(uint16_t address) -> uint8_t override;
    void onWriteOwnedByte(uint16_t address, uint8_t newValue, uint8_t currentValue) override;

  private:
    static constexpr uint8_t DMA_START_DELAY_CYCLES = 2;

    enum class State : uint8_t
    {
        Idle,
        Starting,
        Active,
        // Bus stays driven for one cycle after the last byte is transferred.
        Ending,
    };

    [[nodiscard]] auto busDriven() const -> bool { return state_ == State::Active || state_ == State::Ending; }

    void transferOneByte();

    RAM *ram_;
    std::array<uint8_t, OAM_SIZE> oamData_{};

    State state_ = State::Idle;
    uint8_t transferSourceAddress_ = 0;
    uint8_t transferIndex_ = 0;
    uint8_t startDelayCyclesRemaining_ = 0;
};

} // namespace gbemu::backend

#endif // GBEMU_BACKEND_OAM_H
