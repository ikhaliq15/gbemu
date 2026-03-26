#ifndef GBEMU_CONFIG
#define GBEMU_CONFIG

#include <optional>
#include <string>

namespace gbemu::config
{

class Config
{
  public:
    Config(bool enableBlarggSerialLogging, bool runHeadless, const std::string &dumpDisplayOnExitPath);

    [[nodiscard]] bool enableBlarggSerialLogging() const
    {
        return enableBlarggSerialLogging_;
    }

    [[nodiscard]] bool runHeadless() const
    {
        return runHeadless_;
    }

    [[nodiscard]] std::optional<std::string> dumpDisplayOnExitPath() const
    {
        if (dumpDisplayOnExitPath_.empty())
        {
            return std::nullopt;
        }
        return dumpDisplayOnExitPath_;
    }

  private:
    const bool enableBlarggSerialLogging_;
    const bool runHeadless_;
    const std::string dumpDisplayOnExitPath_;
};

} // namespace gbemu::config

#endif // GBEMU_CONFIG