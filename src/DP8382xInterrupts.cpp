/// @file DP8382xInterrupts.cpp
/// @brief Owned IRQ policy, finite stimulus and single-collector deferred service.
#include <DP8382x/DP8382x.h>

namespace DP8382x {
namespace {
Status invalidIrq(const char* message) noexcept {
  return Status::Error(Err::INVALID_CONFIG,message,static_cast<int32_t>(ConfigDetail::SOURCE_GATE));
}
Status busyIrq(BusyDetail detail) noexcept {
  return Status::Error(Err::BUSY,"Interrupt owner unavailable",static_cast<int32_t>(detail));
}
MdioBatch readIrq(uint8_t address) noexcept {
  MdioBatch batch; batch.frameCount=1; batch.frames[0]={FrameKind::READ,address,0,0,NO_READ_INDEX}; return batch;
}
MdioBatch updateIrq(uint8_t address,uint16_t preserve,uint16_t value) noexcept {
  auto batch=readIrq(address); batch.frameCount=2;
  batch.frames[1]={FrameKind::WRITE,address,value,preserve,0}; return batch;
}
Status interruptSyntax(const InterruptSettings& s) noexcept {
  if (s.pin>InterruptPin::LED3 || s.sharedSource==InterruptSharedSource::UNKNOWN || s.sharedSource>InterruptSharedSource::WAKE ||
      s.misr1Mask>0x00FFU || s.misr2Mask>0x00FFU) {
    return Status::Error(Err::INVALID_PARAM,"Invalid interrupt encoding",static_cast<int32_t>(ParamDetail::VALUE_WIDTH));
  }
  return Status::Ok();
}
Status interruptAllowance(const OperationBudget& budget) noexcept {
  if (budget.io.maxFrames==0U || budget.io.deadlineUs==0U) {
    return Status::Error(Err::OPERATION_TIMEOUT,"Empty interrupt allowance",static_cast<int32_t>(
        budget.io.maxFrames==0U?TimeoutDetail::FRAME_BUDGET:TimeoutDetail::DEADLINE));
  }
  return Status::Ok();
}
Status serviceBudgetError() noexcept {
  return Status::Error(Err::OPERATION_TIMEOUT,"Interrupt service allowance exhausted",static_cast<int32_t>(TimeoutDetail::FRAME_BUDGET));
}
}

Status DP8382x::_validateInterruptSettings(const InterruptSettings& s) const noexcept {
  const auto syntax=interruptSyntax(s); if (!syntax.ok()) { return syntax; }
  const uint8_t pin=ledIndex(s.pin);
  const bool a=isA(_config.variant);
  if (!a && (pin==0U || pin==3U)) { return Status::Error(Err::UNSUPPORTED,"Read-only IRQ mux on this grade"); }
  if (_interface.desiredValid &&
      ((a && pin==0U && _interface.desired.clockOutput!=ClockOutput::UNOWNED) ||
       (pin==1U && _interface.desired.clockOutput==ClockOutput::CLKOUT25) ||
       (pin==2U && _interface.desired.collisionFullDuplex))) {
    return invalidIrq("Interrupt route conflicts with the interface clock/COL owner");
  }
  if (_pins.desiredValid) {
    for (uint8_t i=0;i<4U;++i) {
      if (_pins.desired.pins[i].function==PinFunction::INTERRUPT && pin!=i) {
        return invalidIrq("Release the dependent pin profile before moving the interrupt route");
      }
    }
  }
  if (pin<4U && _pins.desiredValid && _pins.desired.pins[pin].function!=PinFunction::UNOWNED &&
      _pins.desired.pins[pin].function!=PinFunction::INTERRUPT) {
    return invalidIrq("Interrupt route conflicts with the owned pin profile");
  }
  if (_wake.desiredValid) {
    // Disabling the detector does not release its separately selected route.
    if ((_wake.desired.enabled || _wake.desired.route == WakeRoute::INTERRUPT) &&
        s.sharedSource != InterruptSharedSource::WAKE) { return invalidIrq("Owned WoL requires WAKE shared-source selection"); }
    if (pin<4U && ledIndex(_wake.desired.route)==pin) {
      return invalidIrq("Direct WoL and interrupt output cannot share the same LED mux");
    }
    if (_wake.desired.route == WakeRoute::INTERRUPT &&
        (!s.globalEnable || (s.misr2Mask & 2U) == 0U ||
         (_identity.mode == OperatingMode::ENHANCED && !s.outputEnable))) {
      return invalidIrq("Owned wake interrupt route requires enabled delivery");
    }
  }
  if (_eee.desiredValid && _eee.desired.enabled && s.pin != InterruptPin::INTR_PWRDN) {
    return invalidIrq("Disable EEE before conflicting LED interrupt routing");
  }
  if (!s.maintenanceAdmitted || !s.pinAvailable) { return invalidIrq("Interrupt pin maintenance not admitted"); }
  if (_identity.mode==OperatingMode::BASIC) {
    if (pin==2U || pin==3U) { return Status::Error(Err::UNSUPPORTED,"BASIC exposes only LED0 and LED1"); }
  } else {
    if (!s.outputEnable && !s.powerDownInputSafe) { return invalidIrq("ENHANCED power-down input needs board compatibility"); }
    if (s.pin==InterruptPin::LED1 && !s.led1ClockUnused) { return invalidIrq("LED1 clock consumer conflicts with IRQ"); }
    if (pin==3U && _config.interface==MacInterface::MII && !s.crsUnused) {
      return invalidIrq("MII CRS consumer conflicts with LED3 IRQ");
    }
    if (s.pin==InterruptPin::LED2 && _config.interface==MacInterface::MII && !s.colUnused) {
      return invalidIrq("MII COL consumer conflicts with LED2 IRQ");
    }
  }
  return Status::Ok();
}
Status DP8382x::_admitInterrupt(const OperationBudget& budget,bool stop) const noexcept {
  if (_config.variant == Variant::DP83825I) { return _unsupportedFeature(); }
  if (budget.pollIntervalUs==0U || budget.maxStalledPolls==0U) {
    return Status::Error(Err::INVALID_PARAM,"Invalid interrupt operation budget",static_cast<int32_t>(ParamDetail::BUDGET_RANGE));
  }
  if (!_bound) { return Status::Error(Err::NOT_BOUND,"No transport binding"); }
  if (_active || _polling || _operation.state==OperationState::RUNNING) { return busyIrq(BusyDetail::OPERATION_ACTIVE); }
  if (_operation.state==OperationState::RESULT_PENDING) { return busyIrq(BusyDetail::RESULT_PENDING); }
  if (_transport.clockInvalid) {
    return Status::Error(Err::INVALID_CONFIG,"Repair binding clock before interrupt work",static_cast<int32_t>(ConfigDetail::CLOCK_CONTRACT));
  }
  if (stop) {
    if (!_interruptTestSession) { return invalidIrq("No owned interrupt test to stop"); }
  } else {
    if (_operation.cleanupOwed || _operation.temporaryMayBeActive) { return busyIrq(BusyDetail::CLEANUP_REQUIRED); }
    if (!_initialized || !capabilities().valid) { return Status::Error(Err::NOT_INITIALIZED,"Initialize before interrupt configuration"); }
    if (!_otherControlsVerified(OperationKind::INTERRUPT_CONFIGURE)) { return invalidIrq("Verify other owned controls first"); }
  }
  return Status::Ok();
}
Status DP8382x::startConfigureInterrupts(const InterruptSettings& settings,const OperationBudget& budget,OperationToken& token) noexcept {
  auto status=interruptSyntax(settings); if (!status.ok()) { return status; }
  status=_admitInterrupt(budget,false); if (!status.ok()) { return status; }
  status=_validateInterruptSettings(settings); if (!status.ok()) { return status; }
  status=interruptAllowance(budget); if (!status.ok()) { return status; }
  _beginOperation(OperationKind::INTERRUPT_CONFIGURE,budget);
  _interrupts.desired=settings; _interrupts.desiredValid=true;
  _interrupts.trust=ConfigurationTrust::DIRTY; _interrupts.observedStale=true;
  _interruptTailPending=false; _interruptVerifyOnly=false;
  _setInterruptStep(InterruptStep::IDENTIFY);
  token=_operation.token; return _operation.status;
}
Status DP8382x::startInterruptTest(const OperationBudget& budget,OperationToken& token) noexcept {
  auto status=_admitInterrupt(budget,false); if (!status.ok()) { return status; }
  if (!_interrupts.desiredValid || _interrupts.trust!=ConfigurationTrust::VERIFIED) { return invalidIrq("Verified IRQ policy required for stimulus"); }
  status=_validateInterruptSettings(_interrupts.desired); if (!status.ok()) { return status; }
  status=interruptAllowance(budget); if (!status.ok()) { return status; }
  _beginOperation(OperationKind::INTERRUPT_TEST_START,budget);
  _interruptTailPending=false; _interruptVerifyOnly=false;
  _setInterruptStep(InterruptStep::IDENTIFY);
  token=_operation.token; return _operation.status;
}
Status DP8382x::stopInterruptTest(const OperationBudget& budget,OperationToken& token) noexcept {
  auto status=_admitInterrupt(budget,true); if (!status.ok()) { return status; }
  status=interruptAllowance(budget); if (!status.ok()) { return status; }
  _beginOperation(OperationKind::INTERRUPT_TEST_STOP,budget);
  _interruptTailPending=false; _interruptVerifyOnly=false;
  _setInterruptStep(InterruptStep::IDENTIFY);
  token=_operation.token; return _operation.status;
}
uint16_t DP8382x::_interruptControlMask() const noexcept { return _identity.mode==OperatingMode::BASIC ? 0x000A : 0x000B; }
uint16_t DP8382x::_interruptControlValue() const noexcept {
  const auto& s=_interrupts.desired;
  return static_cast<uint16_t>((s.activeLow?8U:0U)|(s.globalEnable?2U:0U)|
      (_identity.mode!=OperatingMode::BASIC && s.outputEnable?1U:0U));
}
void DP8382x::_setInterruptStep(InterruptStep step) noexcept {
  _pinsWorking=false; _interfaceWorking=false; _qualityWorking=false;
  _wakeWorking=false;
  _interruptWorking=true; _cableWorking=_bistWorking=_loopWorking=_configuring=false;
  _interruptStep=step; _operation.nextPollValid=false;
  if (step==InterruptStep::IDENTIFY) { _interruptExpectedMode=_identity.mode; _interruptPinRestore=false; }
  _operation.phase=step==InterruptStep::IDENTIFY ? OperationPhase::IDENTIFY : OperationPhase::VERIFY;
}
void DP8382x::_interruptPassCost(bool verifyOnly,uint32_t& frames,uint32_t& callbacks) const noexcept {
  frames=callbacks=0;
  if (!_interrupts.desiredValid) return;
  const bool check=_interruptWorking;
  const auto pin=ledIndex(_interrupts.desired.pin);
  if (check && _interruptStep==InterruptStep::DISABLE) {
    // Mask delivery, drain, select/read/cross, set both masks, enable last,
    // then read both masks and PHYSCR. The pin preflight resolved replay.
    frames=23; callbacks=12;
    if (_interruptPinRestore) { frames+=9; callbacks+=2; }
    return;
  }
  if (check && _interruptStep==InterruptStep::TEST_WRITE) { frames=3; callbacks=2; return; }
  if (check && _interruptStep!=InterruptStep::IDENTIFY) return;
  if (check && _operation.kind==OperationKind::INTERRUPT_TEST_STOP) { frames=5; callbacks=3; return; }
  if (check && _operation.kind==OperationKind::INTERRUPT_TEST_START) { frames=6; callbacks=4; }
  else if (verifyOnly) { frames=10; callbacks=6; } // ID, selector, crossing, MISR1/2, PHYSCR
  else {
    frames=25; callbacks=13; // ID and complete mask/delivery pass
    if (check && _operation.kind==OperationKind::INTERRUPT_CONFIGURE) { ++frames; ++callbacks; } // CHECK_TEST
    // A prior reset invalidates live mux evidence. The WoL tail restores a
    // shared IRQ mux once, otherwise reserve this owner's RMW plus verification.
    if (!check && pin<4U && !(_wake.desiredValid && _wake.desired.route==WakeRoute::INTERRUPT)) {
      frames+=9; callbacks+=2;
    }
  }
  if (pin<4U) { frames+=pin>=2U?5U:4U; ++callbacks; }
  if (pin==1U && _identity.mode==OperatingMode::ENHANCED) { frames+=4; ++callbacks; }
}

MdioBatch DP8382x::_interruptBatch() const noexcept {
  switch (_interruptStep) {
    case InterruptStep::IDENTIFY: {
      auto batch=readIrq(2); batch.frameCount=2; batch.frames[1]={FrameKind::READ,3,0,0,NO_READ_INDEX}; return batch;
    }
    case InterruptStep::PIN_CLOCK: return _mmdBatch(0x0308,FrameKind::READ,0);
    case InterruptStep::PIN_MUX: case InterruptStep::PIN_VERIFY: {
      auto batch=_mmdBatch(static_cast<uint16_t>(0x0303U+ledIndex(_interrupts.desired.pin)),FrameKind::READ,0);
      if (_interruptStep==InterruptStep::PIN_MUX && ledIndex(_interrupts.desired.pin)>=2U) {
        batch.frames[batch.frameCount++]={FrameKind::READ,0x17,0,0,NO_READ_INDEX};
      }
      return batch;
    }
    case InterruptStep::PIN_APPLY: {
      auto batch=_mmdBatch(static_cast<uint16_t>(0x0303U+ledIndex(_interrupts.desired.pin)),FrameKind::READ,0);
      batch.frameCount=5; batch.frames[4]={FrameKind::WRITE,0x0E,4,0xFFF8,3}; return batch;
    }
    case InterruptStep::CHECK_TEST: case InterruptStep::VERIFY_CONTROL: case InterruptStep::TEST_VERIFY: return readIrq(0x11);
    case InterruptStep::DISABLE: return updateIrq(0x11,0xFFF9,0); // Global off; transient test is never replayed.
    case InterruptStep::DRAIN: case InterruptStep::CROSSING: case InterruptStep::VERIFY2: return readIrq(0x13);
    case InterruptStep::SELECT: return _mmdBatch(0x04A1,FrameKind::WRITE,_interrupts.desired.sharedSource==InterruptSharedSource::WAKE?0x1000:0);
    case InterruptStep::VERIFY_SELECT: return _mmdBatch(0x04A1,FrameKind::READ,0);
    case InterruptStep::MASK1: return updateIrq(0x12,0,_interrupts.desired.misr1Mask);
    case InterruptStep::MASK2: return updateIrq(0x13,0,_interrupts.desired.misr2Mask);
    case InterruptStep::OUTPUT: return updateIrq(0x11,static_cast<uint16_t>(~(_interruptControlMask()|0x0004U)),
                                                static_cast<uint16_t>(_interruptControlValue()&~0x0002U));
    case InterruptStep::ENABLE: return updateIrq(0x11,0xFFFD,static_cast<uint16_t>(_interruptControlValue()&0x0002U));
    case InterruptStep::VERIFY1: return readIrq(0x12);
    case InterruptStep::TEST_WRITE: return updateIrq(0x11,0xFFFB,_operation.kind==OperationKind::INTERRUPT_TEST_START?4:0);
  }
  return {};
}
Status DP8382x::_advanceInterrupt(Status status,bool called,const MdioBatch& batch) noexcept {
  const uint8_t last=static_cast<uint8_t>(batch.frameCount-1U);
  const auto value=_transport.lastBatch.frames[_interruptStep==InterruptStep::PIN_MUX?3U:last].value;
  const bool complete=called && _result.failingBatch.completedFrames==batch.frameCount;
  const bool testing=_operation.kind==OperationKind::INTERRUPT_TEST_START || _operation.kind==OperationKind::INTERRUPT_TEST_STOP;
  uint16_t mask=0,expected=0,address=batch.frames[last].registerAddress;
  if (called && batch.frames[last].kind==FrameKind::WRITE) {
    const auto& receipt=_transport.lastBatch.frames[last];
    const bool possible=receipt.effectPossible || receipt.completed || receipt.valueValid || _transport.violation!=TransportViolation::NONE;
    _result.mutationAttempted|=receipt.attempted;
    _result.mutationCompleted|=complete;
    _result.mutationPossible|=possible;
    if (testing && (receipt.attempted || possible)) {
      _interruptTestSession=true; _interrupts.testMayBeActive=true;
      _operation.cleanupOwed=_operation.temporaryMayBeActive=true;
      _cleanupOwner=OperationKind::INTERRUPT_TEST_START;
    }
  }
  // Retain every valid control read, including the read prefix of a failed
  // atomic test RMW. A later write/release failure cannot erase that evidence.
  if (called) {
    for (uint8_t i=0;i<_result.failingBatch.completedFrames;++i) {
      if (batch.frames[i].kind!=FrameKind::READ || batch.frames[i].registerAddress!=0x11U) { continue; }
      const auto raw=_transport.lastBatch.frames[i].value;
      _interrupts.rawPhyscr=raw; _interrupts.physcrValid=true;
      if (testing && ((raw^_interruptControlValue())&_interruptControlMask())!=0U) {
        _interrupts.trust=ConfigurationTrust::DIRTY; _interrupts.observedStale=true;
        if (!_result.mismatch.valid) {
          _result.mismatch={true,false,0,0x11,_interruptControlMask(),_interruptControlValue(),raw};
        }
      }
    }
  }
  if (complete && batch.frames[last].kind==FrameKind::READ) {
    switch (_interruptStep) {
      case InterruptStep::PIN_VERIFY:
        address=static_cast<uint16_t>(0x0303U+ledIndex(_interrupts.desired.pin)); mask=7; expected=4; break;
      case InterruptStep::VERIFY1:
        _interrupts.rawMisr1=value; _interrupts.misr1Valid=true; mask=0x00FF; expected=_interrupts.desired.misr1Mask; break;
      case InterruptStep::VERIFY2:
        _interrupts.rawMisr2=value; _interrupts.misr2Valid=true; mask=0x00FF; expected=_interrupts.desired.misr2Mask; break;
      case InterruptStep::VERIFY_SELECT:
        _interrupts.rawRxfs=value; _interrupts.rxfsValid=true; address=0x04A1; mask=0x1000;
        expected=_interrupts.desired.sharedSource==InterruptSharedSource::WAKE?0x1000:0; break;
      case InterruptStep::CHECK_TEST: case InterruptStep::TEST_VERIFY: case InterruptStep::VERIFY_CONTROL:
        _interrupts.rawPhyscr=value; _interrupts.physcrValid=true;
        if (_interruptStep!=InterruptStep::CHECK_TEST) {
          mask=static_cast<uint16_t>(_interruptControlMask()|4U); expected=_interruptControlValue();
          if (_operation.kind==OperationKind::INTERRUPT_TEST_START) { expected|=4U; }
        }
        break;
      default: break;
    }
    if (mask!=0U && (value&mask)!=expected && !_result.mismatch.valid) {
      _result.mismatch={true,address>31U,31,address,mask,expected,value};
    }
    if (testing && _interruptStep==InterruptStep::TEST_VERIFY &&
        ((value^expected)&_interruptControlMask())!=0U) {
      _interrupts.trust=ConfigurationTrust::DIRTY;
      _interrupts.observedStale=true;
    }
  }
  if (!status.ok()) { return _finishOperation(status); }
  if (_result.mismatch.valid) { return _finishOperation(Status::Error(Err::CONFIG_MISMATCH,"Interrupt readback mismatch")); }
  const auto nextAfterPin=[&]() noexcept {
    if (testing) { _setInterruptStep(_operation.kind==OperationKind::INTERRUPT_TEST_STOP?InterruptStep::TEST_WRITE:InterruptStep::CHECK_TEST); }
    else { _setInterruptStep(_interruptVerifyOnly?InterruptStep::VERIFY_SELECT:
        (_operation.kind==OperationKind::INTERRUPT_CONFIGURE?InterruptStep::CHECK_TEST:InterruptStep::DISABLE)); }
  };
  switch (_interruptStep) {
    case InterruptStep::IDENTIFY: {
      if (_interruptExpectedMode!=OperatingMode::UNKNOWN && _identity.mode!=_interruptExpectedMode) {
        return _finishOperation(Status::Error(Err::INVALID_CONFIG,"Identity changed during interrupt procedure",
                                              static_cast<int32_t>(ConfigDetail::DEVICE_INVALIDATED)));
      }
      if (!_initialized && (_operation.kind==OperationKind::INTERRUPT_CONFIGURE ||
                            _operation.kind==OperationKind::INTERRUPT_TEST_START)) {
        return _finishOperation(invalidIrq("Initialize before interrupt mutation"));
      }
      const auto valid=_validateInterruptSettings(_interrupts.desired); if (!valid.ok()) { return _finishOperation(valid); }
      _interrupts.basicStrapInterrupt=_identity.mode==OperatingMode::BASIC;
      if (_operation.kind==OperationKind::INTERRUPT_TEST_STOP) { _setInterruptStep(InterruptStep::TEST_WRITE); }
      else if (_interrupts.desired.pin==InterruptPin::LED1 && _identity.mode==OperatingMode::ENHANCED) { _setInterruptStep(InterruptStep::PIN_CLOCK); }
      else if (_interrupts.desired.pin!=InterruptPin::INTR_PWRDN) { _setInterruptStep(InterruptStep::PIN_MUX); }
      else { nextAfterPin(); }
      break;
    }
    case InterruptStep::PIN_CLOCK:
      if ((value&1U)==0U) { return _finishOperation(invalidIrq("LED1 is routed to clock output")); }
      _setInterruptStep(InterruptStep::PIN_MUX); break;
    case InterruptStep::PIN_MUX:
      if (ledIndex(_interrupts.desired.pin)>=2U &&
          (_transport.lastBatch.frames[4].value&0x0020U)!=(_config.interface==MacInterface::RMII?0x0020U:0U)) {
        return _finishOperation(invalidIrq("Observed interface differs from LED2 IRQ wiring"));
      }
      if ((value&7U)!=4U) {
        if (_interruptVerifyOnly || testing) {
          return _finishOperation(invalidIrq("Pin owner has not selected interrupt mux"));
        }
        // The IRQ source owner configures and restores its narrow mux field.
        // Global delivery is disabled before changing the route; the pin profile
        // records a dependency and never writes the same field independently.
        _interruptPinRestore=true;
      }
      nextAfterPin(); break;
    case InterruptStep::PIN_APPLY: _setInterruptStep(InterruptStep::PIN_VERIFY); break;
    case InterruptStep::PIN_VERIFY: _interruptPinRestore=false; _setInterruptStep(InterruptStep::DRAIN); break;
    case InterruptStep::CHECK_TEST:
      if ((value&4U)!=0U) {
        _interrupts.testMayBeActive=true;
        return _finishOperation(invalidIrq("Unowned interrupt test already active"));
      }
      _setInterruptStep(testing?InterruptStep::TEST_WRITE:InterruptStep::DISABLE); break;
    case InterruptStep::DISABLE: _setInterruptStep(_interruptPinRestore?InterruptStep::PIN_APPLY:InterruptStep::DRAIN); break;
    case InterruptStep::DRAIN: _setInterruptStep(InterruptStep::SELECT); break;
    case InterruptStep::SELECT: _setInterruptStep(InterruptStep::VERIFY_SELECT); break;
    case InterruptStep::VERIFY_SELECT:
      // Readback never retrospectively labels pending events. Drain crossing history
      // as UNKNOWN, then future samples can use this verified selector epoch.
      _interrupts.selector=InterruptSharedSource::UNKNOWN;
      _setInterruptStep(InterruptStep::CROSSING); break;
    case InterruptStep::CROSSING:
      _interrupts.selector=_interrupts.desired.sharedSource;
      _setInterruptStep(_interruptVerifyOnly?InterruptStep::VERIFY1:InterruptStep::MASK1); break;
    case InterruptStep::MASK1: _setInterruptStep(InterruptStep::MASK2); break;
    case InterruptStep::MASK2: _setInterruptStep(InterruptStep::OUTPUT); break;
    case InterruptStep::OUTPUT: _setInterruptStep(InterruptStep::ENABLE); break;
    case InterruptStep::ENABLE: _setInterruptStep(InterruptStep::VERIFY1); break;
    case InterruptStep::VERIFY1: _setInterruptStep(InterruptStep::VERIFY2); break;
    case InterruptStep::VERIFY2: _setInterruptStep(InterruptStep::VERIFY_CONTROL); break;
    case InterruptStep::VERIFY_CONTROL:
      if (_operation.kind==OperationKind::INTERRUPT_CONFIGURE &&
          !_otherControlsVerified(OperationKind::INTERRUPT_CONFIGURE)) {
        return _finishOperation(invalidIrq("Other owned controls lost verification"));
      }
      _interrupts.trust=ConfigurationTrust::VERIFIED; _interrupts.observedStale=false;
      _interrupts.timeValid=true; _interrupts.observedAtUs=_lastTimeUs+_config.transport.bounds.clockLagUs;
      _interrupts.testMayBeActive=false;
      _result.persistentRestored=true;
      if (_operation.kind!=OperationKind::VERIFY) { _initialized=true; }
      return _finishOperation(Status::Ok());
    case InterruptStep::TEST_WRITE: _setInterruptStep(InterruptStep::TEST_VERIFY); break;
    case InterruptStep::TEST_VERIFY:
      if (_operation.kind==OperationKind::INTERRUPT_TEST_STOP) {
        _interrupts.testMayBeActive=false;
        if (!_initialized || _interrupts.trust!=ConfigurationTrust::VERIFIED ||
            !_otherControlsVerified(OperationKind::INTERRUPT_CONFIGURE)) {
          return _finishOperation(invalidIrq("Test cleared; persistent state still needs restoration"));
        }
        _result.persistentRestored=true;
      }
      return _finishOperation(Status::Ok());
  }
  return _operation.status;
}

Status DP8382x::serviceInterrupts(InterruptServiceSnapshot& snapshot,const InterruptServiceBudget& budget) noexcept {
  if (_config.variant == Variant::DP83825I) { return _unsupportedFeature(); }
  if (_active || _polling) { return busyIrq(BusyDetail::OPERATION_ACTIVE); }
  const auto fail=[&](Status status) noexcept {
    _interruptService.stale=true; _interruptService.morePossiblyPending=true;
    _interruptService.lastStatus=status; return status;
  };
  if (budget.maxTransactions==0U || budget.maxSamples<3U || budget.io.maxFrames<3U) { return fail(serviceBudgetError()); }
  EvidenceSnapshot evidence;
  auto status=acquireEvidence(evidenceBit(EvidenceSource::PHYSTS)|evidenceBit(EvidenceSource::MISR1)|evidenceBit(EvidenceSource::MISR2),evidence,budget.io);
  if (!status.ok()) { return fail(status); }
  InterruptServiceSnapshot next;
  next.rawPhysts=evidence.samples[static_cast<uint8_t>(EvidenceSource::PHYSTS)].raw;
  next.misr1=evidence.samples[static_cast<uint8_t>(EvidenceSource::MISR1)];
  next.misr2=evidence.samples[static_cast<uint8_t>(EvidenceSource::MISR2)];
  next.fcscrRequested=(_evidence.pendingInterruptCounters&1U)!=0U;
  next.recrRequested=(_evidence.pendingInterruptCounters&2U)!=0U;
  const uint32_t extra=(next.fcscrRequested?1U:0U)+(next.recrRequested?1U:0U);
  if (extra!=0U) {
    if (budget.maxTransactions<2U || budget.maxSamples<3U+extra || budget.io.maxFrames<3U+extra) { return fail(serviceBudgetError()); }
    const uint32_t sources=(next.fcscrRequested?evidenceBit(EvidenceSource::FCSCR):0U)|
        (next.recrRequested?evidenceBit(EvidenceSource::RECR):0U);
    status=acquireEvidence(sources,evidence,{budget.io.deadlineUs,budget.io.maxFrames-3U});
    if (!status.ok()) { return fail(status); }
    if (next.fcscrRequested) { next.fcscr=evidence.samples[static_cast<uint8_t>(EvidenceSource::FCSCR)]; }
    if (next.recrRequested) { next.recr=evidence.samples[static_cast<uint8_t>(EvidenceSource::RECR)]; }
  }
  next.valid=true; next.stale=false; next.lastStatus=Status::Ok();
  next.morePossiblyPending=((next.misr1.raw|next.misr2.raw)&0xFF00U)!=0U ||
      budget.maxTransactions==(extra!=0U?2U:1U) || budget.maxSamples==3U+extra || budget.io.maxFrames==3U+extra;
  _interruptService=next; snapshot=next; return Status::Ok();
}
InterruptSnapshot DP8382x::interruptSnapshot() const noexcept {
  auto snapshot=_interrupts;
  if (snapshot.desiredValid && snapshot.desired.pin==InterruptPin::LED1 && !_led1RouteVerified()) {
    snapshot.trust=ConfigurationTrust::DIRTY; snapshot.observedStale=true;
  }
  return snapshot;
}
} // namespace DP8382x
