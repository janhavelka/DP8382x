// SNLS638C 6.3.14..16 and tables 7-25/30/31/32/40..59.
// The fake checks controls, lifetime and restoration. Packet and electrical
// observations on an identified RMII board belong to physical qualification.
void p25LoopbackVectors(bool follower) {
  struct Vector { LoopbackMode mode; LinkSpeed speed; uint16_t biscr, bmcr; };
  const Vector vectors[] = {
    {LoopbackMode::MII, LinkSpeed::MBPS_10, 1, 0x4100},
    {LoopbackMode::MII, LinkSpeed::MBPS_100, 0x44, 0x6100},
    {LoopbackMode::PCS_INPUT, LinkSpeed::MBPS_10, 1, 0x0100},
    {LoopbackMode::PCS_OUTPUT, LinkSpeed::MBPS_10, 2, 0x0100},
    {LoopbackMode::PCS_OUTPUT, LinkSpeed::MBPS_100, 2, 0x2100},
    {LoopbackMode::DIGITAL, LinkSpeed::MBPS_100, 4, 0x2100},
    {LoopbackMode::ANALOG, LinkSpeed::MBPS_10, 8, 0x0100},
    {LoopbackMode::ANALOG, LinkSpeed::MBPS_100, 8, 0x2100},
    {LoopbackMode::REVERSE, LinkSpeed::MBPS_100, 16, 0x2100},
    {LoopbackMode::EXTERNAL, LinkSpeed::MBPS_100, 0, 0x1000}
  };
  for (const auto& entry : vectors) {
    Controller bus;
    p25Board(bus, follower, follower, follower);
    Driver driver;
    ready(driver, bus);
    LoopbackOptions options;
    options.mode = entry.mode;
    options.speed = entry.speed;
    options.maintenanceAdmitted = options.partnerReady = options.externalFixture = true;
    options.transmitOnLine = (entry.biscr & 0x40U) != 0;
    OperationToken token{0xBEEF};
    const auto start = bus.frames;
    if (entry.mode == LoopbackMode::ANALOG || entry.mode == LoopbackMode::REVERSE) {
      CHECK(driver.startLoopback(options, budget(), token).is(Err::INVALID_CONFIG));
      CHECK(bus.frames == start && token.value == 0xBEEF);
    }
    options.termination100Ohm = true;
    options.reverseFifoDepth = entry.mode == LoopbackMode::REVERSE ? 3 : 0;
    CHECK(driver.startLoopback(options, budget(), token).inProgress());
    CHECK(drain(driver, bus, token).ok());
    const auto on = take(driver, token);
    CHECK(on.loopback.controlsVerified && !on.loopback.hardResetExit && on.cleanupOwed);
    CHECK((bus.words[0] & 0x7100U) == entry.bmcr);
    CHECK((bus.words[22] & 0x705FU) == entry.biscr);
    if (entry.mode == LoopbackMode::REVERSE) CHECK((bus.words[17] & 0x300U) == 0x300U);
    for (unsigned i=start; i<bus.frames; ++i) {
      if (semantic(bus.trace[i]) && bus.trace[i].target==0x16) CHECK((bus.trace[i].value & ~0x705FU)==0);
    }
    CHECK(driver.stopLoopback(budget(), token).inProgress() && drain(driver, bus, token).ok());
    CHECK(take(driver, token).persistentRestored && !driver.operationSnapshot().temporaryMayBeActive);
    CHECK((bus.words[0] & 0x5000U) == 0x1000 && (bus.words[22] & 0x705FU) == 0);
    for (uint16_t reg : std::array<uint16_t,4>{0x0122, 0x0123, 0x0130, 0x001F}) CHECK(writes(bus, start, reg) == 0);
  }
}

void p25BistVectors(bool follower) {
  for (unsigned code=0; code<4; ++code) for (bool wrap : {false,true}) {
    Controller bus;
    p25Board(bus, follower, follower, follower);
    Driver driver;
    ready(driver, bus);
    bus.words[27] = 0x25;
    bus.words[28] = 0xFDFE;
    BistOptions request;
    request.mode = static_cast<BistMode>(code);
    request.counterMode = wrap ? BistCounterMode::WRAP : BistCounterMode::SATURATE;
    request.loopback.maintenanceAdmitted = true;
    request.useLoopback = code == 3;
    request.loopback.mode = LoopbackMode::ANALOG;
    request.loopback.termination100Ohm = true;
    request.packetBytes = 2047;
    request.interPacketGapBytes = 1020;
    request.observationUs = 1000000;
    OperationToken token;
    const auto allowance = budget(2048, 5000000);
    CHECK(driver.startBist(request, allowance, token).inProgress() && drain(driver, bus, token).ok());
    CHECK(take(driver, token).cleanupOwed);
    const uint16_t control = static_cast<uint16_t>((code<<12U) | (wrap?0x4000U:0U) | (code==3?8U:0U));
    CHECK((bus.words[22] & 0x705FU) == control && bus.words[27] == 255 && bus.words[28] == 0xFFFF);
    bus.bistErrors=0x7C;
    CHECK(driver.startBistAcquire({}, allowance, token).inProgress() && drain(driver, bus, token).ok());
    const auto result=take(driver, token);
    CHECK(result.bist.counterValid && result.bist.errorBytes==0x7C && result.bist.wrapPossible==wrap);
    CHECK(result.bist.rawCounter == 0x7CFF && !result.bist.historyLost && bus.bistErrors==0);
    // Freeze/clear succeeded, the count read failed. The previous epoch cannot
    // fill this hole, even if the application retries acquisition.
    bus.bistErrors=2;
    CHECK(driver.startBistAcquire({}, allowance, token).inProgress());
    CHECK(poll(driver,bus,token).inProgress());
    bus.failFrame=static_cast<int>(bus.frames+1U);
    CHECK(poll(driver,bus,token).is(Err::MDIO_TIMEOUT));
    const auto partial=take(driver,token);
    CHECK(partial.bist.historyLost && !partial.bist.counterValid && bus.bistErrors==0);
    bus.failFrame=-1;
    CHECK(driver.stopBist(allowance, token).inProgress() && drain(driver, bus, token).ok());
    CHECK(take(driver, token).persistentRestored && bus.words[27]==0x25 && bus.words[28]==0xFDFE);
  }
}

void p25CableVectors(bool follower) {
  for (unsigned average=0; average<=6; ++average) for (uint8_t segment=1; segment<=5; ++segment) {
    Controller bus;
    p25Board(bus, follower, follower, follower);
    Driver driver;
    ready(driver, bus);
    bus.words[16] = 0;
    bus.words[30] = 0x0102;
    bus.ext[0x180] = 0xA5;
    bus.ext[0x185] = 0x71;
    bus.ext[0x18A] = 0x0820;
    bus.cableFails=(average==6);
    CableSettings request;
    request.maintenanceAdmitted=request.peerQuiet=request.allowHardResetExit=true;
    OperationToken token;
    const auto start=bus.frames;
    CHECK(driver.startCableTest(request, budget(), token).is(Err::UNSUPPORTED) && bus.frames==start);
    request.recipe=CableRecipe::RAW;
    request.runMask=1;
    const auto channel=average%4U;
    request.controls.channel=static_cast<CableChannel>(channel);
    request.controls.averaging=static_cast<uint8_t>(average);
    request.controls.segment=segment;
    request.controls.shortThreshold=31;
    request.controls.locationThreshold=19;
    request.controls.pulseWidth=5;
    CHECK(driver.startCableTest(request, budget(), token).inProgress() && drain(driver, bus, token).ok());
    const auto result=take(driver, token);
    const auto& first=result.cable.samples[0];
    CHECK(result.cable.complete && first.triggerAccepted && first.freshnessObserved && first.failed==bus.cableFails);
    CHECK(first.index.valid && first.index.raw==0xA5 && first.amplitude.raw==0x71 && first.peak.raw==0x0820);
    CHECK(bus.ext[0x170]==uint16_t((channel<<13U) | 0x1802U | (average<<8U) | (uint32_t{segment}<<4U)));
    CHECK(bus.ext[0x173]==0x0D07 && bus.ext[0x175]==0x1004 && bus.ext[0x176]==0x13 &&
          bus.ext[0x177]==0x1F00 && bus.ext[0x178]==5);
    CHECK(writes(bus,start,0x0411)==0 && writes(bus,start,0x0416)==0 && writes(bus,start,0x0456)==0);
    for (unsigned i=start; i<bus.frames; ++i) {
      if (semantic(bus.trace[i]) && bus.trace[i].target==30) CHECK(bus.trace[i].value==0x8000);
    }
    CHECK(driver.stopCableTest(budget(), token).inProgress() && drain(driver,bus,token).ok());
    CHECK(take(driver,token).persistentRestored && bus.ext[0x170]==0x0C12);
  }
}

void p25MseVectors(bool follower) {
  Controller bus;
  p25Board(bus, follower, follower, follower);
  Driver driver;
  ready(driver, bus);
  SignalQualitySnapshot out;
  out.rawMse=0xBEEF;
  auto start=bus.frames;
  CHECK(driver.readSignalQuality(out,{1000000,4}).is(Err::RESULT_NOT_AVAILABLE));
  CHECK(out.rawMse==0xBEEF && bus.frames==start);
  LinkSnapshot link;
  CHECK(driver.readLinkStatus(link,{1000000,32}).ok() && link.linkUp);
  struct Vector { uint16_t value; MseQuality quality; };
  const Vector vectors[]={{0x209,MseQuality::EXCELLENT},{0x20A,MseQuality::UNCLASSIFIED},
    {0x20B,MseQuality::GOOD},{0x33A,MseQuality::GOOD},{0x33B,MseQuality::UNCLASSIFIED},{0x33C,MseQuality::POOR}};
  for (const auto& point:vectors) {
    bus.ext[0x218]=point.value;
    start=bus.frames;
    CHECK(driver.readSignalQuality(out,{1000000,4}).ok() && out.rawMse==point.value && out.quality==point.quality);
    CHECK(bus.frames==start+4 && bus.trace[start].value==31 && bus.trace[start+1].value==0x218 &&
          bus.trace[start+2].value==0x401F && bus.trace[start+3].kind==FrameKind::READ);
  }
  for (unsigned cut=0; cut<4; ++cut) {
    bus.failFrame=static_cast<int>(bus.frames+cut);
    CHECK(driver.readSignalQuality(out,{1000000,4}).is(Err::MDIO_TIMEOUT));
    CHECK(out.rawMse==0x33C && driver.signalQualitySnapshot().stale);
  }
  bus.failFrame=-1;
  bus.words[1]=0;
  bus.words[16]=0;
  CHECK(driver.readLinkStatus(link,{1000000,32}).ok() && !link.linkUp);
  start=bus.frames;
  CHECK(driver.readSignalQuality(out,{1000000,4}).is(Err::RESULT_NOT_AVAILABLE) && bus.frames==start);
}

void p25CableFaults(bool follower) {
  unsigned frames=0, calls=0, firstIndex=0, firstAmplitude=0, firstPeak=0;
  for (unsigned faultType=0; faultType<3; ++faultType) {
    const unsigned count=faultType==0?1U:faultType==1?frames:calls;
    for (unsigned cut=0;cut<count;++cut) {
      Controller bus;
      p25Board(bus,follower,follower,follower);
      Driver driver;
      ready(driver,bus);
      bus.words[16]=0;
      bus.ext[0x180]=0xCE; bus.ext[0x185]=0x71; bus.ext[0x18A]=0x0820;
      const auto start=bus.frames, startCall=bus.calls;
      CableSettings options;
      options.recipe=CableRecipe::RAW; options.runMask=1;
      options.maintenanceAdmitted=options.peerQuiet=options.allowHardResetExit=true;
      if (faultType==1) bus.failFrame=static_cast<int>(start+cut);
      if (faultType==2) bus.releaseCall=static_cast<int>(startCall+cut);
      OperationToken token;
      CHECK(driver.startCableTest(options,budget(),token).inProgress());
      const auto status=drain(driver,bus,token);
      const auto result=take(driver,token);
      if (faultType==0) {
        CHECK(status.ok());
        frames=bus.frames-start; calls=bus.calls-startCall;
        for(unsigned frame=start;frame<bus.frames;++frame) {
          const auto& access=bus.trace[frame];
          if(access.kind!=FrameKind::READ) continue;
          if(access.target==0x180) firstIndex=frame-start+1U;
          if(access.target==0x185) firstAmplitude=frame-start+1U;
          if(access.target==0x18A) firstPeak=frame-start+1U;
        }
        CHECK(firstIndex<firstAmplitude && firstAmplitude<firstPeak && firstPeak<frames);
        continue;
      }
      CHECK(status.is(faultType==1?Err::MDIO_TIMEOUT:Err::MDIO_ERROR) && !result.cable.complete);
      const unsigned completed=bus.frames-start-(faultType==1?1U:0U);
      const auto& sample=result.cable.samples[0];
      CHECK(sample.index.valid==(completed>=firstIndex));
      CHECK(sample.amplitude.valid==(completed>=firstAmplitude));
      CHECK(sample.peak.valid==(completed>=firstPeak));
      if(sample.index.valid) CHECK(sample.index.raw==0xCE);
      if(sample.amplitude.valid) CHECK(sample.amplitude.raw==0x71);
      if(sample.peak.valid) CHECK(sample.peak.raw==0x0820);
      if(faultType==1 && result.temporaryMayBeActive) {
        bus.failFrame=-1;
        CHECK(driver.stopCableTest(budget(),token).inProgress() && drain(driver,bus,token).ok());
        CHECK(take(driver,token).persistentRestored);
      }
    }
  }
  Controller bus;
  p25Board(bus,follower,follower,follower);
  Driver driver;
  ready(driver,bus);
  OperationToken token;
  CHECK(driver.startConfigureCableAutorun({true,true},budget(),token).inProgress() && drain(driver,bus,token).ok());
  CHECK(take(driver,token).persistentRestored && (bus.words[9]&0x0100U));
  bus.words[30]=3; bus.ext[0x180]=8; bus.ext[0x185]=11; bus.ext[0x18A]=32;
  const auto start=bus.frames;
  CHECK(driver.startCableAutorunAcquire(budget(),token).inProgress() && drain(driver,bus,token).ok());
  const auto raw=take(driver,token).cable;
  CHECK(raw.autorun && raw.complete && raw.stale && raw.samples[0].failed && !raw.samples[0].triggerAccepted);
  CHECK(raw.samples[0].index.raw==8 && raw.samples[0].amplitude.raw==11 && raw.samples[0].peak.raw==32);
  CHECK(writes(bus,start,30)==0);
}

void p25ElectricalVectors(bool follower) {
  for (const auto mode : {ComplianceMode::TX100, ComplianceMode::NLP10, ComplianceMode::PRBS10,
                          ComplianceMode::PATTERN10, ComplianceMode::COUNT100}) {
    const unsigned last=mode==ComplianceMode::COUNT100 ? 31U : mode==ComplianceMode::PATTERN10 ? 13U : 0U;
    for (unsigned code=0; code<=last; ++code) {
      Controller bus;
      p25Board(bus,follower,follower,follower);
      Driver driver;
      ready(driver,bus);
      ComplianceOptions request;
      request.mode=mode;
      request.pattern=static_cast<uint8_t>(code);
      request.maintenanceAdmitted=request.macStopped=request.fixtureAdmitted=request.hardResetAdmitted=true;
      request.scramblerBypass=mode==ComplianceMode::TX100 || (mode==ComplianceMode::COUNT100 && (code&1U));
      OperationToken token;
      const auto first=bus.frames;
      CHECK(driver.startCompliance(request,budget(),token).inProgress());
      CHECK(drain(driver,bus,token).ok());
      const auto active=take(driver,token);
      CHECK(active.electrical.controlsVerified && active.temporaryMayBeActive);
      const bool highSpeed=mode==ComplianceMode::TX100 || mode==ComplianceMode::COUNT100;
      CHECK((bus.words[0]&0xFF80U)==(highSpeed?0x2100U:0x0100U));
      CHECK(bool(bus.words[17]&0x800U)==request.scramblerBypass);
      CHECK((bus.words[25]&31U)==1);
      if(mode==ComplianceMode::COUNT100) {
        unsigned step=0;
        const uint16_t high=code>=16U?0x20:0;
        const std::array<uint16_t,3> addresses{0x428,0x27,0x428};
        const std::array<uint16_t,3> values{high,static_cast<uint16_t>(code&15U),static_cast<uint16_t>(high|0x10U)};
        for (unsigned i=first;i<bus.frames;++i) {
          const auto& entry=bus.trace[i];
          if (!semantic(entry) || (entry.target!=0x428 && entry.target!=0x27)) continue;
          CHECK(step<3);
          if (step<3) { CHECK(entry.target==addresses[step] && entry.value==values[step]); }
          ++step;
        }
        CHECK(step==3);
      }
      if(mode==ComplianceMode::PATTERN10) CHECK((bus.ext[0x27]&0x1FU)==(0x10U|code));
      if(mode==ComplianceMode::PRBS10) CHECK((bus.words[22]&0x705FU)==0x7001);
      CHECK(driver.stopCompliance(budget(),token).inProgress() && drain(driver,bus,token).ok());
      CHECK(take(driver,token).electrical.stopped && bus.ext[0x428]==0 && (bus.words[17]&0x800U)==0U);
      CHECK(driver.startInitialize({ResetKind::NONE},budget(),token).inProgress() && drain(driver,bus,token).ok());
      CHECK(take(driver,token).persistentRestored);
    }
  }
  Controller bus;
  p25Board(bus,follower,follower,follower);
  Driver driver;
  ready(driver,bus);
  for (unsigned code=0;code<=8;++code) {
    QualitySettings settings;
    settings.maintenanceAdmitted=settings.macStopped=settings.tx10.owned=true;
    settings.tx10.lowerThreshold=settings.tx10.nlpDisable=settings.tx10.jabberDisable=true;
    settings.tx10.squelchCode=static_cast<uint8_t>(code);
    OperationToken token;
    CHECK(driver.startConfigureQuality(settings,budget(),token).inProgress() && drain(driver,bus,token).ok());
    CHECK(take(driver,token).quality.trust==ConfigurationTrust::VERIFIED);
    CHECK(bus.words[0x1A]==(0x2081U|(code<<9U)));
  }
  CollisionOptions collision;
  collision.maintenanceAdmitted=collision.macStopped=true;
  collision.speed=LinkSpeed::MBPS_100;
  OperationToken token{0xCAFE};
  const auto before=bus.calls;
  CHECK(driver.startCollisionTest(collision,budget(),token).is(Err::INVALID_CONFIG) && bus.calls==before);
  collision.registerOnly=true;
  CHECK(driver.startCollisionTest(collision,budget(),token).inProgress() && drain(driver,bus,token).ok());
  CHECK(take(driver,token).electrical.controlsVerified && (bus.words[0]&0x80U));
  CHECK(driver.stopCollisionTest(budget(),token).inProgress() && drain(driver,bus,token).ok());
  CHECK(take(driver,token).electrical.stopped && (bus.words[0]&0x80U)==0);
}

// Re-run from a fresh device for every transfer in the count ladder. A failed
// data write can take effect. Recovery uses a new budget and cannot depend on
// either the failed command's completion or the following readback.
void p25CountFaults(bool follower) {
  unsigned startOffset=0, endOffset=0, startCall=0, endCall=0;
  for (unsigned fault=0; fault<4; ++fault) {
    const bool perFrame=fault==1 || fault==2;
    const unsigned first=fault==0?0:perFrame?startOffset:startCall;
    const unsigned last=fault==0?1:perFrame?endOffset:endCall;
    for (unsigned cut=first;cut<last;++cut) {
      Controller bus;
      p25Board(bus,follower,follower,follower);
      Driver driver;
      ready(driver,bus);
      ComplianceOptions request;
      request.mode=ComplianceMode::COUNT100; request.pattern=31;
      request.maintenanceAdmitted=request.fixtureAdmitted=request.macStopped=request.hardResetAdmitted=true;
      const auto origin=bus.frames, originCall=bus.calls;
      if(perFrame) { bus.failFrame=static_cast<int>(origin+cut); bus.acceptedFailure=fault==2; }
      if(fault==3) bus.releaseCall=static_cast<int>(originCall+cut);
      OperationToken token;
      CHECK(driver.startCompliance(request,budget(),token).inProgress());
      const auto status=drain(driver,bus,token);
      const auto result=take(driver,token);
      if(fault==0) {
        CHECK(status.ok());
        for(unsigned i=origin;i<bus.frames;++i) {
          if(bus.trace[i].target==0x428 && bus.trace[i].kind==FrameKind::READ && !startOffset) startOffset=i-origin-3U;
        }
        endOffset=bus.frames-origin;
        for(unsigned i=originCall;i<bus.calls;++i) if(bus.firstFrame[i]==origin+startOffset) startCall=i-originCall;
        endCall=bus.calls-originCall;
        CHECK(startOffset && startCall && endOffset>startOffset && endCall>startCall);
        continue;
      }
      CHECK(status.is(perFrame?Err::MDIO_TIMEOUT:Err::MDIO_ERROR));
      CHECK(!result.electrical.controlsVerified && result.temporaryMayBeActive && result.cleanupOwed);
      CHECK((bus.ext[0x428]&4U)==0);
      if(perFrame) {
        bus.failFrame=-1;
        CHECK(driver.stopCompliance(budget(),token).inProgress() && drain(driver,bus,token).ok());
        CHECK(take(driver,token).electrical.stopped && bus.ext[0x428]==0 && bus.ext[0x27]==0);
        CHECK(driver.startInitialize({ResetKind::NONE},budget(),token).inProgress() && drain(driver,bus,token).ok());
        CHECK(take(driver,token).persistentRestored);
      }
    }
  }
}

void p25DspControls(bool follower) {
  // Independently transcribed documented subfields from SNLS638C pp67..70.
  // Use asymmetric reserved/unowned bits so a whole-word write is observable.
  constexpr std::array<uint16_t,7> address{0x308,0x30F,0x311,0x31C,0x404,0x31F,0x33E};
  constexpr std::array<uint16_t,7> masks{0xFCF0,0x00FF,0xE000,0xC07F,0x00E0,0xFFC0,0x0060};
  constexpr std::array<uint16_t,7> values{0x84A0,0x5A,0xA000,0x8043,0x0080,0xF800,0x0040};
  Controller bus;
  p25Board(bus,follower,follower,follower);
  Driver driver;
  ready(driver,bus);
  AnalogSettings settings;
  settings.maintenanceAdmitted=settings.macStopped=settings.fixtureAdmitted=true;
  OperationToken token;
  for (size_t i=0;i<address.size();++i) {
    bus.ext[address[i]]=0x0A12;
    for (unsigned bit=0;bit<16;++bit) {
      const auto candidate=static_cast<uint16_t>(1U<<bit);
      if (masks[i]&candidate) continue;
      settings.dspMasks[i]=candidate;
      const auto calls=bus.calls, clocks=bus.clocks;
      CHECK(!driver.startConfigureAnalog(settings,budget(),token).ok() && calls==bus.calls && clocks==bus.clocks);
    }
    settings.dspMasks[i]=masks[i];
    settings.dspValues[i]=values[i];
  }
  const auto firstCall=bus.calls;
  CHECK(driver.startConfigureAnalog(settings,budget(),token).inProgress());
  // Between snapshot and APPLY, a different owner changes the two non-owned
  // bits of 0308. Its live RMW must preserve both, including tied-high bit 1.
  const unsigned preflight=1;
  for (size_t i=0;i<address.size()+preflight;++i) CHECK(poll(driver,bus,token).inProgress());
  bus.ext[0x308]|=0x0102U;
  CHECK(drain(driver,bus,token).ok());
  auto result=take(driver,token);
  CHECK(result.analog.trust==ConfigurationTrust::VERIFIED && (result.analog.validMask&0xFE0U)==0xFE0);
  for (size_t i=0;i<address.size();++i) {
    const uint16_t other=i==0?0x0B12:0x0A12;
    const auto expected=static_cast<uint16_t>((other&~masks[i])|values[i]);
    CHECK(bus.ext[address[i]]==expected && result.analog.raw[5+i]==expected);
    const auto rowCall=firstCall+1U+static_cast<unsigned>(address.size()+2U*i);
    const auto frame=bus.firstFrame[rowCall];
    CHECK(bus.firstFrame[rowCall+1U]-frame==5);
    CHECK(bus.trace[frame+3].kind==FrameKind::READ && bus.trace[frame+4].kind==FrameKind::WRITE &&
          bus.trace[frame+4].target==address[i]);
  }
  const auto calls=bus.calls;
  CHECK(driver.startInitialize({ResetKind::HARD,FunctionalMode::POWER_OPTIMIZED},budget(),token).is(Err::INVALID_CONFIG));
  CHECK(driver.startInitialize({ResetKind::HARD,FunctionalMode::EXTENDED_REACH_EEE},budget(),token).is(Err::INVALID_CONFIG));
  CHECK(calls==bus.calls);
  settings.dspMasks[2]=0xA000; // Partial CAGC mapping.
  CHECK(!driver.startConfigureAnalog(settings,budget(),token).ok());
  settings.dspMasks=masks;
  settings.dspValues[2]=0xE000; // Fourth mapping is explicitly prohibited.
  CHECK(!driver.startConfigureAnalog(settings,budget(),token).ok());
  settings.dspValues=values;
  for (unsigned code=0;code<=7;++code) {
    settings.dspValues[4]=static_cast<uint16_t>(code<<5U);
    if (code!=0 && code!=4 && code!=7) {
      CHECK(!driver.startConfigureAnalog(settings,budget(),token).ok());
    } else {
      CHECK(driver.startConfigureAnalog(settings,budget(),token).inProgress() && drain(driver,bus,token).ok());
      CHECK(take(driver,token).analog.trust==ConfigurationTrust::VERIFIED && (bus.ext[0x404]&0xE0U)==code<<5U);
    }
  }
  CHECK(driver.startInitialize({ResetKind::HARD},budget(),token).inProgress() && drain(driver,bus,token).ok());
  CHECK(take(driver,token).persistentRestored && driver.analogSnapshot().trust==ConfigurationTrust::VERIFIED);
  for (size_t i=0;i<address.size();++i) CHECK((bus.ext[address[i]]&masks[i])==settings.dspValues[i]);
}

void p25DspFaults(bool follower) {
  for (unsigned cut=0; cut<6; ++cut) for (bool accepted : {false,true}) {
    Controller bus;
    p25Board(bus,follower,follower,follower);
    Driver driver;
    ready(driver,bus);
    AnalogSettings settings;
    settings.maintenanceAdmitted=settings.macStopped=settings.fixtureAdmitted=true;
    settings.dspMasks[1]=0x80;
    settings.dspValues[1]=0x80;
    bus.ext[0x030F]=0x0464;
    OperationToken token;
    CHECK(driver.startConfigureAnalog(settings,budget(),token).inProgress());
    CHECK(poll(driver,bus,token).inProgress() && poll(driver,bus,token).inProgress());
    // One shared-lock read/modify/write: failures at selector, offset, data
    // mode, data read, target write and post-write controller release.
    if(cut<5U) bus.failFrame=static_cast<int>(bus.frames+cut);
    else bus.releaseCall=static_cast<int>(bus.calls);
    bus.acceptedFailure=accepted;
    CHECK(!poll(driver,bus,token).ok());
    const auto failure=take(driver,token);
    CHECK(failure.status.is(cut<5U?Err::MDIO_TIMEOUT:Err::MDIO_ERROR));
    CHECK(failure.mutationCompleted==(cut==5U));
    CHECK(failure.mutationPossible==(cut==5U || (cut==4U && accepted)));
    CHECK(bus.ext[0x30F]==((cut==5U || (cut==4U && accepted))?0x04E4:0x0464));
    CHECK(failure.analog.trust==ConfigurationTrust::DIRTY && failure.analog.observedStale);
    if(cut<5U) {
      bus.failFrame=-1;
      CHECK(driver.startInitialize({ResetKind::NONE},budget(),token).inProgress() && drain(driver,bus,token).ok());
      CHECK(take(driver,token).persistentRestored && bus.ext[0x030F]==0x04E4);
    }
  }
}

void p25Diagnostics() {
  for (bool follower : {false,true}) {
    p25MseVectors(follower);
    p25LoopbackVectors(follower);
    p25BistVectors(follower);
    p25CableVectors(follower);
    p25ElectricalVectors(follower);
    p25CableFaults(follower);
    p25CountFaults(follower);
    p25DspControls(follower);
    p25DspFaults(follower);
  }
}
