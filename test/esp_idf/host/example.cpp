#include "InstrumentedMac.h"
#include "../../../examples/esp_idf/basic/main/BoardConfig.h"

#include <cassert>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>

#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_netif.h"

#define DP8382X_EXAMPLE_TEST
#include "../../../examples/esp_idf/basic/main/main.cpp"

namespace {
struct Faults {
    esp_err_t netifInit{ESP_OK};
    esp_err_t eventLoopCreate{ESP_OK};
    esp_err_t eventLoopDelete{ESP_OK};
    bool nullNetif{false};
    bool nullMac{false};
    esp_err_t gpioLevel{ESP_OK};
    esp_err_t resetRelease{ESP_OK};
    esp_err_t gpioDirection{ESP_OK};
    bool eventLoopOwned{false};
    unsigned eventLoopDeletes{0};
    unsigned netifDestroys{0};
    const char* macErrorOperation{nullptr};
    esp_err_t macError{ESP_OK};
};
Faults faults;
std::unique_ptr<Host::InstrumentedMac> mac;
}

extern "C" {
// The host runs the real Ethernet driver and glue with a fake netif, without lwIP.
const esp_netif_inherent_config_t _g_esp_netif_inherent_eth_config = ESP_NETIF_INHERENT_DEFAULT_ETH();
const esp_netif_netstack_config_t* _g_esp_netif_netstack_default_eth = nullptr;

const char* esp_err_to_name(esp_err_t error) {
    return error == ESP_OK ? "ESP_OK" : "INJECTED_OR_SDK_ERROR";
}
esp_err_t esp_netif_init(void) {
    Host::state().calls.emplace_back("netif.init");
    return faults.netifInit;
}
esp_netif_t* esp_netif_new(const esp_netif_config_t* config) {
    assert(config && config->base == ESP_NETIF_BASE_DEFAULT_ETH);
    assert(config->base->get_ip_event == IP_EVENT_ETH_GOT_IP && config->base->lost_ip_event == IP_EVENT_ETH_LOST_IP);
    Host::state().calls.emplace_back("netif.new");
    return faults.nullNetif ? nullptr : Host::makeNetif();
}
void esp_netif_destroy(esp_netif_t* netif) {
    Host::state().calls.emplace_back("netif.destroy");
    ++faults.netifDestroys;
    Host::destroyNetif(netif);
}
esp_err_t esp_event_loop_create_default(void) {
    Host::state().calls.emplace_back("event.loop.create");
    if (faults.eventLoopCreate == ESP_OK) faults.eventLoopOwned = true;
    return faults.eventLoopCreate;
}
esp_err_t esp_event_loop_delete_default(void) {
    Host::state().calls.emplace_back("event.loop.delete");
    ++faults.eventLoopDeletes;
    if (faults.eventLoopDelete == ESP_OK) faults.eventLoopOwned = false;
    return faults.eventLoopDelete;
}
esp_eth_mac_t* esp_eth_mac_new_esp32(const eth_esp32_emac_config_t*, const eth_mac_config_t*) {
    Host::state().calls.emplace_back("mac.new");
    if (faults.nullMac) return nullptr;
    mac = std::make_unique<Host::InstrumentedMac>();
    if (faults.macErrorOperation) mac->errors[faults.macErrorOperation] = faults.macError;
    return &mac->native;
}
esp_err_t gpio_set_level(gpio_num_t, uint32_t level) {
    Host::state().calls.emplace_back("gpio.level");
    if (level == 1 && faults.resetRelease != ESP_OK) return faults.resetRelease;
    return faults.gpioLevel;
}
esp_err_t gpio_set_direction(gpio_num_t, gpio_mode_t) {
    Host::state().calls.emplace_back("gpio.direction");
    return faults.gpioDirection;
}
}

namespace {
void boardConfig() {
    eth_esp32_emac_config_t config = ETH_ESP32_EMAC_DEFAULT_CONFIG();
    Board::apply(config);
    assert(Board::valid(config));
    assert(config.smi_gpio.mdc_num == 40 && config.smi_gpio.mdio_num == 39);
    assert(config.clock_config.rmii.clock_mode == EMAC_CLK_EXT_IN);
    assert(config.clock_config.rmii.clock_gpio == 44);
    const auto rejects = [&](auto change) {
        auto wrong = config;
        change(wrong);
        assert(!Board::valid(wrong));
    };
    rejects([](auto& c) { c.smi_gpio.mdc_num = -1; });
    rejects([](auto& c) { c.smi_gpio.mdio_num = -1; });
    rejects([](auto& c) { c.interface = EMAC_DATA_INTERFACE_MII; });
    rejects([](auto& c) { c.clock_config.rmii.clock_mode = EMAC_CLK_OUT; });
    rejects([](auto& c) { c.clock_config.rmii.clock_gpio = -1; });
    rejects([](auto& c) { c.emac_dataif_gpio.rmii.tx_en_num = -1; });
    rejects([](auto& c) { c.emac_dataif_gpio.rmii.txd0_num = -1; });
    rejects([](auto& c) { c.emac_dataif_gpio.rmii.txd1_num = -1; });
    rejects([](auto& c) { c.emac_dataif_gpio.rmii.crs_dv_num = -1; });
    rejects([](auto& c) { c.emac_dataif_gpio.rmii.rxd0_num = -1; });
    rejects([](auto& c) { c.emac_dataif_gpio.rmii.rxd1_num = -1; });
}
void gateClosed() {
    Host::reset();
    run(false);
    assert(Host::state().calls.empty() && !app.installed && app.primary.ok());
}
void acquisitionFailure(const std::string& caseName) {
    Host::reset();
    if (caseName == "netif_init_failure") faults.netifInit = ESP_FAIL;
    else if (caseName == "event_loop_failure") faults.eventLoopCreate = ESP_FAIL;
    else if (caseName == "netif_new_failure") faults.nullNetif = true;
    else if (caseName == "mutex_failure") Host::state().failSemaphore = true;
    else if (caseName == "mac_new_failure") faults.nullMac = true;
    else if (caseName == "factory_failure") Host::state().nextTimerCreateError = ESP_ERR_NO_MEM;
    else if (caseName == "board_failure") faults.gpioLevel = ESP_FAIL;
    else if (caseName == "board_direction_failure") faults.gpioDirection = ESP_FAIL;
    else if (caseName == "board_release_failure") faults.resetRelease = ESP_FAIL;
    else if (caseName == "driver_allocation_failure") Host::state().failAllocationNumber = 1;
    else if (caseName == "install_failure") {
        faults.macErrorOperation = "mac.init";
        faults.macError = ESP_FAIL;
    }
    else assert(false);
    run(true);
    assert(!app.installed && !app.primary.ok());
    assert(Host::activeHandlers() == 0 && Host::activeNetifs() == 0);
    assert(app.netif == nullptr && app.bus.mutex == nullptr);
    assert(!app.loopOwned && !faults.eventLoopOwned);
    if (caseName == "install_failure") assert(mac && !mac->alive);
    if (caseName == "board_release_failure") {
        assert(std::count(Host::state().calls.begin(), Host::state().calls.end(), "gpio.level") == 2);
        assert(app.primary.sdkError == ESP_FAIL && std::string(app.failedStage) == "board preparation");
        assert(app.bus.admission.load() == BusAdmission::FAILED_CLOSED);
        assert(mac && mac->frames.empty() && !mac->alive);
    }
}
void cleanupFailure() {
    Host::reset();
    faults.nullNetif = true;
    faults.eventLoopDelete = ESP_FAIL;
    run(true);
    // A null factory reports no cause: the stage is known, no SDK code is invented.
    assert(!app.installed && app.primary.sdkError == ESP_OK);
    assert(app.primary.coreStatus.is(Err::INVALID_CONFIG) && std::string(app.failedStage) == "netif new");
    assert(app.cleanup.sdkError == ESP_FAIL && app.loopOwned && faults.eventLoopOwned);
}
void macCleanupFailure() {
    Host::reset();
    Host::state().nextTimerCreateError = ESP_ERR_NO_MEM;
    faults.macErrorOperation = "mac.del";
    faults.macError = ESP_FAIL;
    run(true);
    assert(!app.installed && app.primary.sdkError == ESP_ERR_NO_MEM);
    assert(std::string(app.failedStage) == "PHY create");
    // The PHY borrower goes first; the MAC that failed deletion stays, and the
    // bus mutex (used only by the PHY) is released.
    assert(app.cleanup.sdkError == ESP_FAIL && app.mac && !app.phy && !app.bus.mutex);
    assert(Host::activeNetifs() == 0 && !app.loopOwned);
}
void quarantinedMutexRetained() {
    Host::reset();
    // The board drain takes the bus mutex and its give fails: the adapter
    // quarantines the bus, so the possibly held mutex must not be deleted.
    Host::state().beforeSemaphoreTake = [](SemaphoreHandle_t handle) {
        if (handle == app.bus.mutex && !Host::state().failNextGive) Host::state().failNextGive = handle;
    };
    run(true);
    Host::state().beforeSemaphoreTake = nullptr;
    assert(!app.installed && !app.primary.ok() && app.bus.quarantined.load());
    assert(std::string(app.failedStage) == "board preparation");
    assert(app.bus.mutex && !app.phy && !app.mac);
    Host::state().failNextGive = nullptr;
}
void glueAllocationFailure() {
    Host::reset();
    Host::state().failAllocationNumber = 2;
    run(true);
    assert(app.installed && app.primary.coreStatus.is(Err::INVALID_CONFIG));
    assert(std::string(app.failedStage) == "netif glue");
    assert(Host::state().allocationAttempts == 2);
    assert(!app.glue && app.driver && app.netif && mac && mac->alive);
    assert(Host::activeHandlers() == 0 && Host::activeNetifs() == 1);
}
void sharedLoop() {
    Host::reset();
    faults.eventLoopCreate = ESP_ERR_INVALID_STATE;
    faults.nullNetif = true;
    run(true);
    assert(!app.installed && faults.eventLoopDeletes == 0);
}
void glueRegistrationFailure(unsigned registration) {
    Host::reset();
    Host::state().failHandlerRegistrationNumber = registration;
    Host::state().nextHandlerRegisterError = ESP_FAIL;
    run(true);
    assert(app.installed && !app.primary.ok());
    assert(Host::state().handlerRegistrationCount == registration);
    assert(Host::activeHandlers() == 0);
    assert(Host::activeNetifs() == 1 && app.netif);
    assert(app.phy && app.driver && mac && mac->alive);
}
void appRegistrationFailure(unsigned registration) {
    Host::reset();
    Host::state().failHandlerRegistrationNumber = registration;
    Host::state().nextHandlerRegisterError = ESP_FAIL;
    run(true);
    assert(app.installed && !app.primary.ok());
    assert(Host::state().handlerRegistrationCount == registration);
    assert(Host::activeHandlers() == registration - 1);
    assert(Host::activeNetifs() == 1 && app.netif);
    assert(app.phy && app.driver && app.glue && mac && mac->alive);
}
void attachPartialFailure() {
    Host::reset();
    Host::state().nextNetifAttachError = ESP_FAIL;
    run(true);
    assert(app.installed && app.primary.sdkError == ESP_FAIL);
    assert(app.netif && app.glue && app.driver && mac && mac->alive);
    assert(Host::activeNetifs() == 1 && Host::activeHandlers() == 5);
    mac->pauseRxAfterMediatorLoad();
    assert(mac->resumeRx() == ESP_OK && Host::state().netifRx == 1);
}
void attachAddressFailure(const std::string& suite) {
    Host::reset();
    if (suite == "emac_address_failure") {
        faults.macErrorOperation = "mac.get_addr";
        faults.macError = ESP_FAIL;
    } else if (suite == "netif_address_mismatch") {
        Host::state().nextSetMacError = ESP_FAIL; // Pinned glue hides this return.
    } else {
        assert(suite == "netif_address_read_failure");
        Host::state().nextGetMacError = ESP_ERR_TIMEOUT;
    }
    run(true);
    assert(app.installed && Host::isLive(app.driver) && mac->alive && mac->frames.empty());
    assert(std::count(Host::state().calls.begin(), Host::state().calls.end(), "timer.periodic") == 0);
    if (suite == "emac_address_failure") {
        assert(!app.glue && app.primary.sdkError == ESP_FAIL);
        assert(std::string(app.failedStage) == "EMAC address");
    } else if (suite == "netif_address_mismatch") {
        assert(app.glue && app.primary.coreStatus.is(Err::CONFIG_MISMATCH) && app.primary.sdkError == ESP_OK);
        assert(std::string(app.failedStage) == "netif MAC mismatch");
    } else {
        assert(app.glue && app.primary.sdkError == ESP_ERR_TIMEOUT);
        assert(std::string(app.failedStage) == "netif address readback");
    }
}
void eventBurstWithoutConsole() {
    Host::reset();
    app.driver = reinterpret_cast<esp_eth_handle_t>(0x1234);
    app.netif = Host::makeNetif();
    for (unsigned i = 0; i < 1000; ++i) {
        onEthernet(&app, ETH_EVENT, ETHERNET_EVENT_CONNECTED, &app.driver);
        onEthernet(&app, ETH_EVENT, ETHERNET_EVENT_DISCONNECTED, &app.driver);
    }
    ip_event_got_ip_t ip{};
    ip.esp_netif = app.netif;
    ip.ip_info.ip.addr = 0x04030201U;
    onIp(&app, IP_EVENT, IP_EVENT_ETH_GOT_IP, &ip);
    assert(app.ipAddress.load() == 0x04030201U);
    ip_event_got_ip_t foreign{};
    foreign.esp_netif = Host::makeNetif();
    onLostIp(&app, IP_EVENT, IP_EVENT_ETH_LOST_IP, &foreign);
    assert(app.ipAddress.load() == 0x04030201U);
    onLostIp(&app, IP_EVENT, IP_EVENT_ETH_LOST_IP, &ip);
    assert(app.ipAddress.load() == 0);
    onEthernet(&app, ETH_EVENT, ETHERNET_EVENT_STOP, &app.driver);
    assert(app.shutdownRequested.load() && !app.linked.load() && app.ipAddress.load() == 0);
    assert(app.events.load() == 2003U);
    // No app callback entered an SDK function or owner wait; console can stall
    // independently without keeping the default event loop in a callback.
    assert(Host::state().calls.empty());
}
void startStop(bool startFailure, bool stopFailure, bool stopEventFailure) {
    Host::reset();
    assert(initialize());
    assert(app.installed && app.primary.ok());
    if (startFailure) {
        Host::state().eventErrorId = ETHERNET_EVENT_START;
        Host::state().nextEventError = ESP_FAIL;
    }
    if (stopFailure) mac->errors["mac.stop"] = ESP_FAIL;
    if (stopEventFailure) {
        Host::state().eventErrorId = ETHERNET_EVENT_STOP;
        Host::state().nextEventError = ESP_FAIL;
    }
    serve();
    finish();
    assert(app.installed && app.driver && app.glue && app.netif && mac && mac->alive);
    assert(Host::activeNetifs() == 1 && Host::activeHandlers() == 8);
    LifecycleRecord record{};
    assert(app.phy->lifecycleSnapshot(record).ok() && record.phase == Phase::MAINTENANCE);
    assert(app.primary.coreStatus.ok() && app.cleanup.coreStatus.ok() && !app.resultOwed);
    if (startFailure) {
        assert(std::string(app.failedStage) == "start" && app.primary.sdkError == ESP_FAIL);
        assert(app.cleanup.sdkError == ESP_ERR_INVALID_STATE); // Link timer was never armed.
    } else if (stopFailure) {
        assert(std::string(app.failedStage) == "owner stop" && app.primary.sdkError == ESP_FAIL);
        assert(app.cleanup.sdkError == ESP_FAIL);
    } else if (stopEventFailure) {
        assert(app.primary.ok() && app.cleanup.sdkError == ESP_FAIL);
        assert(std::string(app.sessionEnd) == "demonstration complete"); // STOP was reposted.
    } else {
        assert(app.primary.ok() && app.cleanup.ok());
        const auto& calls = Host::state().calls;
        assert(std::count(calls.begin(), calls.end(), "timer.periodic") == 2);
        assert(std::count(calls.begin(), calls.end(), "mac.stop") == 2);
        assert(std::string(app.sessionEnd) == "demonstration complete");
    }
}
/// Start one typed operation and leave it RUNNING for finishOperation.
OperationToken runningConfigure() {
    OperationToken token{};
    LinkSettings settings{};
    settings.advertisement.technologyMask = LINK_100_FULL;
    const Status status = app.phy->startConfigure(settings, operationBudget(), token);
    assert(status.inProgress());
    return token;
}
void operationBusyRetry() {
    Host::reset();
    assert(initialize());
    const OperationToken token = runningConfigure();
    // The first poll meets a contended gate while the operation is still running.
    // That BUSY is not a terminal result; delivery must still happen.
    Host::state().failSemaphoreTakes = 1;
    const Status status = finishOperation(token);
    assert(status.ok() && !app.resultOwed);
    assert(app.lastResult.token.value == token.value && app.lastResult.status.ok());
    LifecycleRecord record{};
    assert(app.phy->lifecycleSnapshot(record).ok() && record.phase == Phase::STOPPED);
    OperationSnapshot snapshot{};
    assert(app.phy->operationSnapshot(snapshot).ok() && snapshot.state == OperationState::IDLE);
}
void operationResultOwed() {
    Host::reset();
    assert(initialize());
    const OperationToken token = runningConfigure();
    // Every gate entry fails: poll, cancel and take all exhaust their finite counts.
    Host::state().failSemaphoreTakes = 1U << 30;
    const Status status = finishOperation(token);
    Host::state().failSemaphoreTakes = 0;
    assert(status.is(Err::OPERATION_TIMEOUT));
    assert(app.resultOwed && app.owedToken.value == token.value && app.cleanup.coreStatus.is(Err::BUSY));
    LifecycleRecord record{};
    assert(app.phy->lifecycleSnapshot(record).ok() && record.phase == Phase::CONTROL);
    // Retirement is refused while CONTROL remains, and is reported, not claimed.
    finish();
    assert(app.phy->lifecycleSnapshot(record).ok() && record.phase == Phase::CONTROL);
    // The retained token still allows a later bounded disposition.
    OperationResult result{};
    assert(app.phy->cancel(token).ok() && app.phy->takeResult(token, result).ok());
    assert(app.phy->lifecycleSnapshot(record).ok() && record.phase == Phase::STOPPED);
}
void runningStopRequired() {
    Host::reset();
    assert(initialize());
    // Five service steps into the first interval, every PHY read fails. The
    // adapter quarantines, loses trust and latches stopRequired; the owner must
    // stop and must not restart.
    const auto base = Host::state().delays.size();
    mac->onCall = [base](const char* operation) {
        if (Host::state().delays.size() >= base + 5 && std::string(operation) == "mac.read")
            mac->errors["mac.read"] = ESP_FAIL;
    };
    serve();
    mac->onCall = nullptr;
    const auto& calls = Host::state().calls;
    assert(std::string(app.sessionEnd) == "adapter requires owner stop");
    assert(std::count(calls.begin(), calls.end(), "timer.periodic") == 1);
    assert(!app.primary.ok() && std::string(app.failedStage) == "running observation");
    LifecycleRecord record{};
    assert(app.phy->lifecycleSnapshot(record).ok() && record.phase == Phase::STOPPED && !record.stopRequired);
    finish();
    assert(app.phy->lifecycleSnapshot(record).ok() && record.phase == Phase::MAINTENANCE);
}
unsigned postedEvents(int32_t id) {
    const auto& events = Host::state().events;
    return static_cast<unsigned>(std::count_if(events.begin(), events.end(),
        [id](const Host::Event& event) { return event.id == id; }));
}
/// Start with the link down and raise it a few service steps into the running
/// session, so CONNECTED is published by a running poll rather than by start().
void linkUpWhileRunning() {
    mac->registers[1] = 0x7849; mac->registers[0x10] = 0;
    const auto base = Host::state().delays.size();
    mac->onCall = [base](const char* operation) {
        if (Host::state().delays.size() >= base + 3 && std::string(operation) == "mac.read") {
            mac->registers[1] = 0x786D; mac->registers[0x10] = 0x0015;
        }
    };
}
unsigned callCount(const char* name) {
    const auto& calls = Host::state().calls;
    return static_cast<unsigned>(std::count(calls.begin(), calls.end(), name));
}
void lostEventRetry() {
    Host::reset();
    assert(initialize());
    // One CONNECTED post fails after the P4 MAC accepted UP; the owner's
    // event-only retry must deliver it once, without replaying MAC UP.
    linkUpWhileRunning();
    Host::state().eventErrorId = ETHERNET_EVENT_CONNECTED;
    Host::state().nextEventError = ESP_FAIL;
    serve();
    mac->onCall = nullptr;
    // One delivered CONNECTED per running session, and MAC UP was not replayed
    // by the retry: one UP per session as well.
    assert(postedEvents(ETHERNET_EVENT_CONNECTED) == 2 && callCount("mac.up") == 2);
    assert(app.eventRetries == 0 && std::string(app.sessionEnd) == "demonstration complete");
    assert(callCount("timer.periodic") == 2);
}
void lostEventUnresolved() {
    Host::reset();
    assert(initialize());
    linkUpWhileRunning();
    Host::state().onEventAttempt = [](int32_t id) {
        if (id == ETHERNET_EVENT_CONNECTED) {
            Host::state().eventErrorId = ETHERNET_EVENT_CONNECTED;
            Host::state().nextEventError = ESP_FAIL;
        }
    };
    serve();
    Host::state().onEventAttempt = nullptr;
    mac->onCall = nullptr;
    assert(postedEvents(ETHERNET_EVENT_CONNECTED) == 0 && callCount("mac.up") == 1);
    assert(std::string(app.sessionEnd) == "link event delivery unresolved");
    assert(app.eventRetries == MAX_EVENT_RETRIES);
    assert(callCount("timer.periodic") == 1);
    LifecycleRecord record{};
    assert(app.phy->lifecycleSnapshot(record).ok() && record.phase != Phase::RUNNING);
}
void linkEvidenceAge() {
    LinkSnapshot link{};
    uint64_t age = 0;
    assert(!linkAge(link, 5000000U, age)); // No usable time.
    link.timeValid = true;
    link.observedSinceUs = 2000000U;
    link.observedAtUs = 2000001U;
    assert(linkAge(link, 5000000U, age) && age == 3000000U && age <= MAX_LINK_AGE_US);
    assert(linkAge(link, 5000001U, age) && age > MAX_LINK_AGE_US);
    assert(!linkAge(link, 1000000U, age)); // A sample from the future is unusable.
    link.observedAtUs = 1999999U;
    assert(!linkAge(link, 5000000U, age));
    link.observedAtUs = 5000001U;
    assert(!linkAge(link, 5000000U, age));
    static_assert(MAX_LINK_AGE_US == 3000000U, "three 1000 ms poll periods");
}
void timerFailure(bool duringStart) {
    Host::reset();
    assert(initialize());
    if (duringStart) Host::state().nextTimerStartError = ESP_FAIL;
    else Host::state().nextTimerStopError = ESP_FAIL;
    serve();
    finish();
    assert(app.installed && app.driver && app.glue && app.netif && mac && mac->alive);
    assert(app.primary.coreStatus.ok() && app.cleanup.coreStatus.ok());
    if (duringStart) {
        assert(std::string(app.failedStage) == "start" && app.primary.sdkError == ESP_FAIL);
        assert(app.cleanup.sdkError == ESP_ERR_INVALID_STATE);
    } else {
        // The SDK timer remained active. Its original stop failure is preserved
        // separately from the rejected attempt to arm it on restart.
        assert(std::string(app.failedStage) == "restart" && app.primary.sdkError == ESP_ERR_INVALID_STATE);
        assert(app.cleanup.sdkError == ESP_FAIL);
    }
    LifecycleRecord record{};
    assert(app.phy->lifecycleSnapshot(record).ok() && record.phase == Phase::MAINTENANCE);
}
void startMdioFailure() {
    Host::reset();
    assert(initialize());
    mac->errors["mac.read"] = ESP_FAIL;
    serve();
    finish();
    assert(app.installed && app.driver && app.glue && app.netif && mac && mac->alive);
    assert(std::string(app.failedStage) == "start" && app.primary.is(Err::MDIO_ERROR));
    assert(app.primary.coreStatus.detail == ESP_FAIL);
    assert(app.cleanup.coreStatus.ok() && app.cleanup.sdkError == ESP_ERR_INVALID_STATE);
    LifecycleRecord record{};
    assert(app.phy->lifecycleSnapshot(record).ok() && record.phase == Phase::MAINTENANCE);
}
void queuedExpectedStop() {
    Host::reset();
    assert(initialize());
    assert(app.phy->start(deadlineUs()) == ESP_OK);
    assert(ownerStop() == ESP_OK);
    assert(app.phy->start(deadlineUs()) == ESP_OK);
    assert(app.expectedStops.load() == 1 && !app.shutdownRequested.load());
    Host::deliverEvents(); // The old STOP arrives after the second START call.
    assert(app.expectedStops.load() == 0 && !app.shutdownRequested.load());
    finish();
    assert(app.installed && app.driver && app.glue && app.netif && mac && mac->alive);
}
void unexpectedStop() {
    Host::reset();
    assert(initialize());
    assert(app.phy->start(deadlineUs()) == ESP_OK);
    assert(!app.shutdownRequested.load() && app.expectedStops.load() == 0);
    // Deliver STOP from the framework during the final interval delay.
    mac->onCall = [](const char* name) {
        if (std::string(name) == "mac.read")
            onEthernet(&app, ETH_EVENT, ETHERNET_EVENT_STOP, &app.driver);
    };
    assert(!runInterval(1) && app.shutdownRequested.load());
    mac->onCall = nullptr;
    finish();
    assert(app.installed && app.driver && app.glue && app.netif && mac && mac->alive);
}

void unexpectedStopGap() {
    Host::reset();
    assert(initialize());
    Host::state().onEventPost = [](int32_t id) {
        if (id != ETHERNET_EVENT_STOP) return;
        Host::deliverEvents(); // Consume the receipt for the intentional stop.
        assert(!app.shutdownRequested.load() && app.expectedStops.load() == 0);
        onEthernet(&app, ETH_EVENT, ETHERNET_EVENT_STOP, &app.driver);
    };
    serve();
    Host::state().onEventPost = nullptr;
    assert(app.shutdownRequested.load() && callCount("timer.periodic") == 1);
    assert(std::string(app.sessionEnd) == "restart withheld");
    finish();
}

void lastEventRetry() {
    Host::reset();
    assert(initialize());
    linkUpWhileRunning();
    unsigned attempts = 0;
    Host::state().onEventAttempt = [&](int32_t id) {
        if (id != ETHERNET_EVENT_CONNECTED) return;
        ++attempts;
        if (attempts <= MAX_EVENT_RETRIES) {
            Host::state().eventErrorId = id;
            Host::state().nextEventError = ESP_FAIL;
        } else if (attempts == MAX_EVENT_RETRIES + 1) {
            Host::state().nowUs += 200001U; // Last retry delivers, just beyond its deadline.
        }
    };
    serve();
    Host::state().onEventAttempt = nullptr;
    assert(std::string(app.sessionEnd) == "demonstration complete" && app.eventRetries == 0);
    assert(postedEvents(ETHERNET_EVENT_CONNECTED) == 2 && callCount("mac.up") == 2);
    finish();
}

void negotiatingStart() {
    Host::reset();
    assert(initialize());
    mac->registers[1] = 0x784D; mac->registers[0x10] = 0x0001;
    assert(app.phy->start(deadlineUs()) == ESP_OK && callCount("mac.up") == 0);
    assert(runInterval(4));
    LinkSnapshot link;
    assert(app.phy->linkSnapshot(link).ok() && link.valid && link.linkUp && !link.resolvedModeValid);
    mac->registers[1] = 0x786D; mac->registers[0x10] = 0x0015;
    assert(runInterval(1) && callCount("mac.up") == 1);
    finish();
}
}

int main(int argc, char** argv) {
    assert(argc == 2);
    const std::string suite = argv[1];
    if (suite == "board_config") boardConfig();
    else if (suite == "gate_closed") gateClosed();
    else if (suite == "shared_loop") sharedLoop();
    else if (suite == "cleanup_failure") cleanupFailure();
    else if (suite == "mac_cleanup_failure") macCleanupFailure();
    else if (suite == "quarantined_mutex_retained") quarantinedMutexRetained();
    else if (suite == "operation_busy_retry") operationBusyRetry();
    else if (suite == "operation_result_owed") operationResultOwed();
    else if (suite == "running_stop_required") runningStopRequired();
    else if (suite == "lost_event_retry") lostEventRetry();
    else if (suite == "lost_event_unresolved") lostEventUnresolved();
    else if (suite == "last_event_retry") lastEventRetry();
    else if (suite == "unexpected_stop_gap") unexpectedStopGap();
    else if (suite == "negotiating_start") negotiatingStart();
    else if (suite == "link_evidence_age") linkEvidenceAge();
    else if (suite == "glue_allocation_failure") glueAllocationFailure();
    else if (suite == "netif_init_failure" || suite == "event_loop_failure" ||
             suite == "netif_new_failure" || suite == "mutex_failure" ||
             suite == "mac_new_failure" || suite == "factory_failure" ||
             suite == "board_failure" || suite == "board_direction_failure" || suite == "board_release_failure" ||
             suite == "driver_allocation_failure" || suite == "install_failure") acquisitionFailure(suite);
    else if (suite.rfind("glue_registration_", 0) == 0)
        glueRegistrationFailure(static_cast<unsigned>(std::stoul(suite.substr(18))));
    else if (suite.rfind("app_registration_", 0) == 0)
        appRegistrationFailure(static_cast<unsigned>(std::stoul(suite.substr(17))));
    else if (suite == "attach_partial_failure") attachPartialFailure();
    else if (suite == "emac_address_failure" || suite == "netif_address_mismatch" ||
             suite == "netif_address_read_failure") attachAddressFailure(suite);
    else if (suite == "event_burst_no_console") eventBurstWithoutConsole();
    else if (suite == "start_stop") startStop(false, false, false);
    else if (suite == "start_failure") startStop(true, false, false);
    else if (suite == "stop_failure") startStop(false, true, false);
    else if (suite == "stop_event_failure") startStop(false, false, true);
    else if (suite == "start_timer_failure") timerFailure(true);
    else if (suite == "stop_timer_failure") timerFailure(false);
    else if (suite == "start_mdio_failure") startMdioFailure();
    else if (suite == "queued_expected_stop") queuedExpectedStop();
    else if (suite == "unexpected_stop") unexpectedStop();
    else return 2;
    Host::reset();
    std::cout << suite << " passed\n";
}
