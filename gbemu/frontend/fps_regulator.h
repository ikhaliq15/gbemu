#ifndef GBEMU_FRONTEND_FPS_REGULATOR_H
#define GBEMU_FRONTEND_FPS_REGULATOR_H

#include <chrono>
#include <thread>

namespace gbemu::frontend
{

class FPSRegulator
{
  public:
    explicit FPSRegulator(double targetFPS)
        : frameDuration_(std::chrono::nanoseconds(static_cast<int64_t>(1.0 / targetFPS * 1e9)))
    {
        nextFrameTime_ = std::chrono::steady_clock::now() + frameDuration_;
    }

    void enable()
    {
        enabled_ = true;
        nextFrameTime_ = std::chrono::steady_clock::now() + frameDuration_;
    }
    void disable() { enabled_ = false; }

    void waitForNextFrame()
    {
        if (!enabled_)
        {
            return;
        }

        const auto now = std::chrono::steady_clock::now();
        if (now < nextFrameTime_)
        {
            std::this_thread::sleep_until(nextFrameTime_);
        }
        nextFrameTime_ += frameDuration_;

        if (nextFrameTime_ < now)
        {
            nextFrameTime_ = now + frameDuration_;
        }
    }

  private:
    bool enabled_ = true;
    const std::chrono::nanoseconds frameDuration_;
    std::chrono::steady_clock::time_point nextFrameTime_;
};

} // namespace gbemu::frontend

#endif // GBEMU_FRONTEND_FPS_REGULATOR_H
