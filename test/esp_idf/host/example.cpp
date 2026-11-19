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
const char* esp_err_to_name(esp_err_t error) {
    return error == ESP_OK ? "ESP_OK" : "INJECTED_OR_SDK_ERROR";
}
esp_err_t esp_netif_init(void) {
    Host::state().calls.emplace_back("netif.init");
    return faults.netifInit;
}
esp_netif_t* esp_netif_new(const esp_netif_config_t*) {
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
esp_err_t gpio_set_level(gpio_num_t, uint32_t) {
    Host::state().calls.emplace_back("gpio.level");
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
}
void cleanupFailure() {
    Host::reset();
    faults.nullNetif = true;
    faults.eventLoopDelete = ESP_FAIL;
    run(true);
    assert(!app.installed && app.primary.sdkError == ESP_ERR_NO_MEM);
    assert(app.cleanup.sdkError == ESP_FAIL && app.loopOwned && faults.eventLoopOwned);
}
void macCleanupFailure() {
    Host::reset();
    Host::state().nextTimerCreateError = ESP_ERR_NO_MEM;
    faults.macErrorOperation = "mac.del";
    faults.macError = ESP_FAIL;
    run(true);
    assert(!app.installed && app.primary.sdkError == ESP_ERR_NO_MEM);
    assert(app.cleanup.sdkError == ESP_FAIL && app.mac && app.bus.mutex);
    assert(Host::activeNetifs() == 0 && !app.loopOwned);
}
void glueAllocationFailure() {
    Host::reset();
    Host::state().failAllocationNumber = 2;
    run(true);
    assert(app.installed && app.primary.sdkError == ESP_ERR_NO_MEM);
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
void eventBurstWithoutConsole() {
    Host::reset();
    app.driver = reinterpret_cast<esp_eth_handle_t>(0x1234);
    app.netif = Host::makeNetif();
    app.startDesired.store(true);
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
    if (startFailure || stopFailure || stopEventFailure) assert(!app.primary.ok() || !app.cleanup.ok());
    else assert(app.primary.ok() && app.cleanup.ok());
    if (!startFailure && !stopFailure && !stopEventFailure) {
        const auto& calls = Host::state().calls;
        assert(std::count(calls.begin(), calls.end(), "timer.periodic") == 2);
        assert(std::count(calls.begin(), calls.end(), "mac.stop") == 2);
    }
    LifecycleRecord record{};
    assert(app.phy->lifecycleSnapshot(record).ok());
    assert(record.phase == Phase::MAINTENANCE || record.phase == Phase::STOPPING);
}
void timerFailure(bool duringStart) {
    Host::reset();
    assert(initialize());
    if (duringStart) Host::state().nextTimerStartError = ESP_FAIL;
    else Host::state().nextTimerStopError = ESP_FAIL;
    serve();
    finish();
    assert(app.installed && app.driver && app.glue && app.netif && mac && mac->alive);
    assert(!app.primary.ok() || !app.cleanup.ok());
}
void startMdioFailure() {
    Host::reset();
    assert(initialize());
    mac->errors["mac.read"] = ESP_FAIL;
    serve();
    finish();
    assert(app.installed && app.driver && app.glue && app.netif && mac && mac->alive);
    assert(!app.primary.ok() || !app.cleanup.ok());
}
void queuedExpectedStop() {
    Host::reset();
    assert(initialize());
    app.startDesired.store(true);
    assert(app.phy->start(deadlineUs()) == ESP_OK);
    app.startDesired.store(false);
    app.expectedStops.store(1);
    assert(app.phy->stop(deadlineUs()) == ESP_OK);
    app.startDesired.store(true);
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
    app.startDesired.store(true);
    assert(app.phy->start(deadlineUs()) == ESP_OK);
    assert(!app.shutdownRequested.load() && app.expectedStops.load() == 0);
    onEthernet(&app, ETH_EVENT, ETHERNET_EVENT_STOP, &app.driver);
    assert(app.shutdownRequested.load());
    finish();
    assert(app.installed && app.driver && app.glue && app.netif && mac && mac->alive);
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
    else if (suite == "glue_allocation_failure") glueAllocationFailure();
    else if (suite == "netif_init_failure" || suite == "event_loop_failure" ||
             suite == "netif_new_failure" || suite == "mutex_failure" ||
             suite == "mac_new_failure" || suite == "factory_failure" ||
             suite == "board_failure" || suite == "board_direction_failure" ||
             suite == "driver_allocation_failure" || suite == "install_failure") acquisitionFailure(suite);
    else if (suite.rfind("glue_registration_", 0) == 0)
        glueRegistrationFailure(static_cast<unsigned>(std::stoul(suite.substr(18))));
    else if (suite.rfind("app_registration_", 0) == 0)
        appRegistrationFailure(static_cast<unsigned>(std::stoul(suite.substr(17))));
    else if (suite == "attach_partial_failure") attachPartialFailure();
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
