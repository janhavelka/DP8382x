// Literal SNLS638C table 7-26 and section 5.6 footnote 2. This suite uses the
// same exercising controller as the existing interface/reset tests.
void p25TimingAndInterface() {
  for (bool follower : {false, true}) {
    for (bool revision10 : {false, true}) {
      for (uint8_t depth = 0; depth < 4; ++depth) {
        Controller bus;
        Driver driver;
        p25Board(bus, follower, follower, follower);
        ready(driver, bus);
        auto request = settings();
        request.rmiiClock = bus.boardRole;
        request.referenceClockMHz = follower ? 50 : 25;
        request.revision10 = revision10;
        request.elasticityCode = depth;
        request.rxDv = request.repeater = request.repeaterWiringAdmitted = true;
        request.fastRxDv = request.oddNibbleDisable = request.shortIpg = true;
        bus.words[23] |= 0x000C;
        const auto start = bus.frames;
        configured(driver, bus, request);
        CHECK(bus.words[23] == uint16_t(0x0060 | (follower ? 0x0080 : 0) | (revision10 ? 0x10 : 0) | depth));
        CHECK((bus.words[9] & 2) && (bus.words[10] & 2) && (bus.ext[0x456] & 8));
        CHECK((bus.ext[0x302] & 0x0100) != 0 && writes(bus, start, 0x42) == 0);
        CHECK(writes(bus, start, 0x17) == 1 && writes(bus, start, 0x11) == 0);
      }
    }
  }

  // Locate the two distinct writes from actual physical frames. Cut the
  // complete second transaction at each frame, with and without an effect.
  unsigned companionOffset = 0, rcsrOffset = 0, rcsrCallOffset = 0;
  for (int cut = -1; cut <= 4; ++cut) {
    for (bool accepted : {false, true}) {
      Controller bus;
      Driver driver;
      p25Board(bus, true, true, true);
      ready(driver, bus);
      auto request = settings();
      request.rmiiClock = bus.boardRole;
      request.referenceClockMHz = 50;
      request.txClockShift = true;
      const auto start = bus.frames;
      const auto startCall = bus.calls;
      bus.words[23] |= 0x000C;
      CHECK(driver.clearEvidence().ok());
      if (cut >= 0) {
        bus.failFrame = int(start + (cut == 0 ? rcsrOffset : companionOffset - 4U + static_cast<unsigned>(cut)));
        bus.acceptedFailure = accepted;
      }
      InterfaceResult output;
      const auto status = driver.configureInterface(request, output, {1000000, 256});
      CHECK(status.ok() == (cut < 0));
      CHECK(output.mutationPossible && output.attemptedFrames == bus.frames - start);
      const auto sample = driver.evidenceSnapshot().samples[static_cast<unsigned>(EvidenceSource::RCSR)];
      CHECK((sample.eventBits & 0x000C) == 0x000C && sample.valid);
      if (cut < 0) {
        for (unsigned i = start; i < bus.frames; ++i) {
          if (!semantic(bus.trace[i])) continue;
          if (bus.trace[i].target == 0x17) rcsrOffset = i - start;
          if (bus.trace[i].target == 0x42) companionOffset = i - start;
        }
        CHECK(rcsrOffset && companionOffset == rcsrOffset + 4);
        for (unsigned i = startCall; i < bus.calls; ++i) {
          if (bus.firstFrame[i] == start + rcsrOffset - 1) rcsrCallOffset = i - startCall;
        }
        CHECK(bus.ext[0x42] == 0x0014 && (bus.words[23] & 0x0180) == 0x0180);
        const auto callbacks = bus.calls, clocks = bus.clocks;
        request.txClockShift = false;
        CHECK(!driver.configureInterface(request, output, {1000000, 256}).ok());
        CHECK(bus.calls == callbacks && bus.clocks == clocks);
      } else {
        CHECK(status.is(Err::MDIO_TIMEOUT) && driver.operationSnapshot().cleanupOwed);
        CHECK(!output.verified && output.status.detail == -33033);
        CHECK(bus.ext[0x42] == ((cut == 4 && accepted) ? 0x0014 : 0));
      }
      // Restoring the owned shifted profile is idempotent, including a
      // previously uncertain opaque write. A later verify only observes it.
      bus.failFrame = -1;
      OperationToken token;
      CHECK(driver.startInitialize({ResetKind::HARD}, budget(), token).inProgress());
      CHECK(drain(driver, bus, token).ok());
      CHECK(take(driver, token).persistentRestored);
      CHECK(bus.ext[0x42] == 0x0014 && !driver.operationSnapshot().cleanupOwed);
      const auto afterReset = bus.frames;
      CHECK(driver.startVerify(budget(), token).inProgress());
      CHECK(drain(driver, bus, token).ok() && take(driver, token).status.ok());
      CHECK(writes(bus, afterReset, 0x42) == 0 && writes(bus, 0, 0x030E) == 0);
    }
  }
  // A newly consumed FIFO sample survives a failed release. A changed
  // reference on the RMW read suppresses its write even after earlier rows.
  for (bool changedRole : {false, true}) {
    Controller bus;
    Driver driver;
    p25Board(bus, true, true, true);
    ready(driver, bus);
    CHECK(driver.clearEvidence().ok());
    auto request = settings();
    request.rmiiClock = bus.boardRole;
    request.referenceClockMHz = 50;
    request.txClockShift = true;
    bus.words[23] |= 0x000C;
    const auto first = bus.frames;
    if (changedRole) {
      bus.corruptFrame = static_cast<int>(first + rcsrOffset - 1);
      bus.corruptMask = 0x80;
    } else {
      CHECK(rcsrCallOffset != 0);
      bus.releaseCall = static_cast<int>(bus.calls + rcsrCallOffset);
    }
    InterfaceResult result;
    const auto status = driver.configureInterface(request, result, {1000000, 256});
    CHECK(status.is(changedRole ? Err::CONFIG_MISMATCH : Err::MDIO_ERROR));
    CHECK(result.mismatch.valid == changedRole && result.mutationPossible && !result.verified);
    CHECK(writes(bus, first, 0x17) == (changedRole ? 0U : 1U));
    CHECK(writes(bus, first, 0x42) == 0);
    const auto fifo = driver.evidenceSnapshot().samples[static_cast<unsigned>(EvidenceSource::RCSR)];
    CHECK(fifo.samples == 1 && fifo.eventBits == 0x000C && !fifo.historyLost);
  }
}

void p25ClockLossAndGuards() {
  for (bool approved : {false, true}) {
    Controller bus;
    Driver driver;
    p25Board(bus, false, false, false);
    bus.clockLossApproved = approved;
    ready(driver, bus);
    auto request = settings();
    request.isolationOwned = request.isolate = true;
    InterfaceResult output;
    const auto calls = bus.calls, clocks = bus.clocks;
    CHECK(driver.configureInterface(request, output, {1000000, 256}).ok() == approved);
    CHECK(((bus.words[0] & 0x0400) != 0) == approved);
    if (!approved) CHECK(bus.calls == calls && bus.clocks == clocks);
    bus.ext[0x305] = 0xF8E0;
    CHECK(driver.modifyMmd(31, 0x0305, 7, 4, {1000000, 5}).ok() == approved);
    CHECK(bus.ext[0x305] == (approved ? 0xF8E4 : 0xF8E0));
    if (!approved) CHECK(bus.calls == calls && bus.clocks == clocks);
  }

  for (bool atWrite : {false, true}) {
    Controller bus;
    Driver driver;
    p25Board(bus, false, false, false);
    ready(driver, bus);
    LinkSettings intent;
    intent.advertisement.technologyMask = 15;
    OperationToken token;
    CHECK(driver.startConfigure(intent, {}, budget(), token).inProgress());
    if (!atWrite) bus.words[0] |= 0x0400;
    else {
      // Cross the earlier preflight, then change isolation before the
      // dependent read. The same-lock predicate must suppress the BMCR write.
      while (driver.operationSnapshot().phase != OperationPhase::COMMAND) {
        if (!poll(driver, bus, token).inProgress()) break;
      }
      bus.words[0] |= 0x0400;
    }
    const auto first = bus.frames;
    CHECK(!drain(driver, bus, token).ok());
    const auto result = take(driver, token);
    CHECK(writes(bus, first, 0) == 0 && (bus.words[0] & 0x0400) != 0);
    CHECK(result.savedBmcrValid && result.mutationPossible == atWrite);
    if (atWrite) CHECK(result.status.is(Err::CONFIG_MISMATCH));
    CHECK(driver.transportSnapshot().violation == TransportViolation::NONE);
  }

  Controller bus;
  Driver driver;
  p25Board(bus, false, false, false);
  bus.clockLossApproved = true;
  ready(driver, bus);
  bus.ext[0x467] ^= 8;
  uint16_t observed = 0;
  CHECK(driver.readMmd(31, 0x467, observed, {1000000, 4}).ok() && (observed & 8));
  const auto calls = bus.calls, clocks = bus.clocks;
  CHECK(!driver.modifyMmd(31, 0x305, 7, 1, {1000000, 5}).ok());
  CHECK(!driver.writeMmd(31, 0x428, 4, {1000000, 4}).ok());
  CHECK(bus.calls == calls && bus.clocks == clocks && !driver.isInitialized());
}

void p25OptionalAdmission() {
  Controller bus;
  Driver driver;
  p25Board(bus, false, false, false);
  ready(driver, bus);
  const auto calls = bus.calls, clocks = bus.clocks;
  const IoBudget io{1000000, 256};
  OperationToken token{0xCAFE};
  LoopbackOptions loop;
  loop.maintenanceAdmitted = true;
  BistOptions bist;
  bist.loopback = loop;
  CHECK(driver.startLoopback(loop, budget(), token).is(Err::UNSUPPORTED));
  CHECK(driver.startBist(bist, budget(), token).is(Err::UNSUPPORTED));
  CHECK(driver.startConfigureEee({}, budget(), token).is(Err::UNSUPPORTED));
  CHECK(driver.startConfigurePower({}, budget(), token).is(Err::UNSUPPORTED));
  CHECK(driver.acknowledgeWake(budget(), token).is(Err::UNSUPPORTED));
  CHECK(driver.startInterruptTest(budget(), token).is(Err::UNSUPPORTED));
  CHECK(driver.startCableAcquire(budget(), token).is(Err::UNSUPPORTED));
  CHECK(driver.startConfigureAnalog({}, budget(), token).is(Err::UNSUPPORTED));
  CHECK(driver.modifyInterruptEnables(0x12, 1, 1, io).is(Err::UNSUPPORTED));
  CHECK(driver.modifyWakeControl(1, 1, false, io).is(Err::UNSUPPORTED));
  PinResult pins;
  CHECK(driver.configurePins({}, pins, io).is(Err::UNSUPPORTED));
  CounterSnapshot counters;
  bool invoked = true;
  CHECK(driver.collectEvents(counters, io, &invoked).is(Err::UNSUPPORTED) && !invoked);
  EvidenceSnapshot evidence;
  CHECK(!driver.acquireEvidence(evidenceBit(EvidenceSource::RXFS), evidence, io).ok());
  CHECK(bus.calls == calls && bus.clocks == clocks && token.value == 0xCAFE);
  CHECK(driver.operationSnapshot().state == OperationState::IDLE && driver.isInitialized());
  CHECK(!driver.powerSnapshot().desiredValid && !driver.eeeSnapshot().desiredValid);
  CHECK(!driver.pinsSnapshot().desiredValid && !driver.wakeSnapshot().desiredValid);
}

void p25Profiles() {
  p25OptionalAdmission();
  p25TimingAndInterface();
  p25ClockLossAndGuards();
  Controller bus;
  Driver driver;
  p25Board(bus, false, false, false);
  ready(driver, bus);
  for (uint16_t subset = 0; subset < 32; ++subset) {
    QualitySettings options;
    options.fastDrop.owned = options.tx100.owned = true;
    options.maintenanceAdmitted = options.macStopped = true;
    options.fastDrop.causeMask = (subset & 0x0F) | ((subset & 0x10) ? 0x0400 : 0);
    options.tx100.rxErrorDuringIdle = (subset & 1) != 0;
    const auto start = bus.frames;
    bus.words[11] |= 0x0060;
    OperationToken token;
    CHECK(driver.startConfigureQuality(options, budget(), token).inProgress());
    CHECK(drain(driver, bus, token).ok());
    CHECK(take(driver, token).status.ok());
    CHECK(bus.words[11] == (0x0060 | options.fastDrop.causeMask));
    CHECK(((bus.words[10] & 4) != 0) == options.tx100.rxErrorDuringIdle);
    CHECK(writes(bus, start, 0x117) == 0 && writes(bus, start, 0x131) == 0);
    CHECK(!driver.qualitySnapshot().fldMaximumApplicable);
  }
  auto invalid = bus.config();
  for (auto medium : {Medium::FIBER, static_cast<Medium>(255)}) {
    invalid.medium = medium;
    CHECK(!driver.bind(invalid).ok());
  }
  invalid = bus.config();
  invalid.mdcHz = 20000001;
  CHECK(!driver.bind(invalid).ok() && driver.isInitialized());
  invalid.mdcHz = 20000000;
  CHECK(driver.bind(invalid).ok());
  CHECK(invalid.hardResetQuietUs() == 2000);
  invalid.mdcHz = 333;
  CHECK(invalid.hardResetQuietUs() == 3004);
  CHECK(driver.bind(invalid).ok());
  OperationToken token;
  CHECK(driver.startInitialize({ResetKind::HARD}, budget(), token).inProgress());
  while (driver.operationSnapshot().phase != OperationPhase::COMMAND) {
    if (!poll(driver, bus, token).inProgress()) break;
  }
  CHECK(poll(driver, bus, token).inProgress());
  CHECK(driver.operationSnapshot().busQuietUntilUs >= bus.now + 3004);
  CHECK(drain(driver, bus, token).ok() && take(driver, token).status.ok());
}
