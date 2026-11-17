/// @file Phy.h
/// @brief Native ESP-IDF v6.1 owner, terminal MAC transport and retained lifecycle.
#pragma once

#include <DP8382x/DP8382x.h>
#include <esp_eth.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
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
  START, ///< Framework start and initial link collection.
  STOP, ///< Framework stop request.
  FENCE, ///< Timer-task callback fence.
  MAC_STOP, ///< Direct actual MAC stop observation.
  RETAINED ///< Admission closed; objects retained until restart.
};
/// @brief Last accepted MAC projection; down never invents speed or duplex.
struct PublishedTuple {
  bool up{false}; ///< True only after accepted LINK UP notification.
  LinkSpeed speed{LinkSpeed::UNKNOWN}; ///< Qualified speed for the last UP.
  Duplex duplex{Duplex::UNKNOWN}; ///< Qualified duplex for the last UP.
  bool peerPause{false}; ///< Narrow SDK symmetric partner capability, not resolved RX/TX.
  bool speedAccepted{false}; ///< SPEED callback accepted this field in the latest update.
  bool duplexAccepted{false}; ///< DUPLEX callback accepted this field in the latest update.
  bool pauseAccepted{false}; ///< PAUSE callback accepted this field in the latest update.
};
/// @brief Persistent primary/cleanup evidence for framework partial effects.
struct LifecycleRecord {
  LifecycleStage stage{LifecycleStage::FACTORY}; ///< Last attempted boundary.
  Status firstError{}; ///< First failure; detail preserves the native error.
  Status cleanupError{}; ///< First stop/fence/publication cleanup failure.
  PublishedTuple publishedTuple{}; ///< Last accepted projection.
  bool publicationUncertain{false}; ///< Unknown MAC partial effect; admission closes.
  bool eventLost{false}; ///< Current accepted LINK event is pending; older failures remain in firstError.
};
/// @brief Borrowed controller-wide exclusion and readiness shared by every PHY.
/// @pre mutex is an application-created nonrecursive mutex. This object and mutex
/// outlive all MAC/driver callbacks, including terminal retained-resource state.
struct SharedBus {
  SemaphoreHandle_t mutex{nullptr}; ///< Serializes complete MDIO batches.
  std::atomic<bool> quarantined{false}; ///< Unknown controller completion; external repair only.
  std::atomic<bool> suspended{true}; ///< Board reset/readiness or owner exclusion.
  std::atomic<uint64_t> quietUntilUs{0}; ///< Shared nonwrapping esp_timer readiness floor.
};
/// @brief Explicit board reset procedure, supplied by the application.
struct BoardControl {
  void* context{nullptr}; ///< Borrowed through all calls and retained resources.
  /// @brief Finite terminal board callback; true asserts RESET_N, false releases it.
  esp_err_t (*setReset)(void*, bool){nullptr};
  uint64_t powerOnAtUs{0}; ///< Actual power-on lower reference; time zero is valid.
  uint64_t clockStableAtUs{0}; ///< Known beginning of uninterrupted valid REF_CLK.
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
  uint32_t linkPollMs{2000}; ///< Explicit owner scheduling policy; not EtherCAT timing.
  uint32_t callbackBudgetUs{100000}; ///< Aggregate live-link/restart callback allowance.
  uint32_t notificationUs{0}; ///< Required owner-qualified bound per mediator notification; scheduling included.
  uint32_t lockWaitUs{1000}; ///< Controller mutex wait cap; scheduler latency remains conditional.
  uint32_t maxFencePolls{100000}; ///< Finite timer fence polling cap.
  uint32_t maxStopAttempts{1}; ///< Finite actual MAC stop attempts.
  bool pinnedP4Mac{false}; ///< Attests actual pinned P4 MAC, whose valid set_link always succeeds.
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
  /// @return Native error/timeout; failed reset leaves shared admission closed, no MDIO occurs.
  esp_err_t prepareBoard(const BoardControl& board, uint64_t deadlineUs) noexcept;
  /// @brief Install with mandatory rejecting raw hooks after board readiness; no PHY I/O.
  /// @param[out] output Published only on success; SDK install failure has no partial handle.
  /// @return SDK error unchanged; failed install leaves only owner MAC/PHY deletion.
  esp_err_t install(esp_eth_handle_t& output) noexcept;
  /// @brief Start only after typed initialization and verified AN configuration; callbacks do no waits.
  /// @param[in] deadlineUs Absolute enclosing allowance, including failure stop/fence.
  /// @return Primary SDK error; failed start performs bounded stop and retains cleanup separately.
  esp_err_t start(uint64_t deadlineUs) noexcept;
  /// @brief Stop, fence once, repair missing DOWN/STOP, then observe actual MAC stop status.
  /// @param[in] deadlineUs Absolute allowance; timeout retains objects and retry polls the same fence.
  /// @return First stop/cleanup error; no controller/instance gate survives waits.
  esp_err_t stop(uint64_t deadlineUs) noexcept;
  /// @brief Close admission permanently and retain installed resources under RX lifetime gate.
  /// @param[in] deadlineUs Absolute enclosing stop/fence allowance, microseconds.
  /// @return ESP_ERR_NOT_SUPPORTED after bounded stop; earlier errors remain in the record.
  esp_err_t retire(uint64_t deadlineUs) noexcept;
  /// @brief Delete only a never-installed idle adapter; never deletes the borrowed MAC/bus.
  /// @return INVALID_STATE if installed, active or fence live; otherwise actual timer deletion error/OK.
  esp_err_t destroy() noexcept;
  /// @brief Copy the lifecycle journal locally; unchanged output if gate is busy.
  /// @param[out] output Value copy on OK.
  /// @return OK or BUSY; no clock, MDIO or SDK callback.
  Status lifecycleSnapshot(LifecycleRecord& output) noexcept;
  /// @brief Admit the core's explicit reset/initialization while stopped; token unchanged on failure.
  /// @param[in] options NONE/PCS/RESTART; HARD requires the later shared-board owner route.
  /// @param[in] budget Original aggregate core deadline/frame/poll policy.
  /// @param[out] token Assigned only on IN_PROGRESS.
  /// @return Core admission status, BUSY or INVALID_CONFIG for unavailable native route.
  Status startInitialize(const InitializeOptions& options, const OperationBudget& budget,
                         OperationToken& token) noexcept;
  /// @brief Admit verified AN intent while stopped; mode changes remain prompt 38.
  /// @param[in] settings Validated persistent AN intent.
  /// @param[in] budget Original aggregate core allowance.
  /// @param[out] token Assigned only on IN_PROGRESS.
  /// @return Core admission status, BUSY or INVALID_CONFIG for unavailable native route.
  Status startConfigure(const LinkSettings& settings, const OperationBudget& budget,
                        OperationToken& token) noexcept;
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
  static esp_err_t rejectPause(esp_eth_phy_t*, uint32_t) noexcept;
  static esp_err_t rejectLoopback(esp_eth_phy_t*, bool) noexcept;
  static esp_err_t rejectSpeed(esp_eth_phy_t*, eth_speed_t) noexcept;
  static esp_err_t rejectDuplex(esp_eth_phy_t*, eth_duplex_t) noexcept;
  static esp_err_t deletePhy(esp_eth_phy_t*) noexcept;
  bool enter() noexcept;
  void leave() noexcept;
  void record(esp_err_t error, bool cleanup = false) noexcept;
  esp_err_t publish(PublishedTuple tuple, uint64_t deadlineUs) noexcept;
  esp_err_t finishStop(uint64_t deadlineUs, esp_err_t initial, bool stopCalled = true) noexcept;
  IoBudget callbackBudget(uint32_t frames) const noexcept;
  void shareQuiet() noexcept;
  PhyConfig _config;
  PhyHandle _handle{};
  DP8382x _core{};
  SemaphoreHandle_t _gate{nullptr};
  esp_timer_handle_t _fence{nullptr};
  esp_eth_mediator_t* _mediator{nullptr};
  esp_eth_handle_t _ethernet{nullptr};
  esp_eth_handle_t _installCandidate{nullptr};
  std::atomic<int64_t> _installOutcome{std::numeric_limits<int64_t>::min()};
  std::atomic<bool> _installing{false};
  Phase _phase{Phase::STOPPED};
  LifecycleRecord _record{};
  DiscoverySnapshot _discovery{};
  bool _boardReady{false}, _installed{false}, _installAttempted{false}, _fenceArmed{false};
  bool _startPosted{false}, _stopPosted{false}, _notifying{false};
  uint64_t _lifecycleDeadline{0};
};
} // namespace DP8382x::EspIdf
