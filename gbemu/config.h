#ifndef GBEMU_CONFIG
#define GBEMU_CONFIG

namespace gbemu::config
{

class Config
{
  public:
    Config(bool enableBlarggSerialLogging, bool runHeadless);

    [[nodiscard]] bool enableBlarggSerialLogging() const
    {
        return enableBlarggSerialLogging_;
    }

    [[nodiscard]] bool runHeadless() const
    {
        return runHeadless_;
    }

  private:
    const bool enableBlarggSerialLogging_;
    const bool runHeadless_;
};

} // namespace gbemu::config

#endif // GBEMU_CONFIG