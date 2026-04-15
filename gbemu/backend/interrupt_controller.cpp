#include "gbemu/backend/interrupt_controller.h"

namespace gbemu::backend
{

namespace
{

constexpr uint8_t interruptMask(InterruptType interrupt)
{
    switch (interrupt)
    {
    case InterruptType::VBlank: return 1 << 0;
    case InterruptType::Stat: return 1 << 1;
    case InterruptType::Timer: return 1 << 2;
    }

    return 0;
}

} // namespace

InterruptController::InterruptController(RAM *ram) : ram_(ram) {}

void InterruptController::requestInterrupt(InterruptType interrupt)
{
    const auto current = ram_->get(RAM::IF);
    ram_->set(RAM::IF, static_cast<uint8_t>(current | interruptMask(interrupt)));
}

void InterruptController::clearInterrupt(InterruptType interrupt)
{
    const auto current = ram_->get(RAM::IF);
    ram_->set(RAM::IF, static_cast<uint8_t>(current & ~interruptMask(interrupt)));
}

} // namespace gbemu::backend
