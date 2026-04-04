#ifndef GBEMU_FRONTEND_FRONTEND_H
#define GBEMU_FRONTEND_FRONTEND_H

namespace gbemu::backend
{

class Gameboy;

} // namespace gbemu::backend

namespace gbemu::frontend
{

class IFrontend
{
  public:
    virtual bool init(gbemu::backend::Gameboy *gameboy) = 0;
    virtual bool update() = 0;
    virtual void done() = 0;

    virtual ~IFrontend()
    {
    }
};

} // namespace gbemu::frontend

#endif // GBEMU_FRONTEND_FRONTEND_H
