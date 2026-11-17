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
Status busy() noexcept { return Status::Error(Err::BUSY, "Native owner is unavailable"); }
Status busDenied(bool quarantined) noexcept {
  return Status::Error(Err::BUSY, "Shared controller unavailable", quarantined ?
    static_cast<int32_t>(BusyDetail::CONTROLLER_QUARANTINED) : 0);
}
Status nativeStatus(esp_err_t error) noexcept {
  if (error == ESP_OK) return Status::Ok();
  return Status::Error(error == ESP_ERR_TIMEOUT ? Err::MDIO_TIMEOUT : Err::MDIO_ERROR,
                       "Native SDK failure", error);
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
  const uint64_t ticks = us / (uint64_t{portTICK_PERIOD_MS} * 1000U);
  const auto cap = static_cast<uint64_t>(portMAX_DELAY - 1U);
  return static_cast<TickType_t>(ticks < cap ? ticks : cap);
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
  phy.advertise_pause_ability = rejectPause; phy.loopback = rejectLoopback;
  phy.set_speed = rejectSpeed; phy.set_duplex = rejectDuplex; phy.del = deletePhy;
}
Phy& Phy::owner(esp_eth_phy_t* phy) noexcept { return *reinterpret_cast<PhyHandle*>(phy)->owner; }
uint64_t Phy::now(void*) noexcept { return timeNow(); }
void Phy::fenceCallback(void*) noexcept {} // No borrowed context can outlive a fence invocation.
bool Phy::enter() noexcept {
  if (xSemaphoreTake(_gate, 0) != pdTRUE) return false;
  const auto outcome = _installOutcome.exchange(std::numeric_limits<int64_t>::min());
  if (outcome != std::numeric_limits<int64_t>::min()) {
    const auto error = static_cast<esp_err_t>(outcome);
    _installed = error == ESP_OK; _ethernet = _installCandidate;
    _phase = Phase::STOPPED; record(error);
    if (!_installed) _mediator = nullptr;
  }
  return true;
}
void Phy::leave() noexcept { (void)xSemaphoreGive(_gate); }
void Phy::record(esp_err_t error, bool cleanup) noexcept {
  auto& target = cleanup ? _record.cleanupError : _record.firstError;
  if (error != ESP_OK && target.ok()) target = nativeStatus(error);
}

esp_err_t Phy::create(const PhyConfig& config, Phy*& output) noexcept {
  if (!config.mac || !config.bus || !config.bus->mutex ||
      !config.mac->read_phy_reg || !config.mac->write_phy_reg || !config.mac->stop ||
      !config.mac->set_mediator || !config.mac->init || !config.mac->deinit ||
      !config.mac->set_link || !config.mac->set_speed || !config.mac->set_duplex ||
      !config.mac->set_peer_pause_ability ||
      !config.linkPollMs || !config.callbackBudgetUs || !config.notificationUs || !config.lockWaitUs ||
      config.core.transport.bounds.fixedUs < config.lockWaitUs ||
      !config.maxFencePolls || !config.maxStopAttempts) return ESP_ERR_INVALID_ARG;
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
  if (bus.quarantined.load() || bus.suspended.load() || timeNow() < bus.quietUntilUs.load()) {
    const bool quarantined = bus.quarantined.load();
    report.status = busDenied(quarantined); report.controllerReusable = !quarantined; return report;
  }
  const auto before = timeNow();
  const auto& bounds = self._config.core.transport.bounds;
  const uint64_t required = uint64_t{bounds.fixedUs} + uint64_t{batch.frameCount} * bounds.frameUs + bounds.clockLagUs;
  if (before >= budget.deadlineUs || batch.frameCount > budget.maxFrames || required >= budget.deadlineUs - before) {
    report.status = Status::Error(Err::OPERATION_TIMEOUT, "Native batch budget expired"); return report;
  }
  const auto remaining = budget.deadlineUs - before;
  const auto wait = remaining < self._config.lockWaitUs ? remaining : self._config.lockWaitUs;
  if (xSemaphoreTake(bus.mutex, ticksWithin(wait)) != pdTRUE) { report.status = busy(); return report; }
  if (bus.quarantined.load() || bus.suspended.load() || timeNow() < bus.quietUntilUs.load()) {
    report.status = busDenied(bus.quarantined.load());
  } else {
    for (uint8_t i = 0; i < batch.frameCount; ++i) {
      if (timeNow() >= budget.deadlineUs) {
        report.status = Status::Error(Err::OPERATION_TIMEOUT, "Native frame deadline expired"); break;
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
  if (xSemaphoreGive(bus.mutex) != pdTRUE) {
    report.releaseStatus = nativeStatus(ESP_FAIL); bus.quarantined.store(true);
  }
  report.controllerReusable = !bus.quarantined.load();
  return report;
}

esp_err_t Phy::prepareBoard(const BoardControl& board, uint64_t deadlineUs) noexcept {
  if (!board.setReset || board.assertionUs < 25U || !board.maxPolls) return ESP_ERR_INVALID_ARG;
  if (!enter()) return ESP_ERR_INVALID_STATE;
  if (_installAttempted || _phase != Phase::STOPPED || _notifying) { leave(); return ESP_ERR_INVALID_STATE; }
  const auto entry = timeNow();
  if (entry >= deadlineUs || board.powerOnAtUs > entry || board.clockStableAtUs > entry) {
    leave(); return ESP_ERR_INVALID_ARG;
  }
  _phase = Phase::CONTROL; _boardReady = false; _record.stage = LifecycleStage::BOARD;
  auto& bus = *_config.bus;
  bus.suspended.store(true);
  leave();
  // Drain existing whole batches. No mutex survives pin callbacks or waits.
  esp_err_t error = ESP_OK;
  if (xSemaphoreTake(bus.mutex, ticksWithin(deadlineUs - entry)) != pdTRUE) error = ESP_ERR_TIMEOUT;
  else if (xSemaphoreGive(bus.mutex) != pdTRUE) { bus.quarantined.store(true); error = ESP_FAIL; }
  uint32_t polls = 0;
  auto waitUntil = [&](uint64_t due) {
    while (timeNow() < due) {
      if (timeNow() >= deadlineUs || ++polls >= board.maxPolls) return ESP_ERR_TIMEOUT;
      vTaskDelay(1);
    }
    return timeNow() < deadlineUs ? ESP_OK : ESP_ERR_TIMEOUT;
  };
  uint64_t clockDue = 0, porDue = 0, due = 0;
  if (!addTime(board.clockStableAtUs, 100U, clockDue) || !addTime(board.powerOnAtUs, 50000U, porDue))
    error = ESP_ERR_INVALID_ARG;
  if (error == ESP_OK) error = waitUntil(clockDue > porDue ? clockDue : porDue);
  if (error == ESP_OK) error = board.setReset(board.context, true);
  if (error == ESP_OK) {
    if (!addTime(timeNow(), board.assertionUs, due)) error = ESP_ERR_TIMEOUT;
    else error = waitUntil(due);
  }
  if (error == ESP_OK) error = board.setReset(board.context, false);
  if (error == ESP_OK) {
    if (!addTime(timeNow(), 2000U, due)) error = ESP_ERR_TIMEOUT;
    else { bus.quietUntilUs.store(due); error = waitUntil(due); }
  }
  if (!enter()) return ESP_ERR_INVALID_STATE; // Admission remains closed if owner protocol was violated.
  record(error); _boardReady = error == ESP_OK; _phase = Phase::STOPPED;
  if (_boardReady) bus.suspended.store(false);
  leave(); return error;
}

esp_err_t Phy::install(esp_eth_handle_t& output) noexcept {
  if (!enter()) return ESP_ERR_INVALID_STATE;
  if (!_boardReady || _installAttempted || _phase != Phase::STOPPED || _notifying ||
      _config.bus->suspended.load() || _config.bus->quarantined.load()) { leave(); return ESP_ERR_INVALID_STATE; }
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
  _installOutcome.store(error);
  _installing.store(false);
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
  if (!self.enter()) return ESP_ERR_INVALID_STATE;
  const bool installed = self._installed;
  if (installed) {
    // Unadmitted raw uninstall already deleted the SDK timer before this callback.
    self._phase = Phase::MAINTENANCE; self._record.stage = LifecycleStage::RETAINED;
    self.record(ESP_ERR_INVALID_STATE);
  }
  self.leave(); return installed ? ESP_ERR_INVALID_STATE : ESP_OK;
}
esp_err_t Phy::rejectReset(esp_eth_phy_t*) noexcept { return ESP_ERR_NOT_SUPPORTED; }
esp_err_t Phy::rejectPower(esp_eth_phy_t*, bool) noexcept { return ESP_ERR_NOT_SUPPORTED; }
esp_err_t Phy::rejectPause(esp_eth_phy_t*, uint32_t) noexcept { return ESP_ERR_NOT_SUPPORTED; }
esp_err_t Phy::rejectLoopback(esp_eth_phy_t*, bool) noexcept { return ESP_ERR_NOT_SUPPORTED; }
esp_err_t Phy::rejectSpeed(esp_eth_phy_t*, eth_speed_t) noexcept { return ESP_ERR_NOT_SUPPORTED; }
esp_err_t Phy::rejectDuplex(esp_eth_phy_t*, eth_duplex_t) noexcept { return ESP_ERR_NOT_SUPPORTED; }
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
  // Immutable declared AN policy, including installation before any hardware I/O.
  if (command == ESP_ETH_PHY_AUTONEGO_G_STAT) { *enabled = true; return ESP_OK; }
  auto& self = owner(phy);
  if (!self.enter()) return ESP_ERR_INVALID_STATE;
  if (command != ESP_ETH_PHY_AUTONEGO_RESTART) { self.leave(); return ESP_ERR_NOT_SUPPORTED; }
  if (self._phase != Phase::STARTING || self._notifying) { self.leave(); return ESP_ERR_INVALID_STATE; }
  const auto status = self._core.restartAutoNegotiation(self.callbackBudget(3));
  if (status.ok()) *enabled = true;
  else if (self._record.firstError.ok()) self._record.firstError = status;
  self.shareQuiet(); self.leave(); return nativeError(status);
}

esp_err_t Phy::publish(PublishedTuple tuple, uint64_t deadlineUs) noexcept {
  if (!enter()) return ESP_ERR_INVALID_STATE;
  if (!_mediator || _notifying || _record.publicationUncertain ||
      (tuple.up && _phase != Phase::RUNNING && _phase != Phase::STARTING)) { leave(); return ESP_ERR_INVALID_STATE; }
  if (same(tuple, _record.publishedTuple)) { leave(); return ESP_OK; }
  _notifying = true;
  // If reacquisition fails after an external callback, this precommitted guard
  // retains the object and refuses subsequent publication rather than guessing.
  _record.publicationUncertain = true;
  const auto old = _record.publishedTuple;
  if (tuple.up) {
    _record.publishedTuple.speedAccepted = false;
    _record.publishedTuple.duplexAccepted = false;
    _record.publishedTuple.pauseAccepted = false;
  }
  auto* mediator = _mediator;
  leave();
  auto notify = [&](esp_eth_state_t state, uintptr_t value, bool link) {
    const auto before = timeNow();
    if (before >= deadlineUs || _config.notificationUs >= deadlineUs - before) {
      if (enter()) { record(ESP_ERR_TIMEOUT, _phase == Phase::STOPPING); _notifying = false; _record.publicationUncertain = false; leave(); }
      return ESP_ERR_TIMEOUT;
    }
    const auto error = mediator->on_state_changed(mediator, state, reinterpret_cast<void*>(value));
    if (!enter()) return ESP_ERR_INVALID_STATE;
    if (link && (error == ESP_OK || _config.pinnedP4Mac)) {
      _record.publishedTuple.up = value == ETH_LINK_UP;
      _record.eventLost = error != ESP_OK;
    } else if (error == ESP_OK) {
      if (state == ETH_STATE_SPEED) { _record.publishedTuple.speed = tuple.speed; _record.publishedTuple.speedAccepted = true; }
      if (state == ETH_STATE_DUPLEX) { _record.publishedTuple.duplex = tuple.duplex; _record.publishedTuple.duplexAccepted = true; }
      if (state == ETH_STATE_PAUSE) { _record.publishedTuple.peerPause = tuple.peerPause; _record.publishedTuple.pauseAccepted = true; }
    }
    const auto terminal = error == ESP_OK && timeNow() >= deadlineUs ? ESP_ERR_TIMEOUT : error;
    if (terminal != ESP_OK) {
      record(terminal, _phase == Phase::STOPPING);
      _record.publicationUncertain = error != ESP_OK && !(link && _config.pinnedP4Mac);
      _notifying = false;
    }
    leave(); return terminal;
  };
  esp_err_t error = ESP_OK;
  if (old.up) error = notify(ETH_STATE_LINK, ETH_LINK_DOWN, true);
  if (error == ESP_OK && tuple.up) {
    error = notify(ETH_STATE_SPEED, tuple.speed == LinkSpeed::MBPS_100 ? ETH_SPEED_100M : ETH_SPEED_10M, false);
    if (error == ESP_OK) error = notify(ETH_STATE_DUPLEX, tuple.duplex == Duplex::FULL ? ETH_DUPLEX_FULL : ETH_DUPLEX_HALF, false);
    if (error == ESP_OK) error = notify(ETH_STATE_PAUSE, tuple.peerPause ? 1U : 0U, false);
    if (error == ESP_OK) error = notify(ETH_STATE_LINK, ETH_LINK_UP, true);
  }
  if (error == ESP_OK) {
    if (!enter()) return ESP_ERR_INVALID_STATE;
    _record.publicationUncertain = false; _notifying = false; leave();
  }
  return error;
}
esp_err_t Phy::getLink(esp_eth_phy_t* phy) noexcept {
  auto& self = owner(phy);
  if (!self.enter()) return ESP_ERR_INVALID_STATE;
  if (self._phase != Phase::STARTING && self._phase != Phase::RUNNING) { self.leave(); return ESP_ERR_INVALID_STATE; }
  if (self._phase == Phase::STARTING) self._startPosted = true; // SDK posts START before this callback.
  if (self._notifying || self._record.publicationUncertain) { self.leave(); return ESP_ERR_INVALID_STATE; }
  const auto budget = self.callbackBudget(16);
  const uint64_t reserve = uint64_t{self._config.notificationUs} * 5U;
  const auto current = timeNow();
  if (current >= budget.deadlineUs || reserve >= budget.deadlineUs - current) {
    self.record(ESP_ERR_TIMEOUT); self.leave(); return ESP_ERR_TIMEOUT;
  }
  LinkSnapshot observation{};
  const auto status = self._core.readLinkStatus(observation, {budget.deadlineUs - reserve, budget.maxFrames});
  if (!status.ok()) {
    if (self._record.firstError.ok()) self._record.firstError = status;
    self.leave(); return nativeError(status);
  }
  if (observation.linkUp && !observation.resolvedModeValid) { self.record(ESP_ERR_INVALID_STATE); self.leave(); return ESP_ERR_INVALID_STATE; }
  PublishedTuple tuple{};
  tuple.up = observation.linkUp;
  if (tuple.up) {
    tuple.speed = observation.speed; tuple.duplex = observation.duplex;
    tuple.peerPause = observation.duplex == Duplex::FULL && (observation.rawAnlpar & 0x0400U) != 0U;
  }
  self.leave(); return self.publish(tuple, budget.deadlineUs);
}
esp_err_t Phy::setLink(esp_eth_phy_t* phy, eth_link_t link) noexcept {
  if (link != ETH_LINK_DOWN) return ESP_ERR_NOT_SUPPORTED;
  auto& self = owner(phy);
  if (!self.enter()) return ESP_ERR_INVALID_STATE;
  if (self._phase != Phase::STOPPING) { self.leave(); return ESP_ERR_INVALID_STATE; }
  const auto deadline = self._lifecycleDeadline;
  self.leave(); return self.publish({}, deadline);
}

esp_err_t Phy::start(uint64_t deadlineUs) noexcept {
  if (!enter()) return ESP_ERR_INVALID_STATE;
  const auto settings = _core.settingsSnapshot();
  if (!_installed || _phase != Phase::STOPPED || _notifying || _record.publicationUncertain ||
      !_core.isInitialized() || settings.trust != ConfigurationTrust::VERIFIED ||
      !settings.desiredValid || settings.desired.mode != LinkMode::AUTONEGOTIATED || timeNow() >= deadlineUs) {
    leave(); return ESP_ERR_INVALID_STATE;
  }
  _phase = Phase::STARTING; _lifecycleDeadline = deadlineUs; _record.stage = LifecycleStage::START;
  _startPosted = false; _stopPosted = false;
  leave();
  auto error = esp_eth_start(_ethernet);
  if (error == ESP_OK && timeNow() >= deadlineUs) error = ESP_ERR_TIMEOUT;
  if (!enter()) return ESP_ERR_INVALID_STATE;
  record(error);
  if (error == ESP_OK) { _phase = Phase::RUNNING; leave(); return ESP_OK; }
  _phase = Phase::STOPPING; leave();
  const auto stopped = esp_eth_stop(_ethernet);
  (void)finishStop(deadlineUs, stopped);
  return error;
}
esp_err_t Phy::stop(uint64_t deadlineUs) noexcept {
  if (!enter()) return ESP_ERR_INVALID_STATE;
  if (!_installed || _phase == Phase::CONTROL || _phase == Phase::MAINTENANCE || _notifying) {
    leave(); return ESP_ERR_INVALID_STATE;
  }
  if (timeNow() >= deadlineUs) { leave(); return ESP_ERR_TIMEOUT; }
  const bool resuming = _phase == Phase::STOPPING;
  _phase = Phase::STOPPING; _record.stage = LifecycleStage::STOP; _lifecycleDeadline = deadlineUs; leave();
  return finishStop(deadlineUs, resuming ? ESP_OK : esp_eth_stop(_ethernet), !resuming);
}
esp_err_t Phy::finishStop(uint64_t deadlineUs, esp_err_t initial, bool stopCalled) noexcept {
  if (!enter()) return ESP_ERR_INVALID_STATE;
  if (stopCalled && initial == ESP_OK) _stopPosted = true;
  record(initial, true); _record.stage = LifecycleStage::FENCE;
  bool arm = !_fenceArmed;
  leave();
  esp_err_t error = ESP_OK;
  if (arm) {
    error = esp_timer_start_once(_fence, 0);
    if (!enter()) return ESP_ERR_INVALID_STATE;
    if (error == ESP_OK) _fenceArmed = true;
    record(error, true); leave();
  }
  uint32_t polls = 0;
  while (error == ESP_OK && esp_timer_is_active(_fence)) {
    if (timeNow() >= deadlineUs || ++polls >= _config.maxFencePolls) { error = ESP_ERR_TIMEOUT; break; }
    vTaskDelay(1);
  }
  if (error != ESP_OK) {
    if (enter()) { record(error, true); leave(); }
    return initial != ESP_OK ? initial : error;
  }
  error = publish({}, deadlineUs);
  if (error == ESP_OK && _startPosted && !_stopPosted) {
    const auto current = timeNow();
    error = current >= deadlineUs ? ESP_ERR_TIMEOUT :
      esp_event_post(ETH_EVENT, ETHERNET_EVENT_STOP, &_ethernet, sizeof(_ethernet), ticksWithin(deadlineUs - current));
    if (error == ESP_OK) _stopPosted = true;
  }
  if (enter()) { record(error, true); _record.stage = LifecycleStage::MAC_STOP; leave(); }
  esp_err_t macError = ESP_ERR_TIMEOUT;
  for (uint32_t attempt = 0; attempt < _config.maxStopAttempts && timeNow() < deadlineUs; ++attempt) {
    macError = _config.mac->stop(_config.mac);
    if (macError == ESP_OK) break;
  }
  if (macError == ESP_OK && timeNow() >= deadlineUs) macError = ESP_ERR_TIMEOUT;
  if (!enter()) return ESP_ERR_INVALID_STATE;
  record(macError, true);
  if (error == ESP_OK && macError == ESP_OK && !_record.publicationUncertain) {
    _phase = Phase::STOPPED; _fenceArmed = false;
  }
  leave();
  return initial != ESP_OK ? initial : (error != ESP_OK ? error : macError);
}
esp_err_t Phy::retire(uint64_t deadlineUs) noexcept {
  if (!enter()) return ESP_ERR_INVALID_STATE;
  if (_phase == Phase::CONTROL || _notifying) { leave(); return ESP_ERR_INVALID_STATE; }
  leave();
  const auto stopped = stop(deadlineUs);
  if (!enter()) return ESP_ERR_INVALID_STATE;
  if (!_installed) { leave(); return ESP_ERR_INVALID_STATE; }
  // Admission failures (including an already expired deadline) may precede the
  // stop journal. Retirement still owes an explicit cleanup outcome.
  record(stopped, true);
  _phase = Phase::MAINTENANCE; _record.stage = LifecycleStage::RETAINED;
  leave(); return ESP_ERR_NOT_SUPPORTED; // native_rx_quiescence: no public safe destruction route.
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
  output = _record; leave(); return Status::Ok();
}
void Phy::shareQuiet() noexcept {
  const auto progress = _core.operationSnapshot();
  if (progress.busQuietValid) {
    auto& floor = _config.bus->quietUntilUs;
    auto previous = floor.load();
    while (previous < progress.busQuietUntilUs && !floor.compare_exchange_weak(previous, progress.busQuietUntilUs)) {}
  }
}
Status Phy::startInitialize(const InitializeOptions& options, const OperationBudget& budget, OperationToken& token) noexcept {
  if (!enter()) return busy();
  if (!_installed || _phase != Phase::STOPPED || _notifying) { leave(); return busy(); }
  // Hard reset shared silence must already exclude every other participant;
  // stage 38 adds the coordinated disruptive-owner route.
  if (options.reset == ResetKind::HARD) { leave(); return Status::Error(Err::INVALID_CONFIG, "Native hard reset requires shared-board maintenance"); }
  const auto status = _core.startInitialize(options, budget, token);
  if (status.inProgress()) { _phase = Phase::CONTROL; _record.stage = LifecycleStage::INITIALIZE; }
  leave(); return status;
}
Status Phy::startConfigure(const LinkSettings& settings, const OperationBudget& budget, OperationToken& token) noexcept {
  if (!enter()) return busy();
  if (!_installed || _phase != Phase::STOPPED || _notifying) { leave(); return busy(); }
  if (settings.mode != LinkMode::AUTONEGOTIATED) { leave(); return Status::Error(Err::INVALID_CONFIG, "SDK mode transition requires stage 38 control route"); }
  const auto status = _core.startConfigure(settings, budget, token);
  if (status.inProgress()) _phase = Phase::CONTROL;
  leave(); return status;
}
Status Phy::pollOperation(OperationToken token, uint32_t maxFrames) noexcept {
  if (!enter()) return busy();
  if (_phase != Phase::CONTROL || !_installed || _notifying) { leave(); return busy(); }
  const auto status = _core.pollOperation(token, maxFrames); shareQuiet();
  if (!status.ok() && !status.inProgress() && _record.firstError.ok()) _record.firstError = status;
  leave(); return status;
}
Status Phy::takeResult(OperationToken token, OperationResult& output) noexcept {
  if (!enter()) return busy();
  const auto status = _core.takeResult(token, output);
  if (status.ok() && _phase == Phase::CONTROL) _phase = Phase::STOPPED;
  leave(); return status;
}
Status Phy::cancel(OperationToken token) noexcept {
  if (!enter()) return busy();
  const auto status = _core.cancel(token); leave(); return status;
}
Status Phy::operationSnapshot(OperationSnapshot& output) noexcept {
  if (!enter()) return busy();
  output = _core.operationSnapshot(); leave(); return Status::Ok();
}
Status Phy::readClause22(uint32_t address, uint16_t& output, const IoBudget& budget) noexcept {
  if (!enter()) return busy();
  if (!_installed || _phase != Phase::STOPPED || _notifying) { leave(); return busy(); }
  const auto status = _core.readClause22(address, output, budget); leave(); return status;
}
Status Phy::readMmd(uint32_t devad, uint32_t address, uint16_t& output, const IoBudget& budget) noexcept {
  if (!enter()) return busy();
  if (!_installed || _phase != Phase::STOPPED || _notifying) { leave(); return busy(); }
  const auto status = _core.readMmd(devad, address, output, budget); leave(); return status;
}
Status Phy::discover(uint32_t addressMask, DiscoverySnapshot& output, const IoBudget& budget) noexcept {
  if (!addressMask) return Status::Error(Err::INVALID_PARAM, "Empty discovery mask");
  if (!enter()) return busy();
  const auto operation = _core.operationSnapshot();
  if (!_installed || _phase != Phase::STOPPED || _notifying || operation.cleanupOwed ||
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
    leave(); return Status::Error(Err::OPERATION_TIMEOUT, "Insufficient whole discovery allowance");
  }
  _discovery = {}; _discovery.requestedMask = addressMask; _discovery.status = Status::Ok();
  uint64_t previous = before;
  for (uint8_t address = 0; address < 32U; ++address) {
    if ((addressMask & (uint32_t{1} << address)) == 0U) continue;
    MdioBatch batch{}; batch.phyAddress = address; batch.frameCount = 2;
    batch.frames[0].registerAddress = 2; batch.frames[1].registerAddress = 3;
    _discovery.lastBatch = execute(this, batch, {budget.deadlineUs, budget.maxFrames - _discovery.framesAttempted});
    const auto& report = _discovery.lastBatch;
    auto& entry = _discovery.entries[address];
    for (uint8_t index = 0; index < 2; ++index) {
      const auto& frame = report.frames[index];
      if (frame.attempted) ++_discovery.framesAttempted;
      if (frame.completed && frame.valueValid) {
        if (index == 0) { entry.id1 = frame.value; entry.id1Valid = true; }
        else { entry.id2 = frame.value; entry.id2Valid = true; }
      }
    }
    auto status = !report.status.ok() ? report.status : report.releaseStatus;
    const auto after = timeNow();
    if (after < previous) {
      _config.bus->quarantined.store(true);
      if (status.ok()) status = Status::Error(Err::INVALID_CONFIG, "Discovery clock regressed", static_cast<int32_t>(ConfigDetail::CLOCK_CONTRACT));
    } else if (status.ok() && (after >= budget.deadlineUs || bounds.clockLagUs >= budget.deadlineUs - after)) {
      status = Status::Error(Err::OPERATION_TIMEOUT, "Discovery completed after allowance");
    }
    if (!status.ok()) { _discovery.status = status; break; }
    previous = after;
  }
  const auto status = _discovery.status;
  if (status.ok()) output = _discovery;
  leave(); return status;
}
Status Phy::discoverySnapshot(DiscoverySnapshot& output) noexcept {
  if (!enter()) return busy();
  output = _discovery; leave(); return Status::Ok();
}
esp_err_t Phy::retryLinkEvent(uint64_t deadlineUs) noexcept {
  if (!enter()) return ESP_ERR_INVALID_STATE;
  if (!_installed || !_record.eventLost || _record.publicationUncertain || _notifying) { leave(); return ESP_ERR_INVALID_STATE; }
  if (_phase != Phase::RUNNING && !(_phase == Phase::STOPPED && !_record.publishedTuple.up)) {
    leave(); return ESP_ERR_INVALID_STATE;
  }
  const auto id = _record.publishedTuple.up ? ETHERNET_EVENT_CONNECTED : ETHERNET_EVENT_DISCONNECTED;
  _notifying = true; leave();
  const auto current = timeNow();
  const auto error = current >= deadlineUs ? ESP_ERR_TIMEOUT :
    esp_event_post(ETH_EVENT, id, &_ethernet, sizeof(_ethernet), ticksWithin(deadlineUs - current));
  if (!enter()) return ESP_ERR_INVALID_STATE;
  if (error == ESP_OK) _record.eventLost = false;
  _notifying = false; leave(); return error;
}
} // namespace DP8382x::EspIdf
