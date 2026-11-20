/// @file Dp8382xCliControls.h
/// @brief Typed interrupt, power, EEE, LPI and wake command routes.
#pragma once

#include "Dp8382xCliFeatureTypes.h"
#include <cinttypes>

namespace DP8382x::Cli {

/// @brief Application attestations requested by the IRQ, power, LPI and wake routes.
enum class ControlProof : uint32_t {
  INTR_PIN = 1U << 0U, ///< Application owns the selected INTR/PWRDN route.
  LED1_PIN = 1U << 1U, ///< Application owns LED1 and the required pin 21 role.
  LED2_PIN = 1U << 2U, ///< Application owns LED2 and the required pin 21 role.
  LED1_CLOCK_UNUSED = 1U << 3U, ///< No board consumer requires the LED1 clock.
  COL_UNUSED = 1U << 4U, ///< No board consumer requires MII COL on LED2.
  POWER_DOWN_INPUT_SAFE = 1U << 5U, ///< Board may put pin 21 in the power-down input role.
  MANAGEMENT_CLOCK = 1U << 6U, ///< Board retains the reference/MDIO clock for the transition.
  CLOCK_LOSS = 1U << 7U, ///< Application admits interruption of PHY-generated clocks.
  RX_DRAIN = 1U << 8U, ///< Application has quiesced receive traffic.
  TX_DRAIN = 1U << 9U ///< Application has stopped and drained transmit traffic.
};

namespace control_detail {
/// @cond INTERNAL

/// @brief Decode six colon-separated octets in transmission order.
/// @param[in] token Exact hexadecimal token, with no trailing data.
/// @param[out] octets Decoded bytes. Invalid input can leave a partial prefix.
/// @return True only for a complete match.
inline bool octets(std::string_view token, uint8_t (&octets)[6]) noexcept {
  if (token.size() != 17) return false;
  const auto nibble = [](char ch) -> int {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return -1;
  };
  for (std::size_t i = 0; i < 6; ++i) {
    const auto high = nibble(token[i * 3]);
    const auto low = nibble(token[i * 3 + 1]);
    if (high < 0 || low < 0 || (i < 5 && token[i * 3 + 2] != ':')) return false;
    octets[i] = static_cast<uint8_t>(high * 16 + low);
  }
  return true;
}

/// @brief Encode retained interrupt policy. The raw MISRs can include consumed events.
/// @param[in] view Copied core snapshot.
/// @param[out] out Bounded record fields.
inline void format(const InterruptSnapshot& view, FeatureFields& out) noexcept {
  out.set(" valid=%u trust=%u stale=%u selector=%u test_active=%u physcr_valid=%u physcr=%u"
          " misr1_valid=%u misr1=%u misr2_valid=%u misr2=%u rxfs_valid=%u rxfs=%u",
          unsigned(view.desiredValid), unsigned(view.trust), unsigned(view.observedStale),
          unsigned(view.selector), unsigned(view.testMayBeActive), unsigned(view.physcrValid), unsigned(view.rawPhyscr),
          unsigned(view.misr1Valid), unsigned(view.rawMisr1), unsigned(view.misr2Valid), unsigned(view.rawMisr2),
          unsigned(view.rxfsValid), unsigned(view.rawRxfs));
}

/// @brief Encode one interrupt service acquisition, with optional half-full counter evidence.
/// @param[in] view Fresh success or explicitly requested cached snapshot.
/// @param[out] out Bounded record fields.
inline void format(const InterruptServiceSnapshot& view, FeatureFields& out) noexcept {
  out.set(" valid=%u stale=%u more_pending=%u physts=%u misr1=%u misr2=%u shared_source=%u"
          " fcscr_requested=%u fcscr_valid=%u fcscr=%u fcscr_lower_bound=%u recr_requested=%u recr_valid=%u recr=%u"
          " last_status=%s", unsigned(view.valid), unsigned(view.stale), unsigned(view.morePossiblyPending),
          unsigned(view.rawPhysts), unsigned(view.misr1.raw), unsigned(view.misr2.raw), unsigned(view.misr2.sharedSource),
          unsigned(view.fcscrRequested), unsigned(view.fcscr.valid), unsigned(view.fcscr.raw),
          unsigned(view.fcscr.counterLowerBound), unsigned(view.recrRequested), unsigned(view.recr.valid),
          unsigned(view.recr.raw), errorName(view.lastStatus.code));
}

/// @brief Encode power intent and independent observations, including indication validity.
/// @param[in] view Copied core snapshot.
/// @param[out] out Bounded record fields.
inline void format(const PowerSnapshot& view, FeatureFields& out) noexcept {
  out.set(" valid=%u trust=%u stale=%u power_state=%u requested_down=%u indicated_down=%u"
          " requested_sleep=%u requested_pll_off=%u"
          " bmcr_valid=%u bmcr=%u physcr_valid=%u physcr=%u biscr_valid=%u biscr=%u",
          unsigned(view.desiredValid), unsigned(view.trust), unsigned(view.observedStale), unsigned(view.state),
          unsigned(view.requestedPowerDown), unsigned(view.observedBmcrPowerDown),
          unsigned(view.desired.activeSleep), unsigned(view.desired.disablePll), unsigned(view.bmcrValid),
          unsigned(view.rawBmcr), unsigned(view.physcrValid), unsigned(view.rawPhyscr), unsigned(view.biscrValid),
          unsigned(view.rawBiscr));
}

/// @brief Encode EEE fields and independent validity, without a physical readiness claim.
/// @param[in] view Copied core snapshot.
/// @param[out] out Bounded record fields.
inline void format(const EeeSnapshot& view, FeatureFields& out) noexcept {
  out.set(" valid=%u stale=%u trust=%u desired_valid=%u enabled=%u capability_valid=%u capability=%u"
          " advertised_valid=%u advertised=%u partner_valid=%u partner=%u eee_status_valid=%u eee_status=%u"
          " wake_errors_valid=%u wake_errors=%u control_valid=%u control=%u negotiated_valid=%u negotiated=%u",
          unsigned(view.valid), unsigned(view.stale), unsigned(view.trust), unsigned(view.desiredValid),
          unsigned(view.desired.enabled), unsigned(view.capabilityValid), unsigned(view.capability),
          unsigned(view.advertisementValid), unsigned(view.advertisement), unsigned(view.partnerValid),
          unsigned(view.partner), unsigned(view.statusValid), unsigned(view.status), unsigned(view.wakeErrorsValid),
          unsigned(view.wakeErrors), unsigned(view.controlValid), unsigned(view.control),
          unsigned(view.negotiatedValid), unsigned(view.negotiatedValid && view.negotiated));
}

/// @brief Encode wake controls and event evidence. Match MAC and password bytes are never emitted.
/// @param[in] view Copied core snapshot; credential validity is independent of detector status.
/// @param[out] out Bounded record fields containing no match values.
inline void format(const WakeSnapshot& view, FeatureFields& out) noexcept {
  out.set(" desired_valid=%u trust=%u stale=%u route=%u control_valid=%u control=%u enabled=%u secure=%u"
          " level=%u pulse=%u crc=%u magic=%u match_word_mask=%u rxfs_valid=%u rxfs=%u misr2_valid=%u misr2=%u",
          unsigned(view.desiredValid), unsigned(view.trust), unsigned(view.observedStale), unsigned(view.desired.route),
          unsigned(view.controlValid), unsigned(view.rawControl), unsigned(view.enabled), unsigned(view.secureOn),
          unsigned(view.levelMode), unsigned(view.pulseWidth), unsigned(view.crcGate), unsigned(view.magicMatch),
          unsigned(view.credentialValidMask), unsigned(view.rxfsValid), unsigned(view.rawRxfs),
          unsigned(view.misr2Valid), unsigned(view.rawMisr2));
}

/// @brief Encode the completed or uncertain forced LPI request, including controller release.
/// @param[in] view Latest admitted result or default if the call was rejected before a batch.
/// @param[out] out Bounded record fields. Control is usable only when readback_valid.
inline void format(const LpiResult& view, FeatureFields& out) noexcept {
  out.set(" lpi=%u readback_valid=%u control=%u batch_primary=%s primary_detail=%" PRId32
          " batch_release=%s release_detail=%" PRId32 " attempted=%u completed=%u"
          " transactions=%u effect_completed=%u effect_possible=%u", unsigned(view.forcedLpi),
          unsigned(view.readbackValid), unsigned(view.control), errorName(view.primary.code), view.primary.detail,
          errorName(view.release.code), view.release.detail, unsigned(view.attemptedFrames), unsigned(view.completedFrames),
          unsigned(view.transactions), unsigned(view.mutationCompleted), unsigned(view.mutationPossible));
}

template<class Access>
bool approve(Access& io, ControlProof proof) { return io.attest(static_cast<uint32_t>(proof)); }

template<class Value, auto Accessor, bool Cached, class Access>
Status observe(Access& io, FeatureFields& out) {
  Value view{};
  Status status{};
  if constexpr (Cached) status = io.template cached<Accessor>(view);
  else status = io.template read<Accessor>(view);
  if (status.ok()) format(view, out);
  return status;
}
/// @endcond
} // namespace control_detail

/// @brief Dispatch one typed control request through the current application owner.
/// @param[in,out] io Serialized owner access and original bounded request allowance.
/// @param[in] feature Validated command selector.
/// @param[in] args Bounded tokens, borrowed only for this call.
/// @param[out] out Fresh or explicitly cached evidence fields, never credentials.
/// @return Core status, input or owner refusal, or IN_PROGRESS for an accepted start.
template<class Access>
Status controlFeature(Access& io, Feature feature, const FeatureArgs& args, FeatureFields& out) {
  using namespace control_detail;
  switch (feature) {
    case Feature::IRQ_CONFIGURE: {
      uint32_t first{}, second{}, flags{};
      const int source = featureChoice(args[2], "polarity|wake");
      const int pin = featureChoice(args[3], "intr|led1|led2");
      if (source < 0 || pin < 0 || !featureNumber(args[0], 255, first) || !featureNumber(args[1], 255, second) ||
          !featureNumber(args[4], 7, flags)) return featureInvalid();
      InterruptSettings settings{};
      settings.misr1Mask = static_cast<uint16_t>(first);
      settings.misr2Mask = static_cast<uint16_t>(second);
      settings.sharedSource = source == 0 ? InterruptSharedSource::POLARITY : InterruptSharedSource::WAKE;
      settings.pin = static_cast<InterruptPin>(pin);
      settings.activeLow = (flags & 1U) == 0U;
      settings.globalEnable = (flags & 2U) != 0U;
      settings.outputEnable = (flags & 4U) == 0U;
      settings.maintenanceAdmitted = true;
      settings.pinAvailable = approve(io, pin == 0 ? ControlProof::INTR_PIN :
          (pin == 1 ? ControlProof::LED1_PIN : ControlProof::LED2_PIN));
      if (!settings.pinAvailable) return featureDenied();
      settings.led1ClockUnused = pin == 1 && approve(io, ControlProof::LED1_CLOCK_UNUSED);
      settings.colUnused = pin == 2 && approve(io, ControlProof::COL_UNUSED);
      settings.powerDownInputSafe = !settings.outputEnable && approve(io, ControlProof::POWER_DOWN_INPUT_SAFE);
      return io.template start<&Core::startConfigureInterrupts>(settings);
    }
    case Feature::IRQ_STATUS: return observe<InterruptSnapshot, &Core::interruptSnapshot, true>(io, out);
    case Feature::IRQ_CACHED_SERVICE:
      return observe<InterruptServiceSnapshot, &Core::interruptServiceSnapshot, true>(io, out);
    case Feature::IRQ_SERVICE: {
      auto budget = InterruptServiceBudget{io.budget().io, 2, 5};
      if (budget.io.maxFrames > 5U) budget.io.maxFrames = 5;
      if (args.size() != 0) {
        uint32_t transactions{}, samples{}, frames{};
        if (args.size() != 3 || !featureNumber(args[0], 2, transactions) || transactions == 0 ||
            !featureNumber(args[1], 5, samples) || samples == 0 || !featureNumber(args[2], 5, frames) ||
            frames == 0) return featureInvalid();
        budget.maxTransactions = static_cast<uint8_t>(transactions);
        budget.maxSamples = static_cast<uint8_t>(samples);
        if (frames < budget.io.maxFrames) budget.io.maxFrames = frames;
      }
      InterruptServiceSnapshot view{};
      const auto status = io.template readWith<&Core::serviceInterrupts>(view, budget);
      if (status.ok()) format(view, out);
      return status;
    }
    case Feature::IRQ_TEST_START:
      if (!io.attest(0)) return featureDenied();
      return io.template start<&Core::startInterruptTest>();
    case Feature::IRQ_TEST_STOP: return io.template start<&Core::stopInterruptTest>();
    case Feature::POWER_CONFIGURE: {
      uint32_t sleep{}, pll{};
      const int request = featureChoice(args[0], "normal|down|deep");
      if (request < 0 || !featureNumber(args[1], 1, sleep) || !featureNumber(args[2], 1, pll))
        return featureInvalid();
      const bool clockLoss = request == 1 && approve(io, ControlProof::CLOCK_LOSS);
      const bool clockAvailable = request != 2 && approve(io, ControlProof::MANAGEMENT_CLOCK);
      if ((request == 1 && !clockLoss) || (request != 2 && !clockAvailable)) return featureDenied();
      return io.template start<&Core::startConfigurePower>(PowerSettings{static_cast<PowerRequest>(request),
          sleep != 0, pll != 0, true, clockLoss, clockAvailable});
    }
    case Feature::POWER_STATUS: return observe<PowerSnapshot, &Core::powerSnapshot, true>(io, out);
    case Feature::POWER_READ: return observe<PowerSnapshot, &Core::readPowerStatus, false>(io, out);
    case Feature::EEE_CONFIGURE: {
      uint32_t advertisement{}, bypass{};
      const int enabled = featureChoice(args[0], "off|on");
      if (enabled < 0 || !featureNumber(args[1], 2, advertisement) || advertisement == 1 ||
          !featureNumber(args[2], 3, bypass)) return featureInvalid();
      if (!io.attest(0)) return featureDenied();
      return io.template start<&Core::startConfigureEee>(EeeSettings{enabled != 0, static_cast<uint16_t>(advertisement),
          (bypass & 1U) != 0, (bypass & 2U) != 0, true});
    }
    case Feature::EEE_STATUS: return observe<EeeSnapshot, &Core::eeeSnapshot, true>(io, out);
    case Feature::EEE_READ: return observe<EeeSnapshot, &Core::readEeeStatus, false>(io, out);
    case Feature::LPI_STATUS: {
      LpiSnapshot view{};
      const auto status = io.template cached<&Core::lpiSnapshot>(view);
      if (status.ok()) {
        view.lastResult.forcedLpi = view.forcedLpi;
        format(view.lastResult, out);
      }
      return status;
    }
    case Feature::LPI_START: {
      LpiResult result{};
      if (!approve(io, ControlProof::TX_DRAIN)) return featureDenied();
      const auto status = io.template direct<&Core::startLpi>(LpiOptions{true, true}, result);
      format(result, out);
      return status;
    }
    case Feature::LPI_STOP: {
      LpiResult result{};
      const auto status = io.template read<&Core::stopLpi>(result);
      format(result, out);
      return status;
    }
    case Feature::WOL_CONFIGURE: {
      WakeSettings settings{};
      uint32_t crc{}, width{};
      const int mode = featureChoice(args[0], "off|magic|secure");
      const int route = featureChoice(args[1], "status|irq|led1|led2|led0|led3");
      const int indication = featureChoice(args[3], "level|pulse");
      if (mode < 0 || route < 0 || indication < 0 || !featureNumber(args[2], 1, crc) ||
          !featureNumber(args[4], 3, width) || !octets(args[5], settings.mac) ||
          (mode == 2 && args.size() != 7) || (args.size() == 7 && !octets(args[6], settings.password)))
        return featureInvalid();
      settings.enabled = mode != 0;
      settings.secureOn = mode == 2;
      settings.crcGate = crc != 0;
      settings.levelMode = indication == 0;
      settings.pulseWidth = static_cast<uint8_t>(width);
      settings.route = static_cast<WakeRoute>(route);
      settings.maintenanceAdmitted = true;
      settings.receiveTrafficQuiesced = approve(io, ControlProof::RX_DRAIN);
      if (!settings.receiveTrafficQuiesced) return featureDenied();
      if (route == 1) settings.pinAvailable = approve(io, ControlProof::INTR_PIN);
      if (route == 2) settings.pinAvailable = approve(io, ControlProof::LED1_PIN);
      if (route == 3) settings.pinAvailable = approve(io, ControlProof::LED2_PIN);
      if (route >= 1 && route <= 3 && !settings.pinAvailable) return featureDenied();
      settings.led1ClockUnused = route == 2 && approve(io, ControlProof::LED1_CLOCK_UNUSED);
      settings.colUnused = route == 3 && approve(io, ControlProof::COL_UNUSED);
      return io.template start<&Core::startConfigureWake>(settings);
    }
    case Feature::WOL_STATUS: return observe<WakeSnapshot, &Core::wakeSnapshot, true>(io, out);
    case Feature::WOL_READ: return observe<WakeSnapshot, &Core::readWakeStatus, false>(io, out);
    case Feature::WOL_ACKNOWLEDGE: return io.template start<&Core::acknowledgeWake>();
    default: return Status::Error(Err::UNSUPPORTED, "Unknown control feature");
  }
}

/// @brief Project only the selected control's retained operation evidence.
/// @param[in] feature Command that started the operation, including an explicit stop.
/// @param[in] result Delivered result, including partial effects after failure.
/// @param[out] out Bounded feature fields. Wake match values are always omitted.
inline void controlResult(Feature feature, const OperationResult& result, FeatureFields& out) noexcept {
  switch (feature) {
    case Feature::IRQ_CONFIGURE: case Feature::IRQ_TEST_START: case Feature::IRQ_TEST_STOP:
      control_detail::format(result.interrupts, out); break;
    case Feature::POWER_CONFIGURE: control_detail::format(result.power, out); break;
    case Feature::EEE_CONFIGURE: control_detail::format(result.eee, out); break;
    case Feature::WOL_CONFIGURE: case Feature::WOL_ACKNOWLEDGE: control_detail::format(result.wake, out); break;
    default: break;
  }
}
} // namespace DP8382x::Cli
