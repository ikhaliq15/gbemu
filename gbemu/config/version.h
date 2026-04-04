#ifndef GBEMU_CONFIG_VERSION_H
#define GBEMU_CONFIG_VERSION_H

#ifndef GBEMU_VERSION
#define GBEMU_VERSION "dev"
#endif

namespace gbemu::config
{

inline constexpr const char kAppName[] = "GBEmu";
inline constexpr const char kVersion[] = GBEMU_VERSION;
inline constexpr const char kDisplayName[] = "GBEmu v" GBEMU_VERSION;

} // namespace gbemu::config

#endif // GBEMU_CONFIG_VERSION_H
