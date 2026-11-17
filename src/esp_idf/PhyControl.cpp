/// @file PhyControl.cpp
/// @brief Public SDK cache acknowledgement and separately typed MAC/PHY policy calls.
#include <DP8382x/esp_idf/Phy.h>

namespace DP8382x::EspIdf {
namespace {
uint64_t nowUs() noexcept { return static_cast<uint64_t>(esp_timer_get_time()); }
Status denied() noexcept {
  return Status::Error(Err::BUSY, "Native control owner unavailable", static_cast<int32_t>(BusyDetail::OWNER_ADMISSION));
}
Status controlStatus(esp_err_t error) noexcept {
  if (error == ESP_OK) return Status::Ok();
  if (error == ESP_ERR_TIMEOUT)
    return Status::Error(Err::OPERATION_TIMEOUT, "Native control deadline expired", static_cast<int32_t>(TimeoutDetail::DEADLINE));
  return Status::Error(Err::MDIO_ERROR, "Native public control failed", error);
}
}

Status Phy::reconcileSdkState(uint64_t deadlineUs) noexcept {
  if (!enter()) return denied();
  refreshAgreement();
  if (!stoppedAdmission()) { leave(); return denied(); }
  if (!verifiedSettings()) {
    leave(); return Status::Error(Err::NOT_INITIALIZED, "Restore and verify core settings before SDK reconciliation");
  }
  const auto current = nowUs();
  // S_AN, G_AN and (in forced mode) S_SPEED/S_DUPLEX. Each public call must
  // fit the caller-qualified callback overhead even though these callbacks do no I/O.
  const auto settings = _core.settingsSnapshot().desired;
  const uint64_t calls = settings.mode == LinkMode::FORCED ? 4U : 2U;
  if (current >= deadlineUs || calls * _config.notificationUs >= deadlineUs - current) {
    leave(); return controlStatus(ESP_ERR_TIMEOUT);
  }
  _phase = Phase::CONTROL; _controlTask = xTaskGetCurrentTaskHandle(); _controlDeadline = deadlineUs;
  _sdkState = {}; _sdkState.status = denied();
  SdkStateSnapshot receipt{};
  bool enabled = settings.mode == LinkMode::AUTONEGOTIATED;
  const bool desiredAn = enabled;
  leave();

  auto call = [&](ControlEntry entry, esp_eth_io_cmd_t command, void* value) {
    if (!enter()) return ESP_ERR_INVALID_STATE;
    const auto before = nowUs();
    if (before >= deadlineUs || _config.notificationUs >= deadlineUs - before || !verifiedSettings() || _frameworkStopOwed) {
      leave(); return ESP_ERR_TIMEOUT;
    }
    _controlEntry = entry;
    leave();
    const auto error = esp_eth_ioctl(_ethernet, command, value);
    if (error == ESP_OK) {
      if (command == ETH_CMD_S_AUTONEGO) receipt.autoNegotiationAccepted = true;
      if (command == ETH_CMD_G_AUTONEGO) { receipt.cacheValid = true; receipt.cachedAutoNegotiation = enabled; }
      if (command == ETH_CMD_S_SPEED) receipt.speedAccepted = true;
      if (command == ETH_CMD_S_DUPLEX_MODE) receipt.duplexAccepted = true;
    }
    return error == ESP_OK && nowUs() >= deadlineUs ? ESP_ERR_TIMEOUT : error;
  };
  auto error = call(ControlEntry::AUTONEGO, ETH_CMD_S_AUTONEGO, &enabled);
  if (error == ESP_OK) error = call(ControlEntry::NONE, ETH_CMD_G_AUTONEGO, &enabled);
  if (error == ESP_OK && enabled != desiredAn) error = ESP_ERR_INVALID_RESPONSE;
  if (error == ESP_OK && !desiredAn) {
    eth_speed_t speed = settings.forced.speed == LinkSpeed::MBPS_100 ? ETH_SPEED_100M : ETH_SPEED_10M;
    error = call(ControlEntry::SPEED, ETH_CMD_S_SPEED, &speed);
    if (error == ESP_OK) {
      eth_duplex_t duplex = settings.forced.duplex == Duplex::FULL ? ETH_DUPLEX_FULL : ETH_DUPLEX_HALF;
      error = call(ControlEntry::DUPLEX, ETH_CMD_S_DUPLEX_MODE, &duplex);
    }
  }
  receipt.status = controlStatus(error); receipt.reconciled = error == ESP_OK;
  _pendingSdk = receipt;
  _controlCompletion.store(ControlCompletion::SDK);
  return receipt.status;
}

esp_err_t Phy::setFlowControl(bool enabled, uint32_t pauseAbility, uint64_t deadlineUs) noexcept {
  if (pauseAbility > 3U) return ESP_ERR_INVALID_ARG;
  if (!enter()) return ESP_ERR_INVALID_STATE;
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
  const auto macError = _config.mac->enable_flow_ctrl(_config.mac, enabled);
  receipt.macError = macError; receipt.macAccepted = macError == ESP_OK;
  auto error = macError;
  const auto after = nowUs();
  if (error == ESP_OK && (after >= deadlineUs || _config.notificationUs >= deadlineUs - after))
    error = ESP_ERR_TIMEOUT;
  if (error == ESP_OK) {
    if (!enter()) error = ESP_ERR_INVALID_STATE;
    else { _controlEntry = ControlEntry::PAUSE; leave(); }
  }
  if (error == ESP_OK) {
    receipt.phyAttempted = true;
    error = _handle.phy.advertise_pause_ability(&_handle.phy, pauseAbility);
  }
  receipt.phyError = error;
  receipt.phyAccepted = error == ESP_OK;
  if (error == ESP_OK && nowUs() >= deadlineUs) error = ESP_ERR_TIMEOUT;
  if (_unexpectedFlowEffect.load() && error == ESP_OK) error = ESP_ERR_INVALID_STATE;
  receipt.reconciled = error == ESP_OK;
  _pendingFlow = receipt; _pendingControlError = error;
  _controlCompletion.store(ControlCompletion::FLOW);
  return error;
}
} // namespace DP8382x::EspIdf
