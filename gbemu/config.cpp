#include "gbemu/config.h"

namespace gbemu::config
{

Config::Config(bool enableBlarggSerialLogging, bool runHeadless, const std::string &dumpDisplayOnExitPath)
    : enableBlarggSerialLogging_(enableBlarggSerialLogging), runHeadless_(runHeadless),
      dumpDisplayOnExitPath_(dumpDisplayOnExitPath)
{
}

} // namespace gbemu::config