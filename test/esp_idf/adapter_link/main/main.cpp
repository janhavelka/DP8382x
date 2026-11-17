// Real ESP32-P4 link fixture. Its default boot does not operate the board.
#include <DP8382x/esp_idf/Phy.h>
#include "FixtureIdentity.h"

#include <cstdio>
#include <cstdlib>
#include "driver/gpio.h"
#include "esp_eth_mac_esp.h"
#include "esp_event.h"
#include "esp_system.h"
#include "freertos/task.h"

namespace {
using namespace ::DP8382x;
using namespace ::DP8382x::EspIdf;

// Volatile preserves the complete real lifecycle route in the linked image.
// Change only after the board and conditional timing assumptions in README.md
// have been verified. Merely compiling this fixture does not qualify hardware.
volatile bool runBoardFixture = false;

struct Fixture {
  SharedBus bus{};
  Phy* phy{nullptr};
  esp_eth_mac_t* mac{nullptr};
  esp_eth_handle_t ethernet{nullptr};
};
Fixture fixture; // Retained through all callbacks and terminal resource state.

uint64_t nowUs() { return static_cast<uint64_t>(esp_timer_get_time()); }

esp_err_t setReset(void*, bool asserted) {
  // Set the output latch before enabling the pad to avoid a release glitch.
  esp_err_t error = gpio_set_level(GPIO_NUM_54, asserted ? 0 : 1);
  if (error == ESP_OK) error = gpio_set_direction(GPIO_NUM_54, GPIO_MODE_OUTPUT);
  return error;
}

esp_err_t dropFrame(esp_eth_handle_t, uint8_t* buffer, uint32_t, void*) {
  // RX storage remains valid even after retire. Every arrival is consumed.
  std::free(buffer);
  return ESP_OK;
}

OperationBudget operationBudget() {
  OperationBudget budget{};
  budget.io = {nowUs() + 5000000U, 2048U};
  budget.pollIntervalUs = 1000;
  budget.maxStalledPolls = 10000;
  return budget;
}

Status finishOperation(OperationToken token) {
  // A finite owner session supplements the core's aggregate frame/deadline caps.
  for (uint32_t polls = 0; polls < 10000U; ++polls) {
    const Status status = fixture.phy->pollOperation(token);
    if (!status.inProgress()) {
      OperationResult result{};
      const Status delivered = fixture.phy->takeResult(token, result);
      return delivered.ok() ? result.status : delivered;
    }
    vTaskDelay(1);
  }
  const Status cancelled = fixture.phy->cancel(token);
  OperationResult result{};
  const Status delivered = fixture.phy->takeResult(token, result);
  return !cancelled.ok() ? cancelled : !delivered.ok() ? delivered : result.status;
}

void showStatus(const char* operation, Status status) {
  std::printf("%s: code=%s detail=%ld\n", operation, errorName(status.code),
              static_cast<long>(status.detail));
}

esp_err_t showSdkCachedSettings(bool expectedAutonegotiation) {
  bool autonegotiation = !expectedAutonegotiation;
  eth_speed_t speed{};
  eth_duplex_t duplex{};
  esp_err_t error = esp_eth_ioctl(fixture.ethernet, ETH_CMD_G_AUTONEGO, &autonegotiation);
  if (error == ESP_OK) error = esp_eth_ioctl(fixture.ethernet, ETH_CMD_G_SPEED, &speed);
  if (error == ESP_OK) error = esp_eth_ioctl(fixture.ethernet, ETH_CMD_G_DUPLEX_MODE, &duplex);
  if (error == ESP_OK) {
    // Speed/duplex are SDK publication caches, possibly default or stale.
    // Reconfiguration does not make them desired-setting readback.
    std::printf("SDK cached AN=%u; last published speed=%u duplex=%u\n",
                static_cast<unsigned>(autonegotiation), static_cast<unsigned>(speed),
                static_cast<unsigned>(duplex));
    if (autonegotiation != expectedAutonegotiation) error = ESP_ERR_INVALID_STATE;
  }
  std::printf("SDK cached getter check: %s\n", esp_err_to_name(error));
  return error;
}

void releaseBeforeInstall() {
  // Installed resources must never reach this path. Failed deletions retain
  // ownership; no later operation uses the associated borrowed context.
  if (fixture.mac) {
    const esp_err_t error = fixture.mac->del(fixture.mac);
    std::printf("MAC delete before install: %s\n", esp_err_to_name(error));
    if (error != ESP_OK) return;
    fixture.mac = nullptr;
  }
  if (fixture.phy) {
    const esp_err_t error = fixture.phy->destroy();
    std::printf("PHY delete before install: %s\n", esp_err_to_name(error));
    if (error != ESP_OK) return;
    fixture.phy = nullptr;
  }
  if (fixture.bus.mutex) {
    vSemaphoreDelete(fixture.bus.mutex);
    fixture.bus.mutex = nullptr;
  }
}

void runFixture() {
  esp_err_t error = esp_event_loop_create_default();
  if (error != ESP_OK) {
    std::printf("Event loop: %s\n", esp_err_to_name(error));
    return;
  }
  fixture.bus.mutex = xSemaphoreCreateMutex();
  if (!fixture.bus.mutex) return;
  eth_mac_config_t macConfig = ETH_MAC_DEFAULT_CONFIG();
  eth_esp32_emac_config_t emacConfig = ETH_ESP32_EMAC_DEFAULT_CONFIG();
  emacConfig.smi_gpio.mdc_num = 40;
  emacConfig.smi_gpio.mdio_num = 39;
  emacConfig.clock_config.rmii.clock_mode = EMAC_CLK_EXT_IN;
  emacConfig.clock_config.rmii.clock_gpio = 44;
  emacConfig.emac_dataif_gpio.rmii = {49, 41, 42, 51, 52, 53};
  fixture.mac = esp_eth_mac_new_esp32(&emacConfig, &macConfig);
  if (!fixture.mac) { releaseBeforeInstall(); return; }

  PhyConfig config{};
  config.core.variant = Variant::DP83826I;
  config.core.interface = MacInterface::RMII;
  config.core.requiredMode = OperatingMode::ENHANCED;
  config.core.phyAddress = 0;
  // Conditional application admission values, UNMEASURED. SDK busy-loop
  // limits do not prove these scheduling-inclusive bounds; see README.md.
  config.core.transport.bounds = {2000, 1500, 1};
  config.notificationUs = 101000; // Conditional per-mediator-call bound; unmeasured.
  config.callbackBudgetUs = 1000000;
  config.mac = fixture.mac;
  config.bus = &fixture.bus;
  config.pinnedP4Mac = true; // Actual esp_eth_mac_new_esp32 from the pinned SDK.
  config.linkPollMs = 1000;
  error = Phy::create(config, fixture.phy);
  if (error != ESP_OK) {
    std::printf("PHY factory: %s\n", esp_err_to_name(error));
    releaseBeforeInstall(); return;
  }

  BoardControl board{};
  board.setReset = setReset;
  board.powerOnAtUs = 0; // Conservative boot origin, after actual power-on.
  // Enabling the guard attests valid continuous PHY/MAC clocks at this sample.
  board.clockStableAtUs = nowUs();
  board.assertionUs = 1000; // Application policy; qualify actual R21/C24 waveform.
  error = fixture.phy->prepareBoard(board, nowUs() + 1000000U);
  if (error == ESP_OK) error = fixture.phy->install(fixture.ethernet);
  if (error != ESP_OK) {
    std::printf("Readiness/install: %s\n", esp_err_to_name(error));
    releaseBeforeInstall(); return;
  }
  // From this point all resources and callback contexts remain alive until restart.
  error = esp_eth_update_input_path(fixture.ethernet, dropFrame, &fixture);
  OperationToken token{};
  Status status = Status::Ok();
  if (error == ESP_OK) {
    InitializeOptions initialize{};
    initialize.reset = ResetKind::NONE;
    status = fixture.phy->startInitialize(initialize, operationBudget(), token);
    if (status.inProgress()) status = finishOperation(token);
    showStatus("Initialize", status);
  }
  if (error == ESP_OK && status.ok()) {
    LinkSettings settings{};
    settings.advertisement.technologyMask = LINK_100_FULL;
    status = fixture.phy->startConfigure(settings, operationBudget(), token);
    if (status.inProgress()) status = finishOperation(token);
    showStatus("Configure 100/full AN", status);
  }
  if (error == ESP_OK && status.ok()) {
    status = fixture.phy->reconcileSdkState(nowUs() + 1000000U);
    showStatus("Reconcile AN SDK state", status);
  }
  if (error == ESP_OK && status.ok()) {
    // Both pause advertisement bits were already verified clear above. This
    // typed owner route avoids the pinned ioctl's bool/uint32_t width defect.
    error = fixture.phy->setFlowControl(false, 0U, nowUs() + 1000000U);
    std::printf("Flow control disabled, pause ability clear: %s\n", esp_err_to_name(error));
    FlowControlRecord flow{};
    const Status copied = fixture.phy->flowControlSnapshot(flow);
    if (copied.ok()) {
      std::printf("Flow receipt: MAC attempted=%u accepted=%u possible=%u error=%s; "
                  "PHY attempted=%u accepted=%u error=%s; reconciled=%u\n",
                  static_cast<unsigned>(flow.macAttempted), static_cast<unsigned>(flow.macAccepted),
                  static_cast<unsigned>(flow.macEffectPossible), esp_err_to_name(flow.macError),
                  static_cast<unsigned>(flow.phyAttempted), static_cast<unsigned>(flow.phyAccepted),
                  esp_err_to_name(flow.phyError),
                  static_cast<unsigned>(flow.reconciled));
    } else showStatus("Flow receipt snapshot", copied);
  }
  if (error == ESP_OK && status.ok()) {
    uint16_t configuration = 0;
    status = fixture.phy->readMmd(31U, 0x0018U, configuration,
                                  {nowUs() + 1000000U, 4U});
    showStatus("Owner MMD31:0x0018 read", status);
    if (status.ok()) std::printf("MMD value=0x%04X\n", static_cast<unsigned>(configuration));
  }
  if (error == ESP_OK && status.ok()) error = showSdkCachedSettings(true);
  if (error == ESP_OK && status.ok()) {
    error = fixture.phy->start(nowUs() + 1000000U);
    std::printf("AN framework start: %s\n", esp_err_to_name(error));
    if (error == ESP_OK) vTaskDelay(pdMS_TO_TICKS(100));
  }
  if (error == ESP_OK && status.ok()) {
    error = fixture.phy->stop(nowUs() + 1000000U);
    std::printf("AN framework stop: %s\n", esp_err_to_name(error));
  }
  if (error == ESP_OK && status.ok()) {
    LinkSettings settings{};
    settings.mode = LinkMode::FORCED;
    settings.forced = {LinkSpeed::MBPS_10, Duplex::HALF};
    settings.crossover.pairSelection = PairSelection::MDI;
    status = fixture.phy->startConfigure(settings, operationBudget(), token);
    if (status.inProgress()) status = finishOperation(token);
    showStatus("Configure forced 10/half", status);
  }
  if (error == ESP_OK && status.ok()) {
    status = fixture.phy->reconcileSdkState(nowUs() + 1000000U);
    showStatus("Reconcile forced SDK state", status);
  }
  if (error == ESP_OK && status.ok()) error = showSdkCachedSettings(false);
  if (error == ESP_OK && status.ok()) {
    error = fixture.phy->start(nowUs() + 1000000U);
    std::printf("Forced framework start: %s\n", esp_err_to_name(error));
    if (error == ESP_OK) vTaskDelay(pdMS_TO_TICKS(100));
  }
  std::printf("RX/configuration/start route: %s\n", esp_err_to_name(error));
  showStatus("Final core configuration route", status);
  error = fixture.phy->retire(nowUs() + 1000000U);
  std::printf("Retire: %s; reusable teardown remains gated\n", esp_err_to_name(error));
  LifecycleRecord record{};
  status = fixture.phy->lifecycleSnapshot(record);
  if (status.ok()) {
    showStatus("Lifecycle primary", record.firstError);
    showStatus("Lifecycle cleanup", record.cleanupError);
  }
}
} // namespace

extern "C" void app_main(void) {
  std::printf("DP8382x adapter link fixture: SDK=%s target=%s git=%s tree=%s\n",
              esp_get_idf_version(), CONFIG_IDF_TARGET, FIXTURE_GIT_COMMIT, FIXTURE_GIT_TREE);
  if (runBoardFixture) runFixture();
  else std::puts("Board execution disabled; linked-image evidence only; physical qualification NOT RUN.");
}
