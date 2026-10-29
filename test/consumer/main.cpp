#include <DP8382x/CommandTable.h>
#include <DP8382x/Config.h>
#include <DP8382x/Status.h>
#include <DP8382x/Version.h>

#ifdef _MSVC_LANG
static_assert(_MSVC_LANG >= 201703L, "DP8382x must propagate C++17");
#else
static_assert(__cplusplus >= 201703L, "DP8382x must propagate C++17");
#endif
static_assert(DP8382x::VERSION_STRING[0] != '\0',
              "committed version header is available");

int main() {
  DP8382x::LinkSettings settings;
  settings.advertisement.technologyMask = DP8382x::LINK_100_FULL;
  const auto status = DP8382x::validateLinkSettings(settings);
  return status.ok() && DP8382x::isValidPhyAddress(31U) &&
         DP8382x::cmd::REG_PHYRCR == 0x1FU ? 0 : 1;
}
