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
        request.rxDv = true;
        request.repeater = request.repeaterWiringAdmitted = follower;
        request.fastRxDv = request.oddNibbleDisable = request.shortIpg = true;
        bus.words[23] |= 0x000C;
        const auto start = bus.frames;
        configured(driver, bus, request);
        CHECK(bus.words[23] == uint16_t(0x0060 | (follower ? 0x0080 : 0) | (revision10 ? 0x10 : 0) | depth));
        CHECK((bus.words[9] & 2) && (bus.words[10] & 2) && (bus.ext[0x456] & 8));
        CHECK((bus.ext[0x302] & 0x0100) != 0 && writes(bus, start, 0x42) == 0);
        CHECK(writes(bus, start, 0x17) == 1 && writes(bus, start, 0x11) == 0);
        if (!follower) {
          // Figure 6-4 uses a common 50 MHz oscillator for both PHYs.
          // Section 6.3.9 separately permits RX_DV on this ordinary leader.
          request.repeater = request.repeaterWiringAdmitted = true;
          InterfaceResult result;
          result.attemptedFrames = 999;
          const auto calls = bus.calls, clocks = bus.clocks;
          CHECK(driver.configureInterface(request, result, {1000000, 256}).is(Err::INVALID_CONFIG));
          CHECK(bus.calls == calls && bus.clocks == clocks && result.attemptedFrames == 999);
          const auto view = driver.interfaceSnapshot();
          CHECK(view.desiredValid && view.desired.rxDv && !view.desired.repeater);
          CHECK(view.trust == ConfigurationTrust::VERIFIED);
        }
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
    if (!approved) {
      request.clockOutput=ClockOutput::LED1;
      InterfaceResult denied;
      const auto before=bus.calls, clock=bus.clocks;
      CHECK(driver.configureInterface(request,denied,{1000000,256}).is(Err::INVALID_CONFIG));
      CHECK(bus.calls==before && bus.clocks==clock);
      request.clockOutput=ClockOutput::UNOWNED;
    }
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
  CHECK(driver.startConfigureEee({}, budget(), token).is(Err::INVALID_CONFIG));
  CHECK(driver.startConfigurePower({}, budget(), token).is(Err::INVALID_CONFIG));
  CHECK(driver.acknowledgeWake(budget(), token).is(Err::INVALID_CONFIG));
  CHECK(driver.startInterruptTest(budget(), token).is(Err::INVALID_CONFIG));
  CHECK(driver.startCableAcquire(budget(), token).is(Err::RESULT_NOT_AVAILABLE));
  CHECK(driver.startConfigureAnalog({}, budget(), token).is(Err::INVALID_CONFIG));
  QualitySettings quality;
  quality.tx100.owned = quality.maintenanceAdmitted = quality.macStopped = true;
  quality.tx100.scramblerBypass = true;
  CHECK(driver.startConfigureQuality(quality, budget(), token).is(Err::UNSUPPORTED));
  quality.tx100.scramblerBypass = false;
  for (uint8_t code = 1; code <= 3; ++code) {
    quality.tx100.fifoDepthCode = code;
    CHECK(driver.startConfigureQuality(quality, budget(), token).is(Err::UNSUPPORTED));
  }
  CHECK(!driver.qualitySnapshot().desiredValid);
  CHECK(!driver.modifyInterruptEnables(0x0C, 1, 1, io).ok());
  CHECK(!driver.modifyWakeControl(0x40, 0x40, false, io).ok());
  PinResult pins;
  CHECK(driver.configurePins({}, pins, io).is(Err::INVALID_CONFIG));
  CHECK(bus.calls == calls && bus.clocks == clocks && token.value == 0xCAFE);
  CHECK(driver.operationSnapshot().state == OperationState::IDLE && driver.isInitialized());
  CHECK(!driver.powerSnapshot().desiredValid && !driver.eeeSnapshot().desiredValid);
  CHECK(!driver.pinsSnapshot().desiredValid && !driver.wakeSnapshot().desiredValid);
}

void p25QualityClockEvidence() {
  for (bool follower : {false, true}) {
    for (auto expected : {Err::CONFIG_MISMATCH, Err::MDIO_TIMEOUT, Err::MDIO_ERROR}) {
      Controller bus;
      Driver driver;
      p25Board(bus, follower, follower, follower);
      ready(driver, bus);
      CHECK(driver.clearEvidence().ok());
      bus.words[23] ^= 0x0080;
      bus.words[23] |= 0x000C;
      const auto observed = bus.words[23];
      QualitySettings settings;
      settings.tx100.owned = settings.maintenanceAdmitted = settings.macStopped = true;
      OperationToken token;
      CHECK(driver.startConfigureQuality(settings, budget(), token).inProgress());
      // RCSR is the third preflight read. Fail the next read or the batch release.
      if (expected == Err::MDIO_TIMEOUT) bus.failFrame = static_cast<int>(bus.frames + 3);
      if (expected == Err::MDIO_ERROR) bus.releaseCall = static_cast<int>(bus.calls);
      const auto mutations = bus.mutations;
      const auto status = drain(driver, bus, token);
      const auto result = take(driver, token);
      const auto& mismatch = result.mismatch;
      CHECK(status.is(expected) && result.status.is(expected));
      CHECK(mismatch.valid && !mismatch.mmd && mismatch.registerAddress == 0x17);
      CHECK(mismatch.mask == 0x0080 && mismatch.expected == (follower ? 0x0080 : 0));
      CHECK(mismatch.observed == observed);
      const auto fifo = driver.evidenceSnapshot().samples[static_cast<unsigned>(EvidenceSource::RCSR)];
      CHECK(fifo.samples == 1 && fifo.eventBits == 0x000C);
      CHECK(!driver.isInitialized() && bus.mutations == mutations && !result.mutationPossible);
    }
  }
}

void p25Profiles() {
  p25OptionalAdmission();
  p25TimingAndInterface();
  p25ClockLossAndGuards();
  p25QualityClockEvidence();
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
    // PHYSCR[11,9:8] belong to compliance and far-end loopback (table 7-20).
    bus.words[0x11] = 0x0900;
    OperationToken token;
    CHECK(driver.startConfigureQuality(options, budget(), token).inProgress());
    CHECK(drain(driver, bus, token).ok());
    CHECK(take(driver, token).status.ok());
    CHECK(bus.words[11] == (0x0060 | options.fastDrop.causeMask));
    CHECK(((bus.words[10] & 4) != 0) == options.tx100.rxErrorDuringIdle);
    CHECK(writes(bus, start, 0x11) == 0 && bus.words[0x11] == 0x0900);
    CHECK(writes(bus, start, 0x117) == 0 && writes(bus, start, 0x131) == 0);
    CHECK(!driver.qualitySnapshot().fldMaximumApplicable);
  }
  // A separate owner changes both fields before persistent quality replay.
  bus.words[0x11] = 0x0200;
  OperationToken restore;
  const auto first = bus.frames;
  CHECK(driver.startInitialize({ResetKind::NONE}, budget(), restore).inProgress());
  CHECK(drain(driver, bus, restore).ok() && take(driver, restore).persistentRestored);
  CHECK(writes(bus, first, 0x11) == 0 && bus.words[0x11] == 0x0200);
  CHECK(driver.qualitySnapshot().trust == ConfigurationTrust::VERIFIED);
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

// SNLS638C tables 7-15 and 7-19..26. 000C and MISR1 are independent
// inputs here. The relationship between their actual silicon latches is open.
void p25Events() {
  for (bool follower : {false, true}) {
    Controller bus;
    Driver driver;
    p25Board(bus, follower, follower, follower);
    ready(driver, bus);
    CHECK(driver.clearEvidence().ok());
    const IoBudget io{1000000, 32};
    const auto stamp = bus.frames;
    bus.words[12] = 0x815A;
    bus.words[18] = 0x8302;
    bus.words[19] = 0x0408;
    bus.words[20] = 0x00FF;
    bus.words[21] = 0xFAC7;
    InterruptServiceSnapshot service;
    bus.failFrame = static_cast<int>(stamp + 2); // MISR1 completed, MISR2 did not.
    CHECK(driver.serviceInterrupts(service, {io, 2, 5}).is(Err::MDIO_TIMEOUT));
    CHECK(!service.valid && bus.trace[stamp].target == 16 && bus.trace[stamp+1].target == 18);
    auto evidence = driver.evidenceSnapshot();
    CHECK(evidence.samples[static_cast<unsigned>(EvidenceSource::MISR1)].eventBits == 0x8300);
    CHECK(evidence.pendingInterruptCounters == 3 && bus.words[12] == 0x815A);
    bus.failFrame = -1;
    CHECK(driver.serviceInterrupts(service, {io, 2, 5}).ok());
    CHECK(service.fcscrRequested && service.fcscr.sampledSum == 255 && service.fcscr.counterLowerBound);
    CHECK(service.recrRequested && service.recr.raw == 0xFAC7 && service.recr.sampledSum == 0);
    CHECK(bus.words[12] == 0x815A);
    const auto reg12 = EvidenceSource::REGISTER_12;
    bus.releaseCall = static_cast<int>(bus.calls);
    CHECK(driver.acquireEvidence(evidenceBit(reg12), evidence, io).is(Err::MDIO_ERROR));
    const auto latched = driver.evidenceSnapshot().samples[static_cast<unsigned>(reg12)];
    CHECK(latched.raw == 0x815A && latched.eventBits == 0x8100 && bus.words[12] == 0x005A);
    bus.releaseCall = -1;
    bus.mmd3[1] = 0x0F40;
    bus.mmd3[22] = 0xFFFF;
    auto mask = evidenceBit(EvidenceSource::EEE_STATUS) | evidenceBit(EvidenceSource::EEE_ERRORS);
    CHECK(driver.acquireEvidence(mask, evidence, {1000000,8}).ok());
    CHECK(evidence.samples[static_cast<unsigned>(EvidenceSource::EEE_STATUS)].eventBits == 0x0C00);
    CHECK(evidence.samples[static_cast<unsigned>(EvidenceSource::EEE_ERRORS)].counterLowerBound);
    CHECK(bus.mmd3[1] == 0x0340 && bus.mmd3[22] == 0);
    const auto calls=bus.calls, clocks=bus.clocks;
    uint16_t word=0xBEEF;
    for (unsigned reg : std::array<unsigned,13>{1,6,12,13,14,15,16,18,19,20,21,23,26}) {
      CHECK(!driver.readClause22(reg,word,io).ok() && !driver.readMmd(31,reg,word,io).ok());
    }
    for (unsigned reg : std::array<unsigned,10>{0x4A1,0x218,0x30E,0x333,0x31B,0x33A,0x33D,0x4A5,0x4A6,0x4A7}) {
      CHECK(!driver.readMmd(31,reg,word,io).ok());
    }
    CHECK(!driver.readMmd(3,1,word,io).ok() && !driver.readMmd(3,22,word,io).ok());
    CHECK(bus.calls==calls && bus.clocks==clocks && word==0xBEEF);
    for (unsigned reg : std::array<unsigned,2>{0,20}) CHECK(driver.readMmd(3,reg,word,io).ok());
    for (unsigned reg : std::array<unsigned,2>{60,61}) CHECK(driver.readMmd(7,reg,word,io).ok());
    CounterSnapshot counters;
    bus.words[15] = 0x01F0; bus.words[21]=0x8231;
    CHECK(driver.collectEvents(counters,io).ok() && counters.fldCauses==0x01F0 && counters.rawRecr==0x8231);
  }
}

void p25ClockOutputAndIrq(bool follower, bool irqFirst) {
  Controller bus;
  Driver driver;
  p25Board(bus,follower,follower,follower);
  bus.clockLossApproved=true;
  ready(driver,bus);
  auto interface=settings();
  interface.rmiiClock=bus.boardRole;
  interface.referenceClockMHz=follower ? 50 : 25;
  // 0302[6] selects CLKOUT25 availability. The pin 2 mux is independent.
  for (auto route : {ClockOutput::CLKOUT25, ClockOutput::LED1}) {
    interface.clockOutput=route;
    InterfaceResult result;
    CHECK(driver.configureInterface(interface,result,{1000000,256}).ok());
    CHECK((result.interfaceState.raw[4]&64U)==(route==ClockOutput::LED1?64U:0U));
    CHECK((result.interfaceState.raw[7]&1U)==(route==ClockOutput::LED1?1U:0U));
  }
  InterruptSettings irq;
  irq.pin=InterruptPin::LED2;
  irq.globalEnable=irq.pinAvailable=irq.maintenanceAdmitted=true;
  EeeSettings eee;
  eee.enabled=eee.maintenanceAdmitted=true;
  OperationToken token;
  for (bool first : {true,false}) {
    const auto status=first==irqFirst ? driver.startConfigureInterrupts(irq,budget(),token) :
                                        driver.startConfigureEee(eee,budget(),token);
    CHECK(status.inProgress());
    CHECK(drain(driver,bus,token).ok() && take(driver,token).persistentRestored);
  }
  CHECK(driver.startInitialize({ResetKind::HARD},budget(),token).inProgress());
  CHECK(drain(driver,bus,token).ok() && take(driver,token).persistentRestored);
  CHECK((bus.ext[0x305]&7U)==4 && (bus.ext[0x302]&64U)==64 && bus.mmd7[60]==2);
  CHECK(driver.interruptSnapshot().trust==ConfigurationTrust::VERIFIED);
  CHECK(driver.interfaceSnapshot().trust==ConfigurationTrust::VERIFIED);
}

void p25Outputs() {
  for (bool follower : {false, true}) {
    Controller bus;
    Driver driver;
    p25Board(bus,follower,follower,follower);
    bus.clockLossApproved=true;
    ready(driver,bus);
    p25ClockOutputAndIrq(follower,false);
    p25ClockOutputAndIrq(follower,true);
    // RCSR[5] is reserved on this RMII-only PHY. Test a zero readback.
    bus.words[23] &= 0xFFDF;
    PinResult result;
    PinSettings pins;
    pins.maintenanceAdmitted=pins.macStopped=pins.outputsQuiesced=true;
    pins.availableMask=5;
    pins.pins[0].function=PinFunction::LED;
    // LED0 source plus drive: 12 frames / four callbacks, cost 80 us.
    const auto beforePins=bus.frames;
    result.attemptedFrames=999;
    bus.delayedClock=bus.clocks+2;
    CHECK(driver.configurePins(pins,result,{bus.now+321U,12}).is(Err::OPERATION_TIMEOUT));
    CHECK(bus.frames==beforePins && result.attemptedFrames==999 && !driver.pinsSnapshot().desiredValid);
    pins.pins[0].function=pins.pins[2].function=PinFunction::LED;
    pins.pins[2].drive=LedDrive::HIGH;
    pins.pins[2].polarity=LedPolarity::ACTIVE_HIGH;
    pins.stretchOwned=true;
    bus.words[25]|=0x20;
    for (unsigned code=0;code<15;++code) {
      pins.pins[0].source=pins.pins[2].source=static_cast<LedSource>(code);
      CHECK(driver.configurePins(pins,result,{1000000,256}).ok() && result.verified);
      CHECK((bus.ext[0x25]&0x79)==(1U|(code<<3)) && (bus.ext[0x460]&0xF0)==(code<<4));
      CHECK((bus.ext[0x469]&0x70)==0x70 && (bus.ext[0x305]&7)==1 && (bus.words[25]&0x20));
    }
    const auto calls=bus.calls;
    pins.pins[2].source=static_cast<LedSource>(15);
    CHECK(!driver.configurePins(pins,result,{1000000,256}).ok() && bus.calls==calls);
    pins.pins[2]={};
    for (auto item : {PinFunction::LOW3,PinFunction::LOW5,PinFunction::HIGH,PinFunction::LOW7}) {
      pins.pins[2].function=item;
      CHECK(driver.configurePins(pins,result,{1000000,256}).ok());
      const unsigned expected=item==PinFunction::LOW3?3:item==PinFunction::LOW5?5:item==PinFunction::HIGH?6:7;
      CHECK((bus.ext[0x305]&7)==expected);
    }
    pins.pins[2]={};
    CHECK(driver.configurePins(pins,result,{1000000,256}).ok());
    InterruptSettings irq;
    irq.pin=InterruptPin::LED2; irq.globalEnable=irq.pinAvailable=irq.maintenanceAdmitted=true;
    irq.misr1Mask=0xFF; irq.misr2Mask=0xFF;
    OperationToken token;
    CHECK(driver.startConfigureInterrupts(irq,budget(),token).inProgress());
    CHECK(drain(driver,bus,token).ok()); take(driver,token);
    CHECK((bus.ext[0x305]&7)==4 && (bus.words[17]&11)==11);
    CHECK((bus.words[18]&255)==255 && (bus.words[19]&255)==255);
    CHECK(driver.startInterruptTest(budget(),token).inProgress() && drain(driver,bus,token).ok());
    take(driver,token); CHECK(bus.words[17]&4);
    CHECK(driver.stopInterruptTest(budget(),token).inProgress() && drain(driver,bus,token).ok());
    take(driver,token); CHECK(!(bus.words[17]&4));
    WakeSettings wake;
    wake.maintenanceAdmitted=wake.receiveTrafficQuiesced=wake.pinAvailable=true;
    wake.enabled=wake.secureOn=true; wake.route=WakeRoute::INTERRUPT;
    uint8_t pass[]={0x3C,0x41,0x9D,0x44,0xBB,0x5E};
    for (unsigned i=0;i<6;++i) { wake.mac[i]=static_cast<uint8_t>(0x11*(i+1)); wake.password[i]=pass[i]; }
    CHECK(driver.startConfigureWake(wake,budget(),token).inProgress());
    CHECK(drain(driver,bus,token).ok()); take(driver,token);
    CHECK(bus.ext[0x4A2]==0x2211 && bus.ext[0x4A3]==0x4433 && bus.ext[0x4A4]==0x6655);
    CHECK(bus.ext[0x4A5]==0x413C && bus.ext[0x4A6]==0x449D && bus.ext[0x4A7]==0x5EBB);
    CHECK((bus.ext[0x4A0]&0x17A1)==0x11A1);
    bus.ext[0x4A1]|=0xE1; bus.words[19]|=0x0200;
    CHECK(driver.acknowledgeWake(budget(),token).inProgress() && drain(driver,bus,token).ok()); take(driver,token);
    CHECK((bus.ext[0x4A0]&0x0800)==0);
    const auto evidence=driver.evidenceSnapshot();
    CHECK(evidence.samples[static_cast<unsigned>(EvidenceSource::RXFS)].eventBits==0xE1);
    CHECK(evidence.samples[static_cast<unsigned>(EvidenceSource::MISR2)].wakeSeen);
    // Plain Magic Packet rekey, observation and HARD replay never access the password.
    const auto first=bus.frames;
    wake.secureOn=false;
    wake.mac[0]=0xEE;
    CHECK(driver.startConfigureWake(wake,budget(),token).inProgress());
    CHECK(drain(driver,bus,token).ok()); take(driver,token);
    WakeSnapshot observed;
    CHECK(driver.readWakeStatus(observed,{1000000,21}).ok() && observed.credentialValidMask==7);
    CHECK(driver.startInitialize({ResetKind::HARD},budget(),token).inProgress());
    CHECK(drain(driver,bus,token).ok() && take(driver,token).persistentRestored);
    for (unsigned i=first;i<bus.frames;++i) {
      CHECK(bus.trace[i].target<0x4A5 || bus.trace[i].target>0x4A7);
    }
    bus.ext[0x467]^=8;
    uint16_t strap=0;
    bus.releaseCall=static_cast<int>(bus.calls);
    CHECK(!driver.readMmd(31,0x467,strap,{1000000,4}).ok());
    CHECK(driver.interruptSnapshot().trust==ConfigurationTrust::DIRTY &&
          driver.wakeSnapshot().trust==ConfigurationTrust::DIRTY);
  }
  const uint16_t coarse[]={0x00E0,0x0DC0,0x0D80,0x0D40,0x0D00,0x0CC0,0x0C80,0x0C40,0x0C00,
                           0x0BC0,0x0B80,0x0B40,0x0B00,0x0AC0,0x0A80,0x0A40,0x0A00};
  Controller bus;
  Driver driver;
  p25Board(bus,false,false,false); ready(driver,bus);
  bus.ext[0x333]=0x520B;
  AnalogSettings analog;
  analog.vodOwned=analog.impedanceOwned=analog.fixtureAdmitted=analog.maintenanceAdmitted=analog.macStopped=true;
  OperationToken token;
  for(unsigned i=0;i<17;++i) {
    analog.vodCoarseStep=static_cast<uint8_t>(i); analog.vodFineStep=static_cast<int8_t>(static_cast<int>(i%5)-2);
    analog.impedance25Ohm=(i%2)==1;
    CHECK(driver.startConfigureAnalog(analog,budget(),token).inProgress() && drain(driver,bus,token).ok());
    take(driver,token);
    constexpr uint16_t trim[]={0x4800,0x5800,0x6800,0x7800,0x8800};
    CHECK(bus.ext[0x30B]==coarse[i] && bus.ext[0x30C]==8+i && bus.ext[0x30E]==trim[i%5]);
    CHECK((bus.ext[0x302]&0xC000)==(analog.impedance25Ohm?0x4000:0));
  }
  for(bool upper : {false,true}) {
    bus.ext[0x333]=upper?0x07C0:0xF800;
    analog.vodFineStep=0;
    CHECK(driver.startConfigureAnalog(analog,budget(),token).inProgress() && drain(driver,bus,token).ok()); take(driver,token);
    CHECK(bus.ext[0x30E]==(upper?0xF800:0x0800));
    analog.vodFineStep=upper?1:-1; const auto mutations=bus.mutations;
    CHECK(driver.startConfigureAnalog(analog,budget(),token).inProgress());
    CHECK(drain(driver,bus,token).is(Err::INVALID_CONFIG)); take(driver,token);
    CHECK(bus.mutations==mutations);
  }
}
