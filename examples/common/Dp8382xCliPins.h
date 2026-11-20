/// @file Dp8382xCliPins.h
/// @brief Typed CLI handlers for pins, interface timing, link quality and analog settings.
#pragma once

#include "Dp8382xCliFeatureTypes.h"

#include <cinttypes>

namespace DP8382x::Cli {

/// @brief Internal grammar and result formatting for the pin and link controls.
namespace pin_commands {

/// @brief Split a comma-delimited profile into exactly N nonempty tokens.
/// @param[in] text One CLI token without whitespace.
/// @param[out] parts Borrowed substrings. Ignore them if parsing fails.
/// @return True when the profile has exactly N fields.
template<std::size_t N>
bool split(std::string_view text, std::array<std::string_view, N>& parts) {
  for (std::size_t i = 0; i < N; ++i) {
    const auto comma = text.find(',');
    if ((comma == std::string_view::npos) != (i + 1 == N) || comma == 0 || text.empty()) return false;
    parts[i] = text.substr(0, comma);
    if (comma != std::string_view::npos) text.remove_prefix(comma + 1);
  }
  return true;
}

/// @brief Parse fixed comma-delimited unsigned fields with individual bounds.
/// @param[in] text One CLI token.
/// @param[in] limits Inclusive field maxima in order.
/// @param[out] values Ignore the output if parsing fails.
/// @return True when all fields are present and within range.
template<std::size_t N>
bool numbers(std::string_view text, const std::array<uint32_t, N>& limits, std::array<uint32_t, N>& values) {
  std::array<std::string_view, N> parts{};
  if (!split(text, parts)) return false;
  for (std::size_t i = 0; i < N; ++i) {
    if (!featureNumber(parts[i], limits[i], values[i])) return false;
  }
  return true;
}

/// @brief Decode one pin's requested role, LED source, polarity and drive.
/// @param[in] text Role or LED profile. Source codes are SNLS647I tables 8-49/85.
/// @param[out] pin Typed intent. Core validation checks exact pin/mode applicability.
/// @return True for syntactically valid intent.
inline bool decodePin(std::string_view text, PinSetting& pin) {
  if (text.substr(0, 4) == "led,") {
    std::array<std::string_view, 3> parts{};
    uint32_t source = 0;
    if (!split(text.substr(4), parts) || !featureNumber(parts[0], 14, source)) return false;
    const int polarity = featureChoice(parts[1], "keep|low|high");
    const int drive = featureChoice(parts[2], "normal|low|high");
    if (polarity < 0 || drive < 0) return false;
    pin = {PinFunction::LED, static_cast<LedSource>(source), static_cast<LedPolarity>(polarity),
           static_cast<LedDrive>(drive)};
    return true;
  }
  const int role = featureChoice(text, "keep|wake|irq|col3|col5|col6|high|tx-error|clock");
  constexpr PinFunction roles[]{PinFunction::UNOWNED, PinFunction::WAKE, PinFunction::INTERRUPT,
      PinFunction::COLLISION3, PinFunction::COLLISION5, PinFunction::COLLISION6, PinFunction::HIGH,
      PinFunction::TX_ERROR, PinFunction::CLOCK};
  if (role < 0) return false;
  pin.function = roles[role];
  return true;
}

/// @brief Decode the complete pin, blink and polarity profile.
/// @param[in] args Eight command arguments.
/// @param[out] settings Typed owned settings. Available mask records the pins requested for release.
/// @return True for syntactically valid settings. Does not attest board permissions.
inline bool decodePins(const FeatureArgs& args, PinSettings& settings) {
  for (unsigned i = 0; i < 4; ++i) {
    if (!decodePin(args[i], settings.pins[i])) return false;
    if (settings.pins[i].function != PinFunction::UNOWNED)
      settings.availableMask |= static_cast<uint8_t>(1U << i);
  }
  uint32_t blink = 0;
  settings.blinkOwned = args[4] != "keep";
  if (settings.blinkOwned) {
    if (!featureNumber(args[4], 3, blink)) return false;
    settings.blinkCode = static_cast<uint8_t>(blink);
  }
  const int stretch = featureChoice(args[5], "keep|enabled|bypass");
  const int swap = featureChoice(args[6], "keep|off|on");
  const int clock = featureChoice(args[7], "keep|override");
  if (stretch < 0 || swap < 0 || clock < 0) return false;
  settings.stretchOwned = stretch != 0;
  settings.stretchBypass = stretch == 2;
  settings.led0SwapOwned = swap != 0;
  settings.led0Swap = swap == 2;
  settings.led1ClockOverride = clock == 1;
  return true;
}

/// @brief Decode a complete interface profile, including an optional repeater side.
/// @param[in] args Eight command arguments. Packet length is bytes and tolerance is ppm.
/// @param[out] settings Typed intent. The owner separately attests wiring and clock availability.
/// @return True for valid field syntax. The core checks cross-field and current-owner constraints.
inline bool decodeInterface(const FeatureArgs& args, InterfaceSettings& settings) {
  const int wiring = featureChoice(args[0], "mii|rmii25|rmii50");
  uint32_t modeFlags = 0, elasticity = 0, bytes = 0, ppm = 0, controls = 0;
  const int clock = featureChoice(args[6], "keep|clkout25|led1|pll|recovered");
  const int isolate = featureChoice(args[7], "keep|off|on");
  if (wiring < 0 || clock < 0 || isolate < 0 || !featureNumber(args[1], 15, modeFlags) ||
      !featureNumber(args[2], 3, elasticity) || !featureNumber(args[3], UINT32_MAX, bytes) ||
      !featureNumber(args[4], UINT16_MAX, ppm) || !featureNumber(args[5], 15, controls)) return false;
  settings.interface = wiring == 0 ? MacInterface::MII : MacInterface::RMII;
  settings.rmiiClock = wiring == 2 ? RmiiClock::FOLLOWER_50MHZ : RmiiClock::LEADER_25MHZ;
  settings.referenceClockMHz = wiring == 2 ? 50 : 25;
  settings.txClockShift = (modeFlags & 1U) != 0;
  settings.revision10 = (modeFlags & 2U) != 0;
  settings.rxDv = (modeFlags & 4U) != 0;
  settings.repeater = (modeFlags & 8U) != 0;
  settings.elasticityCode = static_cast<uint8_t>(elasticity);
  settings.maxFrameBytes = bytes;
  settings.clockTolerancePpm = static_cast<uint16_t>(ppm);
  settings.fastRxDv = (controls & 1U) != 0;
  // SNLS647I table 8-29 defines a disable bit. Some TI helpers name this backwards.
  settings.oddNibbleDisable = (controls & 2U) != 0;
  settings.collisionFullDuplex = (controls & 4U) != 0;
  settings.shortIpg = (controls & 8U) != 0;
  settings.clockOutput = static_cast<ClockOutput>(clock);
  settings.isolationOwned = isolate != 0;
  settings.isolate = isolate == 2;
  return true;
}

/// @brief Decode Fast Link Drop, TX100, TX10 and link recovery ownership.
/// @param[in] args Four tokens. Commas delimit fields of an owned group.
/// @param[out] settings Typed intent. Use keep for a group the caller will leave unowned.
/// @return True for legal field syntax. Mode and partner admission remain with the owner/core.
inline bool decodeQuality(const FeatureArgs& args, QualitySettings& settings) {
  if (args[0] != "keep") {
    auto& fld = settings.fastDrop;
    uint32_t causes = 0;
    fld.owned = true;
    if (args[0].find(',') == std::string_view::npos) {
      if (!featureNumber(args[0], 0x040F, causes)) return false;
    } else {
      std::array<uint32_t, 4> values{};
      if (!numbers(args[0], {0x040F, 63, 63, 63}, values) || values[1] == 0 || values[2] == 0) return false;
      causes = values[0];
      fld.rawThresholdOverride = true;
      fld.windowCode = static_cast<uint8_t>(values[1]);
      fld.errorThreshold = static_cast<uint8_t>(values[2]);
      fld.descramblerThreshold = static_cast<uint8_t>(values[3]);
    }
    if ((causes & ~0x040FU) != 0) return false;
    fld.causeMask = static_cast<uint16_t>(causes);
  }
  if (args[1] != "keep") {
    std::array<uint32_t, 3> values{};
    if (!numbers(args[1], {1, 3, 1}, values)) return false;
    settings.tx100 = {true, values[0] != 0, static_cast<uint8_t>(values[1]), values[2] != 0};
  }
  if (args[2] != "keep") {
    std::array<uint32_t, 5> values{};
    if (!numbers(args[2], {1, 8, 1, 1, 1}, values)) return false;
    settings.tx10 = {true, values[0] != 0, static_cast<uint8_t>(values[1]), values[2] != 0,
                     values[3] != 0, values[4] != 0};
  }
  const int recovery = featureChoice(args[3], "keep|off|on");
  if (recovery < 0) return false;
  settings.recoveryOwned = recovery != 0;
  settings.linkRecovery = recovery == 2;
  return true;
}

/// @brief Decode exact-part DAC, termination and I/O slew codes.
/// @param[in] args Five owner profile arguments. Codes have no implied physical calibration.
/// @param[out] settings Typed request, including a source-gated receiver gain when named.
/// @return True for valid field syntax.
inline bool decodeAnalog(const FeatureArgs& args, AnalogSettings& settings) {
  if (args[0] != "keep") {
    std::array<uint32_t, 5> values{};
    if (!numbers(args[0], {0x38, 0x38, 0x18, 0x18, 63}, values) || values[0] < 0x28 ||
        values[1] < 0x28 || values[2] < 8 || values[3] < 8) return false;
    settings.amplitudeOwned = true;
    settings.minusMdi = static_cast<uint8_t>(values[0]);
    settings.minusMdix = static_cast<uint8_t>(values[1]);
    settings.plusMdi = static_cast<uint8_t>(values[2]);
    settings.plusMdix = static_cast<uint8_t>(values[3]);
    settings.zeroCode = static_cast<uint8_t>(values[4]);
  }
  if (args[1] != "keep") {
    std::array<uint32_t, 4> values{};
    if (!numbers(args[1], {15, 15, 1, 1}, values)) return false;
    settings.terminationOwned = true;
    settings.mdiTermination = static_cast<uint8_t>(values[0]);
    settings.mdixTermination = static_cast<uint8_t>(values[1]);
    settings.mdiTerminationEnable = values[2] != 0;
    settings.mdixTerminationEnable = values[3] != 0;
  }
  const int mac = featureChoice(args[2], "keep|fast|slow");
  const int output = featureChoice(args[3], "keep|slow|fast");
  if (mac < 0 || output < 0) return false;
  settings.macSlewOwned = mac != 0;
  settings.macSlewCode = mac == 2 ? 0x11 : 0x10;
  settings.ioSlewOwned = output != 0;
  settings.ioSlewCode = output == 2 ? 1 : 0;
  if (args[4] != "keep") {
    uint32_t gain = 0;
    if (!featureNumber(args[4], 31, gain)) return false;
    settings.receiverGainOwned = true;
    settings.receiverGainCode = static_cast<uint8_t>(gain);
  }
  return true;
}

/// @brief Name an unavailable board prerequisite independently of silicon support.
/// @param[out] out Record with the board gate marked.
/// @return BUSY/OWNER_ADMISSION. No transfer is authorized.
inline Status boardDenied(FeatureFields& out) {
  out.set(" gate=board");
  return featureDenied();
}

/// @brief Format retained LED and GPIO words without asserting electrical behavior.
/// @param[in] s Cached or direct-operation proof.
/// @param[out] out Machine fields with explicit ownership, validity and staleness.
inline void fields(const PinSnapshot& s, FeatureFields& out) {
  out.set(" desired=%u trust=%u stale=%u valid_mask=%u ledcr=%u phycr=%u mledcr=%u ledcfg=%u ledcfg2=%u io_cfg1=%u gpio0=%u gpio1=%u gpio2=%u gpio3=%u",
          unsigned(s.desiredValid), unsigned(s.trust), unsigned(s.observedStale), unsigned(s.validMask),
          unsigned(s.raw[0]), unsigned(s.raw[1]), unsigned(s.raw[2]), unsigned(s.raw[3]), unsigned(s.raw[4]),
          unsigned(s.raw[5]), unsigned(s.raw[6]), unsigned(s.raw[7]), unsigned(s.raw[8]), unsigned(s.raw[9]));
}

/// @brief Format retained interface and compatibility verification.
/// @param[in] s Cached or direct-operation proof.
/// @param[out] out Machine fields with validity independent of mode and clock evidence.
inline void fields(const InterfaceSnapshot& s, FeatureFields& out) {
  out.set(" desired=%u trust=%u stale=%u valid_mask=%u mode_valid=%u cr1=%u cr2=%u physcr=%u gencfg=%u io_cfg1=%u rcsr=%u bmcr=%u clk_led_status=%u",
          unsigned(s.desiredValid), unsigned(s.trust), unsigned(s.observedStale), unsigned(s.validMask),
          unsigned(s.modeValid), unsigned(s.raw[0]), unsigned(s.raw[1]), unsigned(s.raw[2]), unsigned(s.raw[3]),
          unsigned(s.raw[4]), unsigned(s.raw[5]), unsigned(s.raw[6]), unsigned(s.raw[7]));
}

/// @brief Format terminal or cached quality configuration evidence.
/// @param[in] s Partial or complete core snapshot.
/// @param[out] out Raw register words with a validity mask and stable-field trust.
inline void fields(const QualitySnapshot& s, FeatureFields& out) {
  out.set(" desired=%u trust=%u stale=%u valid_mask=%u fld_bound_applicable=%u cr3=%u fld_cfg1=%u fld_cfg2=%u physcr=%u cr2=%u btscr=%u config10=%u cr1=%u",
          unsigned(s.desiredValid), unsigned(s.trust), unsigned(s.observedStale), unsigned(s.validMask),
          unsigned(s.fldMaximumApplicable), unsigned(s.raw[0]), unsigned(s.raw[1]), unsigned(s.raw[2]),
          unsigned(s.raw[3]), unsigned(s.raw[4]), unsigned(s.raw[5]), unsigned(s.raw[6]), unsigned(s.raw[7]));
}

/// @brief Format terminal or cached analog configuration evidence.
/// @param[in] s Partial or complete core snapshot.
/// @param[out] out Raw words with a validity mask and stable-field trust.
inline void fields(const AnalogSnapshot& s, FeatureFields& out) {
  out.set(" desired=%u trust=%u stale=%u valid_mask=%u vod1=%u vod2=%u vod3=%u ioctrl=%u io_cfg1=%u",
          unsigned(s.desiredValid), unsigned(s.trust), unsigned(s.observedStale), unsigned(s.validMask),
          unsigned(s.raw[0]), unsigned(s.raw[1]), unsigned(s.raw[2]), unsigned(s.raw[3]), unsigned(s.raw[4]));
}

/// @brief Format raw MSE and the independent prior link context.
/// @param[in] s Current or cached sample. Invalid samples do not imply zero calibrated error.
/// @param[out] out Raw measurement and provenance fields.
inline void fields(const SignalQualitySnapshot& s, FeatureFields& out) {
  out.set(" valid=%u stale=%u raw_mse=%u observed_valid=%u observed_us=%" PRIu64
          " context_valid=%u context_stale=%u context_up=%u last_status=%s",
          unsigned(s.valid), unsigned(s.stale), unsigned(s.rawMse), unsigned(s.timeValid),
          s.timeValid ? s.observedAtUs : 0, unsigned(s.linkContext.valid), unsigned(s.linkContext.stale),
          unsigned(s.linkContext.linkUp), errorName(s.lastStatus.code));
}

/// @brief Append direct owner effects and budget evidence to the observed snapshot.
/// @param[in] snapshot Retained exact-part snapshot from the terminal call.
/// @param[in] result Status and attributed counts from the same call.
/// @param[out] out Independent backend primary, release and verification fields.
template<class Snapshot, class Result>
void directFields(const Snapshot& snapshot, const Result& result, FeatureFields& out) {
  FeatureFields values{};
  fields(snapshot, values);
  out.set("%s attempted=%" PRIu32 " completed=%" PRIu32 " transactions=%" PRIu32
          " batch_primary=%s batch_detail=%" PRId32 " release_status=%s release_detail=%" PRId32
          " mutation_attempted=%u mutation_possible=%u verified=%u",
          values.text.data(), result.attemptedFrames, result.completedFrames, result.transactions,
          errorName(result.primary.code), result.primary.detail, errorName(result.release.code),
          result.release.detail, unsigned(result.mutationAttempted), unsigned(result.mutationPossible),
          unsigned(result.verified));
  out.valid = out.valid && values.valid;
}

} // namespace pin_commands

/// @brief Parse and apply typed LED/GPIO, interface, quality or analog commands.
/// @param[in,out] io Borrowed serialized owner. It supplies maintenance and board admission separately.
/// @param[in] feature Selected command after arity validation.
/// @param[in] args Borrowed CLI arguments, valid only during the call.
/// @param[out] out Bounded current result or passive snapshot fields.
/// @return Terminal status, or IN_PROGRESS with the owner's retained token for a cooperative profile.
template<class Access>
Status pinFeature(Access& io, Feature feature, const FeatureArgs& args, FeatureFields& out) {
  using namespace pin_commands;
  switch (feature) {
    case Feature::PINS: {
      PinSettings settings{};
      if (!decodePins(args, settings)) return featureInvalid();
      // Attest requested pin releases plus output quiescence and shared CLKOUT/COL consumers.
      const bool led1 = (settings.availableMask & 2U) != 0;
      const auto role2 = settings.pins[2].function;
      const bool col = role2 != PinFunction::UNOWNED && role2 != PinFunction::COLLISION3 &&
          role2 != PinFunction::COLLISION5 && role2 != PinFunction::COLLISION6;
      const uint32_t admission = settings.availableMask | (led1 ? 0x10U : 0U) | (col ? 0x20U : 0U) | 0x40U;
      if (!io.attest(admission)) return boardDenied(out);
      settings.maintenanceAdmitted = settings.macStopped = settings.outputsQuiesced = true;
      settings.led1ClockUnused = led1;
      settings.colUnused = col;
      PinResult result{};
      const auto status = io.template direct<&Core::configurePins>(settings, result);
      if (result.status.code != Err::RESULT_NOT_AVAILABLE) directFields(result.pins, result, out);
      return status;
    }
    case Feature::PINS_STATUS: {
      PinSnapshot snapshot{};
      const auto status = io.template cached<&Core::pinsSnapshot>(snapshot);
      if (status.ok()) fields(snapshot, out);
      return status;
    }
    case Feature::INTERFACE: {
      InterfaceSettings settings{};
      if (!decodeInterface(args, settings)) return featureInvalid();
      // Bits 0..3 attest pins, reference, SMI and clock changes. Bit 4 adds repeater wiring.
      const uint32_t admission = 0x0FU | (settings.repeater ? 0x10U : 0U) |
          (static_cast<uint32_t>(settings.referenceClockMHz) << 8U);
      if (!io.attest(admission)) return boardDenied(out);
      settings.maintenanceAdmitted = settings.macStopped = settings.pinsAvailable = true;
      settings.referenceClockStable = settings.managementClockAvailable = settings.clockChangeApproved = true;
      settings.repeaterWiringAdmitted = settings.repeater;
      InterfaceResult result{};
      const auto status = io.template direct<&Core::configureInterface>(settings, result);
      if (result.status.code != Err::RESULT_NOT_AVAILABLE) directFields(result.interfaceState, result, out);
      return status;
    }
    case Feature::INTERFACE_STATUS: {
      InterfaceSnapshot snapshot{};
      const auto status = io.template cached<&Core::interfaceSnapshot>(snapshot);
      if (status.ok()) fields(snapshot, out);
      return status;
    }
    case Feature::QUALITY: {
      QualitySettings settings{};
      if (!decodeQuality(args, settings)) return featureInvalid();
      if (!io.attest(settings.tx10.owned ? 1U : 0U)) return boardDenied(out);
      settings.maintenanceAdmitted = settings.macStopped = true;
      settings.partnerPreambleAdmitted = settings.tx10.owned;
      return io.template start<&Core::startConfigureQuality>(settings);
    }
    case Feature::QUALITY_STATUS: {
      QualitySnapshot snapshot{};
      const auto status = io.template cached<&Core::qualitySnapshot>(snapshot);
      if (status.ok()) fields(snapshot, out);
      return status;
    }
    case Feature::MSE:
    case Feature::MSE_STATUS: {
      SignalQualitySnapshot snapshot{};
      const auto status = feature == Feature::MSE ? io.template read<&Core::readSignalQuality>(snapshot)
                                                 : io.template cached<&Core::signalQualitySnapshot>(snapshot);
      if (status.ok()) fields(snapshot, out);
      return status;
    }
    case Feature::ANALOG: {
      AnalogSettings settings{};
      if (!decodeAnalog(args, settings)) return featureInvalid();
      if (!io.attest(1U | (settings.terminationOwned ? 2U : 0U))) return boardDenied(out);
      settings.maintenanceAdmitted = settings.macStopped = settings.fixtureAdmitted = true;
      settings.terminationOverrideAdmitted = settings.terminationOwned;
      return io.template start<&Core::startConfigureAnalog>(settings);
    }
    case Feature::ANALOG_STATUS: {
      AnalogSnapshot snapshot{};
      const auto status = io.template cached<&Core::analogSnapshot>(snapshot);
      if (status.ok()) fields(snapshot, out);
      return status;
    }
    default: return Status::Error(Err::UNSUPPORTED, "Unknown pin/interface/quality feature");
  }
}

/// @brief Format retained cooperative profile proof after the core token is consumed.
/// @param[in] feature Original command, for correlation with its terminal row.
/// @param[in] result Frozen core result, including valid partial observations.
/// @param[out] out Result fields. Other feature families leave it untouched.
inline void pinResult(Feature feature, const OperationResult& result, FeatureFields& out) {
  if (feature == Feature::QUALITY) pin_commands::fields(result.quality, out);
  if (feature == Feature::ANALOG) pin_commands::fields(result.analog, out);
}

} // namespace DP8382x::Cli
