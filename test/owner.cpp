#include "support/Check.h"
#include "../examples/esp_idf/owner/main/PhyOwner.h"
#include <limits>

namespace {
using namespace DP8382x;
using namespace owner_example;

LinkSettings policy() {
  LinkSettings value;
  value.advertisement.technologyMask = 0x08;
  return value;
}
struct Fixture {
  PhyOwner owner;
  HealthSnapshot health{};
  SettingsSnapshot settings{};
  LinkSnapshot link{};
  bool frameworkReady{true};
  uint64_t now{100};
  Fixture() {
    health.initialized = true;
    health.state = DriverState::READY;
    settings.desired = settings.observed = policy();
    settings.trust = ConfigurationTrust::VERIFIED;
    settings.desiredValid = settings.observedValid = true;
    settings.observedStale = false;
    settings.lastVerificationStatus = Status::Ok();
    link.valid = link.timeValid = link.linkUp = link.resolvedModeValid = true;
    link.stale = false;
    link.lastRefreshStatus = Status::Ok();
    link.negotiation = NegotiationState::COMPLETE;
    link.speed = LinkSpeed::MBPS_100;
    link.duplex = Duplex::FULL;
    link.observedSinceUs = 90;
    link.observedAtUs = now;
    link.observations = link.changes = 1;
    link.rawBmsrFirst = 0x0024;
    CHECK(owner.begin(policy()).ok());
  }
  void observe() { owner.observe(now, health, settings, link, frameworkReady); }
  uint64_t admit() {
    observe();
    uint64_t generation = 0;
    CHECK(owner.admit(now, generation).ok());
    CHECK(generation != 0);
    return generation;
  }
};

void policyEvidence() {
  PhyOwner owner;
  uint64_t generation = 41;
  auto invalid = policy();
  invalid.crossover.swapPair = true;
  CHECK(!owner.begin(invalid).ok());
  invalid = policy();
  invalid.advertisement.technologyMask = 1;
  CHECK(!owner.begin(invalid).ok());
  CHECK(!owner.admit(0, generation).ok() && generation == 41);
  CHECK(owner.snapshot().phase == OwnerPhase::UNCONFIGURED);
  CHECK(!owner.beginMaintenance(true).ok());

  for (unsigned fault = 0; fault < 19; ++fault) {
    Fixture f;
    switch (fault) {
      case 0: f.health.state = DriverState::OFFLINE; break;
      case 1: f.settings.trust = ConfigurationTrust::DIRTY; break;
      case 2: f.settings.observedStale = true; break;
      case 3: f.settings.observed.advertisement.technologyMask = 0xF; break;
      case 4: f.settings.desired.crossover.robust = false; break;
      case 5: f.link.valid = false; break;
      case 6: f.link.stale = true; break;
      case 7: f.link.linkUp = false; break;
      case 8: f.link.negotiation = NegotiationState::IN_PROGRESS; break;
      case 9: f.link.speed = LinkSpeed::MBPS_10; break;
      case 10: f.link.duplex = Duplex::HALF; break;
      case 11: f.link.resolvedModeValid = false; break;
      case 12: f.link.timeValid = false; break;
      case 13: f.link.observedAtUs = 101; break;
      case 14: f.link.observedSinceUs = 101; break;
      case 15: f.now = f.link.observedSinceUs + MAX_LINK_AGE_US + 1; break;
      case 16: f.link.temporaryMayBeActive = true; break;
      case 17: f.link.changes = std::numeric_limits<uint32_t>::max(); break;
      case 18: f.frameworkReady = false; break;
    }
    f.observe();
    CHECK(!f.owner.admit(f.now, generation).ok() && generation == 41);
    CHECK(f.owner.snapshot().phase == OwnerPhase::WAITING);
    CHECK(f.owner.snapshot().health.state == f.health.state);
    CHECK(f.owner.snapshot().link.linkUp == f.link.linkUp);
  }
}

void continuity() {
  for (unsigned loss = 0; loss < 9; ++loss) {
    Fixture f;
    const auto generation = f.admit();
    CHECK(f.owner.allowFrame(f.now,generation).ok());
    CHECK(!f.owner.allowFrame(f.now,generation + 1).ok());
    CHECK(!f.owner.beginMaintenance(true).ok());
    switch (loss) {
      case 0: f.link.linkUp = false; break;
      case 1: f.link.changes += 2; break; // DOWN then UP between application checks.
      case 2: ++f.link.observations; f.link.rawBmsrFirst = 0x0020; break;
      case 3: f.link.observations += 2; break; // History unavailable for skipped sample.
      case 4: f.now += MAX_SERVICE_GAP_US + 1; break;
      case 5: --f.now; break;
      case 6: f.owner.missingEvidence(f.now); break; // Native owner unavailable.
      case 7: f.link.stale = true; break;
      case 8: f.settings.trust = ConfigurationTrust::DIRTY; break;
    }
    f.observe();
    CHECK(f.owner.snapshot().phase == OwnerPhase::WITHDRAWN);
    unsigned frameCalls = 0;
    if (f.owner.allowFrame(f.now, generation).ok()) ++frameCalls;
    CHECK(frameCalls == 0);
    CHECK(!f.owner.beginMaintenance(true).ok());
    f.link.linkUp = true;
    f.link.stale = false;
    f.settings.trust = ConfigurationTrust::VERIFIED;
    f.observe();
    CHECK(!f.owner.allowFrame(f.now,generation).ok());
    CHECK(!f.owner.release(generation + 1).ok());
    CHECK(f.owner.release(generation).ok());
    CHECK(!f.owner.beginMaintenance(false).ok());
    CHECK(f.owner.beginMaintenance(true).ok());
    CHECK(!f.owner.beginMaintenance(true).ok());
  }

  Fixture f;
  f.now = f.link.observedSinceUs + MAX_LINK_AGE_US;
  const auto generation = f.admit();
  CHECK(f.owner.allowFrame(f.now,generation).ok());
  CHECK(!f.owner.allowFrame(++f.now,generation).ok()); // Age, independent of service gap.
  Fixture gap;
  const auto live = gap.admit();
  CHECK(gap.owner.allowFrame(gap.now + MAX_SERVICE_GAP_US,live).ok());
  CHECK(!gap.owner.allowFrame(gap.now + MAX_SERVICE_GAP_US + 1,live).ok());

  Fixture clock;
  const auto session = clock.admit();
  CHECK(clock.owner.allowFrame(clock.now + 10, session).ok());
  clock.now += 5; // Later than the last service, earlier than the final TX check.
  clock.observe();
  CHECK(clock.owner.snapshot().phase == OwnerPhase::WITHDRAWN);
  CHECK(clock.owner.release(session).ok());
  clock.now += 20;
  clock.observe();
  CHECK(clock.owner.snapshot().phase == OwnerPhase::WAITING);

  Fixture lost;
  const auto old = lost.admit();
  lost.owner.cachedLinkDown(++lost.now);
  CHECK(lost.owner.release(old).ok());
  lost.observe();
  CHECK(lost.owner.snapshot().phase == OwnerPhase::WAITING);
  lost.link.observedAtUs = lost.link.observedSinceUs = ++lost.now;
  lost.observe(); // Fresh PHY evidence cannot repair the SDK's own link cache.
  CHECK(lost.owner.snapshot().frameworkRestartRequired && !lost.owner.snapshot().frameworkReady);
  uint64_t refused = old;
  CHECK(!lost.owner.admit(lost.now, refused).ok() && refused == old);
  CHECK(lost.owner.beginMaintenance(true).ok());
  lost.owner.finishMaintenance(++lost.now);
  lost.owner.frameworkRestarted();
  lost.observe(); // Pre-maintenance sample.
  CHECK(lost.owner.snapshot().phase == OwnerPhase::WAITING);
  lost.link.observedAtUs = lost.link.observedSinceUs = ++lost.now;
  CHECK(lost.admit() != old);
}

void recovery() {
  Fixture f;
  const auto old = f.admit();
  f.link.linkUp = false;
  f.observe();
  CHECK(f.owner.release(old).ok());
  CHECK(f.owner.beginMaintenance(true).ok());
  OperationResult cancelled;
  cancelled.token = {1};
  cancelled.kind = OperationKind::INITIALIZE;
  cancelled.status = Status::Error(Err::CANCELLED, "Cancelled after reset");
  cancelled.cleanupOwed = cancelled.mutationPossible = true;
  CHECK(f.owner.retainResult(cancelled).ok());
  f.owner.finishMaintenance(f.now);
  CHECK(!f.owner.beginMaintenance(true).ok());
  OperationResult restored;
  restored.token = {2};
  restored.kind = OperationKind::INITIALIZE;
  restored.status = Status::Ok();
  restored.persistentRestored = true;
  CHECK(!f.owner.retainResult(restored).ok());
  OperationResult original;
  CHECK(f.owner.takeResult(original).ok());
  CHECK(original.token.value == 1 && original.status.is(Err::CANCELLED));
  CHECK(original.cleanupOwed && original.mutationPossible);
  CHECK(f.owner.beginMaintenance(true).ok());
  CHECK(f.owner.retainResult(restored).ok());
  f.owner.finishMaintenance(++f.now);
  OperationResult result;
  CHECK(f.owner.takeResult(result).ok() && result.token.value == 2);
  CHECK(result.persistentRestored && original.cleanupOwed);
  CHECK(!f.owner.takeResult(original).ok() && original.token.value == 1);
  f.link.linkUp = true;
  f.observe();
  CHECK(f.owner.snapshot().phase == OwnerPhase::WAITING); // Pre-maintenance sample.
  f.link.observedAtUs = f.link.observedSinceUs = ++f.now;
  f.observe();
  CHECK(!f.owner.allowFrame(f.now,old).ok());
  const auto next = f.admit();
  CHECK(next != old && !f.owner.allowFrame(f.now,old).ok());
  CHECK(f.owner.allowFrame(f.now,next).ok());
}
}
int main(int argc, char** argv) {
  const test::Suite suites[] = {{"policy",policyEvidence},{"continuity",continuity},{"recovery",recovery}};
  return test::runSuites(argc,argv,suites);
}
