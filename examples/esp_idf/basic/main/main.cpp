/// @file main.cpp
/// @brief One application owner for the pinned ESP32-P4 MAC, PHY and IP stack.
#include "BoardConfig.h"
#ifndef DP8382X_EXAMPLE_TEST
#include "ExampleIdentity.h"
#endif
#include <DP8382x/Version.h>
#include <DP8382x/esp_idf/Phy.h>
#include "driver/gpio.h"
#include "esp_eth_mac_esp.h"
#include "esp_eth_netif_glue.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"
#include "esp_system.h"
#include "freertos/task.h"
#include <atomic>
#include <cstdio>

namespace {
using namespace ::DP8382x;
using namespace ::DP8382x::EspIdf;

/// Static callback context survives every installed SDK resource until restart.
struct Application {
  SharedBus bus{};
  Phy* phy{nullptr};
  esp_eth_mac_t* mac{nullptr};
  esp_eth_handle_t driver{nullptr};
  esp_netif_t* netif{nullptr};
  esp_eth_netif_glue_handle_t glue{nullptr};
  esp_event_handler_instance_t ethHandler{nullptr};
  esp_event_handler_instance_t ipHandler{nullptr};
  esp_event_handler_instance_t lostIpHandler{nullptr};
  bool loopOwned{false};
  bool installed{false};
  std::atomic<bool> linked{false};
  std::atomic<bool> shutdownRequested{false};
  std::atomic<bool> startDesired{false};
  std::atomic<unsigned> expectedStops{0};
  std::atomic<uint32_t> ipAddress{0};
  std::atomic<uint32_t> events{0};
  NativeStatus primary{};
  NativeStatus cleanup{};
};
Application app;

uint64_t nowUs() { return static_cast<uint64_t>(esp_timer_get_time()); }
uint64_t deadlineUs() { return nowUs() + 1000000U; }

void fail(esp_err_t error) {
  if (error != ESP_OK && app.primary.ok()) app.primary.sdkError = error;
}
void fail(Status status) {
  if (!status.ok() && app.primary.ok()) app.primary.coreStatus = status;
}
void cleanupError(esp_err_t error) {
  if (error != ESP_OK && app.cleanup.ok()) app.cleanup.sdkError = error;
}
void retainLifecycle() {
  if (!app.phy) return;
  LifecycleRecord record{};
  if (!app.phy->lifecycleSnapshot(record).ok()) return;
  if (!record.firstError.ok() && app.primary.ok()) app.primary = record.firstError;
  if (!record.cleanupError.ok() && app.cleanup.ok()) app.cleanup = record.cleanupError;
}

/// GPIO latch precedes output enable. Return errors even if the pin effect occurred.
esp_err_t setReset(void*, bool asserted) {
  esp_err_t error = gpio_set_level(static_cast<gpio_num_t>(Board::RESET_N), asserted ? 0 : 1);
  if (error == ESP_OK)
    error = gpio_set_direction(static_cast<gpio_num_t>(Board::RESET_N), GPIO_MODE_OUTPUT);
  return error;
}

/// Event-loop callback copies bounded facts; the application task owns all work.
void onEthernet(void* context, esp_event_base_t, int32_t id, void* data) {
  auto& owner = *static_cast<Application*>(context);
  if (!data || *static_cast<esp_eth_handle_t*>(data) != owner.driver) return;
  owner.events.fetch_add(1, std::memory_order_relaxed);
  if (id == ETHERNET_EVENT_CONNECTED) owner.linked.store(true);
  if (id == ETHERNET_EVENT_DISCONNECTED || id == ETHERNET_EVENT_STOP) {
    owner.linked.store(false);
    owner.ipAddress.store(0);
  }
  if (id == ETHERNET_EVENT_STOP && owner.expectedStops.exchange(0) == 0 &&
      owner.startDesired.load()) owner.shutdownRequested.store(true);
}

/// DHCP outcome is network evidence; it never changes PHY transport health.
void onIp(void* context, esp_event_base_t, int32_t, void* data) {
  auto& owner = *static_cast<Application*>(context);
  if (!data) return;
  const auto* event = static_cast<const ip_event_got_ip_t*>(data);
  if (event->esp_netif != owner.netif) return;
  owner.ipAddress.store(event->ip_info.ip.addr);
  owner.events.fetch_add(1, std::memory_order_relaxed);
}

/// A lost-IP callback only updates cached network evidence.
void onLostIp(void* context, esp_event_base_t, int32_t, void* data) {
  auto& owner = *static_cast<Application*>(context);
  if (!data || static_cast<const ip_event_got_ip_t*>(data)->esp_netif != owner.netif) return;
  owner.ipAddress.store(0);
  owner.events.fetch_add(1, std::memory_order_relaxed);
}

/// Release only pre-install objects. lwIP initialization cannot be undone by IDF.
/// If MAC deletion fails, borrowed PHY/bus storage remains alive.
void releaseBeforeInstall() {
  if (app.mac) {
    const esp_err_t error = app.mac->del(app.mac);
    cleanupError(error);
    if (error == ESP_OK) app.mac = nullptr;
  }
  if (!app.mac && app.phy) {
    const esp_err_t error = app.phy->destroy();
    cleanupError(error);
    if (error == ESP_OK) app.phy = nullptr;
  }
  if (!app.mac && !app.phy && app.bus.mutex) {
    vSemaphoreDelete(app.bus.mutex);
    app.bus.mutex = nullptr;
  }
  if (app.netif) {
    esp_netif_destroy(app.netif);
    app.netif = nullptr;
  }
  if (app.loopOwned) {
    const esp_err_t error = esp_event_loop_delete_default();
    cleanupError(error);
    if (error == ESP_OK) app.loopOwned = false;
  }
}

OperationBudget operationBudget() {
  OperationBudget budget{};
  budget.io = {nowUs() + 5000000U, 2048U};
  budget.pollIntervalUs = 1000;
  budget.maxStalledPolls = 10000;
  return budget;
}

/// Finite owner polling; cancellation still keeps accepted hardware effects.
Status finishOperation(OperationToken token) {
  for (uint32_t poll = 0; poll < 10000U; ++poll) {
    const Status progress = app.phy->pollOperation(token);
    if (!progress.inProgress()) {
      OperationResult result{};
      const Status taken = app.phy->takeResult(token, result);
      return taken.ok() ? result.status : taken;
    }
    vTaskDelay(1);
  }
  const Status cancelled = app.phy->cancel(token);
  OperationResult result{};
  const Status taken = app.phy->takeResult(token, result);
  return !cancelled.ok() ? cancelled : !taken.ok() ? taken : result.status;
}

/// Construct and configure all resources before explicit framework start.
bool initialize() {
  esp_err_t error = esp_netif_init();
  if (error != ESP_OK) { fail(error); return false; }
  error = esp_event_loop_create_default();
  if (error == ESP_OK) app.loopOwned = true;
  else if (error != ESP_ERR_INVALID_STATE) { fail(error); return false; }

  esp_netif_config_t netifConfig = ESP_NETIF_DEFAULT_ETH(); // DHCP is application policy.
  app.netif = esp_netif_new(&netifConfig);
  if (!app.netif) { fail(ESP_ERR_NO_MEM); return false; }
  app.bus.mutex = xSemaphoreCreateMutex();
  if (!app.bus.mutex) { fail(ESP_ERR_NO_MEM); return false; }

  eth_mac_config_t macConfig = ETH_MAC_DEFAULT_CONFIG();
  eth_esp32_emac_config_t emacConfig = ETH_ESP32_EMAC_DEFAULT_CONFIG();
  Board::apply(emacConfig);
  if (!Board::valid(emacConfig)) { fail(ESP_ERR_INVALID_ARG); return false; }
  app.mac = esp_eth_mac_new_esp32(&emacConfig, &macConfig);
  if (!app.mac) { fail(ESP_ERR_NO_MEM); return false; }

  PhyConfig config{};
  config.core.variant = Variant::DP83826I;
  config.core.interface = MacInterface::RMII;
  config.core.requiredMode = OperatingMode::ENHANCED;
  config.core.phyAddress = Board::PHY_ADDRESS;
  // Conditional application admission values, not measured wall-time promises.
  config.core.transport.bounds = {2000, 1500, 1};
  config.notificationUs = 101000;
  config.callbackBudgetUs = 1000000;
  config.mac = app.mac;
  config.bus = &app.bus;
  config.pinnedP4Mac = true;
  config.linkPollMs = Board::LINK_POLL_MS;
  error = Phy::create(config, app.phy);
  if (error != ESP_OK) { fail(error); return false; }

  BoardControl board{};
  board.setReset = setReset;
  // The board gate attests both conditions at this sample. Waiting from a
  // later sample is conservative when power/clock were already ready earlier.
  const uint64_t readySample = nowUs();
  board.powerOnAtUs = readySample;
  board.clockStableAtUs = readySample;
  board.assertionUs = 1000; // R21/C24 pulse must be measured before enabling board gate.
  error = app.phy->prepareBoard(board, deadlineUs());
  if (error == ESP_OK) error = app.phy->install(app.driver);
  if (error != ESP_OK) { fail(error); return false; }
  app.installed = true;

  app.glue = esp_eth_new_netif_glue(app.driver);
  if (!app.glue) { fail(ESP_ERR_NO_MEM); return false; }
  // A failed attach may already have published netif to the driver RX callback.
  error = esp_netif_attach(app.netif, app.glue);
  if (error != ESP_OK) { fail(error); return false; }
  error = esp_event_handler_instance_register(ETH_EVENT, ESP_EVENT_ANY_ID,
                                               onEthernet, &app, &app.ethHandler);
  if (error != ESP_OK) { fail(error); return false; }
  error = esp_event_handler_instance_register(IP_EVENT, IP_EVENT_ETH_GOT_IP,
                                               onIp, &app, &app.ipHandler);
  if (error != ESP_OK) { fail(error); return false; }
  error = esp_event_handler_instance_register(IP_EVENT, IP_EVENT_ETH_LOST_IP,
                                               onLostIp, &app, &app.lostIpHandler);
  if (error != ESP_OK) { fail(error); return false; }

  OperationToken token{};
  InitializeOptions init{};
  init.reset = ResetKind::NONE;
  Status status = app.phy->startInitialize(init, operationBudget(), token);
  if (status.inProgress()) status = finishOperation(token);
  if (!status.ok()) { fail(status); return false; }

  LinkSettings settings{};
  settings.advertisement.technologyMask = LINK_100_FULL;
  status = app.phy->startConfigure(settings, operationBudget(), token);
  if (status.inProgress()) status = finishOperation(token);
  if (!status.ok()) { fail(status); return false; }
  const NativeStatus reconciliation = app.phy->reconcileSdkState(deadlineUs());
  if (!reconciliation.ok()) { app.primary = reconciliation; return false; }
  error = app.phy->setFlowControl(false, 0U, deadlineUs());
  if (error != ESP_OK) { fail(error); return false; }
  return true;
}

/// Print separate cached PHY, profile and IP evidence on the owner task.
void report() {
  HealthSnapshot health{};
  SettingsSnapshot settings{};
  LinkSnapshot link{};
  LifecycleRecord life{};
  const Status h = app.phy->healthSnapshot(health);
  const Status s = app.phy->settingsSnapshot(settings);
  const Status l = app.phy->linkSnapshot(link);
  const Status n = app.phy->lifecycleSnapshot(life);
  const bool currentLink = l.ok() && link.valid && !link.stale;
  const bool resolved = currentLink && link.linkUp && link.resolvedModeValid;
  const bool profile = resolved && link.speed == LinkSpeed::MBPS_100 && link.duplex == Duplex::FULL;
  const char* linkEvidence = !l.ok() ? "BUSY" : !link.valid ? "UNKNOWN" :
      link.stale ? "STALE" : "CURRENT";
  const char* trust = !s.ok() ? "BUSY" :
      settings.trust == ConfigurationTrust::VERIFIED ? "VERIFIED" :
      settings.trust == ConfigurationTrust::DIRTY ? "DIRTY" : "UNKNOWN";
  const uint32_t ip = app.ipAddress.load();
  std::printf("MDIO=%s configTrust=%s linkEvidence=%s link=%u eventCarrier=%u resolved=%u speed=%u duplex=%u "
              "profile100full=%u IP=%s %u.%u.%u.%u reachability=not-requested "
              "events=%lu nativePhase=%u\n",
              h.ok() ? driverStateName(health.state) : "BUSY",
              trust, linkEvidence,
              static_cast<unsigned>(currentLink && link.linkUp),
              static_cast<unsigned>(app.linked.load()),
              static_cast<unsigned>(resolved),
              resolved ? static_cast<unsigned>(link.speed) : 0U,
              resolved ? static_cast<unsigned>(link.duplex) : 0U,
              static_cast<unsigned>(profile), ip != 0U ? "ready" : "pending/lost",
              static_cast<unsigned>(ip & 0xffU), static_cast<unsigned>((ip >> 8U) & 0xffU),
              static_cast<unsigned>((ip >> 16U) & 0xffU), static_cast<unsigned>(ip >> 24U),
              static_cast<unsigned long>(app.events.load()),
              n.ok() ? static_cast<unsigned>(life.phase) : 0U);
}

/// A bounded, console-independent stop/start cycle exercises the single owner.
void serve() {
  app.startDesired.store(true);
  esp_err_t error = app.phy->start(deadlineUs());
  retainLifecycle();
  if (error != ESP_OK) { fail(error); return; }
  for (unsigned cycle = 0; cycle < 2 && !app.shutdownRequested.load(); ++cycle) {
    const unsigned ticks = cycle == 0 ? 100U : 200U;
    for (unsigned step = 0; step < ticks && !app.shutdownRequested.load(); ++step) {
      vTaskDelay(pdMS_TO_TICKS(100));
      if (step % 10U == 0U) report();
    }
    if (cycle == 0 && !app.shutdownRequested.load()) {
      app.startDesired.store(false);
      // STOP is queued. Keep this receipt until callback delivery, even if it
      // arrives after the next START; current desired state is not its origin.
      app.expectedStops.store(1);
      error = app.phy->stop(deadlineUs());
      retainLifecycle();
      if (error != ESP_OK) { fail(error); break; }
      vTaskDelay(pdMS_TO_TICKS(1000));
      app.startDesired.store(true);
      error = app.phy->start(deadlineUs());
      retainLifecycle();
      if (error != ESP_OK) { fail(error); break; }
    }
  }
  app.startDesired.store(false);
}

/// Installed objects, including netif/glue/handlers, remain alive until restart.
/// Callback context is static; no event callback waits on its own unregister.
void finish() {
  if (app.installed) {
    const esp_err_t error = app.phy->retire(deadlineUs());
    retainLifecycle();
    if (error != ESP_ERR_NOT_SUPPORTED) cleanupError(error);
  } else {
    releaseBeforeInstall();
  }
  std::printf("primary core=%s detail=%ld SDK=%s cleanup core=%s detail=%ld SDK=%s; installed resources %s\n",
              errorName(app.primary.coreStatus.code),
              static_cast<long>(app.primary.coreStatus.detail),
              esp_err_to_name(app.primary.sdkError),
              errorName(app.cleanup.coreStatus.code),
              static_cast<long>(app.cleanup.coreStatus.detail),
              esp_err_to_name(app.cleanup.sdkError),
              app.installed ? "retained until restart" : "released where safe");
}

/// @brief Called only after application/board authorization; never by a handler.
void run(bool boardReady) {
  if (!boardReady) {
    std::puts("Board gate closed: verify PHY 3V3 supply, netlist, straps, clocks and reset waveform before enabling.");
    return;
  }
  if (initialize()) serve();
  finish();
}
} // namespace

#ifndef DP8382X_EXAMPLE_TEST
extern "C" void app_main(void) {
  std::printf("DP8382x %s source=%s tree=%s SDK=%s target=%s "
              "P4 RMII MDC=%d MDIO=%d REF_CLK=%d TX_EN=%d TXD0=%d TXD1=%d "
              "CRS_DV=%d RXD0=%d RXD1=%d reset=%d PHY-address=%lu "
              "poll=%lu-ms DHCP; profile=100/full; peer-probe=off\n",
              ::DP8382x::VERSION_STRING, EXAMPLE_GIT_COMMIT, EXAMPLE_GIT_TREE,
              esp_get_idf_version(), CONFIG_IDF_TARGET,
              Board::MDC, Board::MDIO, Board::REF_CLK, Board::TX_EN,
              Board::TXD0, Board::TXD1, Board::CRS_DV, Board::RXD0,
              Board::RXD1, Board::RESET_N,
              static_cast<unsigned long>(Board::PHY_ADDRESS),
              static_cast<unsigned long>(Board::LINK_POLL_MS));
#ifdef CONFIG_DP8382X_BOARD_READY
  run(true);
#else
  run(false);
#endif
}
#endif
