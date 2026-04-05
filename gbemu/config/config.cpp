#include "gbemu/config/config.h"

#include <utility>

namespace gbemu::config
{

Config::Config(bool enableBlarggSerialLogging, bool runHeadless, std::string dumpDisplayOnExitPath)
    : enableBlarggSerialLogging_(enableBlarggSerialLogging), runHeadless_(runHeadless),
      dumpDisplayOnExitPath_(std::move(dumpDisplayOnExitPath))
{}

} // namespace gbemu::config
