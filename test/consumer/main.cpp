#include <DP8382x/CommandTable.h>
#include <DP8382x/Config.h>
#include <DP8382x/Status.h>
#include <DP8382x/Version.h>
#include "../../examples/portable/DirectRead.h"
#include "../../examples/portable/MmdAccess.h"
#include "../../examples/portable/EvidenceAccess.h"
#include "../../examples/portable/IdentityProbe.h"

#ifdef _MSVC_LANG
static_assert(_MSVC_LANG >= 201703L, "DP8382x must propagate C++17");
#else
static_assert(__cplusplus >= 201703L, "DP8382x must propagate C++17");
#endif
static_assert(DP8382x::VERSION_STRING[0] != '\0',
              "committed version header is available");

// A host-only simulated controller, not a physical backend or timing claim.
static uint64_t nowUs(void*) noexcept { return 0; }
struct SimulatedPhy {
  uint16_t led{0};
  uint16_t misr{0xA55A};
};
static DP8382x::BatchResult executeEvidence(void* context, const DP8382x::MdioBatch& batch,
                                          const DP8382x::IoBudget&) noexcept {
  auto& phy = *static_cast<SimulatedPhy*>(context);
  DP8382x::BatchResult result;
  result.releaseStatus = DP8382x::Status::Ok();
  result.controllerReusable = true;
  for (uint8_t i = 0; i < batch.frameCount; ++i) {
    const auto& frame = batch.frames[i];
    auto& receipt = result.frames[i];
    receipt.attempted = true;
    receipt.completed = true;
    if (frame.kind == DP8382x::FrameKind::READ) {
      receipt.valueValid = true;
      receipt.value = frame.registerAddress == 0x12U ? phy.misr : uint16_t{0x0004};
      if (frame.registerAddress == 0x12U) { phy.misr &= 0x00FFU; }
    } else {
      phy.misr = static_cast<uint16_t>((result.frames[frame.readIndex].value & frame.preserveMask) | frame.value);
    }
  }
  result.status = DP8382x::Status::Ok();
  return result;
}
static DP8382x::BatchResult executeMmd(void* context, const DP8382x::MdioBatch& batch,
                                     const DP8382x::IoBudget&) noexcept {
  auto& phy = *static_cast<SimulatedPhy*>(context);
  DP8382x::BatchResult result;
  result.releaseStatus = DP8382x::Status::Ok();
  result.controllerReusable = true;
  if (batch.frameCount != 4U && batch.frameCount != 5U) { return result; }
  if (batch.frames[0].registerAddress != 0x0DU || batch.frames[0].value != 0x001FU ||
      batch.frames[1].registerAddress != 0x0EU ||
      batch.frames[2].registerAddress != 0x0DU || batch.frames[2].value != 0x401FU) { return result; }
  const auto address = batch.frames[1].value;
  if (address != 0x0467U && address != 0x0018U) { return result; }
  for (uint8_t i = 0; i < batch.frameCount; ++i) {
    const auto& frame = batch.frames[i];
    auto& receipt = result.frames[i];
    receipt.attempted = true;
    receipt.completed = true;
    if (frame.kind == DP8382x::FrameKind::READ) {
      receipt.valueValid = true;
      receipt.value = address == 0x0018U ? phy.led : uint16_t{0x0123};
    } else if (i >= 3U && address == 0x0018U) {
      phy.led = frame.readIndex == DP8382x::NO_READ_INDEX ? frame.value :
          static_cast<uint16_t>((result.frames[frame.readIndex].value & frame.preserveMask) | frame.value);
    }
  }
  result.status = DP8382x::Status::Ok();
  return result;
}
static DP8382x::BatchResult execute(void*, const DP8382x::MdioBatch& batch,
                                  const DP8382x::IoBudget&) noexcept {
  DP8382x::BatchResult result;
  result.releaseStatus = DP8382x::Status::Ok();
  result.controllerReusable = true;
  if (batch.phyAddress != 31U || (batch.frameCount != 1U && batch.frameCount != 2U)) { return result; }
  for (uint8_t i = 0; i < batch.frameCount; ++i) {
    if (batch.frames[i].kind != DP8382x::FrameKind::READ ||
        batch.frames[i].registerAddress != 2U + i) { return result; }
    result.frames[i].attempted = true;
    result.frames[i].completed = true;
    result.frames[i].valueValid = true;
    result.frames[i].value = i == 0U ? uint16_t{0x2000} : uint16_t{0xA131};
  }
  result.status = DP8382x::Status::Ok();
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
  config.transport = {nullptr, execute, nowUs, {1, 1, 1}};
  uint16_t word = 0xCAFE;
  const auto read = readIdentityWord(config, {100, 1}, word);
  if (!status.ok() || !read.ok() || word != 0x2000U) { return 1; }
  DP8382x::DeviceIdentity identity;
  DP8382x::Capabilities capabilities;
  const auto probe = probeExactAddress(config, {100, 2}, identity, capabilities);
  if (!probe.ok()) { return 1; }
  if (!identity.decodedValid || identity.mode != DP8382x::OperatingMode::ENHANCED ||
      !capabilities.valid || capabilities.linkModeMask != 0x0FU || capabilities.interfaceMask != 3U) { return 1; }
  SimulatedPhy simulated;
  config.transport = {&simulated, executeMmd, nowUs, {1, 1, 1}};
  DP8382x::DP8382x phy;
  if (!phy.bind(config).ok()) { return 1; }
  uint16_t straps = 0;
  if (!readStrapWord(phy, {100, 4}, straps).ok() || straps != 0x0123U) { return 1; }
  if (!phy.writeMmd(31, 0x0018, 0x0012, {100, 4}).ok()) { return 1; }
  if (!setLedBlink100ms(phy, {100, 5}).ok() || simulated.led != 0x0212U) { return 1; }
  const auto receipt = phy.transportSnapshot();
  if (receipt.totalBatches != 3U || receipt.completedFrames != 13U || !phy.end().ok()) { return 1; }
  config.transport.executeBatch = executeEvidence;
  if (!phy.bind(config).ok()) { return 1; }
  DP8382x::EvidenceSnapshot evidence;
  if (!collectBasicHistory(phy, {100, 2}, evidence).ok()) { return 1; }
  const auto& basic = evidence.samples[static_cast<uint8_t>(DP8382x::EvidenceSource::BMSR)];
  if (!basic.valid || basic.raw != 0x0004U) { return 1; }
  if (!phy.modifyInterruptEnables(0x12, 0x0024, 0x0004, {100, 2}).ok()) { return 1; }
  const auto cached = phy.evidenceSnapshot();
  const auto& interrupt = cached.samples[static_cast<uint8_t>(DP8382x::EvidenceSource::MISR1)];
  return interrupt.valid && interrupt.raw == 0xA55AU && simulated.misr == 0x005EU &&
         cached.configurationDirty && phy.end().ok() ? 0 : 1;
}
