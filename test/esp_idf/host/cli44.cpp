#include "InstrumentedMac.h"
#include "../../../examples/esp_idf/common/Dp8382xCliNative.h"
#include "../../support/Check.h"

#include <algorithm>
#include <string>
#include <string_view>

namespace {
using namespace DP8382x;
using namespace DP8382x::Cli;
using namespace DP8382x::EspIdf;

uint64_t now() { return Host::state().nowUs; }
OperationBudget allowance(uint32_t frames = 512) { return {{now() + 1000000, frames}, 10, 16}; }

struct Endpoint {
  Host::InstrumentedMac mac;
  Phy* phy{nullptr};
  esp_eth_handle_t handle{nullptr};
  bool maintenance{true};
  unsigned maintenanceChecks{0};
  std::string transcript;
  NativeOwner owner{nullptr, this, nullptr, &canMaintain};
  Platform route{owner.platform()};

  static bool canMaintain(void* context) {
    auto& endpoint = *static_cast<Endpoint*>(context);
    ++endpoint.maintenanceChecks;
    return endpoint.maintenance;
  }

  static std::size_t write(void* context, const char* bytes, std::size_t size) {
    static_cast<Endpoint*>(context)->transcript.append(bytes, size);
    return size;
  }

  bool install(SharedBus& bus, uint32_t address) {
    PhyConfig config{};
    config.mac = &mac.native;
    config.bus = &bus;
    config.core.variant = Variant::DP83826I;
    config.core.interface = MacInterface::RMII;
    config.core.phyAddress = address;
    config.core.transport.bounds = {2000, 1000, 1};
    config.notificationUs = 10;
    config.maxFencePolls = 10;
    mac.extended[0x030E] = 0x8400;
    mac.registers[0x19] = 0x8001;
    mac.registers[0x1C] = 0x0100;
    if (Phy::create(config, phy) != ESP_OK) return false;
    BoardControl board{};
    board.setReset = [](void*, bool) { return ESP_OK; };
    if (phy->prepareBoard(board, now() + 100000) != ESP_OK) return false;
    if (phy->install(handle) != ESP_OK) return false;
    owner.phy = phy;
    return true;
  }

  Status call(Request request, Receipt& receipt) {
    return route.execute(route.context, request, receipt);
  }

  bool terminal(OperationToken token, Receipt& receipt) {
    Request step{};
    step.action = Action::POLL;
    step.token = token;
    step.budget = allowance();
    for (unsigned i = 0; i < 256; ++i) {
      OperationSnapshot progress{};
      if (!phy->operationSnapshot(progress).ok()) return false;
      if (progress.nextPollValid) Host::state().nowUs = std::max(now(), progress.nextPollAtUs);
      const Status status = call(step, receipt);
      if (status.inProgress()) continue;
      step.action = Action::TAKE;
      return call(step, receipt).ok() && receipt.result.code == status.code;
    }
    return false;
  }

  bool initialize() {
    OperationToken token{};
    Receipt receipt{};
    return phy->startInitialize({ResetKind::NONE}, allowance(), token).inProgress() &&
        terminal(token, receipt) && receipt.result.ok();
  }
};

Request request(Action action, uint32_t frames = 512) {
  Request value{};
  value.action = action;
  value.budget = allowance(frames);
  return value;
}

bool install(Endpoint& endpoint, SharedBus& bus) {
  bus.mutex = xSemaphoreCreateMutex();
  return bus.mutex && endpoint.install(bus, 0);
}

bool attach(Endpoint& endpoint, Processor& cli) {
  endpoint.owner.write = Endpoint::write;
  Identity identity{};
  identity.boot = 45;
  identity.variant = Variant::DP83826I;
  return cli.setup(endpoint.owner.platform(), identity);
}

bool drain(Processor& cli) {
  for (unsigned i = 0; i < 128 && (cli.workPending() || cli.outputPending()); ++i) {
    Host::state().nowUs += 1000;
    cli.tick();
  }
  return !cli.workPending() && !cli.outputPending();
}

bool command(Processor& cli, std::string_view line) {
  return cli.feed(line) == line.size() && drain(cli);
}

void admission() {
  Host::reset();
  SharedBus bus;
  Endpoint endpoint;
  if (!install(endpoint, bus)) { CHECK(false); return; }
  const auto before = now();
  CachedSnapshot cached{};
  CHECK(endpoint.route.readCached(endpoint.route.context, SnapshotKind::IDENTITY, cached).ok());
  CHECK(!cached.identity.decodedValid);
  CHECK(endpoint.route.readCached(endpoint.route.context, SnapshotKind::CAPABILITIES, cached).ok());
  CHECK(!cached.capabilities.valid && endpoint.mac.frames.empty() && now() == before);

  endpoint.maintenance = false;
  Receipt receipt{};
  for (const auto action : {Action::START_CONFIGURE, Action::INVALIDATE, Action::WRITE_C22,
                            Action::WRITE_MMD, Action::MODIFY_MMD, Action::MODIFY_BMCR,
                            Action::RECONCILE}) {
    CHECK(endpoint.call(request(action), receipt).is(Err::BUSY));
    CHECK(!receipt.batchValid && receipt.token.value == 0);
  }
  CHECK(endpoint.maintenanceChecks == 7 && endpoint.mac.frames.empty());
  endpoint.owner.maintenanceAllowed = nullptr;
  CHECK(endpoint.call(request(Action::WRITE_C22), receipt).is(Err::BUSY));
  CHECK(endpoint.maintenanceChecks == 7 && endpoint.mac.frames.empty());

  CHECK(endpoint.call(request(Action::PROBE, 2), receipt).ok());
  CHECK(receipt.batchValid && receipt.identity.id1 == 0x2000 && receipt.identity.id2 == 0xA131);
  CHECK(endpoint.mac.frames.size() == 2 && endpoint.mac.frames[0].reg == 2 &&
        endpoint.mac.frames[1].reg == 3);
  CHECK(endpoint.route.readCached(endpoint.route.context, SnapshotKind::CAPABILITIES, cached).ok());
  CHECK(cached.capabilities.valid && endpoint.mac.frames.size() == 2);
  endpoint.mac.registers[3] = 0xA130; // Complete but rejected revision.
  CHECK(endpoint.call(request(Action::PROBE, 2), receipt).is(Err::DEVICE_MISMATCH));
  CHECK(receipt.batchValid && receipt.identity.id1Valid && receipt.identity.id2Valid &&
        receipt.identity.id2 == 0xA130 && receipt.identity.revision == 0 &&
        receipt.identity.decodedValid && receipt.identity.acceptance == IdentityAcceptance::REJECTED);
  CHECK(endpoint.route.readCached(endpoint.route.context, SnapshotKind::IDENTITY, cached).ok());
  CHECK(cached.identity.acceptance == IdentityAcceptance::REJECTED && endpoint.mac.frames.size() == 4);
}

void reconciliation() {
  Host::reset();
  SharedBus bus;
  Endpoint endpoint;
  if (!install(endpoint, bus) || !endpoint.initialize()) { CHECK(false); return; }
  Receipt receipt{};
  Request apply = request(Action::START_CONFIGURE);
  apply.settings.advertisement.technologyMask = LINK_100_FULL;
  Processor cli;
  endpoint.owner.write = Endpoint::write;
  Identity identity{};
  identity.boot = 44;
  identity.variant = Variant::DP83826I;
  CHECK(cli.setup(endpoint.owner.platform(), identity));
  constexpr std::string_view command = "configuration apply an 8 none auto 1\n";
  CHECK(cli.feed(command) == command.size());
  for (unsigned i = 0; i < 128 && (cli.workPending() || cli.outputPending()); ++i) {
    Host::state().nowUs += 1000;
    cli.tick();
  }
  CHECK(!cli.workPending() && !cli.outputPending());
  CHECK(endpoint.transcript.find("cmd=configuration.apply op=2 state=admitted primary=IN_PROGRESS") != std::string::npos);
  CHECK(endpoint.transcript.find("cmd=configuration.apply op=2 state=terminal primary=OK") != std::string::npos);
  CHECK(endpoint.transcript.find("core_status=OK core_detail=0 sdk_error=0") != std::string::npos);
  bool an = false;
  CHECK(esp_eth_ioctl(endpoint.handle, ETH_CMD_G_AUTONEGO, &an) == ESP_OK && an);
  CHECK(endpoint.phy->start(now() + 100000) == ESP_OK);
  const auto runningFrames = endpoint.mac.frames.size();
  endpoint.maintenance = false;
  CHECK(endpoint.call(apply, receipt).is(Err::BUSY));
  CHECK(endpoint.call(request(Action::START_VERIFY), receipt).is(Err::BUSY));
  CHECK(endpoint.mac.frames.size() == runningFrames);
  CHECK(endpoint.phy->stop(now() + 100000) == ESP_OK);
  endpoint.maintenance = true;

  apply = request(Action::START_CONFIGURE);
  apply.settings.mode = LinkMode::FORCED;
  apply.settings.forced = {LinkSpeed::MBPS_10, Duplex::HALF};
  apply.settings.crossover.pairSelection = PairSelection::MDI;
  CHECK(endpoint.call(apply, receipt).inProgress());
  if (!endpoint.terminal(receipt.token, receipt) || !receipt.result.ok()) { CHECK(false); return; }
  CHECK((endpoint.mac.registers[0] & 0x3100U) == 0);
  CHECK(esp_eth_ioctl(endpoint.handle, ETH_CMD_G_AUTONEGO, &an) == ESP_OK && an);
  const auto unreconciledFrames = endpoint.mac.frames.size();
  CHECK(endpoint.phy->start(now() + 100000) == ESP_ERR_INVALID_STATE);
  CHECK(endpoint.mac.frames.size() == unreconciledFrames);
  CHECK(endpoint.call(request(Action::RECONCILE), receipt).ok() && receipt.sdkError == ESP_OK);
  CHECK(esp_eth_ioctl(endpoint.handle, ETH_CMD_G_AUTONEGO, &an) == ESP_OK && !an);
  eth_speed_t speed = ETH_SPEED_10M;
  eth_duplex_t duplex = ETH_DUPLEX_HALF;
  CHECK(esp_eth_ioctl(endpoint.handle, ETH_CMD_G_SPEED, &speed) == ESP_OK && speed == ETH_SPEED_100M);
  CHECK(esp_eth_ioctl(endpoint.handle, ETH_CMD_G_DUPLEX_MODE, &duplex) == ESP_OK && duplex == ETH_DUPLEX_FULL);
  CHECK(endpoint.mac.frames.size() == unreconciledFrames);

  const auto verifyAt = endpoint.mac.frames.size();
  CHECK(endpoint.call(request(Action::START_VERIFY), receipt).inProgress());
  if (!endpoint.terminal(receipt.token, receipt) || !receipt.result.ok()) { CHECK(false); return; }
  CHECK(!receipt.mutationCompleted && !receipt.mutationPossible && !receipt.cleanupOwed);
  CHECK(endpoint.mac.frames.size() > verifyAt);
  for (auto i = verifyAt; i < endpoint.mac.frames.size(); ++i) {
    const auto& frame = endpoint.mac.frames[i];
    CHECK(!frame.write || frame.reg == 13 || frame.reg == 14);
  }
  CHECK(endpoint.phy->start(now() + 100000) == ESP_OK);
  CHECK(esp_eth_ioctl(endpoint.handle, ETH_CMD_G_SPEED, &speed) == ESP_OK && speed == ETH_SPEED_10M);
  CHECK(esp_eth_ioctl(endpoint.handle, ETH_CMD_G_DUPLEX_MODE, &duplex) == ESP_OK && duplex == ETH_DUPLEX_HALF);
  CHECK(endpoint.phy->stop(now() + 100000) == ESP_OK);

  Request raw = request(Action::MODIFY_BMCR, 2);
  raw.mask = 0x1000;
  raw.value = 0x1000;
  CHECK(endpoint.call(raw, receipt).ok() && receipt.batchValid && receipt.batch.frames[1].completed);
  CHECK((endpoint.mac.registers[0] & 0x3100U) == 0x1000U);
  CHECK(esp_eth_ioctl(endpoint.handle, ETH_CMD_G_AUTONEGO, &an) == ESP_OK && !an);
  const auto rawFrames = endpoint.mac.frames.size();
  CHECK(endpoint.call(request(Action::RECONCILE), receipt).is(Err::NOT_INITIALIZED) && receipt.sdkError == ESP_OK);
  CHECK(endpoint.phy->start(now() + 100000) == ESP_ERR_INVALID_STATE);
  CHECK(endpoint.mac.frames.size() == rawFrames);
  if (!endpoint.initialize()) { CHECK(false); return; }
  CHECK((endpoint.mac.registers[0] & 0x3100U) == 0);
  CHECK(endpoint.phy->start(now() + 100000) == ESP_ERR_INVALID_STATE);

  auto* native = endpoint.phy->nativePhy();
  const auto originalAn = native->autonego_ctrl;
  native->autonego_ctrl = [](esp_eth_phy_t*, eth_phy_autoneg_cmd_t, bool*) { return ESP_ERR_TIMEOUT; };
  CHECK(endpoint.call(request(Action::RECONCILE), receipt).ok() && receipt.sdkError == ESP_ERR_TIMEOUT);
  native->autonego_ctrl = originalAn;
  CHECK(endpoint.phy->start(now() + 100000) == ESP_ERR_INVALID_STATE);
  CHECK(endpoint.call(request(Action::RECONCILE), receipt).ok() && receipt.sdkError == ESP_OK);
  CHECK(endpoint.phy->start(now() + 100000) == ESP_OK);
  CHECK(endpoint.phy->stop(now() + 100000) == ESP_OK);
}

void restartAfterRawObservation() {
  for (const bool forced : {false, true}) for (const bool resetObserved : {false, true}) {
    Host::reset();
    SharedBus bus;
    Endpoint endpoint;
    if (!install(endpoint, bus) || !endpoint.initialize()) { CHECK(false); return; }
    auto apply = request(Action::START_CONFIGURE);
    if (forced) {
      apply.settings.mode = LinkMode::FORCED;
      apply.settings.forced = {LinkSpeed::MBPS_10, Duplex::FULL};
      apply.settings.crossover.pairSelection = PairSelection::MDI;
    } else {
      apply.settings.advertisement.technologyMask = LINK_100_FULL;
    }
    Receipt receipt{};
    if (!endpoint.call(apply, receipt).inProgress() || !endpoint.terminal(receipt.token, receipt) ||
        !receipt.result.ok() || !endpoint.phy->reconcileSdkState(now() + 100000).ok()) { CHECK(false); return; }
    CHECK(endpoint.phy->start(now() + 100000) == ESP_OK);
    CHECK(endpoint.phy->stop(now() + 100000) == ESP_OK);
    if (resetObserved) {
      // Table 8-48, PHYRCR HARD (0x1F bit 15), read via its DEVAD 31 alias.
      endpoint.mac.registers[0x1F] = 0x8000;
      auto raw = request(Action::READ_MMD, 4);
      raw.devad = 31;
      raw.address = 0x1F;
      CHECK(endpoint.call(raw, receipt).ok() && receipt.value == 0x8000);
      endpoint.mac.registers[0x1F] = 0; // Model later self-clear.
      CHECK(endpoint.call(raw, receipt).ok() && receipt.value == 0);
    } else {
      auto raw = request(Action::MODIFY_BMCR, 2);
      raw.mask = 0x1000;
      raw.value = forced ? 0x1000 : 0; // Table 8-19, change the actual AN bit.
      CHECK(endpoint.call(raw, receipt).ok() && receipt.batch.frames[1].completed);
      raw = request(Action::READ_C22, 1); // A successful read cannot reauthorize restart.
      raw.address = 0;
      CHECK(endpoint.call(raw, receipt).ok() && (receipt.value & 0x1000U) == (forced ? 0x1000U : 0U));
    }
    const auto before = endpoint.mac.frames.size();
    SdkStateSnapshot sdk{};
    SettingsSnapshot settings{};
    bool cachedAn = forced;
    CHECK(endpoint.phy->settingsSnapshot(settings).ok() && settings.trust == ConfigurationTrust::DIRTY &&
          settings.desiredValid && settings.desired.mode == apply.settings.mode && settings.observedStale);
    CHECK(endpoint.phy->sdkStateSnapshot(sdk).ok() && !sdk.reconciled && sdk.cacheValid &&
          sdk.cachedAutoNegotiation == !forced);
    CHECK(esp_eth_ioctl(endpoint.handle, ETH_CMD_G_AUTONEGO, &cachedAn) == ESP_OK && cachedAn == !forced);
    CHECK(endpoint.phy->start(now() + 100000) == ESP_ERR_INVALID_STATE);
    CHECK(endpoint.phy->reconcileSdkState(now() + 100000).is(Err::NOT_INITIALIZED));

    // esp_eth_start changes the SDK FSM first. With AN off it posts START
    // before the installed PHY rejects get_link; with AN on the restart
    // callback rejects before the event. Only the owner can finish cleanup.
    const auto events = Host::state().events.size();
    CHECK(esp_eth_start(endpoint.handle) == ESP_ERR_INVALID_STATE);
    CHECK(Host::state().events.size() == events + (forced ? 1U : 0U));
    if (forced && Host::state().events.size() > events)
      CHECK(Host::state().events.back().id == ETHERNET_EVENT_START &&
            Host::state().events.back().handle == endpoint.handle);
    CHECK(esp_eth_start(endpoint.handle) == ESP_ERR_INVALID_STATE);
    CHECK(endpoint.phy->stop(now() + 100000) == ESP_OK);
    CHECK(Host::state().events.size() == events + (forced ? 2U : 0U));
    if (forced && Host::state().events.size() > events)
      CHECK(Host::state().events.back().id == ETHERNET_EVENT_STOP &&
            Host::state().events.back().handle == endpoint.handle);
    CHECK(endpoint.phy->start(now() + 100000) == ESP_ERR_INVALID_STATE && endpoint.mac.frames.size() == before);
    if (!endpoint.initialize()) { CHECK(false); return; }
    CHECK(endpoint.phy->reconcileSdkState(now() + 100000).ok());
    CHECK(endpoint.phy->sdkStateSnapshot(sdk).ok() && sdk.reconciled && sdk.cachedAutoNegotiation == !forced);
    CHECK(esp_eth_ioctl(endpoint.handle, ETH_CMD_G_AUTONEGO, &cachedAn) == ESP_OK && cachedAn == !forced);
    CHECK(((endpoint.mac.registers[0] & 0x1000U) != 0) == !forced);
    CHECK(endpoint.phy->start(now() + 100000) == ESP_OK && endpoint.phy->stop(now() + 100000) == ESP_OK);
  }
}

void clockFault() {
  Host::reset();
  SharedBus bus;
  Endpoint endpoint;
  if (!install(endpoint, bus) || !endpoint.initialize()) { CHECK(false); return; }
  Processor cli;
  endpoint.owner.write = Endpoint::write;
  Identity identity{};
  identity.boot = 45;
  identity.variant = Variant::DP83826I;
  CHECK(cli.setup(endpoint.owner.platform(), identity));
  constexpr std::string_view command = "configuration apply an 8 none auto 1\n";
  CHECK(cli.feed(command) == command.size());
  cli.tick(); // The core accepts the token without taking a frame.
  const auto frames = endpoint.mac.frames.size();
  Host::state().nowUs += 10000;
  CHECK(cli.feed("session later\n") == 14);
  --Host::state().nowUs; // Still beyond the core's last sample.
  endpoint.maintenance = false; // Delivery must not require a new maintenance grant.
  for (unsigned i = 0; i < 16 && (cli.workPending() || cli.outputPending()); ++i) cli.tick();
  CHECK(!cli.workPending() && !cli.outputPending());
  CHECK(endpoint.mac.frames.size() == frames && endpoint.maintenanceChecks == 1);
  CHECK(endpoint.transcript.find("cmd=configuration.apply op=2 state=terminal primary=INVALID_CONFIG detail=5") !=
        std::string::npos);
  CHECK(endpoint.transcript.find("time_valid=0 time_us=0") != std::string::npos);
  CHECK(endpoint.transcript.find("core_status=CANCELLED core_detail=0 sdk_error=0 mutation_completed=0") !=
        std::string::npos);
  OperationSnapshot operation{};
  CHECK(endpoint.phy->operationSnapshot(operation).ok() && operation.state == OperationState::IDLE);
}

void discovery() {
  Host::reset();
  SharedBus bus;
  Endpoint endpoint;
  if (!install(endpoint, bus)) { CHECK(false); return; }
  Receipt receipt{};
  Request scan = request(Action::DISCOVER, 2);
  scan.address = 0;
  CHECK(endpoint.call(scan, receipt).ok());
  CHECK(receipt.batchValid && receipt.identity.id1Valid && receipt.identity.id2Valid);
  CHECK(receipt.identity.id1 == 0x2000 && receipt.identity.id2 == 0xA131);
  CHECK(endpoint.mac.frames.size() == 2);

  scan.budget.io.maxFrames = 1;
  CHECK(endpoint.call(scan, receipt).is(Err::OPERATION_TIMEOUT));
  CHECK(!receipt.batchValid && !receipt.identity.id1Valid && !receipt.identity.id2Valid);
  DiscoverySnapshot historical{};
  CHECK(endpoint.phy->discoverySnapshot(historical).ok() && historical.entries[0].id2Valid);
  CHECK(endpoint.mac.frames.size() == 2);

  scan.budget.io.maxFrames = 2;
  scan.address = 31;
  endpoint.mac.readResults = {ESP_OK, ESP_ERR_TIMEOUT};
  const auto error = endpoint.call(scan, receipt);
  CHECK(error.is(Err::MDIO_TIMEOUT) && error.detail == ESP_ERR_TIMEOUT);
  CHECK(receipt.batchValid && receipt.identity.id1Valid && !receipt.identity.id2Valid);
  CHECK(receipt.batch.frames[0].completed && receipt.batch.frames[1].attempted &&
        !receipt.batch.frames[1].completed && bus.quarantined);
  CachedSnapshot cached{};
  CHECK(endpoint.route.readCached(endpoint.route.context, SnapshotKind::TRANSPORT, cached).ok());
  CHECK(cached.transport.totalBatches == 2 && cached.transport.attemptedFrames == 4 &&
        cached.transport.completedFrames == 3);
  CHECK(endpoint.mac.frames.size() == 3 && endpoint.mac.frames.back().address == 31);
  CHECK(endpoint.route.readCached(endpoint.route.context, SnapshotKind::IDENTITY, cached).ok());
  CHECK(!cached.identity.id1Valid && !cached.identity.id2Valid);

  Host::reset();
  SharedBus invalidBus;
  Endpoint invalid;
  if (!install(invalid, invalidBus)) { CHECK(false); return; }
  // Return completed words while forcing contradictory controller-state evidence.
  invalid.mac.onFrame = [&](const Host::Frame& frame) {
    if (frame.reg == 3) invalidBus.quarantined.store(true);
  };
  for (const auto action : {Action::DISCOVER, Action::PROBE}) {
    invalidBus.quarantined.store(false);
    CHECK(invalid.call(request(action, 2), receipt).is(Err::MDIO_ERROR));
    CHECK(receipt.batchValid && receipt.violation == TransportViolation::CONTROLLER_STATE);
    CHECK(!receipt.identity.id1Valid && !receipt.identity.id2Valid);
    CHECK(receipt.batch.frames[0].valueValid && receipt.batch.frames[1].valueValid);
  }
}

void rawAccess() {
  Host::reset();
  SharedBus bus;
  Endpoint endpoint;
  if (!install(endpoint, bus)) { CHECK(false); return; }
  Receipt receipt{};
  Request raw = request(Action::READ_C22, 1);
  raw.address = 0x1F;
  CHECK(endpoint.call(raw, receipt).ok() && receipt.batchValid);
  CHECK(endpoint.mac.frames.size() == 1 && endpoint.mac.frames.back().reg == 0x1F);
  for (const uint32_t forbidden : {0x0DU, 0x0EU, 0x12U, 0x8000U}) {
    raw.address = forbidden;
    CHECK(endpoint.call(raw, receipt).is(Err::INVALID_PARAM) && !receipt.batchValid);
  }
  CHECK(endpoint.mac.frames.size() == 1);

  raw = request(Action::WRITE_C22, 1);
  raw.address = 0x18;
  raw.value = 0x0080;
  CHECK(endpoint.call(raw, receipt).ok() && receipt.batchValid && receipt.batch.frames[0].completed);

  raw = request(Action::READ_MMD, 4);
  raw.address = 0x0018;
  raw.devad = 31;
  const auto beforeRead = endpoint.mac.frames.size();
  CHECK(endpoint.call(raw, receipt).ok() && receipt.batchValid && receipt.value == 0x0080);
  CHECK(endpoint.mac.frames.size() == beforeRead + 4);
  if (endpoint.mac.frames.size() != beforeRead + 4) return;
  const uint32_t registers[] = {13, 14, 13, 14};
  const uint32_t values[] = {0x001F, 0x0018, 0x401F, 0x0080};
  for (unsigned i = 0; i < 4; ++i) {
    const auto& frame = endpoint.mac.frames[beforeRead + i];
    CHECK(frame.reg == registers[i] && frame.value == values[i] && frame.write == (i < 3));
  }

  raw = request(Action::WRITE_MMD, 4);
  raw.address = 0x0018;
  raw.devad = 31;
  raw.value = 0x0010;
  CHECK(endpoint.call(raw, receipt).ok() && receipt.batchValid && receipt.batch.frames[3].completed);
  CHECK(endpoint.mac.registers[0x18] == 0x0010);
  raw.action = Action::MODIFY_MMD;
  raw.mask = 0x0080;
  raw.value = 0x0080;
  raw.budget.io.maxFrames = 5;
  CHECK(endpoint.call(raw, receipt).ok() && receipt.batchValid && receipt.batch.frames[4].completed);
  CHECK(endpoint.mac.registers[0x18] == 0x0090);

  raw.action = Action::WRITE_MMD;
  endpoint.mac.writeResults = {ESP_OK, ESP_OK, ESP_OK, ESP_ERR_TIMEOUT};
  const auto failed = endpoint.call(raw, receipt);
  CHECK(failed.is(Err::MDIO_TIMEOUT) && failed.detail == ESP_ERR_TIMEOUT);
  CHECK(receipt.batchValid && receipt.batch.frames[3].attempted &&
        !receipt.batch.frames[3].completed && receipt.batch.frames[3].effectPossible && bus.quarantined);
  CHECK(receipt.batch.releaseStatus.is(Err::MDIO_ERROR) &&
        receipt.batch.releaseStatus.detail == ESP_ERR_TIMEOUT);
  CHECK(receipt.releaseStatus.is(Err::MDIO_ERROR) && receipt.releaseStatus.detail == ESP_ERR_TIMEOUT);
  const auto explicitInvalidation = endpoint.mac.frames.size();
  CHECK(endpoint.call(request(Action::INVALIDATE), receipt).ok());
  CHECK(!receipt.batchValid && endpoint.mac.frames.size() == explicitInvalidation);
}

void checkRawHooks(Endpoint& endpoint, uint32_t address, uint32_t otherAddress) {
  uint32_t reported = 0xCAFE;
  CHECK(esp_eth_ioctl(endpoint.handle, ETH_CMD_S_PHY_ADDR, &address) == ESP_OK);
  CHECK(esp_eth_ioctl(endpoint.handle, ETH_CMD_S_PHY_ADDR, &otherAddress) == ESP_ERR_NOT_SUPPORTED);
  CHECK(esp_eth_ioctl(endpoint.handle, ETH_CMD_G_PHY_ADDR, &reported) == ESP_OK && reported == address);

  // Include an ordinary word, both indirect ports, and a read-clear counter.
  // The installed hooks reject even an otherwise safe word at the correct PHY.
  auto* mediator = endpoint.mac.mediator;
  for (const uint32_t reg : {2U, 0x0DU, 0x0EU, 0x14U}) {
    uint32_t word = 0xCAFE;
    esp_eth_phy_reg_rw_data_t raw{reg, &word};
    CHECK(esp_eth_ioctl(endpoint.handle, ETH_CMD_READ_PHY_REG, &raw) == ESP_ERR_NOT_SUPPORTED);
    CHECK(esp_eth_ioctl(endpoint.handle, ETH_CMD_WRITE_PHY_REG, &raw) == ESP_ERR_NOT_SUPPORTED);
    for (const auto target : {address, otherAddress}) {
      CHECK(mediator->phy_reg_read(mediator, target, reg, &word) == ESP_ERR_NOT_SUPPORTED);
      CHECK(mediator->phy_reg_write(mediator, target, reg, word) == ESP_ERR_NOT_SUPPORTED);
    }
    CHECK(word == 0xCAFE);
  }
}

void attemptReentry(Endpoint& holder, Endpoint& peer, Processor& cli) {
  const auto before = now();
  checkRawHooks(holder, 0, 1);
  checkRawHooks(peer, 1, 0);
  auto* base = holder.phy->nativePhy();
  bool cachedAn = false;
  CHECK(base->reset(base) == ESP_ERR_NOT_SUPPORTED);
  CHECK(base->set_speed(base, ETH_SPEED_10M) == ESP_ERR_INVALID_STATE);
  CHECK(base->set_duplex(base, ETH_DUPLEX_HALF) == ESP_ERR_INVALID_STATE);
  CHECK(esp_eth_ioctl(holder.handle, ETH_CMD_S_AUTONEGO, &cachedAn) == ESP_ERR_INVALID_STATE);
  CHECK(esp_eth_ioctl(holder.handle, ETH_CMD_G_AUTONEGO, &cachedAn) == ESP_OK && cachedAn);

  uint16_t word = 0xCAFE;
  const auto io = allowance().io;
  const auto denied = holder.phy->readClause22(2, word, io);
  CHECK(denied.is(Err::BUSY) && denied.detail == static_cast<int32_t>(BusyDetail::OWNER_ADMISSION));
  CHECK(word == 0xCAFE);
  holder.transcript.clear();
  constexpr std::string_view read = "registers read c22 2\n";
  CHECK(cli.feed(read) == read.size());
  for (unsigned i = 0; i < 16 && (cli.workPending() || cli.outputPending()); ++i) cli.tick();
  CHECK(!cli.workPending() && !cli.outputPending());
  CHECK(holder.transcript.find("cmd=registers.read op=0 state=terminal primary=BUSY detail=" +
        std::to_string(static_cast<int32_t>(BusyDetail::OWNER_ADMISSION)) + " ") != std::string::npos);

  RawBatchReceipt evidence{};
  const auto contended = peer.phy->readMmd(31, 2, word, io, &evidence);
  // The installed peer has its own instance gate and a one-tick controller allowance.
  CHECK(contended.is(Err::MDIO_TIMEOUT) && contended.detail == ESP_ERR_TIMEOUT && word == 0xCAFE);
  CHECK(evidence.valid && evidence.batch.controllerReusable && evidence.releaseStatus.ok());
  CHECK(!evidence.batch.frames[0].attempted && !evidence.batch.frames[0].effectPossible);

  // A sub-tick allowance admits normal flow callbacks but forbids sleeping
  // for the busy owner. Exercise the SDK's two typed callback boundaries
  // separately because its raw flow ioctl aliases bool and uint32_t pointers.
  const auto effects = holder.mac.flowRequests.size();
  CHECK(holder.phy->setFlowControl(true, 0, now() + 500) == ESP_ERR_INVALID_STATE);
  CHECK(holder.mac.flowRequests.size() == effects);
  CHECK(holder.mac.native.enable_flow_ctrl(&holder.mac.native, true) == ESP_OK);
  CHECK(base->advertise_pause_ability(base, 0) == ESP_ERR_INVALID_STATE);
  CHECK(holder.mac.flowRequests.size() == effects + 1 && holder.mac.flowEnabled);
  // These callbacks can issue MDIO during an admitted SDK start. Rejection
  // must preserve possible earlier framework effects for the owner's stop.
  bool enabled = false;
  CHECK(base->autonego_ctrl(base, ESP_ETH_PHY_AUTONEGO_RESTART, &enabled) == ESP_ERR_INVALID_STATE && !enabled);
  CHECK(base->get_link(base) == ESP_ERR_INVALID_STATE);
  CHECK(now() == before);
}

void crossEntryBoundaries() {
  for (const auto action : {Action::READ_MMD, Action::WRITE_MMD, Action::MODIFY_MMD}) {
    const unsigned count = action == Action::MODIFY_MMD ? 5 : 4;
    for (unsigned cut = 1; cut <= count; ++cut) {
      Host::reset();
      SharedBus bus;
      Endpoint holder, peer;
      Processor cli;
      if (!install(holder, bus) || !peer.install(bus, 1) || !attach(holder, cli)) { CHECK(false); return; }
      // Establish a valid stopped flow-control policy before contention. An
      // unauthorized callback must invalidate a previously reconciled receipt.
      auto apply = request(Action::START_CONFIGURE);
      apply.settings.advertisement = {LINK_100_FULL, false, false};
      Receipt receipt{};
      if (!holder.initialize() || !holder.call(apply, receipt).inProgress() ||
          !holder.terminal(receipt.token, receipt) || !receipt.result.ok() ||
          !holder.phy->reconcileSdkState(now() + 100000).ok()) { CHECK(false); return; }
      FlowControlRecord flow{};
      CHECK(holder.phy->setFlowControl(false, 0, now() + 500) == ESP_OK);
      CHECK(holder.phy->flowControlSnapshot(flow).ok() && flow.reconciled && !holder.mac.flowEnabled);
      TransportSnapshot before{};
      CHECK(holder.phy->transportSnapshot(before).ok());
      holder.mac.frames.clear();
      holder.mac.registers[0x18] = 0x0080;
      holder.mac.registers[0x14] = peer.mac.registers[0x14] = 0x0042;
      Host::state().semaphoreEvents.clear();
      unsigned position = 0;
      holder.mac.onFrame = [&](const Host::Frame&) {
        if (++position != cut) return;
        CHECK(Host::semaphoreHeld(bus.mutex) && holder.mac.frames.size() == cut);
        for (const auto task : {1U, 2U}) {
          Host::state().currentTask = task;
          attemptReentry(holder, peer, cli);
          CHECK(Host::semaphoreHeld(bus.mutex) && holder.mac.frames.size() == cut && peer.mac.frames.empty());
        }
        Host::state().currentTask = 1;
      };
      auto outer = request(action, count);
      outer.devad = 31;
      outer.address = 0x18;
      outer.mask = outer.value = 0x0012;
      CHECK(holder.call(outer, receipt).ok() && receipt.batchValid && receipt.releaseStatus.ok());
      holder.mac.onFrame = nullptr;
      CHECK(position == count && holder.mac.frames.size() == count && peer.mac.frames.empty());
      if (holder.mac.frames.size() != count) return;
      // SNLS647I section 8.3.11.1 and tables 8-31/32. The last write of
      // the modify preserves the sampled 0x0080 outside mask 0x0012.
      const uint32_t registers[] = {0x0D, 0x0E, 0x0D, 0x0E, 0x0E};
      const uint32_t values[] = {0x001F, 0x0018, 0x401F, action == Action::WRITE_MMD ? 0x0012U : 0x0080U, 0x0092};
      for (unsigned i = 0; i < count; ++i) {
        const auto& frame = holder.mac.frames[i];
        CHECK(frame.address == 0 && frame.reg == registers[i] && frame.value == values[i]);
        CHECK(frame.write == (i != 3 || action == Action::WRITE_MMD));
        CHECK(receipt.batch.frames[i].attempted && receipt.batch.frames[i].completed);
      }
      CHECK(holder.mac.registers[0x18] == values[count - 1]);
      if (action == Action::READ_MMD) CHECK(receipt.value == 0x0080);
      CHECK(holder.mac.registers[0x14] == 0x0042 && peer.mac.registers[0x14] == 0x0042);

      std::vector<Host::SemaphoreEvent> controller;
      for (const auto& event : Host::state().semaphoreEvents)
        if (event.handle == bus.mutex) controller.push_back(event);
      CHECK(controller.size() == 4);
      if (controller.size() != 4) return;
      CHECK(controller[0].take && controller[0].accepted);
      for (unsigned i : {1U, 2U}) CHECK(controller[i].take && !controller[i].accepted && controller[i].ticks == 1);
      CHECK(!controller[3].take && controller[3].accepted && !Host::semaphoreHeld(bus.mutex) && !bus.quarantined);
      TransportSnapshot transport{};
      CHECK(holder.phy->transportSnapshot(transport).ok() && transport.totalBatches == before.totalBatches + 1 &&
            transport.attemptedFrames == before.attemptedFrames + count &&
            transport.completedFrames == before.completedFrames + count);
      CHECK(peer.phy->transportSnapshot(transport).ok() && transport.totalBatches == 2 &&
            transport.attemptedFrames == 0 && transport.completedFrames == 0);
      // Preserve both accepted halves of the last owned call. An unadmitted
      // pause callback can only report uncertainty about the later MAC change.
      CHECK(holder.phy->flowControlSnapshot(flow).ok() && flow.macEffectPossible && !flow.reconciled &&
            flow.macAccepted && flow.phyAccepted && flow.phyError == ESP_ERR_INVALID_STATE);
      LifecycleRecord lifecycle{};
      CHECK(holder.phy->lifecycleSnapshot(lifecycle).ok() && lifecycle.phase == Phase::STOPPING &&
            lifecycle.stage == LifecycleStage::START && lifecycle.firstError.sdkError == ESP_ERR_INVALID_STATE);
      uint16_t peerWord = 0xCAFE;
      CHECK(peer.phy->readClause22(2, peerWord, allowance().io).ok() && peerWord == 0x2000);
      CHECK(peer.mac.frames.size() == 1 && peer.mac.frames[0].address == 1 && peer.mac.frames[0].reg == 2);
    }
  }
}

bool issueHard(Endpoint& endpoint, OperationToken token, bool regress) {
  bool issued = false;
  endpoint.mac.onFrame = [&](const Host::Frame& frame) {
    if (frame.write && frame.reg == 0x1F && (frame.value & 0x8000U)) {
      issued = true;
      if (regress) Host::state().nowUs = 1;
    }
  };
  auto poll = request(Action::POLL, MAX_BATCH_FRAMES);
  poll.token = token;
  Receipt receipt{};
  for (unsigned i = 0; i < 16 && !issued; ++i) {
    if (!endpoint.call(poll, receipt).inProgress()) break;
  }
  endpoint.mac.onFrame = nullptr;
  return issued;
}

void sharedSilence() {
  for (const bool regress : {false, true}) {
    Host::reset();
    SharedBus bus;
    Endpoint first, peer;
    // Separate instrumented MAC objects share the management reservation and
    // lock. This models two owners and does not install two physical P4 MACs.
    if (!install(first, bus) || !peer.install(bus, 1)) { CHECK(false); continue; }
    Receipt receipt{};
    auto initialize = request(Action::START_INITIALIZE);
    initialize.reset = ResetKind::HARD;
    CHECK(first.call(initialize, receipt).inProgress());
    const auto token = receipt.token;
    Request scan = request(Action::DISCOVER, 2);
    scan.address = 1;
    CHECK(peer.call(scan, receipt).is(Err::BUSY));
    CHECK(peer.mac.frames.empty() && !receipt.identity.id1Valid);
    if (!issueHard(first, token, regress)) { CHECK(false); continue; }
    OperationSnapshot progress{};
    CHECK(first.phy->operationSnapshot(progress).ok());
    CHECK(progress.busQuietValid == !regress);
    auto result = request(Action::CANCEL);
    result.token = token;
    if (!regress) CHECK(first.call(result, receipt).ok());
    result.action = Action::TAKE;
    CHECK(first.call(result, receipt).ok() && receipt.commandAccepted && receipt.reset == ResetKind::HARD);
    CHECK(bus.admission == BusAdmission::AVAILABLE);
    if (regress) {
      CHECK(receipt.clockInvalid && bus.quarantined);
      Host::state().nowUs = 100000;
      scan.budget = allowance(2);
      CHECK(peer.call(scan, receipt).detail == static_cast<int32_t>(BusyDetail::CONTROLLER_QUARANTINED));
      CHECK(peer.mac.frames.empty() && !receipt.identity.id1Valid);
    } else {
      const auto floor = bus.quietUntilUs.load();
      CHECK(floor == progress.busQuietUntilUs && !bus.quarantined && floor > now());
      Host::state().nowUs = floor - 1;
      scan.budget = allowance(2);
      CHECK(peer.call(scan, receipt).is(Err::BUSY) && peer.mac.frames.empty());
      Host::state().nowUs = floor;
      CHECK(peer.call(scan, receipt).ok() && peer.mac.frames.size() == 2);
      Host::state().nowUs = floor + 1;
      CHECK(peer.call(scan, receipt).ok() && peer.mac.frames.size() == 4);
    }
  }
}

void linkEvents() {
  Host::reset();
  SharedBus bus;
  Endpoint endpoint;
  Processor cli;
  if (!install(endpoint, bus) || !endpoint.initialize() || !attach(endpoint, cli)) { CHECK(false); return; }
  const auto beforeLink = endpoint.mac.frames.size();
  endpoint.mac.registers[1] = 0x7869; // First BMSR still shows historical loss.
  endpoint.mac.onFrame = [&](const Host::Frame& frame) {
    if (!frame.write && frame.reg == 1) endpoint.mac.registers[1] = 0x786D;
  };
  CHECK(command(cli, "link read\n"));
  CHECK(endpoint.mac.frames.size() == beforeLink + 10);
  CHECK(endpoint.transcript.find("cmd=link.read op=0 state=terminal primary=OK") != std::string::npos);
  CHECK(endpoint.transcript.find("observation=up valid=1 stale=0 up=1 resolved=1 speed_mbps=100 duplex=full") !=
        std::string::npos);
  CHECK(endpoint.transcript.find("bmsr_first=30825 bmsr_current=30829") != std::string::npos);
  CHECK(command(cli, "link cached\n"));
  CHECK(endpoint.mac.frames.size() == beforeLink + 10);
  endpoint.mac.onFrame = nullptr;

  if (!command(cli, "configuration apply an 8 none auto 1\n") || endpoint.phy->start(now() + 100000) != ESP_OK) {
    CHECK(false);
    return;
  }
  const auto runningFrames = endpoint.mac.frames.size();
  CHECK(command(cli, "link read\n") && command(cli, "link collect\n"));
  CHECK(endpoint.mac.frames.size() == runningFrames);
  CHECK(endpoint.transcript.find("cmd=link.read op=0 state=terminal primary=BUSY") != std::string::npos);
  CHECK(endpoint.transcript.find("cmd=link.collect op=0 state=terminal primary=BUSY") != std::string::npos);
  if (endpoint.phy->stop(now() + 100000) != ESP_OK) { CHECK(false); return; }

  const auto beforeEvents = endpoint.mac.frames.size();
  TransportSnapshot transportBefore{};
  CHECK(endpoint.phy->transportSnapshot(transportBefore).ok());
  endpoint.mac.registers[0x12] = 0x2001;
  endpoint.mac.readResults = {ESP_OK, ESP_OK, ESP_OK, ESP_ERR_TIMEOUT};
  endpoint.mac.onFrame = [&](const Host::Frame& frame) {
    if (!frame.write && frame.reg == 0x12) endpoint.mac.registers[0x12] = 0;
  };
  CHECK(command(cli, "link collect\n"));
  CHECK(endpoint.transcript.find("cmd=link.collect op=0 state=terminal primary=MDIO_TIMEOUT") != std::string::npos);
  CHECK(endpoint.transcript.find("event_mask=7 physts=21 flds=0 misr1=8193 misr2=0") != std::string::npos);
  const auto afterEvents = endpoint.mac.frames.size();
  CHECK(afterEvents == beforeEvents + 3);
  if (afterEvents == beforeEvents + 3) CHECK(endpoint.mac.frames.back().reg == 0x12);
  TransportSnapshot transportAfter{};
  CHECK(endpoint.phy->transportSnapshot(transportAfter).ok());
  CHECK(transportAfter.totalBatches == transportBefore.totalBatches + 1 &&
        transportAfter.attemptedFrames == transportBefore.attemptedFrames + 4 &&
        transportAfter.completedFrames == transportBefore.completedFrames + 3);
  CHECK(endpoint.mac.registers[0x12] == 0 && bus.quarantined);

  endpoint.transcript.clear();
  CHECK(command(cli, "cached events\n"));
  CHECK(endpoint.transcript.find("cmd=cached.events op=0 state=terminal primary=OK") != std::string::npos);
  CHECK(endpoint.transcript.find("valid=0 stale=1 event_mask=7 physts=21 flds=0 misr1=8193 misr2=0") != std::string::npos);
  CHECK(endpoint.mac.frames.size() == afterEvents);
  endpoint.transcript.clear();
  CHECK(command(cli, "link collect\n"));
  CHECK(endpoint.transcript.find("cmd=link.collect op=0 state=terminal primary=BUSY") != std::string::npos);
  CHECK(endpoint.transcript.find("event_mask=0 physts=0 flds=0 misr1=0 misr2=0") != std::string::npos);
  CHECK(endpoint.mac.frames.size() == afterEvents);
}

void lifecycle() {
  Host::reset();
  SharedBus bus;
  Endpoint endpoint;
  Processor cli;
  if (!install(endpoint, bus) || !attach(endpoint, cli)) { CHECK(false); return; }
  CHECK(endpoint.mac.frames.empty());
  CHECK(command(cli, "connection end\n"));
  CHECK(endpoint.transcript.find("cmd=connection.end op=0 state=terminal primary=INVALID_CONFIG") != std::string::npos);
  CHECK(endpoint.mac.frames.empty());

  endpoint.owner.end = [](void* context, uint64_t deadlineUs) {
    auto& app = *static_cast<Endpoint*>(context);
    const auto retired = app.phy->retire(deadlineUs);
    // The native SDK keeps installed storage; preserve its actual refusal to
    // relinquish that storage, including its signed SDK detail.
    return retired == ESP_OK ? Status::Ok() : Status::Error(Err::INVALID_CONFIG, "Native retirement", retired);
  };
  const std::string_view start = "owner start initialize\n";
  CHECK(cli.feed(start) == start.size());
  cli.tick();
  OperationSnapshot operation{};
  CHECK(endpoint.phy->operationSnapshot(operation).ok() && operation.state == OperationState::RUNNING);
  CHECK(operation.token.value != 0 && endpoint.mac.frames.empty());
  Receipt receipt{};
  const auto activeEnd = endpoint.call(request(Action::END), receipt);
  CHECK(activeEnd.is(Err::BUSY) && activeEnd.detail == static_cast<int32_t>(BusyDetail::OPERATION_ACTIVE));
  CHECK(endpoint.call(request(Action::OBSERVE_LINK), receipt).is(Err::BUSY) && !receipt.link.valid);
  CHECK(endpoint.call(request(Action::COLLECT_EVENTS), receipt).is(Err::BUSY) && !receipt.batchValid);

  const std::string token = std::to_string(operation.token.value);
  const std::string cancel = "owner cancel " + token + '\n';
  endpoint.maintenance = false;
  const auto checks = endpoint.maintenanceChecks;
  CHECK(cli.feed(cancel) == cancel.size());
  cli.tick();
  CHECK(endpoint.maintenanceChecks == checks && endpoint.mac.frames.empty());
  CHECK(endpoint.phy->operationSnapshot(operation).ok() && operation.state == OperationState::RESULT_PENDING);
  endpoint.maintenance = true;
  const auto pendingEnd = endpoint.call(request(Action::END), receipt);
  CHECK(pendingEnd.is(Err::BUSY) && pendingEnd.detail == static_cast<int32_t>(BusyDetail::RESULT_PENDING));
  CHECK(drain(cli));
  const std::string terminal = "cmd=owner.start op=" + token + " state=terminal primary=CANCELLED";
  const auto at = endpoint.transcript.find(terminal);
  CHECK(at != std::string::npos && at == endpoint.transcript.rfind(terminal));
  CHECK(command(cli, "owner result " + token + '\n'));
  CHECK(endpoint.transcript.find("cmd=owner.result op=" + token + " state=terminal primary=OK") != std::string::npos);
  CHECK(endpoint.transcript.find("origin_req=2 outcome=CANCELLED") != std::string::npos);
  CHECK(endpoint.phy->operationSnapshot(operation).ok() && operation.state == OperationState::IDLE);
  CHECK(endpoint.mac.frames.empty());

  CHECK(command(cli, "recovery reset pcs\n"));
  CHECK(endpoint.transcript.find("requested_reset=pcs effective_reset=pcs reset_required=1 reset_accepted=1 reset_complete=1") !=
        std::string::npos);
  CHECK(std::any_of(endpoint.mac.frames.begin(), endpoint.mac.frames.end(), [](const Host::Frame& frame) {
    return frame.write && frame.reg == 0 && (frame.value & 0x8000U) != 0;
  }));
  const auto beforeEnd = endpoint.mac.frames.size();
  endpoint.transcript.clear();
  CHECK(command(cli, "connection end\n"));
  const auto unsupported = "cmd=connection.end op=0 state=terminal primary=INVALID_CONFIG detail=" +
      std::to_string(ESP_ERR_NOT_SUPPORTED);
  CHECK(endpoint.transcript.find(unsupported) != std::string::npos);
  LifecycleRecord life{};
  CHECK(endpoint.phy->lifecycleSnapshot(life).ok() && life.phase == Phase::MAINTENANCE);
  CHECK(endpoint.handle && endpoint.mac.alive && endpoint.mac.frames.size() == beforeEnd);
}
void features() {
  Host::reset();
  SharedBus bus;
  Endpoint endpoint;
  Endpoint peer;
  if (!install(endpoint, bus) || !peer.install(bus, 1) || !endpoint.initialize()) { CHECK(false); return; }
  endpoint.owner.write = Endpoint::write;
  Processor cli;
  CHECK(cli.setup(endpoint.owner.platform(), Identity{}));
  const auto original = endpoint.mac.frames.size();
  CHECK(command(cli, "power status\n"));
  CHECK(command(cli, "power configure normal 0 0\n"));
  CHECK(endpoint.transcript.find("cmd=power.configure op=0 state=terminal primary=BUSY") != std::string::npos);
  CHECK(endpoint.mac.frames.size() == original);

  endpoint.owner.featureAttestation = [](void*, Feature feature, uint32_t proof) {
    return (feature == Feature::POWER_CONFIGURE && proof == (1U << 6U)) ||
        (feature == Feature::EEE_CONFIGURE && proof == 0);
  };
  endpoint.maintenance = false;
  CHECK(command(cli, "power configure normal 0 0\n"));
  CHECK(endpoint.mac.frames.size() == original);
  endpoint.maintenance = true;
  endpoint.transcript.clear();
  CHECK(command(cli, "power configure normal 0 0\n"));
  CHECK(endpoint.transcript.find("cmd=power.configure op=2 state=terminal primary=OK") != std::string::npos);
  CHECK(endpoint.mac.frames.size() > original);

  constexpr std::string_view eee = "eee configure off 0 0\n";
  CHECK(cli.feed(eee) == eee.size());
  cli.tick();
  CHECK(bus.admission == BusAdmission::INITIALIZING && cli.workPending());
  const auto peerFrames = peer.mac.frames.size();
  Receipt receipt{};
  CHECK(peer.call(request(Action::DISCOVER, 2), receipt).is(Err::BUSY));
  CHECK(peer.mac.frames.size() == peerFrames);
  CHECK(command(cli, "owner cancel 3\n"));
  CHECK(bus.admission == BusAdmission::AVAILABLE && !cli.workPending());
  CHECK(endpoint.transcript.find("cmd=eee.configure op=3 state=terminal primary=CANCELLED") != std::string::npos);
  CHECK(endpoint.transcript.find("effective_reset=hard") != std::string::npos);

  bool issued = false;
  endpoint.mac.onFrame = [&](const Host::Frame& frame) {
    if (frame.write && frame.reg == 0x1F && (frame.value & 0x8000U)) {
      issued = true;
      Host::state().nowUs = 1;
    }
  };
  CHECK(command(cli, eee));
  endpoint.mac.onFrame = nullptr;
  CHECK(issued && bus.quarantined && bus.admission == BusAdmission::AVAILABLE);
  CHECK(endpoint.transcript.find("cmd=eee.configure op=4 state=terminal primary=INVALID_CONFIG") != std::string::npos);
  Host::state().nowUs = 1000000; // Give the peer a valid clock; the controller remains quarantined.
  CHECK(peer.call(request(Action::DISCOVER, 2), receipt).detail ==
        static_cast<int32_t>(BusyDetail::CONTROLLER_QUARANTINED));
  CHECK(peer.mac.frames.size() == peerFrames);
}

void featureAdmission() {
  Host::reset();
  SharedBus bus;
  Endpoint endpoint;
  Processor cli;
  if (!install(endpoint, bus) || !attach(endpoint, cli)) { CHECK(false); return; }
  auto before = endpoint.mac.frames.size();
  CHECK(command(cli, "power read\n"));
  CHECK(endpoint.transcript.find("cmd=power.read op=0 state=terminal primary=NOT_INITIALIZED") != std::string::npos);
  CHECK(endpoint.mac.frames.size() == before);

  endpoint.mac.registers[3] = 0xA130;
  Receipt receipt{};
  CHECK(endpoint.call(request(Action::PROBE), receipt).is(Err::DEVICE_MISMATCH));
  before = endpoint.mac.frames.size();
  CHECK(command(cli, "wol read\n"));
  CHECK(command(cli, "irq service\n"));
  CHECK(endpoint.transcript.find("cmd=wol.read op=0 state=terminal primary=DEVICE_MISMATCH") != std::string::npos);
  CHECK(endpoint.transcript.find("cmd=irq.service op=0 state=terminal primary=DEVICE_MISMATCH") != std::string::npos);
  CHECK(endpoint.mac.frames.size() == before);

  endpoint.mac.registers[3] = 0xA131;
  CHECK(endpoint.call(request(Action::PROBE), receipt).ok());
  before = endpoint.mac.frames.size();
  CHECK(command(cli, "power read\n"));
  CHECK(endpoint.transcript.find("cmd=power.read op=0 state=terminal primary=OK") != std::string::npos);
  CHECK(endpoint.mac.frames.size() == before + 3);
}

void blockedPoll() {
  Host::reset();
  SharedBus bus;
  Endpoint endpoint;
  Processor cli;
  if (!install(endpoint, bus) || !endpoint.initialize() || !attach(endpoint, cli)) { CHECK(false); return; }
  endpoint.owner.featureAttestation = [](void*, Feature, uint32_t) { return true; };
  constexpr std::string_view start = "power configure normal 0 0\n";
  CHECK(cli.feed(start) == start.size());
  cli.tick();
  OperationSnapshot active{};
  CHECK(endpoint.phy->operationSnapshot(active).ok() && active.state == OperationState::RUNNING);
  const auto before = endpoint.mac.frames.size();
  CHECK(esp_eth_start(endpoint.handle) != ESP_OK);
  CHECK(endpoint.phy->pollOperation(active.token).is(Err::BUSY));
  cli.tick();
  CHECK(cli.workPending());
  Host::state().nowUs = active.remaining.deadlineUs;
  CHECK(drain(cli));
  CHECK(endpoint.phy->operationSnapshot(active).ok() && active.state == OperationState::IDLE);
  CHECK(endpoint.transcript.find("cmd=power.configure op=2 state=terminal primary=OPERATION_TIMEOUT") != std::string::npos);
  CHECK(endpoint.transcript.find("core_status=CANCELLED") != std::string::npos);
  CHECK(endpoint.mac.frames.size() == before);
}
} // namespace

int main(int argc, char** argv) {
  const test::Suite suites[] = {{"admission", admission}, {"reconciliation", reconciliation},
                                {"discovery", discovery}, {"raw_access", rawAccess},
                                {"cross_entry_boundaries", crossEntryBoundaries},
                                {"restart_after_raw_observation", restartAfterRawObservation},
                                {"shared_silence", sharedSilence}, {"clock_fault", clockFault},
                                {"link_events", linkEvents}, {"lifecycle", lifecycle}, {"features", features},
                                {"feature_admission", featureAdmission}, {"blocked_poll", blockedPoll}};
  return test::runSuites(argc, argv, suites);
}
