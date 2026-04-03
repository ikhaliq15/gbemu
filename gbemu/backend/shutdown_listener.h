#ifndef GBEMU_BACKEND_SHUTDOWN_LISTENER_H
#define GBEMU_BACKEND_SHUTDOWN_LISTENER_H

namespace gbemu::backend
{

class ShutDownListener
{
  public:
    ShutDownListener() = default;
    virtual ~ShutDownListener() = default;

    virtual void onShutDown() = 0;
};

} // namespace gbemu::backend

#endif // GBEMU_BACKEND_SHUTDOWN_LISTENER_H
