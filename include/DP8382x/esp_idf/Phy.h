/// @file Phy.h
/// @brief Native ESP-IDF v6.1 owner, terminal MAC transport and retained lifecycle.
#pragma once

#include <DP8382x/DP8382x.h>
#include <esp_eth.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <atomic>
#include <limits>

/// @brief SDK-only integration; no SDK types enter the portable library.
namespace DP8382x::EspIdf {

/// @brief Single logical admission phase, independent of the controller mutex.
enum class Phase : uint8_t {
  STOPPED, ///< Installed/stopped owner work may be admitted.
  CONTROL, ///< One typed operation or installation owns admission.
  STARTING, ///< Synchronous framework start callbacks are admitted.
  RUNNING, ///< Framework timer is the only live link poller.
  STOPPING, ///< Stop/fence is in progress; no new PHY I/O.
  MAINTENANCE ///< Terminal retained resources; system restart required.
};
/// @brief Last lifecycle boundary reached; not a hardware qualification.
enum class LifecycleStage : uint8_t {
  FACTORY, ///< Local allocation and binding.
  BOARD, ///< Application reset and shared readiness.
  INSTALL, ///< Public framework installation.
  INITIALIZE, ///< Typed core initialization.
  CONFIGURE, ///< Typed persistent configuration and retained result.
  START, ///< Framework start and initial link collection.
  STOP, ///< Framework stop request.
  FENCE, ///< Timer-task callback fence.
  MAC_STOP, ///< Direct actual MAC stop observation.
  RETAINED ///< Admission closed; objects retained until restart.
};
/// @brief Native outcome keeps portable chip/admission errors distinct from SDK errors.
/// @note At most one member is non-success. SDK queue, timer and MAC-policy
/// failures never become portable MDIO errors or transport-health samples.
struct NativeStatus {
  Status coreStatus{}; ///< Portable procedure, owner admission or aggregate deadline result.
  esp_err_t sdkError{ESP_OK}; ///< Exact SDK result; ESP_OK when coreStatus owns the failure.
  /// @brief Test both result domains without I/O.
  /// @return True only when neither domain reports a failure.
  bool ok() const noexcept { return coreStatus.ok() && sdkError == ESP_OK; }
  /// @brief Test a portable result code without reclassifying SDK errors.
  /// @param[in] error Portable code to compare.
  /// @return True only for a matching portable result and no SDK failure.
  bool is(Err error) const noexcept { return sdkError == ESP_OK && coreStatus.is(error); }
};
/// @brief Last accepted MAC projection; down never invents speed or duplex.
struct PublishedTuple {
  bool up{false}; ///< True only after accepted LINK UP notification.
  LinkSpeed speed{LinkSpeed::UNKNOWN}; ///< Qualified speed for the last UP.
  Duplex duplex{Duplex::UNKNOWN}; ///< Qualified duplex for the last UP.
  bool peerPause{false}; ///< SDK Boolean: true only for valid resolved RX AND TX pause.
  bool speedAccepted{false}; ///< SPEED callback accepted this field in the latest update.
  bool duplexAccepted{false}; ///< DUPLEX callback accepted this field in the latest update.
  bool pauseAccepted{false}; ///< PAUSE callback accepted this field in the latest update.
};
/// @brief Persistent primary/cleanup evidence for framework partial effects.
struct LifecycleRecord {
  Phase phase{Phase::STOPPED}; ///< Current owner admission copied by lifecycleSnapshot; no separate state machine.
  uint64_t sequence{0}; ///< Saturating start-attempt sequence; zero precedes the first admitted start.
  LifecycleStage stage{LifecycleStage::FACTORY}; ///< Last attempted boundary.
  NativeStatus firstError{}; ///< First failure in this start cycle, or pre-start setup.
  NativeStatus cleanupError{}; ///< First cleanup failure; retries preserve it until the next admitted start.
  NativeStatus lastCleanupError{}; ///< Most recent cleanup failure; preserves a later failed repair separately from its first cause.
  PublishedTuple publishedTuple{}; ///< Last accepted projection.
  bool publicationUncertain{false}; ///< Unknown MAC partial effect; admission closes.
  bool eventLost{false}; ///< Current accepted LINK event is pending; older failures remain in firstError.
  bool stopRequired{false}; ///< Observation/publication safety lost; application must stop before restarting traffic.
  Status lastObservationStatus{Err::RESULT_NOT_AVAILABLE, 0, "No native observation"}; ///< Latest admitted live query result, independent of firstError.
  uint32_t consecutiveObservationMisses{0}; ///< Saturating unsuccessful RUNNING queries; success clears, gate denial does not count.
  bool observationDownOwed{false}; ///< Transient miss limit reached; withdraw UP when the current publisher finishes, without requiring stop.
};
/// @brief Last public SDK control reconciliation; cached fields are not live PHY evidence.
struct SdkStateSnapshot {
  bool reconciled{false}; ///< True only after verified core settings and successful public controls.
  bool cacheValid{false}; ///< cachedAutoNegotiation was read through ETH_CMD_G_AUTONEGO.
  bool cachedAutoNegotiation{false}; ///< SDK's cache, distinct from desired mode and raw BMCR.
  bool autoNegotiationAccepted{false}; ///< Public S_AUTONEGO completed in the last attempt.
  bool speedAccepted{false}; ///< Forced S_SPEED acknowledged the verified desired speed.
  bool duplexAccepted{false}; ///< Forced S_DUPLEX_MODE acknowledged the verified desired duplex.
  NativeStatus status{{Err::NOT_INITIALIZED, 0, "SDK state not reconciled"}}; ///< Last reconciliation result, with distinct SDK domain.
};
/// @brief Separate MAC policy and PHY advertisement receipts; no implicit rollback.
struct FlowControlRecord {
  bool requestedEnabled{false}; ///< Application MAC flow-control policy.
  uint32_t pauseAbility{0}; ///< Bit 0 symmetric, bit 1 asymmetric; exact values 0..3.
  bool macAttempted{false}; ///< enable_flow_ctrl was invoked.
  bool macAccepted{false}; ///< MAC returned ESP_OK, even if completion was late.
  bool macEffectPossible{false}; ///< Accepted or uncertain MAC policy change.
  bool phyAttempted{false}; ///< Correctly typed PHY acknowledgement callback was invoked.
  bool phyAccepted{false}; ///< Correctly typed PHY callback acknowledged verified advertisement.
  esp_err_t macError{ESP_OK}; ///< Actual MAC callback result, including accepted late work.
  esp_err_t phyError{ESP_OK}; ///< PHY callback or admission/deadline result.
  bool reconciled{false}; ///< Both parts completed in the same admitted, in-budget request.
};
/// @brief One controller-wide silence reservation, independent of its short mutex.
enum class BusAdmission : uint8_t {
  BOOT_CLOSED, ///< Initial state; only board preparation may claim the controller.
  AVAILABLE, ///< Ordinary complete batches and a new exclusive procedure may run.
  INITIALIZING, ///< Only the admitting PHY's initialization batches may run.
  BOARD_PREPARING, ///< No MDIO while reset/readiness owns the controller domain.
  INSTALLING, ///< MAC reset/clock setup excludes every shared-controller participant.
  FAILED_CLOSED ///< Board readiness failed; external disposition or restart required.
};
/// @brief Borrowed controller-wide exclusion and readiness shared by every PHY.
/// @pre mutex is an application-created nonrecursive mutex. This object and mutex
/// outlive all MAC/driver callbacks, including terminal retained-resource state.
/// @pre Every participant checks admission, quarantine and quietUntilUs both
/// before AND after acquiring mutex, then holds it for the complete transaction.
/// Only the INITIALIZING reservation owner may bypass that admission state.
struct SharedBus {
  SemaphoreHandle_t mutex{nullptr}; ///< Serializes complete MDIO batches.
  std::atomic<bool> quarantined{false}; ///< Unknown controller completion; external repair only.
  std::atomic<BusAdmission> admission{BusAdmission::BOOT_CLOSED}; ///< Sole silence authority; never overwrite another reservation.
  std::atomic<uint64_t> quietUntilUs{0}; ///< Shared nonwrapping esp_timer readiness floor.
};
/// @brief Explicit board reset procedure, supplied by the application.
struct BoardControl {
  void* context{nullptr}; ///< Borrowed through all calls and retained resources.
  /// @brief Finite terminal board callback; true asserts RESET_N, false releases it.
  esp_err_t (*setReset)(void*, bool){nullptr};
  uint64_t powerOnAtUs{0}; ///< Actual power-on sample in esp_timer us; event no later than sample+clockLagUs. Zero is valid.
  uint64_t clockStableAtUs{0}; ///< Stable REF_CLK event sample with the same clockLagUs bound; uninterrupted thereafter.
  uint32_t assertionUs{100}; ///< At least 25 us; board RC pulse qualification remains external.
  uint32_t maxPolls{100000}; ///< Finite wait cap, including a frozen clock.
};
/// @brief One discovery address's independent raw identity samples, never accepted part identity.
struct DiscoveryEntry {
  uint16_t id1{0}; ///< C22 PHYIDR1, usable only with id1Valid.
  uint16_t id2{0}; ///< C22 PHYIDR2, usable only with id2Valid.
  bool id1Valid{false}; ///< First read completed successfully.
  bool id2Valid{false}; ///< Second read completed successfully.
};
/// @brief Fixed explicit discovery receipt; does not select or rebind an address.
struct DiscoverySnapshot {
  uint32_t requestedMask{0}; ///< Address N is selected by bit N, 0..31.
  uint32_t framesAttempted{0}; ///< Physical frame calls, maximum 64.
  std::array<DiscoveryEntry, 32> entries{}; ///< Per-address partial samples.
  Status status{Err::RESULT_NOT_AVAILABLE, 0, "No discovery"}; ///< Terminal aggregate status.
  BatchResult lastBatch{}; ///< Last attempted complete identity batch, including failure.
};
/// @brief Validated native construction input. Address and MAC identity are immutable.
struct PhyConfig {
  Config core{}; ///< Explicit part/interface/address and truthful transport bounds; callbacks replaced.
  esp_eth_mac_t* mac{nullptr}; ///< Borrowed actual MAC, never the mediator route.
  SharedBus* bus{nullptr}; ///< Borrowed bus shared by all participants.
  uint32_t linkPollMs{2000}; ///< 1..4294967 ms, at least callbackBudgetUs rounded up to ms; not a response-time guarantee.
  uint32_t callbackBudgetUs{100000}; ///< Must exceed B(8)+B(2)+clockLagUs+5*notificationUs; includes local/gate/clock overhead.
  uint32_t cleanupBudgetUs{100000}; ///< Positive failed-start attempt allowance from observed completion; may be too short to finish. No qualified fence/scheduler/MAC minimum exists; explicit stop retry supplies its own deadline.
  uint32_t notificationUs{0}; ///< Required qualified per-notification bound, microseconds including scheduling; late return is detected, not preempted.
  uint32_t lockWaitUs{1000}; ///< Controller mutex wait cap; scheduler latency remains conditional.
  uint32_t maxFencePolls{100000}; ///< Finite timer fence polling cap.
  uint32_t maxStopAttempts{1}; ///< Finite actual MAC stop attempts.
  bool pinnedP4Mac{false}; ///< Attests actual pinned P4 MAC, whose valid set_link always succeeds.
  uint32_t maxObservationMisses{3}; ///< Positive consecutive admitted RUNNING misses before recoverable administrative DOWN; no wall-time bound if polling is denied.
};

class Phy;
/// @brief Only the public PHY vtable is embedded; no IEEE base/private SDK storage.
struct PhyHandle {
  esp_eth_phy_t phy{}; ///< First member, statically checked for pointer recovery.
  Phy* owner{nullptr}; ///< Owning nonmovable adapter.
};

/// @brief One noncopyable native owner with a shared portable chip implementation.
/// @pre Lifecycle callers serialize themselves and run outside ESP_TIMER_TASK.
/// End TX/RX sessions before retire; retain RX drop/free callbacks and netif/glue
/// until system restart. Installed objects cannot be destructively released on
/// the pinned SDK (native_rx_quiescence). No method preempts an SDK callback.
/// @note Finite wall time requires qualified controller, RTOS scheduling, logging
/// and event-service bounds. SDK loop counts alone do not establish those bounds.
class Phy {
public:
  /// @brief Allocate owner/gate/fence and bind locally; no MDIO, GPIO or SDK install.
  /// @param[in] config Explicit address 0..31, MAC, shared bus and positive bounds.
  /// @param[out] output Assigned only on ESP_OK; unchanged on validation/allocation failure.
  /// @return ESP_OK, INVALID_ARG, NO_MEM or the actual fence creation error.
  static esp_err_t create(const PhyConfig& config, Phy*& output) noexcept;
  /// @brief Owners cannot be copied; callbacks retain this object's address.
  Phy(const Phy&) = delete;
  /// @brief Assignment cannot create a second native operation owner.
  Phy& operator=(const Phy&) = delete;
  /// @brief Moving would invalidate installed callback/context addresses.
  Phy(Phy&&) = delete;
  /// @brief Move assignment is prohibited even before installation.
  Phy& operator=(Phy&&) = delete;
  /// @brief Public vtable for the owner-controlled installer; never delete directly after install.
  /// @return Borrowed stable address, valid through this adapter's retained lifetime.
  esp_eth_phy_t* nativePhy() noexcept { return &_handle.phy; }
  /// @brief Assert/reset board under shared silence, then wait 2 ms release/50 ms POR and clock readiness.
  /// @param[in] board Finite borrowed pin callback and real power/clock timestamps.
  /// @param[in] deadlineUs Exclusive absolute esp_timer microsecond deadline, no renewed timeout.
  /// @return Native error/timeout; another reservation rejects before GPIO. Failed
  /// readiness leaves FAILED_CLOSED until external disposition or system restart.
  esp_err_t prepareBoard(const BoardControl& board, uint64_t deadlineUs) noexcept;
  /// @brief Install with mandatory rejecting raw hooks after board readiness; no PHY I/O.
  /// @param[out] output Published only on success; SDK install failure has no partial handle.
  /// @return SDK error unchanged; failed install leaves only owner MAC/PHY deletion.
  esp_err_t install(esp_eth_handle_t& output) noexcept;
  /// @brief Start after verified settings and SDK reconciliation; bounded gate waits, no negotiation wait.
  /// @param[in] deadlineUs Absolute primary allowance, including synchronous start callbacks.
  /// @return Primary error; failed/late start attempts stop with a separate cleanupBudgetUs allowance.
  /// @note The terminal receipt is adopted by the next gated entry, so an already
  /// running timer callback cannot strand STARTING after SDK success.
  esp_err_t start(uint64_t deadlineUs) noexcept;
  /// @brief Stop, fence once, repair missing DOWN/STOP, then observe actual MAC stop status.
  /// @param[in] deadlineUs Absolute allowance; timeout retains objects and retry polls the same fence.
  /// @return OK when native stop is complete (also if already stopped/retained).
  /// A new stop denied admission leaves phase and hardware unchanged. Timeout of
  /// already-owed cleanup records that failure; admitted work remains retryable.
  /// A lost DISCONNECTED cannot withhold fence/STOP/MAC stop; its error and repair
  /// debt remain. STOP may precede that repaired event after a queue failure.
  /// Forbidden raw uninstall skips the deleted SDK timer, retaining native cleanup
  /// and bus-silent cancel/takeResult. Core result delivery remains independently owed.
  /// @note Earlier cleanup errors stay in the journal even after successful repair.
  esp_err_t stop(uint64_t deadlineUs) noexcept;
  /// @brief Close admission permanently and retain installed resources under RX lifetime gate.
  /// @param[in] deadlineUs Absolute enclosing stop/fence allowance, microseconds.
  /// @return ESP_ERR_NOT_SUPPORTED only after complete stop and terminal retention;
  /// otherwise the stop error with retryable admission and retained effects.
  esp_err_t retire(uint64_t deadlineUs) noexcept;
  /// @brief Delete only a never-installed idle adapter; never deletes the borrowed MAC/bus.
  /// @return INVALID_STATE if installed, active or fence live; otherwise actual timer deletion error/OK.
  esp_err_t destroy() noexcept;
  /// @brief Copy the lifecycle journal locally; unchanged output if gate is busy.
  /// @param[out] output Value copy on OK.
  /// @return OK or BUSY; no clock, MDIO or SDK callback.
  Status lifecycleSnapshot(LifecycleRecord& output) noexcept;
  /// @brief Copy the acquired core link evidence separately from SDK publication.
  /// @param[out] output Value copy on OK; unchanged on gate contention.
  /// @return OK or BUSY; no clock, MDIO, event delivery or refresh of evidence age.
  /// @note A valid observation may precede failed/partial SDK notification. Inspect
  /// LifecycleRecord for accepted fields, uncertainty and pending event delivery.
  Status linkSnapshot(LinkSnapshot& output) noexcept;
  /// @brief Admit the core's explicit reset/initialization while stopped; token unchanged on failure.
  /// @param[in] options Explicit core reset choice; initialization reserves shared-bus silence.
  /// @param[in] budget Original aggregate core deadline/frame/poll policy.
  /// @param[out] token Assigned only on IN_PROGRESS.
  /// @return Core admission status, BUSY or INVALID_CONFIG for unavailable native route.
  Status startInitialize(const InitializeOptions& options, const OperationBudget& budget,
                         OperationToken& token) noexcept;
  /// @brief Admit persistent AN or forced intent while stopped; reconcile SDK after taking success.
  /// @param[in] settings Complete legal 10/100 profile, with application-owned policy.
  /// @param[in] budget Original aggregate core allowance.
  /// @param[out] token Assigned only on IN_PROGRESS.
  /// @return Core admission status, BUSY or INVALID_CONFIG for unavailable native route.
  Status startConfigure(const LinkSettings& settings, const OperationBudget& budget,
                        OperationToken& token) noexcept;
  /// @brief Configure through the same core procedure, including optional finite AN/next-page waits.
  /// @param[in] settings Complete persistent link profile.
  /// @param[in] options Core negotiation/wait policy; no native polling loop is created.
  /// @param[in] budget Original aggregate core allowance.
  /// @param[out] token Assigned only on IN_PROGRESS; invalid input has no SDK/MDIO effect.
  /// @return Core admission status or BUSY; SDK reconciliation is a separate explicit call.
  Status startConfigure(const LinkSettings& settings, const ConfigureOptions& options,
                        const OperationBudget& budget, OperationToken& token) noexcept;
  /// @brief Reconcile already verified stopped settings through public SDK setters/getter.
  /// @param[in] deadlineUs Exclusive absolute esp_timer allowance for all controls, microseconds.
  /// @return Terminal status; partial/late SDK acceptance stays in sdkStateSnapshot.
  /// @note No MDIO: callbacks attest shared core readback; stale settings require initialize/configure first.
  NativeStatus reconcileSdkState(uint64_t deadlineUs) noexcept;
  /// @brief Apply MAC flow policy and attest separately configured PHY pause using correctly typed calls.
  /// @param[in] enabled Application MAC policy; never passed through defective S_FLOW_CTRL ioctl.
  /// @param[in] pauseAbility Bit 0 symmetric/bit 1 asymmetric, 0..3; must match verified AN advertisement.
  /// @param[in] deadlineUs Exclusive aggregate esp_timer allowance, microseconds.
  /// @return Native result; local validation is effect-free, accepted/uncertain MAC effects are retained.
  esp_err_t setFlowControl(bool enabled, uint32_t pauseAbility, uint64_t deadlineUs) noexcept;
  /// @brief Copy last SDK reconciliation locally; output unchanged on gate contention.
  /// @param[out] output Copied receipt on OK.
  /// @return OK or BUSY; no clock or I/O.
  Status sdkStateSnapshot(SdkStateSnapshot& output) noexcept;
  /// @brief Copy retained MAC/PHY flow-control effects locally.
  /// @param[out] output Copied receipt on OK, unchanged on BUSY.
  /// @return OK or BUSY; no clock or I/O.
  Status flowControlSnapshot(FlowControlRecord& output) noexcept;
  /// @brief Copy desired/observed/trusted core settings; SDK getters have different semantics.
  /// @param[out] output Copied snapshot on OK, unchanged on BUSY.
  /// @return OK or BUSY; no clock or I/O.
  Status settingsSnapshot(SettingsSnapshot& output) noexcept;
  /// @brief Withdraw core proof and SDK agreement without I/O; pending results remain historical.
  /// @param[in] reason Valid core invalidation reason, including external reset/writer.
  /// @return Core status or BUSY; stopped/running owner only, close external traffic first.
  Status invalidateDeviceState(InvalidationReason reason) noexcept;
  /// @brief Advance one admitted core operation by at most maxFrames, without sleeping.
  /// @param[in] token Exact admitted instance token.
  /// @param[in] maxFrames Physical frames allowed in this call, zero is bookkeeping only.
  /// @return Core progress/terminal error, or BUSY for native admission.
  Status pollOperation(OperationToken token, uint32_t maxFrames = MAX_BATCH_FRAMES) noexcept;
  /// @brief Deliver a full terminal core result; only successful take releases control admission.
  /// @param[in] token Exact token, never another instance's serial.
  /// @param[out] output Complete result on OK; unchanged on failure.
  /// @return Core delivery status; OK does not mean the delivered operation succeeded.
  Status takeResult(OperationToken token, OperationResult& output) noexcept;
  /// @brief Cancel locally without MDIO; retained effects/restoration barrier survive.
  /// @param[in] token Exact operation token.
  /// @return Core cancellation status or BUSY; no hardware rollback is implied.
  Status cancel(OperationToken token) noexcept;
  /// @brief Copy core progress locally; unchanged output on busy gate.
  /// @param[out] output Copied snapshot on OK.
  /// @return OK or BUSY; no clock or I/O.
  Status operationSnapshot(OperationSnapshot& output) noexcept;
  /// @brief Owner-routed policy-checked C22 read while stopped; output unchanged on failure.
  /// @param[in] address Core-admitted C22 register, 0..31 before policy checking.
  /// @param[out] output Assigned only on complete success.
  /// @param[in] budget Aggregate absolute deadline/frame allowance.
  /// @return Core validation/transport status or native BUSY.
  Status readClause22(uint32_t address, uint16_t& output, const IoBudget& budget) noexcept;
  /// @brief Owner-routed complete MMD read while stopped; output unchanged on failure.
  /// @param[in] devad Core-admitted MMD device address, 0..31.
  /// @param[in] address Core-admitted wire offset, 0..65535.
  /// @param[out] output Assigned only on complete success.
  /// @param[in] budget Aggregate absolute deadline/frame allowance.
  /// @return Core validation/transport status or native BUSY.
  Status readMmd(uint32_t devad, uint32_t address, uint16_t& output, const IoBudget& budget) noexcept;
  /// @brief Stopped owner raw write through the core's LEDCR-only policy.
  /// @param[in] address C22 LEDCR (0x18); selectors/commands reject.
  /// @param[in] value Sixteen-bit allowed field value.
  /// @param[in] budget Whole terminal transaction allowance.
  /// @return Core status or BUSY; accepted/uncertain mutation withdraws SDK agreement.
  Status writeClause22(uint32_t address, uint32_t value, const IoBudget& budget) noexcept;
  /// @brief Stopped owner complete four-frame MMD maintenance through core policy.
  /// @param[in] devad Vendor 31 only for writes.
  /// @param[in] address LEDCR alias only (0x0018); selectors/other aliases reject.
  /// @param[in] value Sixteen-bit permitted field word.
  /// @param[in] budget Whole unsplit batch allowance.
  /// @return Core terminal status or BUSY; failure preserves uncertain effects.
  Status writeMmd(uint32_t devad, uint32_t address, uint32_t value, const IoBudget& budget) noexcept;
  /// @brief Stopped owner atomic five-frame stable LEDCR RMW.
  /// @param[in] devad Vendor 31 only.
  /// @param[in] address LEDCR alias 0x0018.
  /// @param[in] mask Nonzero sixteen-bit permitted stable field mask.
  /// @param[in] value Replacement subset of mask.
  /// @param[in] budget Whole unsplit transaction allowance.
  /// @return Core status or BUSY; no public selector or recursive lock path.
  Status modifyMmd(uint32_t devad, uint32_t address, uint32_t mask, uint32_t value, const IoBudget& budget) noexcept;
  /// @brief Explicit raw BMCR stable-field maintenance; never persistent desired configuration.
  /// @param[in] mask Nonzero subset of AN/speed/duplex mask 0x3100.
  /// @param[in] value Replacement subset of mask; commands/power fields reject.
  /// @param[in] budget Complete two-frame read/write allowance.
  /// @return Core status or BUSY; reinitialize/replay and public-reconcile before restart.
  Status modifyBasicControl(uint32_t mask, uint32_t value, const IoBudget& budget) noexcept;
  /// @brief Explicit stopped destructive acquisition through the sole core collector.
  /// @param[in] sourceMask Nonzero subset of ALL_EVIDENCE.
  /// @param[out] output Assigned on aggregate success; consumed prefixes remain cached on error.
  /// @param[in] budget Complete acquisition allowance.
  /// @return Core status or BUSY; ordinary raw reads cannot bypass this collector.
  Status acquireEvidence(uint32_t sourceMask, EvidenceSnapshot& output, const IoBudget& budget) noexcept;
  /// @brief Copy retained destructive samples locally, without acknowledging hardware.
  /// @param[out] output Copied history on OK, unchanged on BUSY.
  /// @return OK or BUSY; no clock or I/O.
  Status evidenceSnapshot(EvidenceSnapshot& output) noexcept;
  /// @brief Copy complete frame/release/uncertainty receipt locally.
  /// @param[out] output Copied snapshot on OK, unchanged on BUSY.
  /// @return OK or BUSY; no clock or I/O.
  Status transportSnapshot(TransportSnapshot& output) noexcept;
  /// @brief Explicit stopped-owner scan of selected addresses, at most 64 frames total.
  /// @param[in] addressMask Nonzero bit mask; no address is automatically selected.
  /// @param[out] output Complete scan only on OK, unchanged on failure; partial receipt stays cached.
  /// @param[in] budget Whole-scan absolute deadline and physical-frame allowance.
  /// @return OK for complete raw reads, otherwise input/admission/transport/deadline failure.
  /// @note Only fixed PHYIDR1/2 reads; returned raw words do not establish exact-part support.
  Status discover(uint32_t addressMask, DiscoverySnapshot& output, const IoBudget& budget) noexcept;
  /// @brief Copy retained discovery prefixes/status locally; unchanged on gate contention.
  /// @param[out] output Value copy on OK, including partial failed acquisitions.
  /// @return OK or BUSY; no I/O or clock.
  Status discoverySnapshot(DiscoverySnapshot& output) noexcept;
  /// @brief Retry only a lost link event, never replay MAC LINK UP; finite event wait.
  /// @param[in] deadlineUs Exclusive absolute esp_timer allowance in microseconds.
  /// @return SDK post error, TIMEOUT, or INVALID_STATE when no current event is pending.
  /// @note Accepted late delivery clears eventLost but returns TIMEOUT; it must not
  /// be replayed. The receipt survives concurrent snapshot gate contention.
  esp_err_t retryLinkEvent(uint64_t deadlineUs) noexcept;
  /// @brief Mandatory raw ioctl hooks; unconditional rejection, no context lookup or output changes.
  /// @return ESP_ERR_NOT_SUPPORTED for every input, including null/unpublished handles.
  static esp_err_t rejectRead(esp_eth_handle_t, uint32_t, uint32_t, uint32_t*) noexcept;
  /// @brief Mandatory raw write hook; rejects even calls from the owner's task.
  /// @return ESP_ERR_NOT_SUPPORTED for every input; no MAC frame is issued.
  static esp_err_t rejectWrite(esp_eth_handle_t, uint32_t, uint32_t, uint32_t) noexcept;

private:
  explicit Phy(const PhyConfig& config) noexcept;
  ~Phy() = default;
  static Phy& owner(esp_eth_phy_t* phy) noexcept;
  static uint64_t now(void*) noexcept;
  static BatchResult execute(void*, const MdioBatch&, const IoBudget&) noexcept;
  static void fenceCallback(void*) noexcept;
  static esp_err_t setMediator(esp_eth_phy_t*, esp_eth_mediator_t*) noexcept;
  static esp_err_t localInit(esp_eth_phy_t*) noexcept;
  static esp_err_t localDeinit(esp_eth_phy_t*) noexcept;
  static esp_err_t resetHardware(esp_eth_phy_t*) noexcept;
  static esp_err_t rejectReset(esp_eth_phy_t*) noexcept;
  static esp_err_t rejectPower(esp_eth_phy_t*, bool) noexcept;
  static esp_err_t autonego(esp_eth_phy_t*, eth_phy_autoneg_cmd_t, bool*) noexcept;
  static esp_err_t getLink(esp_eth_phy_t*) noexcept;
  static esp_err_t setLink(esp_eth_phy_t*, eth_link_t) noexcept;
  static esp_err_t getAddress(esp_eth_phy_t*, uint32_t*) noexcept;
  static esp_err_t setAddress(esp_eth_phy_t*, uint32_t) noexcept;
  static esp_err_t acknowledgePause(esp_eth_phy_t*, uint32_t) noexcept;
  static esp_err_t rejectLoopback(esp_eth_phy_t*, bool) noexcept;
  static esp_err_t acknowledgeSpeed(esp_eth_phy_t*, eth_speed_t) noexcept;
  static esp_err_t acknowledgeDuplex(esp_eth_phy_t*, eth_duplex_t) noexcept;
  static esp_err_t deletePhy(esp_eth_phy_t*) noexcept;
  bool enter() noexcept;
  bool enterUntil(uint64_t deadlineUs) noexcept;
  bool enterStartCallback() noexcept;
  bool enterControlCallback() noexcept;
  void leave() noexcept;
  void record(esp_err_t error, bool cleanup = false) noexcept;
  void recordCore(Status status, bool cleanup = false) noexcept;
  esp_err_t publish(PublishedTuple tuple, uint64_t deadlineUs, bool stopping = false) noexcept;
  esp_err_t finishStop(uint64_t deadlineUs, esp_err_t downError) noexcept;
  void rememberCleanup(esp_err_t error, bool deadline = false) noexcept;
  IoBudget callbackBudget(uint32_t frames) const noexcept;
  void shareQuiet() noexcept;
  bool verifiedSettings() const noexcept;
  void refreshAgreement() noexcept;
  void unexpectedStart(bool startPosted) noexcept;
  void deferStart(bool startPosted) noexcept;
  bool stoppedAdmission() const noexcept;
  enum class ControlEntry : uint8_t { NONE, AUTONEGO, SPEED, DUPLEX, PAUSE };
  enum class ControlCompletion : uint8_t { NONE, SDK, FLOW };
  bool admittedControl(ControlEntry entry) const noexcept;
  ControlEntry _controlEntry{ControlEntry::NONE};
  std::atomic<TaskHandle_t> _controlTask{nullptr};
  std::atomic<uint64_t> _controlDeadline{0};
  SdkStateSnapshot _sdkState{};
  FlowControlRecord _flow{};
  SdkStateSnapshot _pendingSdk{};
  FlowControlRecord _pendingFlow{};
  NativeStatus _pendingControlError{};
  std::atomic<ControlCompletion> _controlCompletion{ControlCompletion::NONE};
  std::atomic<bool> _unexpectedFlowEffect{false};
  struct PublicationResult {
    PublishedTuple tuple{};
    bool uncertain{false}, eventLost{false}, cleanup{false};
    NativeStatus status{};
  };
  PublicationResult _pendingPublication{};
  std::atomic<bool> _publicationComplete{false};
  bool _frameworkStopOwed{false}, _ownsSilence{false};
  PhyConfig _config;
  PhyHandle _handle{};
  DP8382x _core{};
  SemaphoreHandle_t _gate{nullptr};
  esp_timer_handle_t _fence{nullptr};
  esp_eth_mediator_t* _mediator{nullptr};
  esp_eth_handle_t _ethernet{nullptr};
  esp_eth_handle_t _installCandidate{nullptr};
  std::atomic<int64_t> _installOutcome{std::numeric_limits<int64_t>::min()};
  std::atomic<int64_t> _startOutcome{std::numeric_limits<int64_t>::min()};
  std::atomic<int64_t> _cleanupOutcome{std::numeric_limits<int64_t>::min()};
  std::atomic<int64_t> _latestCleanupOutcome{std::numeric_limits<int64_t>::min()};
  std::atomic<int64_t> _boardOutcome{std::numeric_limits<int64_t>::min()};
  std::atomic<bool> _installing{false};
  std::atomic<bool> _uninstallSeen{false};
  std::atomic<TaskHandle_t> _startTask{nullptr};
  bool _preStopDownAttempted{false}; ///< Lifecycle owner tries a lost DOWN once before framework stop.
  static constexpr int64_t OWNER_DEADLINE = std::numeric_limits<int64_t>::max();
  std::atomic<Phase> _phase{Phase::STOPPED};
  std::atomic<uint8_t> _deferredStart{0}; ///< Bit 0 FSM changed; bit 1 START was posted before get_link.
  LifecycleRecord _record{};
  DiscoverySnapshot _discovery{};
  std::atomic<bool> _installed{false};
  bool _boardReady{false}, _installAttempted{false}, _fenceArmed{false};
  bool _startPosted{false}, _stopPosted{false}, _notifying{false};
  bool _macStopAccepted{false};
  bool _cleanupPublicationReported{false}; ///< Sole lifecycle caller detects an already-recorded inner callback failure.
  std::atomic<uint64_t> _lifecycleDeadline{0};
};
} // namespace DP8382x::EspIdf
