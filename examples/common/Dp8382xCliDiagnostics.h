/// @file Dp8382xCliDiagnostics.h
/// @brief Map diagnostic tokens and evidence to the shared owner routes.
#pragma once

#include "Dp8382xCliFeatureTypes.h"
#include <cinttypes>
#include <cstdio>

namespace DP8382x::Cli {
/// @cond INTERNAL
namespace diagnostic_detail {

constexpr uint32_t LINE_TRANSMIT = 1U;
constexpr uint32_t TERMINATED = 2U;
constexpr uint32_t PEER_READY = 4U;
constexpr uint32_t EXTERNAL_PATH = 8U;
constexpr uint32_t HARD_EXIT = 16U;

inline LinkSpeed speed(std::string_view token) noexcept {
  if (token == "10") return LinkSpeed::MBPS_10;
  if (token == "100") return LinkSpeed::MBPS_100;
  return LinkSpeed::UNKNOWN;
}

inline unsigned mbps(LinkSpeed value) noexcept {
  return value == LinkSpeed::MBPS_10 ? 10U : value == LinkSpeed::MBPS_100 ? 100U : 0U;
}

// Required fixture declarations depend on the selected path; irrelevant
// declarations are rejected so a stale option cannot authorize a new mode.
inline bool loopOptions(LoopbackMode mode, LinkSpeed selectedSpeed, uint32_t flags,
                        uint32_t fifo, LoopbackOptions& options) noexcept {
  if (selectedSpeed == LinkSpeed::UNKNOWN || (mode != LoopbackMode::REVERSE && fifo != 0U)) return false;
  switch (mode) {
    case LoopbackMode::MII:
      if (flags != 0U && (flags != LINE_TRANSMIT || selectedSpeed != LinkSpeed::MBPS_100)) return false;
      break;
    case LoopbackMode::PCS_INPUT:
      if (flags != 0U || selectedSpeed != LinkSpeed::MBPS_10) return false;
      break;
    case LoopbackMode::PCS_OUTPUT:
      if (flags != 0U) return false;
      break;
    case LoopbackMode::DIGITAL:
      if (flags != HARD_EXIT || selectedSpeed != LinkSpeed::MBPS_100) return false;
      break;
    case LoopbackMode::ANALOG:
      if (flags != TERMINATED) return false;
      break;
    case LoopbackMode::REVERSE:
      if (flags != PEER_READY) return false;
      break;
    case LoopbackMode::EXTERNAL:
      if (flags != EXTERNAL_PATH) return false;
      break;
  }
  options = {mode, selectedSpeed, flags == LINE_TRANSMIT, true, flags == TERMINATED,
             flags == PEER_READY, flags == EXTERNAL_PATH, flags == HARD_EXIT, static_cast<uint8_t>(fifo)};
  return true;
}

inline const char* loopMode(LoopbackMode mode) noexcept {
  constexpr const char* names[] = {"mii", "pcs_in", "pcs_out", "digital", "analog", "reverse", "external"};
  const auto index = static_cast<unsigned>(mode);
  return index < sizeof(names) / sizeof(names[0]) ? names[index] : "unknown";
}

inline void bistFields(const BistResult& result, FeatureFields& out, bool stale,
                       const BistSnapshot* snapshot = nullptr) {
  constexpr const char* outcomes[] = {"inconclusive", "errors_observed", "single_packet_observed", "disabled"};
  const auto outcome = static_cast<unsigned>(result.outcome);
  out.set(" session_op=%" PRIu64 " mode=%u outcome=%s active_valid=%u active=%u stale=%u"
          " before_valid=%u before=%u counter_valid=%u counter=%u after_valid=%u after=%u"
          " measurement_valid=%u activity=%u lock=%u sync_loss=%u errors=%u saturated=%u"
          " wrap_possible=%u interval_uncertain=%u history_lost=%u",
          result.session.value, unsigned(result.mode), outcome < 4 ? outcomes[outcome] : "unknown",
          unsigned(snapshot != nullptr), unsigned(snapshot && snapshot->active), unsigned(stale),
          unsigned(result.beforeValid), unsigned(result.rawBefore),
          unsigned(result.counterValid), unsigned(result.rawCounter), unsigned(result.afterValid), unsigned(result.rawAfter),
          unsigned(result.measurementValid), unsigned(result.activityObserved), unsigned(result.lockObserved),
          unsigned(result.syncLossObserved), unsigned(result.errorBytes), unsigned(result.saturated),
          unsigned(result.wrapPossible), unsigned(result.intervalUncertain), unsigned(result.historyLost));
}

inline void cableFields(const CableResult& result, FeatureFields& out) {
  // Six fixed rows carry raw words and validity without exceeding the record allowance.
  constexpr std::size_t ROW_BYTES = 25; // Five groups of four hex digits, four colons and one terminator.
  char rows[6 * ROW_BYTES]{};
  for (std::size_t i = 0; i < result.samples.size(); ++i) {
    const auto& row = result.samples[i];
    const auto flags = static_cast<uint16_t>(unsigned(row.cdcr.valid) | (unsigned(row.index.valid) << 1U) |
        (unsigned(row.amplitude.valid) << 2U) | (unsigned(row.peak.valid) << 3U) |
        (unsigned(row.cdcr.stale) << 4U) | (unsigned(row.index.stale) << 5U) |
        (unsigned(row.amplitude.stale) << 6U) | (unsigned(row.peak.stale) << 7U) |
        (unsigned(row.triggerAccepted) << 8U) | (unsigned(row.triggerPossible) << 9U) |
        (unsigned(row.freshnessObserved) << 10U) | (unsigned(row.failed) << 11U) |
        (unsigned(row.state) << 12U));
    std::snprintf(rows + i * ROW_BYTES, ROW_BYTES, "%04X:%04X:%04X:%04X:%04X", unsigned(flags),
                  unsigned(row.cdcr.raw), unsigned(row.index.raw), unsigned(row.amplitude.raw), unsigned(row.peak.raw));
  }
  out.set(" session_op=%" PRIu64 " recipe=%s requested_mask=%u complete=%u stale=%u stopped=%u autorun=%u"
          " status_changed=%u prerequisite_valid=%u prerequisite=%u r1a=%s r1b=%s r2=%s r3=%s r4=%s r5=%s",
          result.session.value, result.recipe == CableRecipe::RAW ? "raw" : "snla428",
          unsigned(result.requestedMask), unsigned(result.complete), unsigned(result.stale), unsigned(result.stopped),
          unsigned(result.autorun), unsigned(result.statusChanged), unsigned(result.prerequisiteCdcr.valid),
          unsigned(result.prerequisiteCdcr.raw), rows, rows + ROW_BYTES, rows + 2U * ROW_BYTES, rows + 3U * ROW_BYTES,
          rows + 4U * ROW_BYTES, rows + 5U * ROW_BYTES);
}

inline void autorunFields(const CableAutorunSnapshot& policy, FeatureFields& out) {
  out.set(" desired_valid=%u desired=%u observed_valid=%u observed=%u stale=%u trust=%u",
          unsigned(policy.desiredValid), unsigned(policy.desired), unsigned(policy.observedValid),
          unsigned(policy.observed), unsigned(policy.observedStale), unsigned(policy.trust));
}

inline void electricalFields(const ElectricalSnapshot& state, FeatureFields& out) {
  out.set(" compliance=%u collision=%u controls_verified=%u stopped=%u stimulus_possible=%u"
          " mode=%u pair=%u pattern=%u collision_mbps=%u cdcr_valid=%u cdcr=%u recovery_needed=%u hard_owed=%u"
          " electrical_reset_complete=%u",
          unsigned(state.complianceSession), unsigned(state.collisionSession), unsigned(state.controlsVerified),
          unsigned(state.stopped), unsigned(state.temporaryMayBeActive), unsigned(state.compliance.mode),
          unsigned(state.compliance.pair), unsigned(state.compliance.pattern), mbps(state.collision.speed),
          unsigned(state.cdcrValid), unsigned(state.rawCdcr), unsigned(state.recoveryNeeded),
          unsigned(state.hardResetOwed), unsigned(state.resetComplete));
}

template<class Access>
Status startBist(Access& io, const FeatureArgs& args) {
  const int mode = featureChoice(args[0], "off|single|checker|both");
  const int counter = featureChoice(args[1], "saturate|wrap");
  const int path = args.size() > 6U ? featureChoice(args[6], "line|digital|analog|external") : 0;
  uint32_t bytes = 0, gap = 0, flags = 0;
  uint64_t observation = 0;
  if (mode < 0 || counter < 0 || path < 0 || !featureNumber(args[2], 2047, bytes) ||
      !featureNumber(args[3], 1020, gap) || (gap % 4U) != 0U ||
      !featureNumber(args[4], observation) || observation == 0U ||
      (args.size() > 7U && !featureNumber(args[7], 31, flags))) return featureInvalid();
  const auto selectedSpeed = speed(args[5]);
  if (selectedSpeed == LinkSpeed::UNKNOWN) return featureInvalid();
  BistOptions options;
  options.mode = static_cast<BistMode>(mode);
  options.counterMode = static_cast<BistCounterMode>(counter);
  options.packetBytes = static_cast<uint16_t>(bytes);
  options.interPacketGapBytes = static_cast<uint16_t>(gap);
  options.observationUs = observation;
  options.useLoopback = path != 0;
  if (options.useLoopback) {
    constexpr LoopbackMode paths[] = {LoopbackMode::EXTERNAL, LoopbackMode::DIGITAL, LoopbackMode::ANALOG,
                                      LoopbackMode::EXTERNAL};
    if (!loopOptions(paths[path], selectedSpeed, flags, 0, options.loopback)) return featureInvalid();
  } else {
    if (flags != 0U) return featureInvalid();
    options.loopback.speed = selectedSpeed;
    options.loopback.maintenanceAdmitted = true;
  }
  if (!io.attest(flags)) return featureDenied();
  return io.template start<&Core::startBist>(options);
}

template<class Access>
Status startCable(Access& io, Feature feature, const FeatureArgs& args) {
  CableSettings settings;
  uint32_t flags = 0;
  if (feature == Feature::CABLE_RAW) {
    const int channel = featureChoice(args[0], "ab|ba|aa|bb");
    uint32_t dc = 0, avg = 0, segment = 0, duration = 0, shadow = 0, thresholds = 0;
    if (channel < 0 || !featureNumber(args[1], 1, dc) || !featureNumber(args[2], 6, avg) ||
        !featureNumber(args[3], 5, segment) || segment == 0 || !featureNumber(args[4], 65535, duration) ||
        !featureNumber(args[5], 0x7F, shadow) || !featureNumber(args[6], 0x1FFF, thresholds) ||
        !featureNumber(args[7], 3, flags)) return featureInvalid();
    settings.recipe = CableRecipe::RAW;
    settings.runMask = 1;
    settings.controls = {static_cast<CableChannel>(channel), dc != 0, static_cast<uint8_t>(avg),
                         static_cast<uint8_t>(segment), static_cast<uint8_t>(duration >> 8U),
                         static_cast<uint8_t>(duration), static_cast<uint8_t>(shadow >> 4U),
                         static_cast<uint8_t>(shadow & 15U), static_cast<uint8_t>(thresholds >> 8U),
                         static_cast<uint8_t>((thresholds >> 3U) & 31U), static_cast<uint8_t>(thresholds & 7U)};
  } else {
    uint32_t mask = 0;
    if (!featureNumber(args[0], 0x3F, mask) || mask == 0 || !featureNumber(args[1], 3, flags))
      return featureInvalid();
    settings.runMask = static_cast<uint8_t>(mask);
  }
  if (flags != 3) return featureInvalid();
  if (!io.attest(flags)) return featureDenied();
  settings.maintenanceAdmitted = settings.peerQuiet = settings.allowHardResetExit = true;
  return io.template start<&Core::startCableTest>(settings);
}

} // namespace diagnostic_detail
/// @endcond

/// @brief Validate one diagnostic command and route it through the borrowed owner.
/// @param[in,out] io Serialized access, original budget and application fixture policy.
/// @param[in] feature Typed command selected by the shared parser.
/// @param[in] args Parsed tokens with the table's checked arity.
/// @param[out] out Bounded evidence from one successful cached query, if applicable.
/// @return Owner status, validation error or unsupported selector. A started session
/// requires its own later stop and independent restore result.
template<class Access>
Status diagnosticFeature(Access& io, Feature feature, const FeatureArgs& args, FeatureFields& out) {
  using namespace diagnostic_detail;
  switch (feature) {
    case Feature::LOOPBACK_START: {
      const int mode = featureChoice(args[0], "mii|pcs-in|pcs-out|digital|analog|reverse|external");
      uint32_t flags = 0, fifo = 0;
      if (mode < 0 || (args.size() > 2U && !featureNumber(args[2], 31, flags)) ||
          (args.size() > 3U && !featureNumber(args[3], 3, fifo))) return featureInvalid();
      LoopbackOptions options;
      if (!loopOptions(static_cast<LoopbackMode>(mode), speed(args[1]), flags, fifo, options)) return featureInvalid();
      if (!io.attest(flags)) return featureDenied();
      return io.template start<&Core::startLoopback>(options);
    }
    case Feature::LOOPBACK_STOP: return io.template start<&Core::stopLoopback>();
    case Feature::LOOPBACK_STATUS: {
      LoopbackSnapshot state;
      const auto status = io.template cached<&Core::loopbackSnapshot>(state);
      if (status.ok()) out.set(" session_op=%" PRIu64 " mode=%s speed_mbps=%u controls_verified=%u stimulus_possible=%u",
                               state.token.value, state.token.value ? loopMode(state.options.mode) : "unknown",
                               state.token.value ? mbps(state.options.speed) : 0U,
                               unsigned(state.controlsVerified), unsigned(state.temporaryMayBeActive));
      return status;
    }
    case Feature::BIST_START: return diagnostic_detail::startBist(io, args);
    case Feature::BIST_READ: {
      const int evidence = args.size() == 0 ? 0 : featureChoice(args[0], "none|single|stimulus|both");
      if (evidence < 0) return featureInvalid();
      if (evidence != 0 && !io.attest(static_cast<uint32_t>(evidence))) return featureDenied();
      return io.template start<&Core::startBistAcquire>(BistObservation{(evidence & 1) != 0, (evidence & 2) != 0});
    }
    case Feature::BIST_STOP: return io.template start<&Core::stopBist>();
    case Feature::BIST_STATUS: {
      BistSnapshot state;
      const auto status = io.template cached<&Core::bistSnapshot>(state);
      if (status.ok()) bistFields(state.measurement, out, state.stale, &state);
      return status;
    }
    case Feature::CABLE_START: case Feature::CABLE_RAW: return startCable(io, feature, args);
    case Feature::CABLE_READ: return io.template start<&Core::startCableAcquire>();
    case Feature::CABLE_STOP: return io.template start<&Core::stopCableTest>();
    case Feature::CABLE_AUTORUN_READ: return io.template start<&Core::startCableAutorunAcquire>();
    case Feature::CABLE_STATUS: {
      CableSnapshot state;
      const auto status = io.template cached<&Core::cableSnapshot>(state);
      if (status.ok()) cableFields(state.result, out);
      return status;
    }
    case Feature::CABLE_INTERPRET: {
      uint32_t delay = 0;
      if (!featureNumber(args[0], UINT32_MAX, delay) || delay == 0) return featureInvalid();
      CableSnapshot state;
      const auto cached = io.template cached<&Core::cableSnapshot>(state);
      if (!cached.ok()) return cached;
      CableDiagnosis diagnosis;
      const auto status = interpretCable(state.result, static_cast<double>(delay) / 1000.0, diagnosis);
      if (status.ok()) {
        constexpr const char* names[] = {"no_peak", "open", "short"};
        const auto micrometres = static_cast<uint64_t>(diagnosis.metres * 1000000.0);
        out.set(" conclusion=%s row=%u distance_valid=%u distance_um=%" PRIu64 " amplitude_limit=%u bit11=%u bit5=%u",
                names[static_cast<unsigned>(diagnosis.conclusion)], unsigned(diagnosis.run),
                unsigned(diagnosis.distanceValid), micrometres, unsigned(diagnosis.amplitudeAtLimit),
                unsigned(diagnosis.rawBit11), unsigned(diagnosis.rawBit5));
      }
      return status;
    }
    case Feature::CABLE_AUTORUN_CONFIGURE: {
      const int enabled = featureChoice(args[0], "off|on");
      if (enabled < 0) return featureInvalid();
      if (enabled == 1 && !io.attest(0)) return featureDenied();
      return io.template start<&Core::startConfigureCableAutorun>(CableAutorunOptions{enabled == 1, true});
    }
    case Feature::CABLE_AUTORUN_STATUS: {
      CableAutorunSnapshot state;
      const auto status = io.template cached<&Core::cableAutorunSnapshot>(state);
      if (status.ok()) autorunFields(state, out);
      return status;
    }
    case Feature::COMPLIANCE_START: {
      const int mode = featureChoice(args[0], "tx100|nlp10|prbs10|pattern10|count100");
      const int pair = featureChoice(args[1], "mdi|mdix");
      uint32_t pattern = 0, flags = 0;
      if (mode < 0 || pair < 0 || !featureNumber(args[2], mode == 4 ? 31U : mode == 3 ? 13U : 0U, pattern) ||
          !featureNumber(args[3], 3, flags) || flags != 3) return featureInvalid();
      if (mode == 4) return Status::Error(Err::INVALID_CONFIG, "Source gate: exact-part COMPT configuration at 0428",
                                          static_cast<int32_t>(ConfigDetail::SOURCE_GATE));
      if (!io.attest(flags)) return featureDenied();
      const ComplianceOptions options{static_cast<ComplianceMode>(mode), pair == 0 ? PairSelection::MDI : PairSelection::MDIX,
                                      static_cast<uint8_t>(pattern), true, true, true, true};
      return io.template start<&Core::startCompliance>(options);
    }
    case Feature::COMPLIANCE_STOP: return io.template start<&Core::stopCompliance>();
    case Feature::COLLISION_START: {
      const auto selectedSpeed = speed(args[0]);
      uint32_t flags = 0;
      if (selectedSpeed == LinkSpeed::UNKNOWN || !featureNumber(args[1], 3, flags) || flags != 3) return featureInvalid();
      if (!io.attest(flags)) return featureDenied();
      return io.template start<&Core::startCollisionTest>(CollisionOptions{selectedSpeed, true, true, true, true});
    }
    case Feature::COLLISION_STOP: return io.template start<&Core::stopCollisionTest>();
    case Feature::CALIBRATION_RECOVER: {
      uint32_t flags = 0;
      if (!featureNumber(args[0], 1, flags) || flags != 1) return featureInvalid();
      if (!io.attest(flags)) return featureDenied();
      return io.template start<&Core::startCalibrationRecovery>(CalibrationOptions{true, true, true});
    }
    case Feature::ELECTRICAL_STATUS: {
      ElectricalSnapshot state;
      const auto status = io.template cached<&Core::electricalSnapshot>(state);
      if (status.ok()) electricalFields(state, out);
      return status;
    }
    default: return Status::Error(Err::UNSUPPORTED, "Unknown diagnostic feature");
  }
}

/// @brief Serialize per-operation diagnostic evidence, including partial results.
/// @param[in] feature Command that started this operation.
/// @param[in] result Frozen result for that operation token.
/// @param[out] out Bounded extra machine fields. The shared record owns status and OP-9 debt.
inline void diagnosticResult(Feature feature, const OperationResult& result, FeatureFields& out) {
  using namespace diagnostic_detail;
  switch (feature) {
    case Feature::LOOPBACK_START: case Feature::LOOPBACK_STOP: {
      const auto& loop = result.loopback;
      out.set(" mode=%s speed_mbps=%u controls_verified=%u stopped=%u hard_exit=%u physts_valid=%u physts=%u",
              loopMode(loop.mode), mbps(loop.speed), unsigned(loop.controlsVerified), unsigned(loop.stopped),
              unsigned(loop.hardResetExit), unsigned(loop.phystsValid), unsigned(loop.rawPhysts));
      break;
    }
    case Feature::BIST_START: case Feature::BIST_READ: case Feature::BIST_STOP:
      bistFields(result.bist, out, feature != Feature::BIST_READ || !result.bist.measurementValid);
      break;
    case Feature::CABLE_START: case Feature::CABLE_RAW: case Feature::CABLE_READ: case Feature::CABLE_STOP:
    case Feature::CABLE_AUTORUN_READ: cableFields(result.cable, out); break;
    case Feature::CABLE_AUTORUN_CONFIGURE: autorunFields(result.cableAutorun, out); break;
    case Feature::COMPLIANCE_START: case Feature::COMPLIANCE_STOP: case Feature::COLLISION_START:
    case Feature::COLLISION_STOP: case Feature::CALIBRATION_RECOVER: electricalFields(result.electrical, out); break;
    default: break;
  }
}

} // namespace DP8382x::Cli
