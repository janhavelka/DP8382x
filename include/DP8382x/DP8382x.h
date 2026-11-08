/// @file DP8382x.h
/// @brief Portable bounded direct/indirect access and passive transport evidence.
#pragma once

#include <DP8382x/Config.h>

/// @brief Portable PHY driver and copied transport observations.
namespace DP8382x {

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
  COUNT ///< Number of supported sources, not a readable source.
};

/// @brief Build an acquisition mask; invalid enum values produce zero.
/// @param[in] source Named destructive source.
/// @return One mask bit or zero, without I/O.
constexpr uint32_t evidenceBit(EvidenceSource source) noexcept {
  return source < EvidenceSource::COUNT ? uint32_t{1} << static_cast<uint8_t>(source) : 0U;
}
/// @brief Every currently supported destructive source; no automatic polling.
constexpr uint32_t ALL_EVIDENCE = (uint32_t{1} << static_cast<uint8_t>(EvidenceSource::COUNT)) - 1U;

/// @brief Retained evidence from one destructive source, separate from aggregate success.
struct EvidenceSample {
  uint16_t raw{0}; ///< Latest successfully returned full word, including unknown bits.
  uint16_t observedOnes{0}; ///< Union of raw ones; no implied physical event decoding.
  uint16_t observedZeros{0}; ///< Union of raw zeros, retaining latched-low observations.
  uint32_t samples{0}; ///< Successful consumed words, saturating; not an event count.
  uint64_t sampledSum{0}; ///< FCSCR low-byte or RECR raw-word arithmetic sum; not a qualified physical total.
  bool sumSaturated{false}; ///< Sticky arithmetic overflow; sampledSum is then a lower bound.
  uint64_t observedAtUs{0}; ///< Conservative completion bound in the binding clock domain.
  bool valid{false}; ///< At least one independently valid word has been retained.
  bool timeValid{false}; ///< Timestamp is usable; zero is a valid timestamp.
  bool stale{true}; ///< Failed refresh or known external/configuration change.
  bool historyLost{false}; ///< Sticky: a possible consumption returned no trusted word.
};

/// @brief Passive destructive evidence; binding/end begin a new local history.
struct EvidenceSnapshot {
  std::array<EvidenceSample, static_cast<uint8_t>(EvidenceSource::COUNT)> samples{}; ///< Indexed by EvidenceSource.
  uint32_t lastRequested{0}; ///< Sources requested by the latest admitted acquisition.
  Status lastStatus{Err::RESULT_NOT_AVAILABLE, 0, "No evidence acquisition"}; ///< Latest acquisition or admitted mixed update result; comparison-only wake exit leaves it unchanged.
  bool configurationDirty{false}; ///< Historical mutation/uncertainty latch; not the initialized-state flag.
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
  CONFIGURE ///< Complete forced/negotiated profile and optional finite next-page exchange.
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
  RESTART ///< Restart PHY circuitry, retaining register configuration.
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
/// @brief Copied initialization request; NONE still writes/verifies mandatory VOD.
struct InitializeOptions {
  ResetKind reset{ResetKind::PCS}; ///< Selected reset.
};
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
/// @brief Current foundational configuration evidence, separate from desired intent.
enum class ConfigurationTrust : uint8_t {
  UNKNOWN, ///< No established configuration evidence.
  VERIFIED, ///< Complete currently owned stable settings verified.
  DIRTY ///< Admitted desired change or uncertainty needs verification/restoration.
};
/// @brief Passive profile evidence. VERIFIED covers active advertisement/mode, routing, extension and mandatory VOD.
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
  bool pagePayloadMissing{false}; ///< Consumed event lacks a trusted payload; history is incomplete.
  bool partnerRefused{false}; ///< Partner did not advertise next-page capability.
  bool complete{false}; ///< Page service and AN-complete were observed.
  bool npMayRemainSet{false}; ///< Failed/cancelled epoch may still oblige page service.
  uint8_t transmittedPages{0}; ///< Successfully loaded pages, including generated null pages.
  uint8_t receivedPages{0}; ///< Consumed next-page events, including duplicate/missing payload slots.
  uint32_t duplicatePages{0}; ///< Repeated toggle evidence; never advances local page index.
  uint16_t basePage{0}; ///< Raw base page from ANLPAR, not ANLNPTR.
  std::array<PageEvidence, MAX_NEXT_PAGES> pages{}; ///< Consumed event slots; invalid payloads remain explicit.
};
/// @brief Raw captured straps, never assumed pin levels or decoded BASIC/ENHANCED defaults.
struct StrapEvidence {
  uint16_t sor1{0}; ///< Latest returned SOR1 word, subject to sor1Valid.
  uint16_t sor2{0}; ///< Latest returned SOR2 word, subject to sor2Valid.
  bool sor1Valid{false}; ///< A valid SOR1 word has been captured.
  bool sor2Valid{false}; ///< A valid SOR2 word has been captured.
  uint64_t observedAtUs{0}; ///< Conservative stamp after complete current capture.
  bool timeValid{false}; ///< Complete-capture timestamp is usable.
  bool stale{true}; ///< Reset or partial recapture can mix historical/current words.
};
/// @brief First failed stable-bit comparison in a complete verification pass.
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
/// @brief Frozen evidence delivered once; restoration uses a separate token/result.
struct OperationResult {
  OperationToken token{}; ///< Owning instance's lifetime token.
  OperationKind kind{OperationKind::NONE}; ///< Operation that produced this result.
  ResetKind reset{ResetKind::NONE}; ///< Requested reset, separate from external observations.
  Status status{Err::RESULT_NOT_AVAILABLE, 0, "No operation"}; ///< Immutable terminal outcome.
  bool mutationAttempted{false}; ///< Semantic target write attempted; excludes selectors.
  bool mutationCompleted{false}; ///< Semantic target completed, even if later failure occurred.
  bool mutationPossible{false}; ///< Semantic effect may have occurred.
  bool commandRequired{false}; ///< This operation requests a reset command.
  bool commandAccepted{false}; ///< Reset frame completed; does not prove reset completion.
  bool protocolComplete{false}; ///< Requested command subsequently observed clear on time.
  bool negotiationComplete{false}; ///< BMSR AN-complete observed by an explicitly requested wait.
  bool temporaryMayBeActive{false}; ///< Initialization alone never starts a temporary stimulus.
  bool persistentRestored{false}; ///< All currently owned persistent state verified.
  bool cleanupOwed{false}; ///< Barrier at termination; current state is in operationSnapshot.
  bool clockInvalid{false}; ///< Clock contract failed during the operation.
  bool late{false}; ///< A batch returned at/after the deadline.
  DeviceIdentity identity{}; ///< Latest operation-owned identity evidence.
  StrapEvidence straps{}; ///< Captured raw straps with historical validity.
  ConfigurationMismatch mismatch{}; ///< First stable mismatch, if any.
  BatchSummary failingBatch{}; ///< First failure, otherwise latest batch.
  NextPageResult nextPage{}; ///< Exchange evidence, including partial consumption.
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
  bool temporaryMayBeActive{false}; ///< No retained diagnostic stimulus is implemented yet.
  uint64_t nextPollAtUs{0}; ///< Earliest legal next scheduled observation.
  bool nextPollValid{false}; ///< A valid scheduled wait exists.
  uint64_t busQuietUntilUs{0}; ///< Hard-reset silence floor; shared owner holds every PHY quiet.
  bool busQuietValid{false}; ///< Valid floor anchor; cached until a timed call observes expiry.
  IoBudget remaining{}; ///< Original deadline and remaining physical frames.
  uint32_t stalledPolls{0}; ///< Consecutive polls without work or clock advance.
};

/// @brief Noncopyable, application-serialized DP83826I driver.
/// @pre Serialize every call, including cached access. The borrowed context and
/// callbacks outlive calls. No callback may retain buffers or re-enter this object.
/// @note No allocation, SDK, hidden retries or work without caller polling.
/// Maintenance writes return a cached failed/rejected readIdentity status before
/// any callback. Successful reprobe or explicit rebind clears that restriction.
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
  Status startInitialize(const InitializeOptions& options, const OperationBudget& budget,
                         OperationToken& token) noexcept;
  /// @brief Advance at most one complete owned batch; return while waiting or short of slice.
  /// @param[in] token Exact occupied token; stale/foreign tokens cannot access new results.
  /// @param[in] maxFrames Per-poll allowance; zero/short slices yield without splitting a batch.
  /// @return IN_PROGRESS or frozen terminal status; token/lifetime errors perform no I/O.
  /// @note SNLS647I table8-19 PCS uses literal 00=8000; completion polls that bit clear.
  /// Every initialization writes 31:030E=4A40 and verifies FFC0 (table8-81).
  /// HARD/RESTART use literal 1F=8000/4000 (table8-48), never sampled-word RMW.
  /// Any attempted HARD write publishes a persistent max(2000, interval) us floor
  /// anchored after callback return; the bus owner also excludes other PHYs.
  /// Waits use conservative post-callback time; no controller lock spans polls.
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
  /// @return True only after identity/reset/mandatory VOD verification; never a link claim.
  bool isInitialized() const noexcept { return _initialized; }

  /// @brief Admit a complete negotiated or forced profile without MDIO or clock callbacks.
  /// @param[in] settings AN advertisement or forced10/100 half/full, coupled routing/mirror and extension.
  /// @param[in] options Optional bounded Clause 28 exchange/completion wait.
  /// @param[in] budget Exclusive aggregate deadline, frame and stall allowances.
  /// @param[out] token Assigned only on admission; result must be taken before reuse.
  /// @return IN_PROGRESS on admission; invalid intent, unavailable feature, binding,
  /// operation/barrier, initialization or budget errors otherwise, with no changes.
  /// @note SNLS647I 8.3.1/.2, tables 8-19/23..30/43; IEEE 802.3 Clause 28.
  /// Forced requires robust routing, no AN intent/options, and never accesses
  /// ANAR. Automatic routing preserves PHYCR14; mirror sets both CR3 bits0060.
  /// Extension owns CR2 bit0020 only, rejects forced10 and proves no partner agreement.
  /// Admission copies desired intent and marks trust DIRTY. NP belongs only to
  /// this exchange. Failure/cancel stops without cleanup writes; restoration is
  /// a new initialization or eligible configuration operation. No VOD write here.
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

  /// @brief Acquire and accept identity at the bound address in one two-read batch.
  /// @param[out] identity Assigned only after complete in-budget transport and identity acceptance.
  /// @param[in] budget Aggregate microsecond deadline and at least two frames.
  /// @return Transport/admission errors unchanged; DEVICE_NOT_FOUND for either
  /// required word zero/all-ones; DEVICE_MISMATCH for OUI/model/revision/mode,
  /// checked in that order. No error implies physical absence or an MDIO ACK.
  /// @note SNLS647I tables 8-21/22: declared DP83826I, OUI 080028, model 11/13,
  /// revision 1. Shared IDs cannot identify I/E suffix. Partial/rejected evidence
  /// stays in identitySnapshot; never combine words from different calls.
  /// @pre Bound, serialized owner with clock/power/reset readiness established.
  /// Uses the ordinary raw-read guard; no reset, scan, write or retry is performed.
  Status readIdentity(DeviceIdentity& identity, const IoBudget& budget) noexcept;
  /// @brief Copy latest identity evidence without MDIO or clock access.
  /// @return Historical/partial fields with explicit validity, admission and staleness.
  DeviceIdentity identitySnapshot() const noexcept { return _identity; }
  /// @brief Query accepted source capabilities without MDIO or clock access.
  /// @return Invalid until a fresh identity is accepted; no link/settings/qualification claim.
  Capabilities capabilities() const noexcept;

  /// @brief Read one nonconsuming Clause 22 word using one terminal owned batch.
  /// @param[in] registerAddress Five-bit address, validated before narrowing.
  /// @param[out] value Assigned only on complete valid in-budget success.
  /// @param[in] budget Original exclusive microsecond deadline and frame allowance.
  /// @return INVALID_PARAM for width/access, NOT_BOUND, BUSY, clock/configuration
  /// error, transport failure or OPERATION_TIMEOUT; never IN_PROGRESS.
  /// @note SNLS647I tables 8-19..48. Selectors, holes, consuming reads and CDCR
  /// (resistor-calibration follow-up obligation) are rejected before callbacks.
  /// Status words may change asynchronously; a raw word is not a coherent link result.
  Status readClause22(uint32_t registerAddress, uint16_t& value,
                      const IoBudget& budget) noexcept;
  /// @brief Write the safe maintenance subset in one terminal owned batch.
  /// @param[in] registerAddress Only LEDCR (0x18) is currently admitted.
  /// @param[in] value Sixteen-bit word; only mask 0x0692 may be nonzero.
  /// @param[in] budget Original exclusive microsecond deadline and frame allowance.
  /// @pre Application establishes exact part/address and safe LED0 electrical use,
  /// and serializes maintenance. Binding alone does not verify those facts.
  /// @return Direct-access errors or the retained failed identity status; OK is
  /// frame completion, not verified pin state. Failed/late writes may have applied.
  /// @note SNLS647I table 8-42: blink, polarity, drive enable/value; reserved zero.
  /// No cached setting is claimed. Copy transportSnapshot before another batch.
  /// Replaces all four fields: unset bits select 20 Hz blink and active-low
  /// polarity, overriding the strap. Use modifyMmd to change one field.
  Status writeClause22(uint32_t registerAddress, uint32_t value,
                       const IoBudget& budget) noexcept;
  /// @brief Read one admitted nonconsuming MMD word in one four-frame batch.
  /// @param[in] devad Device selector; vendor31 plus the three source-implied EEE reads.
  /// @param[in] registerAddress Sixteen-bit documented offset; no indirect selectors.
  /// @param[out] value Unchanged unless all frames, release and timing succeed.
  /// @param[in] budget Original exclusive deadline and at least four frames.
  /// @return Direct-access terminal errors; INVALID_PARAM for holes, consuming reads
  /// or unadmitted DEVAD/offsets. EEE mappings remain physically unqualified.
  /// @note SNLS647I 8.3.11.1/.5, tables 8-31/32. Clause 22 aliases inherit the
  /// same policy. Raw TDR/MSE/strap words do not establish freshness or units.
  Status readMmd(uint32_t devad, uint32_t registerAddress, uint16_t& value,
                 const IoBudget& budget) noexcept;
  /// @brief Write one admitted vendor-MMD word in one four-frame batch.
  /// @param[in] devad Device selector; only 31 is admitted for mutation.
  /// @param[in] registerAddress Currently only the LEDCR alias (0x0018).
  /// @param[in] value Sixteen-bit word; reserved bits must be zero.
  /// @param[in] budget Original exclusive deadline and at least four frames.
  /// @pre Same exact-part, maintenance and pin-use preconditions as writeClause22.
  /// @return Terminal status; errors retain selector/possible target effects.
  /// @note SNLS647I 8.3.11.4 and table 8-42. Always reselects the address;
  /// no retry or interpretation of a prior ambiguous write as verified success.
  /// Replaces all four fields: unset bits select 20 Hz blink and active-low
  /// polarity, overriding the strap. Use modifyMmd to change one field.
  Status writeMmd(uint32_t devad, uint32_t registerAddress, uint32_t value,
                  const IoBudget& budget) noexcept;
  /// @brief Modify safe stable fields under one five-frame MMD transaction.
  /// @param[in] devad Device selector; only 31 is admitted for mutation.
  /// @param[in] registerAddress Currently only the LEDCR alias (0x0018).
  /// @param[in] mask Nonzero sixteen-bit subset of LEDCR's documented stable bits.
  /// @param[in] value Sixteen-bit replacement bits; must be a subset of mask.
  /// @param[in] budget Original exclusive deadline and at least five frames.
  /// @pre Owner excludes reset/external writers and meets writeClause22 preconditions.
  /// @return Terminal status; a failed or late dependent write may have applied.
  /// @note Five frames are inferred from SNLS647I 8.3.11.1, not a printed sequence.
  /// Nonincrementing selection persists through
  /// the read and following write. Preserve only stable bits outside mask, force
  /// LEDCR reserved bits zero. Other targets require per-register reserved handling.
  /// Never replay commands or update mixed/destructive fields through generic RMW.
  /// No readback verification, automatic retry or separate public lock scope.
  Status modifyMmd(uint32_t devad, uint32_t registerAddress, uint32_t mask,
                   uint32_t value, const IoBudget& budget) noexcept;
  /// @brief Explicitly acquire destructive words through the sole private collector.
  /// @param[in] sourceMask Nonzero subset of ALL_EVIDENCE; no implicit sources.
  /// @param[out] evidence Assigned only when every requested read/release succeeds on time.
  /// @param[in] budget Aggregate deadline and frames; complete requested count admitted first.
  /// @pre Application owns the instance, exact part/address and diagnostic admission.
  /// @return Terminal transport/validation status; errors retain consumed partial samples.
  /// @note PHYSTS is read first when requested. RXFS uses a complete MMD sequence.
  /// Reads can clear IRQ/history; ambiguous reads set historyLost, never zero events.
  /// A collection spans bounded batches; it is not a simultaneous hardware snapshot.
  Status acquireEvidence(uint32_t sourceMask, EvidenceSnapshot& evidence,
                         const IoBudget& budget) noexcept;
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
  /// @pre Exact DP83826I, quiesced traffic, safe wake wiring and exclusive semantic
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
  Config _config{};
  TransportSnapshot _transport{};
  EvidenceSnapshot _evidence{};
  DeviceIdentity _identity{};
  SettingsSnapshot _settings{};
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
  Status _execute(MdioBatch batch, const IoBudget& budget, bool* called = nullptr,
                  bool operation = false) noexcept;
  Status _readIdentity(DeviceIdentity& identity, const IoBudget& budget, bool operation,
                       bool* called = nullptr) noexcept;
  void _retainEvidence(const MdioBatch& batch, const BatchResult& result,
                       bool clockValid, uint64_t before, uint64_t upper) noexcept;
  void _invalidateEvidence() noexcept;
  Status _checkMutation() noexcept;
  Status _checkToken(OperationToken token) const noexcept;
  Status _finishOperation(Status status, bool freshObservation = false) noexcept;
  Status _yieldOperation(uint64_t raw) noexcept;
  void _setInitStep(InitStep step) noexcept;
  Status _waitOperation() noexcept;
  bool _quietAt(uint64_t raw) noexcept;
  Status _checkQuiet() noexcept;
  Status _anchorQuiet() noexcept;
  uint32_t _commandWaitUs() const noexcept;
  void _invalidateResetEvidence(bool configurationLost) noexcept;
  MdioBatch _configurationBatch() const noexcept;
  Status _advanceConfiguration(Status status, bool called, const MdioBatch& batch) noexcept;
  void _beginConfiguration(bool exchange) noexcept;
  void _setConfigStep(ConfigStep step) noexcept;
  void _observeConfiguration() noexcept;
  void _retainPageEvidence(const MdioBatch& batch, const BatchResult& receipt,
                           uint8_t completed, bool clockValid, uint64_t upper) noexcept;
};

} // namespace DP8382x
