/// @file Phy.cpp
/// @brief Public-API native lifecycle and complete terminal MAC batches.
#include <DP8382x/esp_idf/Phy.h>
#include <esp_event.h>
#include <freertos/task.h>
#include <cstddef>
#include <limits>
#include <new>
#include <type_traits>

namespace DP8382x::EspIdf {
static_assert(std::is_standard_layout<PhyHandle>::value && offsetof(PhyHandle, phy) == 0,
              "Public PHY must be the first handle member");
namespace {
// Tagged owner receipts cannot alias a signed 32-bit esp_err_t or the empty sentinel.
constexpr int64_t OWNER_TIMEOUT_BASE = int64_t{1} << 32;
constexpr int64_t OWNER_CLOCK_REGRESSION = -OWNER_TIMEOUT_BASE;
constexpr int64_t OWNER_DEADLINE = OWNER_TIMEOUT_BASE + static_cast<int32_t>(TimeoutDetail::DEADLINE);
int64_t timeoutReceipt(TimeoutDetail reason) noexcept {
  return OWNER_TIMEOUT_BASE + static_cast<int32_t>(reason);
}
NativeStatus terminalReceipt(int64_t receipt, const char* message) noexcept {
  if (receipt == OWNER_CLOCK_REGRESSION)
    return {Status::Error(Err::INVALID_CONFIG, "Board clock regressed", static_cast<int32_t>(ConfigDetail::CLOCK_CONTRACT))};
  if (receipt >= OWNER_TIMEOUT_BASE)
    return {Status::Error(Err::OPERATION_TIMEOUT, message, static_cast<int32_t>(receipt - OWNER_TIMEOUT_BASE))};
  return {{}, static_cast<esp_err_t>(receipt)};
}
Status busy() noexcept { return Status::Error(Err::BUSY, "Native owner is unavailable", static_cast<int32_t>(BusyDetail::OWNER_ADMISSION)); }
Status busDenied(bool quarantined) noexcept {
  return Status::Error(Err::BUSY, "Shared controller unavailable", quarantined ?
    static_cast<int32_t>(BusyDetail::CONTROLLER_QUARANTINED) : static_cast<int32_t>(BusyDetail::OWNER_ADMISSION));
}
Status nativeStatus(esp_err_t error) noexcept {
  if (error == ESP_OK) return Status::Ok();
  return Status::Error(error == ESP_ERR_TIMEOUT ? Err::MDIO_TIMEOUT : Err::MDIO_ERROR,
                       "Native MDIO failure", error);
}
esp_err_t nativeError(Status status) noexcept {
  if (status.ok() || status.inProgress()) return ESP_OK;
  if ((status.code == Err::MDIO_ERROR || status.code == Err::MDIO_TIMEOUT) && status.detail != 0)
    return static_cast<esp_err_t>(status.detail);
  if (status.code == Err::OPERATION_TIMEOUT) return ESP_ERR_TIMEOUT;
  if (status.code == Err::INVALID_PARAM || status.code == Err::INVALID_CONFIG) return ESP_ERR_INVALID_ARG;
  return ESP_ERR_INVALID_STATE;
}
uint64_t timeNow() noexcept { return static_cast<uint64_t>(esp_timer_get_time()); }
bool addTime(uint64_t value, uint64_t delta, uint64_t& output) noexcept {
  if (delta > std::numeric_limits<uint64_t>::max() - value) return false;
  output = value + delta;
  return true;
}
TickType_t ticksWithin(uint64_t us) noexcept {
  const auto cap = static_cast<uint64_t>(portMAX_DELAY - 1U);
  // Divide before multiplying: neither a nonintegral tick period nor a large
  // absolute remaining allowance may round a requested wait upward or overflow.
  const auto seconds = us / 1000000U;
  if (seconds > cap / configTICK_RATE_HZ) return static_cast<TickType_t>(cap);
  const uint64_t ticks = seconds * configTICK_RATE_HZ +
      (us % 1000000U) * configTICK_RATE_HZ / 1000000U;
  return static_cast<TickType_t>(ticks < cap ? ticks : cap);
}
void extendQuiet(SharedBus& bus, uint64_t due) noexcept {
  auto previous = bus.quietUntilUs.load();
  while (previous < due && !bus.quietUntilUs.compare_exchange_weak(previous, due)) {}
}
bool same(PublishedTuple a, PublishedTuple b) noexcept {
  return a.up == b.up && (!a.up || (a.speed == b.speed && a.duplex == b.duplex && a.peerPause == b.peerPause));
}
} // namespace

Phy::Phy(const PhyConfig& config) noexcept : _config(config) {
  _handle.owner = this;
  auto& phy = _handle.phy;
  phy.set_mediator = setMediator; phy.reset = rejectReset; phy.reset_hw = resetHardware;
  phy.init = localInit; phy.deinit = localDeinit; phy.autonego_ctrl = autonego;
  phy.get_link = getLink; phy.set_link = setLink; phy.pwrctl = rejectPower;
  phy.set_addr = setAddress; phy.get_addr = getAddress;
  phy.advertise_pause_ability = acknowledgePause; phy.loopback = rejectLoopback;
  phy.set_speed = acknowledgeSpeed; phy.set_duplex = acknowledgeDuplex; phy.del = deletePhy;
}
Phy& Phy::owner(esp_eth_phy_t* phy) noexcept { return *reinterpret_cast<PhyHandle*>(phy)->owner; }
uint64_t Phy::now(void*) noexcept { return timeNow(); }
void Phy::fenceCallback(void*) noexcept {} // No borrowed context can outlive a fence invocation.
bool Phy::enter() noexcept {
  if (xSemaphoreTake(_gate, 0) != pdTRUE) return false;
  const auto boardOutcome = _boardOutcome.exchange(std::numeric_limits<int64_t>::min());
  if (boardOutcome != std::numeric_limits<int64_t>::min()) {
    const auto status = terminalReceipt(boardOutcome, "Board readiness allowance exhausted");
    if (!status.coreStatus.ok()) recordCore(status.coreStatus);
    else record(status.sdkError);
    _boardReady = boardOutcome == ESP_OK; _phase = Phase::STOPPED;
  }
  const auto outcome = _installOutcome.exchange(std::numeric_limits<int64_t>::min());
  if (outcome != std::numeric_limits<int64_t>::min()) {
    const auto error = static_cast<esp_err_t>(outcome);
    _installed = error == ESP_OK; _ethernet = _installCandidate;
    _phase = Phase::STOPPED; record(error);
    if (!_installed) _mediator = nullptr;
  }
  if (_publicationComplete.exchange(false)) {
    _record.publishedTuple = _pendingPublication.tuple;
    _record.publicationUncertain = _pendingPublication.uncertain;
    _record.eventLost = _pendingPublication.eventLost;
    auto& status = _pendingPublication.cleanup ? _record.cleanupError : _record.firstError;
    if (status.ok() && !_pendingPublication.status.ok()) status = _pendingPublication.status;
    if (_pendingPublication.cleanup && !_pendingPublication.status.ok()) _record.lastCleanupError = _pendingPublication.status;
    if (_pendingPublication.uncertain) _record.stopRequired = true;
    if (!_pendingPublication.uncertain && !_pendingPublication.tuple.up) _record.observationDownOwed = false;
    _notifying = false;
  }
  const auto started = _deferredStart.exchange(0);
  if (started) {
    if (_phase == Phase::STARTING && (started & 2U)) _startPosted = true;
    else unexpectedStart((started & 2U) != 0U);
  }
  // Adopt earlier START-event evidence while its STARTING phase still exists.
  // Failed-start cleanup may need it to repair the unarmed-timer STOP path.
  const auto startOutcome = _startOutcome.exchange(std::numeric_limits<int64_t>::min());
  if (startOutcome != std::numeric_limits<int64_t>::min()) {
    if (startOutcome == OWNER_DEADLINE)
      recordCore(Status::Error(Err::OPERATION_TIMEOUT, "Native start completed late", static_cast<int32_t>(TimeoutDetail::DEADLINE)));
    else record(static_cast<esp_err_t>(startOutcome));
    _phase = startOutcome == ESP_OK ? Phase::RUNNING : Phase::STOPPING;
  }
  const auto cleanupOutcome = _cleanupOutcome.exchange(std::numeric_limits<int64_t>::min());
  if (cleanupOutcome != std::numeric_limits<int64_t>::min()) {
    const auto status = terminalReceipt(cleanupOutcome, "Native cleanup allowance exhausted");
    if (!status.coreStatus.ok()) recordCore(status.coreStatus, true);
    else record(status.sdkError, true);
  }
  const auto latest = _latestCleanupOutcome.exchange(std::numeric_limits<int64_t>::min());
  if (latest != std::numeric_limits<int64_t>::min())
    _record.lastCleanupError = terminalReceipt(latest, "Native cleanup allowance exhausted");
  // One logical control owner publishes its complete terminal receipt without
  // needing to win a race with copied snapshots after an external callback.
  const auto completed = _controlCompletion.exchange(ControlCompletion::NONE);
  if (completed != ControlCompletion::NONE) {
    _controlEntry = ControlEntry::NONE; _controlTask = nullptr;
    if (_phase == Phase::CONTROL) _phase = _frameworkStopOwed ? Phase::STOPPING : Phase::STOPPED;
    if (completed == ControlCompletion::SDK) {
      _sdkState = _pendingSdk;
      _sdkState.reconciled = _sdkState.reconciled && verifiedSettings() && !_frameworkStopOwed && _phase == Phase::STOPPED;
      if (!_sdkState.reconciled && _sdkState.status.ok())
        _sdkState.status = {Status::Error(Err::NOT_INITIALIZED, "SDK proof invalidated before publication")};
      if (!_sdkState.status.ok() && _record.firstError.ok()) _record.firstError = _sdkState.status;
    } else {
      _flow = _pendingFlow;
      _flow.reconciled = _flow.reconciled && !_frameworkStopOwed && _phase == Phase::STOPPED;
      if (!_pendingControlError.ok() && _record.firstError.ok()) _record.firstError = _pendingControlError;
    }
  }
  if (_unexpectedFlowEffect.load()) {
    _flow.macEffectPossible = true; _flow.reconciled = false;
    _flow.phyError = ESP_ERR_INVALID_STATE;
  }
  // Uninstall deletes the SDK timer before it calls deinit. This independent
  // latch wins over every late start/control receipt and never reopens that timer.
  if (_uninstallSeen.load()) {
    // The deleted framework timer is never touched again. Independent fence,
    // MAC stop, cancel and result delivery remain available until resolved.
    _frameworkStopOwed = false; _sdkState.reconciled = false;
    if (_phase == Phase::STOPPED || _phase == Phase::MAINTENANCE) {
      _phase = Phase::MAINTENANCE; _record.stage = LifecycleStage::RETAINED;
    } else {
      _phase = Phase::STOPPING; _record.stopRequired = true;
    }
    record(ESP_ERR_INVALID_STATE);
  }
  return true;
}
bool Phy::enterUntil(uint64_t deadlineUs, TimeoutDetail* reason) noexcept {
  const auto denied = [&](TimeoutDetail value) {
    if (reason) *reason = value;
    return false;
  };
  for (uint32_t polls = 0; polls < _config.maxFencePolls; ++polls) {
    const auto current = timeNow();
    if (current >= deadlineUs) return denied(TimeoutDetail::DEADLINE);
    if (enter()) {
      if (timeNow() < deadlineUs) return true;
      leave(); return denied(TimeoutDetail::DEADLINE);
    }
    const auto after = timeNow();
    if (after >= deadlineUs) return denied(TimeoutDetail::DEADLINE);
    if (polls + 1U == _config.maxFencePolls) return denied(TimeoutDetail::POLL_BUDGET);
    if (!ticksWithin(deadlineUs - after)) return denied(TimeoutDetail::BOUND_ADMISSION);
    vTaskDelay(1);
  }
  return denied(TimeoutDetail::POLL_BUDGET);
}
bool Phy::enterStartCallback(StartEntry entry) noexcept {
  if (_phase.load() == Phase::STARTING && _startTask.load() == xTaskGetCurrentTaskHandle()) {
    if (_startEntry != entry) return false;
    // Consume before any gate wait or MDIO. A nested call in this same task
    // cannot repeat the step or borrow the next step while this one runs.
    _startEntry = StartEntry::NONE;
    if (enterUntil(_lifecycleDeadline)) return true;
    // The pinned driver posts START before its initial link callback, even
    // when the callback cannot enter. Cleanup still owes the matching STOP.
    if (entry == StartEntry::LINK) _deferredStart.fetch_or(uint8_t{3});
    return false;
  }
  if (!enter()) {
    // An unowned SDK start in STOPPED/CONTROL can already have changed its
    // FSM. A contending task during an owned start supplies no such evidence.
    const auto phase = _phase.load();
    if (phase == Phase::STOPPED || phase == Phase::CONTROL)
      _deferredStart.fetch_or(entry == StartEntry::LINK ? uint8_t{3} : uint8_t{1});
    return false;
  }
  // enter() first adopts any completed start. A timer can then observe RUNNING.
  if (_phase == Phase::STARTING) { leave(); return false; }
  return true;
}
bool Phy::enterControlCallback() noexcept {
  return _phase.load() == Phase::CONTROL && _controlTask.load() == xTaskGetCurrentTaskHandle()
      ? enterUntil(_controlDeadline.load()) : enter();
}
void Phy::leave() noexcept { (void)xSemaphoreGive(_gate); }
void Phy::record(esp_err_t error, bool cleanup) noexcept {
  auto& target = cleanup ? _record.cleanupError : _record.firstError;
  if (error != ESP_OK && target.ok()) target = {{}, error};
  if (error != ESP_OK && cleanup) _record.lastCleanupError = {{}, error};
}
void Phy::recordCore(Status status, bool cleanup) noexcept {
  auto& target = cleanup ? _record.cleanupError : _record.firstError;
  if (!status.ok() && !status.inProgress() && target.ok()) target = {status};
  if (!status.ok() && !status.inProgress() && cleanup) _record.lastCleanupError = {status};
}

esp_err_t Phy::create(const PhyConfig& config, Phy*& output) noexcept {
  if (!config.mac || !config.bus || !config.bus->mutex ||
      !config.mac->read_phy_reg || !config.mac->write_phy_reg || !config.mac->stop ||
      !config.mac->set_mediator || !config.mac->init || !config.mac->deinit ||
      !config.mac->set_link || !config.mac->set_speed || !config.mac->set_duplex ||
      !config.mac->set_peer_pause_ability ||
      !config.linkPollMs || config.linkPollMs > std::numeric_limits<uint32_t>::max() / 1000U ||
      !config.callbackBudgetUs || !config.cleanupBudgetUs || !config.notificationUs || !config.lockWaitUs ||
      config.core.transport.bounds.fixedUs < config.lockWaitUs ||
      !config.maxFencePolls || !config.maxStopAttempts || !config.maxObservationMisses) return ESP_ERR_INVALID_ARG;
  const auto& bounds = config.core.transport.bounds;
  const uint64_t linkBound = uint64_t{2} * bounds.fixedUs + uint64_t{10} * bounds.frameUs +
      bounds.clockLagUs + uint64_t{5} * config.notificationUs;
  if (config.callbackBudgetUs <= linkBound || uint64_t{config.linkPollMs} * 1000U < config.callbackBudgetUs)
    return ESP_ERR_INVALID_ARG;
  auto* phy = new (std::nothrow) Phy(config);
  if (!phy) return ESP_ERR_NO_MEM;
  auto coreConfig = config.core;
  coreConfig.transport.context = phy;
  coreConfig.transport.executeBatch = execute;
  coreConfig.transport.nowUs = now;
  auto status = phy->_core.bind(coreConfig);
  if (!status.ok()) { delete phy; return ESP_ERR_INVALID_ARG; }
  phy->_gate = xSemaphoreCreateMutex();
  if (!phy->_gate) { delete phy; return ESP_ERR_NO_MEM; }
  esp_timer_create_args_t args{};
  args.callback = fenceCallback; args.dispatch_method = ESP_TIMER_TASK; args.name = "dp8382x_fence";
  const auto error = esp_timer_create(&args, &phy->_fence);
  if (error != ESP_OK) { vSemaphoreDelete(phy->_gate); delete phy; return error; }
  output = phy;
  return ESP_OK;
}

BatchResult Phy::execute(void* context, const MdioBatch& batch, const IoBudget& budget) noexcept {
  auto& self = *static_cast<Phy*>(context);
  auto& bus = *self._config.bus;
  BatchResult report{};
  report.status = Status::Ok(); report.releaseStatus = Status::Ok();
  report.controllerReusable = !bus.quarantined.load();
  const auto admitted = [&]() {
    const auto state = bus.admission.load();
    return state == BusAdmission::AVAILABLE || (state == BusAdmission::INITIALIZING && self._ownsSilence);
  };
  if (bus.quarantined.load() || !admitted() || timeNow() < bus.quietUntilUs.load()) {
    const bool quarantined = bus.quarantined.load();
    report.status = busDenied(quarantined); report.controllerReusable = !quarantined; return report;
  }
  const auto before = timeNow();
  const auto& bounds = self._config.core.transport.bounds;
  const uint64_t required = uint64_t{bounds.fixedUs} + uint64_t{batch.frameCount} * bounds.frameUs + bounds.clockLagUs;
  if (before >= budget.deadlineUs || batch.frameCount > budget.maxFrames || required >= budget.deadlineUs - before) {
    const auto detail = before >= budget.deadlineUs ? TimeoutDetail::DEADLINE :
        batch.frameCount > budget.maxFrames ? TimeoutDetail::FRAME_BUDGET : TimeoutDetail::BOUND_ADMISSION;
    report.status = Status::Error(Err::OPERATION_TIMEOUT, "Native batch budget expired", static_cast<int32_t>(detail)); return report;
  }
  const auto remaining = budget.deadlineUs - before;
  const auto wait = remaining < self._config.lockWaitUs ? remaining : self._config.lockWaitUs;
  const auto ticks = ticksWithin(wait);
  if (xSemaphoreTake(bus.mutex, ticks) != pdTRUE) {
    report.status = ticks == 0 ? busy() : nativeStatus(ESP_ERR_TIMEOUT);
    return report;
  }
  if (bus.quarantined.load() || !admitted() || timeNow() < bus.quietUntilUs.load()) {
    report.status = busDenied(bus.quarantined.load());
  } else {
    for (uint8_t i = 0; i < batch.frameCount; ++i) {
      if (timeNow() >= budget.deadlineUs) {
        report.status = Status::Error(Err::OPERATION_TIMEOUT, "Native frame deadline expired", static_cast<int32_t>(TimeoutDetail::DEADLINE)); break;
      }
      const auto& frame = batch.frames[i]; auto& result = report.frames[i];
      result.attempted = true;
      esp_err_t error;
      uint32_t word = 0;
      if (frame.kind == FrameKind::READ) {
        error = self._config.mac->read_phy_reg(self._config.mac, batch.phyAddress, frame.registerAddress, &word);
      } else {
        word = frame.value;
        if (frame.readIndex != NO_READ_INDEX)
          word |= uint32_t{report.frames[frame.readIndex].value} & frame.preserveMask;
        error = self._config.mac->write_phy_reg(self._config.mac, batch.phyAddress, frame.registerAddress, word);
      }
      if (error == ESP_OK && frame.kind == FrameKind::READ && word > 0xFFFFU) error = ESP_ERR_INVALID_RESPONSE;
      if (error == ESP_ERR_INVALID_STATE && self._config.pinnedP4Mac) {
        // Pinned P4 read/write reject a busy controller before issuing a command.
        // Preserve a completed prefix, but never invent an attempted failing frame.
        // Busy while we own the whole-batch mutex means controller idle is unknown.
        result = {};
        report.status = nativeStatus(error);
        report.releaseStatus = Status::Error(Err::MDIO_ERROR, "Controller idle not established", error);
        bus.quarantined.store(true);
        break;
      }
      result.completed = error == ESP_OK;
      result.valueValid = result.completed && frame.kind == FrameKind::READ;
      if (result.valueValid) result.value = static_cast<uint16_t>(word);
      result.effectPossible = error != ESP_OK;
      if (error != ESP_OK) {
        report.status = nativeStatus(error);
        report.releaseStatus = Status::Error(Err::MDIO_ERROR, "Controller completion remains uncertain", error);
        // A public MAC failure does not prove controller idle or a no-effect read/write.
        bus.quarantined.store(true); break;
      }
    }
  }
  // This receipt describes our transaction; a later owner may quarantine the
  // bus immediately after unlock without invalidating our completed samples.
  report.controllerReusable = !bus.quarantined.load();
  if (xSemaphoreGive(bus.mutex) != pdTRUE) {
    report.releaseStatus = Status::Error(Err::MDIO_ERROR, "Native batch mutex release failed", ESP_FAIL);
    report.controllerReusable = false; bus.quarantined.store(true);
  }
  return report;
}

esp_err_t Phy::prepareBoard(const BoardControl& board, uint64_t deadlineUs) noexcept {
  if (!board.setReset || board.assertionUs < 25U || !board.maxPolls || !board.pinCallbackUs)
    return ESP_ERR_INVALID_ARG;
  if (!enter()) return ESP_ERR_INVALID_STATE;
  if (_installAttempted || _phase != Phase::STOPPED || _notifying) { leave(); return ESP_ERR_INVALID_STATE; }
  const auto entry = timeNow();
  if (board.powerOnAtUs > entry || board.clockStableAtUs > entry) {
    leave(); return ESP_ERR_INVALID_ARG;
  }
  auto& bus = *_config.bus;
  const uint64_t lag = _config.core.transport.bounds.clockLagUs;
  uint64_t porDue = 0;
  const bool porKnown = addTime(board.powerOnAtUs, 50000U + lag, porDue);
  // A reported power event withdraws this owner's earlier reset proof and
  // publishes the new POR silence even when this call is rejected below
  // (expired allowance, quarantine or another reservation). The floor merges
  // by atomic maximum and never replaces another owner's reservation.
  _boardReady = false;
  if (porKnown) extendQuiet(bus, porDue);
  if (entry >= deadlineUs) {
    recordCore(Status::Error(Err::OPERATION_TIMEOUT, "Board readiness allowance expired", static_cast<int32_t>(TimeoutDetail::DEADLINE)));
    leave(); return ESP_ERR_TIMEOUT;
  }
  auto admission = bus.admission.load();
  if (bus.quarantined.load() ||
      (admission != BusAdmission::BOOT_CLOSED && admission != BusAdmission::AVAILABLE) ||
      !bus.admission.compare_exchange_strong(admission, BusAdmission::BOARD_PREPARING)) {
    leave(); return ESP_ERR_INVALID_STATE;
  }
  _phase = Phase::CONTROL; _record.stage = LifecycleStage::BOARD;
  leave();
  uint64_t assertionReady = 0, clockStable = 0, due = 0;
  const bool readinessKnown = porKnown && addTime(board.clockStableAtUs, lag, clockStable) &&
      addTime(clockStable > porDue ? clockStable : porDue, 100U, assertionReady);
  bool resetAttempted = false;
  // A valid interval spans phase transitions as well as the callbacks. A
  // regression invalidates shared readiness even before either pin is touched.
  uint64_t last = entry;
  bool clockRegressed = false;
  const auto sample = [&]() {
    const auto current = timeNow();
    const bool monotonic = current >= last;
    last = current;
    if (!monotonic) clockRegressed = true;
    return monotonic;
  };
  // Drain existing whole batches. No mutex survives pin callbacks or waits.
  int64_t result = readinessKnown ? ESP_OK : ESP_ERR_INVALID_ARG;
  if (result == ESP_OK && !sample()) result = OWNER_CLOCK_REGRESSION;
  if (result == ESP_OK && last >= deadlineUs) result = timeoutReceipt(TimeoutDetail::DEADLINE);
  if (result == ESP_OK) {
    if (xSemaphoreTake(bus.mutex, ticksWithin(deadlineUs - last)) != pdTRUE) {
      result = sample() ? timeoutReceipt(last >= deadlineUs ? TimeoutDetail::DEADLINE : TimeoutDetail::BOUND_ADMISSION)
                        : OWNER_CLOCK_REGRESSION;
    } else {
      if (bus.quarantined.load()) result = ESP_ERR_INVALID_STATE;
      if (xSemaphoreGive(bus.mutex) != pdTRUE) { bus.quarantined.store(true); result = ESP_FAIL; }
      if (!sample() && result == ESP_OK) result = OWNER_CLOCK_REGRESSION;
    }
  }
  uint32_t polls = 0;
  auto waitUntil = [&](uint64_t due) -> int64_t {
    for (;;) {
      if (!sample()) return OWNER_CLOCK_REGRESSION;
      if (last >= deadlineUs) return timeoutReceipt(TimeoutDetail::DEADLINE);
      if (last >= due) return ESP_OK;
      if (++polls >= board.maxPolls) return timeoutReceipt(TimeoutDetail::POLL_BUDGET);
      if (!ticksWithin(deadlineUs - last)) return timeoutReceipt(TimeoutDetail::BOUND_ADMISSION);
      vTaskDelay(1);
    }
  };
  // esp_timer's P4 counter conversion floors ticks/16. Use completion upper
  // bounds, including event timestamps supplied in the same time domain.
  if (result == ESP_OK) result = waitUntil(assertionReady);
  const uint64_t holdUs = board.assertionUs + lag;
  const uint64_t quietUs = 2000U + lag;
  // Each elapsed wait may overshoot by one rational RTOS tick. A first delay
  // immediately before a tick edge may also consume a poll with almost no wait.
  constexpr uint64_t tickUs = (1000000ULL + configTICK_RATE_HZ - 1) / configTICK_RATE_HZ;
  const auto maxDelays = [](uint64_t us) { return (us * configTICK_RATE_HZ + 999999U) / 1000000U + 1; };
  const uint64_t required = holdUs + quietUs + 2 * (board.pinCallbackUs + tickUs);
  if (result == ESP_OK) {
    if (!sample()) result = OWNER_CLOCK_REGRESSION;
    else if (last >= deadlineUs) result = timeoutReceipt(TimeoutDetail::DEADLINE);
    else if (required >= deadlineUs - last) result = timeoutReceipt(TimeoutDetail::BOUND_ADMISSION);
    else if (maxDelays(holdUs) + maxDelays(quietUs) >= board.maxPolls - polls)
      result = timeoutReceipt(TimeoutDetail::POLL_BUDGET);
  }
  if (result == ESP_OK) {
    resetAttempted = true;
    const auto beforePin = last;
    result = board.setReset(board.context, true);
    const bool assertionClockValid = sample();
    if (result == ESP_OK) {
      if (!assertionClockValid) result = OWNER_CLOCK_REGRESSION;
      else if (last >= deadlineUs) result = timeoutReceipt(TimeoutDetail::DEADLINE);
      else if (last - beforePin > board.pinCallbackUs || !addTime(last, holdUs, due))
        result = timeoutReceipt(TimeoutDetail::BOUND_ADMISSION);
      else result = waitUntil(due);
    }
    // Even an assertion error or scheduling overrun can leave RESET_N low.
    // Attempt release exactly once. Cleanup has its own finite callback bound.
    const bool releaseEntryValid = sample();
    const bool primaryBeforeRelease = result != ESP_OK;
    if (!releaseEntryValid && !primaryBeforeRelease) result = OWNER_CLOCK_REGRESSION;
    const auto beforeRelease = last;
    int64_t release = board.setReset(board.context, false);
    const bool releaseClockValid = sample();
    const auto releasedAt = last;
    if (release == ESP_OK) {
      if (!releaseClockValid || (!releaseEntryValid && primaryBeforeRelease)) release = OWNER_CLOCK_REGRESSION;
      else if (result == ESP_OK && releasedAt >= deadlineUs) release = timeoutReceipt(TimeoutDetail::DEADLINE);
      else if (releasedAt - beforeRelease > board.pinCallbackUs)
        release = timeoutReceipt(TimeoutDetail::BOUND_ADMISSION);
    }
    if (result == ESP_OK) result = release;
    else if (release != ESP_OK) rememberCleanupReceipt(release);
    if (!addTime(releasedAt, quietUs, due)) {
      due = std::numeric_limits<uint64_t>::max();
      if (result == ESP_OK) result = timeoutReceipt(TimeoutDetail::BOUND_ADMISSION);
    }
    extendQuiet(bus, due); // A reported release error can still have changed the pin.
    if (result == ESP_OK) result = waitUntil(due);
  }
  // Terminal receipt cannot be lost to an unrelated snapshot's short gate hold.
  // Release shared admission here; the next entry adopts only local readiness.
  _boardOutcome.store(result);
  bus.admission.store(result == ESP_OK ? BusAdmission::AVAILABLE :
      !resetAttempted && readinessKnown && !clockRegressed && !bus.quarantined.load() ? admission : BusAdmission::FAILED_CLOSED);
  if (enter()) leave();
  if (result == OWNER_CLOCK_REGRESSION) return ESP_ERR_INVALID_ARG;
  return result >= OWNER_TIMEOUT_BASE ? ESP_ERR_TIMEOUT : static_cast<esp_err_t>(result);
}

esp_err_t Phy::install(esp_eth_handle_t& output) noexcept {
  if (!enter()) return ESP_ERR_INVALID_STATE;
  if (!_boardReady || _installAttempted || _phase != Phase::STOPPED || _notifying ||
      _config.bus->quarantined.load() || timeNow() < _config.bus->quietUntilUs.load()) { leave(); return ESP_ERR_INVALID_STATE; }
  auto& bus = *_config.bus;
  auto admission = BusAdmission::AVAILABLE;
  if (!bus.admission.compare_exchange_strong(admission, BusAdmission::INSTALLING)) { leave(); return ESP_ERR_INVALID_STATE; }
  // MAC init resets the shared controller and changes its CSR clock. Drain any
  // earlier batch, then keep logical exclusion (not a mutex) across SDK waits.
  if (xSemaphoreTake(bus.mutex, 0) != pdTRUE) {
    bus.admission.store(BusAdmission::AVAILABLE); leave(); return ESP_ERR_INVALID_STATE;
  }
  const bool ready = !bus.quarantined.load() && timeNow() >= bus.quietUntilUs.load();
  if (xSemaphoreGive(bus.mutex) != pdTRUE) {
    bus.quarantined.store(true); bus.admission.store(BusAdmission::FAILED_CLOSED);
    leave(); return ESP_FAIL;
  }
  if (!ready) { bus.admission.store(BusAdmission::AVAILABLE); leave(); return ESP_ERR_INVALID_STATE; }
  _phase = Phase::CONTROL; _record.stage = LifecycleStage::INSTALL; _installAttempted = true;
  _installing.store(true);
  leave();
  esp_eth_config_t config = ETH_DEFAULT_CONFIG(_config.mac, &_handle.phy);
  config.check_link_period_ms = _config.linkPollMs;
  config.read_phy_reg = rejectRead; config.write_phy_reg = rejectWrite;
  esp_eth_handle_t candidate = nullptr;
  const auto error = esp_eth_driver_install(&config, &candidate);
  // Publish the terminal installer receipt independently of snapshot contention.
  // The next gated entry adopts it before inspecting any lifecycle state. No
  // timer PHY callback runs before start, and no callback reads this mailbox.
  _installCandidate = candidate;
  _installed.store(error == ESP_OK); // deinit must see lifetime even before receipt adoption.
  _installOutcome.store(error);
  _installing.store(false);
  bus.admission.store(error == ESP_OK ? BusAdmission::AVAILABLE : BusAdmission::FAILED_CLOSED);
  if (error == ESP_OK) output = candidate;
  return error;
}

IoBudget Phy::callbackBudget(uint32_t frames) const noexcept {
  uint64_t deadline = 0;
  if (!addTime(timeNow(), _config.callbackBudgetUs, deadline)) return {};
  if (_phase == Phase::STARTING && _lifecycleDeadline < deadline) deadline = _lifecycleDeadline;
  return {deadline, frames};
}
esp_err_t Phy::setMediator(esp_eth_phy_t* phy, esp_eth_mediator_t* mediator) noexcept {
  auto& self = owner(phy);
  if (!self._installing.load()) return ESP_ERR_INVALID_STATE;
  // The synchronous installer is the sole writer; concurrent snapshots never
  // read this field. Actual publication starts only after the install receipt.
  self._mediator = mediator; return ESP_OK;
}
esp_err_t Phy::resetHardware(esp_eth_phy_t* phy) noexcept { return localInit(phy); }
esp_err_t Phy::localInit(esp_eth_phy_t* phy) noexcept {
  auto& self = owner(phy);
  return self._installing.load() ? ESP_OK : ESP_ERR_INVALID_STATE;
}
esp_err_t Phy::localDeinit(esp_eth_phy_t* phy) noexcept {
  auto& self = owner(phy);
  if (self._installing.load()) return ESP_OK;
  if (!self._installed.load()) return ESP_OK;
  self._uninstallSeen.store(true);
  if (self.enter()) self.leave();
  return ESP_ERR_INVALID_STATE;
}
esp_err_t Phy::rejectReset(esp_eth_phy_t*) noexcept { return ESP_ERR_NOT_SUPPORTED; }
esp_err_t Phy::rejectPower(esp_eth_phy_t*, bool) noexcept { return ESP_ERR_NOT_SUPPORTED; }
esp_err_t Phy::rejectLoopback(esp_eth_phy_t*, bool) noexcept { return ESP_ERR_NOT_SUPPORTED; }
esp_err_t Phy::rejectRead(esp_eth_handle_t, uint32_t, uint32_t, uint32_t*) noexcept { return ESP_ERR_NOT_SUPPORTED; }
esp_err_t Phy::rejectWrite(esp_eth_handle_t, uint32_t, uint32_t, uint32_t) noexcept { return ESP_ERR_NOT_SUPPORTED; }
esp_err_t Phy::getAddress(esp_eth_phy_t* phy, uint32_t* address) noexcept {
  if (!address) return ESP_ERR_INVALID_ARG;
  *address = owner(phy)._config.core.phyAddress; return ESP_OK;
}
esp_err_t Phy::setAddress(esp_eth_phy_t* phy, uint32_t address) noexcept {
  return address == owner(phy)._config.core.phyAddress ? ESP_OK : ESP_ERR_NOT_SUPPORTED;
}
esp_err_t Phy::deletePhy(esp_eth_phy_t* phy) noexcept { return owner(phy).destroy(); }
esp_err_t Phy::autonego(esp_eth_phy_t* phy, eth_phy_autoneg_cmd_t command, bool* enabled) noexcept {
  if (!enabled) return ESP_ERR_INVALID_ARG;
  auto& self = owner(phy);
  // Installer reads declared initial AN policy before any hardware I/O.
  if (command == ESP_ETH_PHY_AUTONEGO_G_STAT && self._installing.load()) { *enabled = true; return ESP_OK; }
  if (!(command == ESP_ETH_PHY_AUTONEGO_RESTART ? self.enterStartCallback(StartEntry::RESTART) : self.enterControlCallback())) {
    return ESP_ERR_INVALID_STATE;
  }
  const auto settings = self._core.settingsSnapshot();
  if (command == ESP_ETH_PHY_AUTONEGO_G_STAT) {
    const bool valid = settings.desiredValid;
    if (valid) *enabled = settings.desired.mode == LinkMode::AUTONEGOTIATED;
    self.leave(); return valid ? ESP_OK : ESP_ERR_INVALID_STATE;
  }
  if (command == ESP_ETH_PHY_AUTONEGO_EN || command == ESP_ETH_PHY_AUTONEGO_DIS) {
    const bool requested = command == ESP_ETH_PHY_AUTONEGO_EN;
    const bool admitted = self.admittedControl(ControlEntry::AUTONEGO) &&
        requested == (settings.desired.mode == LinkMode::AUTONEGOTIATED);
    if (admitted) *enabled = requested; // Public SDK owns the passed cache storage.
    self.leave(); return admitted ? ESP_OK : ESP_ERR_INVALID_STATE;
  }
  if (command != ESP_ETH_PHY_AUTONEGO_RESTART) { self.leave(); return ESP_ERR_INVALID_ARG; }
  if (self._phase != Phase::STARTING || self._notifying || !self._sdkState.reconciled || !self.verifiedSettings()) {
    self.unexpectedStart(false); self.leave(); return ESP_ERR_INVALID_STATE;
  }
  const auto status = self._core.restartAutoNegotiation(self.callbackBudget(3));
  if (status.ok()) { *enabled = true; self._startEntry = StartEntry::LINK; }
  else self.recordCore(status);
  self.refreshAgreement(); self.shareQuiet(); self.leave(); return nativeError(status);
}

bool Phy::verifiedSettings() const noexcept {
  const auto settings = _core.settingsSnapshot();
  const auto operation = _core.operationSnapshot();
  const auto transport = _core.transportSnapshot();
  return _core.isInitialized() && _core.capabilities().valid && settings.desiredValid &&
      settings.trust == ConfigurationTrust::VERIFIED && operation.state == OperationState::IDLE &&
      !operation.cleanupOwed && !operation.temporaryMayBeActive && !transport.clockInvalid &&
      !transport.stopped && !_config.bus->quarantined.load() &&
      _config.bus->admission.load() != BusAdmission::FAILED_CLOSED;
}
void Phy::refreshAgreement() noexcept {
  if (!verifiedSettings()) _sdkState.reconciled = false;
}
bool Phy::stoppedAdmission() const noexcept {
  return _installed && _phase == Phase::STOPPED && !_notifying && !_frameworkStopOwed;
}
bool Phy::admittedControl(ControlEntry entry) const noexcept {
  return _phase == Phase::CONTROL && _controlEntry == entry && _controlTask == xTaskGetCurrentTaskHandle() &&
      !_notifying && !_frameworkStopOwed && timeNow() < _controlDeadline && verifiedSettings();
}
void Phy::unexpectedStart(bool startPosted) noexcept {
  // With cached AN disabled SDK posts START before get_link. With AN enabled
  // its FSM already changed before RESTART. Both need a real stop, not only a fence.
  if (_installed && (_phase == Phase::STOPPED || _phase == Phase::CONTROL)) {
    _frameworkStopOwed = true; _startPosted = startPosted; _stopPosted = false;
    _macStopAccepted = false;
    _sdkState.reconciled = false; _record.stage = LifecycleStage::START;
    record(ESP_ERR_INVALID_STATE);
    if (_phase == Phase::STOPPED) _phase = Phase::STOPPING;
  }
}
esp_err_t Phy::acknowledgeSpeed(esp_eth_phy_t* phy, eth_speed_t speed) noexcept {
  auto& self = owner(phy);
  if (speed != ETH_SPEED_10M && speed != ETH_SPEED_100M) return ESP_ERR_INVALID_ARG;
  if (!self.enterControlCallback()) return ESP_ERR_INVALID_STATE;
  const auto settings = self._core.settingsSnapshot().desired;
  const bool accepted = self.admittedControl(ControlEntry::SPEED) && settings.mode == LinkMode::FORCED &&
      speed == (settings.forced.speed == LinkSpeed::MBPS_100 ? ETH_SPEED_100M : ETH_SPEED_10M);
  self.leave(); return accepted ? ESP_OK : ESP_ERR_INVALID_STATE;
}
esp_err_t Phy::acknowledgeDuplex(esp_eth_phy_t* phy, eth_duplex_t duplex) noexcept {
  auto& self = owner(phy);
  if (duplex != ETH_DUPLEX_HALF && duplex != ETH_DUPLEX_FULL) return ESP_ERR_INVALID_ARG;
  if (!self.enterControlCallback()) return ESP_ERR_INVALID_STATE;
  const auto settings = self._core.settingsSnapshot().desired;
  const bool accepted = self.admittedControl(ControlEntry::DUPLEX) && settings.mode == LinkMode::FORCED &&
      duplex == (settings.forced.duplex == Duplex::FULL ? ETH_DUPLEX_FULL : ETH_DUPLEX_HALF);
  self.leave(); return accepted ? ESP_OK : ESP_ERR_INVALID_STATE;
}
esp_err_t Phy::acknowledgePause(esp_eth_phy_t* phy, uint32_t ability) noexcept {
  auto& self = owner(phy);
  if (!self.enterControlCallback()) { self._unexpectedFlowEffect.store(true); return ESP_ERR_INVALID_STATE; }
  const auto settings = self._core.settingsSnapshot().desired;
  const uint32_t desired = (settings.advertisement.symmetricPause ? 1U : 0U) |
      (settings.advertisement.asymmetricPause ? 2U : 0U);
  const bool accepted = ability <= 3U && self.admittedControl(ControlEntry::PAUSE) &&
      settings.mode == LinkMode::AUTONEGOTIATED && ability == desired;
  if (!accepted) {
    // Arbitrary S_FLOW_CTRL can already have changed MAC policy before this
    // callback rejects. A direct callback cannot prove that it did not.
    self._flow.reconciled = false; self._flow.macEffectPossible = true;
    self._flow.phyError = ESP_ERR_INVALID_STATE;
    self._unexpectedFlowEffect.store(true);
  }
  self.leave(); return accepted ? ESP_OK : ESP_ERR_INVALID_STATE;
}

esp_err_t Phy::publish(PublishedTuple tuple, uint64_t deadlineUs, bool stopping) noexcept {
  const bool ownerCall = stopping || _startTask.load() == xTaskGetCurrentTaskHandle();
  auto gateFailure = TimeoutDetail::DEADLINE;
  if (!(ownerCall ? enterUntil(deadlineUs, &gateFailure) : enter())) {
    if (stopping) {
      rememberCleanup(gateFailure); _cleanupPublicationReported = true;
      return ESP_ERR_TIMEOUT;
    }
    return ESP_ERR_INVALID_STATE;
  }
  return publishAdmitted(tuple, deadlineUs, stopping);
}
esp_err_t Phy::publishAdmitted(PublishedTuple tuple, uint64_t deadlineUs, bool stopping) noexcept {
  if (!_mediator || _notifying || _record.publicationUncertain ||
      (stopping ? _phase != Phase::STOPPING : (_phase != Phase::RUNNING && _phase != Phase::STARTING))) {
    leave(); return ESP_ERR_INVALID_STATE;
  }
  if (same(tuple, _record.publishedTuple)) { leave(); return ESP_OK; }
  PublicationResult receipt{};
  receipt.tuple = _record.publishedTuple; receipt.eventLost = _record.eventLost;
  receipt.cleanup = _phase == Phase::STOPPING;
  const bool wasUp = receipt.tuple.up;
  if (tuple.up) {
    receipt.tuple.speedAccepted = false;
    receipt.tuple.duplexAccepted = false;
    receipt.tuple.pauseAccepted = false;
  }
  _notifying = true;
  _record.publicationUncertain = true; // In-flight evidence, replaced by the terminal receipt.
  auto* mediator = _mediator;
  leave();
  const auto late = [&](TimeoutDetail reason = TimeoutDetail::DEADLINE) {
    receipt.status = {Status::Error(Err::OPERATION_TIMEOUT, "Native publication allowance expired",
                                   static_cast<int32_t>(reason))};
    return ESP_ERR_TIMEOUT;
  };
  auto notify = [&](esp_eth_state_t state, uintptr_t value, bool link) {
    const auto before = timeNow();
    if (before >= deadlineUs) return late();
    if (_config.notificationUs >= deadlineUs - before) return late(TimeoutDetail::BOUND_ADMISSION);
    const auto error = mediator->on_state_changed(mediator, state, reinterpret_cast<void*>(value));
    const auto after = timeNow();
    if (link && (error == ESP_OK || _config.pinnedP4Mac)) {
      // On pinned P4 valid set_link always returns OK: the mediator error can
      // only be event delivery. Never replay this accepted MAC/cache transition.
      receipt.tuple.up = value == ETH_LINK_UP;
      receipt.eventLost = error != ESP_OK;
    } else if (error == ESP_OK) {
      if (state == ETH_STATE_SPEED) { receipt.tuple.speed = tuple.speed; receipt.tuple.speedAccepted = true; }
      if (state == ETH_STATE_DUPLEX) { receipt.tuple.duplex = tuple.duplex; receipt.tuple.duplexAccepted = true; }
      if (state == ETH_STATE_PAUSE) { receipt.tuple.peerPause = tuple.peerPause; receipt.tuple.pauseAccepted = true; }
    }
    receipt.uncertain = error != ESP_OK && !(link && _config.pinnedP4Mac);
    if (error != ESP_OK) { receipt.status = {{}, error}; return error; }
    return after < before || after >= deadlineUs || after - before > _config.notificationUs ? late() : ESP_OK;
  };
  esp_err_t error = ESP_OK;
  if (wasUp) error = notify(ETH_STATE_LINK, ETH_LINK_DOWN, true);
  if (error == ESP_OK && tuple.up) {
    error = notify(ETH_STATE_SPEED, tuple.speed == LinkSpeed::MBPS_100 ? ETH_SPEED_100M : ETH_SPEED_10M, false);
    if (error == ESP_OK) error = notify(ETH_STATE_DUPLEX, tuple.duplex == Duplex::FULL ? ETH_DUPLEX_FULL : ETH_DUPLEX_HALF, false);
    if (error == ESP_OK) error = notify(ETH_STATE_PAUSE, tuple.peerPause ? 1U : 0U, false);
    if (error == ESP_OK) error = notify(ETH_STATE_LINK, ETH_LINK_UP, true);
  }
  // The synchronous notification owner is the only writer until this release.
  // No post-callback try-lock can lose accepted effects or strand _notifying.
  _pendingPublication = receipt;
  if (receipt.cleanup) _cleanupPublicationReported = !receipt.status.ok();
  _publicationComplete.store(true);
  return error;
}
esp_err_t Phy::getLink(esp_eth_phy_t* phy) noexcept {
  auto& self = owner(phy);
  if (!self.enterStartCallback(StartEntry::LINK)) return ESP_ERR_INVALID_STATE;
  if (self._phase != Phase::STARTING && self._phase != Phase::RUNNING) {
    self.unexpectedStart(true); self.leave(); return ESP_ERR_INVALID_STATE;
  }
  if (self._phase == Phase::STARTING) self._startPosted = true; // SDK posts START before this callback.
  const auto budget = self.callbackBudget(16);
  const auto fail = [&](Status failure, bool trustLost = false) {
    self.recordCore(failure);
    self._record.lastObservationStatus = failure;
    if (self._phase == Phase::RUNNING) {
      if (self._record.consecutiveObservationMisses < std::numeric_limits<uint32_t>::max())
        ++self._record.consecutiveObservationMisses;
      if (self._record.consecutiveObservationMisses >= self._config.maxObservationMisses)
        self._record.observationDownOwed = true;
    }
    if (trustLost || self._phase == Phase::STARTING) self._record.stopRequired = true;
    const bool withdraw = self._record.publishedTuple.up && !self._notifying &&
        (self._record.stopRequired || self._record.observationDownOwed);
    self.leave();
    // Administrative DOWN does not rewrite retained physical link evidence.
    // If publication is itself uncertain, the journal requires application stop.
    if (withdraw) (void)self.publish({}, budget.deadlineUs);
    return nativeError(failure);
  };
  self.refreshAgreement();
  if (self._record.stopRequired)
    return fail(self._record.lastObservationStatus.ok() || self._record.lastObservationStatus.is(Err::RESULT_NOT_AVAILABLE)
        ? Status::Error(Err::NOT_INITIALIZED, "Owner stop required after running proof was lost")
        : self._record.lastObservationStatus, true);
  if (self._config.bus->admission.load() == BusAdmission::FAILED_CLOSED)
    return fail(Status::Error(Err::NOT_INITIALIZED, "Shared board readiness failed"), true);
  if (!self._sdkState.reconciled)
    return fail(Status::Error(Err::NOT_INITIALIZED, "Running SDK or PHY agreement lost"), true);
  // publicationUncertain is also the in-flight marker. Only a terminal unknown
  // effect loses trust; our synchronous publisher retains its own receipt.
  if (self._notifying) return fail(busy());
  if (self._record.publicationUncertain) return fail(busy(), true);
  if (self._record.observationDownOwed) {
    if (self._record.publishedTuple.up) {
      self.leave(); return self.publish({}, budget.deadlineUs);
    }
    self._record.observationDownOwed = false;
  }
  const uint64_t reserve = uint64_t{self._config.notificationUs} * 5U;
  const auto current = timeNow();
  if (current >= budget.deadlineUs || reserve >= budget.deadlineUs - current) {
    return fail(Status::Error(Err::OPERATION_TIMEOUT, "Link callback allowance exhausted",
        static_cast<int32_t>(current >= budget.deadlineUs ? TimeoutDetail::DEADLINE : TimeoutDetail::BOUND_ADMISSION)));
  }
  LinkSnapshot observation{};
  const auto status = self._core.readLinkStatus(observation, {budget.deadlineUs - reserve, budget.maxFrames});
  self.refreshAgreement();
  if (!status.ok()) return fail(status, !self._sdkState.reconciled);
  // A coherent physical sample may itself reveal reset or lost configuration.
  // Neither unresolved UP nor observed DOWN can restore the owner's agreement.
  if (!self._sdkState.reconciled)
    return fail(Status::Error(Err::NOT_INITIALIZED, "Running SDK or PHY agreement lost"), true);
  const bool awaitingNegotiation = self._config.waitForNegotiatedLink &&
      observation.negotiation == NegotiationState::IN_PROGRESS;
  if (observation.linkUp && !observation.resolvedModeValid && !awaitingNegotiation)
    return fail(Status::Error(Err::NOT_INITIALIZED, "Running link mode is unresolved"));
  self._record.lastObservationStatus = Status::Ok();
  self._record.consecutiveObservationMisses = 0;
  PublishedTuple tuple{};
  tuple.up = observation.linkUp && observation.resolvedModeValid;
  if (tuple.up) {
    tuple.speed = observation.speed; tuple.duplex = observation.duplex;
    // P4's Boolean enables RX and TX together; a one-way or invalid resolution
    // must never enable the other direction merely because the peer advertises it.
    tuple.peerPause = observation.pauseValid && observation.pauseRx && observation.pauseTx;
  }
  // Keep observation validation and publication admission under the same gate;
  // external invalidation must not succeed between them and then publish stale UP.
  return self.publishAdmitted(tuple, budget.deadlineUs);
}
esp_err_t Phy::setLink(esp_eth_phy_t* phy, eth_link_t link) noexcept {
  if (link != ETH_LINK_DOWN) return ESP_ERR_NOT_SUPPORTED;
  auto& self = owner(phy);
  if (self._phase.load() != Phase::STOPPING) return ESP_ERR_INVALID_STATE;
  return self.publish({}, self._lifecycleDeadline.load(), true);
}

void Phy::rememberCleanup(esp_err_t error) noexcept {
  if (error == ESP_OK) return;
  rememberCleanupReceipt(static_cast<int64_t>(error));
}
void Phy::rememberCleanup(TimeoutDetail reason) noexcept {
  rememberCleanupReceipt(timeoutReceipt(reason));
}
void Phy::rememberCleanupReceipt(int64_t receipt) noexcept {
  int64_t empty = std::numeric_limits<int64_t>::min();
  (void)_cleanupOutcome.compare_exchange_strong(empty, receipt);
  _latestCleanupOutcome.store(receipt);
}

esp_err_t Phy::start(uint64_t deadlineUs) noexcept {
  if (!enterUntil(deadlineUs)) return ESP_ERR_TIMEOUT;
  const auto settings = _core.settingsSnapshot();
  refreshAgreement();
  if (!stoppedAdmission() || _record.publicationUncertain || !_sdkState.reconciled ||
      (_flow.macEffectPossible && !_flow.reconciled) || !verifiedSettings() ||
      (settings.desired.mode == LinkMode::FORCED && settings.desired.extendedFullDuplex &&
       settings.desired.forced.duplex == Duplex::HALF)) {
    leave(); return ESP_ERR_INVALID_STATE;
  }
  if (timeNow() >= deadlineUs) { leave(); return ESP_ERR_TIMEOUT; }
  if (_record.sequence < std::numeric_limits<uint64_t>::max()) ++_record.sequence;
  _record.firstError = {}; _record.cleanupError = {}; _record.lastCleanupError = {};
  _record.stopRequired = false;
  _record.consecutiveObservationMisses = 0; _record.observationDownOwed = false;
  _record.lastObservationStatus = Status::Error(Err::RESULT_NOT_AVAILABLE, "No native observation");
  _phase = Phase::STARTING; _lifecycleDeadline = deadlineUs; _record.stage = LifecycleStage::START;
  _startPosted = false; _stopPosted = false; _macStopAccepted = false; _preStopDownAttempted = false;
  _frameworkStopOwed = true; // SDK changes its FSM before calling the PHY or posting START.
  _startEntry = _sdkState.cachedAutoNegotiation ? StartEntry::RESTART : StartEntry::LINK;
  _startTask.store(xTaskGetCurrentTaskHandle());
  leave();
  const auto sdkError = esp_eth_start(_ethernet);
  _startTask.store(nullptr);
  const auto completed = timeNow();
  const bool late = sdkError == ESP_OK && completed >= deadlineUs;
  _startOutcome.store(late ? OWNER_DEADLINE : static_cast<int64_t>(sdkError));
  if (sdkError == ESP_OK && !late) return ESP_OK;
  uint64_t cleanupDeadline;
  if (!addTime(completed, _config.cleanupBudgetUs, cleanupDeadline))
    cleanupDeadline = std::numeric_limits<uint64_t>::max();
  (void)stop(cleanupDeadline);
  return late ? ESP_ERR_TIMEOUT : sdkError;
}

esp_err_t Phy::stop(uint64_t deadlineUs) noexcept {
  auto gateFailure = TimeoutDetail::DEADLINE;
  const auto timeout = [&](TimeoutDetail reason = TimeoutDetail::DEADLINE) {
    rememberCleanup(reason);
    return ESP_ERR_TIMEOUT;
  };
  // A new stop has no effects until admitted. A failed start or a prior
  // admitted stop already owes cleanup, including when this retry cannot enter.
  if (!enterUntil(deadlineUs, &gateFailure)) {
    const auto start = _startOutcome.load();
    const bool cleanupOwed = _phase.load() == Phase::STOPPING ||
        (start != std::numeric_limits<int64_t>::min() && start != ESP_OK);
    return cleanupOwed ? timeout(gateFailure) : ESP_ERR_TIMEOUT;
  }
  if (!_installed || _phase == Phase::CONTROL) { leave(); return ESP_ERR_INVALID_STATE; }
  if (_phase == Phase::MAINTENANCE || (_phase == Phase::STOPPED && !_frameworkStopOwed)) {
    leave(); return ESP_OK;
  }
  _phase = Phase::STOPPING; _record.stage = LifecycleStage::STOP; _lifecycleDeadline = deadlineUs;
  _record.stopRequired = true;
  // Once admitted, close new live observations while an existing publisher
  // finishes outside the gate. Later owner calls resume the same obligations.
  for (uint32_t poll = 0; _notifying; ++poll) {
    leave();
    const auto current = timeNow();
    if (current >= deadlineUs) return timeout();
    if (poll + 1 == _config.maxFencePolls) return timeout(TimeoutDetail::POLL_BUDGET);
    if (!ticksWithin(deadlineUs - current)) return timeout(TimeoutDetail::BOUND_ADMISSION);
    vTaskDelay(1);
    if (!enterUntil(deadlineUs, &gateFailure)) return timeout(gateFailure);
  }
  if (timeNow() >= deadlineUs) { leave(); return timeout(); }
  esp_err_t downError = ESP_OK;
  if (_frameworkStopOwed && !_preStopDownAttempted && _record.eventLost && !_record.publishedTuple.up) {
    // Try to restore normal event order once. If this consumes the entire
    // allowance, the next owner call proceeds to framework stop immediately.
    _preStopDownAttempted = true;
    leave();
    _cleanupPublicationReported = false;
    downError = retryLinkEvent(deadlineUs);
    if (!_cleanupPublicationReported) rememberCleanup(downError);
    if (!enterUntil(deadlineUs, &gateFailure)) return timeout(gateFailure);
    if (_phase != Phase::STOPPING) { leave(); return ESP_ERR_INVALID_STATE; }
  }
  if (timeNow() >= deadlineUs) { leave(); return timeout(); }
  // Installed deinit proves the SDK FSM has already stopped and its timer
  // was deleted. Only the adapter's independent fence may be used from here.
  const bool callFramework = _frameworkStopOwed && !_uninstallSeen.load();
  _frameworkStopOwed = false;
  _preStopDownAttempted = false;
  _record.stage = LifecycleStage::FENCE;
  leave();
  _cleanupPublicationReported = false;
  const auto initial = callFramework ? esp_eth_stop(_ethernet) : ESP_OK;
  if (callFramework && initial == ESP_OK) _stopPosted = true;
  if (!_cleanupPublicationReported) rememberCleanup(initial);
  return finishStop(deadlineUs, downError);
}

esp_err_t Phy::finishStop(uint64_t deadlineUs, esp_err_t downError) noexcept {
  auto gateFailure = TimeoutDetail::DEADLINE;
  const auto timeout = [&](TimeoutDetail reason = TimeoutDetail::DEADLINE) {
    rememberCleanup(reason);
    return ESP_ERR_TIMEOUT;
  };
  if (timeNow() >= deadlineUs) return timeout();
  if (!_fenceArmed) {
    const auto armed = esp_timer_start_once(_fence, 0);
    if (armed != ESP_OK) { rememberCleanup(armed); return armed; }
    _fenceArmed = true;
  }
  uint32_t polls = 0;
  while (esp_timer_is_active(_fence)) {
    const auto current = timeNow();
    if (current >= deadlineUs) return timeout();
    if (++polls >= _config.maxFencePolls) return timeout(TimeoutDetail::POLL_BUDGET);
    if (!ticksWithin(deadlineUs - current)) return timeout(TimeoutDetail::BOUND_ADMISSION);
    vTaskDelay(1);
  }
  if (!enterUntil(deadlineUs, &gateFailure)) return timeout(gateFailure);
  const bool needDown = _record.publishedTuple.up;
  const bool uncertain = _record.publicationUncertain;
  leave();
  esp_err_t events = uncertain ? ESP_ERR_INVALID_STATE : downError;
  _cleanupPublicationReported = false;
  if (!uncertain && needDown) events = publish({}, deadlineUs, true);
  if (needDown || uncertain) {
    if (!_cleanupPublicationReported) rememberCleanup(events);
  }
  if (!enterUntil(deadlineUs, &gateFailure)) return timeout(gateFailure);
  const bool lostDown = !_record.publicationUncertain && !_record.publishedTuple.up && _record.eventLost;
  leave();
  if (lostDown && downError == ESP_OK) {
    _cleanupPublicationReported = false;
    events = retryLinkEvent(deadlineUs);
    if (!_cleanupPublicationReported) rememberCleanup(events);
  }
  // An accepted link/cache DOWN can have a missing DISCONNECTED. Keep that
  // error and repair independently while still delivering STOP to netif glue.
  if ((events == ESP_OK || lostDown) && _startPosted && !_stopPosted) {
    const auto before = timeNow();
    if (before >= deadlineUs) return timeout();
    const auto stopEvent = esp_event_post(ETH_EVENT, ETHERNET_EVENT_STOP, &_ethernet, sizeof(_ethernet), 0);
    rememberCleanup(stopEvent);
    if (stopEvent == ESP_OK) {
      _stopPosted = true;
      if (timeNow() >= deadlineUs) return timeout();
    } else if (events == ESP_OK) events = stopEvent;
  }
  if (!enterUntil(deadlineUs, &gateFailure)) return timeout(gateFailure);
  _record.stage = LifecycleStage::MAC_STOP;
  leave();
  esp_err_t macError = ESP_OK;
  for (uint32_t attempt = 0; !_macStopAccepted && attempt < _config.maxStopAttempts; ++attempt) {
    if (timeNow() >= deadlineUs) return timeout();
    macError = _config.mac->stop(_config.mac);
    _macStopAccepted = macError == ESP_OK;
    rememberCleanup(macError);
    if (macError == ESP_OK && timeNow() >= deadlineUs) return timeout();
  }
  if (events != ESP_OK) return events;
  if (macError != ESP_OK) return macError;
  if (!enterUntil(deadlineUs, &gateFailure)) return timeout(gateFailure);
  if (_phase != Phase::STOPPING || _record.publicationUncertain || _record.publishedTuple.up ||
      _record.eventLost || (_startPosted && !_stopPosted)) {
    record(ESP_ERR_INVALID_STATE, true); leave(); return ESP_ERR_INVALID_STATE;
  }
  if (timeNow() >= deadlineUs) { leave(); return timeout(); }
  _fenceArmed = false; _record.stopRequired = false;
  if (_uninstallSeen.load()) {
    _phase = Phase::MAINTENANCE; _record.stage = LifecycleStage::RETAINED;
  } else _phase = Phase::STOPPED;
  leave();
  return ESP_OK;
}

esp_err_t Phy::retire(uint64_t deadlineUs) noexcept {
  const auto stopped = stop(deadlineUs);
  if (stopped != ESP_OK) return stopped;
  // Stop already completed. Failure to acquire the terminal admission gate
  // cannot create a cleanup error or undo those observed effects.
  if (!enterUntil(deadlineUs)) return ESP_ERR_TIMEOUT;
  if (_phase == Phase::MAINTENANCE) { leave(); return ESP_ERR_NOT_SUPPORTED; }
  if (!_installed || _phase != Phase::STOPPED || _frameworkStopOwed || _notifying ||
      _record.publicationUncertain || _record.publishedTuple.up || _record.eventLost) {
    leave(); return ESP_ERR_INVALID_STATE;
  }
  _phase = Phase::MAINTENANCE; _record.stage = LifecycleStage::RETAINED;
  leave(); return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t Phy::destroy() noexcept {
  if (!enter()) return ESP_ERR_INVALID_STATE;
  if (_installed || _phase != Phase::STOPPED || _notifying || esp_timer_is_active(_fence)) {
    leave(); return ESP_ERR_INVALID_STATE;
  }
  const auto ended = _core.end();
  if (!ended.ok()) { leave(); return ESP_ERR_INVALID_STATE; }
  auto error = esp_timer_delete(_fence);
  if (error != ESP_OK) { leave(); return error; }
  leave(); vSemaphoreDelete(_gate); delete this; return ESP_OK;
}
Status Phy::lifecycleSnapshot(LifecycleRecord& output) noexcept {
  if (!enter()) return busy();
  output = _record; output.phase = _phase.load(); leave(); return Status::Ok();
}
Status Phy::linkSnapshot(LinkSnapshot& output) noexcept {
  if (!enter()) return busy();
  output = _core.linkSnapshot(); leave(); return Status::Ok();
}
Status Phy::readLinkStatus(LinkSnapshot& output, const IoBudget& budget) noexcept {
  return stoppedRaw(nullptr, [&](bool*) { return _core.readLinkStatus(output, budget); });
}
Status Phy::withAdmissionEvidence(void* context, void (*decide)(void*, const LinkSnapshot&,
    const SettingsSnapshot&, const HealthSnapshot&, bool, uint64_t) noexcept) noexcept {
  if (!decide) return Status::Error(Err::INVALID_PARAM, "Missing admission callback");
  if (!enter()) return busy();
  uint64_t upper;
  if (!addTime(timeNow(), _config.core.transport.bounds.clockLagUs, upper)) {
    leave();
    return Status::Error(Err::INVALID_CONFIG, "Admission clock overflow",
                         static_cast<int32_t>(ConfigDetail::CLOCK_CONTRACT));
  }
  const auto link = _core.linkSnapshot();
  const auto& published = _record.publishedTuple;
  const bool ready = _phase == Phase::RUNNING && !_notifying && !_record.stopRequired &&
      !_record.observationDownOwed && !_record.eventLost && !_record.publicationUncertain &&
      _record.lastObservationStatus.ok() && _sdkState.reconciled && verifiedSettings() &&
      _config.bus->admission.load() == BusAdmission::AVAILABLE &&
      (!_flow.macEffectPossible || _flow.reconciled) && published.up && link.linkUp &&
      link.valid && !link.stale && link.resolvedModeValid &&
      published.speed == link.speed && published.duplex == link.duplex;
  decide(context, link, _core.settingsSnapshot(), _core.healthSnapshot(), ready, upper);
  leave();
  return Status::Ok();
}
Status Phy::healthSnapshot(HealthSnapshot& output) noexcept {
  if (!enter()) return busy();
  output = _core.healthSnapshot(); leave(); return Status::Ok();
}
void Phy::shareQuiet() noexcept {
  const auto progress = _core.operationSnapshot();
  if (progress.busQuietValid) {
    extendQuiet(*_config.bus, progress.busQuietUntilUs);
  }
  // A shared clock fault can leave an attempted HARD reset without a usable
  // silence anchor. Keep peers closed even after this owner releases its slot.
  if (_ownsSilence && _core.transportSnapshot().clockInvalid)
    _config.bus->quarantined.store(true);
}
Status Phy::reserveOperationSilence() noexcept {
  auto& bus = *_config.bus;
  // Claim silence before draining: another PHY's board reset cannot replace
  // this reservation between polls. Only this core may issue its own batches.
  auto admission = BusAdmission::AVAILABLE;
  if (bus.quarantined.load() || !bus.admission.compare_exchange_strong(admission, BusAdmission::INITIALIZING)) {
    return busDenied(bus.quarantined.load());
  }
  if (xSemaphoreTake(bus.mutex, 0) != pdTRUE) {
    bus.admission.store(BusAdmission::AVAILABLE); return busy();
  }
  if (bus.quarantined.load()) {
    const bool released = xSemaphoreGive(bus.mutex) == pdTRUE;
    bus.admission.store(released ? BusAdmission::AVAILABLE : BusAdmission::FAILED_CLOSED);
    if (!released) record(ESP_FAIL);
    return busDenied(true);
  }
  _ownsSilence = true;
  if (xSemaphoreGive(bus.mutex) != pdTRUE) {
    bus.quarantined.store(true); bus.admission.store(BusAdmission::FAILED_CLOSED);
    _ownsSilence = false; record(ESP_FAIL); return busDenied(true);
  }
  return Status::Ok();
}

void Phy::releaseOperationSilence() noexcept {
  if (_ownsSilence) {
    _ownsSilence = false;
    _config.bus->admission.store(BusAdmission::AVAILABLE);
  }
}

Status Phy::enterFeature(bool silence) noexcept {
  if (!enter()) return busy();
  if (!stoppedAdmission()) { leave(); return busy(); }
  const auto slot = _core.operationSnapshot().state;
  if (slot != OperationState::IDLE) {
    const auto detail = slot == OperationState::RUNNING ? BusyDetail::OPERATION_ACTIVE : BusyDetail::RESULT_PENDING;
    leave();
    return Status::Error(Err::BUSY, "Native feature operation slot occupied", static_cast<int32_t>(detail));
  }
  const auto admitted = silence ? reserveOperationSilence() : Status::Ok();
  if (!admitted.ok()) leave();
  return admitted;
}

Status Phy::enterFeatureSnapshot() noexcept {
  return enter() ? Status::Ok() : busy();
}

void Phy::finishFeatureStart(Status status, bool observation) noexcept {
  if (status.inProgress()) {
    _phase = Phase::CONTROL;
    _record.stage = LifecycleStage::CONFIGURE;
    if (!observation) _sdkState.reconciled = false;
  } else {
    releaseOperationSilence();
  }
  leave();
}

Status Phy::startInitialize(const InitializeOptions& options, const OperationBudget& budget, OperationToken& token) noexcept {
  const auto admitted = enterFeature(true);
  if (!admitted.ok()) return admitted;
  const auto status = _core.startInitialize(options, budget, token);
  if (status.inProgress()) {
    _phase = Phase::CONTROL; _record.stage = LifecycleStage::INITIALIZE; _sdkState.reconciled = false;
  } else { releaseOperationSilence(); }
  leave(); return status;
}
Status Phy::startConfigure(const LinkSettings& settings, const OperationBudget& budget, OperationToken& token) noexcept {
  return startConfigure(settings, ConfigureOptions{}, budget, token);
}
Status Phy::startConfigure(const LinkSettings& settings, const ConfigureOptions& options,
                           const OperationBudget& budget, OperationToken& token) noexcept {
  const auto valid = validateLinkSettings(settings);
  if (!valid.ok()) return valid;
  if (settings.mode == LinkMode::FORCED && settings.extendedFullDuplex && settings.forced.duplex == Duplex::HALF)
    return Status::Error(Err::INVALID_CONFIG, "Extended forced half duplex cannot produce qualified native UP", static_cast<int32_t>(ConfigDetail::LINK_COMBINATION));
  if (!enter()) return busy();
  if (!stoppedAdmission()) { leave(); return busy(); }
  const auto status = _core.startConfigure(settings, options, budget, token);
  if (status.inProgress()) { _phase = Phase::CONTROL; _record.stage = LifecycleStage::CONFIGURE; _sdkState.reconciled = false; }
  leave(); return status;
}
Status Phy::startVerify(const OperationBudget& budget, OperationToken& token) noexcept {
  if (!enter()) return busy();
  if (!stoppedAdmission()) { leave(); return busy(); }
  const auto status = _core.startVerify(budget, token);
  if (status.inProgress()) { _phase = Phase::CONTROL; _record.stage = LifecycleStage::VERIFY; }
  leave(); return status;
}
Status Phy::pollOperation(OperationToken token, uint32_t maxFrames) noexcept {
  if (!enter()) return busy();
  if (_phase != Phase::CONTROL || !_installed || _notifying || _controlEntry != ControlEntry::NONE || _frameworkStopOwed) { leave(); return busy(); }
  const auto status = _core.pollOperation(token, maxFrames); shareQuiet();
  const auto snapshot = _core.operationSnapshot();
  if (snapshot.token.value == token.value && snapshot.state == OperationState::RESULT_PENDING) recordCore(snapshot.status);
  leave(); return status;
}
Status Phy::takeResult(OperationToken token, OperationResult& output) noexcept {
  if (!enter()) return busy();
  const auto status = _core.takeResult(token, output);
  if (status.ok()) recordCore(output.status);
  if (status.ok()) {
    if (_phase == Phase::CONTROL) _phase = _frameworkStopOwed ? Phase::STOPPING : Phase::STOPPED;
    if (_ownsSilence) { shareQuiet(); releaseOperationSilence(); }
    refreshAgreement();
  }
  leave(); return status;
}
Status Phy::cancel(OperationToken token) noexcept {
  if (!enter()) return busy();
  const auto status = _core.cancel(token);
  const auto snapshot = _core.operationSnapshot();
  if (snapshot.token.value == token.value && snapshot.state == OperationState::RESULT_PENDING) recordCore(snapshot.status);
  leave(); return status;
}
Status Phy::abandonCleanup() noexcept {
  if (!enter()) return busy();
  if (!stoppedAdmission()) { leave(); return busy(); }
  const auto status = _core.abandonCleanup(); refreshAgreement(); leave(); return status;
}
Status Phy::operationSnapshot(OperationSnapshot& output) noexcept {
  if (!enter()) return busy();
  output = _core.operationSnapshot(); leave(); return Status::Ok();
}
Status Phy::identitySnapshot(DeviceIdentity& output) noexcept {
  if (!enter()) return busy();
  output = _core.identitySnapshot(); leave(); return Status::Ok();
}
Status Phy::capabilitiesSnapshot(Capabilities& output) noexcept {
  if (!enter()) return busy();
  output = _core.capabilities(); leave(); return Status::Ok();
}
template<class Call>
Status Phy::stoppedRaw(RawBatchReceipt* receipt, Call call) noexcept {
  if (receipt) receipt->valid = false;
  if (!enter()) return busy();
  if (!stoppedAdmission()) { leave(); return busy(); }
  bool invoked = false;
  const auto status = call(receipt ? &invoked : nullptr);
  if (receipt && invoked) {
    const auto transport = _core.transportSnapshot();
    *receipt = {transport.lastBatch, true, transport.violation, transport.lastReleaseStatus};
  }
  refreshAgreement();
  leave();
  return status;
}
Status Phy::readIdentity(DeviceIdentity& output, const IoBudget& budget, RawBatchReceipt* receipt) noexcept {
  return stoppedRaw(receipt, [&](bool* invoked) { return _core.readIdentity(output, budget, invoked); });
}
Status Phy::readClause22(uint32_t address, uint16_t& output, const IoBudget& budget,
                        RawBatchReceipt* receipt) noexcept {
  return stoppedRaw(receipt, [&](bool* invoked) { return _core.readClause22(address, output, budget, invoked); });
}
Status Phy::readMmd(uint32_t devad, uint32_t address, uint16_t& output, const IoBudget& budget,
                    RawBatchReceipt* receipt) noexcept {
  return stoppedRaw(receipt, [&](bool* invoked) { return _core.readMmd(devad, address, output, budget, invoked); });
}
Status Phy::writeClause22(uint32_t address, uint32_t value, const IoBudget& budget,
                          RawBatchReceipt* receipt) noexcept {
  return stoppedRaw(receipt, [&](bool* invoked) { return _core.writeClause22(address, value, budget, invoked); });
}
Status Phy::writeMmd(uint32_t devad, uint32_t address, uint32_t value, const IoBudget& budget,
                     RawBatchReceipt* receipt) noexcept {
  return stoppedRaw(receipt, [&](bool* invoked) { return _core.writeMmd(devad, address, value, budget, invoked); });
}
Status Phy::modifyMmd(uint32_t devad, uint32_t address, uint32_t mask, uint32_t value,
                      const IoBudget& budget, RawBatchReceipt* receipt) noexcept {
  return stoppedRaw(receipt, [&](bool* invoked) { return _core.modifyMmd(devad, address, mask, value, budget, invoked); });
}
Status Phy::modifyBasicControl(uint32_t mask, uint32_t value, const IoBudget& budget,
                              RawBatchReceipt* receipt) noexcept {
  return stoppedRaw(receipt, [&](bool* invoked) { return _core.modifyBasicControl(mask, value, budget, invoked); });
}
Status Phy::acquireEvidence(uint32_t mask, EvidenceSnapshot& output, const IoBudget& budget) noexcept {
  if (!enter()) return busy();
  if (!stoppedAdmission()) { leave(); return busy(); }
  const auto status = _core.acquireEvidence(mask, output, budget); refreshAgreement(); leave(); return status;
}
Status Phy::collectEvents(CounterSnapshot& output, const IoBudget& budget, RawBatchReceipt* receipt) noexcept {
  return stoppedRaw(receipt, [&](bool* invoked) { return _core.collectEvents(output, budget, invoked); });
}
Status Phy::counterSnapshot(CounterSnapshot& output, EvidenceSnapshot* evidence) noexcept {
  if (!enter()) return busy();
  output = _core.counterSnapshot();
  if (evidence) *evidence = _core.evidenceSnapshot();
  leave(); return Status::Ok();
}
Status Phy::invalidateDeviceState(InvalidationReason reason) noexcept {
  if (!enter()) return busy();
  if (!_installed || (_phase != Phase::STOPPED && _phase != Phase::RUNNING) || _notifying) { leave(); return busy(); }
  const auto status = _core.invalidateDeviceState(reason); refreshAgreement();
  if (status.ok() && _phase == Phase::RUNNING) _record.stopRequired = true;
  leave(); return status;
}
Status Phy::sdkStateSnapshot(SdkStateSnapshot& output) noexcept {
  if (!enter()) return busy();
  output = _sdkState; leave(); return Status::Ok();
}
Status Phy::flowControlSnapshot(FlowControlRecord& output) noexcept {
  if (!enter()) return busy();
  output = _flow; leave(); return Status::Ok();
}
Status Phy::settingsSnapshot(SettingsSnapshot& output) noexcept {
  if (!enter()) return busy();
  output = _core.settingsSnapshot(); leave(); return Status::Ok();
}
Status Phy::transportSnapshot(TransportSnapshot& output) noexcept {
  if (!enter()) return busy();
  output = _core.transportSnapshot(); leave(); return Status::Ok();
}
Status Phy::evidenceSnapshot(EvidenceSnapshot& output) noexcept {
  if (!enter()) return busy();
  output = _core.evidenceSnapshot(); leave(); return Status::Ok();
}
Status Phy::discover(uint32_t addressMask, DiscoverySnapshot& output, const IoBudget& budget,
                    DiscoverySnapshot* fresh) noexcept {
  if (!addressMask) return Status::Error(Err::INVALID_PARAM, "Empty discovery mask", static_cast<int32_t>(ParamDetail::ACCESS_POLICY));
  if (!enter()) return busy();
  const auto operation = _core.operationSnapshot();
  if (!stoppedAdmission() || operation.cleanupOwed ||
      operation.temporaryMayBeActive || operation.state != OperationState::IDLE) { leave(); return busy(); }
  if (_core.transportSnapshot().clockInvalid) {
    leave(); return Status::Error(Err::INVALID_CONFIG, "Core clock contract is latched", static_cast<int32_t>(ConfigDetail::CLOCK_CONTRACT));
  }
  uint32_t count = 0;
  for (uint32_t mask = addressMask; mask; mask >>= 1U) count += mask & 1U;
  const auto before = timeNow();
  const auto& bounds = _config.core.transport.bounds;
  const uint64_t reserve = uint64_t{count} * (uint64_t{bounds.fixedUs} + uint64_t{2} * bounds.frameUs) + bounds.clockLagUs;
  if (budget.maxFrames < count * 2U || before >= budget.deadlineUs || reserve >= budget.deadlineUs - before) {
    const auto detail = budget.maxFrames < count * 2U ? TimeoutDetail::FRAME_BUDGET :
        before >= budget.deadlineUs ? TimeoutDetail::DEADLINE : TimeoutDetail::BOUND_ADMISSION;
    leave(); return Status::Error(Err::OPERATION_TIMEOUT, "Insufficient whole discovery allowance", static_cast<int32_t>(detail));
  }
  _discovery = {}; _discovery.requestedMask = addressMask; _discovery.status = Status::Ok();
  uint64_t previous = before;
  for (uint8_t address = 0; address < 32U; ++address) {
    if ((addressMask & (uint32_t{1} << address)) == 0U) continue;
    uint16_t id1 = 0, id2 = 0;
    bool invoked = false;
    auto status = _core.readIdentityAt(address, id1, id2,
        {budget.deadlineUs, budget.maxFrames - _discovery.framesAttempted}, &invoked);
    _discovery.lastBatchValid = invoked;
    if (invoked) {
      const auto transport = _core.transportSnapshot();
      _discovery.lastBatch = transport.lastBatch;
      _discovery.lastBatchViolation = transport.violation;
    } else {
      _discovery.lastBatch = {};
      _discovery.lastBatchViolation = TransportViolation::NONE;
    }
    const auto& report = _discovery.lastBatch;
    auto& entry = _discovery.entries[address];
    for (uint8_t index = 0; index < 2; ++index) {
      const auto& frame = report.frames[index];
      if (frame.attempted) ++_discovery.framesAttempted;
      if (_discovery.lastBatchViolation == TransportViolation::NONE && frame.completed && frame.valueValid) {
        if (index == 0) { entry.id1 = frame.value; entry.id1Valid = true; }
        else { entry.id2 = frame.value; entry.id2Valid = true; }
      }
    }
    const auto after = timeNow();
    if (after < previous) {
      _config.bus->quarantined.store(true);
      if (status.ok()) status = Status::Error(Err::INVALID_CONFIG, "Discovery clock regressed", static_cast<int32_t>(ConfigDetail::CLOCK_CONTRACT));
    } else if (status.ok() && (after >= budget.deadlineUs || bounds.clockLagUs >= budget.deadlineUs - after)) {
      status = Status::Error(Err::OPERATION_TIMEOUT, "Discovery completed after allowance",
          static_cast<int32_t>(after >= budget.deadlineUs ? TimeoutDetail::DEADLINE : TimeoutDetail::BOUND_ADMISSION));
    }
    if (!status.ok()) { _discovery.status = status; break; }
    previous = after;
  }
  const auto status = _discovery.status;
  if (status.ok()) output = _discovery;
  if (fresh && fresh != &output) *fresh = _discovery;
  leave(); return status;
}
Status Phy::discoverySnapshot(DiscoverySnapshot& output) noexcept {
  if (!enter()) return busy();
  output = _discovery; leave(); return Status::Ok();
}
esp_err_t Phy::retryLinkEvent(uint64_t deadlineUs) noexcept {
  auto gateFailure = TimeoutDetail::DEADLINE;
  if (!enterUntil(deadlineUs, &gateFailure)) {
    if (_phase.load() == Phase::STOPPING) {
      rememberCleanup(gateFailure); _cleanupPublicationReported = true;
      return ESP_ERR_TIMEOUT;
    }
    return gateFailure == TimeoutDetail::DEADLINE ? ESP_ERR_TIMEOUT : ESP_ERR_INVALID_STATE;
  }
  if (!_installed || !_record.eventLost || _record.publicationUncertain || _notifying) { leave(); return ESP_ERR_INVALID_STATE; }
  if (_phase != Phase::RUNNING && !((_phase == Phase::STOPPED || _phase == Phase::STOPPING) && !_record.publishedTuple.up)) {
    leave(); return ESP_ERR_INVALID_STATE;
  }
  PublicationResult receipt{};
  receipt.tuple = _record.publishedTuple; receipt.eventLost = true;
  receipt.cleanup = _phase == Phase::STOPPING;
  const auto id = receipt.tuple.up ? ETHERNET_EVENT_CONNECTED : ETHERNET_EVENT_DISCONNECTED;
  _notifying = true; leave();
  const auto current = timeNow();
  const auto error = current >= deadlineUs ? ESP_ERR_TIMEOUT :
    esp_event_post(ETH_EVENT, id, &_ethernet, sizeof(_ethernet), receipt.cleanup ? 0 : ticksWithin(deadlineUs - current));
  const auto after = timeNow();
  if (error == ESP_OK) receipt.eventLost = false;
  const bool late = current >= deadlineUs || (error == ESP_OK && (after < current || after >= deadlineUs));
  receipt.status = late ? NativeStatus{Status::Error(Err::OPERATION_TIMEOUT, "Native event allowance expired", static_cast<int32_t>(TimeoutDetail::DEADLINE))} : NativeStatus{{}, error};
  _pendingPublication = receipt;
  if (receipt.cleanup) _cleanupPublicationReported = !receipt.status.ok();
  _publicationComplete.store(true);
  return late ? ESP_ERR_TIMEOUT : error;
}
} // namespace DP8382x::EspIdf
