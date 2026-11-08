#include <DP8382x/CommandTable.h>
#include <DP8382x/Config.h>
#include <DP8382x/Status.h>
#include <DP8382x/Version.h>
#include "../../examples/portable/DirectRead.h"
#include "../../examples/portable/MmdAccess.h"
#include "../../examples/portable/EvidenceAccess.h"
#include "../../examples/portable/IdentityProbe.h"
#include "../../examples/portable/Initialize.h"
#include "../../examples/portable/Configure.h"
#include "../../examples/portable/Forced.h"

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

// Independent host model for the public owner-loop example. No physical bus,
// supply readiness, reset duration, electrical behavior or SDK support is proved.
struct InitializationController {
  uint64_t timeUs{0};
  uint16_t selector{0}, address{0}, vod{0x8400};
  uint16_t anar{0x01E1}, bmcr{0x3100};
  uint16_t cr1{0}, phycr{0x8000}, cr3{0}, cr2{0};
  bool failVodWrite{false};
  bool failHardWrite{false};
  uint32_t resetWrites{0}, hardWrites{0}, restartWrites{0}, vodWrites{0}, frames{0};
  uint64_t hardwareQuietUntilUs{0};
  bool quietViolated{false};

  static uint64_t now(void* context) noexcept {
    return static_cast<InitializationController*>(context)->timeUs;
  }
  static DP8382x::BatchResult execute(void* context, const DP8382x::MdioBatch& batch,
                                     const DP8382x::IoBudget&) noexcept {
    auto& controller = *static_cast<InitializationController*>(context);
    DP8382x::BatchResult result;
    result.releaseStatus = DP8382x::Status::Ok();
    result.controllerReusable = true;
    if (controller.timeUs < controller.hardwareQuietUntilUs) controller.quietViolated = true;
    for (uint8_t i = 0; i < batch.frameCount; ++i) {
      const auto& frame = batch.frames[i];
      const uint16_t written = frame.readIndex == DP8382x::NO_READ_INDEX ? frame.value :
          static_cast<uint16_t>((result.frames[frame.readIndex].value & frame.preserveMask) | frame.value);
      auto& receipt = result.frames[i];
      receipt.attempted = true;
      receipt.completed = true;
      ++controller.frames;
      if (frame.kind == DP8382x::FrameKind::READ) {
        receipt.valueValid = true;
        if (frame.registerAddress == 2) { receipt.value = 0x2000; }
        else if (frame.registerAddress == 3) { receipt.value = 0xA131; }
        else if (frame.registerAddress == 0) { receipt.value = controller.bmcr; }
        else if (frame.registerAddress == 4) { receipt.value = controller.anar; }
        else if (frame.registerAddress == 9) { receipt.value = controller.cr1; }
        else if (frame.registerAddress == 25) { receipt.value = controller.phycr; }
        else if (frame.registerAddress == 11) { receipt.value = controller.cr3; }
        else if (frame.registerAddress == 10) { receipt.value = controller.cr2; }
        else if (frame.registerAddress == 0x1F) { receipt.value = 0; }
        else if (frame.registerAddress == 0x0E && controller.selector == 0x401F) {
          if (controller.address == 0x030E) { receipt.value = controller.vod; }
          else if (controller.address == 0x0467) { receipt.value = 0x0123; }
          else if (controller.address == 0x0468) { receipt.value = 0x0456; }
        }
      } else if (frame.registerAddress == 0 && frame.value == 0x8000) {
        ++controller.resetWrites; // This model is complete by the next scheduled read.
        controller.bmcr = 0x3100;
      } else if (frame.registerAddress == 0) {
        controller.bmcr = static_cast<uint16_t>(written & 0xFDFFU);
      } else if (frame.registerAddress == 4) {
        controller.anar = written;
      } else if (frame.registerAddress == 9) { controller.cr1 = written;
      } else if (frame.registerAddress == 25) { controller.phycr = written;
      } else if (frame.registerAddress == 11) { controller.cr3 = written;
      } else if (frame.registerAddress == 10) { controller.cr2 = written;
      } else if (frame.registerAddress == 0x1F && frame.value == 0x8000) {
        ++controller.hardWrites;
        controller.vod = 0x8400;
        controller.hardwareQuietUntilUs = controller.timeUs + 2001; // Post-sample lag plus 2 ms.
        if (controller.failHardWrite) {
          receipt.effectPossible = true;
          result.status = DP8382x::Status::Error(DP8382x::Err::MDIO_TIMEOUT, "Simulated accepted hard reset", -16);
          return result;
        }
      } else if (frame.registerAddress == 0x1F && frame.value == 0x4000) {
        ++controller.restartWrites;
      } else if (frame.registerAddress == 0x0D) { controller.selector = frame.value; }
      else if (frame.registerAddress == 0x0E && controller.selector == 0x001F) {
        controller.address = frame.value;
      } else if (frame.registerAddress == 0x0E && controller.selector == 0x401F && controller.address == 0x030E) {
        controller.vod = frame.value;
        ++controller.vodWrites;
        if (controller.failVodWrite) {
          receipt.effectPossible = true; // Accepted target write, followed by an error.
          result.status = DP8382x::Status::Error(DP8382x::Err::MDIO_TIMEOUT,
                                                "Simulated accepted write failure", -15);
          return result;
        }
      }
    }
    result.status = DP8382x::Status::Ok();
    return result;
  }
};

// Finite host scheduler only. A real application's event loop calls the same
// single-poll helper and arranges its next turn through its own timer/scheduler.
static DP8382x::Status runInitialization(DP8382x::DP8382x& phy,
                                         InitializationController& controller,
                                         SharedMdioQuiet& sharedQuiet,
                                         InitializeAttempt& attempt) noexcept {
  for (unsigned turn = 0; turn < 64; ++turn) {
    if (!sharedQuiet.canIssue(controller.timeUs)) {
      if (sharedQuiet.unanchored) return DP8382x::Status::Error(DP8382x::Err::INVALID_CONFIG, "Owner timing recovery required");
      controller.timeUs = sharedQuiet.untilUs; // Host scheduling, not a production delay loop.
    }
    const auto status = pollInitialization(phy, attempt, sharedQuiet);
    if (!status.is(DP8382x::Err::IN_PROGRESS)) { return status; }
    const auto snapshot = phy.operationSnapshot();
    ++controller.timeUs;
    if (snapshot.nextPollValid && snapshot.nextPollAtUs > controller.timeUs) {
      controller.timeUs = snapshot.nextPollAtUs;
    }
  }
  return DP8382x::Status::Error(DP8382x::Err::OPERATION_TIMEOUT, "Host scheduling limit");
}

static bool initializationExamples() noexcept {
  InitializationController controller;
  SharedMdioQuiet sharedQuiet;
  DP8382x::Config config;
  config.variant = DP8382x::Variant::DP83826I;
  config.interface = DP8382x::MacInterface::RMII;
  config.phyAddress = 1;
  config.transport = {&controller, InitializationController::execute, InitializationController::now, {1, 1, 1}};
  DP8382x::DP8382x phy;
  if (!phy.bind(config).ok()) { return false; }
  for (const auto reset : {DP8382x::ResetKind::NONE, DP8382x::ResetKind::PCS,
                           DP8382x::ResetKind::HARD, DP8382x::ResetKind::RESTART}) {
    InitializeAttempt attempt;
    const DP8382x::OperationBudget budget{{controller.timeUs + 100000, 64}, 1000, 20};
    if (!beginInitialization(phy, reset, budget, attempt).is(DP8382x::Err::IN_PROGRESS) ||
        !runInitialization(phy, controller, sharedQuiet, attempt).ok()) { return false; }
    if (!attempt.resultAvailable || !attempt.result.persistentRestored ||
        attempt.result.commandRequired != (reset != DP8382x::ResetKind::NONE) ||
        !phy.isInitialized() || controller.vod != 0x4A40) { return false; }
    const auto frames = controller.frames;
    if (!pollInitialization(phy, attempt, sharedQuiet).ok() || controller.frames != frames) { return false; }
  }
  if (controller.resetWrites != 1 || controller.hardWrites != 1 || controller.restartWrites != 1 ||
      controller.vodWrites != 4 || controller.frames != 116 || controller.quietViolated) { return false; }

  controller.failVodWrite = true;
  InitializeAttempt primary;
  auto budget = DP8382x::OperationBudget{{controller.timeUs + 100000, 64}, 1000, 20};
  if (!beginInitialization(phy, DP8382x::ResetKind::NONE, budget, primary).is(DP8382x::Err::IN_PROGRESS) ||
      !runInitialization(phy, controller, sharedQuiet, primary).is(DP8382x::Err::MDIO_TIMEOUT)) { return false; }
  if (!primary.result.cleanupOwed || !primary.result.mutationCompleted || phy.end().ok()) { return false; }

  // Restoration is a NEW operation with its own budget/token/result. Neither
  // this failed attempt nor later abandonment overwrites the primary failure.
  InitializeAttempt restoration;
  budget.io.deadlineUs = controller.timeUs + 100000;
  if (!beginInitialization(phy, DP8382x::ResetKind::NONE, budget, restoration).is(DP8382x::Err::IN_PROGRESS) ||
      !runInitialization(phy, controller, sharedQuiet, restoration).is(DP8382x::Err::MDIO_TIMEOUT)) { return false; }
  if (!restoration.result.cleanupOwed || restoration.token.value == primary.token.value ||
      abandonInitialization(phy, false).ok() || !phy.operationSnapshot().cleanupOwed) { return false; }
  // The host owner has closed simulated admission; no real fixture is operated.
  const auto frames = controller.frames;
  return abandonInitialization(phy, true).ok() && !phy.isBound() &&
         controller.frames == frames && primary.result.status.detail == -15 &&
         restoration.result.status.detail == -15 && primary.result.cleanupOwed &&
         restoration.result.cleanupOwed;
}

static bool hardQuietOwnerExamples() noexcept {
  for (const bool failCommand : {false, true}) {
    InitializationController controller;
    controller.failHardWrite = failCommand;
    SharedMdioQuiet sharedQuiet;
    DP8382x::Config config;
    config.variant = DP8382x::Variant::DP83826I;
    config.interface = DP8382x::MacInterface::RMII;
    config.phyAddress = 1;
    config.transport = {&controller, InitializationController::execute, InitializationController::now, {1, 1, 1}};
    DP8382x::DP8382x phy, otherPhy;
    if (!phy.bind(config).ok()) return false;
    config.phyAddress = 2;
    if (!otherPhy.bind(config).ok()) return false;
    InitializeAttempt attempt;
    if (!beginInitialization(phy, DP8382x::ResetKind::HARD,
                             {{100000, 64}, 1000, 20}, attempt).inProgress()) return false;
    // Finite host turns stop immediately after the hard command. The real
    // owner excludes other participants from before this poll until the floor.
    for (unsigned turn = 0; turn < 16 && controller.hardWrites == 0; ++turn) {
      const auto status = pollInitialization(phy, attempt, sharedQuiet);
      if (!status.inProgress() && !status.is(DP8382x::Err::MDIO_TIMEOUT)) return false;
    }
    if (controller.hardWrites != 1 || sharedQuiet.canIssue(controller.timeUs) || sharedQuiet.unanchored) return false;
    const auto floor = sharedQuiet.untilUs;
    const auto frames = controller.frames;
    if (!failCommand) {
      if (!phy.cancel(attempt.token).ok() ||
          !pollInitialization(phy, attempt, sharedQuiet).is(DP8382x::Err::CANCELLED)) return false;
    }
    if (!attempt.resultAvailable || !attempt.result.cleanupOwed || !phy.operationSnapshot().busQuietValid) return false;
    if (!abandonInitialization(phy, true).ok() || phy.isBound() || sharedQuiet.untilUs != floor) return false;
    // Both bindings share this admission decision. Teardown cannot erase the
    // owner-held floor, and the second PHY must not issue an early frame.
    uint16_t word = 0xCAFE;
    if (sharedQuiet.canIssue(floor - 1)) return false;
    if (controller.frames != frames) return false;
    controller.timeUs = floor;
    if (!sharedQuiet.canIssue(controller.timeUs) ||
        !otherPhy.readClause22(2, word, {100000, 1}).ok() || word != 0x2000 || controller.quietViolated) return false;
    if (!otherPhy.end().ok()) return false;
  }
  return true;
}

static bool configurationExamples() noexcept {
  InitializationController controller;
  SharedMdioQuiet quiet;
  DP8382x::Config config;
  config.variant = DP8382x::Variant::DP83826I;
  config.interface = DP8382x::MacInterface::RMII;
  config.phyAddress = 1;
  config.transport = {&controller, InitializationController::execute, InitializationController::now, {1, 1, 1}};
  DP8382x::DP8382x phy;
  InitializeAttempt initialization;
  if (!phy.bind(config).ok() ||
      !beginInitialization(phy, DP8382x::ResetKind::NONE, {{100000, 64}, 1000, 20}, initialization).inProgress() ||
      !runInitialization(phy, controller, quiet, initialization).ok()) return false;
  for (const auto& profile : {allNegotiatedTechnologies(), application100Full()}) {
    ConfigureAttempt attempt;
    if (!beginConfiguration(phy, profile, {}, {{100000, 64}, 1000, 20}, attempt).inProgress()) return false;
    for (unsigned turn = 0; turn < 32 && !attempt.resultAvailable; ++turn) {
      const auto status = pollConfiguration(phy, attempt);
      if (!status.inProgress() && !status.ok()) return false;
      ++controller.timeUs;
      const auto snapshot = phy.operationSnapshot();
      if (snapshot.nextPollValid && snapshot.nextPollAtUs > controller.timeUs) controller.timeUs = snapshot.nextPollAtUs;
    }
    if (!attempt.resultAvailable || !attempt.result.status.ok() ||
        phy.settingsSnapshot().trust != DP8382x::ConfigurationTrust::VERIFIED || controller.vodWrites != 1) return false;
  }
  if (controller.anar != 0x0101 || controller.bmcr != 0x1000) return false;
  ConfigureAttempt forced;
  if (!beginForced100Full(phy, {{100000, 64}, 1000, 20}, forced).inProgress()) return false;
  for (unsigned turn = 0; turn < 32 && !forced.resultAvailable; ++turn) {
    const auto status = pollConfiguration(phy, forced);
    if (!status.inProgress() && !status.ok()) return false;
    ++controller.timeUs;
  }
  return forced.resultAvailable && forced.result.status.ok() &&
      controller.anar == 0x0101 && controller.bmcr == 0x2100 &&
      controller.cr1 == 0x0020 && controller.phycr == 0 && controller.vodWrites == 1 && phy.end().ok();
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
         cached.configurationDirty && phy.end().ok() && initializationExamples() && hardQuietOwnerExamples() && configurationExamples() ? 0 : 1;
}
