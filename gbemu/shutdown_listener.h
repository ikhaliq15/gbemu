#ifndef GBEMU_SHUTDOWN_LISTENER_H
#define GBEMU_SHUTDOWN_LISTENER_H

namespace gbemu
{

class ShutDownListener
{
  public:
    ShutDownListener() = default;
    virtual ~ShutDownListener() = default;

    virtual void onShutDown() = 0;
};

} // namespace gbemu

#endif // GBEMU_SHUTDOWN_LISTENER_H
