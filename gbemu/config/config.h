#ifndef GBEMU_CONFIG_CONFIG_H
#define GBEMU_CONFIG_CONFIG_H

#include <optional>
#include <string>

namespace gbemu::config
{

class Config
{
  public:
    Config(bool dumpSerial, bool runHeadless, std::string dumpDisplayOnExitPath);

    [[nodiscard]] bool dumpSerial() const { return dumpSerial_; }

    [[nodiscard]] bool runHeadless() const { return runHeadless_; }

    [[nodiscard]] std::optional<std::string> dumpDisplayOnExitPath() const
    {
        if (dumpDisplayOnExitPath_.empty())
        {
            return std::nullopt;
        }
        return dumpDisplayOnExitPath_;
    }

  private:
    const bool dumpSerial_;
    const bool runHeadless_;
    const std::string dumpDisplayOnExitPath_;
};

} // namespace gbemu::config

#endif // GBEMU_CONFIG_CONFIG_H
