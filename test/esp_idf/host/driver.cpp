#include "InstrumentedMac.h"
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iostream>

namespace {
struct Phy {
    esp_eth_phy_t native{};
    esp_eth_mediator_t* mediator = nullptr;
    esp_err_t initError = ESP_OK;
    esp_err_t linkError = ESP_OK;
    bool autoNego = false;
    unsigned linkCalls = 0;
    unsigned downCalls = 0;
    Phy() {
        native.set_mediator = [](esp_eth_phy_t* p, esp_eth_mediator_t* m) { self(p).mediator = m; Host::state().calls.emplace_back("phy.mediator"); return ESP_OK; };
        native.reset_hw = [](esp_eth_phy_t*) { Host::state().calls.emplace_back("phy.reset_hw"); return ESP_OK; };
        native.init = [](esp_eth_phy_t* p) { Host::state().calls.emplace_back("phy.init"); return self(p).initError; };
        native.deinit = [](esp_eth_phy_t*) { Host::state().calls.emplace_back("phy.deinit"); return ESP_OK; };
        native.autonego_ctrl = [](esp_eth_phy_t* p, eth_phy_autoneg_cmd_t, bool* out) { *out = self(p).autoNego; return ESP_OK; };
        native.get_link = [](esp_eth_phy_t* p) { ++self(p).linkCalls; return self(p).linkError; };
        native.set_link = [](esp_eth_phy_t* p, eth_link_t link) { ++self(p).downCalls; return self(p).mediator->on_state_changed(self(p).mediator, ETH_STATE_LINK, reinterpret_cast<void*>(static_cast<uintptr_t>(link))); };
        native.get_addr = [](esp_eth_phy_t*, uint32_t* out) { *out = 0; return ESP_OK; };
    }
    static Phy& self(esp_eth_phy_t* p) { return *reinterpret_cast<Phy*>(p); }
};
esp_err_t rejectRead(esp_eth_handle_t, uint32_t, uint32_t, uint32_t*) { return ESP_ERR_NOT_SUPPORTED; }
esp_err_t rejectWrite(esp_eth_handle_t, uint32_t, uint32_t, uint32_t) { return ESP_ERR_NOT_SUPPORTED; }
esp_eth_handle_t install(Host::InstrumentedMac& mac, Phy& phy) {
    esp_eth_config_t config = ETH_DEFAULT_CONFIG(&mac.native, &phy.native);
    config.read_phy_reg = rejectRead; config.write_phy_reg = rejectWrite;
    esp_eth_handle_t handle = nullptr;
    assert(esp_eth_driver_install(&config, &handle) == ESP_OK);
    return handle;
}
void installFailures() {
    Host::reset(); Host::InstrumentedMac mac; Phy phy;
    esp_eth_config_t config = ETH_DEFAULT_CONFIG(&mac.native, &phy.native);
    const auto sentinel = reinterpret_cast<esp_eth_handle_t>(0x1234);
    auto handle = sentinel;
    Host::state().failAllocation = true;
    assert(esp_eth_driver_install(&config, &handle) == ESP_ERR_NO_MEM && handle == sentinel);
    Host::state().nextTimerCreateError = ESP_ERR_NO_MEM;
    assert(esp_eth_driver_install(&config, &handle) == ESP_ERR_NO_MEM && handle == sentinel);
    mac.errors["mac.init"] = ESP_FAIL;
    assert(esp_eth_driver_install(&config, &handle) == ESP_FAIL && handle == sentinel);
    phy.initError = ESP_ERR_TIMEOUT;
    assert(esp_eth_driver_install(&config, &handle) == ESP_ERR_TIMEOUT && handle == sentinel);
    assert(Host::state().timerCreates == Host::state().timerDeletes);
}
void routing() {
    Host::reset(); Host::InstrumentedMac mac; Phy phy; auto handle = install(mac, phy);
    uint32_t value = 0xCAFE;
    esp_eth_phy_reg_rw_data_t request{0, &value};
    assert(esp_eth_ioctl(handle, ETH_CMD_READ_PHY_REG, &request) == ESP_ERR_NOT_SUPPORTED);
    assert(value == 0xCAFE && mac.frames.empty());
    assert(esp_eth_ioctl(handle, ETH_CMD_WRITE_PHY_REG, &request) == ESP_ERR_NOT_SUPPORTED);
    assert(mac.frames.empty());
    assert(esp_eth_driver_uninstall(handle) == ESP_OK);
}
void failedStart() {
    Host::reset(); Host::InstrumentedMac mac; Phy phy; auto handle = install(mac, phy);
    phy.linkError = ESP_ERR_TIMEOUT;
    assert(esp_eth_start(handle) == ESP_ERR_TIMEOUT);
    assert(Host::state().events.size() == 1 && Host::state().events[0].id == ETHERNET_EVENT_START);
    assert(esp_eth_stop(handle) == ESP_ERR_INVALID_STATE); // unarmed link timer
    assert(phy.downCalls == 0 && Host::state().events.size() == 1);
    // FSM was already set STOP even though esp_eth_stop returned INVALID_STATE.
    assert(esp_eth_driver_uninstall(handle) == ESP_OK);
}
void stopEvents() {
    Host::reset(); Host::InstrumentedMac mac; Phy phy; auto handle = install(mac, phy);
    assert(esp_eth_start(handle) == ESP_OK);
    assert(mac.mediator->on_state_changed(mac.mediator, ETH_STATE_LINK, reinterpret_cast<void*>(ETH_LINK_UP)) == ESP_OK);
    assert(esp_eth_stop(handle) == ESP_OK);
    assert(phy.downCalls == 1 && mac.link == ETH_LINK_DOWN);
    assert(Host::state().events.back().id == ETHERNET_EVENT_STOP);
    assert(esp_eth_driver_uninstall(handle) == ESP_OK);
}
void timerSemantics() {
    Host::reset();
    unsigned callbacks = 0;
    esp_timer_create_args_t args{};
    args.callback = [](void* context) { ++*static_cast<unsigned*>(context); };
    args.arg = &callbacks; args.dispatch_method = ESP_TIMER_TASK;
    esp_timer_handle_t timer = nullptr;
    assert(esp_timer_create(&args, &timer) == ESP_OK);
    assert(esp_timer_stop(timer) == ESP_ERR_INVALID_STATE);
    assert(esp_timer_start_once(timer, 0) == ESP_OK);
    assert(esp_timer_is_active(timer));
    assert(esp_timer_start_once(timer, 0) == ESP_ERR_INVALID_STATE);
    vTaskDelay(1);
    assert(!esp_timer_is_active(timer) && callbacks == 1);
    assert(esp_timer_stop(timer) == ESP_ERR_INVALID_STATE);
    assert(esp_timer_delete(timer) == ESP_OK);
}
void rxLifetime(bool netif) {
    Host::reset(); Host::InstrumentedMac mac; Phy phy; auto handle = install(mac, phy);
    esp_netif_t* interface = nullptr;
    esp_eth_netif_glue_handle_t glue = nullptr;
    bool contextAlive = true;
    if (netif) {
        interface = Host::makeNetif();
        glue = esp_eth_new_netif_glue(handle);
        assert(glue && Host::isLive(glue));
        assert(esp_netif_attach(interface, glue) == ESP_OK);
    } else {
        assert(esp_eth_update_input_path(handle, [](esp_eth_handle_t h, uint8_t* buffer, uint32_t, void* context) {
            assert(Host::isLive(h) && *static_cast<bool*>(context));
            ++Host::state().appRx; std::free(buffer); return ESP_OK;
        }, &contextAlive) == ESP_OK);
    }
    assert(esp_eth_start(handle) == ESP_OK);
    mac.pauseRxAfterMediatorLoad();
    assert(Host::state().appRx == 0 && Host::state().netifRx == 0);
    assert(esp_eth_stop(handle) == ESP_OK);
    Host::deliverEvents();
    // Stop delivery and a zero application callback count do not authorize free.
    // Retain the driver, MAC, PHY, glue, netif and callback context across resume.
    assert(Host::isLive(handle) && mac.alive && contextAlive);
    if (netif) assert(Host::isLive(glue));
    assert(mac.resumeRx() == ESP_OK);
    assert(netif ? Host::state().netifRx == 1 : Host::state().appRx == 1);
    // Only the harness model has a fully controlled RX task. Production remains
    // blocked on native_rx_quiescence and must retain resources until restart.
    if (glue) assert(esp_eth_del_netif_glue(glue) == ESP_OK);
    if (interface) Host::destroyNetif(interface);
    assert(esp_eth_driver_uninstall(handle) == ESP_OK);
}
}
int main(int argc, char** argv) {
    assert(argc == 2);
    const std::string suite = argv[1];
    if (suite == "install_failures") installFailures();
    else if (suite == "raw_ioctl") routing();
    else if (suite == "failed_start") failedStart();
    else if (suite == "stop_events") stopEvents();
    else if (suite == "timer") timerSemantics();
    else if (suite == "raw_rx_lifetime") rxLifetime(false);
    else if (suite == "glue_rx_lifetime") rxLifetime(true);
    else return 2;
    Host::reset();
    std::cout << suite << " passed\n";
}
