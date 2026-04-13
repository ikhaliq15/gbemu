#ifndef GBEMU_BACKEND_INTERRUPT_CONTROLLER_H
#define GBEMU_BACKEND_INTERRUPT_CONTROLLER_H

#include "gbemu/backend/ram.h"

namespace gbemu::backend
{

enum class InterruptType
{
    VBlank,
    Stat,
    Timer,
};

class InterruptController
{
  public:
    explicit InterruptController(RAM *ram);

    void requestInterrupt(InterruptType interrupt);
    void clearInterrupt(InterruptType interrupt);

  private:
    RAM *ram_;
};

} // namespace gbemu::backend

#endif
