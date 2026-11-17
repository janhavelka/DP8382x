#include "InstrumentedMac.h"
#include <DP8382x/esp_idf/Phy.h>
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iostream>

namespace {
using namespace DP8382x;
using Native = DP8382x::EspIdf::Phy;
using namespace DP8382x::EspIdf;
struct Fixture {
    Host::InstrumentedMac mac;
    SharedBus bus;
    PhyConfig config;
    Native* owner = nullptr;
    esp_eth_handle_t handle = nullptr;
    Fixture() {
        bus.mutex = xSemaphoreCreateMutex();
        config.mac = &mac.native; config.bus = &bus;
        config.core.variant = Variant::DP83826I;
        config.core.interface = MacInterface::RMII;
        config.core.phyAddress = 0;
        config.core.transport.bounds = {2000, 1000, 1};
        config.notificationUs = 10; // Synthetic terminal notification allowance.
        config.maxFencePolls = 10;
        mac.extended[0x030E] = 0x8400;
        mac.registers[0x19] = 0x8001;
        mac.registers[0x1C] = 0x0100;
    }
    void create() { assert(Native::create(config, owner) == ESP_OK); }
    void board() {
        BoardControl control;
        control.setReset = [](void*, bool asserted) { Host::state().calls.emplace_back(asserted ? "board.assert" : "board.release"); return ESP_OK; };
        assert(owner->prepareBoard(control, Host::state().nowUs + 100000) == ESP_OK);
        assert(Host::state().nowUs >= 50000 && mac.frames.empty());
    }
    void install() { create(); board(); assert(owner->install(handle) == ESP_OK); assert(Host::isLive(handle) && mac.frames.empty()); }
    OperationResult finish(OperationToken token) {
        Status status{Err::IN_PROGRESS, 0, "Pending"};
        for (unsigned i = 0; i < 256 && status.is(Err::IN_PROGRESS); ++i) {
            OperationSnapshot snapshot;
            assert(owner->operationSnapshot(snapshot).ok());
            if (snapshot.nextPollValid) Host::state().nowUs = std::max(Host::state().nowUs, snapshot.nextPollAtUs);
            status = owner->pollOperation(token);
        }
        if (!status.ok()) std::cerr << "operation failure " << static_cast<int>(status.code) << " " << status.detail << "\n";
        assert(status.ok());
        OperationResult result; assert(owner->takeResult(token, result).ok()); return result;
    }
    void initialize(uint8_t technology = LINK_100_FULL) {
        OperationToken token;
        OperationBudget budget{{Host::state().nowUs + 1000000, 512}, 10, 16};
        assert(owner->startInitialize({ResetKind::NONE}, budget, token).is(Err::IN_PROGRESS));
        finish(token);
        LinkSettings settings; settings.advertisement.technologyMask = technology;
        budget.io.deadlineUs = Host::state().nowUs + 1000000;
        assert(owner->startConfigure(settings, budget, token).is(Err::IN_PROGRESS));
        finish(token);
    }
    LifecycleRecord journal() { LifecycleRecord result; assert(owner->lifecycleSnapshot(result).ok()); return result; }
    esp_err_t start() {
        const auto error = owner->start(Host::state().nowUs + 100000);
        if (error != ESP_OK) {
            auto record = journal();
            std::cerr << "start error " << error << " journal " << static_cast<int>(record.firstError.code) << ":" << record.firstError.detail << " cleanup " << record.cleanupError.detail << "\n";
            for (const auto& call : Host::state().calls) std::cerr << call << " ";
            std::cerr << "\n";
        }
        return error;
    }
};
void factory() {
    Host::reset(); Fixture f;
    auto* output = reinterpret_cast<Native*>(0x1234);
    auto invalid = f.config; invalid.core.phyAddress = 32;
    assert(Native::create(invalid, output) == ESP_ERR_INVALID_ARG && output == reinterpret_cast<Native*>(0x1234));
    const std::array<void (*)(esp_eth_mac_t&), 10> removeRequired{{
        [](esp_eth_mac_t& mac) { mac.read_phy_reg = nullptr; },
        [](esp_eth_mac_t& mac) { mac.write_phy_reg = nullptr; },
        [](esp_eth_mac_t& mac) { mac.stop = nullptr; },
        [](esp_eth_mac_t& mac) { mac.set_mediator = nullptr; },
        [](esp_eth_mac_t& mac) { mac.init = nullptr; },
        [](esp_eth_mac_t& mac) { mac.deinit = nullptr; },
        [](esp_eth_mac_t& mac) { mac.set_link = nullptr; },
        [](esp_eth_mac_t& mac) { mac.set_speed = nullptr; },
        [](esp_eth_mac_t& mac) { mac.set_duplex = nullptr; },
        [](esp_eth_mac_t& mac) { mac.set_peer_pause_ability = nullptr; }
    }};
    for (const auto remove : removeRequired) {
        auto incomplete = f.mac.native;
        remove(incomplete);
        invalid = f.config; invalid.mac = &incomplete;
        Host::state().failSemaphore = true;
        const auto created = Host::state().timerCreates;
        assert(Native::create(invalid, output) == ESP_ERR_INVALID_ARG);
        assert(output == reinterpret_cast<Native*>(0x1234));
        assert(Host::state().failSemaphore && Host::state().timerCreates == created && f.mac.frames.empty());
        Host::state().failSemaphore = false;
    }
    Host::state().failSemaphore = true;
    assert(Native::create(f.config, output) == ESP_ERR_NO_MEM && output == reinterpret_cast<Native*>(0x1234));
    Host::state().nextTimerCreateError = ESP_ERR_NO_MEM;
    assert(Native::create(f.config, output) == ESP_ERR_NO_MEM && output == reinterpret_cast<Native*>(0x1234));
    f.create();
    auto* phy = f.owner->nativePhy();
    uint32_t address = 99;
    assert(phy->get_addr(phy, &address) == ESP_OK && address == 0);
    assert(phy->set_addr(phy, 0) == ESP_OK);
    assert(phy->set_addr(phy, 1) == ESP_ERR_NOT_SUPPORTED);
    assert(phy->reset(phy) == ESP_ERR_NOT_SUPPORTED);
    assert(phy->pwrctl(phy, true) == ESP_ERR_NOT_SUPPORTED);
    assert(f.mac.frames.empty());
    assert(f.owner->destroy() == ESP_OK);
}
void boardAndInstallFailures() {
    Host::reset(); Fixture f; f.create();
    BoardControl control; control.setReset = [](void*, bool) { return ESP_ERR_TIMEOUT; };
    assert(f.owner->prepareBoard(control, 100000) == ESP_ERR_TIMEOUT);
    assert(f.bus.suspended && f.mac.frames.empty());
    auto sentinel = reinterpret_cast<esp_eth_handle_t>(0x1234);
    assert(f.owner->install(sentinel) == ESP_ERR_INVALID_STATE && sentinel == reinterpret_cast<void*>(0x1234));
    assert(f.owner->destroy() == ESP_OK);
    Host::reset(); Fixture g; g.create(); g.board();
    g.mac.errors["mac.init"] = ESP_FAIL;
    assert(g.owner->install(sentinel) == ESP_FAIL && sentinel == reinterpret_cast<void*>(0x1234));
    assert(g.owner->destroy() == ESP_OK);
}
void rawRouting() {
    Host::reset(); Fixture f; f.install();
    uint32_t value = 0xCAFE;
    esp_eth_phy_reg_rw_data_t request{2, &value};
    assert(esp_eth_ioctl(f.handle, ETH_CMD_READ_PHY_REG, &request) == ESP_ERR_NOT_SUPPORTED && value == 0xCAFE);
    assert(esp_eth_ioctl(f.handle, ETH_CMD_WRITE_PHY_REG, &request) == ESP_ERR_NOT_SUPPORTED);
    assert(f.mac.frames.empty());
    uint16_t id = 0;
    assert(f.owner->readClause22(2, id, {Host::state().nowUs + 10000, 1}).ok());
    assert(id == 0x2000 && f.mac.frames.size() == 1);
    assert(f.owner->destroy() == ESP_ERR_INVALID_STATE);
    assert(f.owner->retire(Host::state().nowUs + 100000) == ESP_ERR_NOT_SUPPORTED);
    assert(Host::isLive(f.handle) && f.mac.alive);
}
void retainedRx(bool netif) {
    Host::reset(); Fixture f; f.install(); f.initialize();
    auto* interface = netif ? Host::makeNetif() : nullptr;
    auto glue = netif ? esp_eth_new_netif_glue(f.handle) : nullptr;
    bool contextAlive = true;
    if (netif) assert(glue && esp_netif_attach(interface, glue) == ESP_OK);
    else assert(esp_eth_update_input_path(f.handle, [](esp_eth_handle_t handle, uint8_t* buffer, uint32_t, void* context) {
        assert(Host::isLive(handle) && *static_cast<bool*>(context));
        ++Host::state().appRx; std::free(buffer); return ESP_OK;
    }, &contextAlive) == ESP_OK);
    assert(f.start() == ESP_OK);
    f.mac.pauseRxAfterMediatorLoad();
    assert(Host::state().appRx == 0 && Host::state().netifRx == 0);
    assert(f.owner->retire(Host::state().nowUs + 100000) == ESP_ERR_NOT_SUPPORTED);
    Host::deliverEvents();
    assert(Host::isLive(f.handle) && f.mac.alive && contextAlive);
    if (netif) assert(Host::isLive(glue));
    assert(f.owner->destroy() == ESP_ERR_INVALID_STATE);
    assert(f.mac.resumeRx() == ESP_OK);
    assert(netif ? Host::state().netifRx == 1 : Host::state().appRx == 1);
    // The remaining owner/netif resources intentionally survive until simulated
    // process restart. No production teardown route is certified by this model.
    Host::reset(); // Simulated system restart occurs before borrowed contexts leave scope.
}
void stopFailures() {
    Host::reset(); Fixture f; f.install(); f.initialize();
    Host::state().nextEventError = ESP_ERR_TIMEOUT;
    assert(f.owner->start(Host::state().nowUs + 100000) == ESP_ERR_TIMEOUT);
    assert(Host::state().oneShotArms == 1);
    assert(f.journal().firstError.detail == ESP_ERR_TIMEOUT);
    assert(Host::isLive(f.handle) && f.mac.alive);
    Host::reset(); Fixture g; g.install(); g.initialize();
    assert(g.start() == ESP_OK);
    Host::state().holdOneShot = true;
    assert(g.owner->stop(Host::state().nowUs + 2000) == ESP_ERR_TIMEOUT);
    assert(Host::state().oneShotArms == 1 && Host::isLive(g.handle));
    Host::state().holdOneShot = false;
    const auto second = g.owner->stop(Host::state().nowUs + 100000);
    assert(second == ESP_OK || second == ESP_ERR_INVALID_STATE);
    assert(Host::state().oneShotArms == 1);
}
void partialStartFence() {
    Host::reset(); Fixture f; f.install(); f.initialize();
    f.mac.errors["mac.speed"] = ESP_ERR_TIMEOUT;
    Host::state().holdOneShot = true;
    assert(f.owner->start(Host::state().nowUs + 100000) == ESP_ERR_TIMEOUT);
    assert(Host::state().oneShotArms == 1);
    assert(std::count_if(Host::state().events.begin(), Host::state().events.end(), [](const auto& event) { return event.id == ETHERNET_EVENT_START; }) == 1);
    assert(std::none_of(Host::state().events.begin(), Host::state().events.end(), [](const auto& event) { return event.id == ETHERNET_EVENT_STOP; }));
    Host::state().holdOneShot = false;
    // A generic notification failure leaves publication uncertain; use an MDIO
    // observation failure instead for the subsequent repairable START case.
    Host::reset(); Fixture g; g.install(); g.initialize();
    // Successfully post START, publish UP, then fail before the periodic timer arms.
    Host::state().nextTimerStartError = ESP_ERR_TIMEOUT;
    Host::state().holdOneShot = true;
    assert(g.owner->start(Host::state().nowUs + 100000) == ESP_ERR_TIMEOUT);
    assert(Host::state().oneShotArms == 1);
    assert(std::none_of(Host::state().events.begin(), Host::state().events.end(), [](const auto& event) { return event.id == ETHERNET_EVENT_STOP; }));
    Host::state().holdOneShot = false;
    assert(g.owner->stop(Host::state().nowUs + 100000) == ESP_OK);
    assert(Host::state().oneShotArms == 1);
    assert(std::count_if(Host::state().events.begin(), Host::state().events.end(), [](const auto& event) { return event.id == ETHERNET_EVENT_STOP; }) == 1);
    assert(!g.journal().publishedTuple.up && Host::isLive(g.handle));
}
void notifications() {
    Host::reset(); Fixture f; f.install(); f.initialize(LINK_TECHNOLOGY_MASK);
    assert(f.start() == ESP_OK);
    auto count = [](const char* value) { return std::count(Host::state().calls.begin(), Host::state().calls.end(), value); };
    assert(count("mac.up") == 1 && count("mac.speed") == 1 && count("mac.duplex") == 1 && count("mac.pause") == 1);
    Host::tickTimers();
    assert(count("mac.up") == 1 && count("mac.speed") == 1);
    f.mac.registers[0x10] = 0x0017; // Valid 10/full tuple under all-mode local advertisement.
    const auto offset = Host::state().calls.size();
    Host::tickTimers();
    std::vector<std::string> notices;
    for (size_t i = offset; i < Host::state().calls.size(); ++i) {
        const auto& call = Host::state().calls[i];
        if (call == "mac.down" || call == "mac.speed" || call == "mac.duplex" || call == "mac.pause" || call == "mac.up") notices.push_back(call);
    }
    assert((notices == std::vector<std::string>{"mac.down", "mac.speed", "mac.duplex", "mac.pause", "mac.up"}));
    const auto speeds = count("mac.speed");
    f.mac.registers[1] = 0x7849; f.mac.registers[0x10] = 0;
    Host::tickTimers();
    assert(!f.journal().publishedTuple.up && count("mac.speed") == speeds);
    assert(f.journal().publishedTuple.speed == LinkSpeed::MBPS_10); // Last accepted value retained, no invented DOWN mode.
}
void notificationFailures() {
    for (const auto* operation : {"mac.speed", "mac.duplex", "mac.pause", "mac.up"}) {
        Host::reset(); Fixture f; f.install(); f.initialize();
        f.mac.errors[operation] = ESP_ERR_TIMEOUT;
        assert(f.owner->start(Host::state().nowUs + 100000) == ESP_ERR_TIMEOUT);
        assert(f.journal().publicationUncertain && f.journal().firstError.detail == ESP_ERR_TIMEOUT);
        assert(Host::isLive(f.handle) && f.mac.alive && f.owner->destroy() == ESP_ERR_INVALID_STATE);
    }
}
void lostLinkEvent() {
    Host::reset(); Fixture f;
    f.config.pinnedP4Mac = true; // Instrument the pinned valid-LINK infallibility policy.
    f.install(); f.initialize();
    f.mac.registers[1] = 0x7849; f.mac.registers[0x10] = 0;
    assert(f.start() == ESP_OK);
    f.mac.registers[1] = 0x786D; f.mac.registers[0x10] = 0x0015;
    Host::state().eventErrorId = ETHERNET_EVENT_CONNECTED;
    Host::state().nextEventError = ESP_ERR_TIMEOUT;
    Host::tickTimers();
    auto record = f.journal();
    assert(record.publishedTuple.up && record.eventLost && !record.publicationUncertain);
    const auto ups = std::count(Host::state().calls.begin(), Host::state().calls.end(), "mac.up");
    assert(f.owner->retryLinkEvent(Host::state().nowUs + 100000) == ESP_OK);
    assert(!f.journal().eventLost);
    assert(std::count(Host::state().calls.begin(), Host::state().calls.end(), "mac.up") == ups);
    // A successful opposite publication supersedes an earlier lost event.
    f.mac.registers[1] = 0x7849; f.mac.registers[0x10] = 0;
    Host::state().eventErrorId = ETHERNET_EVENT_DISCONNECTED;
    Host::state().nextEventError = ESP_ERR_TIMEOUT;
    Host::tickTimers();
    assert(f.journal().eventLost);
    f.mac.registers[1] = 0x786D; f.mac.registers[0x10] = 0x0015;
    Host::tickTimers();
    assert(!f.journal().eventLost);
}
void sharedQuarantine() {
    Host::reset(); Fixture f; Fixture g;
    g.config.bus = &f.bus;
    f.install(); g.install();
    uint16_t output = 0xCAFE;
    f.mac.errors["mac.read"] = ESP_ERR_TIMEOUT;
    const auto first = f.owner->readClause22(2, output, {Host::state().nowUs + 10000, 1});
    assert(first.detail == ESP_ERR_TIMEOUT && output == 0xCAFE && f.bus.quarantined);
    const auto second = g.owner->readClause22(2, output, {Host::state().nowUs + 10000, 1});
    assert(second.is(Err::BUSY) && output == 0xCAFE && g.mac.frames.empty());
}
void callbackBounds() {
    Host::reset(); Fixture f; f.config.notificationUs = 40000;
    f.install(); f.initialize(); const auto frames = f.mac.frames.size();
    assert(f.owner->start(Host::state().nowUs + 100000) == ESP_ERR_TIMEOUT);
    assert(f.mac.frames.size() == frames + 3); // Restart only; no link reads without full notification allowance.
    assert(std::count(Host::state().calls.begin(), Host::state().calls.end(), "mac.speed") == 0);
    Host::reset(); Fixture g; g.install(); g.initialize();
    g.mac.elapsedUs["mac.speed"] = 100001;
    assert(g.owner->start(Host::state().nowUs + 100000) == ESP_ERR_TIMEOUT);
    const auto record = g.journal();
    assert(record.publishedTuple.speedAccepted && record.publishedTuple.speed == LinkSpeed::MBPS_100);
    assert(!record.publishedTuple.duplexAccepted && !record.publishedTuple.up);
    assert(record.firstError.detail == ESP_ERR_TIMEOUT && Host::isLive(g.handle));
}
void forbiddenLifecycle() {
    Host::reset(); Fixture f; f.create();
    auto* phy = f.owner->nativePhy();
    assert(phy->init(phy) == ESP_ERR_INVALID_STATE);
    assert(phy->reset_hw(phy) == ESP_ERR_INVALID_STATE);
    assert(phy->set_link(phy, ETH_LINK_DOWN) == ESP_ERR_INVALID_STATE);
    assert(f.owner->destroy() == ESP_OK);
    Host::reset(); Fixture g; g.install();
    OperationToken token;
    assert(g.owner->startInitialize({ResetKind::NONE}, {{Host::state().nowUs + 1000000, 512}, 10, 16}, token).is(Err::IN_PROGRESS));
    assert(g.owner->retire(Host::state().nowUs + 100000) == ESP_ERR_INVALID_STATE);
    g.finish(token);
    assert(esp_eth_driver_uninstall(g.handle) == ESP_ERR_INVALID_STATE);
    assert(Host::isLive(g.handle));
    const auto deleted = Host::state().timerDeletes;
    assert(g.owner->stop(Host::state().nowUs + 100000) == ESP_ERR_INVALID_STATE);
    assert(g.owner->destroy() == ESP_ERR_INVALID_STATE);
    assert(Host::state().timerDeletes == deleted);
}
void discovery() {
    Host::reset(); Fixture f; f.install();
    DiscoverySnapshot output; output.requestedMask = 0xCAFE; output.framesAttempted = 99;
    assert(f.owner->discover(0, output, {Host::state().nowUs + 1000000, 64}).is(Err::INVALID_PARAM));
    assert(f.owner->discover(1, output, {Host::state().nowUs + 1000000, 0}).is(Err::OPERATION_TIMEOUT));
    assert(f.owner->discover(1, output, {Host::state().nowUs, 64}).is(Err::OPERATION_TIMEOUT));
    assert(f.mac.frames.empty() && output.requestedMask == 0xCAFE && output.framesAttempted == 99);
    assert(f.owner->discover(UINT32_MAX, output, {Host::state().nowUs + 1000000, 64}).ok());
    assert(output.requestedMask == UINT32_MAX && output.framesAttempted == 64);
    for (unsigned address = 0; address < 32; ++address) {
        assert(output.entries[address].id1Valid && output.entries[address].id2Valid);
        assert(output.entries[address].id1 == 0x2000 && output.entries[address].id2 == 0xA131);
        assert(f.mac.frames[address * 2].address == address && f.mac.frames[address * 2].reg == 2);
        assert(f.mac.frames[address * 2 + 1].address == address && f.mac.frames[address * 2 + 1].reg == 3);
    }
    uint32_t selected = 99;
    assert(f.owner->nativePhy()->get_addr(f.owner->nativePhy(), &selected) == ESP_OK && selected == 0);
    f.mac.readResults = {ESP_OK, ESP_ERR_TIMEOUT};
    output.requestedMask = 0xCAFE; output.framesAttempted = 99;
    assert(f.owner->discover(1, output, {Host::state().nowUs + 100000, 2}).detail == ESP_ERR_TIMEOUT);
    assert(output.requestedMask == 0xCAFE && output.framesAttempted == 99);
    DiscoverySnapshot cached;
    assert(f.owner->discoverySnapshot(cached).ok());
    assert(cached.framesAttempted == 2 && cached.entries[0].id1Valid && !cached.entries[0].id2Valid);
    assert(cached.entries[0].id1 == 0x2000 && cached.status.detail == ESP_ERR_TIMEOUT && f.bus.quarantined);
    Host::reset(); Fixture g; g.install();
    g.mac.elapsedUs["mac.read"] = 10000;
    assert(g.owner->discover(3, output, {Host::state().nowUs + 9000, 4}).is(Err::OPERATION_TIMEOUT));
    assert(g.mac.frames.size() == 1);
    assert(g.owner->discoverySnapshot(cached).ok() && cached.framesAttempted == 1 && cached.entries[0].id1Valid);
}
void discoveryAdmission() {
    Host::reset(); Fixture f;
    f.config.core.transport.bounds.frameUs = UINT32_C(0x80000000);
    f.install();
    DiscoverySnapshot previous;
    assert(f.owner->discover(1, previous, {Host::state().nowUs + UINT64_C(10000000000), 2}).ok());
    const auto frames = f.mac.frames.size();
    DiscoverySnapshot output; output.requestedMask = 0xCAFE;
    assert(f.owner->discover(3, output, {Host::state().nowUs + 10000, 4}).is(Err::OPERATION_TIMEOUT));
    assert(f.mac.frames.size() == frames && output.requestedMask == 0xCAFE);
    DiscoverySnapshot cached;
    assert(f.owner->discoverySnapshot(cached).ok() && cached.requestedMask == 1 && cached.framesAttempted == 2);
    Host::reset(); Fixture g; g.install();
    uint16_t id = 0;
    assert(g.owner->readClause22(2, id, {Host::state().nowUs + 10000, 1}).ok());
    const auto before = g.mac.frames.size();
    Host::state().nowUs = 1;
    assert(g.owner->readClause22(2, id, {100000, 1}).detail == static_cast<int32_t>(ConfigDetail::CLOCK_CONTRACT));
    Host::state().nowUs = 100000;
    assert(g.owner->discover(1, output, {200000, 2}).detail == static_cast<int32_t>(ConfigDetail::CLOCK_CONTRACT));
    assert(g.mac.frames.size() == before && output.requestedMask == 0xCAFE);
}
void terminalEventRetry() {
    Host::reset(); Fixture f; f.config.pinnedP4Mac = true;
    f.install(); f.initialize();
    f.mac.registers[1] = 0x7849; f.mac.registers[0x10] = 0;
    assert(f.start() == ESP_OK);
    f.mac.registers[1] = 0x786D; f.mac.registers[0x10] = 0x0015;
    Host::state().eventErrorId = ETHERNET_EVENT_CONNECTED;
    Host::state().nextEventError = ESP_ERR_TIMEOUT;
    Host::tickTimers();
    assert(f.journal().publishedTuple.up && f.journal().eventLost);
    assert(f.owner->retire(Host::state().nowUs + 1) == ESP_ERR_NOT_SUPPORTED);
    assert(f.journal().publishedTuple.up && f.journal().eventLost);
    const auto events = Host::state().events.size();
    const auto ups = std::count(Host::state().calls.begin(), Host::state().calls.end(), "mac.up");
    assert(f.owner->retryLinkEvent(Host::state().nowUs + 100000) == ESP_ERR_INVALID_STATE);
    assert(Host::state().events.size() == events && std::count(Host::state().calls.begin(), Host::state().calls.end(), "mac.up") == ups);
    assert(Host::isLive(f.handle));
    Host::reset(); Fixture g; g.install(); g.initialize();
    assert(g.start() == ESP_OK);
    const auto frames = g.mac.frames.size();
    const auto calls = Host::state().calls.size();
    assert(g.owner->retire(Host::state().nowUs) == ESP_ERR_NOT_SUPPORTED);
    const auto record = g.journal();
    assert(record.firstError.ok() && record.cleanupError.detail == ESP_ERR_TIMEOUT);
    assert(g.mac.frames.size() == frames && Host::state().calls.size() == calls);
    assert(Host::isLive(g.handle) && g.mac.alive && g.owner->destroy() == ESP_ERR_INVALID_STATE);
}
void installerContention() {
    for (const auto expected : {ESP_OK, ESP_ERR_TIMEOUT}) {
        Host::reset(); Fixture f; f.create(); f.board();
        f.mac.onCall = [&](const char* operation) {
            if (std::string(operation) != "mac.init") return;
            Host::state().failSemaphoreTakes = 2;
            auto* phy = f.owner->nativePhy(); bool enabled = false;
            // Installer callbacks remain local and infallible even while every
            // attempted snapshot/gated owner entry would be denied.
            assert(phy->init(phy) == ESP_OK && phy->reset_hw(phy) == ESP_OK);
            assert(phy->autonego_ctrl(phy, ESP_ETH_PHY_AUTONEGO_G_STAT, &enabled) == ESP_OK && enabled);
        };
        f.mac.errors["mac.init"] = expected;
        auto output = reinterpret_cast<esp_eth_handle_t>(0x1234);
        assert(f.owner->install(output) == expected);
        assert(expected == ESP_OK ? Host::isLive(output) : output == reinterpret_cast<void*>(0x1234));
        assert(Host::state().failSemaphoreTakes == 2 && f.mac.frames.empty());
        LifecycleRecord record; record.eventLost = true;
        assert(f.owner->lifecycleSnapshot(record).is(Err::BUSY) && record.eventLost);
        Host::state().failSemaphoreTakes = 0;
        assert(f.owner->lifecycleSnapshot(record).ok());
        assert(expected == ESP_OK ? record.firstError.ok() : record.firstError.detail == expected);
        assert(f.owner->install(output) == ESP_ERR_INVALID_STATE);
        assert(f.owner->destroy() == (expected == ESP_OK ? ESP_ERR_INVALID_STATE : ESP_OK));
    }
}
}
int main(int argc, char** argv) {
    assert(argc == 2); const std::string suite = argv[1];
    if (suite == "factory") factory();
    else if (suite == "board_install_failures") boardAndInstallFailures();
    else if (suite == "raw_routing") rawRouting();
    else if (suite == "raw_rx_retention") retainedRx(false);
    else if (suite == "glue_rx_retention") retainedRx(true);
    else if (suite == "stop_failures") stopFailures();
    else if (suite == "partial_start_fence") partialStartFence();
    else if (suite == "notifications") notifications();
    else if (suite == "notification_failures") notificationFailures();
    else if (suite == "lost_link_event") lostLinkEvent();
    else if (suite == "shared_quarantine") sharedQuarantine();
    else if (suite == "callback_bounds") callbackBounds();
    else if (suite == "forbidden_lifecycle") forbiddenLifecycle();
    else if (suite == "discovery") discovery();
    else if (suite == "discovery_admission") discoveryAdmission();
    else if (suite == "terminal_event_retry") terminalEventRetry();
    else if (suite == "installer_contention") installerContention();
    else return 2;
    Host::reset(); std::cout << suite << " passed\n";
}
