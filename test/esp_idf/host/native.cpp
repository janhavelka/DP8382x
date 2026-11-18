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
    Fixture(Fixture& controller, uint8_t address) : Fixture() {
        config.mac = &controller.mac.native;
        config.bus = &controller.bus;
        config.core.phyAddress = address;
    }
    void create() { assert(Native::create(config, owner) == ESP_OK); }
    void board() {
        BoardControl control;
        control.setReset = [](void*, bool asserted) { Host::state().calls.emplace_back(asserted ? "board.assert" : "board.release"); return ESP_OK; };
        assert(owner->prepareBoard(control, Host::state().nowUs + 100000) == ESP_OK);
        assert(Host::state().nowUs >= 50000 && mac.frames.empty());
    }
    void install() { create(); board(); assert(owner->install(handle) == ESP_OK); assert(Host::isLive(handle) && mac.frames.empty()); }
    Status pollToResult(OperationToken token) {
        Status status{Err::IN_PROGRESS, 0, "Pending"};
        for (unsigned i = 0; i < 256 && status.is(Err::IN_PROGRESS); ++i) {
            OperationSnapshot snapshot;
            assert(owner->operationSnapshot(snapshot).ok());
            if (snapshot.nextPollValid) Host::state().nowUs = std::max(Host::state().nowUs, snapshot.nextPollAtUs);
            status = owner->pollOperation(token);
        }
        assert(!status.inProgress());
        return status;
    }
    OperationResult finish(OperationToken token) {
        const auto status = pollToResult(token);
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
            std::cerr << "start error " << error << " journal " << errorName(record.firstError.coreStatus.code)
                      << ":" << record.firstError.sdkError << " cleanup " << record.cleanupError.sdkError << "\n";
            for (const auto& call : Host::state().calls) std::cerr << call << " ";
            std::cerr << "\n";
        }
        return error;
    }
};

// A second PHY uses the installed controller's MDIO route under the application
// bus protocol. It never installs another ESP Ethernet driver or sets a mediator.
esp_err_t readPeerIdentity(Fixture& controller, uint16_t& output) {
    auto& bus = controller.bus;
    const auto available = [&] {
        return bus.admission == BusAdmission::AVAILABLE && !bus.quarantined &&
               Host::state().nowUs >= bus.quietUntilUs.load();
    };
    if (!available() || xSemaphoreTake(bus.mutex, 0) != pdTRUE) return ESP_ERR_INVALID_STATE;
    uint32_t value = 0;
    auto& mac = controller.mac.native;
    const auto error = available() ? mac.read_phy_reg(&mac, 1, 2, &value) : ESP_ERR_INVALID_STATE;
    assert(xSemaphoreGive(bus.mutex) == pdTRUE);
    if (error == ESP_OK) output = static_cast<uint16_t>(value);
    return error;
}

size_t countCall(const char* name) {
    return std::count(Host::state().calls.begin(), Host::state().calls.end(), name);
}

std::vector<int32_t> eventOrder() {
    std::vector<int32_t> ids;
    for (const auto& event : Host::state().events) ids.push_back(event.id);
    return ids;
}
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
    assert(phy->deinit(phy) == ESP_OK);
    assert(f.journal().phase == Phase::STOPPED && f.journal().firstError.ok());
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

void assertFailedOperation(Fixture& f, OperationToken token, OperationKind kind, Err expected,
                           int32_t detail, BusAdmission reserved, OperationResult& result) {
    OperationSnapshot pending;
    assert(f.owner->operationSnapshot(pending).ok());
    assert(pending.token.value == token.value && pending.kind == kind && pending.state == OperationState::RESULT_PENDING);
    assert(pending.status.is(expected) && pending.status.detail == detail);
    assert(f.bus.admission == reserved && !Host::semaphoreHeld(f.bus.mutex));
    auto journal = f.journal();
    assert(journal.stage == (kind == OperationKind::INITIALIZE ? LifecycleStage::INITIALIZE : LifecycleStage::CONFIGURE));
    assert(journal.firstError.is(expected) && journal.firstError.coreStatus.detail == detail);
    assert(journal.cleanupError.ok());
    const auto frames = f.mac.frames.size();
    uint16_t id = 0xCAFE;
    assert(f.owner->readClause22(2, id, {Host::state().nowUs + 100000, 1}).is(Err::BUSY) && id == 0xCAFE);
    assert(f.owner->start(Host::state().nowUs + 100000) == ESP_ERR_INVALID_STATE);
    assert(f.owner->takeResult(token, result).ok() && result.kind == kind);
    assert(result.token.value == token.value && result.status.is(expected) && result.status.detail == detail);
    assert(f.owner->takeResult(token, result).is(Err::RESULT_NOT_AVAILABLE));
    assert(f.owner->operationSnapshot(pending).ok() && pending.state == OperationState::IDLE);
    assert(f.bus.admission == BusAdmission::AVAILABLE && !Host::semaphoreHeld(f.bus.mutex));
    assert(f.owner->start(Host::state().nowUs + 100000) == ESP_ERR_INVALID_STATE);
    SdkStateSnapshot sdk;
    assert(f.owner->sdkStateSnapshot(sdk).ok() && !sdk.reconciled);
    assert(f.mac.frames.size() == frames && eventOrder().empty() && Host::state().periodicPeriodsUs.empty());
    assert(Host::isLive(f.handle) && f.mac.alive && f.owner->destroy() == ESP_ERR_INVALID_STATE);
    journal = f.journal();
    assert(journal.firstError.is(expected) && journal.firstError.coreStatus.detail == detail);
}

void initializeFailures() {
    for (const auto failure : {Err::MDIO_TIMEOUT, Err::DEVICE_MISMATCH, Err::CANCELLED}) {
        Host::reset(); Fixture f; f.install();
        OperationToken token;
        const OperationBudget budget{{Host::state().nowUs + 1000000, 512}, 10, 16};
        assert(f.owner->startInitialize({ResetKind::NONE}, budget, token).inProgress());
        assert(f.journal().stage == LifecycleStage::INITIALIZE && f.bus.admission == BusAdmission::INITIALIZING);
        assert(f.owner->start(Host::state().nowUs + 100000) == ESP_ERR_INVALID_STATE);
        const OperationToken wrong{token.value + 1U};
        assert(f.owner->pollOperation(wrong).is(Err::TOKEN_MISMATCH));
        assert(f.owner->cancel(wrong).is(Err::TOKEN_MISMATCH) && f.journal().firstError.ok());
        int32_t detail = 0;
        if (failure == Err::MDIO_TIMEOUT) {
            f.mac.readResults = {ESP_OK, ESP_ERR_TIMEOUT}; // Stop after capturing PHYIDR1.
            detail = ESP_ERR_TIMEOUT;
        } else if (failure == Err::DEVICE_MISMATCH) {
            f.mac.registers[2] = 0x3000;
            detail = static_cast<int32_t>(IdentityDetail::WRONG_OUI);
        }
        if (failure == Err::CANCELLED) {
            assert(f.owner->cancel(token).ok()); // Deliver directly, without an extra poll to journal it.
            assert(f.mac.frames.empty());
        } else {
            assert(f.pollToResult(token).is(failure));
        }
        OperationResult result;
        assertFailedOperation(f, token, OperationKind::INITIALIZE, failure, detail, BusAdmission::INITIALIZING, result);
        assert(!result.mutationAttempted && !result.mutationPossible && !result.cleanupOwed);
        assert(f.bus.quarantined == (failure == Err::MDIO_TIMEOUT));
        if (failure == Err::MDIO_TIMEOUT) {
            assert(result.failingBatch.attemptedFrames == 2 && result.failingBatch.completedFrames == 1);
            assert(f.mac.frames.size() == 1 && f.mac.frames.front().reg == 2);
        }
        if (failure == Err::DEVICE_MISMATCH) {
            assert(result.identity.id1Valid && result.identity.id1 == 0x3000 && result.identity.id2Valid);
            assert(result.identity.acceptance == IdentityAcceptance::REJECTED);
        }
        uint16_t id = 0xCAFE;
        const auto raw = f.owner->readClause22(2, id, {Host::state().nowUs + 100000, 1});
        if (failure == Err::MDIO_TIMEOUT) assert(raw.is(Err::BUSY) && id == 0xCAFE);
        else assert(raw.ok() && id == f.mac.registers[2]);
    }
}

void configurationFailures() {
    for (const auto failure : {Err::MDIO_TIMEOUT, Err::CANCELLED}) {
        Host::reset(); Fixture f; f.install(); f.initialize();
        LinkSettings settings;
        settings.advertisement.technologyMask = LINK_10_FULL;
        OperationToken token;
        assert(f.owner->startConfigure(settings, {{Host::state().nowUs + 1000000, 512}, 10, 16}, token).inProgress());
        assert(f.bus.admission == BusAdmission::AVAILABLE);
        assert(f.owner->start(Host::state().nowUs + 100000) == ESP_ERR_INVALID_STATE);
        bool anarWritten = false;
        f.mac.onFrame = [&](const Host::Frame& frame) {
            if (frame.write && frame.reg == 4) {
                anarWritten = true;
                if (failure == Err::MDIO_TIMEOUT) f.mac.errors["mac.read"] = ESP_ERR_TIMEOUT;
            }
        };
        Status status{Err::IN_PROGRESS, 0, "Pending"};
        for (unsigned poll = 0; poll < 256 && status.inProgress(); ++poll) {
            if (failure == Err::CANCELLED && anarWritten) break;
            OperationSnapshot snapshot;
            assert(f.owner->operationSnapshot(snapshot).ok());
            if (snapshot.nextPollValid) Host::state().nowUs = std::max(Host::state().nowUs, snapshot.nextPollAtUs);
            status = f.owner->pollOperation(token);
        }
        f.mac.onFrame = nullptr;
        assert(anarWritten && (f.mac.registers[4] & 0x01E0) == 0x0040);
        if (failure == Err::CANCELLED) {
            assert(status.inProgress() && f.owner->cancel(token).ok());
        } else {
            assert(status.is(Err::MDIO_TIMEOUT));
        }
        OperationResult result;
        assertFailedOperation(f, token, OperationKind::CONFIGURE, failure, failure == Err::CANCELLED ? 0 : ESP_ERR_TIMEOUT,
                              BusAdmission::AVAILABLE, result);
        assert(result.mutationAttempted && result.mutationCompleted && result.mutationPossible && result.cleanupOwed);
        SettingsSnapshot cached;
        assert(f.owner->settingsSnapshot(cached).ok() && cached.desiredValid && cached.observedStale);
        assert(cached.trust == ConfigurationTrust::DIRTY && cached.desired.advertisement.technologyMask == LINK_10_FULL);
        assert(f.bus.quarantined == (failure == Err::MDIO_TIMEOUT));
        uint16_t id = 0xCAFE;
        const auto raw = f.owner->readClause22(2, id, {Host::state().nowUs + 100000, 1});
        if (failure == Err::MDIO_TIMEOUT) assert(raw.is(Err::BUSY) && id == 0xCAFE);
        else {
            assert(raw.ok() && id == 0x2000);
            f.configure(settings); // Explicit restoration after the accepted prefix.
            f.mac.registers[0x10] = 0x0017;
            assert(f.start() == ESP_OK);
        }
    }
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
    assert(f.journal().firstError.sdkError == ESP_ERR_TIMEOUT);
    assert(Host::isLive(f.handle) && f.mac.alive);
    Host::reset(); Fixture g; g.install(); g.initialize();
    assert(g.start() == ESP_OK);
    Host::state().holdOneShot = true;
    assert(g.owner->stop(Host::state().nowUs + 2000) == ESP_ERR_TIMEOUT);
    assert(Host::state().oneShotArms == 1 && Host::isLive(g.handle));
    Host::state().holdOneShot = false;
    const auto second = g.owner->stop(Host::state().nowUs + 100000);
    assert(second == ESP_OK);
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
        assert(f.journal().publicationUncertain && f.journal().firstError.sdkError == ESP_ERR_TIMEOUT);
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
    Host::reset(); Fixture f; f.install();
    uint16_t output = 0xCAFE;
    assert(readPeerIdentity(f, output) == ESP_OK && output == 0x2000);
    assert(f.mac.frames.size() == 1 && f.mac.frames.back().address == 1);
    assert(countCall("mac.init") == 1);
    output = 0xCAFE;
    f.mac.errors["mac.read"] = ESP_ERR_TIMEOUT;
    const auto first = f.owner->readClause22(2, output, {Host::state().nowUs + 10000, 1});
    assert(first.detail == ESP_ERR_TIMEOUT && output == 0xCAFE && f.bus.quarantined);
    const auto reads = countCall("mac.read");
    assert(readPeerIdentity(f, output) == ESP_ERR_INVALID_STATE && output == 0xCAFE);
    assert(countCall("mac.read") == reads);
}
void callbackBounds() {
    Host::reset(); Fixture f; f.config.notificationUs = 40000; f.config.callbackBudgetUs = 215000;
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
    assert(record.firstError.is(Err::OPERATION_TIMEOUT) && Host::isLive(g.handle));
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
    LinkSettings settings; settings.advertisement = {LINK_TECHNOLOGY_MASK, true, false};
    f.configure(settings);
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
        f.mac.registers[0x19] = row.pause ? 0xB001 : 0x8001;
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
    // Order of local and partner advertisements: none, symmetric, asymmetric,
    // both. Expected outputs are {resolved RX, resolved TX, P4 Boolean}.
    struct Pause { bool rx; bool tx; uint32_t peer; };
    const Pause cases[4][4] = {
        {{false, false, 0}, {false, false, 0}, {false, false, 0}, {false, false, 0}},
        {{false, false, 0}, {true, true, 1},   {false, false, 0}, {true, true, 1}},
        {{false, false, 0}, {false, false, 0}, {false, false, 0}, {false, true, 0}},
        {{false, false, 0}, {true, true, 1},   {true, false, 0},  {true, true, 1}},
    };
    for (uint16_t local = 0; local < 4; ++local) {
        Host::reset(); Fixture f; f.install(); f.initialize();
        LinkSettings settings; settings.advertisement = {LINK_100_FULL, (local & 1U) != 0U, (local & 2U) != 0U};
        f.configure(settings);
        const auto enabled = f.owner->setFlowControl(true, local, Host::state().nowUs + 100000);
        assert(enabled == ESP_OK && f.mac.flowEnabled && f.mac.flowRequests.size() == 1);
        for (uint16_t peer = 0; peer < 4; ++peer) {
            const auto& expected = cases[local][peer];
            const uint16_t partner = static_cast<uint16_t>(0x0101U | (peer << 10U));
            const uint16_t phycr = static_cast<uint16_t>(0x8001U | (expected.rx ? 0x2000U : 0U) |
                                                       (expected.tx ? 0x1000U : 0U));
            f.mac.registers[5] = partner; f.mac.registers[0x19] = phycr;
            if (peer == 0) assert(f.start() == ESP_OK);
            else Host::tickTimers();
            LinkSnapshot observation;
            assert(f.owner->linkSnapshot(observation).ok());
            assert(observation.pauseValid && observation.pauseRx == expected.rx && observation.pauseTx == expected.tx);
            assert(observation.rawAnlpar == partner && observation.rawPhycr == phycr);
            assert(f.mac.pauseRequests.back() == expected.peer && f.journal().publishedTuple.peerPause == (expected.peer != 0));
            assert(f.mac.flowRequests.size() == 1); // Application policy stays separate.
        }
        if (local == 3) {
            f.mac.registers[0x19] = 0xA001; // RX only disagrees with symmetric negotiation.
            Host::tickTimers();
            LinkSnapshot observation;
            assert(f.owner->linkSnapshot(observation).ok() && observation.linkUp && !observation.pauseValid);
            assert(f.mac.pauseRequests.back() == 0 && !f.journal().publishedTuple.peerPause);
        }
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
        assert(!record.publicationUncertain && record.firstError.is(Err::OPERATION_TIMEOUT));
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
        assert(f.owner->stop(Host::state().nowUs + 100000) == ESP_OK && injected);
        Host::state().onTimerOnce = nullptr; Host::state().onEventPost = nullptr;
        const auto stops = [] {
            return std::count_if(Host::state().events.begin(), Host::state().events.end(), [](const auto& event) { return event.id == ETHERNET_EVENT_STOP; });
        };
        assert(stops() == 1 && Host::state().oneShotArms == 1);
        assert(f.owner->stop(Host::state().nowUs + 100000) == ESP_OK);
        assert(stops() == 1 && Host::state().oneShotArms == 1); // Accepted SDK stop and fence arm cannot be replayed.
        assert(f.mac.frames.size() == frames && !f.journal().publishedTuple.up && Host::isLive(f.handle));
    }
}
void stopFailureReceipts() {
    for (const bool macFailure : {false, true}) {
        Host::reset(); Fixture f; f.install(); f.initialize(); assert(f.start() == ESP_OK);
        if (macFailure) {
            f.mac.errors["mac.stop"] = ESP_FAIL;
            f.mac.onCall = [&](const char* operation) {
                if (std::string(operation) == "mac.stop") Host::state().failSemaphoreTakes = 1;
            };
        } else {
            Host::state().holdOneShot = true;
            Host::state().onTimerOnce = [] { Host::state().failSemaphoreTakes = 1; };
        }
        const auto expected = macFailure ? ESP_FAIL : ESP_ERR_TIMEOUT;
        assert(f.owner->stop(Host::state().nowUs + 100000) == expected);
        assert(Host::state().failSemaphoreTakes == 1); // No gate was needed to retain this receipt.
        Host::state().failSemaphoreTakes = 0;
        const auto failure = f.journal().cleanupError;
        if (macFailure) assert(failure.sdkError == ESP_FAIL && failure.coreStatus.ok());
        else assert(failure.is(Err::OPERATION_TIMEOUT));
        f.mac.onCall = nullptr;
        Host::state().holdOneShot = false;
        assert(f.owner->stop(Host::state().nowUs + 100000) == ESP_OK);
        assert(f.journal().cleanupError.sdkError == failure.sdkError);
        assert(f.journal().cleanupError.coreStatus.code == failure.coreStatus.code);
        assert(Host::state().oneShotArms == 1 && countCall("mac.stop") == (macFailure ? 2U : 1U));
        assert((eventOrder() == std::vector<int32_t>{ETHERNET_EVENT_START, ETHERNET_EVENT_CONNECTED,
                                                    ETHERNET_EVENT_DISCONNECTED, ETHERNET_EVENT_STOP}));
    }
    Host::reset(); Fixture f; f.config.pinnedP4Mac = true; f.install(); f.initialize(); assert(f.start() == ESP_OK);
    f.mac.registers[1] = 0x7849; f.mac.registers[0x10] = 0;
    Host::state().eventErrorId = ETHERNET_EVENT_DISCONNECTED;
    Host::state().nextEventError = ESP_ERR_TIMEOUT;
    Host::tickTimers();
    assert(f.journal().eventLost && !f.journal().publishedTuple.up);
    Host::state().nextEventError = ESP_FAIL;
    Host::state().onEventAttempt = [](int32_t id) {
        if (id == ETHERNET_EVENT_DISCONNECTED) Host::state().failSemaphoreTakes = 1;
    };
    assert(f.owner->stop(Host::state().nowUs + 100000) == ESP_FAIL);
    assert(Host::state().failSemaphoreTakes == 1);
    Host::state().failSemaphoreTakes = 0;
    Host::state().onEventAttempt = nullptr;
    auto record = f.journal();
    assert(record.firstError.sdkError == ESP_ERR_TIMEOUT && record.cleanupError.sdkError == ESP_FAIL && record.eventLost);
    assert((eventOrder() == std::vector<int32_t>{ETHERNET_EVENT_START, ETHERNET_EVENT_CONNECTED}));
    assert(f.owner->stop(Host::state().nowUs + 100000) == ESP_OK);
    record = f.journal();
    assert(record.cleanupError.sdkError == ESP_FAIL && !record.eventLost && !record.publishedTuple.up);
    assert(countCall("mac.down") == 1 && countCall("mac.stop") == 1 && Host::state().oneShotArms == 1);
    assert((eventOrder() == std::vector<int32_t>{ETHERNET_EVENT_START, ETHERNET_EVENT_CONNECTED,
                                                ETHERNET_EVENT_DISCONNECTED, ETHERNET_EVENT_STOP}));
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
void dependentCallbackBounds() {
    Host::reset(); Fixture f;
    auto* sentinel = reinterpret_cast<Native*>(0x1234);
    const auto verifyInvalid = [&](const PhyConfig& config) {
        Host::state().failSemaphore = true;
        assert(Native::create(config, sentinel) == ESP_ERR_INVALID_ARG && sentinel == reinterpret_cast<Native*>(0x1234));
        assert(Host::state().failSemaphore && Host::state().timerCreates == 0);
        Host::state().failSemaphore = false;
    };
    auto invalid = f.config; invalid.linkPollMs = 99;
    verifyInvalid(invalid);
    invalid = f.config; invalid.callbackBudgetUs = 50; // Exactly five notifications, no room for MDIO.
    verifyInvalid(invalid);
    invalid.callbackBudgetUs = 14051; // 2*2000 + 10*1000 + 1 + 5*10.
    verifyInvalid(invalid);
    invalid = f.config; invalid.cleanupBudgetUs = 0;
    verifyInvalid(invalid);
    f.config.callbackBudgetUs = 14052; f.config.linkPollMs = 15;
    f.create();
    assert(f.owner->destroy() == ESP_OK);
    f.config.callbackBudgetUs = 100000; f.config.linkPollMs = 100;
    f.create();
    assert(f.owner->destroy() == ESP_OK);
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
    f.config.core.transport.bounds.frameUs = 400000000;
    f.config.callbackBudgetUs = 4100000000U;
    f.config.linkPollMs = 4294967;
    f.install();
    DiscoverySnapshot previous;
    assert(f.owner->discover(1, previous, {Host::state().nowUs + UINT64_C(10000000000), 2}).ok());
    const auto frames = f.mac.frames.size();
    DiscoverySnapshot output; output.requestedMask = 0xCAFE;
    assert(f.owner->discover(0x3F, output, {Host::state().nowUs + 4000000000ULL, 12}).is(Err::OPERATION_TIMEOUT));
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
    assert(f.owner->retire(Host::state().nowUs + 100000) == ESP_ERR_NOT_SUPPORTED);
    assert(!f.journal().publishedTuple.up && !f.journal().eventLost);
    assert((eventOrder() == std::vector<int32_t>{ETHERNET_EVENT_START, ETHERNET_EVENT_DISCONNECTED, ETHERNET_EVENT_STOP}));
    assert(f.mac.link == ETH_LINK_DOWN && countCall("mac.stop") == 1);
    const auto events = Host::state().events.size();
    const auto ups = std::count(Host::state().calls.begin(), Host::state().calls.end(), "mac.up");
    assert(f.owner->retryLinkEvent(Host::state().nowUs + 100000) == ESP_ERR_INVALID_STATE);
    assert(Host::state().events.size() == events && std::count(Host::state().calls.begin(), Host::state().calls.end(), "mac.up") == ups);
    assert(Host::isLive(f.handle));
    Host::reset(); Fixture g; g.install(); g.initialize();
    assert(g.start() == ESP_OK);
    const auto frames = g.mac.frames.size();
    const auto calls = Host::state().calls.size();
    assert(g.owner->retire(Host::state().nowUs) == ESP_ERR_TIMEOUT);
    const auto record = g.journal();
    assert(record.stage != LifecycleStage::RETAINED && record.firstError.ok());
    assert(record.cleanupError.is(Err::OPERATION_TIMEOUT));
    assert(g.mac.frames.size() == frames && Host::state().calls.size() == calls);
    assert(g.owner->retire(Host::state().nowUs + 100000) == ESP_ERR_NOT_SUPPORTED);
    assert(g.journal().stage == LifecycleStage::RETAINED && !g.journal().publishedTuple.up && g.mac.link == ETH_LINK_DOWN);
    assert(Host::isLive(g.handle) && g.mac.alive && g.owner->destroy() == ESP_ERR_INVALID_STATE);
}

void retireAndStopAdmission() {
    for (const bool retire : {false, true}) {
        Host::reset(); Fixture f; f.install(); f.initialize(); assert(f.start() == ESP_OK);
        Host::state().failSemaphoreTakes = 2; // Timer or snapshot loses the first two gate attempts.
        const auto deadline = Host::state().nowUs + 100000;
        const auto status = retire ? f.owner->retire(deadline) : f.owner->stop(deadline);
        assert(status == (retire ? ESP_ERR_NOT_SUPPORTED : ESP_OK));
        assert(!f.journal().publishedTuple.up && f.mac.link == ETH_LINK_DOWN && countCall("mac.stop") == 1);
        assert((eventOrder() == std::vector<int32_t>{ETHERNET_EVENT_START, ETHERNET_EVENT_CONNECTED,
                                                    ETHERNET_EVENT_DISCONNECTED, ETHERNET_EVENT_STOP}));
        assert(Host::state().oneShotArms == 1 && Host::state().failSemaphoreTakes == 0);
        assert(f.journal().stage == (retire ? LifecycleStage::RETAINED : LifecycleStage::MAC_STOP));
        assert(Host::isLive(f.handle));
    }
    Host::reset(); Fixture f; f.install(); f.initialize(); assert(f.start() == ESP_OK);
    Host::state().holdOneShot = true;
    assert(f.owner->retire(Host::state().nowUs + 2000) == ESP_ERR_TIMEOUT);
    assert(f.journal().stage != LifecycleStage::RETAINED && countCall("mac.stop") == 0);
    Host::state().holdOneShot = false;
    assert(f.owner->stop(Host::state().nowUs + 100000) == ESP_OK);
    assert(f.owner->retire(Host::state().nowUs + 100000) == ESP_ERR_NOT_SUPPORTED);
    assert(f.journal().stage == LifecycleStage::RETAINED && !f.journal().publishedTuple.up);
    assert(countCall("mac.stop") == 1 && Host::state().oneShotArms == 1 && Host::isLive(f.handle));
}

void boardCompletionContention() {
    for (const auto releaseError : {ESP_OK, ESP_FAIL}) {
        Host::reset(); Fixture f; f.install(); Fixture other(f, 1); other.create();
        struct Context {
            SharedBus* bus;
            uint64_t floor = 0;
            esp_err_t releaseError;
        } context{&f.bus, 0, releaseError};
        BoardControl control; control.context = &context;
        control.setReset = [](void* opaque, bool asserted) {
            if (asserted) return ESP_OK;
            auto& context = *static_cast<Context*>(opaque);
            context.floor = Host::state().nowUs + 20000;
            context.bus->quietUntilUs.store(context.floor);
            Host::state().failSemaphoreTakes = 1; // Snapshot owns the gate at reset completion.
            return context.releaseError;
        };
        const auto before = f.mac.frames.size();
        assert(other.owner->prepareBoard(control, Host::state().nowUs + 100000) == releaseError);
        assert(f.bus.admission == (releaseError == ESP_OK ? BusAdmission::AVAILABLE : BusAdmission::FAILED_CLOSED));
        assert(!Host::semaphoreHeld(f.bus.mutex) && f.bus.quietUntilUs == context.floor);
        Host::state().failSemaphoreTakes = 0;
        uint16_t id = 0xCAFE;
        const auto peerStatus = readPeerIdentity(f, id); // No intervening call through the board owner.
        if (releaseError == ESP_OK) {
            assert(Host::state().nowUs >= context.floor && peerStatus == ESP_OK && id == 0x2000);
            assert(f.mac.frames.size() == before + 1 && f.mac.frames.back().address == 1);
            assert(other.owner->prepareBoard(control, Host::state().nowUs + 100000) == ESP_OK);
            Host::state().failSemaphoreTakes = 0;
        } else {
            assert(peerStatus == ESP_ERR_INVALID_STATE && id == 0xCAFE && f.mac.frames.size() == before);
            assert(other.journal().firstError.sdkError == ESP_FAIL);
        }
        assert(other.owner->destroy() == ESP_OK && Host::isLive(f.handle));
    }
}

void lateStartCleanup() {
    Host::reset(); Fixture f; f.install(); f.initialize();
    const auto deadline = Host::state().nowUs + 100000;
    Host::state().onTimerPeriodic = [&] { Host::state().nowUs = deadline + 1; };
    assert(f.owner->start(deadline) == ESP_ERR_TIMEOUT);
    Host::state().onTimerPeriodic = nullptr;
    const auto failed = f.journal();
    assert(failed.sequence == 1 && failed.firstError.is(Err::OPERATION_TIMEOUT) && failed.cleanupError.ok());
    assert(!failed.publishedTuple.up && f.mac.link == ETH_LINK_DOWN && countCall("mac.stop") == 1);
    assert((eventOrder() == std::vector<int32_t>{ETHERNET_EVENT_START, ETHERNET_EVENT_CONNECTED,
                                                ETHERNET_EVENT_DISCONNECTED, ETHERNET_EVENT_STOP}));
    assert(Host::state().oneShotArms == 1 && Host::isLive(f.handle));
    const auto frames = f.mac.frames.size();
    Host::tickTimers();
    assert(f.mac.frames.size() == frames);
    uint8_t packet[64]{};
    assert(esp_eth_transmit(f.handle, packet, sizeof(packet)) == ESP_ERR_INVALID_STATE && countCall("mac.transmit") == 0);
    assert(f.start() == ESP_OK);
    const auto next = f.journal();
    assert(next.sequence == 2 && next.firstError.ok() && next.cleanupError.ok() && next.publishedTuple.up);

    Host::reset(); Fixture g; g.install(); g.initialize();
    Host::state().nextTimerStartError = ESP_ERR_NO_MEM; // SDK start accepted UP but did not arm its timer.
    g.mac.errors["mac.stop"] = ESP_FAIL;
    assert(g.owner->start(Host::state().nowUs + 100000) == ESP_ERR_NO_MEM);
    const auto pending = g.journal();
    assert(pending.phase == Phase::STOPPING && pending.stopRequired && !pending.publishedTuple.up);
    assert(pending.sequence == 1 && pending.firstError.sdkError == ESP_ERR_NO_MEM);
    assert(pending.cleanupError.sdkError == ESP_ERR_INVALID_STATE); // Unarmed SDK timer.
    assert(pending.lastCleanupError.sdkError == ESP_FAIL); // Later direct MAC stop.
    assert(countCall("mac.stop") == 1 && Host::state().oneShotArms == 1);
    assert(g.owner->stop(Host::state().nowUs + 100000) == ESP_OK);
    const auto repaired = g.journal();
    assert(repaired.phase == Phase::STOPPED && !repaired.stopRequired && repaired.lastCleanupError.sdkError == ESP_FAIL);
    assert(countCall("mac.stop") == 2 && Host::state().oneShotArms == 1);
    assert((eventOrder() == std::vector<int32_t>{ETHERNET_EVENT_START, ETHERNET_EVENT_CONNECTED,
                                                ETHERNET_EVENT_DISCONNECTED, ETHERNET_EVENT_STOP}));
    assert(g.start() == ESP_OK);
    assert(g.journal().sequence == 2 && g.journal().firstError.ok() && g.journal().cleanupError.ok() &&
           g.journal().lastCleanupError.ok());
}

void uninstallGateContention() {
    Host::reset(); Fixture f; f.install(); f.initialize();
    const auto deleted = Host::state().timerDeletes;
    const auto frames = f.mac.frames.size();
    Host::state().failSemaphoreTakes = 1;
    assert(esp_eth_driver_uninstall(f.handle) == ESP_ERR_INVALID_STATE);
    assert(Host::state().timerDeletes == deleted + 1 && Host::isLive(f.handle));
    assert(f.owner->start(Host::state().nowUs + 100000) == ESP_ERR_INVALID_STATE);
    OperationToken token;
    assert(f.owner->startInitialize({ResetKind::NONE}, {{Host::state().nowUs + 1000000, 512}, 10, 16}, token).is(Err::BUSY));
    assert(f.owner->destroy() == ESP_ERR_INVALID_STATE);
    const auto record = f.journal();
    assert(record.stage == LifecycleStage::RETAINED && record.firstError.sdkError == ESP_ERR_INVALID_STATE);
    assert(f.mac.frames.size() == frames && countCall("timer.periodic") == 0 && eventOrder().empty());
}

void stopWithoutStart() {
    Host::reset(); Fixture f; f.install(); f.initialize();
    const auto calls = Host::state().calls.size();
    assert(f.owner->stop(Host::state().nowUs + 100000) == ESP_OK);
    assert(f.journal().firstError.ok() && f.journal().cleanupError.ok());
    assert(f.owner->retire(Host::state().nowUs + 100000) == ESP_ERR_NOT_SUPPORTED);
    assert(f.journal().stage == LifecycleStage::RETAINED && f.journal().cleanupError.ok());
    assert(Host::state().calls.size() == calls && eventOrder().empty() && Host::isLive(f.handle));
}

void stopRepairsLostDown() {
    Host::reset(); Fixture f; f.config.pinnedP4Mac = true; f.install(); f.initialize(); assert(f.start() == ESP_OK);
    Host::state().eventErrorId = ETHERNET_EVENT_DISCONNECTED;
    Host::state().nextEventError = ESP_ERR_TIMEOUT;
    assert(f.owner->stop(Host::state().nowUs + 100000) == ESP_OK);
    assert((eventOrder() == std::vector<int32_t>{ETHERNET_EVENT_START, ETHERNET_EVENT_CONNECTED,
                                                ETHERNET_EVENT_DISCONNECTED, ETHERNET_EVENT_STOP}));
    const auto record = f.journal();
    assert(!record.eventLost && !record.publishedTuple.up && f.mac.link == ETH_LINK_DOWN);
    assert(record.cleanupError.sdkError == ESP_ERR_TIMEOUT && record.cleanupError.coreStatus.ok());
    assert(record.lastCleanupError.sdkError == ESP_ERR_TIMEOUT && record.lastCleanupError.coreStatus.ok());
    assert(countCall("mac.down") == 1 && countCall("mac.stop") == 1 && Host::state().oneShotArms == 1);
    assert(f.owner->retryLinkEvent(Host::state().nowUs + 100000) == ESP_ERR_INVALID_STATE);
    assert(f.start() == ESP_OK && f.journal().sequence == 2 && f.journal().cleanupError.ok());

    Host::reset(); Fixture g; g.config.pinnedP4Mac = true; g.install(); g.initialize(); assert(g.start() == ESP_OK);
    g.mac.elapsedUs["mac.down"] = g.config.notificationUs + 1;
    assert(g.owner->stop(Host::state().nowUs + 100000) == ESP_OK);
    const auto lateNotification = g.journal();
    assert(lateNotification.phase == Phase::STOPPED && !lateNotification.publishedTuple.up && !lateNotification.eventLost);
    assert(lateNotification.cleanupError.is(Err::OPERATION_TIMEOUT) && lateNotification.lastCleanupError.is(Err::OPERATION_TIMEOUT));
    assert(countCall("mac.down") == 1 && countCall("mac.stop") == 1 && Host::state().oneShotArms == 1);
    assert((eventOrder() == std::vector<int32_t>{ETHERNET_EVENT_START, ETHERNET_EVENT_CONNECTED,
                                                ETHERNET_EVENT_DISCONNECTED, ETHERNET_EVENT_STOP}));

    Host::reset(); Fixture h; h.config.pinnedP4Mac = true; h.install(); h.initialize(); assert(h.start() == ESP_OK);
    h.mac.registers[1] = 0x7849; h.mac.registers[0x10] = 0;
    Host::state().eventErrorId = ETHERNET_EVENT_DISCONNECTED;
    Host::state().nextEventError = ESP_ERR_TIMEOUT;
    Host::tickTimers();
    assert(h.journal().eventLost && !h.journal().publishedTuple.up);
    Host::state().eventElapsedUs = 1001;
    assert(h.owner->stop(Host::state().nowUs + 1000) == ESP_ERR_TIMEOUT);
    const auto lateEvent = h.journal();
    assert(lateEvent.phase == Phase::STOPPING && !lateEvent.eventLost);
    assert(lateEvent.firstError.sdkError == ESP_ERR_TIMEOUT && lateEvent.firstError.coreStatus.ok());
    assert(lateEvent.cleanupError.is(Err::OPERATION_TIMEOUT) && lateEvent.lastCleanupError.is(Err::OPERATION_TIMEOUT));
    assert((eventOrder() == std::vector<int32_t>{ETHERNET_EVENT_START, ETHERNET_EVENT_CONNECTED, ETHERNET_EVENT_DISCONNECTED}));
    assert(Host::state().oneShotArms == 0 && countCall("mac.stop") == 0);
    Host::state().eventElapsedUs = 0;
    assert(h.owner->stop(Host::state().nowUs + 100000) == ESP_OK);
    assert(h.journal().phase == Phase::STOPPED && h.journal().lastCleanupError.is(Err::OPERATION_TIMEOUT));
    assert(countCall("mac.down") == 1 && countCall("mac.stop") == 1 && Host::state().oneShotArms == 1);
    assert((eventOrder() == std::vector<int32_t>{ETHERNET_EVENT_START, ETHERNET_EVENT_CONNECTED,
                                                ETHERNET_EVENT_DISCONNECTED, ETHERNET_EVENT_STOP}));
}

void observationLoss() {
    for (const bool invalidation : {false, true}) {
        Host::reset(); Fixture f; f.config.pinnedP4Mac = true; f.install(); f.initialize(); assert(f.start() == ESP_OK);
        if (invalidation) assert(f.owner->invalidateDeviceState(InvalidationReason::EXTERNAL_RESET).ok());
        else f.mac.errors["mac.read"] = ESP_ERR_TIMEOUT;
        Host::tickTimers();
        const auto record = f.journal();
        assert(record.stopRequired && !record.publishedTuple.up && f.mac.link == ETH_LINK_DOWN);
        assert(record.firstError.is(invalidation ? Err::NOT_INITIALIZED : Err::MDIO_TIMEOUT));
        assert(record.firstError.sdkError == ESP_OK && f.bus.quarantined == !invalidation);
        assert((eventOrder() == std::vector<int32_t>{ETHERNET_EVENT_START, ETHERNET_EVENT_CONNECTED, ETHERNET_EVENT_DISCONNECTED}));
        const auto frames = f.mac.frames.size();
        f.mac.registers[1] = 0x7849; f.mac.registers[0x10] = 0;
        Host::tickTimers();
        assert(f.mac.frames.size() == frames && countCall("mac.down") == 1);
        assert(f.owner->stop(Host::state().nowUs + 100000) == ESP_OK);
        assert(eventOrder().back() == ETHERNET_EVENT_STOP && !f.journal().publishedTuple.up);
    }
}

void installReservation() {
    Host::reset(); Fixture f; Fixture other(f, 1);
    f.create(); f.board(); other.create();
    assert(xSemaphoreTake(f.bus.mutex, 0) == pdTRUE);
    auto rejected = reinterpret_cast<esp_eth_handle_t>(0x1234);
    assert(f.owner->install(rejected) == ESP_ERR_INVALID_STATE && rejected == reinterpret_cast<void*>(0x1234));
    assert(f.bus.admission == BusAdmission::AVAILABLE && countCall("mac.init") == 0);
    assert(xSemaphoreGive(f.bus.mutex) == pdTRUE);
    bool seen = false;
    f.mac.onCall = [&](const char* operation) {
        if (std::string(operation) != "mac.init") return;
        seen = true;
        assert(f.bus.admission == BusAdmission::INSTALLING && !Host::semaphoreHeld(f.bus.mutex));
        uint16_t id = 0xCAFE;
        assert(readPeerIdentity(f, id) == ESP_ERR_INVALID_STATE && id == 0xCAFE);
        BoardControl control;
        control.setReset = [](void*, bool) -> esp_err_t { assert(false); return ESP_FAIL; };
        assert(other.owner->prepareBoard(control, Host::state().nowUs + 100000) == ESP_ERR_INVALID_STATE);
    };
    assert(f.owner->install(f.handle) == ESP_OK && seen && f.bus.admission == BusAdmission::AVAILABLE);
    uint16_t id = 0;
    assert(readPeerIdentity(f, id) == ESP_OK && id == 0x2000);
    assert(f.mac.frames.size() == 1 && f.mac.frames.back().address == 1 && countCall("mac.init") == 1);
    assert(other.owner->destroy() == ESP_OK && Host::isLive(f.handle));
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
        assert(expected == ESP_OK ? record.firstError.ok() : record.firstError.sdkError == expected);
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
    Host::reset(); Fixture f; f.install();
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
        assert(readPeerIdentity(f, sentinel) == ESP_ERR_INVALID_STATE);
        Host::state().currentTask = 1;
        assert(sentinel == 0xCAFE && f.mac.frames.size() == frames);
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
    assert(readPeerIdentity(f, result) == ESP_OK && result == 0x2000);
    assert(f.mac.frames.size() == 5 && f.mac.frames.back().address == 1 && countCall("mac.init") == 1);
}
void rawLockFailures() {
    Host::reset(); Fixture f;
    f.config.lockWaitUs = (1000000U + CONFIG_FREERTOS_HZ - 1U) / CONFIG_FREERTOS_HZ;
    f.config.core.transport.bounds.fixedUs = f.config.lockWaitUs + 1000U;
    f.install();
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
    constexpr uint32_t legacyTickUs = (1000U / CONFIG_FREERTOS_HZ) * 1000U;
    constexpr uint32_t tickUs = (1000000U + CONFIG_FREERTOS_HZ - 1U) / CONFIG_FREERTOS_HZ;
    for (const uint32_t waitUs : {legacyTickUs, tickUs - 1U, tickUs, tickUs * 2U}) {
        Host::reset(); Fixture f;
        f.config.lockWaitUs = waitUs;
        f.config.core.transport.bounds.fixedUs = waitUs + 1000U;
        f.install();
        Host::state().semaphoreEvents.clear();
        Host::state().failNextTake = f.bus.mutex;
        uint16_t output = 0xCAFE;
        const auto status = f.owner->readMmd(31, 0x0467, output, {Host::state().nowUs + 100000, 4});
        const auto receipts = controllerEvents(f.bus.mutex);
        const auto expectedTicks = uint64_t{waitUs} * CONFIG_FREERTOS_HZ / 1000000U;
        const bool waited = expectedTicks != 0;
        assert(status.is(waited ? Err::MDIO_TIMEOUT : Err::BUSY));
        if (waited) assert(status.detail == ESP_ERR_TIMEOUT);
        assert(receipts.size() == 1 && receipts[0].take && !receipts[0].accepted);
        assert(receipts[0].ticks == expectedTicks);
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
void p4BusyBeforeCommand() {
    for (const bool prefix : {false, true}) {
        Host::reset(); Fixture f; f.config.pinnedP4Mac = true; f.install();
        const auto completed = prefix ? 2U : 0U;
        f.mac.writeResults.assign(completed, ESP_OK);
        f.mac.writeResults.push_back(ESP_ERR_INVALID_STATE); // P4 busy before writing the command.
        uint16_t output = 0xCAFE;
        const IoBudget budget{Host::state().nowUs + 100000, 4};
        const auto failure = f.owner->readMmd(31, 0x0467, output, budget);
        assert(prefix ? failure.is(Err::MDIO_ERROR) && failure.detail == ESP_ERR_INVALID_STATE : failure.is(Err::BUSY));
        assert(output == 0xCAFE && f.mac.frames.size() == completed && !f.bus.quarantined);
        TransportSnapshot snapshot;
        assert(f.owner->transportSnapshot(snapshot).ok());
        assert(snapshot.completedFrames == completed && snapshot.attemptedFrames == completed);
        assert(snapshot.violation == TransportViolation::NONE && snapshot.lastBatch.controllerReusable);
        assert(snapshot.lastBatch.releaseStatus.ok() && !snapshot.lastBatch.frames[completed].attempted);
        f.mac.extended[0x0467] = 0xBEEF;
        assert(f.owner->readMmd(31, 0x0467, output, budget).ok() && output == 0xBEEF);
    }
    Host::reset(); Fixture f; f.config.pinnedP4Mac = true; f.install();
    f.mac.readResults = {ESP_ERR_INVALID_STATE};
    uint16_t output = 0xCAFE;
    const IoBudget budget{Host::state().nowUs + 100000, 1};
    const auto failure = f.owner->readClause22(2, output, budget);
    assert(failure.is(Err::BUSY) && output == 0xCAFE && !f.bus.quarantined);
    TransportSnapshot snapshot;
    assert(f.owner->transportSnapshot(snapshot).ok());
    assert(snapshot.attemptedFrames == 0 && snapshot.completedFrames == 0 && snapshot.violation == TransportViolation::NONE);
    assert(snapshot.lastBatch.controllerReusable && snapshot.lastBatch.releaseStatus.ok() && !snapshot.lastBatch.frames[0].attempted);
    assert(f.mac.frames.empty() && countCall("mac.read") == 1);
    assert(f.owner->readClause22(2, output, budget).ok() && output == 0x2000);
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
        assert(f.owner->stop(Host::state().nowUs + 100000) == ESP_OK);
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
        assert(late ? status.is(Err::OPERATION_TIMEOUT) && status.sdkError == ESP_OK : status.ok());
        SdkStateSnapshot record; assert(f.owner->sdkStateSnapshot(record).ok());
        assert(record.autoNegotiationAccepted && record.reconciled == !late && record.cacheValid == !late);
        bool enabled = false;
        assert(esp_eth_ioctl(f.handle, ETH_CMD_G_AUTONEGO, &enabled) == ESP_OK && enabled);
        assert(f.mac.frames.size() == frames);
        phy->autonego_ctrl = original;
        assert(f.owner->reconcileSdkState(Host::state().nowUs + 100000).ok());
        assert(f.owner->sdkStateSnapshot(record).ok() && record.reconciled && record.cacheValid);
    }
    for (const bool loseAgreement : {false, true}) {
        Host::reset(); Fixture f; f.install(); f.initialize();
        auto* phy = f.owner->nativePhy();
        original = phy->autonego_ctrl;
        static SharedBus* quarantine = nullptr;
        quarantine = loseAgreement ? &f.bus : nullptr;
        phy->autonego_ctrl = [](esp_eth_phy_t* native, eth_phy_autoneg_cmd_t command, bool* enabled) {
            if (command != ESP_ETH_PHY_AUTONEGO_EN) return original(native, command, enabled);
            if (!quarantine) return ESP_ERR_TIMEOUT;
            const auto accepted = original(native, command, enabled);
            assert(accepted == ESP_OK);
            quarantine->quarantined.store(true);
            return accepted;
        };
        const auto frames = f.mac.frames.size();
        const auto result = f.owner->reconcileSdkState(Host::state().nowUs + 100000);
        if (loseAgreement) assert(result.is(Err::NOT_INITIALIZED));
        else assert(result.sdkError == ESP_ERR_TIMEOUT && result.coreStatus.ok());
        SdkStateSnapshot record;
        assert(f.owner->sdkStateSnapshot(record).ok() && !record.reconciled && !record.cacheValid);
        assert(record.autoNegotiationAccepted == loseAgreement && f.mac.frames.size() == frames);
        assert(record.status.coreStatus.code == result.coreStatus.code && record.status.sdkError == result.sdkError);
    }
    Host::reset(); Fixture f; f.install(); f.initialize();
    Host::state().failSemaphoreTakes = f.config.maxFencePolls;
    const auto contention = f.owner->reconcileSdkState(Host::state().nowUs + 500000);
    assert(contention.is(Err::BUSY));
}
void rawMutationArbitration() {
    for (const bool modify : {false, true}) {
        Host::reset(); Fixture f; f.install(); f.initialize();
        f.mac.frames.clear();
        f.mac.registers[0x18] = 0x0080;
        Host::state().semaphoreEvents.clear();
        unsigned boundaries = 0;
        f.mac.onFrame = [&](const Host::Frame&) {
            ++boundaries;
            assert(Host::semaphoreHeld(f.bus.mutex));
            const IoBudget budget{Host::state().nowUs + 100000, 5};
            assert(f.owner->writeMmd(31, 0x18, 0x12, budget).is(Err::BUSY));
            Host::state().currentTask = 2;
            uint16_t sentinel = 0xCAFE;
            const auto frames = f.mac.frames.size();
            assert(readPeerIdentity(f, sentinel) == ESP_ERR_INVALID_STATE);
            Host::state().currentTask = 1;
            assert(sentinel == 0xCAFE && f.mac.frames.size() == frames);
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
        f.mac.onFrame = nullptr;
        uint16_t identity = 0;
        assert(readPeerIdentity(f, identity) == ESP_OK && identity == 0x2000);
        assert(f.mac.frames.back().address == 1 && countCall("mac.init") == 1);
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
    Host::reset(); Fixture f; f.install(); f.initialize();
    f.mac.frames.clear();
    OperationToken token;
    assert(f.owner->startInitialize({ResetKind::HARD}, {{Host::state().nowUs + 1000000, 512}, 10, 16}, token).inProgress());
    assert(f.bus.admission == BusAdmission::INITIALIZING && !Host::semaphoreHeld(f.bus.mutex));
    assert(f.owner->pollOperation(token, 0).inProgress() && f.mac.frames.empty());
    Status status{Err::IN_PROGRESS, 0, "Pending"};
    for (unsigned i = 0; i < 256 && status.inProgress(); ++i) {
        uint16_t sentinel = 0xCAFE;
        const auto frames = f.mac.frames.size();
        assert(readPeerIdentity(f, sentinel) == ESP_ERR_INVALID_STATE);
        assert(sentinel == 0xCAFE && f.mac.frames.size() == frames);
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
    assert(readPeerIdentity(f, id) == ESP_OK && id == 0x2000);
    assert(f.mac.frames.back().address == 1 && countCall("mac.init") == 1);
    assert(f.owner->reconcileSdkState(Host::state().nowUs + 100000).ok());
}
void boardRespectsInitializationSilence() {
    Host::reset(); Fixture f; Fixture g(f, 1);
    f.install(); f.initialize(); g.create();
    OperationToken token;
    assert(f.owner->startInitialize({ResetKind::HARD}, {{Host::state().nowUs + 1000000, 512}, 10, 16}, token).inProgress());
    unsigned pins = 0;
    BoardControl control; control.context = &pins;
    control.setReset = [](void* context, bool) { ++*static_cast<unsigned*>(context); return ESP_OK; };
    const auto frames = f.mac.frames.size(); const auto before = Host::state().nowUs;
    assert(g.owner->prepareBoard(control, before + 100000) == ESP_ERR_INVALID_STATE);
    assert(pins == 0 && Host::state().nowUs == before && f.mac.frames.size() == frames);
    assert(f.bus.admission == BusAdmission::INITIALIZING && !Host::semaphoreHeld(f.bus.mutex));
    // Rejection cannot release another owner's reset/replay reservation.
    uint16_t sentinel = 0xCAFE;
    assert(f.owner->readClause22(2, sentinel, {before + 100000, 1}).is(Err::BUSY) && sentinel == 0xCAFE);
    assert(f.finish(token).status.ok() && f.bus.admission == BusAdmission::AVAILABLE);
    const auto initializedFrames = f.mac.frames.size();
    assert(g.owner->prepareBoard(control, Host::state().nowUs + 100000) == ESP_OK);
    assert(pins == 2 && f.bus.admission == BusAdmission::AVAILABLE && f.mac.frames.size() == initializedFrames);
    assert(g.owner->destroy() == ESP_OK);
}
void boardSilenceExcludesOtherOwners() {
    Host::reset(); Fixture f; Fixture g(f, 1); Fixture h(f, 2);
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
        uint16_t identity = 0xCAFE;
        assert(readPeerIdentity(first, identity) == ESP_ERR_INVALID_STATE && identity == 0xCAFE);
        assert(context.otherPins == 0 && first.mac.frames.size() == frames && first.bus.admission == BusAdmission::BOARD_PREPARING);
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
    Host::reset(); Fixture f; Fixture g(f, 1); Fixture h(f, 2);
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
    assert(f.bus.admission == BusAdmission::FAILED_CLOSED && f.mac.frames.size() == frames);
    assert(h.owner->destroy() == ESP_OK);
}
void flowCompletionContention() {
    Host::reset(); Fixture f; f.install(); f.initialize();
    LinkSettings settings; settings.advertisement = {LINK_100_FULL, true, false};
    f.configure(settings);
    f.mac.onCall = [](const char* operation) {
        if (std::string(operation) == "mac.flow") Host::state().failSemaphoreTakes = 1;
    };
    assert(f.owner->setFlowControl(true, 1, Host::state().nowUs + 100000) == ESP_OK);
    f.mac.onCall = nullptr;
    FlowControlRecord record;
    assert(f.owner->flowControlSnapshot(record).ok());
    assert(record.macAttempted && record.macAccepted && record.macEffectPossible && record.macError == ESP_OK);
    assert(record.phyAttempted && record.phyAccepted && record.reconciled && f.mac.flowEnabled);
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
    assert(stopped == ESP_OK);
    const auto stops = std::count_if(Host::state().events.begin(), Host::state().events.end(), [](const auto& e) { return e.id == ETHERNET_EVENT_STOP; });
    assert(stops == 1 && Host::state().oneShotArms == 1);
    assert(!f.journal().publishedTuple.up);
}
void timerPublicationDuringStop() {
    Host::reset(); Fixture f; f.install(); f.initialize(LINK_TECHNOLOGY_MASK); assert(f.start() == ESP_OK);
    f.mac.registers[0x10] = 0x0017; // Timer has a new UP tuple to publish.
    bool armed = false;
    bool stopped = false;
    const auto offset = Host::state().calls.size();
    f.mac.onFrame = [&](const Host::Frame&) {
        if (armed) return;
        armed = true;
        Host::state().beforeSemaphoreTake = [&](SemaphoreHandle_t semaphore) {
            if (semaphore == f.bus.mutex) return;
            assert(!Host::semaphoreHeld(f.bus.mutex)); // Core observation released the complete batch.
            Host::state().beforeSemaphoreTake = nullptr;
            Host::state().currentTask = 2;
            const auto before = Host::state().nowUs;
            assert(f.owner->stop(before + 20000) == ESP_ERR_TIMEOUT);
            const auto elapsed = Host::state().nowUs;
            assert(elapsed <= before + 20000 && f.journal().phase == Phase::STOPPING);
            assert(Host::state().oneShotArms == 1 && f.mac.link == ETH_LINK_DOWN);
            stopped = true;
            Host::state().currentTask = 1;
        };
    };
    Host::tickTimers();
    assert(armed && stopped);
    assert((noticesSince(offset) == std::vector<std::string>{"mac.down"}));
    assert(f.owner->stop(Host::state().nowUs + 100000) == ESP_OK);
    assert(f.journal().phase == Phase::STOPPED && !f.journal().publishedTuple.up);
    assert(countCall("mac.stop") == 1 && Host::state().oneShotArms == 1);
    assert((eventOrder() == std::vector<int32_t>{ETHERNET_EVENT_START, ETHERNET_EVENT_CONNECTED,
                                                ETHERNET_EVENT_DISCONNECTED, ETHERNET_EVENT_STOP}));
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
        assert(f.owner->stop(Host::state().nowUs + 100000) == ESP_OK);
        assert(eventCount(ETHERNET_EVENT_STOP) == (cachedAn ? 0 : 1));
        assert(Host::state().oneShotArms == 1 && f.mac.frames.size() == frames);
        assert(std::count(Host::state().calls.begin(), Host::state().calls.end(), "mac.stop") == 1);
        assert(f.owner->stop(Host::state().nowUs + 100000) == ESP_OK);
        assert(eventCount(ETHERNET_EVENT_STOP) == (cachedAn ? 0 : 1));
        assert(f.mac.frames.size() == frames);
    }
    for (const bool restart : {false, true}) {
        Host::reset(); Fixture f; f.install(); f.initialize();
        if (!restart) f.configure(forcedSettings(LinkSpeed::MBPS_100));
        unsigned gateAttempts = 0;
        bool injected = false;
        Host::state().beforeSemaphoreTake = [&](SemaphoreHandle_t semaphore) {
            if (semaphore == f.bus.mutex) return;
            const bool callbackEntry = restart ? ++gateAttempts == 2 : eventCount(ETHERNET_EVENT_START) == 1;
            if (!callbackEntry) return;
            Host::state().beforeSemaphoreTake = nullptr;
            Host::state().failSemaphoreTakes = 1;
            injected = true;
        };
        assert(f.owner->start(Host::state().nowUs + 100000) == ESP_OK);
        assert(injected && eventCount(ETHERNET_EVENT_START) == 1 && eventCount(ETHERNET_EVENT_STOP) == 0);
        assert(Host::state().oneShotArms == 0 && f.journal().firstError.ok());
        assert(f.owner->stop(Host::state().nowUs + 100000) == ESP_OK);
        assert(eventCount(ETHERNET_EVENT_STOP) == 1 && Host::state().oneShotArms == 1 && countCall("mac.stop") == 1);
    }
}
}
int main(int argc, char** argv) {
    assert(argc == 2); const std::string suite = argv[1];
    if (suite == "factory") factory();
    else if (suite == "board_install_failures") boardAndInstallFailures();
    else if (suite == "raw_routing") rawRouting();
    else if (suite == "initialize_failures") initializeFailures();
    else if (suite == "configuration_failures") configurationFailures();
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
    else if (suite == "stop_failure_receipts") stopFailureReceipts();
    else if (suite == "dependent_callback_bounds") dependentCallbackBounds();
    else if (suite == "forbidden_lifecycle") forbiddenLifecycle();
    else if (suite == "discovery") discovery();
    else if (suite == "discovery_admission") discoveryAdmission();
    else if (suite == "discovery_accounting") discoveryAccounting();
    else if (suite == "terminal_event_retry") terminalEventRetry();
    else if (suite == "retire_and_stop_admission") retireAndStopAdmission();
    else if (suite == "board_completion_contention") boardCompletionContention();
    else if (suite == "late_start_cleanup") lateStartCleanup();
    else if (suite == "uninstall_gate_contention") uninstallGateContention();
    else if (suite == "stop_without_start") stopWithoutStart();
    else if (suite == "stop_repairs_lost_down") stopRepairsLostDown();
    else if (suite == "observation_loss") observationLoss();
    else if (suite == "install_reservation") installReservation();
    else if (suite == "installer_contention") installerContention();
    else if (suite == "raw_read_arbitration") rawReadArbitration();
    else if (suite == "raw_lock_failures") rawLockFailures();
    else if (suite == "controller_wait_classification") controllerWaitClassification();
    else if (suite == "raw_boundary_failures") rawBoundaryFailures();
    else if (suite == "p4_busy_before_command") p4BusyBeforeCommand();
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
    else if (suite == "timer_publication_during_stop") timerPublicationDuringStop();
    else if (suite == "foreign_flow_effect") foreignFlowEffect();
    else if (suite == "start_gate_failures") startGateFailures();
    else return 2;
    Host::reset(); std::cout << suite << " passed\n";
}
