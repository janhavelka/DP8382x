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
#include <array>
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
  std::atomic<unsigned> expectedStops{0};
  std::atomic<uint32_t> ipAddress{0};
  std::atomic<uint32_t> events{0};
  NativeStatus primary{};
  NativeStatus cleanup{};
  const char* failedStage{nullptr}; ///< A null SDK factory has a stage but supplies no cause.
  const char* failedAdapterCall{nullptr}; ///< First failing direct adapter call, independent of the cycle journal.
  esp_err_t adapterCallError{ESP_OK};
  Status journalCopyError{}; ///< First exhausted journal confirmation.
  Status stopObservation{Err::RESULT_NOT_AVAILABLE, 0, "No observation ended service"};
  const char* sessionEnd{"not started"}; ///< Why serve() ended its traffic session.
  bool resultOwed{false}; ///< An admitted core result could not be delivered; CONTROL remains.
  OperationToken owedToken{}; ///< Retained undelivered token, valid while resultOwed.
  Status owedTerminal{Err::RESULT_NOT_AVAILABLE, 0, "Terminal operation status unavailable"};
  OperationResult lastResult{}; ///< Last delivered operation, including possible effects.
  unsigned eventRetries{0}; ///< Consecutive owner retries of one lost link event.
};
Application app;

/// Finite owner allowances. Counts bound loops even if the clock stalls.
constexpr uint32_t MAX_OWNER_POLLS = 10000;   ///< Operation progress polls.
constexpr uint32_t MAX_DELIVERY_TRIES = 100;  ///< Cancel/take attempts on gate contention.
constexpr unsigned MAX_JOURNAL_TRIES = 3; ///< Two single-tick yields between three cached reads.
constexpr unsigned MAX_EVENT_RETRIES = 10;    ///< Lost link-event retries before owner stop.
constexpr uint64_t OPERATION_WAIT_US = 6000000; ///< Owner wait over the 5 s core I/O budget.
constexpr uint32_t CLOCK_LAG_US = 1; ///< Conservative esp_timer timestamp allowance.

uint64_t nowUs() { return static_cast<uint64_t>(esp_timer_get_time()); }
uint64_t deadlineUs() { return nowUs() + 1000000U; }

/// Record the first failing step. Later failures never replace the first cause.
void fail(const char* stage, esp_err_t error) {
  if (error == ESP_OK || app.failedStage) return;
  app.primary.sdkError = error; app.failedStage = stage;
}
void fail(const char* stage, Status status) {
  if (status.ok() || app.failedStage) return;
  app.primary.coreStatus = status; app.failedStage = stage;
}
/// A null SDK factory result reports no cause; do not invent an exact SDK code.
void failNull(const char* stage) {
  if (!app.failedStage) app.failedStage = stage;
}
void cleanupError(esp_err_t error) {
  if (error != ESP_OK && app.cleanup.ok()) app.cleanup.sdkError = error;
}
void cleanupError(Status status) {
  if (!status.ok() && app.cleanup.ok()) app.cleanup.coreStatus = status;
}
/// The journal stores the first error in a cycle, without its originating step.
void retainLifecycle(const LifecycleRecord& record) {
  if (!record.firstError.ok() && !app.failedStage) {
    app.primary = record.firstError;
    app.failedStage = "adapter cycle journal";
  }
  if (!record.cleanupError.ok() && app.cleanup.ok()) app.cleanup = record.cleanupError;
}
/// Confirm delivery before a later start can clear the previous cycle's journal.
bool copyLifecycle() {
  LifecycleRecord record{};
  for (unsigned attempt = 1; ; ++attempt) {
    const Status status = app.phy->lifecycleSnapshot(record);
    if (status.ok()) { retainLifecycle(record); return true; }
    if (!status.is(Err::BUSY) || attempt == MAX_JOURNAL_TRIES) {
      if (app.journalCopyError.ok()) app.journalCopyError = status;
      return false;
    }
    vTaskDelay(1);
  }
}
/// Retain the direct SDK receipt even if a historical journal error predates it.
void checkAdapter(const char* stage, esp_err_t error) {
  if (error != ESP_OK && !app.failedAdapterCall) {
    app.failedAdapterCall = stage; app.adapterCallError = error;
  }
  if (copyLifecycle()) fail(stage, error);
}

const char* phaseName(Phase phase) {
  switch (phase) {
    case Phase::STOPPED: return "STOPPED";
    case Phase::CONTROL: return "CONTROL";
    case Phase::STARTING: return "STARTING";
    case Phase::RUNNING: return "RUNNING";
    case Phase::STOPPING: return "STOPPING";
    case Phase::MAINTENANCE: return "MAINTENANCE";
  }
  return "UNKNOWN";
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
  if (id == ETHERNET_EVENT_STOP) {
    // The single event loop consumes each queued owner-stop announcement once.
    if (owner.expectedStops.load()) owner.expectedStops.fetch_sub(1);
    else owner.shutdownRequested.store(true);
  }
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
/// The never-installed PHY borrows the MAC and bus, so it goes first; if its
/// destruction fails, the MAC and mutex it references stay alive. A quarantined
/// bus may hold an unreleased mutex, so the mutex is then retained as well.
/// Destruction deletes the adapter journal, so an unconfirmed final copy keeps
/// the PHY and its dependencies alive for an explicit retry.
void releaseBeforeInstall() {
  if (app.phy && copyLifecycle()) {
    const esp_err_t error = app.phy->destroy();
    cleanupError(error);
    if (error == ESP_OK) app.phy = nullptr;
  }
  if (!app.phy && app.mac) {
    const esp_err_t error = app.mac->del(app.mac);
    cleanupError(error);
    if (error == ESP_OK) app.mac = nullptr;
  }
  if (!app.phy && app.bus.mutex && !app.bus.quarantined.load()) {
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

/// True when the adapter holds this token's frozen result awaiting delivery.
bool resultPending(OperationToken token, Status& terminal) {
  OperationSnapshot snapshot{};
  if (!app.phy->operationSnapshot(snapshot).ok() || snapshot.token.value != token.value ||
      snapshot.state != OperationState::RESULT_PENDING) return false;
  terminal = snapshot.status;
  return true;
}

/// Finite owner polling and delivery. BUSY from the instance gate or controller
/// is retried, never mistaken for a terminal result. Only a successful take
/// releases CONTROL and owned bus silence, so an undelivered token is retained
/// and reported. Cancellation keeps accepted hardware effects.
Status finishOperation(OperationToken token) {
  const uint64_t until = nowUs() + OPERATION_WAIT_US;
  bool terminal = false;
  Status progress{};
  for (uint32_t poll = 0; poll < MAX_OWNER_POLLS && !terminal && nowUs() < until; ++poll) {
    progress = app.phy->pollOperation(token);
    terminal = !progress.inProgress() && (!progress.is(Err::BUSY) || resultPending(token, progress));
    if (!terminal) vTaskDelay(1);
  }
  Status outcome = Status::Error(Err::OPERATION_TIMEOUT, "Example owner operation wait expired");
  if (!terminal) {
    Status cancelled = Status::Error(Err::BUSY, "Cancel not attempted");
    for (uint32_t i = 0; i < MAX_DELIVERY_TRIES && cancelled.is(Err::BUSY); ++i) {
      cancelled = app.phy->cancel(token);
      if (cancelled.is(Err::BUSY)) vTaskDelay(1);
    }
    if (!cancelled.ok()) cleanupError(cancelled);
  }
  Status taken = Status::Error(Err::BUSY, "Take not attempted");
  for (uint32_t i = 0; i < MAX_DELIVERY_TRIES && taken.is(Err::BUSY); ++i) {
    taken = app.phy->takeResult(token, app.lastResult);
    if (taken.is(Err::BUSY)) vTaskDelay(1);
  }
  if (!taken.ok()) {
    app.resultOwed = true; app.owedToken = token;
    if (terminal) app.owedTerminal = progress;
    cleanupError(taken);
    if (!terminal) return outcome;
    return progress.ok() ? taken : progress;
  }
  app.resultOwed = false;
  app.owedToken = {};
  return terminal ? app.lastResult.status : outcome;
}

/// Construct and configure all resources before explicit framework start.
bool initialize() {
  esp_err_t error = esp_netif_init();
  if (error != ESP_OK) { fail("netif init", error); return false; }
  error = esp_event_loop_create_default();
  if (error == ESP_OK) app.loopOwned = true;
  else if (error != ESP_ERR_INVALID_STATE) { fail("event loop", error); return false; }

  esp_netif_config_t netifConfig = ESP_NETIF_DEFAULT_ETH(); // DHCP is application policy.
  app.netif = esp_netif_new(&netifConfig);
  if (!app.netif) { failNull("netif new"); return false; }
  // Dynamic mutex creation fails only on allocation.
  app.bus.mutex = xSemaphoreCreateMutex();
  if (!app.bus.mutex) { fail("bus mutex", ESP_ERR_NO_MEM); return false; }

  eth_mac_config_t macConfig = ETH_MAC_DEFAULT_CONFIG();
  eth_esp32_emac_config_t emacConfig = ETH_ESP32_EMAC_DEFAULT_CONFIG();
  Board::apply(emacConfig);
  if (!Board::valid(emacConfig)) { fail("board config", ESP_ERR_INVALID_ARG); return false; }
  app.mac = esp_eth_mac_new_esp32(&emacConfig, &macConfig);
  if (!app.mac) { failNull("MAC new"); return false; }

  PhyConfig config{};
  config.core.variant = Variant::DP83826I;
  config.core.interface = MacInterface::RMII;
  config.core.requiredMode = OperatingMode::ENHANCED;
  config.core.phyAddress = Board::PHY_ADDRESS;
  // Conditional application admission values, not measured wall-time promises.
  config.core.transport.bounds = {2000, 1500, CLOCK_LAG_US};
  config.notificationUs = 101000;
  config.callbackBudgetUs = 1000000;
  // A failed start after UP needs more than one notification allowance to withdraw it.
  config.cleanupBudgetUs = 500000;
  config.mac = app.mac;
  config.bus = &app.bus;
  config.pinnedP4Mac = true;
  config.linkPollMs = Board::LINK_POLL_MS;
  config.waitForNegotiatedLink = true;
  error = Phy::create(config, app.phy);
  if (error != ESP_OK) { fail("PHY create", error); return false; }

  BoardControl board{};
  board.setReset = setReset;
  // The board gate attests both conditions at this sample. Waiting from a
  // later sample is conservative when power/clock were already ready earlier.
  const uint64_t readySample = nowUs();
  board.powerOnAtUs = readySample;
  board.clockStableAtUs = readySample;
  board.assertionUs = 1000; // R21/C24 pulse must be measured before enabling board gate.
  error = app.phy->prepareBoard(board, deadlineUs());
  if (error != ESP_OK) { checkAdapter("board preparation", error); return false; }
  error = app.phy->install(app.driver);
  if (error != ESP_OK) { checkAdapter("driver install", error); return false; }
  app.installed = true;

  // Pinned post_attach ignores its MAC read and netif address assignment errors.
  // Establish the MAC address and verify the attached netif before starting DHCP.
  std::array<uint8_t, 6> macAddress{}, netifAddress{};
  error = esp_eth_ioctl(app.driver, ETH_CMD_G_MAC_ADDR, macAddress.data());
  if (error != ESP_OK) { fail("EMAC address", error); return false; }
  app.glue = esp_eth_new_netif_glue(app.driver);
  if (!app.glue) { failNull("netif glue"); return false; }
  // A failed attach may already have published netif to the driver RX callback.
  error = esp_netif_attach(app.netif, app.glue);
  if (error != ESP_OK) { fail("netif attach", error); return false; }
  error = esp_netif_get_mac(app.netif, netifAddress.data());
  if (error != ESP_OK) { fail("netif address readback", error); return false; }
  if (netifAddress != macAddress) {
    fail("netif MAC mismatch", Status::Error(Err::CONFIG_MISMATCH, "Netif and EMAC addresses differ"));
    return false;
  }
  error = esp_event_handler_instance_register(ETH_EVENT, ESP_EVENT_ANY_ID,
                                               onEthernet, &app, &app.ethHandler);
  if (error != ESP_OK) { fail("Ethernet handler", error); return false; }
  error = esp_event_handler_instance_register(IP_EVENT, IP_EVENT_ETH_GOT_IP,
                                               onIp, &app, &app.ipHandler);
  if (error != ESP_OK) { fail("IP handler", error); return false; }
  error = esp_event_handler_instance_register(IP_EVENT, IP_EVENT_ETH_LOST_IP,
                                               onLostIp, &app, &app.lostIpHandler);
  if (error != ESP_OK) { fail("lost-IP handler", error); return false; }

  OperationToken token{};
  InitializeOptions init{};
  init.reset = ResetKind::NONE;
  Status status = app.phy->startInitialize(init, operationBudget(), token);
  if (status.inProgress()) status = finishOperation(token);
  if (!status.ok()) { fail("PHY initialize", status); return false; }

  LinkSettings settings{};
  settings.advertisement.technologyMask = LINK_100_FULL;
  status = app.phy->startConfigure(settings, operationBudget(), token);
  if (status.inProgress()) status = finishOperation(token);
  if (!status.ok()) { fail("PHY configure", status); return false; }
  const NativeStatus reconciliation = app.phy->reconcileSdkState(deadlineUs());
  if (!reconciliation.ok()) {
    if (!app.failedStage) { app.primary = reconciliation; app.failedStage = "SDK reconciliation"; }
    return false;
  }
  error = app.phy->setFlowControl(false, 0U, deadlineUs());
  if (error != ESP_OK) { checkAdapter("flow control", error); return false; }
  return true;
}

/// A copied snapshot is not refreshed by copying, and gate-denied polls can
/// leave an old tuple in place. This application treats evidence older than
/// three of its own poll periods as AGED and withholds the profile verdict.
constexpr uint64_t MAX_LINK_AGE_US = uint64_t{3} * Board::LINK_POLL_MS * 1000U;

/// Age of a link sample from its acquisition start; false when time is unusable.
bool linkAge(const LinkSnapshot& link, uint64_t upperNowUs, uint64_t& ageUs) {
  if (!link.timeValid || link.observedSinceUs > link.observedAtUs || link.observedAtUs > upperNowUs)
    return false;
  ageUs = upperNowUs - link.observedSinceUs;
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
  uint64_t ageUs = 0;
  const bool timed = l.ok() && linkAge(link, nowUs() + CLOCK_LAG_US, ageUs);
  const bool fresh = timed && ageUs <= MAX_LINK_AGE_US;
  const bool currentLink = l.ok() && link.valid && !link.stale && fresh;
  const bool resolved = currentLink && link.linkUp && link.resolvedModeValid;
  const char* profile = !resolved ? "unknown" :
      link.speed == LinkSpeed::MBPS_100 && link.duplex == Duplex::FULL ? "yes" : "no";
  const char* linkEvidence = !l.ok() ? "BUSY" : !link.valid ? "UNKNOWN" :
      link.stale ? "STALE" : !fresh ? "AGED" : "CURRENT";
  const char* trust = !s.ok() ? "BUSY" :
      settings.trust == ConfigurationTrust::VERIFIED ? "VERIFIED" :
      settings.trust == ConfigurationTrust::DIRTY ? "DIRTY" : "UNKNOWN";
  const uint32_t ip = app.ipAddress.load();
  std::printf("MDIO=%s configTrust=%s linkEvidence=%s linkAgeMs=%lu link=%u eventCarrier=%u resolved=%u speed=%u duplex=%u "
              "profile100full=%s IP=%s %u.%u.%u.%u reachability=not-requested "
              "events=%lu nativePhase=%s stopRequired=%s eventLost=%s\n",
              h.ok() ? driverStateName(health.state) : "BUSY",
              trust, linkEvidence,
              static_cast<unsigned long>(timed ? ageUs / 1000U : 0U),
              static_cast<unsigned>(currentLink && link.linkUp),
              static_cast<unsigned>(app.linked.load()),
              static_cast<unsigned>(resolved),
              resolved ? static_cast<unsigned>(link.speed) : 0U,
              resolved ? static_cast<unsigned>(link.duplex) : 0U,
              profile, ip != 0U ? "ready" : "pending/lost",
              static_cast<unsigned>(ip & 0xffU), static_cast<unsigned>((ip >> 8U) & 0xffU),
              static_cast<unsigned>((ip >> 16U) & 0xffU), static_cast<unsigned>(ip >> 24U),
              static_cast<unsigned long>(app.events.load()),
              n.ok() ? phaseName(life.phase) : "BUSY",
              !n.ok() ? "BUSY" : life.stopRequired ? "yes" : "no",
              !n.ok() ? "BUSY" : life.eventLost ? "yes" : "no");
}

/// Service the adapter's cached obligations on the owner task. Copies only; no
/// PHY I/O and no second poller. Returns false when the session must end:
/// the adapter requires an owner stop, or a lost link event stays undelivered.
bool serviceObligations() {
  LifecycleRecord life{};
  if (!app.phy->lifecycleSnapshot(life).ok()) return true; // Gate busy: next step.
  retainLifecycle(life);
  if (life.stopRequired) {
    app.sessionEnd = "adapter requires owner stop";
    app.stopObservation = life.lastObservationStatus;
    fail("running observation", life.lastObservationStatus);
    return false;
  }
  if (!life.eventLost) { app.eventRetries = 0; return true; }
  // Check the receipt on the next turn even for the final attempt: an event
  // accepted late returns TIMEOUT but clears eventLost.
  if (app.eventRetries >= MAX_EVENT_RETRIES) {
    app.sessionEnd = "link event delivery unresolved";
    return false;
  }
  ++app.eventRetries;
  (void)app.phy->retryLinkEvent(nowUs() + 200000U);
  return true;
}

/// Owner stop that keeps the queued STOP receipt until its callback arrives.
esp_err_t ownerStop() {
  // STOP is queued. Keep this receipt until callback delivery, even if it
  // arrives after the next START; current desired state is not its origin.
  app.expectedStops.fetch_add(1);
  const esp_err_t error = app.phy->stop(deadlineUs());
  checkAdapter("owner stop", error);
  return error;
}

/// Run one traffic interval; false when shutdown or an obligation ends the session.
bool runInterval(unsigned steps) {
  for (unsigned step = 0; step < steps; ++step) {
    vTaskDelay(pdMS_TO_TICKS(100));
    if (app.shutdownRequested.load()) { app.sessionEnd = "unexpected framework STOP"; return false; }
    if (!serviceObligations()) return false;
    if (step % 10U == 0U) report();
  }
  return true;
}

/// A bounded, console-independent stop/start cycle exercises the single owner.
/// Lifecycle obligations end the session with an owner stop and no restart.
void serve() {
  // Confirm the journal first: its retries yield, and a STOP delivered during
  // a yield must still withhold the start.
  if (!copyLifecycle()) { app.sessionEnd = "start withheld: journal unavailable"; return; }
  if (app.shutdownRequested.load()) { app.sessionEnd = "start withheld"; return; }
  esp_err_t error = app.phy->start(deadlineUs());
  checkAdapter("start", error);
  if (error != ESP_OK) { app.sessionEnd = "start failed"; return; }
  app.sessionEnd = "running";
  bool active = runInterval(100);
  if (active) {
    if (ownerStop() != ESP_OK) { app.sessionEnd = "owner stop failed"; return; }
    vTaskDelay(pdMS_TO_TICKS(1000));
    // An unexpected STOP delivered while stopped, including during the
    // journal retries, prevents the restart. A completed owner stop has
    // already discharged any stopRequired latch.
    if (!copyLifecycle()) { app.sessionEnd = "restart withheld: journal unavailable"; return; }
    if (app.shutdownRequested.load()) { app.sessionEnd = "restart withheld"; return; }
    error = app.phy->start(deadlineUs());
    checkAdapter("restart", error);
    if (error != ESP_OK) { app.sessionEnd = "restart failed"; return; }
    active = runInterval(200);
    if (active) app.sessionEnd = "demonstration complete";
  }
  if (!active) {
    LifecycleRecord life{};
    if (app.phy->lifecycleSnapshot(life).ok() && life.phase == Phase::RUNNING) (void)ownerStop();
  }
}

/// Installed objects, including netif/glue/handlers, remain alive until restart.
/// Callback context is static; no event callback waits on its own unregister.
void finish() {
  if (app.installed) {
    // An undelivered core result keeps CONTROL, so retirement is refused and
    // reported rather than claimed; the token stays recorded below.
    const esp_err_t error = app.phy->retire(deadlineUs());
    copyLifecycle();
    if (error != ESP_ERR_NOT_SUPPORTED) cleanupError(error);
  } else {
    releaseBeforeInstall();
  }
  // If the terminal journal was inaccessible, expose the exact direct SDK
  // return and name that call. Earlier cycle history stays unknown.
  const char* primaryStage = app.failedStage ? app.failedStage : app.failedAdapterCall;
  const auto primarySdk = app.failedStage ? app.primary.sdkError : app.adapterCallError;
  std::printf("primary stage=%s cause=%s core=%s detail=%ld SDK=%s cleanup core=%s detail=%ld SDK=%s; "
              "session=%s; resultOwed=%s token=%llu; installed resources %s\n",
              primaryStage ? primaryStage : "none",
              !primaryStage ? "none" : app.failedStage && app.primary.ok() ? "unreported" : "reported",
              errorName(app.primary.coreStatus.code),
              static_cast<long>(app.primary.coreStatus.detail),
              esp_err_to_name(primarySdk),
              errorName(app.cleanup.coreStatus.code),
              static_cast<long>(app.cleanup.coreStatus.detail),
              esp_err_to_name(app.cleanup.sdkError),
              app.sessionEnd, app.resultOwed ? "yes" : "no",
              static_cast<unsigned long long>(app.owedToken.value),
              app.installed ? "retained until restart" : "released where safe");
  if (app.failedAdapterCall)
    std::printf("adapter call=%s SDK=%s\n", app.failedAdapterCall, esp_err_to_name(app.adapterCallError));
  if (app.resultOwed)
    std::printf("undelivered operation status=%s detail=%ld\n", errorName(app.owedTerminal.code),
                static_cast<long>(app.owedTerminal.detail));
  if (!app.journalCopyError.ok())
    std::printf("journal unavailable=%s detail=%ld\n", errorName(app.journalCopyError.code),
                static_cast<long>(app.journalCopyError.detail));
  if (!app.stopObservation.is(Err::RESULT_NOT_AVAILABLE))
    std::printf("stop observation=%s detail=%ld\n", errorName(app.stopObservation.code),
                static_cast<long>(app.stopObservation.detail));
  if (app.lastResult.token.value != 0) {
    std::printf("last delivered operation=%llu status=%s possible-write=%u cleanup-owed=%u\n",
                static_cast<unsigned long long>(app.lastResult.token.value),
                errorName(app.lastResult.status.code), static_cast<unsigned>(app.lastResult.mutationPossible),
                static_cast<unsigned>(app.lastResult.cleanupOwed));
  }
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
#ifdef CONFIG_DP8382X_BOARD_READY
static constexpr bool BOARD_GATE = true;
#else
static constexpr bool BOARD_GATE = false;
#endif
/// The volatile read keeps the operational chain in every image, so a default
/// (gate-closed) build still links the complete MAC/PHY/glue/service path.
static volatile bool boardGate = BOARD_GATE;

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
  run(boardGate);
}
#endif
