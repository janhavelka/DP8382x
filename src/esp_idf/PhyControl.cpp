/// @file PhyControl.cpp
/// @brief Public SDK cache acknowledgement and separately typed MAC/PHY policy calls.
#include <DP8382x/esp_idf/Phy.h>

namespace DP8382x::EspIdf {
namespace {
uint64_t nowUs() noexcept { return static_cast<uint64_t>(esp_timer_get_time()); }
NativeStatus denied() noexcept {
  return {Status::Error(Err::BUSY, "Native control owner unavailable", static_cast<int32_t>(BusyDetail::OWNER_ADMISSION))};
}
NativeStatus unverified() noexcept {
  return {Status::Error(Err::NOT_INITIALIZED, "Restore and verify core settings before SDK reconciliation")};
}
NativeStatus expired() noexcept {
  return {Status::Error(Err::OPERATION_TIMEOUT, "Native control budget exhausted", static_cast<int32_t>(TimeoutDetail::DEADLINE))};
}
NativeStatus admissionFailure(uint64_t deadlineUs) noexcept {
  return nowUs() >= deadlineUs ? expired() : denied();
}
esp_err_t flowError(NativeStatus status) noexcept {
  if (status.sdkError != ESP_OK) return status.sdkError;
  if (status.ok()) return ESP_OK;
  return status.is(Err::OPERATION_TIMEOUT) ? ESP_ERR_TIMEOUT : ESP_ERR_INVALID_STATE;
}
}

NativeStatus Phy::reconcileSdkState(uint64_t deadlineUs) noexcept {
  if (!enterUntil(deadlineUs)) return admissionFailure(deadlineUs);
  refreshAgreement();
  if (!stoppedAdmission()) { leave(); return denied(); }
  if (!verifiedSettings()) { leave(); return unverified(); }
  const auto current = nowUs();
  // S_AN, G_AN and (in forced mode) S_SPEED/S_DUPLEX. Each public call must
  // fit the caller-qualified callback overhead even though these callbacks do no I/O.
  const auto settings = _core.settingsSnapshot().desired;
  const uint64_t calls = settings.mode == LinkMode::FORCED ? 4U : 2U;
  if (current >= deadlineUs || calls * _config.notificationUs >= deadlineUs - current) {
    leave(); return expired();
  }
  _phase = Phase::CONTROL; _controlTask = xTaskGetCurrentTaskHandle(); _controlDeadline = deadlineUs;
  _sdkState = {}; _sdkState.status = denied();
  SdkStateSnapshot receipt{};
  bool enabled = settings.mode == LinkMode::AUTONEGOTIATED;
  const bool desiredAn = enabled;
  leave();

  auto call = [&](ControlEntry entry, esp_eth_io_cmd_t command, void* value) -> NativeStatus {
    if (!enterUntil(deadlineUs)) return admissionFailure(deadlineUs);
    const auto before = nowUs();
    if (_phase != Phase::CONTROL || _frameworkStopOwed) { leave(); return denied(); }
    if (!verifiedSettings()) { leave(); return unverified(); }
    if (before >= deadlineUs || _config.notificationUs >= deadlineUs - before) { leave(); return expired(); }
    _controlEntry = entry;
    leave();
    const auto error = esp_eth_ioctl(_ethernet, command, value);
    if (error != ESP_OK) return {{}, error};
    if (command == ETH_CMD_S_AUTONEGO) receipt.autoNegotiationAccepted = true;
    if (command == ETH_CMD_G_AUTONEGO) { receipt.cacheValid = true; receipt.cachedAutoNegotiation = enabled; }
    if (command == ETH_CMD_S_SPEED) receipt.speedAccepted = true;
    if (command == ETH_CMD_S_DUPLEX_MODE) receipt.duplexAccepted = true;
    const auto after = nowUs();
    if (after < before || after >= deadlineUs || after - before > _config.notificationUs) return expired();
    return {};
  };
  auto status = call(ControlEntry::AUTONEGO, ETH_CMD_S_AUTONEGO, &enabled);
  if (status.ok()) status = call(ControlEntry::NONE, ETH_CMD_G_AUTONEGO, &enabled);
  if (status.ok() && enabled != desiredAn)
    status = {Status::Error(Err::CONFIG_MISMATCH, "SDK autonegotiation cache disagrees with verified settings")};
  if (status.ok() && !desiredAn) {
    eth_speed_t speed = settings.forced.speed == LinkSpeed::MBPS_100 ? ETH_SPEED_100M : ETH_SPEED_10M;
    status = call(ControlEntry::SPEED, ETH_CMD_S_SPEED, &speed);
    if (status.ok()) {
      eth_duplex_t duplex = settings.forced.duplex == Duplex::FULL ? ETH_DUPLEX_FULL : ETH_DUPLEX_HALF;
      status = call(ControlEntry::DUPLEX, ETH_CMD_S_DUPLEX_MODE, &duplex);
    }
  }
  receipt.status = status; receipt.reconciled = status.ok();
  _pendingSdk = receipt;
  _controlCompletion.store(ControlCompletion::SDK);
  return receipt.status;
}

esp_err_t Phy::setFlowControl(bool enabled, uint32_t pauseAbility, uint64_t deadlineUs) noexcept {
  if (pauseAbility > 3U) return ESP_ERR_INVALID_ARG;
  if (!enterUntil(deadlineUs)) return flowError(admissionFailure(deadlineUs));
  const auto settings = _core.settingsSnapshot().desired;
  const uint32_t desired = (settings.advertisement.symmetricPause ? 1U : 0U) |
      (settings.advertisement.asymmetricPause ? 2U : 0U);
  refreshAgreement();
  if (!stoppedAdmission() || !_sdkState.reconciled || !verifiedSettings() ||
      settings.mode != LinkMode::AUTONEGOTIATED || pauseAbility != desired) {
    leave(); return ESP_ERR_INVALID_STATE;
  }
  if (!_config.mac->enable_flow_ctrl) { leave(); return ESP_ERR_NOT_SUPPORTED; }
  const auto current = nowUs();
  if (current >= deadlineUs || uint64_t{2} * _config.notificationUs >= deadlineUs - current) {
    leave(); return ESP_ERR_TIMEOUT;
  }
  _phase = Phase::CONTROL; _controlTask = xTaskGetCurrentTaskHandle(); _controlDeadline = deadlineUs;
  _unexpectedFlowEffect.store(false); // Explicit owner retry is the only repair.
  _flow = {}; _flow.requestedEnabled = enabled; _flow.pauseAbility = pauseAbility;
  _flow.macAttempted = _flow.macEffectPossible = true;
  auto receipt = _flow;
  // Precommit the possible MAC effect before invoking external code. Callback
  // failure is not evidence that the policy remained unchanged.
  leave();
  const auto before = nowUs();
  const auto macError = _config.mac->enable_flow_ctrl(_config.mac, enabled);
  receipt.macError = macError; receipt.macAccepted = macError == ESP_OK;
  NativeStatus status{{}, macError};
  const auto after = nowUs();
  if (status.ok() && (after < before || after - before > _config.notificationUs ||
      after >= deadlineUs || _config.notificationUs >= deadlineUs - after))
    status = expired();
  if (status.ok()) {
    if (!enterUntil(deadlineUs)) status = admissionFailure(deadlineUs);
    else {
      const auto reservedAt = nowUs();
      if (_phase != Phase::CONTROL || _frameworkStopOwed || _unexpectedFlowEffect.load()) status = denied();
      else if (!verifiedSettings()) status = unverified();
      else if (reservedAt >= deadlineUs || _config.notificationUs >= deadlineUs - reservedAt) status = expired();
      else _controlEntry = ControlEntry::PAUSE;
      leave();
    }
  }
  if (status.ok()) {
    receipt.phyAttempted = true;
    const auto phyBefore = nowUs();
    receipt.phyError = _handle.phy.advertise_pause_ability(&_handle.phy, pauseAbility);
    receipt.phyAccepted = receipt.phyError == ESP_OK;
    status.sdkError = receipt.phyError;
    const auto phyAfter = nowUs();
    if (status.ok() && (phyAfter < phyBefore || phyAfter - phyBefore > _config.notificationUs))
      status = expired();
  } else {
    receipt.phyError = flowError(status);
  }
  if (status.ok() && nowUs() >= deadlineUs) status = expired();
  if (_unexpectedFlowEffect.load() && status.ok()) status = denied();
  receipt.reconciled = status.ok();
  _pendingFlow = receipt; _pendingControlError = status;
  _controlCompletion.store(ControlCompletion::FLOW);
  return flowError(status);
}
} // namespace DP8382x::EspIdf
