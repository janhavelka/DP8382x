/// @file main.cpp
/// @brief Single P4 application owner. The SDK timer collects link evidence.
#include "../../basic/main/BoardConfig.h"
#ifndef DP8382X_OWNER_EXAMPLE_TEST
#include "ExampleIdentity.h"
#include "esp_random.h"
#endif
#include "PhyOwner.h"
#include "RawMac.h"
#include "../../common/Dp8382xExampleConsole.h"
#include <DP8382x/esp_idf/Phy.h>
#include <DP8382x/Version.h>
#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_system.h"
#include <cstdio>

#if CONFIG_ETH_TRANSMIT_MUTEX || CONFIG_ESP_MM_CACHE_MSYNC_C2M_CHUNKED_OPS || CONFIG_LOG_MAXIMUM_LEVEL > 3
#error "Owner TX under the admission gate requires TX mutex and chunked cache mutex off, maximum logging INFO or lower"
#endif

namespace {
using namespace ::DP8382x;
using namespace ::DP8382x::EspIdf;
namespace Cli = ::DP8382x::Cli;
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
Cli::ExampleConsole console;
bool maintenanceReady{};
bool retired{};
uint64_t experimentalRx{}, otherRx{};
// Local experimental EtherType. The source address is filled from the actual MAC.
uint8_t testFrame[60] = {0x02,0,0,0,0,0x2A, 0,0,0,0,0,0, 0x88,0xB5, 'P','4'};
#ifdef CONFIG_DP8382X_ENABLE_TEST_FRAME
static volatile bool testTrafficEnabled = true;
#else
static volatile bool testTrafficEnabled = false;
#endif
uint64_t ticket{};
OperationToken pending{};
OperationResult startupInit, startupConfig;
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
  console.print("%s %s detail=%ld", label, errorName(status.code), static_cast<long>(status.detail));
}
void show(const char* label, const OperationResult& result) {
  console.print("%s token=%llu status=%s detail=%ld possible-write=%d cleanup-owed=%d restored=%d",
      label, static_cast<unsigned long long>(result.token.value), errorName(result.status.code),
      static_cast<long>(result.status.detail), result.mutationPossible, result.cleanupOwed, result.persistentRestored);
}
bool accepted(const char* label, esp_err_t error) {
  if (error != ESP_OK) console.print("%s SDK error=%s (%ld)", label, esp_err_to_name(error), static_cast<long>(error));
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
  console.native.phy = phy;
  // Register once, while STOPPED and before first start. No netif/glue owns RX.
  if (!accepted("raw RX registration", esp_eth_update_input_path(driver, receiveRaw, &frames))) return false;
  return !testTrafficEnabled ||
      accepted("test source MAC", esp_eth_ioctl(driver, ETH_CMD_G_MAC_ADDR, &testFrame[6]));
}

void closeResources() {
  frames.setSession(0);
  // This clears application queues if accessible; a late RX still owns a
  // static context. Even an empty application queue cannot fence the SDK task.
  if (!frames.quiesce()) console.print("Frame queues retained closed during teardown");
  if (driver) {
    if (pending.value != 0) {
      console.print("Operation result delivery outstanding; retaining installed owner and token=%llu",
          static_cast<unsigned long long>(pending.value));
      return;
    }
    const auto error = phy->retire(deadline());
    if (error == ESP_OK || error == ESP_ERR_NOT_SUPPORTED) {
      retired = true;
      maintenanceReady = false;
      console.native.phy = nullptr;
    }
    LifecycleRecord life{};
    const auto sampled = phy->lifecycleSnapshot(life);
    if (sampled.ok())
      console.print("lifecycle primary=%s/%ld cleanup=%s/%ld", errorName(life.firstError.coreStatus.code),
          static_cast<long>(life.firstError.sdkError), errorName(life.cleanupError.coreStatus.code),
          static_cast<long>(life.cleanupError.sdkError));
    console.print("retire=%s first-tx-error=%ld installed storage retained; outstanding token=%llu",
        esp_err_to_name(error), static_cast<long>(txCounts.firstError), static_cast<unsigned long long>(pending.value));
    return;
  }
  if (phy) {
    if (!accepted("factory cleanup", phy->destroy())) return;
    phy = nullptr;
    console.native.phy = nullptr;
  }
  if (mac) {
    if (!accepted("MAC cleanup", mac->del(mac))) return;
    mac = nullptr;
  }
  if (bus.mutex && !bus.quarantined.load()) { vSemaphoreDelete(bus.mutex); bus.mutex = nullptr; }
  if (eventLoopOwned && accepted("event loop cleanup", esp_event_loop_delete_default())) eventLoopOwned = false;
}

// One batch per tick; successful take copies even a failed/cancelled result.
// Gate contention after completion leaves the same token owed to the owner.
bool finish(OperationResult& result) {
  const auto until = nowUs() + 6000000U;
  bool cancelDue = false;
  unsigned cancelAttempts = 0;
  for (unsigned step = 0; step < 6500U; ++step) {
    if (!cancelDue && (nowUs() >= until || step >= 6400U)) {
      console.print("Owner wait expired; requesting cancellation and retaining the core result");
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
    console.service();
    vTaskDelay(1);
  }
  console.print("result delivery pending token=%llu", static_cast<unsigned long long>(pending.value));
  return false;
}

bool runFramework() {
  const auto state = phy->reconcileSdkState(deadline());
  if (!state.ok()) { show("reconciliation", state.coreStatus); accepted("reconciliation", state.sdkError); return false; }
  if (!accepted("framework start", phy->start(deadline()))) return false;
  owner.frameworkRestarted();
  return true;
}

LinkSettings rawProfile() {
  LinkSettings profile{};
  profile.advertisement.technologyMask = LINK_100_FULL;
  return profile;
}

void decide(void* context, const LinkSnapshot& link, const SettingsSnapshot& settings,
            const HealthSnapshot& health, bool frameworkReady, uint64_t now) noexcept {
  auto* admission = static_cast<Status*>(context);
  owner.observe(now, health, settings, link, frameworkReady);
  if (admission) *admission = owner.admit(now, ticket);
  const auto& view = owner.snapshot();
  const bool active = view.phase == OwnerPhase::ACTIVE;
  if (!frames.setSession(active ? view.generation : 0)) {
    if (active) {
      owner.missingEvidence(upperNowUs());
      if (admission) *admission = Status::Error(Err::BUSY, "Frame ingress contended");
    }
    return;
  }
  if (!active || admission) return;
  if (frames.popRx(view.generation, serviceFrame)) {
    // Classify a callback-delivered raw observation, without interpreting a
    // protocol response or assigning a generation to its on-wire arrival.
    if (serviceFrame.bytes[12] == 0x88 && serviceFrame.bytes[13] == 0xB5)
      ++experimentalRx;
    else ++otherRx;
  }
  if (frames.popTx(view.generation, serviceFrame))
    (void)transmitAdmitted(owner, frames, upperNowUs(), driver, serviceFrame, txCounts);
}

Status observe(Status* admission = nullptr) {
  const auto status = phy->withAdmissionEvidence(admission, decide);
  if (!status.ok()) {
    frames.setSession(0);
    owner.missingEvidence(upperNowUs());
  }
  return status;
}

bool maintenanceAllowed(void*) { return maintenanceReady && !retired; }

// END is terminal: the callback storage remains static even if the SDK cannot
// unpublish its borrowed input path before the next system restart.
Status endOwner(void*, uint64_t until) {
  if (!phy || !driver || retired) return Status::Error(Err::NOT_BOUND, "No installed PHY");
  if (!maintenanceAllowed(nullptr) || until == 0)
    return Status::Error(Err::BUSY, "Stop the framework before END");
  const auto slot = Cli::applicationSlot(*phy);
  if (!slot.ok()) return slot;
  if (!frames.quiesce()) return Status::Error(Err::BUSY, "Frame queues are not quiescent");
  const auto error = phy->retire(until);
  if (error == ESP_OK || error == ESP_ERR_NOT_SUPPORTED) {
    retired = true;
    maintenanceReady = false;
    console.native.phy = nullptr;
  }
  return Cli::applicationSdkStatus(error);
}

// Application commands are serialized by the same task as CLI and frame service.
Status application(void*, Cli::ApplicationCommand command, uint64_t until, Cli::FeatureFields& fields) {
  const auto& view = owner.snapshot();
  if (command == Cli::ApplicationCommand::STATUS) {
    const auto c = frames.counts();
    fields.set(" phase=%s gen=%llu ready=%u restart=%u maintenance=%u"
               " rx=%lu rxd=%lu rxexp=%llu rxother=%llu tx=%lu txd=%lu"
               " dma=%llu txdenied=%llu txbp=%llu txdown=%llu txfailed=%llu"
               " txattempted=%u txfirst=%ld txsdk=%ld retired=%u",
               phaseName(view.phase), static_cast<unsigned long long>(view.generation), unsigned(view.frameworkReady),
               unsigned(view.frameworkRestartRequired), unsigned(maintenanceReady),
               static_cast<unsigned long>(c.rxAccepted), static_cast<unsigned long>(c.rxDropped),
               static_cast<unsigned long long>(experimentalRx), static_cast<unsigned long long>(otherRx),
               static_cast<unsigned long>(c.txQueued), static_cast<unsigned long>(c.txDropped),
               static_cast<unsigned long long>(txCounts.queuedToDma),
               static_cast<unsigned long long>(txCounts.denied),
               static_cast<unsigned long long>(txCounts.backpressure),
               static_cast<unsigned long long>(txCounts.linkDown),
               static_cast<unsigned long long>(txCounts.failed), unsigned(txCounts.attempted),
               static_cast<long>(txCounts.firstError), static_cast<long>(txCounts.lastError), unsigned(retired));
    return Status::Ok();
  }
  if (!phy || !driver || retired) return Status::Error(Err::NOT_BOUND, "No installed PHY");
  if (until == 0) return Status::Error(Err::OPERATION_TIMEOUT, "Application deadline required");

  switch (command) {
    case Cli::ApplicationCommand::STOP: {
      const auto slot = Cli::applicationSlot(*phy);
      if (!slot.ok()) return slot;
      if (view.phase != OwnerPhase::MAINTENANCE) {
        if (view.phase != OwnerPhase::WAITING && view.phase != OwnerPhase::READY)
          return Status::Error(Err::BUSY, "Release the raw session before stop");
        if (!frames.setSession(0) || !frames.quiesce())
          return Status::Error(Err::BUSY, "Frame queues are not quiescent");
        const auto admitted = owner.beginMaintenance(true);
        if (!admitted.ok()) return admitted;
      }
      maintenanceReady = false;
      const auto error = phy->stop(until);
      maintenanceReady = error == ESP_OK;
      return Cli::applicationSdkStatus(error);
    }
    case Cli::ApplicationCommand::START: {
      if (view.phase != OwnerPhase::MAINTENANCE || !maintenanceReady)
        return Status::Error(Err::BUSY, "Verified stop required before start");
      const auto slot = Cli::applicationSlot(*phy);
      if (!slot.ok()) return slot;
      SettingsSnapshot settings{};
      const auto copied = phy->settingsSnapshot(settings);
      if (!copied.ok()) return copied;
      if (!owner.verifiedProfile(settings))
        return Status::Error(Err::INVALID_CONFIG, "Verified 100/full raw profile required");
      const auto state = phy->reconcileSdkState(until);
      if (!state.ok()) return state.sdkError == ESP_OK ? state.coreStatus : Cli::applicationSdkStatus(state.sdkError);
      const auto error = phy->start(until);
      if (error != ESP_OK) { maintenanceReady = false; return Cli::applicationSdkStatus(error); }
      owner.finishMaintenance(upperNowUs());
      owner.frameworkRestarted();
      maintenanceReady = false;
      return Status::Ok();
    }
    case Cli::ApplicationCommand::ADMIT: {
      if (view.phase != OwnerPhase::READY)
        return Status::Error(Err::BUSY, "Current 100/full link required for admission");
      Status admitted;
      const auto observed = observe(&admitted);
      return observed.ok() ? admitted : observed;
    }
    case Cli::ApplicationCommand::RELEASE:
      if (view.phase != OwnerPhase::ACTIVE && view.phase != OwnerPhase::WITHDRAWN)
        return Status::Error(Err::BUSY, "No raw session to release");
      if (!frames.setSession(0) || !frames.quiesce())
        return Status::Error(Err::BUSY, "Frame queues are not quiescent");
      return owner.release(ticket);
    case Cli::ApplicationCommand::SEND:
      if (!testTrafficEnabled) return Status::Error(Err::UNSUPPORTED, "Test frame source disabled");
      if (view.phase != OwnerPhase::ACTIVE || !frames.queueTx(view.generation, testFrame, sizeof testFrame))
        return Status::Error(Err::BUSY, "Admitted frame queue required");
      fields.set(" queued=1 generation=%llu", static_cast<unsigned long long>(view.generation));
      return Status::Ok();
    case Cli::ApplicationCommand::STATUS: break;
  }
  return Status::Error(Err::INVALID_PARAM, "Unknown application command");
}

bool attachConsole(const Cli::Identity& identity) {
  console.native.application = application;
  console.native.maintenanceAllowed = maintenanceAllowed;
  console.native.end = endOwner;
  return console.attach(identity);
}

void runBoard() {
  const auto profile = rawProfile();
  const auto valid = owner.begin(profile);
  if (!valid.ok()) { show("startup profile", valid); return; }
  if (!install()) { closeResources(); return; }
  Status status = phy->startInitialize({ResetKind::NONE}, budget(), pending);
  if (status.inProgress()) {
    if (!finish(startupInit)) { closeResources(); return; }
    show("initialization", startupInit);
    status = startupInit.status;
  }
  if (status.ok()) {
    status = phy->startConfigure(profile, budget(), pending);
    if (status.inProgress()) {
      if (!finish(startupConfig)) { closeResources(); return; }
      show("configuration", startupConfig);
      status = startupConfig.status;
    }
  }
  if (!status.ok()) { show("startup", status); closeResources(); return; }
  if (!runFramework()) closeResources();
}

void serviceIteration(uint64_t& lastEvidenceUs) {
  console.service();
  const auto now = nowUs();
  if (phy && driver && !retired && owner.snapshot().phase != OwnerPhase::MAINTENANCE &&
      (lastEvidenceUs == 0 || now - lastEvidenceUs >= 100000U)) {
    (void)observe();
    lastEvidenceUs = now;
  }
}

#ifndef DP8382X_OWNER_EXAMPLE_TEST
void serve() {
  uint64_t lastEvidenceUs = 0;
  for (;;) {
    serviceIteration(lastEvidenceUs);
    vTaskDelay(1);
  }
}
#endif
} // namespace

#ifndef DP8382X_OWNER_EXAMPLE_TEST
/// @brief Board authorization from menuconfig. Retains the complete owner
/// path in the default closed-gate image.
static volatile bool authorisedBoard =
#ifdef CONFIG_DP8382X_BOARD_READY
    true;
#else
    false;
#endif
/// @brief Start the shared UART service and the explicitly authorized raw owner.
/// @note The application task handles CLI, lifecycle and frame admission.
extern "C" void app_main() {
  Cli::Identity identity{};
  identity.boot = esp_random();
  if (identity.boot == 0) identity.boot = static_cast<uint32_t>(nowUs()) | 1U;
  identity.firmware = EXAMPLE_GIT_COMMIT;
  identity.tree = EXAMPLE_GIT_TREE;
  identity.sdk = esp_get_idf_version();
  identity.target = CONFIG_IDF_TARGET;
  identity.backend = "P4-raw-frame";
  identity.clock = "RMII-50MHz-EXT";
  identity.variant = Variant::DP83826I;
  identity.interface = MacInterface::RMII;
  identity.phyAddress = Board::PHY_ADDRESS;
  identity.console = "uart0";
  identity.baud = CONFIG_ESP_CONSOLE_UART_BAUDRATE;
  if (!attachConsole(identity)) return;
  const auto uart = Cli::startUartConsole(console.queues);
  if (uart != ESP_OK) {
    std::printf("UART0 console setup failed: %s (%ld)\n", esp_err_to_name(uart), static_cast<long>(uart));
    return;
  }
  console.print("DP8382x %s %s %s IDF %s P4 RMII address %lu poll %lu ms",
      ::DP8382x::VERSION_STRING, EXAMPLE_GIT_COMMIT, EXAMPLE_GIT_TREE, esp_get_idf_version(),
      static_cast<unsigned long>(Board::PHY_ADDRESS), static_cast<unsigned long>(Board::LINK_POLL_MS));
  if (authorisedBoard) runBoard();
  else console.print("Board gate closed. Confirm netlist, supplies, reset and clock first.");
  serve();
}
#endif
