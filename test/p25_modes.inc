// Independent wire transcription from SNLS638C tables 6-5 through 6-7.
// Selectors belong to one controller callback. The 001F restart is direct.
void p25Modes() {
  // Include identification, mode writes and the independently counted 24-frame AN replay.
  for (auto entry : {std::pair{FunctionalMode::DEFAULT,42U}, std::pair{FunctionalMode::EXTENDED_REACH,99U},
                     std::pair{FunctionalMode::EXTENDED_REACH_EEE,195U}}) {
    for (unsigned shortBy : {1U,0U}) {
      Controller bus;
      Driver driver;
      p25Board(bus,true,true,true);
      ready(driver,bus);
      OperationToken token;
      CHECK(driver.startInitialize({ResetKind::HARD,entry.first},budget(entry.second-shortBy),token).inProgress());
      CHECK(drain(driver,bus,token).ok()==(shortBy==0U));
      CHECK(take(driver,token).persistentRestored==(shortBy==0U));
      CHECK(bus.hard==(shortBy==0U?1U:0U));
    }
  }
  // A completed external PCS reset observation invalidates a pending dependent operation
  // even if the controller then fails to release. The vendor mode remains established.
  for (bool releaseFails : {false,true}) {
    Controller bus;
    Driver driver;
    p25Board(bus,true,true,true);
    ready(driver,bus);
    OperationToken token;
    CHECK(driver.startInitialize({ResetKind::HARD,FunctionalMode::EXTENDED_REACH},budget(),token).inProgress());
    CHECK(drain(driver,bus,token).ok()); take(driver,token);
    LinkSettings policy;
    policy.advertisement.technologyMask=2;
    CHECK(driver.startConfigure(policy,{},budget(),token).inProgress());
    CHECK(poll(driver,bus,token).inProgress());
    bus.mmd3[0]=0x8000;
    uint16_t word=0xCAFE;
    if (releaseFails) bus.releaseCall=static_cast<int>(bus.calls);
    const auto mutations=bus.mutations;
    CHECK(driver.readMmd(3,0,word,{1000000,4}).ok()!=releaseFails);
    CHECK(word==(releaseFails?0xCAFE:0x8000));
    CHECK(driver.operationSnapshot().state==OperationState::RESULT_PENDING);
    const auto frozen=take(driver,token);
    CHECK(!frozen.persistentRestored && frozen.status.is(Err::INVALID_CONFIG));
    CHECK(driver.functionalModeSnapshot().established && bus.mutations==mutations);
  }
  using Word = std::pair<uint16_t,uint16_t>;
  constexpr Word shortScript[] = {
    {0x416,0x1F30},{0x40D,13},{0x429,0x0200},{0x30B,0x0BC0},{0x30C,17},
    {0x33C,1},{0x311,0},{0x313,0x06E3},{0x33A,0x579C},{0x404,0x80},
    {0x33D,0x8110},{0x31B,0x0048},{31,0x4000}
  };
  constexpr Word longScript[] = {
    {0x416,0x1F30},{0x40D,13},{0x429,0x0200},{0x30B,0x0BC0},{0x30C,17},
    {0x33C,1},{0x311,0},{0x313,0x06E3},{0x33A,0x579C},{0x404,0x80},
    {0x130,0x4750},{0x123,0x0800},{0x30F,0x0400},{0x4D4,0x6633},
    {0x4D5,0x027F},{0x4D6,0x01B0},{0x4D7,0x01B0},{0x31F,0xFC36},
    {0x31C,0x1103},{0x101,0x0882},{0x10A,0x2010},{0x4CE,255},{0x4CD,0xA5A5},
    {0x308,0x0982},{0x4CF,0x231D},{0x4D0,0x0F8F},{0x33E,0x861E},{0x4D1,0x00C2},
    {0x4D2,0x215B},{0x33D,0x8110},{0x31B,0x0048},{31,0x4000}
  };
  for (const bool follower : {false,true}) {
    for (const auto mode : {FunctionalMode::DEFAULT,FunctionalMode::POWER_OPTIMIZED,
                            FunctionalMode::EXTENDED_REACH,FunctionalMode::EXTENDED_REACH_EEE}) {
      Controller bus;
      Driver driver;
      p25Board(bus,follower,follower,follower);
      bus.clockLossApproved = true;
      ready(driver,bus);
      const auto first = bus.frames;
      OperationToken token;
      CHECK(driver.startInitialize({ResetKind::HARD,mode},budget(),token).inProgress());
      CHECK(drain(driver,bus,token).ok());
      OperationResult result;
      FunctionalModeSnapshot snapshot;
      CHECK(driver.takeResult(token,result,snapshot).ok() && result.status.ok() && result.persistentRestored);
      const bool eee = mode == FunctionalMode::EXTENDED_REACH_EEE;
      const auto* script = eee ? longScript : shortScript;
      const unsigned count = mode == FunctionalMode::DEFAULT ? 0U : eee ? 32U : 13U;
      CHECK(snapshot.desired == mode && snapshot.established && snapshot.completedWrites == count);
      CHECK(snapshot.restartCleared == (count > 0) && !snapshot.nextWritePossible);
      unsigned row=0, start=first;
      while (start<bus.frames && !(semantic(bus.trace[start]) && bus.trace[start].target==0x416)) { ++start; }
      for (unsigned i=start; i<bus.frames && row<count; ++i) {
        if (!semantic(bus.trace[i])) { continue; }
        const auto expected=script[row++];
        const auto& actual=bus.trace[i];
        CHECK(actual.target==expected.first && actual.devad==31);
        CHECK(actual.reg==(expected.first==31?31:14));
        CHECK(actual.value==(mode==FunctionalMode::POWER_OPTIMIZED && expected.first==0x404 ? 0 : expected.second));
      }
      CHECK(row==count);
      if (eee) {
        CHECK(bus.mmd7[0x3C]==2 && bus.ext[0x4D1]==0x00C2);
        CHECK(driver.eeeSnapshot().trust==ConfigurationTrust::VERIFIED);
        unsigned advertisement=0, restart=0;
        for (unsigned i=start; i<bus.frames; ++i) {
          const auto& w=bus.trace[i];
          if (!semantic(w)) { continue; }
          if (w.devad==7 && w.target==0x3C && w.value==2) { advertisement=i; }
          if (w.devad==31 && w.target==0 && (w.value&0x0200U)!=0) { restart=i; }
        }
        CHECK(advertisement>start && restart>advertisement);
      }
      const unsigned beforeReplay=bus.frames;
      CHECK(driver.startInitialize({ResetKind::HARD},budget(),token).inProgress());
      CHECK(drain(driver,bus,token).ok() && take(driver,token).persistentRestored);
      CHECK(writes(bus,beforeReplay,0x416)==(count ? 1U : 0U));
      CHECK(driver.startInitialize({ResetKind::HARD,FunctionalMode::DEFAULT},budget(),token).inProgress());
      CHECK(drain(driver,bus,token).ok() && take(driver,token).persistentRestored);
      CHECK(driver.functionalModeSnapshot().established && driver.functionalModeSnapshot().completedWrites==0);
      CHECK(bus.ext[0x416]==0 && bus.mmd7[0x3C]==0);
    }
  }
  // Fail each physical selector or target in the longest script, including
  // an accepted-but-unconfirmed final restart. Replay from a fresh HARD.
  std::array<unsigned,32> targets{};
  unsigned base=0, end=0;
  for (int cut=-1; cut<static_cast<int>(end); ++cut) {
    for (const bool effect : {false,true}) {
      Controller bus;
      Driver driver;
      p25Board(bus,true,true,true);
      ready(driver,bus);
      const auto first=bus.frames;
      if (cut>=0) { bus.failFrame=int(first+base)+cut; bus.acceptedFailure=effect; }
      OperationToken token;
      CHECK(driver.startInitialize({ResetKind::HARD,FunctionalMode::EXTENDED_REACH_EEE},budget(),token).inProgress());
      const auto status=drain(driver,bus,token);
      OperationResult result;
      FunctionalModeSnapshot snapshot;
      CHECK(driver.takeResult(token,result,snapshot).ok());
      if (cut<0) {
        CHECK(status.ok());
        unsigned count=0;
        for (unsigned i=first; i<bus.frames && count<32; ++i) {
          if (!semantic(bus.trace[i]) || (!count && bus.trace[i].target!=0x416)) { continue; }
          if (!count) { base=i-first-3; }
          targets[count++]=i-first-base;
        }
        CHECK(count==32);
        end=targets[31]+1;
      } else {
        CHECK(status.is(Err::MDIO_TIMEOUT) && result.cleanupOwed && !snapshot.established);
        unsigned completed=0;
        while (completed<32 && targets[completed]<static_cast<unsigned>(cut)) { ++completed; }
        CHECK(snapshot.completedWrites==completed);
        CHECK(snapshot.nextWritePossible==(effect && completed<32 && targets[completed]==static_cast<unsigned>(cut)));
        CHECK(!snapshot.restartCleared);
        if (static_cast<unsigned>(cut)==targets[31]) {
          CHECK(result.eee.recipeRestartAttempted && !result.eee.recipeRestartAccepted &&
                result.eee.recipeRestartPossible==effect);
          bus.failFrame=-1;
          CHECK(driver.startInitialize({ResetKind::HARD},budget(),token).inProgress());
          CHECK(drain(driver,bus,token).ok() && take(driver,token).persistentRestored);
          CHECK(driver.functionalModeSnapshot().established && !driver.operationSnapshot().cleanupOwed);
        }
      }
    }
  }
  // Table 7-98: PCS reset is the write to IEEE 3:0000, not the
  // C22 BMCR command. MMD3/MMD7 reset, while vendor table state persists.
  for (const bool follower : {false,true}) {
    Controller bus;
    Driver phy;
    p25Board(bus,follower,follower,follower);
    bus.clockLossApproved=true;
    ready(phy,bus);
    OperationToken token;
    const InitializeOptions mode{ResetKind::HARD,FunctionalMode::EXTENDED_REACH_EEE,PcsRxClock::STOPPABLE};
    CHECK(phy.startInitialize(mode,budget(),token).inProgress());
    CHECK(drain(phy,bus,token).ok() && take(phy,token).persistentRestored);
    CHECK(bus.mmd3[0]==0x0400 && bus.mmd7[0x3C]==2);
    const auto vendor=bus.ext;
    bus.mmd3[0x16]=0xA55A; bus.mmd7[0x3D]=2;
    const auto first=bus.frames;
    CHECK(phy.startInitialize({ResetKind::EEE_PCS},budget(),token).inProgress());
    CHECK(drain(phy,bus,token).ok());
    const auto result=take(phy,token);
    CHECK(result.reset==ResetKind::EEE_PCS && result.commandAccepted && result.protocolComplete && result.persistentRestored);
    CHECK(result.eee.pcsControlValid && result.eee.pcsControl==0x0400);
    CHECK(result.eee.rxClock==PcsRxClock::STOPPABLE && result.eee.trust==ConfigurationTrust::VERIFIED);
    CHECK(bus.mmd3[0x16]==0 && bus.mmd7[0x3D]==0 && bus.mmd7[0x3C]==2);
    CHECK(phy.functionalModeSnapshot().established && bus.ext==vendor);
    CHECK(writes(bus,first,0x416)==0 && writes(bus,first,31)==0);
    unsigned cmd=0, pollFrame=0, ad=0, restart=0;
    for (unsigned i=first;i<bus.frames;++i) {
      const auto& w=bus.trace[i];
      if (semantic(w) && w.devad==3 && w.target==0 && w.value==0x8000) { cmd=i; }
      if (i>cmd && cmd && w.devad==3 && w.target==0 && w.kind==FrameKind::READ && !pollFrame) { pollFrame=i; }
      if (semantic(w) && w.devad==7 && w.target==0x3C) { ad=i; }
      if (semantic(w) && w.devad==31 && w.target==0 && (w.value&0x0200)!=0) { restart=i; }
      CHECK(!(semantic(w) && w.devad==31 && w.target==0 && (w.value&0x8000)!=0));
    }
    CHECK(cmd>=first && pollFrame>cmd && ad>pollFrame && restart>ad);
    CHECK(phy.startInitialize({ResetKind::NONE,FunctionalMode::RETAIN,PcsRxClock::CONTINUOUS},budget(),token).inProgress());
    CHECK(drain(phy,bus,token).ok() && take(phy,token).eee.pcsControl==0);
    CHECK(phy.functionalModeSnapshot().established && phy.functionalModeSnapshot().completedWrites==32);
    bus.mmd3[0]=0x0400; // Same binding, external RX-clock disagreement.
    CHECK(phy.startVerify(budget(),token).inProgress());
    CHECK(drain(phy,bus,token).is(Err::CONFIG_MISMATCH));
    const auto verify=take(phy,token);
    CHECK(verify.mismatch.valid && verify.mismatch.mmd && verify.mismatch.devad==3 &&
          verify.mismatch.registerAddress==0 && verify.mismatch.mask==0x8400);
  }
  // Table 7-92 defines four RX policy combinations. Table 6-7 still writes
  // the exact 00C2 word first. Any standalone override follows its 001F restart.
  for (unsigned code=0;code<4;++code) {
    Controller bus;
    Driver phy;
    p25Board(bus,true,true,true);
    ready(phy,bus);
    EeeSettings settings;
    settings.enabled=settings.maintenanceAdmitted=true;
    settings.disableNextPageReception=(code&2U)!=0;
    settings.shutdownRxDuringQuiet=(code&1U)!=0;
    OperationToken token;
    const uint16_t expected=static_cast<uint16_t>(0x00C0U | (code<<1));
    unsigned first=bus.frames;
    CHECK(phy.startConfigureEee(settings,budget(),token).inProgress());
    for (unsigned pass=0;pass<2;++pass) {
      if (pass) {
        first=bus.frames;
        CHECK(phy.startInitialize({ResetKind::HARD},budget(),token).inProgress());
      }
      CHECK(drain(phy,bus,token).ok());
      const auto result=take(phy,token);
      CHECK(result.eee.controlValid && result.eee.control==expected && result.eee.trust==ConfigurationTrust::VERIFIED);
      CHECK(result.eee.desired.disableNextPageReception==settings.disableNextPageReception);
      CHECK(result.eee.desired.shutdownRxDuringQuiet==settings.shutdownRxDuringQuiet);
      unsigned table=0, restart=0, policy=0, total=0;
      for (unsigned i=first;i<bus.frames;++i) {
        const auto& w=bus.trace[i];
        if (!semantic(w)) { continue; }
        if (w.devad==31 && w.target==0x4D1) {
          if (total++==0) { table=i; CHECK(w.value==0x00C2); }
          else { policy=i; CHECK(w.value==expected); }
        }
        if (w.devad==31 && w.target==31 && w.value==0x4000) { restart=i; }
      }
      CHECK(table>=first && restart>table);
      CHECK(code==1 ? total==1 : (total==2 && policy>restart));
      bus.mmd7[0x3D]=2;
      EeeSnapshot snapshot;
      CHECK(phy.readEeeStatus(snapshot,{1000000,31}).ok());
      CHECK(snapshot.negotiatedValid==(code<2) && snapshot.negotiated==(code<2));
    }
    bus.ext[0x4D1]^=0x0002; // Contradict a single persistent RX field.
    EeeSnapshot mismatch;
    CHECK(phy.readEeeStatus(mismatch,{1000000,31}).ok() && mismatch.trust==ConfigurationTrust::DIRTY);
    settings.enabled=false;
    settings.shutdownRxDuringQuiet=false;
    const auto calls=bus.calls, clocks=bus.clocks;
    CHECK(phy.startConfigureEee(settings,budget(),token).is(Err::INVALID_CONFIG));
    CHECK(bus.calls==calls && bus.clocks==clocks);
  }
  // Sections 6.3.3.2/6.3.4 support negotiated LPI. Table 7-92
  // additionally permits force code 3 only with a separately configured peer.
  for (uint8_t force : {uint8_t{0},uint8_t{3}}) {
    Controller bus;
    Driver phy;
    p25Board(bus,true,true,true);
    ready(phy,bus);
    OperationToken token;
    EeeSettings policy;
    policy.enabled=policy.maintenanceAdmitted=true;
    CHECK(phy.startConfigureEee(policy,budget(),token).inProgress());
    CHECK(drain(phy,bus,token).ok() && take(phy,token).kind==OperationKind::EEE_CONFIGURE);
    bus.mmd7[0x3D]=force ? 0 : 2; // No negotiated EEE for the bilateral case.
    LinkSnapshot link;
    PowerSnapshot power;
    EeeSnapshot eee;
    CHECK(phy.readLinkStatus(link,{1000000,10}).ok() && link.linkUp);
    CHECK(phy.readPowerStatus(power,{1000000,7}).ok() && power.state==PowerState::NORMAL_INDICATED);
    CHECK(phy.readEeeStatus(eee,{1000000,31}).ok() && eee.negotiatedValid && eee.negotiated==(force==0));
    LpiResult lpi;
    LpiOptions options;
    options.macTxQuiesced=options.maintenanceAdmitted=true;
    const auto calls=bus.calls, clocks=bus.clocks;
    for (uint8_t bad : {uint8_t{1},uint8_t{2},uint8_t{255}}) {
      options.forceModeCode=bad;
      CHECK(phy.startLpi(options,lpi,{1000000,18}).is(Err::INVALID_PARAM));
    }
    options.forceModeCode=3;
    CHECK(phy.startLpi(options,lpi,{1000000,18}).is(Err::INVALID_CONFIG));
    CHECK(bus.calls==calls && bus.clocks==clocks);
    options.forceModeCode=force;
    options.partnerForceConfirmed=force==3;
    const auto start=bus.frames;
    // The entry clock jumps to exact equality with the remaining pass bound.
    const unsigned cost=force ? 104U : 52U;
    bus.delayedClock=bus.clocks+2;
    CHECK(phy.startLpi(options,lpi,{bus.now+240U+1U+cost,18}).is(Err::OPERATION_TIMEOUT));
    CHECK(bus.frames==start && !phy.operationSnapshot().cleanupOwed);
    CHECK(phy.startLpi(options,lpi,{1000000,18}).ok());
    CHECK(bus.ext[0x4D1]==(force ? 0x70C2 : 0x10C2));
    CHECK(bus.frames-start==(force ? 18U : 9U) && phy.operationSnapshot().cleanupOwed);
    const auto exitStart=bus.frames;
    bus.delayedClock=bus.clocks+2;
    CHECK(phy.stopLpi(lpi,{bus.now+240U+1U+cost,18}).is(Err::OPERATION_TIMEOUT));
    CHECK(bus.frames==exitStart && phy.operationSnapshot().cleanupOwed);
    CHECK(phy.stopLpi(lpi,{1000000,18}).ok() && lpi.minimumNormalIfgUs==36);
    CHECK(bus.ext[0x4D1]==0x00C2 && !phy.operationSnapshot().cleanupOwed);
    if (force) {
      const uint16_t words[]={0x60C2,0x70C2,0x60C2,0x00C2};
      unsigned n=0;
      for (unsigned i=start;i<bus.frames;++i) {
        if (!semantic(bus.trace[i])) { continue; }
        CHECK(n<4 && bus.trace[i].target==0x04D1 && bus.trace[i].value==words[n]);
        ++n;
      }
      CHECK(n==4);
    }
  }
  // SNLS638C 6.3.6..8 and tables 7-20, 7-25 and 7-75. Check both
  // normal roles and the order of IEEE down, deep entry and explicit exit.
  for (bool follower : {false,true}) {
    Controller bus;
    Driver phy;
    p25Board(bus,follower,follower,follower);
    bus.clockLossApproved=true;
    ready(phy,bus);
    PowerSettings request;
    request.managementClockAvailable=request.maintenanceAdmitted=request.clockLossApproved=true;
    OperationToken token{99};
    request.request=PowerRequest::DEEP_POWER_DOWN;
    const auto calls=bus.calls, clocks=bus.clocks;
    CHECK(phy.startConfigurePower(request,budget(),token).is(Err::INVALID_CONFIG));
    CHECK(bus.calls==calls && bus.clocks==clocks && token.value==99);
    request.recoveryAvailable=true;
    const auto first=bus.frames;
    CHECK(phy.startConfigurePower(request,budget(),token).inProgress());
    CHECK(drain(phy,bus,token).ok() && take(phy,token).persistentRestored);
    auto power=phy.powerSnapshot();
    CHECK(power.deepValid && power.rawDeep==4 && power.observedBmcrPowerDown);
    CHECK(power.state==PowerState::DEEP_POWER_DOWN_INDICATED && power.trust==ConfigurationTrust::VERIFIED);
    unsigned down=0, deep=0;
    for (unsigned i=first;i<bus.frames;++i) {
      if (!semantic(bus.trace[i])) { continue; }
      if (bus.trace[i].target==0 && (bus.trace[i].value&0x0800U)!=0) { down=i; }
      if (bus.trace[i].target==0x428 && bus.trace[i].value==4) { deep=i; }
    }
    CHECK(down>=first && deep>down);
    PowerSnapshot observed;
    CHECK(phy.readPowerStatus(observed,{1000000,6}).is(Err::OPERATION_TIMEOUT));
    CHECK(phy.readPowerStatus(observed,{1000000,7}).ok() && observed.rawDeep==4);
    const auto firstExit=bus.frames;
    request.request=PowerRequest::NORMAL;
    CHECK(phy.startConfigurePower(request,budget(),token).inProgress());
    CHECK(drain(phy,bus,token).ok() && take(phy,token).persistentRestored);
    CHECK((bus.words[0]&0x0800U)==0 && bus.ext[0x428]==0);
    unsigned clear=0, wake=0;
    for (unsigned i=firstExit;i<bus.frames;++i) {
      if (!semantic(bus.trace[i])) { continue; }
      if (bus.trace[i].target==0x428 && bus.trace[i].value==0) { clear=i; }
      if (bus.trace[i].target==0 && (bus.trace[i].value&0x0800U)==0) { wake=i; }
    }
    CHECK(clear>=firstExit && wake>clear);
    request.activeSleep=true;
    CHECK(phy.startConfigurePower(request,budget(),token).inProgress());
    CHECK(drain(phy,bus,token).ok() && take(phy,token).persistentRestored);
    bus.words[0x16]&=0xFEFF;
    CHECK((bus.words[0x11]&0x7000U)==0x6000 && phy.readPowerStatus(observed,{1000000,7}).ok());
    CHECK(observed.state==PowerState::ACTIVE_SLEEP_INDICATED);
    request.activeSleep=false; request.disablePll=true; request.request=PowerRequest::IEEE_POWER_DOWN;
    CHECK(phy.startConfigurePower(request,budget(),token).inProgress());
    CHECK(drain(phy,bus,token).ok() && take(phy,token).persistentRestored);
    CHECK((bus.words[0x11]&0xF000U)==0x8000 && (bus.words[0]&0x0800U)!=0);
    request.disablePll=false; request.request=PowerRequest::DEEP_POWER_DOWN;
    bus.ext[0x428]=0x30; // Running compliance pattern is a different owner.
    const auto at=bus.frames;
    CHECK(phy.startConfigurePower(request,budget(),token).inProgress());
    CHECK(drain(phy,bus,token).is(Err::INVALID_CONFIG) && !take(phy,token).mutationAttempted);
    CHECK(!writes(bus,at,0x428) && !writes(bus,at,0));
  }
  // Separate a confirmed IEEE-down command from an uncertain 0428 target.
  // Fail selectors, the dependent read/write and release at the deep entry.
  unsigned deepOffset=0, deepCall=0;
  for (int cut=-1;cut<6;++cut) {
    for (bool effect : {false,true}) {
      Controller bus;
      Driver phy;
      p25Board(bus,true,true,true);
      ready(phy,bus);
      PowerSettings request;
      request.request=PowerRequest::DEEP_POWER_DOWN;
      request.managementClockAvailable=request.maintenanceAdmitted=request.clockLossApproved=request.recoveryAvailable=true;
      const unsigned first=bus.frames, firstCall=bus.calls;
      if (cut>=0 && cut<5) { bus.failFrame=int(first+deepOffset)+cut; bus.acceptedFailure=effect; }
      if (cut==5) { bus.releaseCall=int(firstCall+deepCall); }
      OperationToken token;
      CHECK(phy.startConfigurePower(request,budget(),token).inProgress());
      const auto status=drain(phy,bus,token);
      const auto result=take(phy,token);
      CHECK(result.power.observedBmcrPowerDown && result.mutationCompleted);
      if (cut<0) {
        CHECK(status.ok() && result.power.deepWriteCompleted && result.power.deepWritePossible);
        for (unsigned i=first;i<bus.frames;++i) {
          if (semantic(bus.trace[i]) && bus.trace[i].target==0x428) { deepOffset=i-first-4; }
        }
        for (unsigned i=firstCall;i<bus.calls;++i) {
          if (bus.firstFrame[i]==first+deepOffset) { deepCall=i-firstCall; }
        }
        CHECK(deepOffset && deepCall);
      } else {
        CHECK(!status.ok() && result.cleanupOwed && result.power.trust==ConfigurationTrust::DIRTY);
        CHECK(result.power.deepWriteAttempted==(cut>=4));
        CHECK(result.power.deepWriteCompleted==(cut==5));
        CHECK(result.power.deepWritePossible==(cut==5 || (cut==4 && effect)));
      }
    }
  }
  // The poll word remains valid on a release failure. A selector failure
  // before the 3:0000 reset write carries no target mutation or cleanup debt.
  unsigned pcsTarget=0, pcsPollCall=0;
  for (int cut=-1;cut<5;++cut) {
    Controller bus;
    Driver phy;
    p25Board(bus,true,true,true);
    ready(phy,bus);
    bus.mmd3[0]=0x0400;
    const unsigned first=bus.frames, firstCall=bus.calls;
    if (cut>=0 && cut<4) { bus.failFrame=int(first+pcsTarget-3)+cut; bus.acceptedFailure=true; }
    if (cut==4) { bus.releaseCall=int(firstCall+pcsPollCall); }
    OperationToken token;
    CHECK(phy.startInitialize({ResetKind::EEE_PCS},budget(),token).inProgress());
    const auto status=drain(phy,bus,token);
    const auto result=take(phy,token);
    if (cut<0) {
      CHECK(status.ok());
      for (unsigned i=first;i<bus.frames;++i) {
        const auto& w=bus.trace[i];
        if (semantic(w) && w.devad==3 && w.target==0 && w.value==0x8000) { pcsTarget=i-first; }
      }
      for (unsigned i=firstCall;i<bus.calls;++i) {
        if (bus.firstFrame[i]==first+pcsTarget+1) { pcsPollCall=i-firstCall; }
      }
      CHECK(pcsTarget && pcsPollCall);
    } else {
      CHECK(!status.ok() && result.commandAccepted==(cut==4));
      CHECK(result.mutationAttempted==(cut>=3) && result.cleanupOwed==(cut>=3));
      if (cut==4) { CHECK(result.eee.pcsControlValid && result.eee.pcsControl==0 && !result.protocolComplete); }
    }
  }
  Controller leader;
  Driver driver;
  p25Board(leader,false,false,false);
  ready(driver,leader);
  OperationToken token{99};
  const auto clocks=leader.clocks, calls=leader.calls;
  CHECK(driver.startInitialize({ResetKind::HARD,FunctionalMode::POWER_OPTIMIZED},budget(),token).is(Err::INVALID_CONFIG));
  CHECK(driver.startInitialize({ResetKind::NONE,FunctionalMode::DEFAULT},budget(),token).is(Err::INVALID_CONFIG));
  CHECK(driver.startInitialize({ResetKind::HARD,static_cast<FunctionalMode>(255)},budget(),token).is(Err::INVALID_PARAM));
  CHECK(token.value==99 && clocks==leader.clocks && calls==leader.calls);
}
