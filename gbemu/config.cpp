#include "config.h"

namespace gbemu::config
{

Config::Config(bool enableBlarggSerialLogging, bool runHeadless)
    : enableBlarggSerialLogging_(enableBlarggSerialLogging), runHeadless_(runHeadless)
{
}

} // namespace gbemu::config