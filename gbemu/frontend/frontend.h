#ifndef GBEMU_FRONTEND_FRONTEND_H
#define GBEMU_FRONTEND_FRONTEND_H

namespace gbemu::backend
{

class Gameboy;

} // namespace gbemu::backend

namespace gbemu::frontend
{

enum class FrontEndMode
{
    NORMAL = 1,
    DEBUGGER = 3,
    EXIT = 2,
};

class IFrontend
{
  public:
    virtual bool init(gbemu::backend::Gameboy *gameboy) = 0;
    virtual FrontEndMode update() = 0;
    virtual void done() = 0;

    virtual ~IFrontend() = default;
};

} // namespace gbemu::frontend

#endif // GBEMU_FRONTEND_FRONTEND_H
