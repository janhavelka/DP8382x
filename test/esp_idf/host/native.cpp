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
        assert(owner->reconcileSdkState(Host::state().nowUs + 100000).ok());
    }
    void configure(const LinkSettings& settings) {
        OperationToken token;
        assert(owner->startConfigure(settings, ConfigureOptions{}, {{Host::state().nowUs + 1000000, 512}, 10, 16}, token).inProgress());
        const auto result = finish(token);
        assert(result.status.ok());
        assert(owner->reconcileSdkState(Host::state().nowUs + 100000).ok());
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
    assert(f.bus.admission == BusAdmission::FAILED_CLOSED && f.mac.frames.empty());
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
    const auto downs = std::count(Host::state().calls.begin(), Host::state().calls.end(), "mac.down");
    f.mac.registers[1] = 0x786D; f.mac.registers[0x10] = 0x0015;
    Host::tickTimers();
    assert(!f.journal().eventLost);
    assert(std::count(Host::state().calls.begin(), Host::state().calls.end(), "mac.down") == downs);
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
std::vector<std::string> noticesSince(size_t offset) {
    std::vector<std::string> result;
    for (size_t i = offset; i < Host::state().calls.size(); ++i) {
        const auto& call = Host::state().calls[i];
        if (call == "mac.down" || call == "mac.speed" || call == "mac.duplex" || call == "mac.pause" || call == "mac.up") result.push_back(call);
    }
    return result;
}
void liveTupleChanges() {
    Host::reset(); Fixture f; f.install(); f.initialize(LINK_TECHNOLOGY_MASK);
    assert(f.start() == ESP_OK);
    struct Case { uint16_t physts; uint16_t partner; eth_speed_t speed; eth_duplex_t duplex; uint32_t pause; };
    const Case cases[] = {
        {0x0017, 0x01E1, ETH_SPEED_10M, ETH_DUPLEX_FULL, 0}, // Speed only.
        {0x0013, 0x01E1, ETH_SPEED_10M, ETH_DUPLEX_HALF, 0}, // Duplex only.
        {0x0015, 0x01E1, ETH_SPEED_100M, ETH_DUPLEX_FULL, 0},
        {0x0015, 0x05E1, ETH_SPEED_100M, ETH_DUPLEX_FULL, 1}, // Peer pause only.
        {0x0015, 0x01E1, ETH_SPEED_100M, ETH_DUPLEX_FULL, 0}
    };
    for (const auto& row : cases) {
        f.mac.registers[0x10] = row.physts; f.mac.registers[5] = row.partner;
        const auto offset = Host::state().calls.size();
        Host::tickTimers();
        assert((noticesSince(offset) == std::vector<std::string>{"mac.down", "mac.speed", "mac.duplex", "mac.pause", "mac.up"}));
        assert(f.mac.speedRequests.back() == row.speed && f.mac.duplexRequests.back() == row.duplex && f.mac.pauseRequests.back() == row.pause);
        auto record = f.journal();
        assert(record.publishedTuple.up && record.publishedTuple.speedAccepted && record.publishedTuple.duplexAccepted && record.publishedTuple.pauseAccepted);
        eth_speed_t speed; eth_duplex_t duplex;
        assert(esp_eth_ioctl(f.handle, ETH_CMD_G_SPEED, &speed) == ESP_OK && speed == row.speed);
        assert(esp_eth_ioctl(f.handle, ETH_CMD_G_DUPLEX_MODE, &duplex) == ESP_OK && duplex == row.duplex);
        const auto unchanged = Host::state().calls.size();
        Host::tickTimers();
        assert(noticesSince(unchanged).empty());
    }
    const auto offset = Host::state().calls.size();
    assert(f.owner->stop(Host::state().nowUs + 100000) == ESP_OK);
    assert((noticesSince(offset) == std::vector<std::string>{"mac.down"}));
    const auto restarted = Host::state().calls.size();
    assert(f.start() == ESP_OK);
    assert((noticesSince(restarted) == std::vector<std::string>{"mac.speed", "mac.duplex", "mac.pause", "mac.up"}));
}
void incompleteNegotiation() {
    Host::reset(); Fixture f; f.install(); f.initialize();
    f.mac.registers[1] = 0x784D; f.mac.registers[0x10] = 0x0001; // Link up, AN incomplete.
    const auto offset = Host::state().calls.size();
    assert(f.owner->start(Host::state().nowUs + 100000) == ESP_ERR_INVALID_STATE);
    assert(noticesSince(offset).empty());
    const auto record = f.journal();
    assert(!record.publishedTuple.up && !record.publishedTuple.speedAccepted && !record.publishedTuple.duplexAccepted);
    assert(record.publishedTuple.speed == LinkSpeed::UNKNOWN && record.publishedTuple.duplex == Duplex::UNKNOWN);
    LinkSnapshot observation;
    const auto frames = f.mac.frames.size();
    assert(f.owner->linkSnapshot(observation).ok());
    assert(observation.valid && observation.linkUp && !observation.resolvedModeValid && observation.negotiation == NegotiationState::IN_PROGRESS);
    assert(f.mac.frames.size() == frames); // Acquired observations and accepted SDK fields have different validity.
}
void pauseProjection() {
    struct Case { bool localSym; bool localAsym; uint16_t partner; uint16_t phycr; bool rx; bool tx; uint32_t peer; };
    const Case cases[] = {
        {false, false, 0x0501, 0x8001, false, false, 1},
        {true, true, 0x0901, 0xA001, true, false, 0},
        {false, true, 0x0D01, 0x9001, false, true, 1},
        {true, false, 0x0501, 0xB001, true, true, 1}
    };
    for (const auto& row : cases) {
        Host::reset(); Fixture f; f.install(); f.initialize();
        LinkSettings settings; settings.advertisement = {LINK_100_FULL, row.localSym, row.localAsym};
        f.configure(settings);
        f.mac.registers[5] = row.partner; f.mac.registers[0x19] = row.phycr;
        assert(f.start() == ESP_OK);
        LinkSnapshot observation;
        assert(f.owner->linkSnapshot(observation).ok());
        assert(observation.pauseValid && observation.pauseRx == row.rx && observation.pauseTx == row.tx);
        assert(observation.rawAnlpar == row.partner && observation.rawPhycr == row.phycr);
        assert(f.mac.pauseRequests.back() == row.peer && f.journal().publishedTuple.peerPause == (row.peer != 0));
        assert(!f.mac.flowEnabled && f.mac.flowRequests.empty()); // Publication cannot choose application MAC policy.
    }
}
void notificationPartialReceipts() {
    const char* operations[] = {"mac.speed", "mac.duplex", "mac.pause", "mac.up"};
    for (unsigned failed = 0; failed < 4; ++failed) {
        Host::reset(); Fixture f; f.install(); f.initialize();
        f.mac.errors[operations[failed]] = ESP_FAIL;
        assert(f.owner->start(Host::state().nowUs + 100000) == ESP_FAIL);
        const auto record = f.journal();
        assert(record.publicationUncertain && !record.publishedTuple.up && !record.eventLost);
        assert(record.publishedTuple.speedAccepted == (failed > 0));
        assert(record.publishedTuple.duplexAccepted == (failed > 1));
        assert(record.publishedTuple.pauseAccepted == (failed > 2));
        LinkSnapshot acquired;
        assert(f.owner->linkSnapshot(acquired).ok() && acquired.valid && acquired.linkUp && acquired.resolvedModeValid);
        const auto frames = f.mac.frames.size(); const auto offset = Host::state().calls.size();
        Host::tickTimers();
        assert(f.mac.frames.size() == frames && noticesSince(offset).empty());
        assert(f.owner->retryLinkEvent(Host::state().nowUs + 100000) == ESP_ERR_INVALID_STATE);
        assert(f.owner->destroy() == ESP_ERR_INVALID_STATE && Host::isLive(f.handle));
    }
    // The same event-post error has a different policy for an unqualified MAC.
    Host::reset(); Fixture f; f.install(); f.initialize();
    Host::state().eventErrorId = ETHERNET_EVENT_CONNECTED; Host::state().nextEventError = ESP_ERR_TIMEOUT;
    assert(f.owner->start(Host::state().nowUs + 100000) == ESP_ERR_TIMEOUT);
    assert(f.journal().publicationUncertain && !f.journal().eventLost);
    assert(f.owner->retryLinkEvent(Host::state().nowUs + 100000) == ESP_ERR_INVALID_STATE);
}
void notificationLateRetry() {
    const char* operations[] = {"mac.speed", "mac.duplex", "mac.pause", "mac.up"};
    for (unsigned late = 0; late < 4; ++late) {
        Host::reset(); Fixture f; f.install(); f.initialize(LINK_TECHNOLOGY_MASK);
        f.mac.elapsedUs["mac.speed"] = 10; // A completion exactly at the qualified bound remains accepted.
        assert(f.start() == ESP_OK);
        f.mac.registers[0x10] = 0x0017;
        f.mac.elapsedUs[operations[late]] = 11; // Qualified limit is 10 us, aggregate still has ample time.
        auto* phy = f.owner->nativePhy();
        assert(phy->get_link(phy) == ESP_ERR_TIMEOUT);
        const auto record = f.journal();
        assert(!record.publicationUncertain && record.firstError.detail == ESP_ERR_TIMEOUT);
        assert(record.publishedTuple.speedAccepted);
        assert(record.publishedTuple.duplexAccepted == (late >= 1));
        assert(record.publishedTuple.pauseAccepted == (late >= 2));
        assert(record.publishedTuple.up == (late == 3));
        f.mac.elapsedUs.clear();
        const auto offset = Host::state().calls.size();
        assert(phy->get_link(phy) == ESP_OK);
        // Incomplete DOWN projections retry the full tuple. Accepted UP is never replayed.
        const std::vector<std::string> retry = late == 3 ? std::vector<std::string>{} :
            std::vector<std::string>{"mac.speed", "mac.duplex", "mac.pause", "mac.up"};
        assert(noticesSince(offset) == retry);
        assert(f.journal().publishedTuple.up && f.journal().publishedTuple.pauseAccepted);
    }
}
void notificationCompletionContention() {
    const char* operations[] = {"mac.speed", "mac.duplex", "mac.pause", "mac.up"};
    for (unsigned completed = 0; completed < 4; ++completed) {
        Host::reset(); Fixture f; f.install(); f.initialize(LINK_TECHNOLOGY_MASK);
        assert(f.start() == ESP_OK);
        f.mac.registers[0x10] = 0x0017;
        bool injected = false;
        f.mac.onCall = [&](const char* operation) {
            if (injected || std::string(operation) != operations[completed]) return;
            injected = true; Host::state().failSemaphoreTakes = 1;
        };
        const auto offset = Host::state().calls.size();
        auto* phy = f.owner->nativePhy();
        const auto error = phy->get_link(phy);
        assert(injected && error == ESP_OK);
        LifecycleRecord record;
        auto copied = f.owner->lifecycleSnapshot(record);
        if (copied.is(Err::BUSY)) copied = f.owner->lifecycleSnapshot(record);
        assert(copied.ok());
        const auto notices = noticesSince(offset);
        const auto called = [&](const char* operation) { return std::find(notices.begin(), notices.end(), operation) != notices.end(); };
        assert(!record.publicationUncertain && record.publishedTuple.speedAccepted == called("mac.speed"));
        assert(record.publishedTuple.duplexAccepted == called("mac.duplex"));
        assert(record.publishedTuple.pauseAccepted == called("mac.pause"));
        assert(record.publishedTuple.up == called("mac.up"));
        assert(called(operations[completed]));
        f.mac.onCall = nullptr;
        assert(f.owner->stop(Host::state().nowUs + 100000) == ESP_OK);
        assert(!f.journal().publishedTuple.up && Host::isLive(f.handle));
    }
    Host::reset(); Fixture f; f.config.pinnedP4Mac = true;
    f.install(); f.initialize(LINK_TECHNOLOGY_MASK); assert(f.start() == ESP_OK);
    f.mac.registers[0x10] = 0x0017;
    f.mac.onCall = [&](const char* operation) {
        if (std::string(operation) == "mac.up") Host::state().failSemaphoreTakes = 1;
    };
    Host::state().eventErrorId = ETHERNET_EVENT_CONNECTED; Host::state().nextEventError = ESP_ERR_TIMEOUT;
    auto* phy = f.owner->nativePhy(); (void)phy->get_link(phy);
    LifecycleRecord record;
    auto copied = f.owner->lifecycleSnapshot(record);
    if (copied.is(Err::BUSY)) copied = f.owner->lifecycleSnapshot(record);
    assert(copied.ok() && record.publishedTuple.up && record.eventLost && !record.publicationUncertain);
    f.mac.onCall = nullptr;
    const auto offset = Host::state().calls.size();
    assert(f.owner->retryLinkEvent(Host::state().nowUs + 100000) == ESP_OK);
    assert(noticesSince(offset).empty() && !f.journal().eventLost);
}
void notificationLateEventRetry() {
    Host::reset(); Fixture f; f.config.pinnedP4Mac = true;
    f.install(); f.initialize();
    f.mac.registers[1] = 0x7849; f.mac.registers[0x10] = 0;
    assert(f.start() == ESP_OK);
    f.mac.registers[1] = 0x786D; f.mac.registers[0x10] = 0x0015;
    Host::state().eventErrorId = ETHERNET_EVENT_CONNECTED; Host::state().nextEventError = ESP_ERR_TIMEOUT;
    Host::tickTimers();
    assert(f.journal().eventLost && f.journal().publishedTuple.up);
    const auto offset = Host::state().calls.size(); const auto events = Host::state().events.size();
    assert(f.owner->retryLinkEvent(Host::state().nowUs) == ESP_ERR_TIMEOUT);
    assert(Host::state().calls.size() == offset && f.journal().eventLost);
    Host::state().eventElapsedUs = 1001;
    assert(f.owner->retryLinkEvent(Host::state().nowUs + 1000) == ESP_ERR_TIMEOUT);
    assert(Host::state().events.size() == events + 1 && !f.journal().eventLost);
    assert(noticesSince(offset).empty()); // Late accepted post is retained; no LINK callback is replayed.
    Host::state().eventElapsedUs = 0;
    const auto posted = Host::state().events.size();
    assert(f.owner->retryLinkEvent(Host::state().nowUs + 100000) == ESP_ERR_INVALID_STATE);
    assert(Host::state().events.size() == posted);
}
void boardTickReadiness() {
    for (const bool beforeTick : {true, false}) {
        Host::reset(); Fixture f; f.create();
        const uint64_t tickUs = uint64_t{1000000} / CONFIG_FREERTOS_HZ;
        Host::state().nowUs = 50000 - 1; // First vTaskDelay(1) lasts only one microsecond.
        BoardControl control;
        struct Times { uint64_t assertedAt; uint64_t releasedAt; bool beforeTick; } times{0, 0, beforeTick};
        control.context = &times;
        control.setReset = [](void* context, bool asserted) {
            auto& times = *static_cast<Times*>(context);
            if (asserted) times.assertedAt = Host::state().nowUs;
            else {
                if (times.beforeTick) {
                    const uint64_t tickUs = uint64_t{1000000} / CONFIG_FREERTOS_HZ;
                    Host::state().nowUs = (Host::state().nowUs / tickUs + 1) * tickUs - 1;
                }
                times.releasedAt = Host::state().nowUs;
            }
            return ESP_OK;
        };
        assert(f.owner->prepareBoard(control, Host::state().nowUs + 100000) == ESP_OK);
        const auto assertedAt = times.assertedAt, releasedAt = times.releasedAt;
        // One-microsecond floor-clock uncertainty is part of the readiness anchor.
        assert(assertedAt >= 50001 && releasedAt >= assertedAt + control.assertionUs + 1);
        assert(Host::state().nowUs >= releasedAt + 2001 && f.bus.admission == BusAdmission::AVAILABLE && f.mac.frames.empty());
        assert(!Host::state().delays.empty() && Host::state().delays.front().afterUs - Host::state().delays.front().beforeUs == 1);
        const auto afterRelease = std::find_if(Host::state().delays.begin(), Host::state().delays.end(), [&](const auto& delay) { return delay.beforeUs == releasedAt; });
        assert(afterRelease != Host::state().delays.end());
        if (beforeTick) {
            assert(afterRelease->afterUs - afterRelease->beforeUs == 1);
            assert(afterRelease->afterUs < releasedAt + 2000); // This wake must not open MDIO admission.
        }
        assert(tickUs >= 1000 && f.bus.quietUntilUs == releasedAt + 2001);
        assert(f.owner->destroy() == ESP_OK);
    }
}
void startCompletionContention() {
    Host::reset(); Fixture f; f.install(); f.initialize();
    bool injected = false;
    Host::state().onTimerPeriodic = [&] { injected = true; Host::state().failSemaphoreTakes = 1; };
    const auto deadline = Host::state().nowUs + 100000;
    assert(f.owner->start(deadline) == ESP_OK && injected);
    Host::state().onTimerPeriodic = nullptr;
    LifecycleRecord record;
    auto copied = f.owner->lifecycleSnapshot(record);
    if (copied.is(Err::BUSY)) copied = f.owner->lifecycleSnapshot(record);
    assert(copied.ok() && record.publishedTuple.up && !record.publicationUncertain);
    Host::state().nowUs = deadline + 1;
    const auto offset = Host::state().calls.size();
    auto* phy = f.owner->nativePhy();
    assert(phy->get_link(phy) == ESP_OK && noticesSince(offset).empty()); // RUNNING has a fresh callback budget.
    assert(f.owner->stop(Host::state().nowUs + 100000) == ESP_OK);
}
void stopCompletionContention() {
    for (const bool afterFenceArm : {false, true}) {
        Host::reset(); Fixture f; f.install(); f.initialize(); assert(f.start() == ESP_OK);
        const auto frames = f.mac.frames.size();
        bool injected = false;
        if (afterFenceArm) {
            Host::state().onTimerOnce = [&] { injected = true; Host::state().failSemaphoreTakes = 1; };
        } else {
            Host::state().onEventPost = [&](int32_t id) {
                if (id == ETHERNET_EVENT_STOP) { injected = true; Host::state().failSemaphoreTakes = 1; }
            };
        }
        assert(f.owner->stop(Host::state().nowUs + 100000) == ESP_ERR_INVALID_STATE && injected);
        Host::state().onTimerOnce = nullptr; Host::state().onEventPost = nullptr;
        const auto stops = [] {
            return std::count_if(Host::state().events.begin(), Host::state().events.end(), [](const auto& event) { return event.id == ETHERNET_EVENT_STOP; });
        };
        assert(stops() == 1 && Host::state().oneShotArms == (afterFenceArm ? 1U : 0U));
        assert(f.owner->stop(Host::state().nowUs + 100000) == ESP_OK);
        assert(stops() == 1 && Host::state().oneShotArms == 1); // Accepted SDK stop and fence arm cannot be replayed.
        assert(f.mac.frames.size() == frames && !f.journal().publishedTuple.up && Host::isLive(f.handle));
    }
}
void pollPeriodBounds() {
    Host::reset(); Fixture f;
    Native* output = reinterpret_cast<Native*>(0x1234);
    for (const auto period : {uint32_t{0}, uint32_t{4294968}, UINT32_MAX}) {
        auto invalid = f.config; invalid.linkPollMs = period;
        Host::state().failSemaphore = true;
        assert(Native::create(invalid, output) == ESP_ERR_INVALID_ARG);
        assert(output == reinterpret_cast<Native*>(0x1234) && Host::state().failSemaphore && Host::state().timerCreates == 0);
        Host::state().failSemaphore = false;
    }
    f.config.linkPollMs = 4294967;
    f.install(); f.initialize(); assert(f.start() == ESP_OK);
    assert(Host::state().periodicPeriodsUs.back() == uint64_t{4294967000});
    assert(f.owner->stop(Host::state().nowUs + 100000) == ESP_OK);
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
    const auto late = g.owner->discover(3, output, {Host::state().nowUs + 9000, 4});
    assert(late.is(Err::OPERATION_TIMEOUT) && late.detail == static_cast<int32_t>(TimeoutDetail::DEADLINE));
    assert(g.mac.frames.size() == 1);
    assert(g.owner->discoverySnapshot(cached).ok() && cached.framesAttempted == 1 && cached.entries[0].id1Valid);
    TransportSnapshot transport;
    assert(g.owner->transportSnapshot(transport).ok() && transport.violation == TransportViolation::NONE);
    assert(transport.late && !transport.stopped && transport.attemptedFrames == 1 && transport.completedFrames == 1);
    assert(transport.lastBatch.status.is(Err::OPERATION_TIMEOUT));
    assert(transport.lastBatch.status.detail == static_cast<int32_t>(TimeoutDetail::DEADLINE));
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
void discoveryAccounting() {
    Host::reset(); Fixture f; f.install(); f.initialize();
    TransportSnapshot base; assert(f.owner->transportSnapshot(base).ok());
    SettingsSnapshot settings; assert(f.owner->settingsSnapshot(settings).ok());
    assert(settings.trust == ConfigurationTrust::VERIFIED);
    SdkStateSnapshot sdk; assert(f.owner->sdkStateSnapshot(sdk).ok() && sdk.reconciled);
    // Discovery reads a foreign address as raw words; it cannot replace the bound part's proof.
    f.mac.registers[2] = 0x1234; f.mac.registers[3] = 0x5678;
    const auto frames = f.mac.frames.size();
    DiscoverySnapshot scan;
    assert(f.owner->discover(2, scan, {Host::state().nowUs + 100000, 2}).ok());
    assert(scan.framesAttempted == 2 && scan.entries[1].id1 == 0x1234 && scan.entries[1].id2 == 0x5678);
    assert(f.mac.frames[frames].address == 1 && f.mac.frames[frames].reg == 2);
    assert(f.mac.frames[frames + 1].address == 1 && f.mac.frames[frames + 1].reg == 3);
    TransportSnapshot receipt; assert(f.owner->transportSnapshot(receipt).ok());
    assert(receipt.totalBatches == base.totalBatches + 1 && receipt.attemptedFrames == base.attemptedFrames + 2);
    assert(receipt.completedFrames == base.completedFrames + 2 && receipt.lastBatch.status.ok());
    assert(receipt.lastBatch.frames[0].value == 0x1234 && receipt.lastBatch.frames[1].value == 0x5678);
    SettingsSnapshot after; assert(f.owner->settingsSnapshot(after).ok());
    assert(after.trust == settings.trust && after.desiredValid == settings.desiredValid);
    assert(after.observedValid == settings.observedValid && after.observedAtUs == settings.observedAtUs);
    assert(f.owner->sdkStateSnapshot(sdk).ok() && sdk.reconciled);
    uint32_t selected = 99;
    assert(f.owner->nativePhy()->get_addr(f.owner->nativePhy(), &selected) == ESP_OK && selected == 0);
    uint16_t id = 0;
    assert(f.owner->readClause22(2, id, {Host::state().nowUs + 100000, 1}).ok() && id == 0x1234);
    assert(f.owner->transportSnapshot(receipt).ok());
    assert(receipt.totalBatches == base.totalBatches + 2 && receipt.attemptedFrames == base.attemptedFrames + 3);
    assert(receipt.completedFrames == base.completedFrames + 3 && receipt.lastBatch.frames[0].value == 0x1234);
    assert(receipt.lastBatch.frames[0].completed && !receipt.lastBatch.frames[1].attempted);
    // One complete address and one failed second word retain every attempted/completed prefix.
    f.mac.readResults = {ESP_OK, ESP_OK, ESP_OK, ESP_ERR_TIMEOUT};
    scan.requestedMask = 0xCAFE; scan.framesAttempted = 99;
    assert(f.owner->discover(10, scan, {Host::state().nowUs + 100000, 4}).detail == ESP_ERR_TIMEOUT);
    assert(scan.requestedMask == 0xCAFE && scan.framesAttempted == 99);
    assert(f.owner->transportSnapshot(receipt).ok());
    assert(receipt.totalBatches == base.totalBatches + 4 && receipt.attemptedFrames == base.attemptedFrames + 7);
    assert(receipt.completedFrames == base.completedFrames + 6 && receipt.lastBatch.status.detail == ESP_ERR_TIMEOUT);
    assert(receipt.lastBatch.frames[0].completed && receipt.lastBatch.frames[0].valueValid);
    assert(receipt.lastBatch.frames[1].attempted && !receipt.lastBatch.frames[1].completed);
    DiscoverySnapshot cached; assert(f.owner->discoverySnapshot(cached).ok());
    assert(cached.framesAttempted == 4 && cached.entries[1].id1Valid && cached.entries[1].id2Valid);
    assert(cached.entries[3].id1Valid && !cached.entries[3].id2Valid);
    const auto stoppedFrames = f.mac.frames.size(); const auto stoppedTime = Host::state().nowUs;
    assert(f.owner->transportSnapshot(receipt).ok() && f.owner->discoverySnapshot(cached).ok());
    assert(f.mac.frames.size() == stoppedFrames && Host::state().nowUs == stoppedTime);

    Host::reset(); Fixture g; g.install(); g.initialize();
    g.mac.onFrame = [](const Host::Frame&) { Host::state().nowUs = 1; };
    scan.requestedMask = 0xCAFE;
    assert(g.owner->discover(2, scan, {Host::state().nowUs + 100000, 2}).detail == static_cast<int32_t>(ConfigDetail::CLOCK_CONTRACT));
    assert(scan.requestedMask == 0xCAFE);
    assert(g.owner->transportSnapshot(receipt).ok() && receipt.clockInvalid);
    const auto clockFrames = g.mac.frames.size(); const auto clockBatches = receipt.totalBatches;
    g.mac.onFrame = nullptr; Host::state().nowUs = 1000000;
    id = 0xCAFE;
    assert(g.owner->readClause22(2, id, {1100000, 1}).detail == static_cast<int32_t>(ConfigDetail::CLOCK_CONTRACT));
    assert(g.owner->transportSnapshot(receipt).ok() && receipt.clockInvalid && receipt.totalBatches == clockBatches);
    assert(id == 0xCAFE && g.mac.frames.size() == clockFrames);
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
std::vector<Host::SemaphoreEvent> controllerEvents(SemaphoreHandle_t mutex) {
    std::vector<Host::SemaphoreEvent> result;
    for (const auto& event : Host::state().semaphoreEvents)
        if (event.handle == mutex) result.push_back(event);
    return result;
}
void rawReadArbitration() {
    Host::reset(); Fixture f; Fixture g;
    g.config.bus = &f.bus;
    f.install(); g.install();
    f.mac.extended[0x0467] = 0xBEEF;
    Host::state().semaphoreEvents.clear();
    unsigned boundaries = 0;
    f.mac.onFrame = [&](const Host::Frame&) {
        ++boundaries;
        assert(Host::semaphoreHeld(f.bus.mutex));
        const auto frames = f.mac.frames.size();
        uint16_t sentinel = 0xCAFE;
        assert(f.owner->readMmd(31, 0x0467, sentinel, {Host::state().nowUs + 100000, 4}).is(Err::BUSY));
        assert(sentinel == 0xCAFE && f.mac.frames.size() == frames);
        Host::state().currentTask = 2; // Independent owner/PHY on the same controller.
        assert(g.owner->readMmd(31, 0x0467, sentinel, {Host::state().nowUs + 100000, 4}).is(Err::MDIO_TIMEOUT));
        Host::state().currentTask = 1;
        assert(sentinel == 0xCAFE && g.mac.frames.empty());
        uint32_t raw = 0xCAFE;
        esp_eth_phy_reg_rw_data_t request{2, &raw};
        assert(esp_eth_ioctl(f.handle, ETH_CMD_READ_PHY_REG, &request) == ESP_ERR_NOT_SUPPORTED);
        assert(esp_eth_ioctl(f.handle, ETH_CMD_WRITE_PHY_REG, &request) == ESP_ERR_NOT_SUPPORTED);
        assert(raw == 0xCAFE && f.mac.frames.size() == frames);
    };
    uint16_t result = 0;
    assert(f.owner->readMmd(31, 0x0467, result, {Host::state().nowUs + 100000, 4}).ok());
    assert(result == 0xBEEF && boundaries == 4 && f.mac.frames.size() == 4);
    const auto& frames = f.mac.frames;
    assert(frames[0].write && frames[0].reg == 13 && frames[0].value == 0x001F);
    assert(frames[1].write && frames[1].reg == 14 && frames[1].value == 0x0467);
    assert(frames[2].write && frames[2].reg == 13 && frames[2].value == 0x401F);
    assert(!frames[3].write && frames[3].reg == 14 && frames[3].value == 0xBEEF);
    const auto receipts = controllerEvents(f.bus.mutex);
    assert(std::count_if(receipts.begin(), receipts.end(), [](const auto& e) { return e.take && e.accepted; }) == 1);
    assert(std::count_if(receipts.begin(), receipts.end(), [](const auto& e) { return !e.take; }) == 1);
    assert(!Host::semaphoreHeld(f.bus.mutex) && !f.bus.quarantined);
    f.mac.onFrame = nullptr;
    assert(g.owner->readMmd(31, 0x0467, result, {Host::state().nowUs + 100000, 4}).ok());
}
void rawLockFailures() {
    Host::reset(); Fixture f; f.install();
    uint16_t result = 0xCAFE;
    Host::state().semaphoreEvents.clear(); Host::state().failNextTake = f.bus.mutex;
    assert(f.owner->readMmd(31, 0x0467, result, {Host::state().nowUs + 100000, 4}).is(Err::MDIO_TIMEOUT));
    auto receipts = controllerEvents(f.bus.mutex);
    assert(receipts.size() == 1 && receipts[0].take && !receipts[0].accepted);
    assert(result == 0xCAFE && f.mac.frames.empty() && !f.bus.quarantined);
    assert(f.owner->readMmd(31, 0x0467, result, {Host::state().nowUs + 100000, 4}).ok());
    Host::state().semaphoreEvents.clear(); Host::state().failNextGive = f.bus.mutex;
    result = 0xCAFE;
    assert(!f.owner->readMmd(31, 0x0467, result, {Host::state().nowUs + 100000, 4}).ok());
    receipts = controllerEvents(f.bus.mutex);
    assert(receipts.size() == 2 && receipts[0].take && receipts[0].accepted && !receipts[1].take && !receipts[1].accepted);
    assert(result == 0xCAFE && f.bus.quarantined);
    const auto frames = f.mac.frames.size();
    assert(!f.owner->readMmd(31, 0x0467, result, {Host::state().nowUs + 100000, 4}).ok());
    assert(result == 0xCAFE && f.mac.frames.size() == frames);
}
void controllerWaitClassification() {
    constexpr uint32_t tickUs = 1000000U / CONFIG_FREERTOS_HZ;
    for (const uint32_t waitUs : {tickUs - 1U, tickUs, tickUs * 2U}) {
        Host::reset(); Fixture f;
        f.config.lockWaitUs = waitUs;
        f.config.core.transport.bounds.fixedUs = waitUs + 1000U;
        f.install();
        Host::state().semaphoreEvents.clear();
        Host::state().failNextTake = f.bus.mutex;
        uint16_t output = 0xCAFE;
        const auto status = f.owner->readMmd(31, 0x0467, output, {Host::state().nowUs + 100000, 4});
        const auto receipts = controllerEvents(f.bus.mutex);
        const bool waited = waitUs >= tickUs;
        assert(status.is(waited ? Err::MDIO_TIMEOUT : Err::BUSY));
        if (waited) assert(status.detail == ESP_ERR_TIMEOUT);
        assert(receipts.size() == 1 && receipts[0].take && !receipts[0].accepted);
        assert(receipts[0].ticks == waitUs / tickUs);
        assert(output == 0xCAFE && f.mac.frames.empty() && !f.bus.quarantined);
        // A timed acquisition failure has no uncertain controller effect and is retryable.
        assert(f.owner->readMmd(31, 0x0467, output, {Host::state().nowUs + 100000, 4}).ok());
    }
}
void rawBoundaryFailures() {
    for (unsigned boundary = 0; boundary < 4; ++boundary) {
        Host::reset(); Fixture f; f.install();
        if (boundary < 3) {
            f.mac.writeResults.assign(boundary, ESP_OK);
            f.mac.writeResults.push_back(ESP_ERR_TIMEOUT);
        } else f.mac.readResults.push_back(ESP_ERR_TIMEOUT);
        Host::state().semaphoreEvents.clear();
        uint16_t result = 0xCAFE;
        const auto status = f.owner->readMmd(31, 0x0467, result, {Host::state().nowUs + 100000, 4});
        assert(status.detail == ESP_ERR_TIMEOUT && result == 0xCAFE);
        assert(f.mac.frames.size() == boundary && f.bus.quarantined && !Host::semaphoreHeld(f.bus.mutex));
        const auto receipts = controllerEvents(f.bus.mutex);
        assert(receipts.size() == 2 && receipts[0].take && receipts[0].accepted && !receipts[1].take && receipts[1].accepted);
    }
}
void rejectedRawReads() {
    Host::reset(); Fixture f; f.install();
    const IoBudget budget{Host::state().nowUs + 100000, 4};
    Host::state().semaphoreEvents.clear();
    for (const auto address : {0x0DU, 0x0EU, 0x12U, 0x13U, 0x20U, 0x10000U}) {
        uint16_t value = 0xCAFE;
        assert(f.owner->readClause22(address, value, budget).is(Err::INVALID_PARAM));
        assert(value == 0xCAFE);
        assert(f.owner->readMmd(31, address, value, budget).is(Err::INVALID_PARAM));
        assert(value == 0xCAFE);
    }
    uint16_t value = 0xCAFE;
    assert(f.owner->readMmd(32, 2, value, budget).is(Err::INVALID_PARAM));
    assert(f.owner->readMmd(31, 2, value, {budget.deadlineUs, 3}).is(Err::OPERATION_TIMEOUT));
    assert(value == 0xCAFE && f.mac.frames.empty() && controllerEvents(f.bus.mutex).empty());
    assert(!f.bus.quarantined && !Host::semaphoreHeld(f.bus.mutex));
}
LinkSettings forcedSettings(LinkSpeed speed, Duplex duplex = Duplex::FULL) {
    LinkSettings settings;
    settings.mode = LinkMode::FORCED;
    settings.forced = {speed, duplex};
    return settings;
}
void forcedLifecycle() {
    for (const auto speed : {LinkSpeed::MBPS_10, LinkSpeed::MBPS_100}) {
        for (const auto duplex : {Duplex::HALF, Duplex::FULL}) {
            Host::reset(); Fixture f; f.install(); f.initialize();
            bool enabled = false;
            eth_speed_t cachedSpeed = ETH_SPEED_10M;
            eth_duplex_t cachedDuplex = ETH_DUPLEX_HALF;
            assert(esp_eth_ioctl(f.handle, ETH_CMD_G_AUTONEGO, &enabled) == ESP_OK && enabled);
            assert(f.start() == ESP_OK);
            assert(f.owner->stop(Host::state().nowUs + 100000) == ESP_OK);
            f.configure(forcedSettings(speed, duplex));
            assert(esp_eth_ioctl(f.handle, ETH_CMD_G_AUTONEGO, &enabled) == ESP_OK && !enabled);
            const auto before = f.mac.frames.size();
            assert(esp_eth_ioctl(f.handle, ETH_CMD_G_SPEED, &cachedSpeed) == ESP_OK && cachedSpeed == ETH_SPEED_100M);
            assert(esp_eth_ioctl(f.handle, ETH_CMD_G_DUPLEX_MODE, &cachedDuplex) == ESP_OK && cachedDuplex == ETH_DUPLEX_FULL);
            assert(f.mac.frames.size() == before); // SDK getters retain the last published tuple.
            const auto bmcr = f.mac.registers[0];
            assert((bmcr & 0x1000) == 0 && ((bmcr & 0x2000) != 0) == (speed == LinkSpeed::MBPS_100));
            assert(((bmcr & 0x0100) != 0) == (duplex == Duplex::FULL));
            assert(f.start() == ESP_OK);
            assert(esp_eth_ioctl(f.handle, ETH_CMD_G_SPEED, &cachedSpeed) == ESP_OK);
            assert(cachedSpeed == (speed == LinkSpeed::MBPS_100 ? ETH_SPEED_100M : ETH_SPEED_10M));
            assert(esp_eth_ioctl(f.handle, ETH_CMD_G_DUPLEX_MODE, &cachedDuplex) == ESP_OK);
            assert(cachedDuplex == (duplex == Duplex::FULL ? ETH_DUPLEX_FULL : ETH_DUPLEX_HALF));
            assert(f.owner->stop(Host::state().nowUs + 100000) == ESP_OK);
            assert(f.start() == ESP_OK);
            assert(f.owner->stop(Host::state().nowUs + 100000) == ESP_OK);
        }
    }
}
void rawStateReconciliation() {
    Host::reset(); Fixture f; f.install(); f.initialize();
    assert(f.owner->modifyBasicControl(0x1000, 0, {Host::state().nowUs + 100000, 2}).ok());
    SdkStateSnapshot sdk;
    SettingsSnapshot settings;
    assert(f.owner->sdkStateSnapshot(sdk).ok() && !sdk.reconciled);
    assert(f.owner->settingsSnapshot(settings).ok() && settings.trust != ConfigurationTrust::VERIFIED);
    bool cached = false;
    assert(esp_eth_ioctl(f.handle, ETH_CMD_G_AUTONEGO, &cached) == ESP_OK && cached);
    const auto frames = f.mac.frames.size();
    assert(f.owner->start(Host::state().nowUs + 100000) == ESP_ERR_INVALID_STATE);
    assert(!f.owner->reconcileSdkState(Host::state().nowUs + 100000).ok());
    assert(f.mac.frames.size() == frames);
    OperationToken token;
    assert(f.owner->startInitialize({ResetKind::NONE}, {{Host::state().nowUs + 1000000, 512}, 10, 16}, token).inProgress());
    f.finish(token);
    assert(f.owner->reconcileSdkState(Host::state().nowUs + 100000).ok());
    assert(esp_eth_ioctl(f.handle, ETH_CMD_G_AUTONEGO, &cached) == ESP_OK && cached);
    assert((f.mac.registers[0] & 0x1000) != 0);
    assert(f.start() == ESP_OK);
}
void directStartInvalidation() {
    for (const bool cachedAn : {true, false}) {
        Host::reset(); Fixture f; f.install(); f.initialize();
        if (!cachedAn) f.configure(forcedSettings(LinkSpeed::MBPS_10));
        assert(f.owner->invalidateDeviceState(InvalidationReason::EXTERNAL_MUTATION).ok());
        const auto frames = f.mac.frames.size();
        const auto starts = std::count_if(Host::state().events.begin(), Host::state().events.end(), [](const auto& e) { return e.id == ETHERNET_EVENT_START; });
        assert(esp_eth_start(f.handle) == ESP_ERR_INVALID_STATE);
        assert(f.mac.frames.size() == frames);
        const auto newStarts = std::count_if(Host::state().events.begin(), Host::state().events.end(), [](const auto& e) { return e.id == ETHERNET_EVENT_START; });
        assert(newStarts - starts == (cachedAn ? 0 : 1));
        assert(esp_eth_start(f.handle) == ESP_ERR_INVALID_STATE); // FSM effect remains until supported cleanup.
        assert(f.owner->stop(Host::state().nowUs + 100000) == ESP_ERR_INVALID_STATE);
        assert(Host::state().oneShotArms == 1 && !f.journal().publishedTuple.up);
        assert(std::count(Host::state().calls.begin(), Host::state().calls.end(), "mac.stop") > 0);
        const auto stops = std::count_if(Host::state().events.begin(), Host::state().events.end(), [](const auto& e) { return e.id == ETHERNET_EVENT_STOP; });
        assert(stops == (cachedAn ? 0 : 1));
        assert(f.owner->start(Host::state().nowUs + 100000) == ESP_ERR_INVALID_STATE);
        OperationToken token;
        assert(f.owner->startInitialize({ResetKind::NONE}, {{Host::state().nowUs + 1000000, 512}, 10, 16}, token).inProgress());
        f.finish(token);
        assert(f.owner->reconcileSdkState(Host::state().nowUs + 100000).ok());
    }
}
void flowControl() {
    Host::reset(); Fixture f; f.install(); f.initialize();
    LinkSettings settings; settings.advertisement = {LINK_100_FULL, true, true};
    f.configure(settings);
    const auto before = f.mac.frames.size();
    assert(f.owner->setFlowControl(true, 4, Host::state().nowUs + 100000) == ESP_ERR_INVALID_ARG);
    assert(f.owner->setFlowControl(true, 0, Host::state().nowUs + 100000) != ESP_OK);
    assert(f.mac.frames.size() == before && f.mac.flowRequests.empty());
    f.mac.onCall = [&](const char* operation) {
        if (std::string(operation) != "mac.flow") return;
        assert(!Host::semaphoreHeld(f.bus.mutex));
        uint16_t sentinel = 0xCAFE;
        assert(f.owner->readClause22(2, sentinel, {Host::state().nowUs + 100000, 1}).is(Err::BUSY));
        assert(sentinel == 0xCAFE && f.mac.frames.size() == before);
        Host::state().currentTask = 2;
        assert(f.owner->setFlowControl(true, 3, Host::state().nowUs + 100000) == ESP_ERR_INVALID_STATE);
        Host::state().currentTask = 1;
    };
    assert(f.owner->setFlowControl(true, 3, Host::state().nowUs + 100000) == ESP_OK);
    FlowControlRecord record;
    assert(f.owner->flowControlSnapshot(record).ok());
    assert(record.requestedEnabled && record.pauseAbility == 3 && record.macAttempted && record.macAccepted && record.phyAttempted && record.phyAccepted && record.reconciled);
    assert(f.mac.flowEnabled && f.mac.frames.size() == before);
    assert(f.mac.flowRequests.size() == 1 && f.mac.flowRequests[0]);
    f.mac.errors["mac.flow"] = ESP_ERR_TIMEOUT;
    assert(f.owner->setFlowControl(false, 3, Host::state().nowUs + 100000) == ESP_ERR_TIMEOUT);
    assert(f.owner->flowControlSnapshot(record).ok());
    assert(record.macAttempted && !record.macAccepted && record.macEffectPossible && !record.phyAttempted && !record.phyAccepted && !record.reconciled);
    assert(record.macError == ESP_ERR_TIMEOUT && f.mac.flowEnabled);
    assert(f.owner->setFlowControl(false, 3, Host::state().nowUs + 100000) == ESP_OK);
    f.mac.elapsedUs["mac.flow"] = 100001;
    assert(f.owner->setFlowControl(true, 3, Host::state().nowUs + 100000) == ESP_ERR_TIMEOUT);
    assert(f.owner->flowControlSnapshot(record).ok());
    assert(record.macAccepted && !record.phyAttempted && !record.phyAccepted && !record.reconciled && f.mac.flowEnabled);
    assert(record.macError == ESP_OK && record.phyError == ESP_ERR_TIMEOUT);
    f.mac.elapsedUs["mac.flow"] = 0;
    assert(f.owner->setFlowControl(false, 3, Host::state().nowUs + 100000) == ESP_OK);
}
void flowControlCallbackBounds() {
    for (const uint32_t elapsed : {10U, 11U}) {
        Host::reset(); Fixture f; f.install(); f.initialize();
        LinkSettings settings; settings.advertisement = {LINK_100_FULL, true, true}; f.configure(settings);
        f.mac.elapsedUs["mac.flow"] = elapsed;
        const auto before = Host::state().nowUs;
        const auto error = f.owner->setFlowControl(true, 3, before + 100000);
        const bool late = elapsed > f.config.notificationUs;
        assert(error == (late ? ESP_ERR_TIMEOUT : ESP_OK));
        FlowControlRecord record; assert(f.owner->flowControlSnapshot(record).ok());
        assert(record.macAccepted && record.macError == ESP_OK && record.macEffectPossible && f.mac.flowEnabled);
        assert(record.phyAttempted == !late && record.phyAccepted == !late && record.reconciled == !late);
        assert(record.phyError == (late ? ESP_ERR_TIMEOUT : ESP_OK));
        assert(Host::state().nowUs - before == elapsed);
        f.mac.elapsedUs["mac.flow"] = 11;
        f.mac.errors["mac.flow"] = ESP_FAIL;
        assert(f.owner->setFlowControl(false, 3, Host::state().nowUs + 100000) == ESP_FAIL);
        assert(f.owner->flowControlSnapshot(record).ok());
        assert(!record.macAccepted && record.macError == ESP_FAIL && record.macEffectPossible);
        assert(!record.phyAttempted && !record.reconciled && f.mac.flowEnabled);
        // Explicit successful retry repairs the retained partial/uncertain policy.
        f.mac.elapsedUs["mac.flow"] = 0;
        assert(f.owner->setFlowControl(false, 3, Host::state().nowUs + 100000) == ESP_OK);
        assert(f.owner->flowControlSnapshot(record).ok() && record.reconciled && !f.mac.flowEnabled);
    }
}
void sdkControlCallbackBounds() {
    static esp_err_t (*original)(esp_eth_phy_t*, eth_phy_autoneg_cmd_t, bool*) = nullptr;
    static uint32_t elapsed = 0;
    for (const uint32_t duration : {10U, 11U}) {
        Host::reset(); Fixture f; f.install(); f.initialize();
        auto* phy = f.owner->nativePhy(); original = phy->autonego_ctrl; elapsed = duration;
        phy->autonego_ctrl = [](esp_eth_phy_t* native, eth_phy_autoneg_cmd_t command, bool* enabled) {
            const auto error = original(native, command, enabled);
            Host::state().nowUs += elapsed;
            return error;
        };
        const auto frames = f.mac.frames.size();
        const auto status = f.owner->reconcileSdkState(Host::state().nowUs + 100000);
        const bool late = duration > f.config.notificationUs;
        assert(late ? status.is(Err::OPERATION_TIMEOUT) : status.ok());
        SdkStateSnapshot record; assert(f.owner->sdkStateSnapshot(record).ok());
        assert(record.autoNegotiationAccepted && record.reconciled == !late && record.cacheValid == !late);
        bool enabled = false;
        assert(esp_eth_ioctl(f.handle, ETH_CMD_G_AUTONEGO, &enabled) == ESP_OK && enabled);
        assert(f.mac.frames.size() == frames);
        phy->autonego_ctrl = original;
        assert(f.owner->reconcileSdkState(Host::state().nowUs + 100000).ok());
        assert(f.owner->sdkStateSnapshot(record).ok() && record.reconciled && record.cacheValid);
    }
}
void rawMutationArbitration() {
    for (const bool modify : {false, true}) {
        Host::reset(); Fixture f; Fixture g; g.config.bus = &f.bus;
        f.install(); g.install(); f.initialize(); g.initialize();
        f.mac.frames.clear(); g.mac.frames.clear();
        f.mac.registers[0x18] = 0x0080;
        Host::state().semaphoreEvents.clear();
        unsigned boundaries = 0;
        f.mac.onFrame = [&](const Host::Frame&) {
            ++boundaries;
            assert(Host::semaphoreHeld(f.bus.mutex));
            const IoBudget budget{Host::state().nowUs + 100000, 5};
            assert(f.owner->writeMmd(31, 0x18, 0x12, budget).is(Err::BUSY));
            Host::state().currentTask = 2;
            assert(g.owner->writeMmd(31, 0x18, 0x12, budget).is(Err::MDIO_TIMEOUT));
            Host::state().currentTask = 1;
            assert(g.mac.frames.empty());
        };
        const IoBudget budget{Host::state().nowUs + 100000, 5};
        const auto status = modify ? f.owner->modifyMmd(31, 0x18, 0x12, 0x12, budget) : f.owner->writeMmd(31, 0x18, 0x12, budget);
        assert(status.ok() && boundaries == (modify ? 5U : 4U));
        assert(f.mac.registers[0x18] == (modify ? 0x92 : 0x12));
        assert(f.mac.frames[0].reg == 13 && f.mac.frames[0].value == 0x001F);
        assert(f.mac.frames[1].reg == 14 && f.mac.frames[1].value == 0x0018);
        assert(f.mac.frames[2].reg == 13 && f.mac.frames[2].value == 0x401F);
        const auto receipts = controllerEvents(f.bus.mutex);
        assert(std::count_if(receipts.begin(), receipts.end(), [](const auto& e) { return e.take && e.accepted; }) == 1);
        assert(std::count_if(receipts.begin(), receipts.end(), [](const auto& e) { return !e.take; }) == 1);
        assert(!Host::semaphoreHeld(f.bus.mutex));
    }
}
void rawMutationAdmission() {
    Host::reset(); Fixture f; f.install(); f.initialize();
    const auto frames = f.mac.frames.size();
    const IoBudget budget{Host::state().nowUs + 100000, 5};
    Host::state().semaphoreEvents.clear();
    for (const auto address : {0U, 13U, 14U, 18U, 19U, 32U, 0x10000U}) {
        assert(f.owner->writeClause22(address, 0, budget).is(Err::INVALID_PARAM));
        assert(f.owner->writeMmd(31, address, 0, budget).is(Err::INVALID_PARAM));
        assert(f.owner->modifyMmd(31, address, 1, 0, budget).is(Err::INVALID_PARAM));
    }
    assert(f.owner->writeClause22(0x18, 0x10000, budget).is(Err::INVALID_PARAM));
    assert(f.owner->writeMmd(31, 0x18, 0x10000, budget).is(Err::INVALID_PARAM));
    assert(f.owner->modifyMmd(31, 0x18, 1, 2, budget).is(Err::INVALID_PARAM));
    assert(f.owner->modifyBasicControl(0x8000, 0, budget).is(Err::INVALID_PARAM));
    assert(f.owner->modifyBasicControl(0x1000, 0x10000, budget).is(Err::INVALID_PARAM));
    assert(f.owner->modifyBasicControl(0x1000, 0x2000, budget).is(Err::INVALID_PARAM));
    assert(f.mac.frames.size() == frames && controllerEvents(f.bus.mutex).empty());
    SdkStateSnapshot sdk;
    assert(f.owner->sdkStateSnapshot(sdk).ok() && sdk.reconciled);
    assert(f.start() == ESP_OK);
    const auto runningFrames = f.mac.frames.size();
    assert(f.owner->writeClause22(0x18, 0x12, budget).is(Err::BUSY));
    assert(f.owner->writeMmd(31, 0x18, 0x12, budget).is(Err::BUSY));
    assert(f.owner->modifyMmd(31, 0x18, 0x12, 0x12, budget).is(Err::BUSY));
    assert(f.owner->modifyBasicControl(0x1000, 0, budget).is(Err::BUSY));
    assert(f.mac.frames.size() == runningFrames);
}
void collectorPartialEvidence() {
    Host::reset(); Fixture f; f.install(); f.initialize();
    f.mac.frames.clear();
    f.mac.registers[0x14] = 7; f.mac.registers[0x15] = 9;
    f.mac.readResults = {ESP_OK, ESP_ERR_TIMEOUT};
    EvidenceSnapshot output; output.lastRequested = 0xCAFE;
    const auto mask = evidenceBit(EvidenceSource::FCSCR) | evidenceBit(EvidenceSource::RECR);
    assert(f.owner->acquireEvidence(mask, output, {Host::state().nowUs + 100000, 2}).detail == ESP_ERR_TIMEOUT);
    assert(output.lastRequested == 0xCAFE && f.mac.frames.size() == 1 && f.mac.registers[0x14] == 0);
    EvidenceSnapshot retained;
    const auto frames = f.mac.frames.size();
    assert(f.owner->evidenceSnapshot(retained).ok() && retained.lastRequested == mask);
    const auto& first = retained.samples[static_cast<size_t>(EvidenceSource::FCSCR)];
    const auto& second = retained.samples[static_cast<size_t>(EvidenceSource::RECR)];
    assert(first.valid && first.raw == 7 && second.historyLost);
    assert(f.mac.frames.size() == frames && !Host::semaphoreHeld(f.bus.mutex));
}
void uncertainBasicControl() {
    Host::reset(); Fixture f; f.install(); f.initialize();
    f.mac.writeResults = {ESP_ERR_TIMEOUT};
    const auto status = f.owner->modifyBasicControl(0x1000, 0, {Host::state().nowUs + 100000, 2});
    assert(status.detail == ESP_ERR_TIMEOUT && f.bus.quarantined);
    const auto frames = f.mac.frames.size();
    TransportSnapshot transport;
    SdkStateSnapshot sdk;
    SettingsSnapshot settings;
    assert(f.owner->transportSnapshot(transport).ok());
    assert(transport.lastBatch.frames[1].attempted && transport.lastBatch.frames[1].effectPossible);
    assert(!transport.lastBatch.releaseStatus.ok() && !transport.lastBatch.controllerReusable);
    assert(f.owner->sdkStateSnapshot(sdk).ok() && !sdk.reconciled);
    assert(f.owner->settingsSnapshot(settings).ok() && settings.trust != ConfigurationTrust::VERIFIED);
    assert(f.mac.frames.size() == frames && !Host::semaphoreHeld(f.bus.mutex));
}
void configurationAdmission() {
    Host::reset(); Fixture f; f.install(); f.initialize();
    const auto frames = f.mac.frames.size();
    const auto calls = Host::state().calls.size();
    const OperationBudget budget{{Host::state().nowUs + 1000000, 512}, 10, 16};
    OperationToken token{0xCAFE};
    auto invalid = forcedSettings(LinkSpeed::UNKNOWN);
    assert(f.owner->startConfigure(invalid, budget, token).is(Err::INVALID_CONFIG));
    assert(token.value == 0xCAFE);
    invalid = forcedSettings(static_cast<LinkSpeed>(255));
    assert(f.owner->startConfigure(invalid, budget, token).is(Err::INVALID_PARAM));
    assert(token.value == 0xCAFE);
    invalid = forcedSettings(LinkSpeed::MBPS_10); invalid.extendedFullDuplex = true;
    assert(f.owner->startConfigure(invalid, budget, token).is(Err::INVALID_CONFIG));
    assert(token.value == 0xCAFE && f.mac.frames.size() == frames && Host::state().calls.size() == calls);
    bool enabled = false;
    eth_speed_t speed = ETH_SPEED_10M;
    eth_duplex_t duplex = ETH_DUPLEX_HALF;
    assert(esp_eth_ioctl(f.handle, ETH_CMD_S_AUTONEGO, &enabled) != ESP_OK);
    assert(esp_eth_ioctl(f.handle, ETH_CMD_S_SPEED, &speed) != ESP_OK);
    assert(esp_eth_ioctl(f.handle, ETH_CMD_S_DUPLEX_MODE, &duplex) != ESP_OK);
    assert(f.mac.frames.size() == frames);
    SdkStateSnapshot sdk;
    assert(f.owner->sdkStateSnapshot(sdk).ok() && sdk.reconciled);
}
void hardResetSilence() {
    Host::reset(); Fixture f; Fixture g; g.config.bus = &f.bus;
    f.install(); g.install(); f.initialize(); g.initialize();
    f.mac.frames.clear(); g.mac.frames.clear();
    OperationToken token;
    assert(f.owner->startInitialize({ResetKind::HARD}, {{Host::state().nowUs + 1000000, 512}, 10, 16}, token).inProgress());
    assert(f.bus.admission == BusAdmission::INITIALIZING && !Host::semaphoreHeld(f.bus.mutex));
    assert(f.owner->pollOperation(token, 0).inProgress() && f.mac.frames.empty());
    Status status{Err::IN_PROGRESS, 0, "Pending"};
    for (unsigned i = 0; i < 256 && status.inProgress(); ++i) {
        uint16_t sentinel = 0xCAFE;
        assert(g.owner->readClause22(2, sentinel, {Host::state().nowUs + 100000, 1}).is(Err::BUSY));
        assert(sentinel == 0xCAFE && g.mac.frames.empty());
        OperationSnapshot operation;
        assert(f.owner->operationSnapshot(operation).ok());
        if (operation.nextPollValid) Host::state().nowUs = std::max(Host::state().nowUs, operation.nextPollAtUs);
        status = f.owner->pollOperation(token);
        assert(f.bus.admission == BusAdmission::INITIALIZING && !Host::semaphoreHeld(f.bus.mutex));
    }
    if (!status.ok()) {
        OperationSnapshot operation;
        assert(f.owner->operationSnapshot(operation).ok());
        std::cerr << "hard reset failure " << static_cast<int>(status.code) << ":" << status.detail << " " << status.msg
                  << " phase " << static_cast<int>(operation.phase) << " next " << operation.nextPollValid << ":" << operation.nextPollAtUs
                  << " quiet " << operation.busQuietValid << ":" << operation.busQuietUntilUs << " now " << Host::state().nowUs << " frames " << f.mac.frames.size() << "\n";
    }
    assert(status.ok());
    OperationResult result;
    assert(f.owner->takeResult(token, result).ok() && result.status.ok());
    assert(f.bus.admission == BusAdmission::AVAILABLE && !Host::semaphoreHeld(f.bus.mutex));
    Host::state().nowUs = std::max(Host::state().nowUs, f.bus.quietUntilUs.load());
    uint16_t id = 0;
    assert(g.owner->readClause22(2, id, {Host::state().nowUs + 100000, 1}).ok() && id == 0x2000);
    assert(f.owner->reconcileSdkState(Host::state().nowUs + 100000).ok());
}
void boardRespectsInitializationSilence() {
    Host::reset(); Fixture f; Fixture g; g.config.bus = &f.bus;
    f.install(); f.initialize(); g.create();
    OperationToken token;
    assert(f.owner->startInitialize({ResetKind::HARD}, {{Host::state().nowUs + 1000000, 512}, 10, 16}, token).inProgress());
    unsigned pins = 0;
    BoardControl control; control.context = &pins;
    control.setReset = [](void* context, bool) { ++*static_cast<unsigned*>(context); return ESP_OK; };
    const auto frames = f.mac.frames.size(); const auto before = Host::state().nowUs;
    assert(g.owner->prepareBoard(control, before + 100000) == ESP_ERR_INVALID_STATE);
    assert(pins == 0 && Host::state().nowUs == before && f.mac.frames.size() == frames && g.mac.frames.empty());
    assert(f.bus.admission == BusAdmission::INITIALIZING && !Host::semaphoreHeld(f.bus.mutex));
    // Rejection cannot release another owner's reset/replay reservation.
    uint16_t sentinel = 0xCAFE;
    assert(f.owner->readClause22(2, sentinel, {before + 100000, 1}).is(Err::BUSY) && sentinel == 0xCAFE);
    assert(f.finish(token).status.ok() && f.bus.admission == BusAdmission::AVAILABLE);
    assert(g.owner->prepareBoard(control, Host::state().nowUs + 100000) == ESP_OK);
    assert(pins == 2 && f.bus.admission == BusAdmission::AVAILABLE && g.mac.frames.empty());
    assert(g.owner->destroy() == ESP_OK);
}
void boardSilenceExcludesOtherOwners() {
    Host::reset(); Fixture f; Fixture g; Fixture h;
    g.config.bus = &f.bus; h.config.bus = &f.bus;
    f.install(); f.initialize(); g.create(); h.create();
    struct Context { Fixture* initialized; Fixture* otherBoard; unsigned pins = 0; unsigned otherPins = 0; } context{&f, &h};
    BoardControl control; control.context = &context;
    control.setReset = [](void* opaque, bool) {
        auto& context = *static_cast<Context*>(opaque);
        auto& first = *context.initialized; auto& other = *context.otherBoard;
        ++context.pins;
        assert(first.bus.admission == BusAdmission::BOARD_PREPARING && !Host::semaphoreHeld(first.bus.mutex));
        const auto frames = first.mac.frames.size();
        OperationToken token{}; const auto original = token;
        assert(first.owner->startInitialize({ResetKind::NONE}, {{Host::state().nowUs + 1000000, 512}, 10, 16}, token).is(Err::BUSY));
        assert(token.value == original.value && first.mac.frames.size() == frames);
        BoardControl nested; nested.context = &context.otherPins;
        nested.setReset = [](void* pins, bool) { ++*static_cast<unsigned*>(pins); return ESP_OK; };
        assert(other.owner->prepareBoard(nested, Host::state().nowUs + 100000) == ESP_ERR_INVALID_STATE);
        assert(context.otherPins == 0 && other.mac.frames.empty() && first.bus.admission == BusAdmission::BOARD_PREPARING);
        return ESP_OK;
    };
    assert(g.owner->prepareBoard(control, Host::state().nowUs + 100000) == ESP_OK);
    assert(context.pins == 2 && context.otherPins == 0 && f.bus.admission == BusAdmission::AVAILABLE);
    // Successful board readiness releases only its own reservation.
    OperationToken token;
    assert(f.owner->startInitialize({ResetKind::NONE}, {{Host::state().nowUs + 1000000, 512}, 10, 16}, token).inProgress());
    assert(f.finish(token).status.ok() && f.bus.admission == BusAdmission::AVAILABLE);
    assert(g.owner->destroy() == ESP_OK && h.owner->destroy() == ESP_OK);
}
void failedBoardSilenceRemainsClosed() {
    Host::reset(); Fixture f; Fixture g; Fixture h;
    g.config.bus = &f.bus; h.config.bus = &f.bus;
    f.install(); f.initialize(); g.create(); h.create();
    unsigned pins = 0;
    BoardControl failed; failed.context = &pins;
    failed.setReset = [](void* context, bool) { ++*static_cast<unsigned*>(context); return ESP_FAIL; };
    assert(g.owner->prepareBoard(failed, Host::state().nowUs + 100000) == ESP_FAIL);
    assert(pins == 1 && f.bus.admission == BusAdmission::FAILED_CLOSED && !Host::semaphoreHeld(f.bus.mutex));
    assert(g.owner->destroy() == ESP_OK); // Shared closure does not borrow the deleted owner's address.
    unsigned otherPins = 0;
    BoardControl other; other.context = &otherPins;
    other.setReset = [](void* context, bool) { ++*static_cast<unsigned*>(context); return ESP_OK; };
    const auto before = Host::state().nowUs; const auto frames = f.mac.frames.size();
    assert(h.owner->prepareBoard(other, before + 100000) == ESP_ERR_INVALID_STATE);
    OperationToken token{}; const auto original = token;
    assert(f.owner->startInitialize({ResetKind::NONE}, {{before + 1000000, 512}, 10, 16}, token).is(Err::BUSY));
    uint16_t sentinel = 0xCAFE;
    assert(f.owner->readClause22(2, sentinel, {before + 100000, 1}).is(Err::BUSY) && sentinel == 0xCAFE);
    assert(token.value == original.value && otherPins == 0 && Host::state().nowUs == before);
    assert(f.bus.admission == BusAdmission::FAILED_CLOSED && f.mac.frames.size() == frames && h.mac.frames.empty());
    assert(h.owner->destroy() == ESP_OK);
}
void flowCompletionContention() {
    Host::reset(); Fixture f; f.install(); f.initialize();
    LinkSettings settings; settings.advertisement = {LINK_100_FULL, true, false};
    f.configure(settings);
    f.mac.onCall = [](const char* operation) {
        if (std::string(operation) == "mac.flow") Host::state().failSemaphoreTakes = 1;
    };
    assert(f.owner->setFlowControl(true, 1, Host::state().nowUs + 100000) == ESP_ERR_INVALID_STATE);
    f.mac.onCall = nullptr;
    FlowControlRecord record;
    assert(f.owner->flowControlSnapshot(record).ok());
    assert(record.macAttempted && record.macAccepted && record.macEffectPossible && record.macError == ESP_OK);
    assert(!record.phyAttempted && !record.phyAccepted && !record.reconciled && f.mac.flowEnabled);
    assert(f.owner->setFlowControl(false, 1, Host::state().nowUs + 100000) == ESP_OK);
    assert(!f.mac.flowEnabled);
}
void forcedPartnerPause() {
    for (const auto speed : {LinkSpeed::MBPS_10, LinkSpeed::MBPS_100}) {
        Host::reset(); Fixture f; f.install(); f.initialize();
        f.configure(forcedSettings(speed));
        f.mac.registers[5] |= 0x0400;
        assert(f.start() == ESP_OK);
        assert(f.journal().publishedTuple.up && !f.journal().publishedTuple.peerPause);
    }
}
void stoppingLatePoll() {
    Host::reset(); Fixture f; f.install(); f.initialize();
    assert(f.start() == ESP_OK);
    Host::state().holdOneShot = true;
    assert(f.owner->stop(Host::state().nowUs + 2000) == ESP_ERR_TIMEOUT);
    const auto events = Host::state().events.size();
    const auto frames = f.mac.frames.size();
    auto* phy = f.owner->nativePhy();
    (void)phy->get_link(phy); // A timer callback queued before the stop reservation.
    assert(Host::state().events.size() == events && f.mac.frames.size() == frames);
    Host::state().holdOneShot = false;
    const auto stopped = f.owner->stop(Host::state().nowUs + 100000);
    assert(stopped == ESP_OK || stopped == ESP_ERR_INVALID_STATE);
    const auto stops = std::count_if(Host::state().events.begin(), Host::state().events.end(), [](const auto& e) { return e.id == ETHERNET_EVENT_STOP; });
    assert(stops == 1 && Host::state().oneShotArms == 1);
    assert(!f.journal().publishedTuple.up);
}
void foreignFlowEffect() {
    Host::reset(); Fixture f; f.install(); f.initialize();
    LinkSettings settings; settings.advertisement = {LINK_100_FULL, true, false};
    f.configure(settings);
    unsigned entries = 0;
    bool injected = false;
    f.mac.onCall = [&](const char* operation) {
        if (std::string(operation) != "mac.flow") return;
        Host::state().beforeSemaphoreTake = [&](SemaphoreHandle_t) {
            if (++entries != 2) return; // Owner's PAUSE callback acquisition.
            Host::state().beforeSemaphoreTake = nullptr;
            f.mac.onCall = nullptr;
            Host::state().currentTask = 2;
            // Exercise the two public callback boundaries of an arbitrary flow
            // ioctl separately, avoiding its known bool*/uint32_t* defect.
            assert(f.mac.native.enable_flow_ctrl(&f.mac.native, false) == ESP_OK);
            Host::state().failSemaphoreTakes = 1;
            auto* phy = f.owner->nativePhy();
            assert(phy->advertise_pause_ability(phy, 1) == ESP_ERR_INVALID_STATE);
            Host::state().currentTask = 1;
            injected = true;
        };
    };
    assert(f.owner->setFlowControl(true, 1, Host::state().nowUs + 100000) == ESP_ERR_INVALID_STATE);
    assert(injected && !f.mac.flowEnabled);
    FlowControlRecord record;
    assert(f.owner->flowControlSnapshot(record).ok());
    assert(record.macAccepted && record.macEffectPossible && record.phyAttempted && !record.reconciled);
    assert(record.phyError == ESP_ERR_INVALID_STATE);
    assert(f.owner->setFlowControl(false, 1, Host::state().nowUs + 100000) == ESP_OK);
    assert(f.owner->flowControlSnapshot(record).ok() && record.reconciled);
}
void startGateFailures() {
    const auto eventCount = [](int32_t id) {
        return std::count_if(Host::state().events.begin(), Host::state().events.end(), [id](const auto& e) { return e.id == id; });
    };
    for (const bool cachedAn : {true, false}) {
        Host::reset(); Fixture f; f.install(); f.initialize();
        if (!cachedAn) f.configure(forcedSettings(LinkSpeed::MBPS_10));
        assert(f.owner->invalidateDeviceState(InvalidationReason::EXTERNAL_MUTATION).ok());
        const auto frames = f.mac.frames.size();
        Host::state().failSemaphoreTakes = 1;
        assert(esp_eth_start(f.handle) == ESP_ERR_INVALID_STATE);
        assert(Host::state().failSemaphoreTakes == 0 && f.mac.frames.size() == frames);
        assert(eventCount(ETHERNET_EVENT_START) == (cachedAn ? 0 : 1));
        assert(esp_eth_start(f.handle) == ESP_ERR_INVALID_STATE); // SDK FSM changed before the denied callback.
        assert(f.owner->stop(Host::state().nowUs + 100000) == ESP_ERR_INVALID_STATE);
        assert(eventCount(ETHERNET_EVENT_STOP) == (cachedAn ? 0 : 1));
        assert(Host::state().oneShotArms == 1 && f.mac.frames.size() == frames);
        assert(std::count(Host::state().calls.begin(), Host::state().calls.end(), "mac.stop") == 1);
        (void)f.owner->stop(Host::state().nowUs + 100000);
        assert(eventCount(ETHERNET_EVENT_STOP) == (cachedAn ? 0 : 1));
        assert(f.mac.frames.size() == frames);
    }
    Host::reset(); Fixture f; f.install(); f.initialize();
    f.configure(forcedSettings(LinkSpeed::MBPS_10));
    const auto frames = f.mac.frames.size();
    bool injected = false;
    Host::state().beforeSemaphoreTake = [&](SemaphoreHandle_t) {
        if (eventCount(ETHERNET_EVENT_START) != 1) return;
        Host::state().beforeSemaphoreTake = nullptr;
        Host::state().failSemaphoreTakes = 1;
        injected = true;
    };
    assert(f.owner->start(Host::state().nowUs + 100000) == ESP_ERR_INVALID_STATE);
    assert(injected && eventCount(ETHERNET_EVENT_START) == 1 && eventCount(ETHERNET_EVENT_STOP) == 1);
    assert(Host::state().oneShotArms == 1 && f.mac.frames.size() == frames);
    assert(std::count(Host::state().calls.begin(), Host::state().calls.end(), "mac.stop") == 1);
    f.configure(forcedSettings(LinkSpeed::MBPS_100)); // Cleanup restored normal stopped admission.
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
    else if (suite == "live_tuple_changes") liveTupleChanges();
    else if (suite == "incomplete_negotiation") incompleteNegotiation();
    else if (suite == "pause_projection") pauseProjection();
    else if (suite == "notification_partial_receipts") notificationPartialReceipts();
    else if (suite == "notification_late_retry") notificationLateRetry();
    else if (suite == "notification_completion_contention") notificationCompletionContention();
    else if (suite == "notification_late_event_retry") notificationLateEventRetry();
    else if (suite == "board_tick_readiness") boardTickReadiness();
    else if (suite == "poll_period_bounds") pollPeriodBounds();
    else if (suite == "start_completion_contention") startCompletionContention();
    else if (suite == "stop_completion_contention") stopCompletionContention();
    else if (suite == "forbidden_lifecycle") forbiddenLifecycle();
    else if (suite == "discovery") discovery();
    else if (suite == "discovery_admission") discoveryAdmission();
    else if (suite == "discovery_accounting") discoveryAccounting();
    else if (suite == "terminal_event_retry") terminalEventRetry();
    else if (suite == "installer_contention") installerContention();
    else if (suite == "raw_read_arbitration") rawReadArbitration();
    else if (suite == "raw_lock_failures") rawLockFailures();
    else if (suite == "controller_wait_classification") controllerWaitClassification();
    else if (suite == "raw_boundary_failures") rawBoundaryFailures();
    else if (suite == "rejected_raw_reads") rejectedRawReads();
    else if (suite == "forced_lifecycle") forcedLifecycle();
    else if (suite == "raw_state_reconciliation") rawStateReconciliation();
    else if (suite == "direct_start_invalidation") directStartInvalidation();
    else if (suite == "flow_control") flowControl();
    else if (suite == "flow_control_callback_bounds") flowControlCallbackBounds();
    else if (suite == "sdk_control_callback_bounds") sdkControlCallbackBounds();
    else if (suite == "raw_mutation_arbitration") rawMutationArbitration();
    else if (suite == "raw_mutation_admission") rawMutationAdmission();
    else if (suite == "collector_partial_evidence") collectorPartialEvidence();
    else if (suite == "uncertain_basic_control") uncertainBasicControl();
    else if (suite == "configuration_admission") configurationAdmission();
    else if (suite == "hard_reset_silence") hardResetSilence();
    else if (suite == "board_respects_initialization_silence") boardRespectsInitializationSilence();
    else if (suite == "board_silence_excludes_other_owners") boardSilenceExcludesOtherOwners();
    else if (suite == "failed_board_silence_remains_closed") failedBoardSilenceRemainsClosed();
    else if (suite == "flow_completion_contention") flowCompletionContention();
    else if (suite == "forced_partner_pause") forcedPartnerPause();
    else if (suite == "stopping_late_poll") stoppingLatePoll();
    else if (suite == "foreign_flow_effect") foreignFlowEffect();
    else if (suite == "start_gate_failures") startGateFailures();
    else return 2;
    Host::reset(); std::cout << suite << " passed\n";
}
