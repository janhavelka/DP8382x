/// @file DP8382x.h
/// @brief Portable bounded direct/indirect access and passive transport evidence.
#pragma once

#include <DP8382x/Config.h>

/// @brief Portable PHY driver and copied transport observations.
namespace DP8382x {

/// @cond INTERNAL
namespace detail { struct PartProfile; }
/// @endcond

/// @brief Passive managed-transport state; never an admission or recovery policy.
enum class DriverState : uint8_t {
  UNINIT,   ///< Initialization is not currently established, regardless of counters.
  READY,    ///< Initialized with no consecutive managed transport failures.
  DEGRADED, ///< Initialized with failures below the configured offline threshold.
  OFFLINE   ///< Initialized with failures at or above threshold; explicit attempts remain legal.
};

/// @brief Name a passive state without I/O, allocation or clock access.
/// @param[in] state State to name, including invalid enum representations.
/// @return Uppercase static name, or UNKNOWN for an invalid value.
inline constexpr const char* driverStateName(DriverState state) noexcept {
  switch (state) {
    case DriverState::UNINIT: return "UNINIT";
    case DriverState::READY: return "READY";
    case DriverState::DEGRADED: return "DEGRADED";
    case DriverState::OFFLINE: return "OFFLINE";
  }
  return "UNKNOWN";
}

/// @brief Managed batch health, separate from link, configuration trust and chip counters.
/// @note One eligible callback is one transaction, irrespective of its frame count.
/// Times are post-callback raw clock samples in microseconds, subject to the
/// binding's clockLagUs uncertainty. Zero is valid when flagged. They are not
/// precise wire completion times or freshness deadlines; a clock fault forbids age use.
struct HealthSnapshot {
  DriverState state{DriverState::UNINIT}; ///< Derived from initialization and the current streak.
  bool initialized{false}; ///< Current initialization evidence, not a link or safety claim.
  uint32_t consecutiveFailures{0}; ///< Saturating managed failure streak; not an admission latch.
  uint32_t totalFailures{0}; ///< Saturating failed managed batch count.
  uint32_t totalSuccess{0}; ///< Saturating successful managed batch count.
  uint64_t lastOkUs{0}; ///< Post-callback raw time of the last managed success.
  uint64_t lastErrorUs{0}; ///< Post-callback raw time of the retained error.
  bool lastOkTimeValid{false}; ///< Independent validity of lastOkUs.
  bool lastErrorTimeValid{false}; ///< Independent validity of lastErrorUs.
  Status lastError{Err::OK, 0, "No managed transport failure"}; ///< Normalized failure and full signed detail; success preserves it.
};

/// @brief Assessment of complete identity words under the exact-part policy.
enum class IdentityAcceptance : uint8_t {
  UNASSESSED, ///< No complete successful acquisition has been assessed.
  ACCEPTED,   ///< Historical assessment accepted; stale still forbids capabilities.
  REJECTED    ///< Complete successful reads failed identity admission.
};

/// @brief Latest identity acquisition; partial words never establish usable identity.
struct DeviceIdentity {
  Variant declaredVariant{Variant::UNKNOWN}; ///< Application declaration, never decoded suffix.
  uint16_t id1{0}; ///< Raw PHYIDR1; meaningful only when id1Valid.
  uint16_t id2{0}; ///< Raw PHYIDR2; meaningful only when id2Valid.
  bool id1Valid{false}; ///< This acquisition returned a trusted first word.
  bool id2Valid{false}; ///< This acquisition returned both words in order.
  uint32_t oui{0}; ///< 22-bit OUI; meaningful only when decodedValid.
  uint8_t model{0}; ///< Six-bit model; meaningful only when decodedValid.
  uint8_t revision{0}; ///< Raw revision nibble; not an acceptance claim.
  OperatingMode mode{OperatingMode::UNKNOWN}; ///< Recognized model's mode, not MAC wiring.
  bool decodedValid{false}; ///< Both raw words decoded, even if admission failed.
  IdentityAcceptance acceptance{IdentityAcceptance::UNASSESSED}; ///< Latest complete assessment.
  Status admissionStatus{Err::NOT_INITIALIZED, 0, "No accepted identity"}; ///< Acquisition/admission outcome.
  uint64_t observedAtUs{0}; ///< Conservative complete-acquisition timestamp, microseconds.
  bool timeValid{false}; ///< Timestamp is usable independently of admission.
  bool stale{true}; ///< Invalidation retains historical words but withdraws capabilities.
};

/// @brief Foundational silicon capabilities of a freshly accepted declared part.
/// @note Invalid means unknown, not absent. These fields do not claim that typed
/// features, the SDK or physical fixtures have been implemented or qualified.
struct Capabilities {
  bool valid{false}; ///< All remaining fields require this flag.
  uint8_t linkModeMask{0}; ///< LINK_* source-supported technology bits.
  uint8_t interfaceMask{0}; ///< INTERFACE_* source-supported MAC interface bits.
  bool copper{false}; ///< Copper medium documented for the accepted part.
  bool fiber{false}; ///< Fiber medium documented for the accepted part.
  bool gigabit{false}; ///< Gigabit speed documented for the accepted part.
  Variant declaredVariant{Variant::UNKNOWN}; ///< Board ordering grade associated with this accepted capability set.
};

/// @brief Current negotiation evidence, separate from link and transport success.
enum class NegotiationState : uint8_t {
  UNKNOWN, ///< No usable observation.
  DISABLED, ///< BMCR disables autonegotiation.
  IN_PROGRESS, ///< AN enabled without current completion evidence.
  COMPLETE ///< AN enabled with coherent completion evidence.
};
/// @brief Latest publication or stale transition; cached readers never consume it.
enum class LinkChange : uint8_t {
  NONE, ///< No coherent observation yet.
  FIRST_VALID, ///< First coherent observation in this binding.
  CHANGED, ///< A semantic link/mode/pause tuple changed.
  REPEATED, ///< Same semantic tuple in another coherent pass.
  STALE, ///< Prior observation became historical after failure or mutation.
  REGAINED ///< Coherent observation restored after staleness.
};
/// @brief Copied link evidence from a bounded consistency check, not simultaneous sampling.
struct LinkSnapshot {
  bool valid{false}; ///< A complete coherent pass exists; stale may make it historical.
  bool stale{true}; ///< Failure or mutation withdrew current use; old values/time survive.
  bool linkUp{false}; ///< Current link only when valid and not stale.
  NegotiationState negotiation{NegotiationState::UNKNOWN}; ///< BMCR and current BMSR evidence.
  LinkSpeed speed{LinkSpeed::UNKNOWN}; ///< Local speed, usable only with resolvedModeValid.
  Duplex duplex{Duplex::UNKNOWN}; ///< Local duplex, never proof of partner duplex in forced mode.
  bool resolvedModeValid{false}; ///< Both speed and duplex have applicable source evidence.
  bool temporaryMayBeActive{false}; ///< A temporary profile may explain link; normal resolution withheld.
  bool pauseRx{false}; ///< Receive pause from IEEE 28B-3 when pauseValid.
  bool pauseTx{false}; ///< Transmit pause from IEEE 28B-3 when pauseValid.
  bool pauseValid{false}; ///< Highest common full-duplex mode and PHYCR qualify computed pause.
  uint64_t observedSinceUs{0}; ///< Acquisition start lower bound in microseconds; use for maximum age.
  uint64_t observedAtUs{0}; ///< Conservative acquisition completion upper bound, microseconds.
  bool timeValid{false}; ///< Timestamp usable; zero itself never means invalid.
  Status lastRefreshStatus{Err::NOT_INITIALIZED, 0, "No link observation"}; ///< Latest acquisition or invalidation outcome.
  LinkChange change{LinkChange::NONE}; ///< Latest sampling/stale outcome; not an event queue.
  uint32_t observations{0}; ///< Coherent publications, saturating at UINT32_MAX.
  uint32_t changes{0}; ///< First valid, tuple changes and fresh/stale transitions, saturating.
  uint16_t rawBmsrFirst{0}; ///< First latch-low sample, historical loss may be present.
  uint16_t rawBmsrCurrent{0}; ///< Second BMSR sample, used for current link.
  uint16_t rawPhystsFirst{0}; ///< First PHYSTS, including mirrors before ANER clears them.
  uint16_t rawBmcr{0}; ///< Mode and local forced speed/duplex.
  uint16_t rawAnar{0}; ///< Local advertisement, including selector and pause.
  uint16_t rawAnlpar{0}; ///< Partner base-page advertisement.
  uint16_t rawAner{0}; ///< Consumed page/capability/parallel-detection evidence.
  uint16_t rawPhycr{0}; ///< Independent RX/TX pause cross-check, not the pause algorithm.
  uint16_t rawBmsrFinal{0}; ///< Final consistency sample; latch-low detects intervening loss.
  uint16_t rawPhystsFinal{0}; ///< Final link/AN/applicable-mode consistency sample.
};

/// @brief First detected inconsistency in the last backend report.
enum class TransportViolation : uint8_t {
  NONE,             ///< Report meets the terminal contract.
  STATUS_CODE,      ///< Illegal primary status or detail.
  NULL_MESSAGE,     ///< Primary message is null.
  FRAME_REPORT,     ///< Frame flags, values, count or timestamp contradict the batch.
  RELEASE_REPORT,   ///< Illegal release status or inconsistent reuse report.
  CONTROLLER_STATE  ///< Release succeeded but controller idle is not established.
};

/// @brief Transport counts and last backend report, separate from managed health.
struct TransportSnapshot {
  uint32_t totalBatches{0};    ///< Callback invocations, saturating at UINT32_MAX.
  uint32_t attemptedFrames{0};///< Valid requested prefix attempts, saturating.
  uint32_t completedFrames{0};///< Valid requested prefix completions, saturating.
  BatchResult lastBatch{};    ///< Exact report, including invalid codes and possible effects.
  TransportViolation violation{TransportViolation::NONE}; ///< Last report defect.
  bool stopped{false};        ///< Malformed report stopped this binding; owner repair required.
  bool late{false};           ///< Raw post-call time reached deadline; invalid if clockInvalid.
  bool clockInvalid{false};   ///< Latched timeline fault; repair requires end/rebind.
  bool controllerReusable{false}; ///< Last validated report; never authorizes shared access.
  Status lastReleaseStatus{}; ///< Audited release outcome for lastBatch; use the raw report for diagnostics.
};

/// @brief Destructive sources, in acquisition order; PHYSTS precedes mirrors.
enum class EvidenceSource : uint8_t {
  PHYSTS, ///< Raw status before dependent clearing reads; table 8-34.
  BMSR, ///< Latched basic status; table 8-20.
  ANER, ///< Page receive consumes PHYSTS mirror; tables 8-25/34.
  FLDS, ///< Latched fast-link-drop causes; table 8-33.
  MISR1, ///< Interrupt events and stable enables; table 8-36.
  MISR2, ///< Interrupt events and stable enables; table 8-37.
  FCSCR, ///< Eight-bit clear-on-read false-carrier sample; table 8-38.
  RECR, ///< Raw clear-on-read word; width/saturation conflict remains; table 8-39.
  RCSR, ///< Raw RMII RC state; polarity/rearm gate remains; table 8-41.
  BTSCR, ///< 10BTSCR polarity history clears despite R type; table 8-44.
  RXFS, ///< WoL/filter history and routing; table 8-91.
  BISCR, ///< BIST current activity/lock and hardware-controlled sync-loss; table 8-40.
  BICSR1, ///< BIST captured error byte and IPG; capture/clear is owner-only, table 8-45.
  EEE_STATUS, ///< MMD 3.1 current and latched LPI evidence; source-implied Q1.
  EEE_ERRORS, ///< MMD 3.22 read-clear wake-error word; source-implied Q1.
  REGISTER_12, ///< DP83825I C22 000C upper-byte latches, consumed separately from MISR IRQ service (SNLS638C table 7-15).
  COUNT ///< Number of supported sources, not a readable source.
};

/// @brief Build an acquisition mask; invalid enum values produce zero.
/// @param[in] source Named destructive source.
/// @return One mask bit or zero, without I/O.
constexpr uint32_t evidenceBit(EvidenceSource source) noexcept {
  return source < EvidenceSource::COUNT ? uint32_t{1} << static_cast<uint8_t>(source) : 0U;
}
/// @brief All common destructive sources. Explicitly add REGISTER_12 to read DP83825I C22 000C. Its effect on MISR1 history is unknown.
constexpr uint32_t ALL_EVIDENCE = evidenceBit(EvidenceSource::REGISTER_12) - 1U;

/// @brief Context for MISR2[9] at consumption; a later selector cannot relabel history.
enum class InterruptSharedSource : uint8_t {
  UNKNOWN, ///< Transition, external change or unowned selector.
  POLARITY, ///< Owned RXFS[12]=0, with crossing evidence already consumed.
  WAKE ///< Owned RXFS[12]=1; not proof that a valid wake packet arrived.
};

/// @brief Retained evidence from one destructive source, separate from aggregate success.
struct EvidenceSample {
  uint16_t raw{0}; ///< Latest successfully returned full word, including unknown bits.
  uint16_t observedOnes{0}; ///< Union of raw ones; no implied physical event decoding.
  uint16_t observedZeros{0}; ///< Union of raw zeros, retaining latched-low observations.
  uint32_t samples{0}; ///< Successful consumed words, saturating; not an event count.
  bool samplesSaturated{false}; ///< Sticky: the retained sample count could no longer advance.
  uint16_t eventBits{0}; ///< Sticky sourced event bits, separate from current raw controls/status.
  bool coalesced{false}; ///< Event occurrence seen; hardware latches cannot prove multiplicity.
  uint64_t sampledSum{0}; ///< FCSCR low-byte sum only; RECR arithmetic is source-gated and remains zero.
  bool sumSaturated{false}; ///< Sticky arithmetic overflow; sampledSum is then a lower bound.
  bool counterSaturated{false}; ///< Sticky FCSCR FF sample: hardware stopped counting at saturation.
  bool counterLowerBound{false}; ///< FCSCR sum has saturation, lost-history or discontinuity evidence.
  bool discontinuous{false}; ///< Reset/external or configuration uncertainty crossed retained history.
  uint64_t observedAtUs{0}; ///< Conservative completion bound in the binding clock domain.
  bool valid{false}; ///< At least one independently valid word has been retained.
  bool timeValid{false}; ///< Timestamp is usable; zero is a valid timestamp.
  bool stale{true}; ///< Failed refresh or known external/configuration change.
  bool historyLost{false}; ///< Sticky: a possible consumption returned no trusted word.
  InterruptSharedSource sharedSource{InterruptSharedSource::UNKNOWN}; ///< MISR2 latest sample's selector context only.
  bool polaritySeen{false}; ///< Sticky MISR2[9] consumed under a proved polarity selector.
  bool wakeSeen{false}; ///< Sticky MISR2[9] consumed under a proved wake selector.
  bool sharedAmbiguous{false}; ///< Sticky MISR2[9] consumed without proved epoch attribution.
};

/// @brief Passive destructive evidence; binding/end begin a new local history.
struct EvidenceSnapshot {
  std::array<EvidenceSample, static_cast<uint8_t>(EvidenceSource::COUNT)> samples{}; ///< Indexed by EvidenceSource.
  uint32_t lastRequested{0}; ///< Sources requested by the latest admitted acquisition.
  Status lastStatus{Err::RESULT_NOT_AVAILABLE, 0, "No evidence acquisition"}; ///< Latest acquisition or admitted mixed update result; comparison-only wake exit leaves it unchanged.
  bool configurationDirty{false}; ///< Historical mutation/uncertainty latch; not the initialized-state flag.
  uint8_t pendingInterruptCounters{0}; ///< Consumed MISR1 requests awaiting a valid counter read: bit 0 FCSCR, bit 1 RECR.
};

/// @brief Complete six-source acquisition, separate from immediately retained partial evidence.
/// @note SNLS647I tables 8-33..39. No atomic silicon snapshot, event multiplicity,
/// RECR count or physical error rate is inferred. History survives reset as stale.
struct CounterSnapshot {
  bool valid{false}; ///< A complete successful acquisition exists.
  bool stale{true}; ///< Failure, another consuming read or invalidation made this historical.
  bool timeValid{false}; ///< Stored timestamp valid in its original clock domain.
  uint64_t observedAtUs{0}; ///< Complete acquisition upper time bound, microseconds.
  Status lastStatus{Err::RESULT_NOT_AVAILABLE, 0, "No event acquisition"}; ///< Last aggregate outcome/invalidation.
  uint16_t rawPhysts{0}; ///< Mirror/status word captured before dependent clearers; not resolved link.
  uint16_t rawFlds{0}; ///< Full fast-link-drop cause word.
  uint16_t rawMisr1{0}; ///< Current returned events plus enables, not lifetime OR.
  uint16_t rawMisr2{0}; ///< Current events/enables; bit 0200 remains polarity-or-WoL.
  uint16_t rawFcscr{0}; ///< Full returned false-carrier word including reserved bits.
  uint16_t rawRecr{0}; ///< Raw receive-error word; width/saturation conflict forbids arithmetic.
  uint16_t misr1Events{0}; ///< This pass's high-byte MISR1 events, with named cmd masks.
  uint16_t misr2Events{0}; ///< This pass's high-byte MISR2 events; shared source is unclassified.
  uint16_t fldCauses{0}; ///< This pass's FLDS causes under mask 01F0.
  uint64_t falseCarrierTotal{0}; ///< Sum of returned FCSCR low bytes in the retained host window.
  bool falseCarrierLowerBound{false}; ///< Saturation, lost read or discontinuity prevents an exact total.
  bool falseCarrierOverflow{false}; ///< Host arithmetic saturated at UINT64_MAX.
};

/// @brief Owner notification of lost device trust; no action is taken on the PHY.
enum class InvalidationReason : uint8_t {
  EXTERNAL_RESET, ///< Owner reports reset; old samples remain historical.
  EXTERNAL_POWER_ON, ///< Owner reports a power cycle.
  EXTERNAL_MUTATION, ///< Another writer may have changed hardware.
  RAW_MUTATION, ///< Owner performed a raw semantic write.
  AMBIGUOUS_MUTATION ///< A write may have applied without valid completion.
};

/// @brief Instance-scoped lifetime operation identity; zero is invalid.
struct OperationToken {
  uint64_t value{0}; ///< Value is never reset by bind/end.
};
/// @brief No operation token.
constexpr OperationToken INVALID_OPERATION_TOKEN{};
/// @brief Implemented cooperative operation kinds.
enum class OperationKind : uint8_t {
  NONE, ///< Empty slot.
  INITIALIZE, ///< Identity, optional reset and owned configuration replay.
  CONFIGURE, ///< Complete forced/negotiated profile and optional finite next-page exchange.
  VERIFY, ///< Read identity and currently owned stable settings without semantic writes.
  LOOPBACK_START, ///< Establish a temporary path; dwell belongs to the application.
  LOOPBACK_STOP, ///< Stop the path and verify restoration in a separate operation.
  BIST_START, ///< Establish a BIST session using the shared loopback procedure.
  BIST_ACQUIRE, ///< Capture one bounded current-session BIST observation.
  BIST_STOP, ///< Disable BIST and verify restoration under a new budget.
  CABLE_START, ///< Configure and capture a finite manual cable plan; explicit stop required.
  CABLE_ACQUIRE, ///< Nondestructively reacquire the current triggered run.
  CABLE_STOP, ///< Stop manual capture, HARD reset and verify saved-state restoration.
  CABLE_AUTORUN_CONFIGURE, ///< Apply one persistent hardware link-down autorun flag.
  CABLE_AUTORUN_ACQUIRE, ///< Acquire one uncorrelated raw autorun result without waiting for a trigger.
  INTERRUPT_CONFIGURE, ///< Apply and verify explicitly owned interrupt policy.
  INTERRUPT_TEST_START, ///< Establish a retained software interrupt stimulus.
  INTERRUPT_TEST_STOP, ///< Clear and verify the owned software interrupt stimulus.
  POWER_CONFIGURE, ///< Apply owned active-sleep, IEEE power intent and PLL control.
  EEE_CONFIGURE, ///< Reset-bearing exact EEE recipe and shared owned-state replay.
  WOL_CONFIGURE, ///< Quiesced match/control update with detector disabled until verification.
  WOL_ACKNOWLEDGE, ///< Explicit W0C acknowledgement after collector harvest; never replayed.
  PIN_CONFIGURE, ///< Direct pin-profile mutation owns restoration debt; no retained operation token.
  ANALOG_CONFIGURE, ///< Persistent electrical profile and split-amplitude transition.
  COMPLIANCE_START, ///< Establish a temporary instrument stimulus.
  COMPLIANCE_STOP, ///< HARD stop; persistent replay is a later initialize.
  COLLISION_START, ///< Set an admitted MII collision stimulus.
  COLLISION_STOP, ///< Clear and verify the owned collision stimulus.
  CALIBRATION_RECOVERY, ///< One CDCR contingency and bounded HARD completion.
  QUALITY_CONFIGURE, ///< Owned quality, TX and hardware link-loss policy.
  INTERFACE_CONFIGURE ///< Direct interface/clock mutation owns restoration debt, without a job token.
};
/// @brief One retained operation slot.
enum class OperationState : uint8_t {
  IDLE, ///< No retained operation.
  RUNNING, ///< Caller polling advances work.
  RESULT_PENDING ///< Frozen result awaits delivery.
};
/// @brief Distinct source-backed initialization reset choices.
enum class ResetKind : uint8_t {
  NONE, ///< Apply mandatory initialization without issuing reset.
  PCS, ///< Reset PCS registers, retaining vendor registers.
  HARD, ///< Reset all registers; eligible straps may resample (Q5 open).
  RESTART, ///< Restart PHY circuitry, retaining register configuration.
  EEE_PCS ///< DP83825I MMD3:0000[15] resets IEEE MMD3/MMD7 and PCS. Vendor programming remains.
};
/// @brief Observable procedure phase; waits never retain a controller lock.
enum class OperationPhase : uint8_t {
  NONE, ///< Empty slot.
  PREFLIGHT, ///< Local request admission.
  IDENTIFY, ///< Identity and raw strap capture.
  QUIESCE, ///< Check for an already active reset.
  COMMAND, ///< Issue a reset command or the owned AN restart.
  WAIT, ///< Scheduled reset, negotiation or page observation.
  RESTORE, ///< Apply implemented persistent settings.
  VERIFY, ///< Read back stable owned settings.
  COMPLETE ///< Frozen terminal result.
};
/// @brief Finite aggregate work and polling policy; no guessed silicon deadlines.
struct OperationBudget {
  IoBudget io{}; ///< Absolute deadline and total physical-frame allowance.
  uint32_t pollIntervalUs{1000}; ///< Positive policy interval after reset/completion reads.
  uint32_t maxStalledPolls{100000}; ///< Positive consecutive no-work/no-clock-advance limit.
};
/// @brief Exact DP83825I functional modes in SNLS638C section 6.4.
enum class FunctionalMode : uint8_t {
  RETAIN, ///< Keep current ownership. An owned mode is replayed on HARD.
  DEFAULT, ///< HARD defaults, with no table writes.
  POWER_OPTIMIZED, ///< Table 6-5, nominal 130 m.
  EXTENDED_REACH, ///< Table 6-6, nominal 150 m.
  EXTENDED_REACH_EEE ///< Table 6-7 with 100BASE-TX EEE advertisement.
};
/// @brief Evidence for the last admitted DP83825I mode pass, separate from link readiness.
struct FunctionalModeSnapshot {
  FunctionalMode desired{FunctionalMode::RETAIN}; ///< RETAIN means this binding owns no mode.
  uint8_t completedWrites{0}; ///< Accepted table writes in order, including the final restart.
  bool nextWritePossible{false}; ///< First unconfirmed row could have affected silicon.
  bool restartCleared{false}; ///< Restart command cleared on a successful poll.
  bool established{false}; ///< Baseline, complete script and role observed after restart.
};
/// @brief DP83825I MMD3:0000[10] RX-clock policy during LPI.
enum class PcsRxClock : uint8_t {
  RETAIN, ///< Preserve current ownership.
  CONTINUOUS, ///< RX clock may not stop during LPI.
  STOPPABLE ///< Application permits RX clock to stop during LPI.
};
/// @brief Copied initialization request; NONE still performs part-specific startup verification.
struct InitializeOptions {
  ResetKind reset{ResetKind::PCS}; ///< Selected reset.
  FunctionalMode functionalMode{FunctionalMode::RETAIN}; ///< Explicit DP83825I selection requires HARD. EEE requires owned AN.
  PcsRxClock rxClock{PcsRxClock::RETAIN}; ///< Explicit ownership of 3:0000[10]; leader STOPPABLE needs board clock-loss approval.
};
/// @brief SNLS647I 8.3.14 paths; the historical MII name also admits RMII.
enum class LoopbackMode : uint8_t {
  MII, ///< BMCR 14 plus speed-specific BISCR companion.
  PCS_INPUT, ///< 10 Mbit/s PCS input path.
  PCS_OUTPUT, ///< PCS output path at 10 or 100 Mbit/s.
  DIGITAL, ///< 100/full exact companion recipe; hard-reset exit required.
  ANALOG, ///< Analog path with application-attested 100-ohm termination.
  REVERSE, ///< Live partner traffic returned to the line.
  EXTERNAL ///< Admitted external fixture, with internal paths disabled.
};
/// @brief Copied session request; no option authorizes general recipe-register access.
struct LoopbackOptions {
  LoopbackMode mode{LoopbackMode::MII}; ///< Requested path, never enabled implicitly.
  LinkSpeed speed{LinkSpeed::MBPS_100}; ///< Required 10/100 speed; internal paths use full duplex.
  bool transmitOnLine{false}; ///< Only MII 100; enable after BMCR 14, clear before disabling it (table 8-40).
  bool maintenanceAdmitted{false}; ///< Owner quiesced normal MAC/SDK traffic and admits disruption.
  bool termination100Ohm{false}; ///< Required for ANALOG and DP83825I REVERSE. The application supplies the terminations.
  bool partnerReady{false}; ///< Required for REVERSE; live link is also checked.
  bool externalFixture{false}; ///< Required for EXTERNAL; no internal path proof is implied.
  bool allowHardResetExit{false}; ///< Permits the DP83826 digital-loop HARD exit and strap resampling. DP83825I uses a regular stop.
  uint8_t reverseFifoDepth{0}; ///< REVERSE code 0/1/2/3 means 4/5/6/8 nibbles; otherwise zero.
};
/// @brief Retained session observations, not measured packet or electrical results.
struct LoopbackResult {
  LoopbackMode mode{LoopbackMode::MII}; ///< Selected internal/external path.
  LinkSpeed speed{LinkSpeed::UNKNOWN}; ///< Requested speed.
  bool controlsVerified{false}; ///< Complete activation readback, not traffic validation.
  bool stopped{false}; ///< Stop and all owned restoration checks completed.
  bool hardResetExit{false}; ///< Opaque digital companion state requires HARD.
  uint16_t rawPhysts{0}; ///< Latest independently consumed live status word.
  bool phystsValid{false}; ///< Raw status was returned; not a simultaneous snapshot.
};
/// @brief Passive session record retained across result consumption and application dwell.
struct LoopbackSnapshot {
  LoopbackOptions options{}; ///< Copied options of the most recently admitted session.
  OperationToken token{}; ///< Start token, distinct from each later stop token.
  bool controlsVerified{false}; ///< Activation completed; invalidation clears this evidence.
  bool temporaryMayBeActive{false}; ///< Hardware may remain overridden; stop/HARD required.
};
/// @brief All table 8-40 PRBS encodings; counter wrapping is independent.
enum class BistMode : uint8_t {
  OFF, ///< Generator and checker disabled; no traffic result.
  SINGLE_PACKET, ///< One constant-data packet; no checker.
  CHECKER, ///< Checker only, with application-provided stimulus.
  GENERATOR_CHECKER ///< Continuous pseudo-random generator and checker.
};
/// @brief Table 8-40 error-byte counter policy, not generator selection.
enum class BistCounterMode : uint8_t {
  SATURATE, ///< Counter stops at FF; the sample is then a lower bound.
  WRAP ///< Counter restarts at zero; any sample has possible unobserved wraps.
};
/// @brief Copied diagnostic settings; no implicit hardware fixture action.
struct BistOptions {
  BistMode mode{BistMode::GENERATOR_CHECKER}; ///< Exact hardware encoding.
  BistCounterMode counterMode{BistCounterMode::SATURATE}; ///< FF saturation or modulo wrapping.
  uint16_t packetBytes{1518}; ///< Table 8-46 raw field 0..2047; no minimum Ethernet-frame claim.
  uint16_t interPacketGapBytes{500}; ///< Table 8-45: multiples of four, 0..1020 bytes.
  uint64_t observationUs{1000}; ///< Minimum us after activation/complete acquisition; failed captures do not rearm it.
  bool useLoopback{false}; ///< Otherwise normal line operation, without claiming an external-loop fixture.
  LoopbackOptions loopback{}; ///< Maintenance/speed admission; optional DIGITAL, ANALOG or EXTERNAL only.
};
/// @brief Correlated external observations for this acquisition, not fixture availability.
struct BistObservation {
  bool singlePacketObserved{false}; ///< Owner actually observed this epoch's single transmitted packet.
  bool externalStimulusObserved{false}; ///< Owner actually supplied/correlated checker stimulus in this epoch.
};
/// @brief Evidence verdict; no enum claims signal integrity or application delivery.
enum class BistOutcome : uint8_t {
  INCONCLUSIVE, ///< Missing evidence or unresolved bist_result_semantics prevents a pass claim.
  ERRORS_OBSERVED, ///< Valid captured nonzero error byte, with required traffic/lock evidence.
  SINGLE_PACKET_OBSERVED, ///< Owner-correlated single packet; checker is disabled.
  DISABLED ///< Selected OFF; no test performed.
};
/// @brief Retained BIST capture; partial raw words survive aggregate failure.
struct BistResult {
  OperationToken session{}; ///< Start token; previous epochs cannot satisfy this result.
  BistMode mode{BistMode::OFF}; ///< Selected generator/checker behavior.
  BistOutcome outcome{BistOutcome::INCONCLUSIVE}; ///< Separate from operation transport status.
  uint16_t rawBefore{0}; ///< BISCR before snapshot command.
  uint16_t rawCounter{0}; ///< Full BICSR1 word after snapshot, including IPG.
  uint16_t rawAfter{0}; ///< BISCR after counter acquisition.
  bool beforeValid{false}; ///< Independent valid pre-capture status.
  bool counterValid{false}; ///< Independent valid post-command raw word.
  bool afterValid{false}; ///< Independent valid post-capture status.
  bool measurementValid{false}; ///< Complete current acquisition, not an exact interval or PASS.
  bool activityObserved{false}; ///< At least one current-session generator-active observation.
  bool lockObserved{false}; ///< Checker enabled and both acquisition bookends show lock.
  bool syncLossObserved{false}; ///< Any current-session collector read observed loss; not an invented RC interval.
  uint8_t errorBytes{0}; ///< Eight-bit captured sample; meaningful only with counterValid.
  bool saturated{false}; ///< SATURATE mode and FF: count is a lower bound.
  bool wrapPossible{false}; ///< WRAP mode cannot prove an exact total, even for zero.
  bool intervalUncertain{true}; ///< Repeat-latch/lock-history source gate remains open.
  bool historyLost{false}; ///< Capture/clear may have happened without a valid returned sample.
  bool timeValid{false}; ///< Capture upper timestamp valid; zero is permitted.
  uint64_t observedAtUs{0}; ///< Capture completion upper bound, microseconds.
};
/// @brief Passive session and latest complete-or-partial acquisition evidence.
struct BistSnapshot {
  BistOptions options{}; ///< Copied start options.
  OperationToken token{}; ///< Session epoch, retained after stop for historical rendering.
  BistResult measurement{}; ///< Most recent acquisition; partial validity is explicit.
  bool active{false}; ///< Typed activation succeeded and has not been stopped/invalidated.
  bool stale{true}; ///< Measurement is historical or incomplete.
  bool temporaryMayBeActive{false}; ///< Hardware may need the owning stop or HARD restoration.
};
/// @brief Exact source recipe or one independently tuned raw measurement.
enum class CableRecipe : uint8_t {
  SNLA428, ///< Selected exact-part six-run recipe, with transmit A and capture A.
  RAW ///< One caller-configured capture without derived distance.
};
/// @brief Table 8-54 transmitter/capture channel selections.
enum class CableChannel : uint8_t {
  A_TO_B, ///< Transmit A, capture B.
  B_TO_A, ///< Transmit B, capture A.
  A_TO_A, ///< Transmit A, capture A.
  B_TO_B ///< Transmit B, capture B.
};
/// @brief Fixed row identity; ordering is SNLA428 table 2-1, never a distance bin.
enum class CableRun : uint8_t {
  RUN_1A, ///< First segment-1 tuning; also the RAW result slot.
  RUN_1B, ///< Second segment-1 tuning.
  RUN_2, ///< Segment-2 tuning.
  RUN_3, ///< Segment-3 tuning.
  RUN_4, ///< Segment-4 tuning.
  RUN_5 ///< Segment-5 tuning.
};
/// @brief Raw controls from SNLS647I tables 8-54/56..60, preserving reserved bits.
struct CableControls {
  CableChannel channel{CableChannel::A_TO_A}; ///< Transmit/capture combination.
  bool dcNoInit{true}; ///< CDSCR 12 DC-removal no-initialization policy.
  uint8_t averaging{4}; ///< CDSCR code 0..6 means 1..64; 7 is reserved.
  uint8_t segment{1}; ///< Documented code 1..5; not measured length.
  uint8_t duration{13}; ///< CDSCR3 high byte, raw code 0..255.
  uint8_t initialSkip{7}; ///< CDSCR3 low byte, raw code 0..255.
  uint8_t shadowAverage{2}; ///< TDR175 code 0..7; no inferred time unit.
  uint8_t forwardShadow{4}; ///< TDR175 code 0..15.
  uint8_t locationThreshold{0}; ///< TDR176 code 0..31.
  uint8_t shortThreshold{0}; ///< CDSCR4 code 0..31.
  uint8_t pulseWidth{2}; ///< TDR178 code 0..7.
};
/// @brief Copied manual plan with explicit disruptive-maintenance admission.
struct CableSettings {
  CableRecipe recipe{CableRecipe::SNLA428}; ///< Exact six-run recipe or raw single capture.
  uint8_t runMask{0x3F}; ///< SNLA428 subset bits in CableRun order; RAW requires 1.
  CableControls controls{}; ///< Used only for RAW; SNLA428 owns its exact controls.
  bool maintenanceAdmitted{false}; ///< Normal MAC/EtherCAT traffic is quiesced.
  bool peerQuiet{false}; ///< Owner disconnected or silenced the peer; link-down is also checked.
  bool allowHardResetExit{false}; ///< Owner accepts HARD effects on unowned state and straps.
};
/// @brief One non-destructive raw word; failed reads retain older data as stale.
struct CableWord {
  uint16_t raw{0}; ///< Full returned word, including unclassified bits.
  bool valid{false}; ///< At least one sound read returned data.
  bool stale{true}; ///< Latest attempted read failed or its generation changed.
  bool timeValid{false}; ///< Completion upper timestamp is valid, including zero.
  uint64_t observedAtUs{0}; ///< Microseconds; never guessed after clock failure.
};
/// @brief Explicit per-row progress; skipped rows are never zero measurements.
enum class CableRunState : uint8_t {
  NOT_RUN, ///< No trigger attempted for this row.
  STARTED, ///< Trigger attempted; complete correlated data not yet retained.
  COMPLETE ///< All result words retained; inspect fail/freshness, and autorun remains uncorrelated.
};
/// @brief Retained current-run evidence, separate from operation transport status.
struct CableSample {
  CableRunState state{CableRunState::NOT_RUN}; ///< Request progress, not cable quality.
  CableWord cdcr{}; ///< Full CDCR completion/fail word.
  CableWord index{}; ///< Full 0180 location word.
  CableWord amplitude{}; ///< Full 0185 amplitude word.
  CableWord peak{}; ///< Full 018A peak word.
  bool triggerAccepted{false}; ///< Trigger frame completed successfully.
  bool triggerPossible{false}; ///< Trigger may have affected hardware despite error.
  bool freshnessObserved{false}; ///< Post-trigger start 1 or done 0 was actually observed.
  bool failed{false}; ///< CDCR fail bit; captured data remains raw evidence.
};
/// @brief Fixed retained capture plan; raw data has no hardware-history-loss semantics.
struct CableResult {
  OperationToken session{}; ///< Original operation/session token, never a silicon generation counter.
  CableRecipe recipe{CableRecipe::SNLA428}; ///< Interpretation provenance.
  uint8_t requestedMask{0}; ///< Selected rows; unselected rows remain NOT_RUN.
  std::array<CableSample, 6> samples{}; ///< Fixed bounded storage in CableRun order.
  CableWord prerequisiteCdcr{}; ///< Live preflight/stop status, including calibration rejection.
  bool complete{false}; ///< All requested words returned; autorun never proves a coherent generation.
  bool stale{true}; ///< Incomplete or reset/external invalidation made current interpretation unavailable.
  bool stopped{false}; ///< HARD replay and saved-field verification completed.
  bool autorun{false}; ///< One raw hardware-autorun observation with unknown freshness/coherence.
  bool statusChanged{false}; ///< CDCR bookends differed; equal bookends cannot prove atomic capture.
};
/// @brief The sole hardware autorun control plus explicit disruptive-maintenance admission.
struct CableAutorunOptions {
  bool enabled{false}; ///< CR1[8] policy; does not trigger a test or attest engine idle.
  bool maintenanceAdmitted{false}; ///< Owner quiesced normal application activity as required.
};
/// @brief Passive settings, capture and outstanding effects.
struct CableSnapshot {
  CableSettings settings{}; ///< Copied manual request, meaningful only when settingsValid.
  bool settingsValid{false}; ///< Autorun has no captured tuning/recipe context; defaults are not evidence.
  CableResult result{}; ///< Partial or historical evidence without additional I/O.
  bool temporaryMayBeActive{false}; ///< Owning stop/HARD may still be required.
};
/// @brief SNLA428-only source-implied classification, not physical qualification.
enum class CableConclusion : uint8_t {
  NO_PEAK, ///< No qualifying peak; no claim of cable health or distance.
  OPEN, ///< Source-implied open on the selected transmit-A path.
  SHORT ///< Source-implied short on the selected transmit-A path.
};
/// @brief Pure derived result for transmit-A only; never describes pair B or the whole cable.
struct CableDiagnosis {
  CableConclusion conclusion{CableConclusion::NO_PEAK}; ///< No qualifying peak is not a good cable.
  CableRun run{CableRun::RUN_1A}; ///< Meaningful only with distanceValid.
  double metres{0}; ///< Source-implied value, not calibrated accuracy.
  bool distanceValid{false}; ///< False for NO_PEAK; zero numeric default is not a distance.
  bool amplitudeAtLimit{false}; ///< Raw 7F; saturation behavior is not inferred.
  bool rawBit11{false}; ///< Raw 018A bit 11 without invented polarity.
  bool rawBit5{false}; ///< Raw 018A bit 5 without invented polarity.
};
/// @brief Validate settings without I/O; source tables 8-54..60 and SNLA428 table 2-1.
/// @param[in] settings Complete request, including maintenance/peer/HARD admission.
/// @return OK or INVALID_PARAM/INVALID_CONFIG; no state changes.
Status validateCableSettings(const CableSettings& settings) noexcept;
/// @brief Independently derived SNLA428 table 2-5 helper over complete retained samples.
/// @param[in] result Fresh complete selected-recipe capture; failed/incomplete/raw plans reject.
/// @param[in] propagationDelayNsPerM Positive finite ns/m; Cat5/5e5.35,Cat6 5,Cat7 4.6 are note examples.
/// @param[out] diagnosis Unchanged on failure; NO_PEAK has no valid distance.
/// @return OK or INVALID_PARAM/INVALID_CONFIG/RESULT_NOT_AVAILABLE. Pure, bounded, Q7 unqualified.
Status interpretCable(const CableResult& result, double propagationDelayNsPerM, CableDiagnosis& diagnosis) noexcept;
/// @brief Fixed maximum number of local or retained partner next pages.
constexpr uint8_t MAX_NEXT_PAGES = 16;
/// @brief Clause 28 page content; NP and the read-only toggle are driver/hardware owned.
struct NextPage {
  uint16_t data{0}; ///< Eleven-bit message code or unformatted payload.
  bool message{false}; ///< True for a message page; false for its unformatted data.
  bool acknowledge2{false}; ///< Can/will comply with the partner message, not receipt acknowledgement.
};
/// @brief Copied finite exchange. Supported messages: null, remote fault, OUI and PHY ID.
struct NextPageExchange {
  std::array<NextPage, MAX_NEXT_PAGES> pages{}; ///< Ordered message and associated unformatted pages.
  uint8_t pageCount{0}; ///< Zero disables exchange. Nonzero includes every unformatted page.
  uint8_t maxPartnerPages{MAX_NEXT_PAGES}; ///< Positive bound, at most MAX_NEXT_PAGES; includes null pages.
};
/// @brief Profile application policy; all waits share the operation's finite budget.
struct ConfigureOptions {
  bool waitForCompletion{false}; ///< Observe BMSR AN-complete, independently of register verification.
  NextPageExchange nextPages{}; ///< Nonempty exchange always waits for AN-complete.
};
/// @brief Validate a complete link profile and negotiation policy with no I/O.
/// @param[in] settings Desired link settings.
/// @param[in] options Negotiation wait and ordered next-page policy.
/// @return OK for legal input or a syntax/combination error. Use the variant overload for part constraints.
/// @note Runtime admission, device evidence and deadlines are checked by the owner.
Status validateConfiguration(const LinkSettings& settings, const ConfigureOptions& options) noexcept;
/// @brief Validate link, next pages and the exact declared part before starting an operation.
/// @param[in] variant Application-declared ordering part.
/// @param[in] settings Desired link settings.
/// @param[in] options Negotiation wait and next-page policy.
/// @return OK or the local admission error, without I/O, clock access or state changes.
Status validateConfiguration(Variant variant, const LinkSettings& settings, const ConfigureOptions& options) noexcept;
/// @brief Current foundational configuration evidence, separate from desired intent.
enum class ConfigurationTrust : uint8_t {
  UNKNOWN, ///< No established configuration evidence.
  VERIFIED, ///< Complete currently owned stable settings verified.
  DIRTY ///< Admitted desired change or uncertainty needs verification/restoration.
};
/// @brief Explicit software power request, never inferred from the power pin.
enum class PowerRequest : uint8_t {
  NORMAL, ///< Clear software power-down once; a pin can still block wake.
  IEEE_POWER_DOWN, ///< Assert BMCR 11; no separate actual-state register exists.
  DEEP_POWER_DOWN ///< DP83825I IEEE down then 0428[2], requiring an independent recovery route.
};
/// @brief Complete opt-in policy. SNLS647I 8.3.6 and tables 8-19/35/40.
struct PowerSettings {
  PowerRequest request{PowerRequest::NORMAL}; ///< Software intent, distinct from BMCR's software/pin OR.
  bool activeSleep{false}; ///< PHYSCR 14:12=110; successful policy need not enter sleep.
  bool disablePll{false}; ///< PHYSCR 15, allowed only with IEEE_POWER_DOWN.
  bool maintenanceAdmitted{false}; ///< Application permits link/power disruption.
  bool clockLossApproved{false}; ///< Application permits loss of PHY-generated clocks.
  bool managementClockAvailable{false}; ///< Board guarantees the reference/MDIO access needed through transition.
  bool recoveryAvailable{false}; ///< Board can recover the PHY and its reference if management access is lost.
};
/// @brief Read indications only, not measured clock, supply or partner readiness.
enum class PowerState : uint8_t {
  UNKNOWN, ///< Incomplete or stale observation.
  NORMAL_INDICATED, ///< BMCR 11=0 and BISCR 8=1; no clock/partner qualification.
  ACTIVE_SLEEP_INDICATED, ///< BMCR 11=0 and BISCR 8=0 under enabled active-sleep policy.
  POWER_DOWN_INDICATED, ///< BMCR 11=1, a software/pin OR rather than actual-state proof.
  WAKE_BLOCKED_OR_UNCONFIRMED, ///< Owner requested normal but BMCR 11 remains high.
  DEEP_POWER_DOWN_INDICATED ///< DP83825I down controls read back asserted. Physical power requires independent evidence.
};
/// @brief Passive power intent and independently valid raw evidence.
struct PowerSnapshot {
  PowerSettings desired{}; ///< Last admitted request, retained on failure/cancellation.
  bool desiredValid{false}; ///< Explicit policy ownership; no default power activation.
  bool requestedPowerDown{false}; ///< Owner software intent, never sampled BMCR 11.
  bool observedBmcrPowerDown{false}; ///< Hardware software/pin OR when bmcrValid.
  uint16_t rawBmcr{0}; ///< Latest returned BMCR.
  uint16_t rawPhyscr{0}; ///< Latest returned PHYSCR.
  uint16_t rawBiscr{0}; ///< Latest collector-routed BISCR.
  uint16_t rawDeep{0}; ///< DP83825I vendor 0428 observed control word.
  bool deepValid{false}; ///< The deep control read returned a valid word.
  bool deepWriteAttempted{false}; ///< Latest power pass attempted its 0428 target write.
  bool deepWriteCompleted{false}; ///< The controller confirmed completion of that target.
  bool deepWritePossible{false}; ///< The 0428 target could have changed despite an error.
  bool bmcrValid{false}; ///< Raw BMCR validity, including partial acquisitions.
  bool physcrValid{false}; ///< Raw PHYSCR validity.
  bool biscrValid{false}; ///< Raw BISCR validity.
  bool observedStale{true}; ///< Aggregate incomplete or invalidated; raw words remain historical.
  bool timeValid{false}; ///< Complete observation timestamp validity.
  uint64_t observedAtUs{0}; ///< Conservative completion upper bound in microseconds.
  PowerState state{PowerState::UNKNOWN}; ///< Current indication only when not stale.
  ConfigurationTrust trust{ConfigurationTrust::UNKNOWN}; ///< Complete owned control verification, separate from physical state.
  Status lastStatus{Err::RESULT_NOT_AVAILABLE,0,"No power observation"}; ///< Last operation/acquisition outcome.
};
/// @brief Passive persistent policy evidence, separate from trigger and capture freshness.
struct CableAutorunSnapshot {
  bool desired{false}; ///< Last admitted enable intent.
  bool desiredValid{false}; ///< This binding explicitly owns policy only after admission.
  bool observed{false}; ///< Last independently returned CR1[8] value.
  bool observedValid{false}; ///< A policy readback returned valid data.
  bool observedStale{true}; ///< Reset/external change or incomplete verification invalidated trust.
  ConfigurationTrust trust{ConfigurationTrust::UNKNOWN}; ///< Owned policy only, not the link profile.
  uint64_t observedAtUs{0}; ///< Conservative readback upper timestamp in microseconds.
  bool timeValid{false}; ///< Timestamp zero is valid when true.
  Status lastStatus{Err::RESULT_NOT_AVAILABLE, 0, "No autorun policy verification"}; ///< Last policy outcome.
};
/// @brief Selected interrupt output route; mux writes belong to the later pin owner.
enum class InterruptPin : uint8_t {
  INTR_PWRDN, ///< Pin 21: BASIC INT or ENHANCED direction selected by OE.
  LED1, ///< Requires live mux 0304=4; ENHANCED also requires GPIO availability.
  LED2, ///< ENHANCED only. MII requires a released COL consumer.
  LED0, ///< A generation. Requires pin 30 released by the clock owner.
  LED3 ///< A generation, ENHANCED. MII requires a released CRS consumer.
};
/// @brief Return the LED mux index for a routed interrupt.
/// @param[in] pin Selected interrupt output.
/// @return LED index 0 through 3, or 4 for the dedicated interrupt pin.
inline constexpr uint8_t ledIndex(InterruptPin pin) noexcept {
  switch (pin) {
    case InterruptPin::LED0: return 0;
    case InterruptPin::LED1: return 1;
    case InterruptPin::LED2: return 2;
    case InterruptPin::LED3: return 3;
    default: return 4;
  }
}
/// @brief Complete opt-in policy; masks use named cmd::MASK_MISR*_ENABLE fields.
/// @note SNLS647I tables 8-35..37/74..78/91. Board declarations are permissions,
/// not measurements. No mux, GPIO, MCU ISR or scheduler is configured here.
struct InterruptSettings {
  uint16_t misr1Mask{0}; ///< Eight enable bits; reject bits outside 00FF before I/O.
  uint16_t misr2Mask{0}; ///< Eight enable bits, distinct source map from MISR1.
  InterruptSharedSource sharedSource{InterruptSharedSource::WAKE}; ///< POLARITY or WAKE; UNKNOWN is not a setting.
  InterruptPin pin{InterruptPin::INTR_PWRDN}; ///< Pin route to verify before mutation.
  bool activeLow{true}; ///< PHYSCR 3; external electrical levels remain unqualified.
  bool globalEnable{false}; ///< PHYSCR 1; disabled delivery does not clear evidence.
  bool outputEnable{true}; ///< PHYSCR 0 in ENHANCED; ignored/preserved in BASIC.
  bool maintenanceAdmitted{false}; ///< Application admits disruptive pin/mask changes.
  bool pinAvailable{false}; ///< Application owns selected route AND pin 21's role; alternate mux does not disconnect pin 21.
  bool led1ClockUnused{false}; ///< Required board declaration for ENHANCED LED1.
  bool colUnused{false}; ///< Required board declaration for ENHANCED MII LED2.
  bool powerDownInputSafe{false}; ///< Required whenever ENHANCED OE=0 is requested.
  bool crsUnused{false}; ///< Board releases MII CRS for alternate use of A pin 29.
};
/// @brief Passive interrupt intent/readback and test-stimulus evidence.
struct InterruptSnapshot {
  InterruptSettings desired{}; ///< Last admitted complete policy; replayed only if owned.
  bool desiredValid{false}; ///< Explicit configuration established policy ownership.
  ConfigurationTrust trust{ConfigurationTrust::UNKNOWN}; ///< Complete stable policy verification only.
  uint16_t rawPhyscr{0}; ///< Latest independently read control word.
  uint16_t rawMisr1{0}; ///< Latest policy readback, including consumed events.
  uint16_t rawMisr2{0}; ///< Latest policy readback, including consumed events.
  uint16_t rawRxfs{0}; ///< Latest policy readback, including consumed filter status.
  bool physcrValid{false}; ///< A valid PHYSCR observation exists.
  bool misr1Valid{false}; ///< A valid MISR1 policy readback exists.
  bool misr2Valid{false}; ///< A valid MISR2 policy readback exists.
  bool rxfsValid{false}; ///< A valid RXFS policy readback exists.
  bool observedStale{true}; ///< Incomplete verification or external/reset uncertainty.
  bool timeValid{false}; ///< Complete verification timestamp validity.
  uint64_t observedAtUs{0}; ///< Microsecond upper bound; zero is valid.
  bool basicStrapInterrupt{false}; ///< Observed BASIC mode gives pin 21 its INT role; OE has no direction meaning.
  InterruptSharedSource selector{InterruptSharedSource::UNKNOWN}; ///< Current attribution context; unknown through transitions.
  bool testMayBeActive{false}; ///< Accepted/uncertain stimulus until explicit clear or reset restoration.
  Status lastStatus{Err::RESULT_NOT_AVAILABLE,0,"No interrupt configuration"}; ///< Last policy/stimulus outcome.
};
/// @brief Bounds for a single owner service pass, never an unlimited drain.
struct InterruptServiceBudget {
  IoBudget io{}; ///< Aggregate exclusive deadline and physical-frame allowance.
  uint8_t maxTransactions{2}; ///< Maximum callbacks; plan needs one, optionally a second for counters.
  uint8_t maxSamples{5}; ///< Maximum consumed words; first PHYSTS/MISR1/MISR2 need three.
};
/// @brief Last complete IRQ service, separate from immediate per-source collector history.
struct InterruptServiceSnapshot {
  bool valid{false}; ///< A complete software acquisition exists, never simultaneous silicon state.
  bool stale{true}; ///< Another read, failure or invalidation makes the stored aggregate historical.
  bool morePossiblyPending{true}; ///< Nonzero event bytes or incomplete/budget-limited work require owner rescheduling.
  uint16_t rawPhysts{0}; ///< Mirrors read before MISRs; never used to skip them.
  EvidenceSample misr1{}; ///< Latest word plus sample identity and retained history.
  EvidenceSample misr2{}; ///< Latest word plus sample identity, history and selector context.
  EvidenceSample fcscr{}; ///< Optional half-full sample and qualified lower-bound accumulation.
  EvidenceSample recr{}; ///< Optional raw half-full sample; arithmetic remains gated.
  bool fcscrRequested{false}; ///< Current or retained FCSCR request serviced in this pass.
  bool recrRequested{false}; ///< Current or retained RECR request serviced in this pass.
  Status lastStatus{Err::RESULT_NOT_AVAILABLE,0,"No interrupt service"}; ///< Failed refresh leaves cached values stale.
};
/// @brief Passive profile evidence. VERIFIED covers active advertisement/mode, routing, extension and part-specific startup requirements.
struct SettingsSnapshot {
  LinkSettings desired{}; ///< Authoritative intent copied at successful start admission.
  bool desiredValid{false}; ///< Desired intent exists; cancellation does not erase it.
  LinkSettings observed{}; ///< Latest complete stable setting tuple.
  bool observedValid{false}; ///< Complete tuple was observed.
  bool observedStale{true}; ///< Invalidation or incomplete verification makes history stale.
  ConfigurationTrust trust{ConfigurationTrust::UNKNOWN}; ///< Never implies link or MAC readiness.
  uint64_t observedAtUs{0}; ///< Conservative complete readback time, microseconds.
  bool timeValid{false}; ///< Timestamp is usable.
  Status lastVerificationStatus{Err::NOT_INITIALIZED, 0, "No settings verification"}; ///< Latest verification outcome.
  uint16_t rawAnar{0}; ///< Latest advertisement word; historical and inactive during forced mode.
  uint16_t rawBmcr{0}; ///< Latest independently returned control word.
  bool anarValid{false}; ///< A raw advertisement was captured; forced operation does not refresh it.
  bool bmcrValid{false}; ///< Raw control has been captured.
  uint16_t rawCr1{0}; ///< Latest robust-routing control readback.
  uint16_t rawPhycr{0}; ///< Latest automatic/manual-routing readback.
  uint16_t rawCr3{0}; ///< Latest coupled-mirror readback.
  uint16_t rawCr2{0}; ///< Latest extended-duplex readback.
  bool cr1Valid{false}; ///< Raw CR1 has been captured.
  bool phycrValid{false}; ///< Raw PHYCR has been captured.
  bool cr3Valid{false}; ///< Raw CR3 has been captured.
  bool cr2Valid{false}; ///< Raw CR2 has been captured.
  uint16_t rawVod{0}; ///< Latest independently returned mandatory VOD_CFG3 word.
  bool vodValid{false}; ///< Raw VOD was captured; aggregate trust still requires a complete pass.
};
/// @brief Independently consumed partner page, retained even if later work fails.
struct PageEvidence {
  uint16_t raw{0}; ///< Raw partner next-page word, including ACK and toggle.
  bool valid{false}; ///< Valid data returned by the controller.
  uint64_t observedAtUs{0}; ///< Conservative completion timestamp, microseconds.
  bool timeValid{false}; ///< Timestamp is usable.
};
/// @brief Finite exchange evidence; successful application alone is not AN completion.
struct NextPageResult {
  bool requested{false}; ///< An exchange epoch was requested.
  bool baseReceived{false}; ///< Fresh base-page event consumed; payload validity is separate.
  bool basePageValid{false}; ///< Base payload was returned after the event.
  bool pagePayloadMissing{false}; ///< Consumed or possibly consumed event lacks a trusted payload.
  bool partnerRefused{false}; ///< Partner did not advertise next-page capability.
  bool complete{false}; ///< Page service and AN-complete were observed.
  bool npMayRemainSet{false}; ///< Failed/cancelled epoch may still oblige page service.
  uint8_t transmittedPages{0}; ///< Completed page-load writes, including null pages and later release/deadline failures.
  uint8_t receivedPages{0}; ///< Consumed next-page events, including duplicate/missing payload slots.
  uint32_t duplicatePages{0}; ///< Repeated toggle evidence; never advances local page index.
  uint16_t basePage{0}; ///< Raw base page from ANLPAR, not ANLNPTR.
  std::array<PageEvidence, MAX_NEXT_PAGES> pages{}; ///< Consumed event slots; invalid payloads remain explicit.
};
/// @brief Raw captured straps, never assumed pin levels or decoded BASIC/ENHANCED defaults.
struct StrapEvidence {
  /// @brief Return captured A Strap11 (pin 19). Use a current valid SOR1 sample.
  /// @return True when SOR1 bit 11 was high.
  bool strap11High() const noexcept { return (sor1 & 0x0800U) != 0U; }
  uint16_t sor1{0}; ///< Latest returned SOR1 word, subject to sor1Valid.
  uint16_t sor2{0}; ///< Latest returned SOR2 word, subject to sor2Valid.
  bool sor1Valid{false}; ///< A valid SOR1 word has been captured.
  bool sor2Valid{false}; ///< A valid SOR2 word has been captured.
  uint64_t observedAtUs{0}; ///< Conservative stamp after complete current capture.
  bool timeValid{false}; ///< Complete-capture timestamp is usable.
  bool stale{true}; ///< Reset or partial recapture can mix historical/current words.
  uint16_t rcsr{0}; ///< DP83825I direct RCSR sample, including consumed FIFO indications.
  bool rcsrValid{false}; ///< RCSR was captured; a stale operation result can retain the pre-reset word.
};

/// @brief DP83822 settings sampled at strap latch-in (SNLS505H tables 7-10..13).
struct Dp83822Straps {
  uint8_t phyAddress{0}; ///< Address 0..31 selected by COL and RX_D0..3.
  Medium medium{Medium::COPPER}; ///< COL FX_EN selection. Live medium requires CR2[14].
  MacInterface interface{MacInterface::UNKNOWN}; ///< UNKNOWN for a reserved XI_50/interface combination.
  RmiiClock rmiiClock{RmiiClock::UNSPECIFIED}; ///< Clock role for a strapped RMII interface only.
  bool autonegotiation{false}; ///< Copper AN_EN strap. FX always selects forced operation.
  uint8_t technologyMask{0}; ///< Advertised LINK_* bits or a single forced speed/duplex bit. Zero for LED_0 test modes.
  bool autoMdix{false}; ///< RX_ER selected automatic crossover on copper.
  bool signalDetect{false}; ///< RX_ER selected the LED_1 signal-detect function with FX_EN set.
  bool eeeEnabled{false}; ///< RX_D1 EEE_EN strap value.
  bool fastLinkDrop{false}; ///< RX_D2 FLD_EN strap value.
  bool led0TestMode{false}; ///< LED_0 was strapped in prohibited mode 2 or 3. LED_1 requires board evidence.
  uint8_t crsMode{0}; ///< CRS strap mode 1..4 selecting LED behavior in table 7-12.
};

/// @brief Decode a pair of DP83822 strap latches without MDIO or clock access.
/// @param[in] capture DEVAD 31 offsets 0467h and 0468h from the same DP83822 capture epoch.
/// @param[out] output Latched selections on success. A reserved interface and an observed LED_0 test mode are reported independently.
/// @return True for two valid words in a non-stale capture. On false the output retains its prior value.
/// @note The caller identifies the part. Live configuration, board wiring and LED_1 strap legality require separate evidence.
bool decodeDp83822Straps(const StrapEvidence& capture, Dp83822Straps& output) noexcept;

/// @brief First failed stable-bit comparison, retained even if the pass is interrupted.
struct ConfigurationMismatch {
  bool valid{false}; ///< Remaining fields describe an actual comparison.
  bool mmd{false}; ///< True for an indirect target.
  uint8_t devad{0}; ///< MMD selector when mmd is true.
  uint16_t registerAddress{0}; ///< C22 register or MMD wire offset.
  uint16_t mask{0}; ///< Verified stable bits only.
  uint16_t expected{0}; ///< Expected masked value.
  uint16_t observed{0}; ///< Actual returned word, including non-owned bits.
};
/// @brief Last operation batch, or its first failing batch; raw receipt stays in transportSnapshot.
struct BatchSummary {
  OperationPhase phase{OperationPhase::NONE}; ///< Phase that submitted the batch.
  uint8_t attemptedFrames{0}; ///< Valid attempted prefix count.
  uint8_t completedFrames{0}; ///< Independently completed prefix count.
  bool effectPossible{false}; ///< At least one frame may have affected hardware.
  Status normalized{Err::RESULT_NOT_AVAILABLE, 0, "No batch"}; ///< Primary operation-call outcome.
  Status release{Err::RESULT_NOT_AVAILABLE, 0, "No batch"}; ///< Normalized release outcome.
  TransportViolation violation{TransportViolation::NONE}; ///< Callback contract violation.
  bool late{false}; ///< Raw post-return clock reached the exclusive deadline.
};
/// @brief Exact LED/GPIO role; unsupported pin-specific choices reject before I/O.
enum class PinFunction : uint8_t {
  UNOWNED, ///< Leave this pin to its existing owner; does not undo its physical mux.
  LED, ///< Normal LED source with optional documented force controls.
  WAKE, ///< Matching existing WoL route only; source configuration remains with WoL.
  INTERRUPT, ///< Matching existing IRQ pin only; masks and source remain with IRQ.
  COLLISION3, ///< LED2 table 8-76 COL code 3, MII only.
  COLLISION5, ///< LED2 table 8-76 COL code 5, MII only.
  COLLISION6, ///< LED2 table 8-76 COL code 6, MII only.
  HIGH, ///< Constant high, mux 6 on DP83825I pin 2 or mux 7 on supported DP83826 pins.
  TX_ERROR, ///< Pin 31 figure/table contradiction; source-gated, never guessed.
  CLOCK, ///< Clock selection belongs to the interface owner.
  LOW3, ///< Constant low through documented mux code 3.
  LOW5, ///< Constant low through documented mux code 5.
  LOW6, ///< Constant low through documented mux code 6.
  CARRIER_SENSE3, ///< MII CRS on A pin 29 through mux code 3.
  CARRIER_SENSE5, ///< MII CRS on A pin 29 through mux code 5.
  CARRIER_SENSE6, ///< MII CRS on A pin 29 through mux code 6.
  LOW7 ///< Constant low on DP83825I pin 2 through mux code 7.
};
/// @brief Source encodings from SNLS647I tables 8-49/85; reserved 15 is rejected.
enum class LedSource : uint8_t {
  LINK, ///< Link indication.
  RX_TX, ///< Receive or transmit activity.
  TX, ///< Transmit activity.
  RX, ///< Receive activity.
  COLLISION, ///< Collision indication.
  SPEED100, ///< 100Mbps speed indication.
  SPEED10, ///< 10Mbps speed indication.
  FULL_DUPLEX, ///< Full-duplex indication.
  LINK_ACTIVITY, ///< Combined link/activity.
  STRETCH, ///< Active pulse-stretch indication.
  AN100_FULL, ///< 100Mbps full-duplex autonegotiation indication.
  LPI, ///< Low-power-idle indication, not a readiness guarantee.
  MII_ERROR, ///< MII error indication.
  LINK_LOST, ///< Latched link loss; collector BMSR reads clear the indication.
  PRBS_ERROR ///< PRBS error indication; BIST counter clearing affects it.
};
/// @brief Polarity ownership; BASIC admits only PRESERVE under the source conflict.
enum class LedPolarity : uint8_t {
  PRESERVE, ///< Keep the reset-loaded/current polarity without owning it.
  ACTIVE_LOW, ///< Own the documented polarity field as active-low.
  ACTIVE_HIGH ///< Own the documented polarity field as active-high.
};
/// @brief Output override on LED0 through LED2 and on A-generation LED3.
enum class LedDrive : uint8_t {
  NORMAL, ///< Source-driven output.
  LOW, ///< Force physical low using the documented drive override.
  HIGH ///< Force physical high using the documented drive override.
};
/// @brief One pin's source-backed settings; no independent object or bus owner.
struct PinSetting {
  PinFunction function{PinFunction::UNOWNED}; ///< Selected role, validated against the exact mux table.
  LedSource source{LedSource::LINK}; ///< Used only with LED; shared collector owns latch acknowledgements.
  LedPolarity polarity{LedPolarity::PRESERVE}; ///< Optional polarity override where the selected pin implements it.
  LedDrive drive{LedDrive::NORMAL}; ///< Physical low/high override where the selected pin implements it.
};
/// @brief Complete owned pin profile; untouched pins and globals are preserved.
struct PinSettings {
  PinSetting pins[4]{}; ///< Array index is LED0..3, not physical package pin number.
  bool blinkOwned{false}; ///< Own common LEDCR blink period when true.
  uint8_t blinkCode{2}; ///< Codes 0..3 mean 50/100/200/500ms; no measured timing claim.
  bool stretchOwned{false}; ///< Own common PHYCR pulse-stretch bypass bit.
  bool stretchBypass{false}; ///< True bypasses pulse stretching.
  bool led0SwapOwned{false}; ///< Own separate MLEDCR polarity-swap bit, ENHANCED only.
  bool led0Swap{false}; ///< Invert LED0 source polarity when owned; separate from drive polarity.
  bool led1ClockOverride{false}; ///< Own original ENHANCED IO_CFG1[6]. Reserved on A.
  bool maintenanceAdmitted{false}; ///< Owner permits disruptive pin reconfiguration.
  bool macStopped{false}; ///< Owner stopped MAC use while changing interface-shared pins.
  bool outputsQuiesced{false}; ///< Owner admits output transitions; no silicon glitch-free promise.
  uint8_t availableMask{0}; ///< Bits 0..3 release board consumers; LED2/3 also require matching live RCSR mode.
  bool led1ClockUnused{false}; ///< No consumer depends on LED1/CLKOUT during this profile.
  bool colUnused{false}; ///< Required for non-COL roles on ENHANCED MII LED2.
  bool crsUnused{false}; ///< Board releases MII CRS for alternate use of A pin 29.
};
/// @brief Pin intent and stable register evidence, never measured electrical output.
struct PinSnapshot {
  PinSettings desired{}; ///< Last admitted profile; remains available after failure.
  bool desiredValid{false}; ///< Explicit profile ownership has been established.
  ConfigurationTrust trust{ConfigurationTrust::UNKNOWN}; ///< Verification of owned stable masks only.
  bool observedStale{true}; ///< Failed, partial or invalidated acquisition.
  bool timeValid{false}; ///< Completion timestamp validity; zero may be valid.
  uint64_t observedAtUs{0}; ///< Conservative completion upper bound, microseconds.
  uint16_t raw[10]{}; ///< LEDCR, PHYCR, MLEDCR, LEDCFG, LEDCFG2, IO_CFG1, GPIO0..3 in that order.
  uint16_t validMask{0}; ///< Bits 0..9 identify successfully retained words; stale values remain historical.
  Status lastStatus{Err::RESULT_NOT_AVAILABLE,0,"No pin observation"}; ///< Last apply/replay result.
};
/// @brief One bounded direct pin application; restoration uses a separate initialize result.
struct PinResult {
  Status status{Err::RESULT_NOT_AVAILABLE,0,"No pin result"}; ///< Terminal logical result.
  Status primary{}; ///< Last terminal backend primary status, separate from release.
  Status release{}; ///< Last terminal ownership-release status.
  uint32_t attemptedFrames{0}; ///< Actual attempted C22 frames, including indirect addressing.
  uint32_t completedFrames{0}; ///< Valid contiguous completed frames.
  uint32_t transactions{0}; ///< Invoked terminal batch callbacks; no retries.
  bool mutationAttempted{false}; ///< At least one owned target write was attempted.
  bool mutationPossible{false}; ///< An accepted/uncertain target may have changed hardware.
  bool verified{false}; ///< Every admitted stable owned mask was read back.
  ConfigurationMismatch mismatch{}; ///< First comparison failure, independent of transport status.
  PinSnapshot pins{}; ///< Retained partial/current evidence at return.
};

/// @brief Clock/LED1 route ownership; uncertain selector paths stay rejected.
enum class ClockOutput : uint8_t {
  UNOWNED, ///< Preserve the existing route and another pin owner's override.
  CLKOUT25, ///< CLKOUT25 enabled (original ENHANCED and DP83825I). Application owns clock consumers.
  LED1, ///< Disable CLKOUT25 after releasing its consumer. On DP83825I this does not select a pin 2 function.
  PLL, ///< A pin 30 PLL clock, LED0_GPIO_CFG[5:0]=0x29.
  RECOVERED ///< A pin 30 recovered 125 MHz clock, requires a live partner.
};
/// @brief DP83825I DSP subcontrols, indexed in AnalogSettings::dspMasks and dspValues.
enum class DspControl : uint8_t {
  LPI_FREEZE, ///< 0308[15:10,7:4] LPI loop freeze and MSE bypass.
  EQUALIZER, ///< 030F[7:0] equalizer freeze and threshold selections.
  CAGC_MAPPING, ///< 0311[15:13] gain mapping and coefficient source. Mapping codes 0..2.
  TIMING_LOOP, ///< 031C[15:14,6:0] forcing, freeze and detector controls.
  LINE_DRIVER_CURRENT, ///< 0404[7:5] codes 0,4,7 indicate 12.7,15.7,19.5 mA.
  LPI_FREEZE_GROUPS, ///< 031F named LPI loop control bits.
  THRESHOLD_SCALING ///< 033E named threshold scaling controls.
};
/// @brief Persistent exact-part electrical intent; SNLS647I tables 8-73/79..81/86.
struct AnalogSettings {
  bool amplitudeOwned{false}; ///< Own both 100M VOD words; legal split-code transition is mandatory.
  uint8_t minusMdi{0x30}; ///< Negative codes 28..38:150..50 percent in 6.25 percent steps.
  uint8_t minusMdix{0x30}; ///< Negative codes 28..38:150..50 percent in 6.25 percent steps.
  uint8_t plusMdi{0x10}; ///< Positive codes 08..18:50..150 percent in 6.25 percent steps.
  uint8_t plusMdix{0x10}; ///< Positive codes 08..18:50..150 percent in 6.25 percent steps.
  uint8_t zeroCode{0}; ///< Raw six-bit zero DAC, no physical scale.
  bool terminationOwned{false}; ///< Explicitly tune after mandatory 4A40 initialization.
  uint8_t mdiTermination{4}; ///< Independent raw codes 0..15; table units are unspecified.
  uint8_t mdixTermination{4}; ///< Independent raw codes 0..15; table units are unspecified.
  bool mdiTerminationEnable{true}; ///< Independent override enables.
  bool mdixTerminationEnable{true}; ///< Independent override enables.
  bool macSlewOwned{false}; ///< Own IOCTRL low five bits only.
  uint16_t macSlewCode{0x10}; ///<10 fast/11 slow;0400 example remains prohibited.
  bool ioSlewOwned{false}; ///< Own IO_CFG1[15:14], preserving clock/interface fields.
  uint8_t ioSlewCode{0}; ///<0 slow/1 fast;2/3 reserved.
  bool receiverGainOwned{false}; ///< Representable source-gated tuning request, rejected before I/O.
  uint8_t receiverGainCode{0}; ///< Raw five-bit request; mapping remains analog_rx_gain gated.
  bool maintenanceAdmitted{false}; ///< Owner authorizes temporary intermediate amplitudes.
  bool macStopped{false}; ///< MAC drained/stopped; does not imply analog transmitter disabled.
  bool fixtureAdmitted{false}; ///< Identified load/board/measurement conditions accepted by owner.
  bool terminationOverrideAdmitted{false}; ///< Explicit departure from initialization 4A40 permitted by owner.
  bool vodOwned{false}; ///< DP83825I whole-word VOD procedure, SNLS638C tables 8-2/3.
  uint8_t vodCoarseStep{8}; ///< 0..16 selects 50..150 percent in steps of 6.25 percent.
  int8_t vodFineStep{0}; ///< Signed table 8-3 offset, -2..2. Unit trim must yield a representable word.
  bool impedanceOwned{false}; ///< Own DP83825I IO_CFG[15:14].
  bool impedance25Ohm{false}; ///< False selects 50 ohms, true selects 25 ohms.
  std::array<uint16_t,7> dspMasks{}; ///< DP83825I ownership masks indexed by DspControl. Multi-bit fields require full masks.
  std::array<uint16_t,7> dspValues{}; ///< Native register bits within the corresponding mask. Changes require an admitted analog fixture.
};
/// @brief Passive electrical profile proof; partial words do not establish a complete profile.
struct AnalogSnapshot {
  AnalogSettings desired{}; ///< Last fully admitted desired settings.
  bool desiredValid{false}; ///< Explicit persistent ownership exists.
  ConfigurationTrust trust{ConfigurationTrust::UNKNOWN}; ///< Whole owned-mask verification.
  std::array<uint16_t,12> raw{}; ///< VOD1/2/3, IOCTRL, IO_CFG and the seven DspControl words.
  uint16_t validMask{0}; ///< Valid retained reads. DP83825I 030E is a recipe write and has no readback bit.
  bool observedStale{true}; ///< Freshness and timestamp validity are independent.
  bool timeValid{false}; ///< Freshness and timestamp validity are independent.
  uint64_t observedAtUs{0}; ///< Complete verification upper bound, microseconds.
  Status lastStatus{Err::RESULT_NOT_AVAILABLE,0,"No analog profile"}; ///< Latest pass outcome.
};
/// @brief Source-qualified compliance stimulus; instrument verdict is outside the PHY.
enum class ComplianceMode : uint8_t {
  TX100, ///< SNLA266B table 5-1 forced 100/full line stimulus.
  NLP10, ///< Table 5-2 forced 10/full normal link pulses.
  PRBS10, ///< Table 5-4 PCS/BIST with owned BISCR 701F=7001.
  PATTERN10, ///< Table 5-5 COMPT exception: 0010|pattern 0..D.
  COUNT100 ///< DP83825I five-bit count 0..31 from 0428[5] and 0027[3:0], table 7-35.
};
/// @brief Explicit compliance fixture admission; dwell belongs to the application.
struct ComplianceOptions {
  ComplianceMode mode{ComplianceMode::TX100}; ///< Exact source procedure.
  PairSelection pair{PairSelection::MDI}; ///< Manual MDI/MDIX only; preserve PHYCR reserved/read-only fields.
  uint8_t pattern{0}; ///< 10M 0..D; 100M count 0..31 retains separate E/F meaning.
  bool maintenanceAdmitted{false}; ///< Owner permits disruptive reset and stimulus.
  bool macStopped{false}; ///< No production traffic during the test.
  bool fixtureAdmitted{false}; ///< Operator accepted termination/probes/reference conditions.
  bool hardResetAdmitted{false}; ///< Reset may change clocks/straps; owner prepares the whole shared bus.
  bool scramblerBypass{false}; ///< DP83825I 100BASE-TX compliance only, restored on exit.
};
/// @brief MII collision test admission; external TX_EN drives the actual timing stimulus.
struct CollisionOptions {
  LinkSpeed speed{LinkSpeed::UNKNOWN}; ///< Owner fixture-frequency policy: expected local 10/100, checked before entry; not a silicon prerequisite.
  bool maintenanceAdmitted{false}; ///< Owner excludes production traffic.
  bool macStopped{false}; ///< Owner excludes production traffic.
  bool colAvailable{false}; ///< Board mux and external generator admitted by owner.
  bool txEnControlled{false}; ///< Board mux and external generator admitted by owner.
  bool registerOnly{false}; ///< DP83825I RMII BMCR control only, without an observable COL pin.
};
/// @brief One calibration contingency; does not prove resistor calibration accuracy.
struct CalibrationOptions {
  bool maintenanceAdmitted{false}; ///< Explicit reset/traffic permission.
  bool macStopped{false}; ///< Explicit reset/traffic permission.
  bool hardResetAdmitted{false}; ///< Explicit reset/traffic permission.
};
/// @brief Passive temporary electrical stimulus and contingency evidence.
struct ElectricalSnapshot {
  ComplianceOptions compliance{}; ///< Last admitted compliance request.
  CollisionOptions collision{}; ///< Last admitted collision request.
  bool complianceSession{false}; ///< Owned sessions, independent of actual emission.
  bool collisionSession{false}; ///< Owned sessions, independent of actual emission.
  bool controlsVerified{false}; ///< Register proof only, never instrument PASS.
  bool stopped{false}; ///< Register proof only, never instrument PASS.
  bool temporaryMayBeActive{false}; ///< Accepted/uncertain stimulus remains until explicit stop.
  uint16_t rawCdcr{0}; ///< One calibration flag observation, no capture freshness guarantee.
  bool hardResetOwed{false}; ///< An accepted/uncertain calibration clear still requires HARD.
  bool cdcrValid{false}; ///< Distinct flag/reset evidence.
  bool recoveryNeeded{false}; ///< Distinct flag/reset evidence.
  bool resetComplete{false}; ///< Distinct flag/reset evidence.
  Status lastStatus{Err::RESULT_NOT_AVAILABLE,0,"No electrical operation"}; ///< Latest operation status.
};
/// @brief Explicit hardware Fast Link Drop policy; SNLS647I 8.3.17 / tables 8-30/52/53.
struct FastLinkDropSettings {
  bool owned{false}; ///< Own this policy; unowned controls are untouched.
  uint16_t causeMask{0}; ///< CR3 subset 0x040F; nonzero requires MII.
  bool rawThresholdOverride{false}; ///< Explicit experimental thresholds under fld_threshold_recipe; no latency guarantee.
  uint8_t windowCode{1}; ///< MLT3 window 1..63: code+1 clocks, 8 ns each; zero reserved.
  uint8_t errorThreshold{1}; ///< MLT3 error count 1..63; zero reserved.
  uint8_t descramblerThreshold{8}; ///< Raw six-bit descrambler code; default source recipe 0x08.
};
/// @brief Owned 100BASE-TX controls (SNLS647I tables 8-29/35).
/// On DP83825I, ownership covers only RX_ER during idle (SNLS638C table 7-13).
/// Leave scramblerBypass false and fifoDepthCode zero to preserve PHYSCR.
struct Tx100Settings {
  bool owned{false}; ///< Apply only on explicit owner request.
  bool scramblerBypass{false}; ///< Coordinated test/partner use, never normal default policy.
  uint8_t fifoDepthCode{0}; ///< Far-end loopback FIFO: codes 0/1/2/3 select 4/5/6/8 nibbles.
  bool rxErrorDuringIdle{false}; ///< CR2[2], independent of odd-nibble and GENCFG IPG.
};
/// @brief Owned 10BASE-Te controls; SNLS647I tables 8-44/51.
struct Tx10Settings {
  bool owned{false}; ///< Apply only on explicit owner request; mandatory VOD remains initialization-owned.
  bool lowerThreshold{false}; ///< Enable documented receive threshold option; no distance guarantee.
  uint8_t squelchCode{0}; ///< Codes 0..8 select 200..600 mV in 50 mV steps; 9..15 prohibited.
  bool nlpDisable{false}; ///< Suppress normal link pulses under maintenance admission.
  bool jabberDisable{false}; ///< Disable jabber protection under maintenance admission.
  bool shortPreamble{false}; ///< Short=true/long=false; requires explicit partner admission.
};
/// @brief Complete quality profile; include every previously owned subsection when replacing it.
struct QualitySettings {
  FastLinkDropSettings fastDrop{}; ///< Hardware link-loss detector policy.
  Tx100Settings tx100{}; ///< 100M transmit/receive controls.
  Tx10Settings tx10{}; ///< 10M receive/link controls.
  bool recoveryOwned{false}; ///< Own CR1[7], independent of software retry policy.
  bool linkRecovery{false}; ///< Enable silicon link-loss recovery; no transition deadline.
  bool maintenanceAdmitted{false}; ///< Application authorizes disruptive traffic changes.
  bool macStopped{false}; ///< Application has stopped/drained MAC traffic before mutation.
  bool partnerPreambleAdmitted{false}; ///< Partner accepts requested 10M preamble.
};
/// @brief Passive desired/observed quality configuration evidence.
struct QualitySnapshot {
  QualitySettings desired{}; ///< Last completely admitted request, retained after failure.
  bool desiredValid{false}; ///< Explicit ownership exists.
  ConfigurationTrust trust{ConfigurationTrust::UNKNOWN}; ///< Whole owned-mask verification only.
  std::array<uint16_t,8> raw{}; ///< CR3,FLD_CFG1,FLD_CFG2,PHYSCR,CR2,10BTSCR,10M_CFG,CR1.
  uint8_t validMask{0}; ///< Raw words observed in the current configuration epoch, by index.
  bool observedStale{true}; ///< Failure/reset/mutation invalidates current aggregate proof.
  bool timeValid{false}; ///< Successful verification timestamp is available.
  uint64_t observedAtUs{0}; ///< Completion upper bound in binding microseconds.
  bool fldMaximumApplicable{false}; ///< Source recipe and footnote 5 conditions only; physical timing still unmeasured.
  Status lastStatus{Err::RESULT_NOT_AVAILABLE,0,"No quality configuration"}; ///< Latest pass outcome.
};
/// @brief DP83825I classification from SDAA081 table 2-6. Boundary gaps retain the raw sample.
enum class MseQuality : uint8_t {
  UNCLASSIFIED, ///< No exact-part classification is available for this sample.
  EXCELLENT, ///< Below 020Ah.
  GOOD, ///< Above 020Ah and below 033Bh.
  POOR ///< Above 033Bh.
};
/// @brief Raw MSE observation with cached link context; no SNR or calibrated units.
struct SignalQualitySnapshot {
  uint16_t rawMse{0}; ///< DEVAD 31:0218 word; 0225 alternative remains source-gated.
  bool valid{false}; ///< A complete terminal read succeeded.
  bool stale{true}; ///< Failed refresh or known device/link change.
  bool timeValid{false}; ///< Observed timestamp is available; zero is valid.
  uint64_t observedAtUs{0}; ///< Conservative completion microseconds.
  LinkSnapshot linkContext{}; ///< Previously cached context, not a simultaneous link measurement.
  DeviceIdentity identity{}; ///< Cached identity at sample acquisition; no new probe.
  Status lastStatus{Err::RESULT_NOT_AVAILABLE,0,"No MSE acquisition"}; ///< Last read/invalidation status.
  MseQuality quality{MseQuality::UNCLASSIFIED}; ///< DP83825I thresholds, meaningful only for a valid sample.
};
/// @brief Complete interface/compatibility intent; board declarations are not measurements.
struct InterfaceSettings {
  MacInterface interface{MacInterface::UNKNOWN}; ///< Must equal the binding's declared MII/RMII wiring.
  RmiiClock rmiiClock{RmiiClock::LEADER_25MHZ}; ///< RMII reference direction; dormant default in MII.
  bool txClockShift{false}; ///< RCSR[8], admitted only in RMII follower mode.
  bool revision10{false}; ///< True selects RMII 1.0; false selects RMII 1.2.
  uint8_t elasticityCode{0}; ///< Codes 0/1/2/3 mean 14/2/6/10 bits, not increasing depth.
  uint32_t maxFrameBytes{1518}; ///< Declared maximum packet bytes for the elasticity bound.
  uint16_t clockTolerancePpm{50}; ///< Per-clock +/- tolerance: admitted source cases 50 or 100ppm.
  bool rxDv{false}; ///< RMII output RX_DV instead of CRS_DV; required for repeater wiring.
  bool repeater{false}; ///< One side of an explicitly wired two-PHY repeater, not pair readiness.
  bool fastRxDv{false}; ///< CR1[1]: assert on /J/ rather than /JK/.
  bool oddNibbleDisable{false}; ///< CR2[1]: one DISABLES odd-nibble detection.
  bool collisionFullDuplex{false}; ///< PHYSCR[4]: also generate COL in full duplex, MII only.
  bool shortIpg{false}; ///< GENCFG[3]: 120ns rather than 200ns minimum IPG.
  ClockOutput clockOutput{ClockOutput::UNOWNED}; ///< Optional declared-part clock route ownership.
  bool isolationOwned{false}; ///< Explicitly own BMCR[10], separately from power-down.
  bool isolate{false}; ///< Isolate MAC; stops RMII-leader 50MHz output, not SMI.
  bool maintenanceAdmitted{false}; ///< Owner permits disruptive interface transitions.
  bool macStopped{false}; ///< MAC traffic and interface consumers are stopped.
  bool pinsAvailable{false}; ///< Board/interface pin assignments permit the complete request.
  bool referenceClockStable{false}; ///< Owner has established the required reference externally.
  bool managementClockAvailable{false}; ///< MDIO controller/reference remains usable through changes.
  bool clockChangeApproved{false}; ///< Owner approves clock removal AND reconnection; neither is implicit.
  uint8_t referenceClockMHz{0}; ///< Actual declared reference 25MHz or 50MHz, matching the requested role.
  /// Crossed data/RX_DV and a single clock driver confirmed. A managed DP83825I pair
  /// requires two followers on a common external 50 MHz clock and simultaneous external reset.
  bool repeaterWiringAdmitted{false};
  bool observePin31{false}; ///< Request ENHANCED A pin 31 status (0308 bit 0) during interface verification.
};
/// @brief Retained interface words and intent, distinct from observed working traffic.
struct InterfaceSnapshot {
  InterfaceSettings desired{}; ///< Last fully admitted request, retained after partial failure.
  bool desiredValid{false}; ///< Explicit persistent ownership exists.
  MacInterface observedInterface{MacInterface::UNKNOWN}; ///< Decoded RCSR mode, independent of desired wiring.
  bool modeValid{false}; ///< RCSR mode word was successfully observed; stale still marks old context.
  ConfigurationTrust trust{ConfigurationTrust::UNKNOWN}; ///< Stable-owned-field verification only.
  uint16_t raw[9]{}; ///< CR1,CR2,PHYSCR,GENCFG,IO_CFG1,RCSR,BMCR,pin 31 status,A pin 30 control.
  uint16_t validMask{0}; ///< Current-epoch words. Reset/invalidation clears the mask while retaining historical raw data.
  bool observedStale{true}; ///< Partial, failed or invalidated proof; old words remain historical.
  bool timeValid{false}; ///< Completion timestamp validity; zero can be valid.
  uint64_t observedAtUs{0}; ///< Conservative completion upper bound in microseconds.
  Status lastStatus{Err::RESULT_NOT_AVAILABLE,0,"No interface observation"}; ///< Last apply/replay status.
};
/// @brief One finite interface application with partial evidence and no guessed rollback.
struct InterfaceResult {
  Status status{Err::RESULT_NOT_AVAILABLE,0,"No interface result"}; ///< Terminal logical outcome.
  Status primary{}; ///< Last invoked backend primary status.
  Status release{}; ///< Separate ownership-release status.
  uint32_t attemptedFrames{0}; ///< Actual attempted MDIO frames, including selectors.
  uint32_t completedFrames{0}; ///< Independently valid contiguous completed frames.
  uint32_t transactions{0}; ///< Terminal callback invocations; no retry.
  bool mutationAttempted{false}; ///< Owned target write attempted.
  bool mutationCompleted{false}; ///< An owned target has valid completion evidence.
  bool mutationPossible{false}; ///< Accepted or uncertain persistent hardware effects remain possible.
  bool verified{false}; ///< Every requested stable field read back; not clock/traffic qualification.
  ConfigurationMismatch mismatch{}; ///< First independently observed contradiction.
  InterfaceSnapshot interfaceState{}; ///< Retained partial/current observations at return.
};

/// @brief Exact-part wake indication route; board wiring is separately admitted.
enum class WakeRoute : uint8_t {
  STATUS_ONLY, ///< Matching/status without a physical output requirement.
  INTERRUPT, ///< Existing verified task 27 WAKE interrupt policy and pin route.
  LED1, ///< Own LED1 mux 2; receiver stays disabled while routing changes.
  LED2, ///< Own ENHANCED LED2 mux 2; MII COL must be unused.
  LED0, ///< A-generation pin 30, independent of the interface clock owner.
  LED3 ///< A-generation pin 29 in ENHANCED, shared with MII CRS.
};
/// @brief Return the LED mux index for a routed wake indication.
/// @param[in] route Selected wake indication route.
/// @return LED index 0 through 3, or 4 for a route outside the LED mux.
inline constexpr uint8_t ledIndex(WakeRoute route) noexcept {
  switch (route) {
    case WakeRoute::LED0: return 0;
    case WakeRoute::LED1: return 1;
    case WakeRoute::LED2: return 2;
    case WakeRoute::LED3: return 3;
    default: return 4;
  }
}
/// @brief Complete opt-in WoL intent; six fixed octets use transmission order.
struct WakeSettings {
  bool enabled{false}; ///< Explicit disable clears enable, Secure-ON and magic-match bits.
  bool secureOn{false}; ///< Require the password when enabled; disable always clears the hardware flag.
  bool crcGate{true}; ///< Suppress indication on bad CRC when true.
  bool levelMode{true}; ///< Level-change indication; false selects a pulse.
  uint8_t pulseWidth{0}; ///< Codes 0..3 select 8/16/32/64 cycles at 125MHz; only applies in pulse mode.
  uint8_t mac[6]{}; ///< Octet 0 is first transmitted; all 48-bit values are representable.
  uint8_t password[6]{}; ///< Secure-ON octet 0 first; no cryptographic security claim.
  WakeRoute route{WakeRoute::STATUS_ONLY}; ///< Owned even while disabled; STATUS_ONLY releases the route requirement, not the physical mux.
  bool maintenanceAdmitted{false}; ///< Owner permits disruptive detector configuration.
  bool receiveTrafficQuiesced{false}; ///< Owner coordinated traffic; disable readback cannot prove quiescence.
  bool pinAvailable{false}; ///< Required for any output route; no physical wiring proof.
  bool led1ClockUnused{false}; ///< Required for ENHANCED LED1 route.
  bool colUnused{false}; ///< Required for ENHANCED MII LED2 route.
  bool crsUnused{false}; ///< Board releases MII CRS for alternate use of A pin 29.
};
/// @brief Persistent intent and partial observations; event history lives in the collector.
struct WakeSnapshot {
  WakeSettings desired{}; ///< Last admitted complete intent, retained across failures.
  bool desiredValid{false}; ///< Initialization replays WoL only after explicit ownership.
  ConfigurationTrust trust{ConfigurationTrust::UNKNOWN}; ///< Owned settings verified independently of wake delivery.
  bool observedStale{true}; ///< Incomplete or invalidated aggregate; successful prefixes remain retained.
  bool timeValid{false}; ///< Complete observation timestamp validity; zero may be valid.
  uint64_t observedAtUs{0}; ///< Conservative completion upper bound in microseconds.
  uint16_t rawControl{0}; ///< RXFCFG1 including command/reserved evidence.
  uint16_t rawMac[3]{}; ///< Exact RXFPMD1..3 words, pair-validity below.
  uint16_t rawPassword[3]{}; ///< Exact RXFSOP1..3 words, pair-validity below.
  uint16_t rawRxfs{0}; ///< Last collector-owned RXFS sample, never repeated consumption.
  uint16_t rawMisr2{0}; ///< Last collector-owned MISR2 sample.
  uint16_t rawPinControl{0}; ///< Last selected LED mux word, when that route is used.
  bool pinControlValid{false}; ///< Raw selected pin word was returned successfully.
  bool controlValid{false}; ///< Current retained control word is valid.
  bool rxfsValid{false}; ///< Latest requested RXFS read succeeded.
  bool misr2Valid{false}; ///< Latest requested MISR2 read succeeded.
  uint8_t credentialValidMask{0}; ///< Bits 0..2 MAC words, 3..5 password words; independent partial validity.
  bool enabled{false}; ///< Observed bit 7 when controlValid; reset does not mean disabled.
  bool secureOn{false}; ///< Observed bit 5 when controlValid.
  bool magicMatch{false}; ///< Observed RH/W bit 0; stability remains physical Q3.
  bool crcGate{false}; ///< Observed bit 12 when controlValid.
  bool levelMode{false}; ///< Observed bit 8 when controlValid.
  uint8_t pulseWidth{0}; ///< Observed code 0..3 when controlValid.
  Status lastStatus{Err::RESULT_NOT_AVAILABLE,0,"No wake observation"}; ///< Last operation/acquisition result.
};

/// @brief Software request evidence, never PHY or partner readiness.
enum class ForcedLpi : uint8_t {
  OFF, ///< No owned request, or bit 12 cleared and verified/reset reconciled.
  REQUESTED, ///< Bit 12 set and verified; MAC traffic must stay drained.
  UNCERTAIN ///< Forced LPI may remain active; frames sent now can be lost.
};
/// @brief Explicit owner admission for disruptive legacy-MAC LPI entry.
struct LpiOptions {
  bool macTxQuiesced{false}; ///< Application has stopped and drained MAC transmission.
  bool maintenanceAdmitted{false}; ///< Application owns dwell and post-exit readiness.
  uint8_t forceModeCode{0}; ///< DP83825I negotiated (0) or bilateral force (3). Encodings 1 and 2 are reserved.
  bool partnerForceConfirmed{false}; ///< Application confirms the peer independently selected force code 3.
};
/// @brief Diagnostic output of one admitted bounded call; published even on failure.
struct LpiResult {
  Status status{Err::RESULT_NOT_AVAILABLE,0,"No LPI call"}; ///< Final normalized call outcome.
  Status primary{Err::RESULT_NOT_AVAILABLE,0,"No LPI batch"}; ///< Exact backend primary status of final/failing batch.
  Status release{Err::RESULT_NOT_AVAILABLE,0,"No LPI batch"}; ///< Separate backend release status.
  ForcedLpi forcedLpi{ForcedLpi::OFF}; ///< Request evidence at return, retained independently of status.
  bool mutationAttempted{false}; ///< Target write attempted, excluding selector effects.
  bool mutationCompleted{false}; ///< Target receipt completed, even if later validation failed.
  bool mutationPossible{false}; ///< Target may have changed, including malformed reports.
  bool readbackValid{false}; ///< Final target word returned valid data; status still gates success.
  uint16_t control{0}; ///< Final raw EEECFG3 readback, valid only when readbackValid.
  uint8_t attemptedFrames{0}; ///< Valid attempted prefixes, at most nine frames.
  uint8_t completedFrames{0}; ///< Valid completed prefixes, at most nine frames.
  uint8_t transactions{0}; ///< Actual callbacks, at most two.
  const char* warning{nullptr}; ///< Static warning when forced LPI may remain active.
  uint32_t minimumNormalIfgUs{0}; ///< MAC Normal Inter-Frame interval before data after a DP83825I stop (SNLA328).
};
/// @brief Passive session and last-call evidence; callers retain earlier result copies.
struct LpiSnapshot {
  ForcedLpi forcedLpi{ForcedLpi::OFF}; ///< Current owned session knowledge.
  LpiResult lastResult{}; ///< Last admitted call, unchanged by local preflight rejection.
};

/// @brief Explicit EEE intent; defaults do not activate the feature.
struct EeeSettings {
  bool enabled{false}; ///< Select the exact-part recipe (18 original, 27 A or 32 DP83825I mode writes). Disable uses HARD and replay.
  uint16_t advertisement{0x0002}; ///< Only 0 or 2 (100BASE-TX EEE); no other technologies.
  bool bypassBit0{false}; ///< Capability bypass control at EEECFG3 bit 0; true encodes zero.
  bool bypassBit3{false}; ///< Capability bypass control at EEECFG3 bit 3; true encodes zero.
  bool maintenanceAdmitted{false}; ///< Application admits link disruption and shared-bus silence.
  bool disableNextPageReception{false}; ///< DP83825I 04D1[2]. Disable reception of EEE next pages when true.
  bool shutdownRxDuringQuiet{true}; ///< DP83825I 04D1[1]. Disable analog RX during LPI Quiet. Default matches table 6-7.
};
/// @brief EEE policy and bounded observation; source-implied fields await physical Q1.
struct EeeSnapshot {
  EeeSettings desired{}; ///< Explicit application intent, separate from hardware evidence.
  bool desiredValid{false}; ///< This binding owns the policy only after an admitted start.
  ConfigurationTrust trust{ConfigurationTrust::UNKNOWN}; ///< Stable controls, never opaque-word readback proof.
  bool stale{true}; ///< Aggregate observation unavailable or invalidated.
  bool valid{false}; ///< Complete ordered observation exists, not simultaneous silicon sampling.
  bool timeValid{false}; ///< observedAtUs may legitimately be zero.
  uint64_t observedAtUs{0}; ///< Microseconds at last complete observation.
  uint16_t capability{0}; ///< Raw MMD 3.20.
  uint16_t advertisement{0}; ///< Raw MMD 7.60.
  uint16_t partner{0}; ///< Raw MMD 7.61.
  uint16_t status{0}; ///< Raw MMD 3.1; bits 11:10 latched, 9:8 current (source-implied).
  uint16_t wakeErrors{0}; ///< Raw saturating RC MMD 3.22; FFFF is a lower bound.
  uint16_t control{0}; ///< Vendor 04D1 observation. Decode only fields sourced for the declared part.
  bool capabilityValid{false}; ///< Latest capability word returned successfully.
  bool advertisementValid{false}; ///< Latest local word returned successfully.
  bool partnerValid{false}; ///< Latest partner word returned successfully.
  bool statusValid{false}; ///< Latest status word returned successfully; collector owns history.
  bool wakeErrorsValid{false}; ///< Latest counter word returned successfully.
  bool controlValid{false}; ///< Latest admitted masked-control read returned successfully.
  bool negotiatedValid{false}; ///< Complete coherent AN 100/full observation and known controls.
  bool negotiated{false}; ///< Local/partner EEE100 advertised; does not prove current LPI.
  Status lastStatus{}; ///< Latest operation or acquisition outcome.
  PcsRxClock rxClock{PcsRxClock::RETAIN}; ///< Persistent MMD3:0000[10] ownership.
  uint16_t pcsControl{0}; ///< Most recent independently returned MMD3:0000 word.
  bool pcsControlValid{false}; ///< Clock/reset information belongs to the current observation epoch.
  bool recipeRestartAttempted{false}; ///< The current EEE recipe reached the restart write.
  bool recipeRestartAccepted{false}; ///< The backend reported a completed recipe restart.
  bool recipeRestartPossible{false}; ///< Restart may have taken effect, including an uncertain write.
};

/// @brief Frozen evidence delivered once; restoration uses a separate token/result.
struct OperationResult {
  OperationToken token{}; ///< Owning instance's lifetime token.
  OperationKind kind{OperationKind::NONE}; ///< Operation that produced this result.
  ResetKind reset{ResetKind::NONE}; ///< Effective reset, including a hard reset required by an owned EEE profile.
  Status status{Err::RESULT_NOT_AVAILABLE, 0, "No operation"}; ///< Immutable terminal outcome.
  bool mutationAttempted{false}; ///< Semantic target write attempted; excludes selectors.
  bool mutationCompleted{false}; ///< Semantic target completed, even if later failure occurred.
  bool mutationPossible{false}; ///< Semantic effect may have occurred.
  bool commandRequired{false}; ///< This operation requests a reset command.
  bool commandAccepted{false}; ///< Reset frame completed; does not prove reset completion.
  bool protocolComplete{false}; ///< Requested reset subsequently observed clear on time; not AN completion.
  bool negotiationComplete{false}; ///< BMSR AN-complete observed by an explicitly requested wait.
  bool temporaryMayBeActive{false}; ///< Copy of retained temporary-state knowledge at termination.
  bool persistentRestored{false}; ///< All currently owned persistent state verified.
  bool cleanupOwed{false}; ///< Barrier at termination; current state is in operationSnapshot.
  bool clockInvalid{false}; ///< Clock contract failed during the operation.
  bool late{false}; ///< A batch returned at/after the deadline.
  DeviceIdentity identity{}; ///< Latest operation-owned identity evidence.
  StrapEvidence straps{}; ///< Captured raw straps with historical validity.
  ConfigurationMismatch mismatch{}; ///< First mismatch; an interrupted pass retains its terminal error separately.
  BatchSummary failingBatch{}; ///< First failure, otherwise latest batch.
  NextPageResult nextPage{}; ///< Exchange evidence, including partial consumption.
  LoopbackResult loopback{}; ///< Selected path and activation/stop evidence, never packet proof.
  BistResult bist{}; ///< Current diagnostic evidence, separate from primary/stop outcomes.
  CableResult cable{}; ///< Non-destructive cable words; primary and restoration results remain separate.
  CableAutorunSnapshot cableAutorun{}; ///< Retained policy evidence; success never proves engine quiescence.
  EeeSnapshot eee{}; ///< Retained EEE intent and partial acquisition evidence.
  WakeSnapshot wake{}; ///< Retained WoL intent, control/credential prefixes and status acquisition.
  PinSnapshot pins{}; ///< Retained pin profile and partial verification during shared restoration.
  AnalogSnapshot analog{}; ///< Retained electrical profile proof.
  ElectricalSnapshot electrical{}; ///< Retained stimulus/contingency evidence.
  QualitySnapshot quality{}; ///< Retained quality policy proof, including partial acquisition.
  InterfaceSnapshot interfaceState{}; ///< Interface/clock/isolation proof from the shared restoration pass.
  PowerSnapshot power{}; ///< Retained power intent, partial observations and transition outcome.
  InterruptSnapshot interrupts{}; ///< Retained policy and transient test effects, independent of restoration result.
  uint16_t savedAnar{0}; ///< Pre-mutation advertisement, never a rollback instruction.
  uint16_t savedBmcr{0}; ///< Pre-mutation control, never a rollback instruction.
  bool savedAnarValid{false}; ///< Pre-mutation advertisement was captured.
  bool savedBmcrValid{false}; ///< Pre-mutation control was captured.
  uint16_t savedCr1{0}; ///< Pre-mutation CR1; never an implicit rollback request.
  uint16_t savedPhycr{0}; ///< Pre-mutation routing word.
  uint16_t savedCr3{0}; ///< Pre-mutation mirror word.
  uint16_t savedCr2{0}; ///< Pre-mutation extension word.
  bool savedCr1Valid{false}; ///< Pre-mutation CR1 was captured.
  bool savedPhycrValid{false}; ///< Pre-mutation PHYCR was captured.
  bool savedCr3Valid{false}; ///< Pre-mutation CR3 was captured.
  bool savedCr2Valid{false}; ///< Pre-mutation CR2 was captured.
};
/// @brief Bus/clock-silent operation progress, without a copy of the retained result.
struct OperationSnapshot {
  OperationToken token{}; ///< Occupied slot token, zero when idle.
  OperationState state{OperationState::IDLE}; ///< Slot state.
  OperationKind kind{OperationKind::NONE}; ///< Occupied operation kind.
  OperationPhase phase{OperationPhase::NONE}; ///< Current phase or COMPLETE.
  Status status{Err::RESULT_NOT_AVAILABLE, 0, "No operation"}; ///< Progress/frozen status.
  bool cleanupOwed{false}; ///< Current restoration barrier, even with an empty slot.
  bool temporaryMayBeActive{false}; ///< Session override may remain, including after failed start/stop.
  uint64_t nextPollAtUs{0}; ///< Earliest legal next scheduled observation.
  bool nextPollValid{false}; ///< A valid scheduled wait exists.
  uint64_t busQuietUntilUs{0}; ///< Hard-reset silence floor; shared owner holds every PHY quiet.
  bool busQuietValid{false}; ///< Valid floor anchor; cached until a timed call observes expiry.
  IoBudget remaining{}; ///< Original deadline and remaining physical frames.
  uint32_t stalledPolls{0}; ///< Consecutive polls without work or clock advance.
};

/// @brief Noncopyable, application-serialized DP83826I/E driver.
/// @pre Serialize every call, including cached access. The borrowed context and
/// callbacks outlive calls. No callback may retain buffers or re-enter this object.
/// @note No allocation, SDK, hidden retries or work without caller polling.
/// Maintenance writes return a cached failed/rejected readIdentity status before
/// any callback. Successful reprobe or explicit rebind clears that restriction.
/// AI/AE admit initialization, link, FLD/recovery and interface/clock control.
/// Optional diagnostic, power, pin, wake, interrupt and EEE procedures are gated
/// for these grades. A rejected A identity permits only identity reads until reprobe.
/// Existing owner-attested maintenance before the first probe remains allowed;
/// stale successful identity withdraws capabilities but permits that maintenance.
class DP8382x {
public:
  /// @brief Construct an unbound object without callbacks.
  DP8382x() noexcept = default;
  /// @brief Destroy locally without callbacks or implicit controller cleanup.
  /// @pre All callers are quiescent; copy any needed receipt before destruction.
  ~DP8382x() noexcept = default;
  /// @brief Copying would create another owner and is forbidden.
  DP8382x(const DP8382x&) = delete;
  /// @brief Copy assignment is forbidden, even while unbound.
  DP8382x& operator=(const DP8382x&) = delete;
  /// @brief Moving would invalidate callback ownership and is forbidden.
  DP8382x(DP8382x&&) = delete;
  /// @brief Move assignment is forbidden, even while unbound.
  DP8382x& operator=(DP8382x&&) = delete;

  /// @brief Validate/copy a quiescent binding without clock or MDIO callbacks.
  /// @param[in] config Exact part, board interface, address and borrowed transport.
  /// @return OK, INVALID_CONFIG, UNSUPPORTED for RGMII, or BUSY for live work, a pending result or cleanup.
  /// @note Failure preserves the old binding/evidence. Success clears local
  /// receipts, never the application's shared controller quarantine.
  /// A discarded hard-reset floor remains the owner's responsibility: wait at least
  /// 2000 us after the last possible PHYRCR hard-reset write before new frames.
  /// Repair a clock/report-contract fault and stop affected controller users
  /// before using a fresh binding to resume I/O.
  Status bind(const Config& config) noexcept;
  /// @brief Detach locally and clear receipts; no release/reset/power callback.
  /// @pre Owner has copied required evidence and retains responsibility for
  /// controller faults; repair is required before a new binding resumes I/O.
  /// Copy any hard-reset floor into shared owner state before end; after discarding
  /// it, the owner waits at least 2000 us after the last possible hard-reset write.
  /// @return OK (also unbound), or BUSY for live work, a pending result or cleanup.
  Status end() noexcept;
  /// @brief Inspect local binding without I/O or clock access.
  /// @return True for a valid declaration, not proof of identity or bus readiness.
  bool isBound() const noexcept { return _bound; }

  /// @brief Admit initialization without MDIO or clock access; copy inputs into one slot.
  /// @param[in] options NONE, PCS, HARD or RESTART; each writes/verifies VOD_CFG3.
  /// @param[in] budget Positive interval/stall limit, absolute deadline and total frames.
  /// @param[out] token Assigned only when IN_PROGRESS is returned; instance lifetime scope.
  /// @return IN_PROGRESS or local syntax/binding/slot/clock/budget rejection.
  /// @pre Owner has established supplies/clock/reset readiness and admitted disruption.
  /// @note SNLS647I tables 8-19/48: valid active-reset readback invalidates its
  /// reset domain even if release or timing then fails; that error remains primary.
  /// Explicitly owned EEE upgrades any requested kind to one effective HARD;
  /// result.reset reports it and the shared-bus silence floor applies.
  Status startInitialize(const InitializeOptions& options, const OperationBudget& budget,
                         OperationToken& token) noexcept;
  /// @brief Advance at most one complete owned batch; return while waiting or short of slice.
  /// @param[in] token Exact occupied token; stale/foreign tokens cannot access new results.
  /// @param[in] maxFrames Per-poll allowance; zero/short slices yield without splitting a batch.
  /// @return IN_PROGRESS or frozen terminal status; token/lifetime errors perform no I/O.
  /// @note SNLS647I table 8-19 PCS uses literal 00=8000; completion polls that bit clear.
  /// Every initialization writes 31:030E=4A40 and verifies FFC0 (table 8-81).
  /// HARD/RESTART use literal 1F=8000/4000 (table 8-48), never sampled-word RMW.
  /// Any attempted HARD write publishes a persistent max(2000, interval) us floor
  /// anchored after callback return; the bus owner also excludes other PHYs.
  /// Waits use conservative post-callback time; no controller lock spans polls.
  /// Reset commands reserve the command, minimum wait and first completion poll
  /// at poll entry and again immediately before the batch callback. EEE also
  /// checks its remaining recipe allowance at that fresh callback-entry sample.
  Status pollOperation(OperationToken token, uint32_t maxFrames = MAX_BATCH_FRAMES) noexcept;
  /// @brief Copy current progress/barrier without MDIO or clock access.
  /// @return Slot state and remaining allowance; not a completion or link promise.
  OperationSnapshot operationSnapshot() const noexcept { return _operation; }
  /// @brief Deliver one frozen result locally, leaving any restoration barrier intact.
  /// @param[in] token Exact pending token.
  /// @param[out] result Assigned once on OK; unchanged on rejection.
  /// @return OK, RESULT_NOT_AVAILABLE, TOKEN_MISMATCH or BUSY during a callback.
  Status takeResult(OperationToken token, OperationResult& result) noexcept;
  /// @brief Cancel locally and freeze evidence; never perform restoration I/O.
  /// @param[in] token Exact occupied token; repeated cancel of pending result is OK.
  /// @return OK or token/lifetime error; terminal outcome is retained separately.
  Status cancel(OperationToken token) noexcept;
  /// @brief Transfer restoration responsibility to the application without callbacks.
  /// @pre Owner established external reset/isolation/power removal or safe responsibility transfer.
  /// @return BUSY while running/in callback; otherwise OK, retaining all historical evidence.
  /// @note Does not release controller quarantine or prove hardware restoration.
  Status abandonCleanup() noexcept;
  /// @brief Query implemented initialization readiness without MDIO or clock access.
  /// @return True after the declared part's identity, reset, startup and owned restoration checks pass.
  bool isInitialized() const noexcept { return _initialized; }

  /// @brief Admit a finite identity/stable-owned-state check without bus/clock callbacks.
  /// @param[in] budget Exclusive aggregate allowance; poll slices never split an MMD batch.
  /// @param[out] token Assigned only on admission; take the result before another start.
  /// @return IN_PROGRESS, or syntax, binding, slot, clock, temporary-state,
  /// initialization/capability or zero-budget error without changing evidence.
  /// @pre Initialized with fresh accepted identity and serialized application ownership.
  /// @note SNLS647I tables 8-19/21/22/23/28/29/30/43/81. Two identity reads,
  /// four-frame VOD read, then six AN or five forced stable-field reads when
  /// desired settings exist. Only indirect selectors are written; no reset,
  /// restart, VOD write, negotiation wait, retry or escalation. Forced skips ANAR.
  /// A complete matching profile restores verified trust, never cleanupOwed.
  /// Without desired settings only mandatory state is checked; profile trust
  /// stays unverified. Partial/mismatched raw words remain diagnostic evidence.
  /// A changed accepted identity requires explicit initialization; an observed
  /// active BMCR reset invalidates readiness and terminates without waiting.
  Status startVerify(const OperationBudget& budget, OperationToken& token) noexcept;

  /// @brief Admit a bounded loopback start; no MDIO or clock callback on admission.
  /// @param[in] options Exact path/speed and explicit maintenance/fixture attestations.
  /// @param[in] budget Aggregate deadline/frames and finite polling bounds.
  /// @param[out] token Assigned only on admission; consume the retained result before stop.
  /// @return IN_PROGRESS or local syntax, lifecycle, trust, capability or budget error.
  /// @pre Initialized, fresh exact identity and VERIFIED persistent desired profile.
  /// @note SNLS647I 8.3.14, tables 8-19/35/40; SNLA423A pp18/20 restart selection.
  /// First attempted semantic write sets the temporary barrier. Failure/cancel does
  /// no cleanup I/O. A successful start releases the slot after takeResult; dwell
  /// and traffic are application-owned. DIGITAL requires HARD on exit; 0123 overlaps EEE.
  Status startLoopback(const LoopbackOptions& options, const OperationBudget& budget,
                       OperationToken& token) noexcept;
  /// @brief Admit stop and verified restoration of this binding's retained loopback session.
  /// @param[in] budget New finite allowance, independent of the start result.
  /// @param[out] token New token assigned only on admission.
  /// @return IN_PROGRESS or local rejection; a failed stop retains effects for explicit retry.
  /// @pre Owner has stopped diagnostic traffic; no operation result remains pending.
  /// @note Clears selectors, reuses initialization/profile replay, then verifies saved
  /// temporary fields. DIGITAL uses HARD; other modes use NONE. No automatic retry.
  Status stopLoopback(const OperationBudget& budget, OperationToken& token) noexcept;
  /// @brief Copy session evidence without bus/clock activity.
  /// @return Current temporary flag and retained start identity/options; no packet proof.
  LoopbackSnapshot loopbackSnapshot() const noexcept;

  /// @brief Admit BIST setup with no bus/clock calls; tables 8-40/45/46 and section 8.3.15.
  /// @param[in] options Legal fields, maintenance and optional source-backed loopback prerequisites.
  /// @param[in] budget Finite setup deadline/frame/poll allowance.
  /// @param[out] token Assigned only on admission; consume result before acquisition.
  /// @return IN_PROGRESS or local validation/lifecycle error; requires verified initialized profile.
  /// @note Baseline capture clears old errors. Dwell is owner-driven; no PASS is inferred.
  Status startBist(const BistOptions& options, const OperationBudget& budget, OperationToken& token) noexcept;
  /// @brief Admit one observation after the configured minimum interval; no automatic resampling.
  /// @param[in] observation Current-epoch externally correlated traffic, never fixture attestation alone.
  /// @param[in] budget Independent finite acquisition allowance.
  /// @param[out] token New token on admission; result.bist retains partial samples on failure.
  /// @return IN_PROGRESS or local error; requires active initialized session.
  /// @note Uses one atomic snapshot-write/read batch and the single collector. Zero errors
  /// remain inconclusive under bist_result_semantics; bit 15-zero unlatch is never invented.
  Status startBistAcquire(const BistObservation& observation, const OperationBudget& budget, OperationToken& token) noexcept;
  /// @brief Admit PRBS disable and full shared restoration, even after failed start/acquire.
  /// @param[in] budget New finite stop allowance; no hidden retry or automatic cleanup.
  /// @param[out] token New token assigned on admission.
  /// @return IN_PROGRESS or local error; failed stop retains temporary state and earlier measurement.
  /// @note Rechecks exact identity before disabling PRBS. HARD restoration always replays
  /// both saved-field tails, including after a partially completed earlier stop.
  Status stopBist(const OperationBudget& budget, OperationToken& token) noexcept;
  /// @brief Copy current/historical BIST evidence without I/O or consuming a result.
  /// @return Retained options, samples and explicit validity/staleness.
  BistSnapshot bistSnapshot() const noexcept;

  /// @brief Admit finite manual configure/trigger/capture, leaving restoration to a new stop.
  /// @param[in] settings Exact recipe subset or one typed raw measurement.
  /// @param[in] budget Aggregate deadline/frame allowance; no callback on admission.
  /// @param[out] token Assigned only on admission; consume result exactly once.
  /// @return IN_PROGRESS or local rejection; requires initialized VERIFIED desired profile.
  /// @note CR1 autorun, active modes, live link and CDCR calibration reject before mutation.
  /// Tables 8-47/54..71 and SNLA428 table 2-1; no maximum completion time is invented.
  Status startCableTest(const CableSettings& settings, const OperationBudget& budget, OperationToken& token) noexcept;
  /// @brief Admit a bounded nondestructive reread of this session's last triggered row.
  /// @param[in] budget New finite allowance; no repeated trigger or automatic retry.
  /// @param[out] token New token only on admission.
  /// @return IN_PROGRESS or local error; generation/reset invalidation rejects acquisition.
  Status startCableAcquire(const OperationBudget& budget, OperationToken& token) noexcept;
  /// @brief Admit identity check, manual stop, HARD and full implemented-state replay.
  /// @param[in] budget Independent restoration allowance; failure retains the OP-9 barrier.
  /// @param[out] token New token only on admission.
  /// @return IN_PROGRESS or local rejection; allowed after failed/cancelled start.
  /// @note Opaque 0411/0416 are never read for replay. Live CDCR 14 rejects clear here (prompt 35).
  Status stopCableTest(const OperationBudget& budget, OperationToken& token) noexcept;
  /// @brief Copy partial/historical cable evidence without MDIO or clock calls.
  /// @return Fixed capture storage and outstanding temporary-state flag.
  CableSnapshot cableSnapshot() const noexcept;
  /// @brief Admit explicit persistent CR1[8] policy configuration without bus/clock I/O.
  /// @param[in] options One enable flag and disruptive maintenance admission.
  /// @param[in] budget Finite aggregate deadline/frame/stall allowance.
  /// @param[out] token Assigned only on admission; take the retained result before reuse.
  /// @return IN_PROGRESS or syntax, lifecycle, cleanup, initialization or budget error.
  /// @note SNLS647I 8.3.16.1/table 8-28. Apply only 0100, preserve FEFF, verify 0100.
  /// Desired intent becomes authoritative at admission. Failure/cancel retains
  /// effects behind OP-9; startInitialize replays it with a new budget/result.
  /// Disable is the manual handoff; engine quiescence remains unknown.
  Status startConfigureCableAutorun(const CableAutorunOptions& options, const OperationBudget& budget,
                                   OperationToken& token) noexcept;
  /// @brief Admit one bounded raw autorun observation using the shared cable result path.
  /// @param[in] budget Finite allowance for 14 frames in 5 indivisible callbacks.
  /// @param[out] token Assigned only on admission; words are retained in result.cable.
  /// @return IN_PROGRESS or lifecycle, temporary-session, initialization or budget error.
  /// @note SNLS647I tables 8-47/61/66/71: read CDCR,0180,0185,018A,CDCR once.
  /// No trigger, wait or retry. Even equal
  /// bookends leave freshness/coherence unknown and aggregate stale. Each returned
  /// nondestructive word has independent validity/time; failures imply no history loss.
  Status startCableAutorunAcquire(const OperationBudget& budget, OperationToken& token) noexcept;
  /// @brief Copy owned autorun intent/readback/trust without MDIO or clock I/O.
  /// @return Policy evidence only; disabled does not imply engine idle.
  CableAutorunSnapshot cableAutorunSnapshot() const noexcept { return _autorun; }

  /// @brief Admit exact EEE enable/disable; start is bus/clock silent.
  /// @param settings Copied legal policy and maintenance approval; enabling requires an owned AN profile.
  /// @param budget Aggregate reset/recipe/replay bound; short poll slices yield whole batches.
  /// @param[out] token Changed only on admission.
  /// @return IN_PROGRESS or local error; accepted effects retain OP-9 debt on failure.
  /// @note SNLS647I 8.3.3.1 eighteen rows and Q1 self-checks. Disable uses HARD;
  /// all owned EEE restoration selects HARD once, then shared VOD/profile replay.
  Status startConfigureEee(const EeeSettings& settings, const OperationBudget& budget, OperationToken& token) noexcept;
  /// @brief Acquire EEE words and coherent AN bookends; destructive words use the collector.
  /// @param[out] snapshot Unchanged on failure; eeeSnapshot/evidenceSnapshot retain partials.
  /// @param budget Finite frame/deadline allowance; no negotiation wait or retry.
  /// @return Acquisition status, separate from negotiated success and physical qualification.
  /// @note Controller quarantine marks cached evidence stale at its old timestamp;
  /// the rejected refresh performs no MDIO and leaves caller output unchanged.
  Status readEeeStatus(EeeSnapshot& snapshot, const IoBudget& budget) noexcept;
  /// @brief Copy EEE intent/evidence without bus or clock I/O.
  /// @return Historical/current validity and trust; serialize with the application owner.
  EeeSnapshot eeeSnapshot() const noexcept { return _eee; }

  /// @brief Apply and verify one finite LED/GPIO profile with no scheduled waits.
  /// @param[in] settings Complete pin ownership, source and board admission values.
  /// @param[out] result Unchanged on preflight rejection; retained partial result after admitted I/O.
  /// @param[in] budget Aggregate deadline/frames covers every complete RMW/readback.
  /// @return Terminal validation/transport/verification status; no retry or implicit restoration.
  /// @note SNLS647I 8.3.18/tables 8-42/49/73..78/85/89. Slot must be idle; cancellation
  /// cannot interrupt a synchronous callback. Failed effects retain OP-9 debt. Shared
  /// initialization replays the same procedure; physical output remains unqualified.
  /// LED2/3 first consume RCSR through the collector and require its current mode
  /// to match declared wiring (tables 5-1/8-41); a mux reset value is not proof.
  Status configurePins(const PinSettings& settings, PinResult& result, const IoBudget& budget) noexcept;
  /// @brief Bus/clock-silent retained pin settings and observed values.
  /// @return Separate desired, valid and trusted evidence; serialize under the owner.
  PinSnapshot pinsSnapshot() const noexcept;
  /// @brief Admit a complete electrical profile without bus/clock calls; poll bounded batches.
  /// @param[in] settings Owned fields and explicit traffic/fixture admission.
  /// @param[in] budget Finite total deadline, frames and stalled-poll limit.
  /// @param[out] token Published only on successful admission.
  /// @return IN_PROGRESS or local rejection; source-gated requests perform no I/O.
  /// @note Split MDIX codes use a verified low 8 bridge; cancellation retains OP-9 debt.
  Status startConfigureAnalog(const AnalogSettings& settings, const OperationBudget& budget, OperationToken& token) noexcept;
  /// @brief Passive electrical configuration; no bus or clock calls.
  /// @return Desired, partial raw and verified state.
  AnalogSnapshot analogSnapshot() const noexcept { return _analog; }
  /// @brief Start a source-qualified, owner-dwelled compliance session.
  /// @param[in] options Explicit mode, fixture and reset admission.
  /// @param[in] budget Finite start allowance; excludes later dwell and stop.
  /// @param[out] token Retained operation token on admission.
  /// @return IN_PROGRESS or zero-I/O validation failure;0428 remains gated.
  Status startCompliance(const ComplianceOptions& options, const OperationBudget& budget, OperationToken& token) noexcept;
  /// @brief HARD-stop compliance, reidentify and retain separate restoration debt.
  /// @param[in] budget Independent finite stop allowance.
  /// @param[out] token New retained stop token.
  /// @return IN_PROGRESS on admission; later startInitialize replays persistent state.
  Status stopCompliance(const OperationBudget& budget, OperationToken& token) noexcept;
  /// @brief Start MII collision-test session; SNLS647I table 8-19, external TX_EN required.
  /// @note Reset observed during the BMCR RMW stops with DEVICE_INVALIDATED
  /// unless a primary failure already occurred; possible stimulus retains debt.
  /// @param[in] options Expected speed and board/traffic admission.
  /// @param[in] budget Independent bounded start allowance.
  /// @param[out] token New retained start token.
  /// @return IN_PROGRESS or local failure; register state does not prove COL timing.
  Status startCollisionTest(const CollisionOptions& options, const OperationBudget& budget, OperationToken& token) noexcept;
  /// @brief Clear and verify owned collision stimulus with neutral commands.
  /// @note A reset observation in the RMW cannot prove successful stop/restoration.
  /// @param[in] budget Independent bounded stop allowance.
  /// @param[out] token New retained stop token.
  /// @return IN_PROGRESS or local failure; uncertain exit retains OP-9 debt.
  Status stopCollisionTest(const OperationBudget& budget, OperationToken& token) noexcept;
  /// @brief Read CDCR once; if 14 is high, submit adjacent clear/HARD writes then prove reset completion.
  /// @param[in] options Explicit maintenance and reset admission.
  /// @param[in] budget Finite aggregate contingency/reset allowance.
  /// @param[out] token New retained result token.
  /// @return IN_PROGRESS or local rejection; not a calibration-start or calibration-done claim.
  /// @note Stop on first callback failure; no automatic retry. Later initialization restores reset state.
  Status startCalibrationRecovery(const CalibrationOptions& options, const OperationBudget& budget, OperationToken& token) noexcept;
  /// @brief Passive stimulus/reset evidence; no hidden I/O.
  /// @return Retained current uncertainty and latest operation evidence.
  ElectricalSnapshot electricalSnapshot() const noexcept;

  /// @brief Start a bus/clock-silent owned quality profile; poll one complete batch per slice.
  /// @param[in] settings Explicit owned controls and maintenance/partner admission.
  /// @param[in] budget Finite total deadline/frames and stall allowance.
  /// @param[out] token Published only after full local validation.
  /// @return IN_PROGRESS on admission; invalid/source-gated requests perform no I/O.
  /// @note SNLS647I 8.3.17/tables 8-28..30/35/44/51..53. Cancellation freezes effects;
  /// later startInitialize restores behind OP-9. No silicon latency or link claim.
  /// On DP83825I, a preflight clock-role mismatch retains the RCSR sample even
  /// when a later transfer or controller release fails.
  Status startConfigureQuality(const QualitySettings& settings, const OperationBudget& budget, OperationToken& token) noexcept;
  /// @brief Passive owned quality configuration; caller serializes all access.
  /// @return Retained desired/partial observed values without I/O or a clock read.
  QualitySnapshot qualitySnapshot() const noexcept {
    auto snapshot = _quality;
    snapshot.fldMaximumApplicable = snapshot.fldMaximumApplicable && !snapshot.observedStale &&
        snapshot.trust == ConfigurationTrust::VERIFIED;
    return snapshot;
  }
  /// @brief One complete four-frame vendor MMD read, SNLS647I table 8-72.
  /// @param[out] snapshot Unchanged unless the complete terminal read succeeds.
  /// @param[in] budget Aggregate lock/controller allowance, minimum four frames.
  /// @return Transport/admission status; failure leaves cached sample stale at its old time.
  /// @note Does not read destructive counters or convert raw MSE to SNR; cached
  /// link context is not simultaneous. Source gate mse_address remains open.
  Status readSignalQuality(SignalQualitySnapshot& snapshot, const IoBudget& budget) noexcept;
  /// @brief Passive raw quality observation; no I/O or clock read.
  /// @return Retained raw value, validity, staleness and original context.
  SignalQualitySnapshot signalQualitySnapshot() const noexcept { return _signalQuality; }

  /// @brief Apply a complete clock/interface/compatibility profile under one finite allowance.
  /// @param[in] settings Valid intent matching bound wiring and explicit stopped-MAC/clock admission.
  /// @param[out] result Unchanged on local rejection; partial evidence after an invoked callback.
  /// @param[in] budget Aggregate deadline and frames for the complete synchronous pass, rechecked before first submission.
  /// @return OK, local admission error, CONFIG_MISMATCH or terminal transport/budget failure.
  /// @note SNLS647I 8-19/28/29/35/41/73/84. RCSR reads consume RC through the
  /// single collector. Failed accepted writes leave OP-9 debt; restoration uses
  /// a separately admitted startInitialize. No clock-source switch or retry is automatic.
  Status configureInterface(const InterfaceSettings& settings, InterfaceResult& result, const IoBudget& budget) noexcept;
  /// @brief Copy retained interface intent/evidence without bus or clock callbacks.
  /// @return Stable-field trust, never proof of working clock, MAC or repeater traffic.
  InterfaceSnapshot interfaceSnapshot() const noexcept { return _interface; }

  /// @brief Admit quiesced WoL configuration; start is bus/clock silent.
  /// @param[in] settings Complete match, Secure-ON, CRC, pulse/level and route intent.
  /// @param[in] budget Aggregate operation deadline/frames and finite stalled-poll limit.
  /// @param[out] token Assigned only on admission; poll and take the retained result.
  /// @return IN_PROGRESS or local validation/ownership error without mutation.
  /// @note SNLS647I 8.3.5/tables 8-90..97. Disable/verify before credential changes,
  /// verify all words, then enable last. Q3 gates physical live-reconfiguration safety.
  /// Owned settings extend shared reset restoration; no default WoL-off assumption.
  Status startConfigureWake(const WakeSettings& settings, const OperationBudget& budget, OperationToken& token) noexcept;
  /// @brief Admit one explicit level acknowledgement, never an automatic drain.
  /// @param[in] budget Separate finite allowance; harvest RXFS/MISR2 before writing W0C zero.
  /// @param[out] token Assigned only on admission; acknowledgement is a retained operation.
  /// @return IN_PROGRESS or validation/ownership error; cancel issues no cleanup I/O.
  /// @note Requires owned verified level policy. Neutral unrelated writes use one.
  /// Completed acknowledgement proves command acceptance, not an awakened host or deasserted pin.
  Status acknowledgeWake(const OperationBudget& budget, OperationToken& token) noexcept;
  /// @brief Acquire control, owned match words and collector RXFS/MISR2 once. On DP83825I password access requires a Secure-ON profile.
  /// @param[out] snapshot Assigned only on complete success; cache retains partials on failure.
  /// @param[in] budget Exclusive deadline and finite frame allowance for the full pass.
  /// @return Terminal transport/admission status; destructive uncertainty remains in the collector.
  Status readWakeStatus(WakeSnapshot& snapshot, const IoBudget& budget) noexcept;
  /// @brief Bus/clock-silent copy of WoL intent and observations.
  /// @return Historical/partial values; trust includes any owned LED1 clock dependency.
  WakeSnapshot wakeSnapshot() const noexcept;

  /// @brief Set forced TX LPI bit 12 and verify it; SNLS647I 8.3.4/7.6, OP-9.
  /// @param[in] options Explicit MAC drain and maintenance admission.
  /// @param[out] result Unchanged on preflight rejection; diagnostic output on admitted failure too.
  /// @param[in] budget Aggregate exclusive deadline and at least nine frames.
  /// @return OK proves bit 12 readback only; failures preserve accepted/uncertain effects.
  /// @note Requires current cached EEE 100 negotiation, resolved 100M link and normal power.
  /// One five-frame RMW and one four-frame readback; no wait, retry or operation token.
  /// Slot stays idle during dwell; OP-9 blocks unrelated mutations. Owner cancellation
  /// before entry means not calling; after entry use a separately budgeted stopLpi.
  Status startLpi(const LpiOptions& options, LpiResult& result, const IoBudget& budget) noexcept;
  /// @brief Clear and verify bit 12 only, preserving all other EEECFG3 bits.
  /// @param[out] result Diagnostic result, with the same publication rule as startLpi.
  /// @param[in] budget Separate nine-frame/two-callback aggregate allowance.
  /// @return OK proves the request cleared, not partner wake or MAC readiness.
  /// @note Requires retained session ownership; stale entry prerequisites do not prevent exit.
  /// Failure retains UNCERTAIN and OP-9 debt. No callback reentry or automatic cleanup.
  Status stopLpi(LpiResult& result, const IoBudget& budget) noexcept;
  /// @brief Bus/clock-silent copied evidence; serialize with every other instance access.
  /// @return Current request evidence and last admitted call result; no I/O.
  LpiSnapshot lpiSnapshot() const noexcept { return _lpi; }


  /// @brief Copy exact-part mode evidence without bus or clock access.
  /// @return Last desired mode and table prefix. Electrical reach needs a separate measurement.
  FunctionalModeSnapshot functionalModeSnapshot() const noexcept { return _functionalMode; }
  /// @brief Take a terminal operation with the frozen mode evidence from that operation.
  /// @param[in] token Pending result token.
  /// @param[out] result Common operation receipt, updated only on success.
  /// @param[out] mode Mode prefix at termination, updated only on success.
  /// @return Same status and ownership transfer as the two-argument takeResult.
  Status takeResult(OperationToken token, OperationResult& result, FunctionalModeSnapshot& mode) noexcept;

  /// @brief Start one bounded power-policy/transition operation; bus and clock silent.
  /// @param settings Copied complete intent. Maintenance and management clock admission required;
  /// clock-loss approval required for IEEE down or PLL changes. DP83825I deep down requires a recovery route.
  /// @param budget Aggregate deadline/frame/stall bounds; no partner-dependent sleep wait.
  /// @param[out] token New lifetime token only on admission; unchanged on rejection.
  /// @return IN_PROGRESS or local validation/ownership error. Poll uses one complete batch;
  /// failed/uncertain writes retain OP9 debt. Wake writes once, then reports blocked readback.
  /// @note Other owned persistent controls must stay verified through completion.
  /// SNLS647I 8.3.6/table 8-19/35/40. Isolation is preserved. No GPIO/rail operation.
  Status startConfigurePower(const PowerSettings& settings, const OperationBudget& budget, OperationToken& token) noexcept;
  /// @brief Acquire BMCR/PHYSCR/BISCR and the applicable DP83825I deep-power control under one allowance.
  /// @param[out] snapshot Unchanged on failure; passive snapshot retains partial words as stale.
  /// @param budget Aggregate terminal allowance; no resampling or partner wait.
  /// @return OK or transport/admission error. BISCR uses the existing collector.
  /// @note A valid reset word freezes a running operation as DEVICE_INVALIDATED;
  /// this acquisition still retains its own transport error if a later read fails.
  Status readPowerStatus(PowerSnapshot& snapshot, const IoBudget& budget) noexcept;
  /// @brief Return copied power intent and evidence without I/O; serialize with the owner.
  /// @return Copied intent, raw validity, trust and indication; never physical wake proof.
  PowerSnapshot powerSnapshot() const noexcept { return _power; }
  /// @brief Admit complete opt-in interrupt policy without bus/clock callbacks.
  /// @param[in] settings Valid masks, source and explicit board/pin permissions.
  /// @param[in] budget Finite primary allowance; cancellation leaves OP-9 restoration debt.
  /// @param[out] token New retained operation token on admission only.
  /// @return IN_PROGRESS or local syntax/lifecycle/capability error, unchanged token on rejection.
  /// @note Polls preserve consumed events and verify all owned controls. Initialize
  /// replays owned policy; startVerify reads only. Needed LED mux changes occur
  /// after disabling global delivery, with masked readback (SNLS647I 8-75/76).
  /// Other owned policies must stay verified; contradictory inter-poll selector,
  /// reset or identity observations prevent obsolete proof from completing.
  Status startConfigureInterrupts(const InterruptSettings& settings, const OperationBudget& budget,
                                  OperationToken& token) noexcept;
  /// @brief Start retained PHYSCR 2 software stimulus; requires verified owned IRQ policy.
  /// @param[in] budget Finite operation allowance; application owns observation duration.
  /// @param[out] token New token only on admission.
  /// @return IN_PROGRESS or local error. No callbacks during start.
  /// @note Table 8-35. Success proves register readback, not pin or MCU delivery.
  /// Cancel is bus-silent; separately call stopInterruptTest after taking the result.
  Status startInterruptTest(const OperationBudget& budget, OperationToken& token) noexcept;
  /// @brief Admit explicit clear/readback of retained test stimulus, with a new budget/result.
  /// @param[in] budget Finite restoration allowance, separate from the primary result.
  /// @param[out] token New token on admission only.
  /// @return IN_PROGRESS or local error. Failed stop keeps accepted effects and OP-9 debt.
  Status stopInterruptTest(const OperationBudget& budget, OperationToken& token) noexcept;
  /// @brief Copy policy and test evidence without MDIO or clock access.
  /// @return Desired/raw facts and trust including owned LED1 clock routing, not pin level.
  InterruptSnapshot interruptSnapshot() const noexcept;
  /// @brief Service once in owner context: PHYSTS, both MISRs, then requested half-full counters.
  /// @param[out] snapshot Published only on complete success; unchanged on failure.
  /// @param[in] budget Maximum callbacks/samples and aggregate physical frames/deadline.
  /// @return OK or bounded transport/admission error; no retries or drain loop.
  /// @note Partial consumed samples remain in the single collector. ISR only signals
  /// the application owner; reschedule if morePossiblyPending, without requiring a new edge.
  Status serviceInterrupts(InterruptServiceSnapshot& snapshot, const InterruptServiceBudget& budget) noexcept;
  /// @brief Copy the last complete service and its freshness/error without consuming events.
  /// @return Passive value; repeated cached readers create no new notifications.
  InterruptServiceSnapshot interruptServiceSnapshot() const noexcept { return _interruptService; }

  /// @brief Project passive health without bus/clock access or changing admission.
  /// @return UNINIT when not initialized; otherwise READY at zero failures,
  /// DEGRADED below offlineThreshold, or OFFLINE at/above it.
  /// @note READY does not establish link, verified settings or application readiness.
  DriverState driverState() const noexcept;
  /// @brief Copy managed health evidence without bus/clock callbacks.
  /// @return Counts and retained error/time evidence with the current derived state.
  /// @note Initialization, configuration and live link callbacks are managed.
  /// Raw/probe and explicit event/register maintenance are transport-only.
  /// Late clean callbacks remain successes; clock/deadline and semantic failures
  /// are reported separately. A later managed restoration can reset the streak,
  /// but cannot replace an earlier operation result or erase lastError.
  HealthSnapshot healthSnapshot() const noexcept;
  /// @brief Clear only cumulative health success/failure totals, without I/O.
  /// @note Preserves streak, state, initialization, error/times, transport and chip
  /// counts, configuration trust, collector history and pending operation results.
  /// Like every instance call, this requires the serialized application owner.
  void resetStatistics() noexcept;

  /// @brief Admit a complete negotiated or forced profile without MDIO or clock callbacks.
  /// @param[in] settings AN advertisement or forced 10/100 half/full, coupled routing/mirror and extension.
  /// @param[in] options Optional bounded Clause 28 exchange/completion wait.
  /// @param[in] budget Exclusive aggregate deadline, frame and stall allowances.
  /// @param[out] token Assigned only on admission; result must be taken before reuse.
  /// @return IN_PROGRESS on admission; invalid intent, unavailable feature, binding,
  /// operation/barrier, initialization or budget errors otherwise, with no changes.
  /// @note SNLS647I 8.3.1/.2, tables 8-19/23..30/43; IEEE 802.3 Clause 28.
  /// Forced mode on DP83826I/E requires robust routing, no AN intent/options, and never accesses
  /// ANAR. Automatic routing preserves PHYCR 14; mirror sets both CR3 bits 0060.
  /// Extension owns CR2 bit 0020 only, rejects forced 10 and proves no partner agreement.
  /// Admission copies desired intent and marks trust DIRTY. NP belongs only to
  /// this exchange. Failure/cancel stops without cleanup writes; restoration is
  /// a new initialization or eligible configuration operation. No VOD write here.
  /// A valid BMCR reset indication during preflight, RMW or verification stops
  /// the operation with INVALID_CONFIG/DEVICE_INVALIDATED unless transport
  /// already failed. Accepted/possible writes retain their cleanup obligation.
  /// @note Without owned power intent, live preflight rejects BMCR power-down with INVALID_CONFIG/POWER_STATE
  /// before profile writes. Table 8-19 combines software request and pin state;
  /// later pin assertion is never echoed into the software request. This does
  /// not prove physical wake, and external writers remain excluded by the owner.
  Status startConfigure(const LinkSettings& settings, const ConfigureOptions& options,
                        const OperationBudget& budget, OperationToken& token) noexcept;
  /// @brief Apply registers without waiting for negotiation or exchanging next pages.
  /// @param[in] settings Validated negotiated intent.
  /// @param[in] budget Finite operation budget.
  /// @param[out] token Admitted operation identity.
  /// @return Same admission contract as the options overload.
  Status startConfigure(const LinkSettings& settings, const OperationBudget& budget,
                        OperationToken& token) noexcept {
    return startConfigure(settings, ConfigureOptions{}, budget, token);
  }
  /// @brief Copy desired/observed/trust evidence without bus or clock callbacks.
  /// @return Historical fields retain explicit validity and staleness.
  SettingsSnapshot settingsSnapshot() const noexcept { return _settings; }

  /// @brief Acquire current link in exactly ten reads, as one eight- and one two-frame batch.
  /// @param[out] snapshot Assigned only on complete coherent success; unchanged on every error.
  /// @param[in] budget Exclusive deadline and at least ten frames; both batch bounds reserved upfront.
  /// @return OK also for cable down, BUSY/OBSERVATION_CHANGED for inconsistent bookends,
  /// or binding, initialization, owner, clock, budget or exact transport error/detail.
  /// @pre Serialize all instance access and all bus participants; initialized accepted identity required.
  /// @note SNLS647I tables 8-19/20/23..25/29/34/43 and IEEE 802.3 Table 28B-3.
  /// Reads BMSR twice, PHYSTS, BMCR, ANAR, ALNPAR, ANER, PHYCR, BMSR, PHYSTS.
  /// Destructive words belong to the existing collector, even on partial failure.
  /// No retry or implicit configuration. OP-14 reset waits/quiet floors and an
  /// active next-page consumer deny this read; semantic writes in a running
  /// operation suppress resolved mode/pause. Raw forced PHYSTS remains unqualified.
  /// Failed refresh, including zero-I/O budget or controller-quarantine rejection, retains old values/time
  /// as stale with the returned error. Local owner/lifecycle rejection is not a refresh.
  /// Use observedSinceUs and an owner-supplied current upper time for maximum age.
  Status readLinkStatus(LinkSnapshot& snapshot, const IoBudget& budget) noexcept;
  /// @brief Copy link, change counters and historical freshness without bus/clock I/O.
  /// @return Last observation and refresh status; caller policy decides acceptable age/mode.
  LinkSnapshot linkSnapshot() const noexcept { return _link; }

  /// @brief Request AN restart in one terminal batch without waiting for negotiation.
  /// @param[in] budget Absolute deadline and allowance for the complete three-frame batch.
  /// @return OK proves completed command write and stable control readback only; not link-up.
  /// @pre Initialized, verified owned AN profile, awake normal mode, no active/result/cleanup slot.
  /// @note Failed or late writes preserve effects and invalidate trust; no retry, token or wait.
  Status restartAutoNegotiation(const IoBudget& budget) noexcept;

  /// @brief Maintain only raw BMCR AN/speed/duplex bits in one atomic read/write.
  /// @param[in] mask Nonzero subset of 0x3100; no reset, power, loopback or selector commands.
  /// @param[in] value Sixteen-bit replacement subset of mask, not persistent desired intent.
  /// @param[in] budget Absolute deadline and at least two frames.
  /// @param[out] invoked Optional flag cleared on entry. True identifies this call's lastBatch.
  /// @pre Initialized accepted exact part, awake normal profile, stopped application traffic.
  /// On DP83826I/E, explicitly enable AN or supply current verified readback showing
  /// that AN stays enabled or CR1 robust crossover is enabled.
  /// @return Validation/admission/transport status; successful or uncertain write invalidates
  /// identity/readiness/trust. Reinitialize/replay before managed use; no automatic retry.
  /// @note SNLS647I tables 8-19/8-28. The write neutralizes RESET/RESTART and never echoes the power-pin OR.
  Status modifyBasicControl(uint32_t mask, uint32_t value, const IoBudget& budget, bool* invoked = nullptr) noexcept;

  /// @brief Acquire and accept identity at the bound address in one two-read batch.
  /// @param[out] identity Assigned only after complete in-budget transport and identity acceptance.
  /// @param[in] budget Aggregate microsecond deadline and at least two frames.
  /// @param[out] invoked Optional flag cleared on entry. True identifies this call's lastBatch.
  /// @return Transport/admission errors unchanged; DEVICE_NOT_FOUND for either
  /// required word zero/all-ones; DEVICE_MISMATCH for OUI/model/revision/mode,
  /// checked in that order. No error implies physical absence or an MDIO ACK.
  /// @note SNLS647I tables 8-21/22: declared DP83826I/E, OUI 080028, model 11/13,
  /// revision 1. Shared IDs cannot identify I/E suffix. Partial/rejected evidence
  /// stays in identitySnapshot; never combine words from different calls.
  /// SNLS505H tables 8-3/4: DP83822I/H/IF/HF accept 2000 A240 with UNKNOWN
  /// operating mode. Board declaration supplies the suffix and medium.
  /// @pre Bound, serialized owner with clock/power/reset readiness established.
  /// Uses the ordinary raw-read guard; no reset, scan, write or retry is performed.
  Status readIdentity(DeviceIdentity& identity, const IoBudget& budget, bool* invoked = nullptr) noexcept;
  /// @brief Read raw PHYIDR1/2 at an explicit bus address without accepting or retargeting a device.
  /// @param[in] phyAddress Clause 22 address 0..31; all other values reject before callbacks.
  /// @param[out] id1 First sixteen-bit ID word, assigned only on complete success.
  /// @param[out] id2 Second ID word, also unchanged on failure.
  /// @param[in] budget Exclusive absolute deadline and at least two frames; one terminal batch.
  /// @param[out] invoked Optional callback receipt flag, cleared on entry even on rejection.
  /// True means transportSnapshot().lastBatch belongs to this attempt, including failures.
  /// @return Validation/admission, controller, clock or deadline status; OK accepts no silicon identity.
  /// @pre Bound, serialized and idle, without a retained result, cleanup or temporary session.
  /// @note Counts RAW transport traffic and preserves partial backend evidence; never changes
  /// the bound address, accepted identity, configuration trust or managed-health counters.
  Status readIdentityAt(uint32_t phyAddress, uint16_t& id1, uint16_t& id2,
                        const IoBudget& budget, bool* invoked = nullptr) noexcept;
  /// @brief Copy latest identity evidence without MDIO or clock access.
  /// @return Historical/partial fields with explicit validity, admission and staleness.
  DeviceIdentity identitySnapshot() const noexcept { return _identity; }
  /// @brief Query accepted source capabilities without MDIO or clock access.
  /// @return Invalid until a fresh identity is accepted; no link/settings/qualification claim.
  Capabilities capabilities() const noexcept;

  /// @brief Capture both DP83822 strap latches under one controller lock.
  /// @pre Serialized idle owner, fresh accepted DP83822 identity and board-established SMI readiness.
  /// @param[in] budget Exclusive absolute deadline in microseconds and at least eight MDIO frames.
  /// @param[out] invoked Optional flag set when the terminal batch callback runs.
  /// @return OK publishes a current pair in strapSnapshot(). A local denial preserves the cache.
  /// A failed callback retains only the completed words from this attempt as stale evidence.
  /// @note Uses vendor MMD selectors. Active medium and LED_1 strap mode require separate evidence.
  Status captureDp83822Straps(const IoBudget& budget, bool* invoked = nullptr) noexcept;

  /// @brief Return the cached strap capture under the application owner without I/O.
  /// @return Independently valid SOR words for this binding and reset epoch.
  StrapEvidence strapSnapshot() const noexcept { return _straps; }

  /// @brief Read one nonconsuming Clause 22 word using one terminal owned batch.
  /// @param[in] registerAddress Five-bit address, validated before narrowing.
  /// @param[out] value Assigned only on complete valid in-budget success.
  /// @param[in] budget Original exclusive microsecond deadline and frame allowance.
  /// @param[out] invoked Optional flag cleared on entry. True identifies this call's lastBatch.
  /// @return INVALID_PARAM for width/access, NOT_BOUND, BUSY, clock/configuration
  /// error, transport failure or OPERATION_TIMEOUT; never IN_PROGRESS.
  /// @note SNLS647I tables 8-19..48. Selectors, holes, consuming reads and CDCR
  /// (resistor-calibration follow-up obligation) are rejected before callbacks.
  /// Status words may change asynchronously; a raw word is not a coherent link result.
  /// A valid active-reset observation invalidates affected current evidence even
  /// if release/timing fails; it interrupts an interleaved operation, not a frozen result.
  Status readClause22(uint32_t registerAddress, uint16_t& value,
                      const IoBudget& budget, bool* invoked = nullptr) noexcept;
  /// @brief Write the safe maintenance subset in one terminal owned batch.
  /// @param[in] registerAddress Only LEDCR (0x18) is currently admitted.
  /// @param[in] value Sixteen-bit word; only mask 0x0692 may be nonzero.
  /// @param[in] budget Original exclusive microsecond deadline and frame allowance.
  /// @param[out] invoked Optional flag cleared on entry. True identifies this call's lastBatch.
  /// @pre Application establishes exact part/address and safe LED0 electrical use,
  /// and serializes maintenance. Binding alone does not verify those facts.
  /// @return Direct-access errors or the retained failed identity status; OK is
  /// frame completion, not verified pin state. Failed/late writes may have applied.
  /// @note SNLS647I table 8-42: blink, polarity, drive enable/value; reserved zero.
  /// No cached setting is claimed. Copy transportSnapshot before another batch.
  /// Replaces all four fields: unset bits select 20 Hz blink and active-low
  /// polarity, overriding the strap. Use modifyMmd to change one field.
  Status writeClause22(uint32_t registerAddress, uint32_t value,
                       const IoBudget& budget, bool* invoked = nullptr) noexcept;
  /// @brief Read one admitted nonconsuming MMD word in one four-frame batch.
  /// @param[in] devad Device selector; vendor 31 plus the three source-implied EEE reads.
  /// @param[in] registerAddress Sixteen-bit documented offset; no indirect selectors.
  /// @param[out] value Unchanged unless all frames, release and timing succeed.
  /// @param[in] budget Original exclusive deadline and at least four frames.
  /// @param[out] invoked Optional flag cleared on entry. True identifies this call's lastBatch.
  /// @return Direct-access terminal errors; INVALID_PARAM for holes, consuming reads
  /// or unadmitted DEVAD/offsets. EEE mappings remain physically unqualified.
  /// @note SNLS647I 8.3.11.1/.5, tables 8-31/32. Clause 22 aliases inherit the
  /// same policy. Raw TDR/MSE/strap words do not establish freshness or units.
  /// Vendor reset aliases retain the same invalidation effects as readClause22.
  Status readMmd(uint32_t devad, uint32_t registerAddress, uint16_t& value,
                 const IoBudget& budget, bool* invoked = nullptr) noexcept;
  /// @brief Write one admitted vendor-MMD word in one four-frame batch.
  /// @param[in] devad Device selector; only 31 is admitted for mutation.
  /// @param[in] registerAddress Currently only the LEDCR alias (0x0018).
  /// @param[in] value Sixteen-bit word; reserved bits must be zero.
  /// @param[in] budget Original exclusive deadline and at least four frames.
  /// @param[out] invoked Optional flag cleared on entry. True identifies this call's lastBatch.
  /// @pre Same exact-part, maintenance and pin-use preconditions as writeClause22.
  /// @return Terminal status; errors retain selector/possible target effects.
  /// @note SNLS647I 8.3.11.4 and table 8-42. Always reselects the address;
  /// no retry or interpretation of a prior ambiguous write as verified success.
  /// Replaces all four fields: unset bits select 20 Hz blink and active-low
  /// polarity, overriding the strap. Use modifyMmd to change one field.
  Status writeMmd(uint32_t devad, uint32_t registerAddress, uint32_t value,
                  const IoBudget& budget, bool* invoked = nullptr) noexcept;
  /// @brief Modify safe stable fields under one five-frame MMD transaction.
  /// @param[in] devad Device selector; only 31 is admitted for mutation.
  /// @param[in] registerAddress Currently only the LEDCR alias (0x0018).
  /// @param[in] mask Nonzero sixteen-bit subset of LEDCR's documented stable bits.
  /// @param[in] value Sixteen-bit replacement bits; must be a subset of mask.
  /// @param[in] budget Original exclusive deadline and at least five frames.
  /// @param[out] invoked Optional flag cleared on entry. True identifies this call's lastBatch.
  /// @pre Owner excludes reset/external writers and meets writeClause22 preconditions.
  /// @return Terminal status; a failed or late dependent write may have applied.
  /// @note Five frames are inferred from SNLS647I 8.3.11.1, not a printed sequence.
  /// Nonincrementing selection persists through
  /// the read and following write. Preserve only stable bits outside mask, force
  /// LEDCR reserved bits zero. Other targets require per-register reserved handling.
  /// Never replay commands or update mixed/destructive fields through generic RMW.
  /// No readback verification, automatic retry or separate public lock scope.
  Status modifyMmd(uint32_t devad, uint32_t registerAddress, uint32_t mask,
                   uint32_t value, const IoBudget& budget, bool* invoked = nullptr) noexcept;
  /// @brief Explicitly acquire destructive words through the sole private collector.
  /// @param[in] sourceMask Common ALL_EVIDENCE bits and optional DP83825I REGISTER_12. No implicit sources.
  /// @param[out] evidence Assigned only when every requested read/release succeeds on time.
  /// @param[in] budget Aggregate deadline and frames; complete requested count admitted first.
  /// @param[out] invoked Optional flag set true if this call invokes the transport for any batch.
  /// @pre Application owns the instance, exact part/address and diagnostic admission.
  /// @return Terminal transport/validation status; errors retain consumed partial samples.
  /// @note PHYSTS is read first when requested. RXFS uses a complete MMD sequence.
  /// Reads can clear IRQ/history; ambiguous reads set historyLost, never zero events.
  /// A collection spans bounded batches; it is not a simultaneous hardware snapshot.
  Status acquireEvidence(uint32_t sourceMask, EvidenceSnapshot& evidence,
                         const IoBudget& budget, bool* invoked = nullptr) noexcept;
  /// @brief Update only MISR1/2 stable enables while retaining consumed events.
  /// @param[in] registerAddress C22 MISR1 (0x12) or MISR2 (0x13).
  /// @param[in] mask Nonzero low-byte owned mask.
  /// @param[in] value Replacement subset of mask.
  /// @param[in] budget Two-frame atomic read/write budget.
  /// @pre Exact-part maintenance owner coordinates interrupt users and pin routing.
  /// @return Terminal status; partial reads remain in evidenceSnapshot on write failure.
  /// @note Tables 8-36/37. This is register maintenance, not IRQ pin setup/service.
  Status modifyInterruptEnables(uint32_t registerAddress, uint32_t mask,
                               uint32_t value, const IoBudget& budget) noexcept;
  /// @brief Maintain RXFCFG1 with exact W0C semantics and pre-write event capture.
  /// @param[in] mask Stable subset of 0x17A1; zero allowed only for acknowledgement.
  /// @param[in] value Replacement subset of mask; full pulse-width field when changed.
  /// @param[in] acknowledge True writes zero to bit 11; false preserves pending level.
  /// @param[in] budget Aggregate allowance of at least 14 frames; no hidden retries.
  /// @pre Exact DP83826I/E, quiesced traffic, safe wake wiring and exclusive semantic
  /// configuration owner. This does not configure a MAC/password or qualify WoL.
  /// @return Terminal status; failed writes leave configuration evidence dirty.
  /// @note Table 8-90. Read stable state, skip unchanged unrelated updates, capture
  /// RXFS/MISR2, then perform one atomic five-frame RMW. Unknown bit 6 and reserved
  /// bits are preserved. Full feature admission/restoration belongs to prompt 31.
  Status modifyWakeControl(uint32_t mask, uint32_t value, bool acknowledge,
                          const IoBudget& budget) noexcept;
  /// @brief Read retained samples without MDIO or clock callbacks.
  /// @return Copied history; valid/timeValid/stale/historyLost are independent.
  EvidenceSnapshot evidenceSnapshot() const noexcept { return _evidence; }
  /// @brief Collect PHYSTS, FLDS, MISR1, MISR2, FCSCR and RECR once in one six-read batch.
  /// @param[out] snapshot Assigned only on complete in-budget success; unchanged on error.
  /// @param[in] budget Exclusive aggregate deadline and at least six frames.
  /// @param[out] invoked Optional flag set true when this call invokes the transport.
  /// @return Same admission/errors as acquireEvidence; partial receipts remain immediately cached.
  /// @pre Serialized exact-part diagnostic owner, including all other controller users.
  /// @note Tables 8-33..39. No implicit RXFS read, drain, retry or counter reset.
  /// FCSCR FF is a lower bound; RECR remains raw under Q16. No MDIO-health accounting claim.
  Status collectEvents(CounterSnapshot& snapshot, const IoBudget& budget,
                       bool* invoked = nullptr) noexcept;
  /// @brief Copy last complete event/counter aggregate without bus/clock callbacks.
  /// @return Historical values with explicit validity/staleness; partials are in evidenceSnapshot.
  CounterSnapshot counterSnapshot() const noexcept { return _counters; }
  /// @brief Discard retained host collector history without reading or clearing hardware.
  /// @return OK, NOT_BOUND, or BUSY during callbacks/polling/a running operation.
  /// @note Owner copies any needed evidence first. Clears all sample sums/flags and
  /// counter aggregate; preserves link, trust, transport, identity and operation results.
  /// The next hardware sample can include events predating this local history clear.
  Status clearEvidence() noexcept;
  /// @brief Invalidate configuration and mark observations historical without I/O.
  /// @param[in] reason Known external/raw uncertainty; invalid enum is rejected.
  /// @return OK or INVALID_PARAM; BUSY during a callback; samples are never erased.
  /// @note RUNNING freezes DEVICE_INVALIDATED; a pending historical result stays immutable.
  /// EXTERNAL_RESET/EXTERNAL_POWER_ON clear current cleanup by owner attestation.
  /// Every reason withdraws initialized state and current identity capabilities.
  Status invalidateDeviceState(InvalidationReason reason) noexcept;
  /// @brief Copy retained raw/normalized evidence without clock or MDIO callbacks.
  /// @return Copied report; raw invalid timestamps/statuses must not be trusted.
  TransportSnapshot transportSnapshot() const noexcept { return _transport; }

private:
  enum class PinStep : uint8_t { INTERFACE, CLOCK_STATUS, MUX0, MUX3, APPLY, VERIFY, COMPLETE };
  enum class AnalogStep : uint8_t { PREFLIGHT, SNAPSHOT, BRIDGE_WRITE, BRIDGE_VERIFY, APPLY, VERIFY, COMPLETE };
  AnalogSnapshot _analog{};
  uint16_t _vodTrimWord{0}, _appliedVodTrim{0};
  bool _vodTrimAccepted{false};
  AnalogStep _analogStep{AnalogStep::COMPLETE};
  uint8_t _analogRow{0};
  bool _analogWorking{false}, _analogTailPending{false}, _analogVerifyOnly{false};
  std::array<uint16_t,12> _analogWords{};
  bool _dspModeConflict(FunctionalMode mode, const AnalogSettings& settings) const noexcept;
  Status _validateAnalog(const AnalogSettings& settings) const noexcept;
  void _beginAnalog(bool verifyOnly) noexcept;
  MdioBatch _analogBatch() const noexcept;
  Status _advanceAnalog(Status status, bool called, const MdioBatch& batch) noexcept;
  void _retainOwnedMode(uint16_t value) noexcept;
  void _retainAnalogWord(uint16_t address, uint16_t value) noexcept;
  bool _analogRowValue(uint8_t row, uint16_t& address, uint16_t& mask, uint16_t& value) const noexcept;
  void _nextAnalogRow() noexcept;
  Status _finishAnalog(Status status) noexcept;
  uint16_t _analogVod3() const noexcept;
  enum class ElectricalStep : uint8_t { PREFLIGHT, COLLISION_WRITE, COLLISION_VERIFY, STIMULUS_WRITE, STIMULUS_VERIFY,
    COUNT_CLEAR, COUNT_VERIFY_CLEAR, COUNT_SELECT, COUNT_VERIFY_SELECT, COUNT_ENABLE, COUNT_VERIFY_ENABLE,
    SCRAMBLER_WRITE, SCRAMBLER_VERIFY, CAL_READ, CAL_PAIR, COMPLETE };
  ElectricalSnapshot _electrical{};
  ElectricalStep _electricalStep{ElectricalStep::COMPLETE};
  bool _electricalWorking{false}, _electricalRestart{false};
  LinkSettings _electricalProfile{};
  Status _admitElectrical(const OperationBudget& budget, bool stop) const noexcept;
  void _setElectricalStep(ElectricalStep step) noexcept;
  void _beginComplianceActivation() noexcept;
  MdioBatch _electricalBatch() const noexcept;
  Status _advanceElectrical(Status status, bool called, const MdioBatch& batch) noexcept;
  Status _finishElectricalReset() noexcept;
  void _retainElectricalEffects(const MdioBatch& batch, bool called) noexcept;
  enum class QualityStep : uint8_t { PREFLIGHT, APPLY, VERIFY, COMPLETE };
  QualitySnapshot _quality{};
  SignalQualitySnapshot _signalQuality{};
  QualityStep _qualityStep{QualityStep::COMPLETE};
  uint8_t _qualityRow{0};
  bool _qualityWorking{false}, _qualityTailPending{false}, _qualityVerifyOnly{false};
  Status _validateQuality(const QualitySettings& settings) const noexcept;
  void _beginQuality(bool verifyOnly) noexcept;
  MdioBatch _qualityBatch() const noexcept;
  Status _advanceQuality(Status status, bool called, const MdioBatch& batch) noexcept;
  void _retainQualityWord(uint16_t address, uint16_t value) noexcept;
  void _qualityPassCost(bool verifyOnly, uint32_t& frames, uint32_t& callbacks) const noexcept;
  bool _qualityRowValue(uint8_t row, uint16_t& address, uint16_t& mask, uint16_t& value,
                        bool readback = false) const noexcept;
  void _nextQualityRow() noexcept;
  Status _finishQuality(Status status) noexcept;
  enum class InterfaceStep : uint8_t { PREFLIGHT, APPLY, VERIFY, FOLLOWER_TIMING, CLOCK_STATUS, COMPLETE };
  InterfaceSnapshot _interface{};
  InterfaceResult _interfaceResult{};
  InterfaceStep _interfaceStep{InterfaceStep::COMPLETE};
  uint8_t _interfaceRow{0};
  bool _p25TimingApplied{false};
  bool _interfaceWorking{false}, _interfaceDirect{false}, _interfaceTailPending{false}, _interfaceVerifyOnly{false};
  Status _validateInterface(const InterfaceSettings& settings) const noexcept;
  void _beginInterface(bool verifyOnly) noexcept;
  MdioBatch _interfaceBatch() const noexcept;
  Status _advanceInterface(Status status, bool called, const MdioBatch& batch) noexcept;
  void _retainInterfaceWord(uint16_t address, uint16_t value) noexcept;
  void _interfacePassCost(bool verifyOnly, uint32_t& frames, uint32_t& callbacks) const noexcept;
  bool _interfaceRowValue(uint8_t row, uint16_t& address, uint16_t& mask, uint16_t& value) const noexcept;
  void _nextInterfaceRow() noexcept;
  Status _finishInterface(Status status) noexcept;
  PinSnapshot _pins{};
  PinResult _pinResult{};
  PinStep _pinStep{PinStep::COMPLETE};
  uint8_t _pinRow{0};
  bool _pinsWorking{false}, _pinsDirect{false}, _pinsTailPending{false}, _pinsVerifyOnly{false};
  Status _validatePins(const PinSettings& settings) const noexcept;
  void _beginPins(bool verifyOnly) noexcept;
  MdioBatch _pinsBatch() const noexcept;
  Status _advancePins(Status status, bool called, const MdioBatch& batch) noexcept;
  void _retainPinWord(uint16_t address, uint16_t value) noexcept;
  bool _pinRowValue(uint8_t row, uint16_t& address, uint16_t& mask, uint16_t& value, bool verifyOnly) const noexcept;
  void _pinPassCost(bool verifyOnly, uint32_t& frames, uint32_t& callbacks) const noexcept;
  void _wakePassCost(bool verifyOnly, uint32_t& frames, uint32_t& callbacks) const noexcept;
  void _interruptPassCost(bool verifyOnly, uint32_t& frames, uint32_t& callbacks) const noexcept;
  void _addOwnedReplayCost(uint32_t& frames, uint32_t& callbacks) const noexcept;
  void _analogReplayCost(uint32_t& frames, uint32_t& callbacks) const noexcept;
  void _nextPinRow() noexcept;
  Status _finishPins(Status status) noexcept;
  enum class WakeStep : uint8_t { IDENTIFY, PIN_CLOCK, PIN_MUX, PIN_APPLY, PIN_VERIFY, READ_CONTROL, READ_CREDENTIAL,
    HARVEST_DISABLE, DISABLE, VERIFY_DISABLE, WRITE_CREDENTIAL, VERIFY_CREDENTIAL,
    HARVEST_APPLY, APPLY, VERIFY_CONTROL, READ_SELECTOR, DRAIN_SELECTOR, HARVEST_ACK, ACK };
  WakeSnapshot _wake{};
  WakeStep _wakeStep{WakeStep::IDENTIFY};
  uint8_t _wakeWord{0};
  bool _wakeCredentialsChanged{false};
  bool _wakePinNeedsApply{false};
  OperatingMode _wakeExpectedMode{OperatingMode::UNKNOWN};
  bool _wakeWorking{false}, _wakeTailPending{false}, _wakeVerifyOnly{false};
  void _setWakeStep(WakeStep step) noexcept;
  MdioBatch _wakeBatch() const noexcept;
  Status _advanceWake(Status status, bool called, const MdioBatch& batch) noexcept;
  Status _validateWake(const WakeSettings& settings) const noexcept;
  uint16_t _wakeControl() const noexcept;
  uint8_t _wakeCredentialWords() const noexcept;
  uint8_t _wakePin() const noexcept;
  uint16_t _wakePinFunction() const noexcept;
  void _retainWakeControl(uint16_t value) noexcept;
  void _retainWakeWord(uint16_t address, uint16_t value) noexcept;
  MdioBatch _wakeControlBatch(uint16_t mask, uint16_t value, bool acknowledge) const noexcept;
  enum class EeeStep : uint8_t { CAPABILITY, RECIPE, ADVERTISEMENT_CHECK, SET_ADVERTISEMENT,
    SET_CONTROL, VERIFY_ADVERTISEMENT, VERIFY_CONTROL, MODE_WRITE, MODE_WAIT, PCS_CHECK, PCS_CLOCK, PCS_VERIFY };
  FunctionalModeSnapshot _functionalMode{}, _terminalMode{};
  bool _modeAppliedThisPass{false}, _pcsClockChecked{false}, _pcsResetChecked{false};
  Status _admitFunctionalMode(const IoBudget& budget, uint64_t raw) const noexcept;
  Status _validateFunctionalMode(const InitializeOptions& options) const noexcept;
  bool _beginFunctionalModePass() noexcept;
  MdioBatch _functionalModeBatch() const noexcept;
  Status _advanceFunctionalMode(Status status, bool called, const MdioBatch& batch) noexcept;
  EeeSnapshot _eee{};
  EeeStep _eeeStep{EeeStep::CAPABILITY};
  uint8_t _eeeRow{1};
  LpiSnapshot _lpi{};
  bool _lpiSession{false};
  bool _lpiForceOwned{false};
  bool _lpiWorking{false};
  Status _setLpi(bool enabled, const LpiOptions& options, LpiResult& result, const IoBudget& budget) noexcept;
  bool _eeeWorking{false};
  bool _eeeRecipePending{false};
  bool _eeeCapabilityChecked{false};
  bool _eeeTailPending{false};
  bool _eeeResumeInit{false};
  bool _eeeOpaqueValid{false};
  Status _validateEee(const EeeSettings& settings) const noexcept;
  void _setEeeStep(EeeStep step) noexcept;
  MdioBatch _eeeBatch() const noexcept;
  Status _advanceEee(Status status, bool called, const MdioBatch& batch) noexcept;
  uint16_t _eeeControl() const noexcept;
  enum class PowerStep : uint8_t { READ_BMCR, READ_PHYSCR, CLEAR_PLL, VERIFY_CLEAR_PLL,
    POLICY, VERIFY_POLICY, BMCR, VERIFY_BMCR, PLL, VERIFY_PLL, BISCR, READ_DEEP, CLEAR_DEEP, VERIFY_CLEAR_DEEP, SET_DEEP, VERIFY_DEEP };
  PowerStep _powerStep{PowerStep::READ_BMCR};
  PowerSnapshot _power{};
  bool _powerWorking{false}, _powerTailPending{false}, _powerVerifyOnly{false}, _powerPreparingReset{false}, _powerPreparationDone{false};
  void _resumeAfterPowerPreparation() noexcept;
  uint16_t _softwarePowerWord() const noexcept;
  Status _validatePower(const PowerSettings& settings) const noexcept;
  void _setPowerStep(PowerStep step) noexcept;
  MdioBatch _powerBatch() const noexcept;
  Status _advancePowerPreflight() noexcept;
  Status _advancePower(Status status, bool called, const MdioBatch& batch) noexcept;
  void _retainPowerWord(uint8_t address, uint16_t value) noexcept;
  void _observePower() noexcept;
  enum class InterruptStep : uint8_t {
    IDENTIFY, PIN_CLOCK, PIN_MUX, PIN_APPLY, PIN_VERIFY, CHECK_TEST, DISABLE, DRAIN,
    SELECT, VERIFY_SELECT, CROSSING, MASK1, MASK2, OUTPUT, ENABLE,
    VERIFY1, VERIFY2, VERIFY_CONTROL, TEST_WRITE, TEST_VERIFY
  };
  InterruptStep _interruptStep{InterruptStep::IDENTIFY};
  InterruptSnapshot _interrupts{};
  InterruptServiceSnapshot _interruptService{};
  bool _interruptWorking{false}, _interruptTailPending{false}, _interruptVerifyOnly{false};
  bool _interruptTestSession{false};
  bool _interruptPinRestore{false};
  OperatingMode _interruptExpectedMode{OperatingMode::UNKNOWN};
  Status _admitInterrupt(const OperationBudget& budget, bool stop) const noexcept;
  Status _validateInterruptSettings(const InterruptSettings& settings) const noexcept;
  void _setInterruptStep(InterruptStep step) noexcept;
  MdioBatch _interruptBatch() const noexcept;
  Status _advanceInterrupt(Status status, bool called, const MdioBatch& batch) noexcept;
  uint16_t _interruptControlMask() const noexcept;
  uint16_t _interruptControlValue() const noexcept;
  enum class CableStep : uint8_t {
    IDENTIFY, BMCR, PHYRCR, CR1, BISCR, LINK, CDCR, SAVE,
    APPLY, VERIFY, TRIGGER, WAIT, INDEX, AMPLITUDE, PEAK, BOOKEND,
    STOP_IDENTITY, STOP_STATUS, STOP_COMMAND, STOP_VERIFY,
    RESTORE_BMCR, VERIFY_BMCR, RESTORE, VERIFY_RESTORE, POLICY_WRITE, POLICY_VERIFY
  };
  CableStep _cableStep{CableStep::BMCR};
  CableSnapshot _cable{};
  std::array<uint16_t, 7> _cableSaved{};
  uint16_t _cableSavedBmcr{0};
  uint8_t _cableRun{0}, _cableField{0};
  bool _cableSession{false}, _cableWorking{false}, _cableRestoreTail{false}, _cableCurrent{false};
  bool _cableCalibration{false};
  CableAutorunSnapshot _autorun{};
  bool _autorunTailPending{false};
  void _setCableStep(CableStep step) noexcept;
  MdioBatch _cableBatch() const noexcept;
  Status _advanceCable(Status status, bool called, const MdioBatch& batch) noexcept;
  Status _admitCable(const OperationBudget& budget, bool stop) const noexcept;
  uint16_t _cableValue(uint8_t field) const noexcept;
  enum class BistStep : uint8_t {
    SAVE_IPG, SAVE_PACKET, APPLY_IPG, APPLY_PACKET, VERIFY_IPG, VERIFY_PACKET,
    BASELINE, LINK, ENABLE, VERIFY_ENABLE, BEFORE, CAPTURE, AFTER,
    DISABLE, VERIFY_DISABLE, RESTORE_IPG, RESTORE_PACKET, VERIFY_RESTORE_IPG, VERIFY_RESTORE_PACKET
  };
  BistStep _bistStep{BistStep::SAVE_IPG};
  BistSnapshot _bist{};
  BistObservation _bistObservation{};
  uint16_t _bistSavedIpg{0}, _bistSavedPacket{0};
  uint64_t _bistDueUs{0};
  bool _bistSession{false}, _bistWorking{false}, _bistRestoreTail{false};
  bool _bistSavedIpgValid{false}, _bistSavedPacketValid{false};
  bool _bistActivity{false}, _bistLoss{false}, _bistCapturePending{false};
  bool _bistIntervalActivity{false}, _bistCapturedActivity{false};
  enum class LoopStep : uint8_t {
    IDENTIFY_STOP, READ_BMCR, READ_PHYRCR, READ_BISCR, READ_PHYSCR, CHECK_LINK,
    CLEAR_BMCR, CLEAR_BISCR, SET_FIFO, DIGITAL_BMCR, SET_BISCR, SET_MII,
    DIGITAL_0122, DIGITAL_0123, DIGITAL_0130, RESTART, WAIT_RESTART,
    VERIFY_BMCR, VERIFY_BISCR, VERIFY_ACTIVE_FIFO, RELINK, STOP_BMCR, STOP_BISCR,
    RESTORE_BMCR, RESTORE_BISCR, RESTORE_FIFO, VERIFY_STOP_BMCR, VERIFY_STOP_BISCR, VERIFY_FIFO
  };
  LoopStep _loopStep{LoopStep::READ_BMCR};
  LoopbackSnapshot _loopback{};
  LinkSettings _loopProfile{};
  uint16_t _loopSavedBiscr{0};
  uint16_t _loopSavedBmcr{0};
  uint16_t _loopSavedPhyscr{0};
  bool _loopSession{false};
  bool _loopWorking{false};
  bool _loopRestoreTail{false};
  const detail::PartProfile* _part{nullptr};
  Config _config{};
  StrapEvidence _straps{};
  HealthSnapshot _health{};
  TransportSnapshot _transport{};
  EvidenceSnapshot _evidence{};
  CounterSnapshot _counters{};
  DeviceIdentity _identity{};
  Status _identityRejection{}; ///< Last conclusive rejection until a successful probe or rebind.
  SettingsSnapshot _settings{};
  LinkSnapshot _link{};
  ConfigureOptions _configureOptions{};
  enum class ConfigStep : uint8_t {
    DRAIN, READ_ANAR, READ_BMCR, READ_CR1, READ_PHYCR, READ_CR3, READ_CR2,
    APPLY_CR1, APPLY_PHYCR, APPLY_CR3, APPLY_CR2, APPLY_ANAR, APPLY_BMCR,
    VERIFY_ANAR, VERIFY_CR1, VERIFY_PHYCR, VERIFY_CR3, VERIFY_CR2, VERIFY_BMCR,
    WAIT_BASE, READ_BASE, LOAD_PAGE, WAIT_PAGE, READ_PAGE, WAIT_COMPLETE, CLEAR_NP, VERIFY_CLEAR
  };
  ConfigStep _configStep{ConfigStep::DRAIN};
  bool _configuring{false};
  OperationKind _cleanupOwner{OperationKind::NONE};
  uint8_t _pageIndex{0};
  bool _partnerToggle{false};
  bool _localMore{false};
  bool _partnerMore{false};
  bool _pageEventPending{false};
  uint16_t _pageEventRaw{0};
  enum class InitStep : uint8_t {
    IDENTIFY, SOR1, SOR2, CHECK_RESET, CHECK_PHYRCR, COMMAND, WAIT_EXISTING, WAIT_PHYRCR, WAIT_COMMAND,
    REIDENTIFY, RESOR1, RESOR2, APPLY, VERIFY
  };
  OperationSnapshot _operation{};
  OperationResult _result{};
  OperationBudget _operationBudget{};
  InitStep _initStep{InitStep::IDENTIFY};
  uint64_t _tokenCounter{0};
  uint64_t _pollTimeUs{0};
  bool _pollTimeValid{false};
  bool _polling{false};
  bool _initialized{false};
  bool _quietPending{false};
  bool _bound{false};
  bool _active{false};
  bool _stopped{false};
  bool _timeValid{false};
  uint64_t _lastTimeUs{0};
  Status _sampleTime(uint64_t& raw, uint64_t& upper) noexcept;
  enum class Traffic : uint8_t { RAW, MANAGED, DP83822_STRAP_CAPTURE };
  Traffic _operationTraffic() const noexcept;
  bool _featureProcedureActive() const noexcept;
  Status _execute(MdioBatch batch, const IoBudget& budget, bool* called = nullptr,
                  bool operation = false, Traffic traffic = Traffic::RAW,
                  uint8_t identityAddress = 32, uint64_t passBoundUs = 0) noexcept;
  Status _readIdentity(DeviceIdentity& identity, const IoBudget& budget, bool operation,
                       bool* called = nullptr) noexcept;
  void _retainEvidence(const MdioBatch& batch, const BatchResult& result,
                       bool clockValid, uint64_t before, uint64_t upper) noexcept;
  void _invalidateEvidence() noexcept;
  void _markEvidenceDiscontinuous() noexcept;
  void _invalidateLink() noexcept;
  Status _dp83822FeatureGate() const noexcept;
  Status _checkMutation() noexcept;
  Status _checkToken(OperationToken token) const noexcept;
  bool _otherControlsVerified(OperationKind owner) const noexcept;
  bool _led1RouteVerified() const noexcept;
  void _beginOperation(OperationKind kind, const OperationBudget& budget) noexcept;
  Status _finishOperation(Status status, bool freshObservation = false) noexcept;
  Status _yieldOperation(uint64_t raw) noexcept;
  bool _beginPcsPass(InitStep step) noexcept;
  void _setInitStep(InitStep step) noexcept;
  Status _finishInitSettings() noexcept;
  Status _waitOperation() noexcept;
  bool _quietAt(uint64_t raw) noexcept;
  Status _checkQuiet() noexcept;
  Status _anchorQuiet() noexcept;
  uint32_t _commandWaitUs() const noexcept;
  Status _admitProfilePass(const IoBudget& budget, uint64_t upper) const noexcept;
  Status _admitResetStep(const IoBudget& budget, uint64_t raw) const noexcept;
  Status _admitEeeStep(const IoBudget& budget, uint64_t raw) const noexcept;
  static Status _unsupportedFeature() noexcept {
    return Status::Error(Err::UNSUPPORTED, "Procedure unavailable on this declared part",
                         static_cast<int32_t>(UnsupportedDetail::FEATURE));
  }
  Status _admitPin2Output() const noexcept;
  ConfigurationMismatch _clockRoleMismatch() const noexcept;
  void _invalidateResetEvidence(bool configurationLost) noexcept;
  MdioBatch _configurationBatch() const noexcept;
  Status _advanceConfiguration(Status status, bool called, const MdioBatch& batch) noexcept;
  void _beginConfiguration(bool exchange) noexcept;
  const LinkSettings& _configurationTarget() const noexcept;
  bool _startingLoopback() const noexcept;
  Status _startLoopback(const LoopbackOptions& options, const OperationBudget& budget, OperationToken& token, bool lineBist) noexcept;
  void _setBistStep(BistStep step) noexcept;
  MdioBatch _bistBatch() const noexcept;
  Status _advanceBist(Status status, bool called, const MdioBatch& batch) noexcept;
  Status _admitBistOperation(const OperationBudget& budget, bool stop) const noexcept;
  MdioBatch _loopbackBatch() const noexcept;
  Status _advanceLoopback(Status status, bool called, const MdioBatch& batch) noexcept;
  void _setLoopStep(LoopStep step) noexcept;
  void _beginLoopbackActivation() noexcept;
  void _beginLoopbackRestore() noexcept;
  static MdioBatch _mmdBatch(uint16_t address, FrameKind kind, uint16_t value,
                            uint16_t devad = 31) noexcept;
  void _setConfigStep(ConfigStep step) noexcept;
  void _observeConfiguration() noexcept;
  void _retainPageEvidence(const MdioBatch& batch, const BatchResult& receipt,
                           uint8_t completed, bool clockValid, uint64_t upper) noexcept;
};

} // namespace DP8382x
