#include "gbemu/config/config.h"

#include <utility>

namespace gbemu::config
{

Config::Config(bool dumpSerial, bool runHeadless, std::string dumpDisplayOnExitPath)
    : dumpSerial_(dumpSerial), runHeadless_(runHeadless), dumpDisplayOnExitPath_(std::move(dumpDisplayOnExitPath))
{}

} // namespace gbemu::config
