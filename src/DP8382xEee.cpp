/// @file DP8382xEee.cpp
/// @brief Exact EEE recipe, explicit policy and bounded source-implied observations.
#include <DP8382x/DP8382x.h>
#include <DP8382x/CommandTable.h>

namespace DP8382x {
namespace {
struct RecipeWord { uint16_t address; uint16_t value; };
// SNLS647I 8.3.3.1. Row 1 uses the shared HARD primitive, row 2 MMD7.60;
// the middle opaque words authorize this sequence only, never generic writes.
constexpr RecipeWord RECIPE[] = {
  {0x001F,0x8000},{0x003C,0x0002},{0x04D1,0x008B},{0x04D3,0x4F12},
  {0x04DF,0x0180},{0x033E,0xA681},{0x033F,0x0003},{0x0123,0x0800},
  {0x031B,0x8848},{0x0466,0xFE00},{0x04CF,0x261D},{0x0416,0x1F30},
  {0x04F5,0x2864},{0x04E0,0xFFF2},{0x031F,0xFE36},{0x0308,0x0000},
  {0x04F4,0x0800},{0x0000,0x3300}
};
Status eeeConfig(const char* message) noexcept {
  return Status::Error(Err::INVALID_CONFIG,message,static_cast<int32_t>(ConfigDetail::SOURCE_GATE));
}
Status eeeBusy(BusyDetail detail) noexcept {
  return Status::Error(Err::BUSY,"EEE owner unavailable",static_cast<int32_t>(detail));
}
Status eeeTimeout(TimeoutDetail detail) noexcept {
  return Status::Error(Err::OPERATION_TIMEOUT,"EEE allowance exhausted",static_cast<int32_t>(detail));
}
MdioBatch directRead(uint8_t address) noexcept {
  MdioBatch batch; batch.frameCount=1;
  batch.frames[0]={FrameKind::READ,address,0,0,NO_READ_INDEX}; return batch;
}
bool readComplete(const MdioBatch& batch,const BatchResult& result) noexcept {
  for (uint8_t i=0;i<batch.frameCount;++i) {
    const auto& f=result.frames[i];
    if (!f.attempted || !f.completed || f.valueValid!=(batch.frames[i].kind==FrameKind::READ)) { return false; }
  }
  return true;
}
} // namespace

Status DP8382x::_validateEee(const EeeSettings& settings) const noexcept {
  if (settings.enabled && _interface.desiredValid &&
      (_interface.desired.clockOutput!=ClockOutput::UNOWNED ||
       (_interface.desired.isolationOwned && _interface.desired.isolate))) {
    return eeeConfig("EEE conflicts with explicit interface clock or isolation ownership");
  }
  if (settings.advertisement!=0U && settings.advertisement!=2U) {
    return Status::Error(Err::INVALID_PARAM,"EEE advertisement is zero or 100BASE-TX",static_cast<int32_t>(ParamDetail::VALUE_WIDTH));
  }
  if (settings.enabled && _pins.desiredValid &&
      (_pins.desired.pins[1].function!=PinFunction::UNOWNED || _pins.desired.led1ClockOverride)) {
    return eeeConfig("EEE clock recipe conflicts with owned LED1 pin controls");
  }
  if (settings.enabled && _wake.desiredValid && _wake.desired.route == WakeRoute::LED1) {
    return eeeConfig("EEE clock recipe conflicts with owned LED1 wake route");
  }
  if (!settings.maintenanceAdmitted) { return eeeConfig("EEE reset maintenance not admitted"); }
  if (!settings.enabled) { return Status::Ok(); }
  if (!_settings.desiredValid || _settings.desired.mode!=LinkMode::AUTONEGOTIATED) {
    return eeeConfig("EEE requires an owned autonegotiated profile");
  }
  const auto link=validateLinkSettings(_settings.desired); if (!link.ok()) { return link; }
  if (_config.interface!=MacInterface::MII && _config.interface!=MacInterface::RMII) {
    return eeeConfig("EEE interface has not been admitted");
  }
  if (_power.desiredValid && (_power.desired.request!=PowerRequest::NORMAL ||
      _power.desired.disablePll || _power.desired.activeSleep)) {
    return eeeConfig("EEE and explicit low-power policy cannot share this recipe");
  }
  if (_interrupts.desiredValid && _interrupts.desired.pin!=InterruptPin::INTR_PWRDN) {
    return eeeConfig("EEE clock recipe conflicts with owned LED interrupt route");
  }
  return Status::Ok();
}

Status DP8382x::startConfigureEee(const EeeSettings& settings,const OperationBudget& budget,OperationToken& token) noexcept {
  if (settings.advertisement!=0U && settings.advertisement!=2U) {
    return Status::Error(Err::INVALID_PARAM,"EEE advertisement is zero or two",static_cast<int32_t>(ParamDetail::VALUE_WIDTH));
  }
  if (budget.pollIntervalUs==0U || budget.maxStalledPolls==0U) {
    return Status::Error(Err::INVALID_PARAM,"Invalid EEE budget",static_cast<int32_t>(ParamDetail::BUDGET_RANGE));
  }
  if (!_bound) { return Status::Error(Err::NOT_BOUND,"No transport binding"); }
  if (_active || _polling || _operation.state==OperationState::RUNNING) { return eeeBusy(BusyDetail::OPERATION_ACTIVE); }
  if (_operation.state==OperationState::RESULT_PENDING) { return eeeBusy(BusyDetail::RESULT_PENDING); }
  if (_transport.clockInvalid) {
    return Status::Error(Err::INVALID_CONFIG,"Clock contract failed",static_cast<int32_t>(ConfigDetail::CLOCK_CONTRACT));
  }
  if (_operation.cleanupOwed || _operation.temporaryMayBeActive) { return eeeBusy(BusyDetail::CLEANUP_REQUIRED); }
  if (!_initialized || !capabilities().valid) { return Status::Error(Err::NOT_INITIALIZED,"Initialize before EEE configuration"); }
  const auto valid=_validateEee(settings); if (!valid.ok()) { return valid; }
  if (!_otherControlsVerified(OperationKind::EEE_CONFIGURE)) { return eeeConfig("Verify other owned controls first"); }
  if (budget.io.maxFrames==0U) { return eeeTimeout(TimeoutDetail::FRAME_BUDGET); }
  if (budget.io.deadlineUs==0U) { return eeeTimeout(TimeoutDetail::DEADLINE); }
  _beginOperation(OperationKind::EEE_CONFIGURE,budget);
  _eee.desired=settings; _eee.desiredValid=true; _eee.trust=ConfigurationTrust::DIRTY;
  _eee.stale=true; _eee.negotiatedValid=false;
  _result.reset=ResetKind::HARD; _result.commandRequired=true;
  _eeeRecipePending=settings.enabled; _eeeCapabilityChecked=false;
  _eeeTailPending=true; _eeeOpaqueValid=false;
  _setInitStep(InitStep::IDENTIFY);
  token=_operation.token; return _operation.status;
}

uint16_t DP8382x::_eeeControl() const noexcept {
  // 8.3.3.2: one disables bypass. Stage 30 owns forced TX LPI bit12.
  return static_cast<uint16_t>((_eee.desired.bypassBit0?0U:1U)|(_eee.desired.bypassBit3?0U:8U));
}
void DP8382x::_setEeeStep(EeeStep step) noexcept {
  _pinsWorking=false; _interfaceWorking=false; _qualityWorking=false;
  _wakeWorking=false;
  _eeeWorking=true; _powerWorking=_interruptWorking=_cableWorking=_bistWorking=_loopWorking=_configuring=false;
  _eeeStep=step; _operation.nextPollValid=false;
  _operation.phase=step==EeeStep::RECIPE || step==EeeStep::SET_ADVERTISEMENT || step==EeeStep::SET_CONTROL ?
      OperationPhase::RESTORE : OperationPhase::VERIFY;
}
MdioBatch DP8382x::_eeeBatch() const noexcept {
  switch (_eeeStep) {
    case EeeStep::CAPABILITY: return _mmdBatch(0x0014,FrameKind::READ,0,3);
    case EeeStep::RECIPE: {
      const auto& row=RECIPE[_eeeRow];
      if (_eeeRow==17U) {
        auto batch=directRead(0); batch.frames[0]={FrameKind::WRITE,0,row.value,0,NO_READ_INDEX}; return batch;
      }
      return _mmdBatch(row.address,FrameKind::WRITE,row.value,_eeeRow==1U?7:31);
    }
    case EeeStep::ADVERTISEMENT_CHECK: case EeeStep::VERIFY_ADVERTISEMENT:
      return _mmdBatch(0x003C,FrameKind::READ,0,7);
    case EeeStep::SET_ADVERTISEMENT:
      return _mmdBatch(0x003C,FrameKind::WRITE,_eee.desired.advertisement,7);
    case EeeStep::SET_CONTROL: {
      auto batch=_mmdBatch(0x04D1,FrameKind::READ,0);
      batch.frameCount=5;
      // Only 0/3/12 have exact-part field authority; bit1 remains opaque.
      batch.frames[4]={FrameKind::WRITE,0x0E,_eeeControl(),0xEFF6,3};
      return batch;
    }
    case EeeStep::VERIFY_CONTROL: return _mmdBatch(0x04D1,FrameKind::READ,0);
  }
  return {};
}
Status DP8382x::_advanceEee(Status status,bool called,const MdioBatch& batch) noexcept {
  const auto last=static_cast<uint8_t>(batch.frameCount-1U);
  const bool complete=called && _result.failingBatch.completedFrames==batch.frameCount;
  if (called && batch.frames[last].kind==FrameKind::WRITE) {
    const auto& target=_transport.lastBatch.frames[last];
    _result.mutationAttempted|=target.attempted;
    _result.mutationCompleted|=complete;
    _result.mutationPossible|=target.completed || target.effectPossible || target.valueValid || _transport.violation!=TransportViolation::NONE;
    if (_eeeStep==EeeStep::SET_CONTROL && _result.failingBatch.completedFrames>=4U) {
      _eee.control=_transport.lastBatch.frames[3].value; _eee.controlValid=true;
    }
  }
  if (complete && batch.frames[last].kind==FrameKind::READ) {
    const auto word=_transport.lastBatch.frames[last].value;
    uint16_t mask=0,expected=0,address=0; uint8_t devad=0;
    if (_eeeStep==EeeStep::CAPABILITY) {
      _eee.capability=word; _eee.capabilityValid=true;
      mask=0xFFFF; expected=2; address=0x0014; devad=3;
    } else if (_eeeStep==EeeStep::ADVERTISEMENT_CHECK || _eeeStep==EeeStep::VERIFY_ADVERTISEMENT) {
      _eee.advertisement=word; _eee.advertisementValid=true;
      mask=_eeeStep==EeeStep::ADVERTISEMENT_CHECK?0xFFFF:0x0002;
      expected=_eeeStep==EeeStep::ADVERTISEMENT_CHECK?2:(_eee.desired.enabled?_eee.desired.advertisement:0);
      address=0x003C; devad=7;
    } else if (_eeeStep==EeeStep::VERIFY_CONTROL) {
      _eee.control=word; _eee.controlValid=true;
      mask=0x1009; expected=_eeeControl(); address=0x04D1; devad=31;
    }
    if (mask!=0U && (word&mask)!=expected && !_result.mismatch.valid) {
      _result.mismatch={true,true,devad,address,mask,expected,word};
      _eee.trust=ConfigurationTrust::DIRTY;
    }
  }
  if (!status.ok()) { return _finishOperation(status); }
  if (_result.mismatch.valid) { return _finishOperation(Status::Error(Err::CONFIG_MISMATCH,"EEE source check/readback mismatch")); }
  switch (_eeeStep) {
    case EeeStep::CAPABILITY:
      _eeeCapabilityChecked=true; _eeeWorking=false; _setInitStep(InitStep::COMMAND); break;
    case EeeStep::RECIPE:
      if (_eeeRow==1U) { _setEeeStep(EeeStep::ADVERTISEMENT_CHECK); }
      else if (++_eeeRow<18U) { _setEeeStep(EeeStep::RECIPE); }
      else {
        _eeeRecipePending=false; _eeeOpaqueValid=true; _eeeResumeInit=true;
        _setEeeStep(EeeStep::SET_ADVERTISEMENT);
      }
      break;
    case EeeStep::ADVERTISEMENT_CHECK:
      _eeeRow=2; _setEeeStep(EeeStep::RECIPE); break;
    case EeeStep::SET_ADVERTISEMENT: _setEeeStep(EeeStep::SET_CONTROL); break;
    case EeeStep::SET_CONTROL: _setEeeStep(EeeStep::VERIFY_ADVERTISEMENT); break;
    case EeeStep::VERIFY_ADVERTISEMENT:
      if (_eee.desired.enabled) { _setEeeStep(EeeStep::VERIFY_CONTROL); break; }
      _eee.trust=ConfigurationTrust::VERIFIED; _result.persistentRestored=true;
      if (_operation.kind!=OperationKind::VERIFY) { _initialized=true; }
      _eeeWorking=false; _eee.lastStatus=Status::Ok();
      return _finishOperation(Status::Ok());
    case EeeStep::VERIFY_CONTROL:
      if (!_eeeOpaqueValid) { return _finishOperation(eeeConfig("Opaque EEE state requires reset/reapply")); }
      if (_eeeResumeInit) {
        _eeeResumeInit=false; _eeeWorking=false;
        _setInitStep(InitStep::APPLY); break;
      }
      _eee.trust=ConfigurationTrust::VERIFIED; _result.persistentRestored=true;
      if (_operation.kind!=OperationKind::VERIFY) { _initialized=true; }
      _eeeWorking=false; _eee.lastStatus=Status::Ok();
      return _finishOperation(Status::Ok());
  }
  return _operation.status;
}

Status DP8382x::readEeeStatus(EeeSnapshot& snapshot,const IoBudget& budget) noexcept {
  if (!_bound) { return Status::Error(Err::NOT_BOUND,"No transport binding"); }
  if (_active || _polling || (_operation.state==OperationState::RUNNING &&
      (_operation.phase==OperationPhase::WAIT || _result.nextPage.requested))) {
    return eeeBusy(BusyDetail::OPERATION_ACTIVE);
  }
  const auto fail=[&](Status status) noexcept {
    _eee.stale=true; _eee.negotiatedValid=false; _eee.lastStatus=status; return status;
  };
  if (_transport.clockInvalid) {
    return fail(Status::Error(Err::INVALID_CONFIG,"Clock contract failed",static_cast<int32_t>(ConfigDetail::CLOCK_CONTRACT)));
  }
  const auto quiet=_checkQuiet(); if (!quiet.ok()) { return quiet; }
  if (!_initialized || !capabilities().valid) { return Status::Error(Err::NOT_INITIALIZED,"Initialize before EEE observation"); }
  if (_stopped) { return eeeBusy(BusyDetail::CONTROLLER_QUARANTINED); }
  // Four leading and three trailing C22 reads surround six indivisible MMD
  // transactions. Reserve all eight callback bounds before consuming history.
  constexpr uint32_t frames=31,callbacks=8;
  if (budget.maxFrames<frames) { return fail(eeeTimeout(TimeoutDetail::FRAME_BUDGET)); }
  uint64_t raw=0,upper=0;
  _active=true; const auto clock=_sampleTime(raw,upper); _active=false;
  if (!clock.ok()) { return fail(clock); }
  if (raw>=budget.deadlineUs) { return fail(eeeTimeout(TimeoutDetail::DEADLINE)); }
  const auto& bounds=_config.transport.bounds;
  const uint64_t bound=uint64_t{callbacks}*bounds.fixedUs+uint64_t{frames}*bounds.frameUs;
  if (upper>=budget.deadlineUs || bound>=budget.deadlineUs-upper) { return fail(eeeTimeout(TimeoutDetail::BOUND_ADMISSION)); }
  uint16_t bookends[7]{};
  constexpr uint8_t direct[]={0,1,1,0x10,1,0x10,0};
  constexpr uint8_t devads[]={3,3,7,7,3,31};
  constexpr uint16_t addresses[]={0x0001,0x0014,0x003C,0x003D,0x0016,0x04D1};
  uint16_t* words[]={&_eee.status,&_eee.capability,&_eee.advertisement,&_eee.partner,&_eee.wakeErrors,&_eee.control};
  bool* validity[]={&_eee.statusValid,&_eee.capabilityValid,&_eee.advertisementValid,&_eee.partnerValid,&_eee.wakeErrorsValid,&_eee.controlValid};
  IoBudget remaining=budget;
  _eee.stale=true; _eee.negotiatedValid=false;
  _evidence.lastRequested=evidenceBit(EvidenceSource::EEE_STATUS)|evidenceBit(EvidenceSource::EEE_ERRORS)|
      evidenceBit(EvidenceSource::BMSR)|evidenceBit(EvidenceSource::PHYSTS);
  for (uint8_t phase=0;phase<8U;++phase) {
    MdioBatch batch;
    if (phase==0U || phase==7U) {
      const uint8_t offset=phase==0U?0:4; batch.frameCount=phase==0U?4:3;
      for (uint8_t i=0;i<batch.frameCount;++i) { batch.frames[i]={FrameKind::READ,direct[offset+i],0,0,NO_READ_INDEX}; }
    } else {
      batch=_mmdBatch(addresses[phase-1U],FrameKind::READ,0,devads[phase-1U]);
      *validity[phase-1U]=false;
    }
    bool called=false; const auto status=_execute(batch,remaining,&called);
    bool resetObserved=false;
    if (called && (phase==0U || phase==7U)) {
      for (uint8_t i=0;i<batch.frameCount;++i) {
        const auto& frame=_transport.lastBatch.frames[i];
        if (!frame.attempted || !frame.completed || !frame.valueValid) { break; }
        if (batch.frames[i].registerAddress==0U && (frame.value&0x8000U)!=0U) {
          resetObserved=true; _invalidateResetEvidence(true);
        }
      }
      if (resetObserved && _operation.state==OperationState::RUNNING) {
        _finishOperation(Status::Error(Err::INVALID_CONFIG,"Reset invalidated running operation",
                                      static_cast<int32_t>(ConfigDetail::DEVICE_INVALIDATED)));
      }
    }
    if (called && readComplete(batch,_transport.lastBatch)) {
      if (phase==0U || phase==7U) {
        const uint8_t offset=phase==0U?0:4;
        for (uint8_t i=0;i<batch.frameCount;++i) { bookends[offset+i]=_transport.lastBatch.frames[i].value; }
      } else {
        *words[phase-1U]=_transport.lastBatch.frames[3].value; *validity[phase-1U]=true;
        // A valid contradiction survives a later phase or release failure.
        if (_eee.desiredValid && ((phase==3U && (_eee.advertisement&2U)!=
            (_eee.desired.enabled?_eee.desired.advertisement:0U)) ||
            (phase==6U && _eee.desired.enabled && (_eee.control&(_lpiSession?0x0009U:0x1009U))!=_eeeControl()))) {
          _eee.trust=ConfigurationTrust::DIRTY;
        }
      }
    }
    if (!status.ok()) { return fail(status); }
    if (resetObserved) {
      return fail(Status::Error(Err::INVALID_CONFIG,"Reset observed during EEE acquisition",
                                static_cast<int32_t>(ConfigDetail::DEVICE_INVALIDATED)));
    }
    remaining.maxFrames-=batch.frameCount;
  }
  if (((bookends[0]^bookends[6])&0xFF00U)!=0U || ((bookends[2]^bookends[4])&0x0024U)!=0U ||
      ((bookends[3]^bookends[5])&0x0017U)!=0U ||
      ((bookends[4]&4U)!=0U)!=((bookends[5]&1U)!=0U) ||
      ((bookends[4]&0x20U)!=0U)!=((bookends[5]&0x10U)!=0U)) {
    return fail(eeeBusy(BusyDetail::OBSERVATION_CHANGED));
  }
  const bool active=_operation.temporaryMayBeActive || (_operation.state==OperationState::RUNNING &&
      (_result.mutationAttempted || _result.mutationPossible));
  _eee.negotiatedValid=!active && _eee.desiredValid && _eee.desired.enabled && _eeeOpaqueValid &&
      _eee.trust==ConfigurationTrust::VERIFIED && (_eee.capability&0xFFFDU)==0U &&
      (bookends[6]&0xDE00U)==0x1000U &&
      (bookends[4]&0x0024U)==0x0024U && (bookends[5]&0x0017U)==0x0015U &&
      (_eee.control&0x1009U)==0x0009U;
  _eee.negotiated=_eee.negotiatedValid && (_eee.capability&_eee.advertisement&_eee.partner&2U)!=0U;
  _eee.valid=true; _eee.stale=false; _eee.timeValid=true;
  _eee.observedAtUs=_lastTimeUs+bounds.clockLagUs; _eee.lastStatus=Status::Ok();
  snapshot=_eee; return Status::Ok();
}
Status DP8382x::startLpi(const LpiOptions& options,LpiResult& result,const IoBudget& budget) noexcept {
  return _setLpi(true,options,result,budget);
}
Status DP8382x::stopLpi(LpiResult& result,const IoBudget& budget) noexcept {
  return _setLpi(false,{},result,budget);
}
Status DP8382x::_setLpi(bool enabled,const LpiOptions& options,LpiResult& result,const IoBudget& budget) noexcept {
  if (!_bound) { return Status::Error(Err::NOT_BOUND,"No transport binding"); }
  if (_active || _polling || _operation.state==OperationState::RUNNING) { return eeeBusy(BusyDetail::OPERATION_ACTIVE); }
  if (_operation.state==OperationState::RESULT_PENDING) { return eeeBusy(BusyDetail::RESULT_PENDING); }
  if (_transport.clockInvalid) {
    return Status::Error(Err::INVALID_CONFIG,"Clock contract failed",static_cast<int32_t>(ConfigDetail::CLOCK_CONTRACT));
  }
  if (_stopped) { return eeeBusy(BusyDetail::CONTROLLER_QUARANTINED); }
  if (enabled) {
    if (_operation.cleanupOwed || _operation.temporaryMayBeActive) { return eeeBusy(BusyDetail::CLEANUP_REQUIRED); }
    if (!options.macTxQuiesced || !options.maintenanceAdmitted) { return eeeConfig("MAC drain and LPI maintenance must be admitted"); }
    if (!_initialized || !capabilities().valid) { return Status::Error(Err::NOT_INITIALIZED,"Initialize before LPI entry"); }
    if (!_otherControlsVerified(OperationKind::NONE) || !_eeeOpaqueValid || !_eee.desiredValid || !_eee.desired.enabled ||
        _eee.desired.bypassBit0 || _eee.desired.bypassBit3 || !_eee.valid || _eee.stale || !_eee.timeValid ||
        !_eee.negotiatedValid || !_eee.negotiated) { return eeeConfig("Verified negotiated EEE with both bypass controls disabled is required"); }
    // 100M restriction is library policy: section7.6 supplies timings only at100M.
    if (!_link.valid || _link.stale || !_link.timeValid || !_link.linkUp || !_link.resolvedModeValid ||
        _link.speed!=LinkSpeed::MBPS_100) { return eeeConfig("Current resolved 100M link is required"); }
    if (_power.observedStale || !_power.timeValid || !_power.bmcrValid || !_power.physcrValid || !_power.biscrValid ||
        _power.state!=PowerState::NORMAL_INDICATED || (_power.rawPhyscr&0xF000U)!=0U ||
        (_power.rawBmcr&0x0800U)!=0U) { return eeeConfig("Current normal power with sleep and PLL policies disabled is required"); }
  } else if (!_lpiSession) { return eeeConfig("No owned forced LPI session"); }
  const auto quiet=_checkQuiet(); if (!quiet.ok()) { return quiet; }
  if (budget.maxFrames<9U) { return eeeTimeout(TimeoutDetail::FRAME_BUDGET); }
  uint64_t raw=0,upper=0;
  _active=true; const auto clock=_sampleTime(raw,upper); _active=false;
  if (!clock.ok()) { return clock; }
  if (raw>=budget.deadlineUs) { return eeeTimeout(TimeoutDetail::DEADLINE); }
  const auto& bounds=_config.transport.bounds;
  const uint64_t bound=uint64_t{2}*bounds.fixedUs+uint64_t{9}*bounds.frameUs;
  if (upper>=budget.deadlineUs || bound>=budget.deadlineUs-upper) { return eeeTimeout(TimeoutDetail::BOUND_ADMISSION); }

  LpiResult next;
  IoBudget remaining=budget;
  const uint16_t expected=enabled?uint16_t{0x1000}:uint16_t{0};
  for (uint8_t phase=0;phase<2U;++phase) {
    auto batch=_mmdBatch(0x04D1,FrameKind::READ,0);
    if (phase==0U) {
      batch.frameCount=5;
      // SNLS647I8.3.4: only bit12. In particular, never import neighbor bits14:13.
      batch.frames[4]={FrameKind::WRITE,0x0E,expected,0xEFFF,3};
    }
    bool called=false;
    _lpiWorking=true;
    const auto status=_execute(batch,remaining,&called);
    _lpiWorking=false;
    next.status=status;
    if (called) {
      ++next.transactions;
      const auto& receipt=_transport.lastBatch;
      next.primary=receipt.status; next.release=receipt.releaseStatus;
      bool completed=true;
      for (uint8_t i=0;i<batch.frameCount;++i) {
        const auto& frame=receipt.frames[i];
        const bool attempted=completed && frame.attempted;
        completed=completed && attempted && frame.completed && frame.valueValid==(batch.frames[i].kind==FrameKind::READ);
        if (attempted) { ++next.attemptedFrames; }
        if (completed) { ++next.completedFrames; }
        if (i==3U && completed) {
          _eee.control=frame.value; _eee.controlValid=true;
          _eee.stale=true; _eee.negotiatedValid=false;
          // Both acquisitions can disprove persistent bypass controls. Stop still
          // succeeds on bit12 alone, but must not forgive this independent debt.
          if (_eee.desiredValid && _eee.desired.enabled && (frame.value&0x0009U)!=_eeeControl()) {
            _eee.trust=ConfigurationTrust::DIRTY;
          }
        }
      }
      if (phase==0U) {
        const auto& target=receipt.frames[4];
        next.mutationAttempted=target.attempted;
        next.mutationCompleted=completed;
        next.mutationPossible=target.completed || target.effectPossible || target.valueValid || _transport.violation!=TransportViolation::NONE;
        if (next.mutationAttempted || next.mutationPossible) {
          _lpiSession=true; _lpi.forcedLpi=ForcedLpi::UNCERTAIN;
          _operation.cleanupOwed=_operation.temporaryMayBeActive=true;
        }
      } else if (completed) {
        next.control=receipt.frames[3].value; next.readbackValid=true;
        if (status.ok() && (next.control&0x1000U)!=expected) {
          next.status=Status::Error(Err::CONFIG_MISMATCH,"Forced LPI bit12 readback mismatch");
        }
      }
    }
    if (!next.status.ok()) { break; }
    remaining.maxFrames-=batch.frameCount;
  }
  // No callback means preflight/admission failed; preserve the previous diagnostic result.
  if (next.transactions==0U) { return next.status; }
  if (next.status.ok()) {
    _lpi.forcedLpi=enabled?ForcedLpi::REQUESTED:ForcedLpi::OFF;
    if (!enabled) {
      _lpiSession=false; _operation.temporaryMayBeActive=false;
      // A clear request cannot repair unrelated persistent uncertainty.
      if (_initialized && _otherControlsVerified(OperationKind::NONE)) {
        _operation.cleanupOwed=false; _cleanupOwner=OperationKind::NONE;
      }
    }
  } else if (_lpiSession) { _lpi.forcedLpi=ForcedLpi::UNCERTAIN; }
  next.forcedLpi=_lpi.forcedLpi;
  if (next.forcedLpi==ForcedLpi::UNCERTAIN) {
    next.warning="forced LPI may remain active; frames sent now can be lost";
  }
  _lpi.lastResult=next; result=next;
  return next.status;
}
} // namespace DP8382x
