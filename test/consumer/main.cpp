#include <DP8382x/CommandTable.h>
#include <DP8382x/Config.h>
#include <DP8382x/Status.h>
#include <DP8382x/Version.h>
#include "../../examples/portable/DirectRead.h"

#ifdef _MSVC_LANG
static_assert(_MSVC_LANG >= 201703L, "DP8382x must propagate C++17");
#else
static_assert(__cplusplus >= 201703L, "DP8382x must propagate C++17");
#endif
static_assert(DP8382x::VERSION_STRING[0] != '\0',
              "committed version header is available");

// A host-only simulated controller, not a physical backend or timing claim.
static uint64_t nowUs(void*) noexcept { return 0; }
static DP8382x::BatchResult execute(void*, const DP8382x::MdioBatch& batch,
                                  const DP8382x::IoBudget&) noexcept {
  DP8382x::BatchResult result;
  result.releaseStatus = DP8382x::Status::Ok();
  result.controllerReusable = true;
  if (batch.phyAddress != 31U || batch.frameCount != 1U ||
      batch.frames[0].kind != DP8382x::FrameKind::READ ||
      batch.frames[0].registerAddress != 2U) { return result; }
  result.status = DP8382x::Status::Ok();
  result.frames[0].attempted = true;
  result.frames[0].completed = true;
  result.frames[0].valueValid = true;
  result.frames[0].value = 0x2000;
  return result;
}

int main() {
  DP8382x::LinkSettings settings;
  settings.advertisement.technologyMask = DP8382x::LINK_100_FULL;
  const auto status = DP8382x::validateLinkSettings(settings);
  DP8382x::Config config;
  config.variant = DP8382x::Variant::DP83826I;
  config.interface = DP8382x::MacInterface::RMII;
  config.phyAddress = 31;
  config.transport = {nullptr, execute, nowUs, {0, 1, 0, 1, 1}};
  uint16_t word = 0xCAFE;
  const auto read = readIdentityWord(config, {100, 1}, word);
  return status.ok() && read.ok() && word == 0x2000U &&
         DP8382x::cmd::REG_PHYRCR == 0x1FU ? 0 : 1;
}
