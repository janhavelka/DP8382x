/// @file main.cpp
/// @brief Single P4 application owner. The SDK timer collects link evidence.
#include "BoardConfig.h"
#include "ExampleIdentity.h"
#include "PhyOwner.h"
#include "RawMac.h"
#include <DP8382x/esp_idf/Phy.h>
#include <DP8382x/Version.h>
#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_system.h"
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>

#if CONFIG_ETH_TRANSMIT_MUTEX || CONFIG_ESP_MM_CACHE_MSYNC_C2M_CHUNKED_OPS || CONFIG_LOG_MAXIMUM_LEVEL > 3
#error "Owner TX under the admission gate requires TX mutex and chunked cache mutex off, maximum logging INFO or lower"
#endif

namespace {
using namespace ::DP8382x;
using namespace ::DP8382x::EspIdf;
using namespace owner_example;

// The installed driver and callback contexts survive until system restart.
SharedBus bus;
Phy* phy{};
esp_eth_mac_t* mac{};
esp_eth_handle_t driver{};
bool eventLoopOwned{};
PhyOwner owner;
FramePort frames;
RawFrame serviceFrame; // One task processes RX, then TX. The SDK borrows it only during TX.
TxCounts txCounts;
uint64_t experimentalRx{}, otherRx{};
// Local experimental EtherType. The source address is filled from the actual MAC.
uint8_t testFrame[60] = {0x02,0,0,0,0,0x2A, 0,0,0,0,0,0, 0x88,0xB5, 'P','4'};
#ifdef CONFIG_DP8382X_ENABLE_TEST_FRAME
static volatile bool testTrafficEnabled = true;
#else
static volatile bool testTrafficEnabled = false;
#endif
bool testFrameQueued{};
uint64_t ticket{};
OperationToken pending{};
OperationResult completed, startupInit, startupConfig, primary, restoration;
bool restoreRequest{};
Status lastAdmission{};
static_assert(Board::LINK_POLL_MS == owner_example::LINK_POLL_MS);

uint64_t nowUs() { return static_cast<uint64_t>(esp_timer_get_time()); }
// esp_timer_get_time has a signed 64-bit range, so adding the 1 us lag fits uint64_t.
uint64_t upperNowUs() { return nowUs() + 1U; }
uint64_t deadline() { return nowUs() + 1500000U; }
OperationBudget budget() { return {{nowUs() + 5000000U, 2048}, 1000, 10000}; }

const char* phaseName(OwnerPhase phase) {
  switch (phase) {
    case OwnerPhase::UNCONFIGURED: return "unconfigured";
    case OwnerPhase::WAITING: return "waiting";
    case OwnerPhase::READY: return "ready";
    case OwnerPhase::ACTIVE: return "active";
    case OwnerPhase::WITHDRAWN: return "withdrawn";
    case OwnerPhase::MAINTENANCE: return "maintenance";
  }
  return "unknown";
}

void show(const char* label, Status status) {
  std::printf("%s %s detail=%ld\n", label, errorName(status.code), static_cast<long>(status.detail));
}
void show(const char* label, const OperationResult& result) {
  std::printf("%s token=%llu status=%s detail=%ld possible-write=%d cleanup-owed=%d restored=%d\n",
      label, static_cast<unsigned long long>(result.token.value), errorName(result.status.code),
      static_cast<long>(result.status.detail), result.mutationPossible, result.cleanupOwed, result.persistentRestored);
}
bool accepted(const char* label, esp_err_t error) {
  if (error != ESP_OK) std::printf("%s SDK error=%s (%ld)\n", label, esp_err_to_name(error), static_cast<long>(error));
  return error == ESP_OK;
}
esp_err_t resetPin(void*, bool asserted) {
  auto pin = static_cast<gpio_num_t>(Board::RESET_N);
  const auto status = gpio_set_level(pin, asserted ? 0 : 1);
  return status == ESP_OK ? gpio_set_direction(pin, GPIO_MODE_OUTPUT) : status;
}

bool install() {
  eth_esp32_emac_config_t boardMac = ETH_ESP32_EMAC_DEFAULT_CONFIG();
  Board::apply(boardMac);
  if (!Board::valid(boardMac)) return false;
  auto error = esp_event_loop_create_default();
  eventLoopOwned = error == ESP_OK;
  if (!eventLoopOwned && !accepted("event loop", error == ESP_ERR_INVALID_STATE ? ESP_OK : error)) return false;
  bus.mutex = xSemaphoreCreateMutex();
  if (!bus.mutex) return false;
  eth_mac_config_t macConfig = ETH_MAC_DEFAULT_CONFIG();
  mac = esp_eth_mac_new_esp32(&boardMac, &macConfig);
  if (!mac) return false;
  PhyConfig config{};
  config.core.variant = Variant::DP83826I;
  config.core.requiredMode = OperatingMode::ENHANCED;
  config.core.interface = MacInterface::RMII;
  config.core.phyAddress = Board::PHY_ADDRESS;
  config.core.transport.bounds = {2000, 1500, 1};
  config.notificationUs = 101000;
  config.callbackBudgetUs = 1000000;
  config.cleanupBudgetUs = 500000;
  config.linkPollMs = Board::LINK_POLL_MS;
  config.waitForNegotiatedLink = true;
  config.mac = mac;
  config.bus = &bus;
  config.pinnedP4Mac = true;
  if (!accepted("factory", Phy::create(config, phy))) return false;
  BoardControl board{};
  board.setReset = resetPin;
  board.powerOnAtUs = board.clockStableAtUs = nowUs(); // Confirmed by board gate.
  board.assertionUs = 1000;
  if (!accepted("board preparation", phy->prepareBoard(board, deadline())) ||
      !accepted("install", phy->install(driver))) return false;
  // Register once, while STOPPED and before first start. No netif/glue owns RX.
  if (!accepted("raw RX registration", esp_eth_update_input_path(driver, receiveRaw, &frames))) return false;
  return !testTrafficEnabled ||
      accepted("test source MAC", esp_eth_ioctl(driver, ETH_CMD_G_MAC_ADDR, &testFrame[6]));
}

void closeResources() {
  frames.setSession(0);
  // This clears application queues if accessible; a late RX still owns a
  // static context. Even an empty application queue cannot fence the SDK task.
  if (!frames.quiesce()) std::puts("Frame queues retained closed during teardown");
  if (driver) {
    if (pending.value != 0) {
      std::printf("Operation result delivery outstanding; retaining installed owner and token=%llu\n",
          static_cast<unsigned long long>(pending.value));
      return;
    }
    const auto error = phy->retire(deadline());
    LifecycleRecord life{};
    const auto sampled = phy->lifecycleSnapshot(life);
    if (sampled.ok())
      std::printf("lifecycle primary=%s/%ld cleanup=%s/%ld\n", errorName(life.firstError.coreStatus.code),
          static_cast<long>(life.firstError.sdkError), errorName(life.cleanupError.coreStatus.code),
          static_cast<long>(life.cleanupError.sdkError));
    std::printf("retire=%s first-tx-error=%ld installed storage retained; outstanding token=%llu\n",
        esp_err_to_name(error), static_cast<long>(txCounts.firstError), static_cast<unsigned long long>(pending.value));
    return;
  }
  if (phy && !accepted("factory cleanup", phy->destroy())) return;
  if (mac && !accepted("MAC cleanup", mac->del(mac))) return;
  if (bus.mutex && !bus.quarantined.load()) vSemaphoreDelete(bus.mutex);
  if (eventLoopOwned) accepted("event loop cleanup", esp_event_loop_delete_default());
}

// One batch per tick; successful take copies even a failed/cancelled result.
// Gate contention after completion leaves the same token owed to the owner.
bool finish(OperationResult& result, bool cancellable) {
  const auto until = nowUs() + 6000000U;
  bool cancelDue = false;
  unsigned cancelAttempts = 0;
  for (unsigned step = 0; step < 6500U; ++step) {
    if (cancellable) {
      char key{};
      if (read(STDIN_FILENO, &key, 1) == 1 && key == 'c') cancelDue = true;
    }
    if (!cancelDue && (nowUs() >= until || step >= 6400U)) {
      std::puts("Owner wait expired; requesting cancellation and retaining the core result");
      cancelDue = true;
    }
    if (cancelDue) {
      if (++cancelAttempts > 100U) break;
      const auto status = phy->cancel(pending);
      if (!status.ok() && !status.is(Err::BUSY)) show("cancel", status);
    } else {
      (void)phy->pollOperation(pending, MAX_BATCH_FRAMES);
    }
    const auto delivered = phy->takeResult(pending, result);
    if (delivered.ok()) { pending = {}; return true; }
    vTaskDelay(1);
  }
  std::printf("result delivery pending token=%llu\n", static_cast<unsigned long long>(pending.value));
  return false;
}

bool runFramework() {
  const auto state = phy->reconcileSdkState(deadline());
  if (!state.ok()) { show("reconciliation", state.coreStatus); accepted("reconciliation", state.sdkError); return false; }
  return accepted("framework start", phy->start(deadline()));
}

void decide(void* action, const LinkSnapshot& link, const SettingsSnapshot& settings,
            const HealthSnapshot& health, bool frameworkReady, uint64_t now) noexcept {
  const char key = *static_cast<const char*>(action);
  owner.observe(now, health, settings, link, frameworkReady);
  if (key == 'a') lastAdmission = owner.admit(now, ticket);
  const auto& view = owner.snapshot();
  const bool active = view.phase == OwnerPhase::ACTIVE;
  if (!frames.setSession(active ? view.generation : 0) || !active) return;
  // A rejected lifecycle request must not service a frame as a side effect.
  if (key == 'm' || key == 'v' || key == 'q' || key == 'r' || key == 's') return;
  if (frames.popRx(view.generation, serviceFrame)) {
    // Classify a callback-delivered raw observation, without interpreting a
    // protocol response or assigning a generation to its on-wire arrival.
    if (serviceFrame.bytes[12] == 0x88 && serviceFrame.bytes[13] == 0xB5)
      ++experimentalRx;
    else ++otherRx;
  }
  if (frames.popTx(view.generation, serviceFrame) &&
      !transmitAdmitted(owner, frames, upperNowUs(), driver, serviceFrame, txCounts)) return;
  // Explicit producer only. Service this new request on a later gated turn.
  if (key == 'x' && testTrafficEnabled && view.phase == OwnerPhase::ACTIVE)
    testFrameQueued = frames.queueTx(view.generation, testFrame, sizeof(testFrame));
}

void observe(char key) {
  testFrameQueued = false;
  const auto status = phy->withAdmissionEvidence(&key, decide);
  if (!status.ok()) {
    frames.setSession(0);
    owner.missingEvidence(upperNowUs());
    lastAdmission = status;
  }
  if (key == 'a') show("admit", lastAdmission);
  if (key == 'x') {
    if (!testTrafficEnabled) std::puts("Test traffic disabled in menuconfig");
    else std::puts(testFrameQueued ? "Test frame queued for owner service" : "Test frame request dropped");
  }
}

bool maintenance(bool restore) {
  const auto phase = owner.snapshot().phase;
  const bool released = phase == OwnerPhase::WAITING || phase == OwnerPhase::READY;
  const auto admitted = owner.beginMaintenance(released && frames.quiesce());
  if (!admitted.ok()) { show("maintenance", admitted); return true; }
  if (!accepted("stop before maintenance", phy->stop(deadline()))) return false;
  const auto status = phy->startInitialize({restore ? ResetKind::NONE : ResetKind::PCS}, budget(), pending);
  if (!status.inProgress()) { show("initialize admission", status); owner.finishMaintenance(upperNowUs()); return true; }
  if (!finish(completed, true)) return false;
  restoreRequest = restore;
  show("maintenance operation", completed);
  const auto retained = owner.retainResult(completed);
  if (!retained.ok()) { show("result backpressure", retained); return false; }
  owner.finishMaintenance(upperNowUs());
  if (completed.status.ok() && completed.persistentRestored && !completed.cleanupOwed) return runFramework();
  return true; // Explicit result consumption and a separate restoration request.
}

void run() {
  LinkSettings profile{};
  profile.advertisement.technologyMask = LINK_100_FULL;
  const auto valid = owner.begin(profile);
  if (!valid.ok()) { show("startup profile", valid); return; }
  const auto flags = fcntl(STDIN_FILENO, F_GETFL);
  if (flags < 0 || fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK) < 0) {
    std::puts("A nonblocking console is required"); return;
  }
  if (!install()) { closeResources(); return; }
  Status status = phy->startInitialize({ResetKind::NONE}, budget(), pending);
  if (status.inProgress()) {
    if (!finish(startupInit, false)) { closeResources(); return; }
    show("initialization", startupInit);
    status = startupInit.status;
  }
  if (status.ok()) {
    status = phy->startConfigure(profile, budget(), pending);
    if (status.inProgress()) {
      if (!finish(startupConfig, false)) { closeResources(); return; }
      show("configuration", startupConfig);
      status = startupConfig.status;
    }
  }
  if (!status.ok()) { show("startup", status); closeResources(); return; }
  if (!runFramework()) { closeResources(); return; }
  std::puts("a admit, r release, x queue test frame, m PCS maintenance, v restore, c cancel during maintenance, t take result, s status, q stop");
  auto previous = owner.snapshot().phase;
  for (;;) {
    char key{};
    (void)read(STDIN_FILENO, &key, 1);
    if (key == 'r') {
      frames.setSession(0);
      show("release", owner.release(ticket));
    }
    observe(key); // Same adapter gate for current evidence and explicit admission.
    const auto& view = owner.snapshot();
    if (view.phase != previous || key == 's') {
      std::printf("phase=%s generation=%llu evidenceAvailable=%d frameworkReady=%d health=%s trust=%u linkValid=%u stale=%u linkUp=%u speed=%u duplex=%u resultPending=%u\n",
          phaseName(view.phase), static_cast<unsigned long long>(view.generation),
          view.evidenceAvailable, view.frameworkReady, driverStateName(view.health.state), static_cast<unsigned>(view.settings.trust),
          view.link.valid, view.link.stale, view.link.linkUp, static_cast<unsigned>(view.link.speed),
          static_cast<unsigned>(view.link.duplex), view.resultPending);
      previous = view.phase;
    }
    if (key == 's') {
      const auto c = frames.counts();
      std::printf("rx accepted=%lu dropped=%lu experimental=%llu other=%llu; tx queued=%lu dropped=%lu denied=%llu dma-queued=%llu backpressure=%llu link-down=%llu failed=%llu attempted=%u first-error=%ld last-sdk=%ld\n",
          static_cast<unsigned long>(c.rxAccepted), static_cast<unsigned long>(c.rxDropped),
          static_cast<unsigned long long>(experimentalRx), static_cast<unsigned long long>(otherRx),
          static_cast<unsigned long>(c.txQueued), static_cast<unsigned long>(c.txDropped),
          static_cast<unsigned long long>(txCounts.denied), static_cast<unsigned long long>(txCounts.queuedToDma),
          static_cast<unsigned long long>(txCounts.backpressure),
          static_cast<unsigned long long>(txCounts.linkDown), static_cast<unsigned long long>(txCounts.failed),
          txCounts.attempted, static_cast<long>(txCounts.firstError), static_cast<long>(txCounts.lastError));
    }
    if ((key == 'm' || key == 'v') && !maintenance(key == 'v')) break;
    if (key == 't') {
      auto& destination = restoreRequest ? restoration : primary;
      const auto taken = owner.takeResult(destination);
      if (taken.ok()) show(restoreRequest ? "restoration" : "primary", destination);
      else show("take", taken);
    }
    if (key == 'q') {
      if (view.resultPending) std::puts("Take the pending operation result before exit");
      else if (view.phase == OwnerPhase::ACTIVE || view.phase == OwnerPhase::WITHDRAWN)
        std::puts("Release the session before exit");
      else break;
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  closeResources();
}
} // namespace

#ifdef CONFIG_DP8382X_BOARD_READY
static volatile bool authorisedBoard = true;
#else
static volatile bool authorisedBoard = false;
#endif
extern "C" void app_main() {
  std::printf("DP8382x %s %s %s IDF %s P4 RMII address %lu poll %lu ms\n",
      ::DP8382x::VERSION_STRING, EXAMPLE_GIT_COMMIT, EXAMPLE_GIT_TREE, esp_get_idf_version(),
      static_cast<unsigned long>(Board::PHY_ADDRESS), static_cast<unsigned long>(Board::LINK_POLL_MS));
  if (authorisedBoard) run();
  else std::puts("Board gate closed. Confirm netlist, supplies, reset and clock first.");
}
