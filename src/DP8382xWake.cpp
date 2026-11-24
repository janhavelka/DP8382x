/// @file DP8382xWake.cpp
/// @brief Quiesced exact-part Magic Packet configuration and retained wake evidence.
#include <DP8382x/DP8382x.h>

namespace DP8382x {
namespace {
Status wakeConfig(const char* message) noexcept {
  return Status::Error(Err::INVALID_CONFIG,message,static_cast<int32_t>(ConfigDetail::SOURCE_GATE));
}
Status wakeBusy(BusyDetail detail) noexcept {
  return Status::Error(Err::BUSY,"Wake owner unavailable",static_cast<int32_t>(detail));
}
Status wakeSyntax(const WakeSettings& settings) noexcept {
  if (settings.route>WakeRoute::LED3 || settings.pulseWidth>3U) {
    return Status::Error(Err::INVALID_PARAM,"Invalid wake route or pulse encoding",static_cast<int32_t>(ParamDetail::ENUM_VALUE));
  }
  return Status::Ok();
}
uint16_t credentialWord(const WakeSettings& settings,uint8_t index) noexcept {
  const auto* octets=index<3U?settings.mac:settings.password;
  const auto offset=static_cast<uint8_t>((index%3U)*2U);
  // SNLS647I 8-92..97: API octet 0 is transmitted first (datasheet MAC byte 5).
  return static_cast<uint16_t>(uint16_t{octets[offset]}|(uint16_t{octets[offset+1U]}<<8U));
}
MdioBatch wakeRead(uint8_t address) noexcept {
  MdioBatch batch; batch.frameCount=1;
  batch.frames[0]={FrameKind::READ,address,0,0,NO_READ_INDEX}; return batch;
}
bool completePrefix(const MdioBatch& batch,const BatchResult& result,uint8_t last) noexcept {
  for (uint8_t i=0;i<=last;++i) {
    const auto& frame=result.frames[i];
    if (!frame.attempted || !frame.completed || frame.valueValid!=(batch.frames[i].kind==FrameKind::READ)) { return false; }
  }
  return true;
}
}

Status DP8382x::_validateWake(const WakeSettings& settings) const noexcept {
  const auto syntax=wakeSyntax(settings); if (!syntax.ok()) { return syntax; }
  if (!settings.maintenanceAdmitted || !settings.receiveTrafficQuiesced) {
    return wakeConfig("Wake settings require quiesced receive traffic and maintenance admission");
  }
  if (settings.route==WakeRoute::LED0 || settings.route==WakeRoute::LED3) {
    return Status::Error(Err::UNSUPPORTED,"Wake pin route lacks admitted GPIO capability",static_cast<int32_t>(ConfigDetail::SOURCE_GATE));
  }
  if (settings.enabled && _interrupts.desiredValid && _interrupts.desired.sharedSource!=InterruptSharedSource::WAKE) {
    return wakeConfig("Enabled WoL conflicts with the owned polarity interrupt source");
  }
  if (_pins.desiredValid) {
    for (uint8_t i=1;i<=2U;++i) {
      if (_pins.desired.pins[i].function==PinFunction::WAKE &&
          settings.route!=(i==1U?WakeRoute::LED1:WakeRoute::LED2)) {
        return wakeConfig("Release the dependent pin profile before moving the wake route");
      }
    }
  }
  if (settings.route==WakeRoute::STATUS_ONLY) { return Status::Ok(); }
  const uint8_t directPin=settings.route==WakeRoute::LED1?1U:settings.route==WakeRoute::LED2?2U:0U;
  if (_interface.desiredValid &&
      ((directPin==1U && _interface.desired.clockOutput==ClockOutput::CLKOUT25) ||
       (directPin==2U && _interface.desired.collisionFullDuplex))) {
    return wakeConfig("Wake route conflicts with the interface clock/COL owner");
  }
  if (directPin!=0U && _pins.desiredValid &&
      _pins.desired.pins[directPin].function!=PinFunction::UNOWNED &&
      _pins.desired.pins[directPin].function!=PinFunction::WAKE) {
    return wakeConfig("Wake route conflicts with the owned pin profile");
  }
  if (!settings.pinAvailable) { return wakeConfig("Wake output requires explicit pin ownership"); }
  if (settings.route==WakeRoute::INTERRUPT) {
    if (!_interrupts.desiredValid || !_interrupts.desired.globalEnable || (_interrupts.desired.misr2Mask&2U)==0U ||
        (_identity.mode==OperatingMode::ENHANCED && !_interrupts.desired.outputEnable) ||
        _interrupts.desired.sharedSource!=InterruptSharedSource::WAKE ||
        ((_operation.kind==OperationKind::WOL_CONFIGURE || !_wakeWorking) && _interrupts.trust!=ConfigurationTrust::VERIFIED)) {
      return wakeConfig("Wake IRQ requires an owned enabled and verified task27 wake route");
    }
    return _validateInterruptSettings(_interrupts.desired);
  }
  if (_eee.desiredValid && _eee.desired.enabled && settings.route==WakeRoute::LED1) {
    return wakeConfig("Disable EEE before conflicting LED1 clock/wake routing");
  }
  if (_identity.mode==OperatingMode::BASIC && settings.route==WakeRoute::LED2) {
    return Status::Error(Err::UNSUPPORTED,"BASIC has no LED2 wake output");
  }
  if (_identity.mode==OperatingMode::ENHANCED && settings.route==WakeRoute::LED1 && !settings.led1ClockUnused) {
    return wakeConfig("LED1 clock consumer conflicts with wake output");
  }
  if (_identity.mode==OperatingMode::ENHANCED && settings.route==WakeRoute::LED2 &&
      _config.interface==MacInterface::MII && !settings.colUnused) {
    return wakeConfig("MII COL consumer conflicts with LED2 wake output");
  }
  if (_interrupts.desiredValid &&
      ((settings.route==WakeRoute::LED1 && _interrupts.desired.pin==InterruptPin::LED1) ||
       (settings.route==WakeRoute::LED2 && _interrupts.desired.pin==InterruptPin::LED2))) {
    return wakeConfig("Wake GPIO and interrupt owner require different pin functions");
  }
  return Status::Ok();
}

Status DP8382x::startConfigureWake(const WakeSettings& settings,const OperationBudget& budget,OperationToken& token) noexcept {
  if (isA(_config.variant)) { return _unsupportedFeature(); }
  auto status=wakeSyntax(settings); if (!status.ok()) { return status; }
  if (budget.pollIntervalUs==0U || budget.maxStalledPolls==0U) {
    return Status::Error(Err::INVALID_PARAM,"Invalid wake budget",static_cast<int32_t>(ParamDetail::BUDGET_RANGE));
  }
  if (!_bound) { return Status::Error(Err::NOT_BOUND,"No transport binding"); }
  if (_active || _polling || _operation.state==OperationState::RUNNING) { return wakeBusy(BusyDetail::OPERATION_ACTIVE); }
  if (_operation.state==OperationState::RESULT_PENDING) { return wakeBusy(BusyDetail::RESULT_PENDING); }
  if (_transport.clockInvalid) { return Status::Error(Err::INVALID_CONFIG,"Clock contract failed",static_cast<int32_t>(ConfigDetail::CLOCK_CONTRACT)); }
  if (_operation.cleanupOwed || _operation.temporaryMayBeActive) { return wakeBusy(BusyDetail::CLEANUP_REQUIRED); }
  if (!_initialized || !capabilities().valid) { return Status::Error(Err::NOT_INITIALIZED,"Initialize before wake configuration"); }
  status=_validateWake(settings); if (!status.ok()) { return status; }
  if (!_otherControlsVerified(OperationKind::WOL_CONFIGURE)) { return wakeConfig("Verify other owned controls first"); }
  if (budget.io.maxFrames==0U || budget.io.deadlineUs==0U) {
    return Status::Error(Err::OPERATION_TIMEOUT,"Empty wake allowance",static_cast<int32_t>(budget.io.maxFrames==0U?TimeoutDetail::FRAME_BUDGET:TimeoutDetail::DEADLINE));
  }
  _beginOperation(OperationKind::WOL_CONFIGURE,budget);
  _wake.desired=settings; _wake.desiredValid=true;
  _wake.trust=ConfigurationTrust::DIRTY; _wake.observedStale=true;
  _wakeTailPending=false; _wakeVerifyOnly=false;
  _setWakeStep(WakeStep::IDENTIFY);
  token=_operation.token; return _operation.status;
}

Status DP8382x::acknowledgeWake(const OperationBudget& budget,OperationToken& token) noexcept {
  if (isA(_config.variant)) { return _unsupportedFeature(); }
  if (budget.pollIntervalUs==0U || budget.maxStalledPolls==0U) {
    return Status::Error(Err::INVALID_PARAM,"Invalid wake acknowledgement budget",static_cast<int32_t>(ParamDetail::BUDGET_RANGE));
  }
  if (!_bound) { return Status::Error(Err::NOT_BOUND,"No transport binding"); }
  if (_active || _polling || _operation.state==OperationState::RUNNING) { return wakeBusy(BusyDetail::OPERATION_ACTIVE); }
  if (_operation.state==OperationState::RESULT_PENDING) { return wakeBusy(BusyDetail::RESULT_PENDING); }
  if (_transport.clockInvalid) { return Status::Error(Err::INVALID_CONFIG,"Clock contract failed",static_cast<int32_t>(ConfigDetail::CLOCK_CONTRACT)); }
  if (_operation.cleanupOwed || _operation.temporaryMayBeActive) { return wakeBusy(BusyDetail::CLEANUP_REQUIRED); }
  if (!_initialized || !capabilities().valid) { return Status::Error(Err::NOT_INITIALIZED,"Initialize before wake acknowledgement"); }
  if (!_wake.desiredValid || !_wake.desired.levelMode || _wake.trust!=ConfigurationTrust::VERIFIED ||
      !_otherControlsVerified(OperationKind::NONE)) { return wakeConfig("Acknowledgement requires known owned level mode and verified controls"); }
  if (budget.io.maxFrames==0U || budget.io.deadlineUs==0U) {
    return Status::Error(Err::OPERATION_TIMEOUT,"Empty wake allowance",static_cast<int32_t>(budget.io.maxFrames==0U?TimeoutDetail::FRAME_BUDGET:TimeoutDetail::DEADLINE));
  }
  _beginOperation(OperationKind::WOL_ACKNOWLEDGE,budget);
  _wakeTailPending=false; _wakeVerifyOnly=false;
  _setWakeStep(WakeStep::IDENTIFY);
  token=_operation.token; return _operation.status;
}

uint16_t DP8382x::_wakeControl() const noexcept {
  const auto& settings=_wake.desired;
  return static_cast<uint16_t>((settings.crcGate?0x1000U:0U)|(uint16_t{settings.pulseWidth}<<9U)|
      (settings.levelMode?0x0100U:0U)|(settings.enabled?0x0081U:0U)|
      (settings.enabled && settings.secureOn?0x0020U:0U));
}
uint8_t DP8382x::_wakePin() const noexcept {
  if (_wake.desired.route==WakeRoute::LED1) { return 1; }
  if (_wake.desired.route==WakeRoute::LED2) { return 2; }
  if (_wake.desired.route==WakeRoute::INTERRUPT && _interrupts.desiredValid) {
    if (_interrupts.desired.pin==InterruptPin::LED1) { return 1; }
    if (_interrupts.desired.pin==InterruptPin::LED2) { return 2; }
  }
  return 0;
}
uint16_t DP8382x::_wakePinFunction() const noexcept {
  return _wake.desired.route==WakeRoute::INTERRUPT?uint16_t{4}:uint16_t{2};
}
void DP8382x::_retainWakeControl(uint16_t value) noexcept {
  _wake.rawControl=value; _wake.controlValid=true;
  _wake.enabled=(value&0x0080U)!=0U; _wake.secureOn=(value&0x0020U)!=0U;
  _wake.magicMatch=(value&0x0001U)!=0U; _wake.crcGate=(value&0x1000U)!=0U;
  _wake.levelMode=(value&0x0100U)!=0U; _wake.pulseWidth=static_cast<uint8_t>((value>>9U)&3U);
}
void DP8382x::_retainWakeWord(uint16_t address,uint16_t value) noexcept {
  const bool pin=_wake.desiredValid && _wakePin()!=0U && address==(_wakePin()==1U?0x0304U:0x0305U);
  if (!pin && (address<0x04A0U || address>0x04A7U)) { return; }
  _wake.observedStale=true;
  uint16_t mask=0,expected=0;
  if (pin) { _wake.rawPinControl=value; _wake.pinControlValid=true; mask=7; expected=_wakePinFunction(); }
  else if (address==0x04A0U) {
    _retainWakeControl(value); mask=0x17A1; expected=_wakeControl();
  } else if (address>=0x04A2U && address<=0x04A7U) {
    const auto index=static_cast<uint8_t>(address-0x04A2U);
    if (index<3U) { _wake.rawMac[index]=value; } else { _wake.rawPassword[index-3U]=value; }
    _wake.credentialValidMask|=static_cast<uint8_t>(1U<<index);
    mask=0xFFFF; expected=credentialWord(_wake.desired,index);
  } else if (address==0x04A1U) {
    _wake.rawRxfs=value; _wake.rxfsValid=true;
    if (_wake.desired.enabled) { mask=0x1000; expected=0x1000; }
  }
  const bool contradiction=_wake.desiredValid && mask!=0U && (value&mask)!=expected;
  if (contradiction) {
    _wake.trust=ConfigurationTrust::DIRTY;
    if (_wakeWorking && _wakeStep==WakeStep::READ_CREDENTIAL && address>=0x04A2U) { _wakeCredentialsChanged=true; }
    if (_wakeWorking && pin && (_wakeStep==WakeStep::PIN_MUX || _wakeStep==WakeStep::READ_CONTROL || _wakeStep==WakeStep::READ_CREDENTIAL)) {
      _wakePinNeedsApply=true;
    }
  }
  // A late external observation cannot erase an earlier verification phase.
  // Transient disable/read-before-write values are intentionally not compared here.
  if (contradiction && _wakeWorking && !_result.mismatch.valid &&
      (_wakeVerifyOnly || _operation.kind==OperationKind::WOL_ACKNOWLEDGE ||
       _wakeStep==WakeStep::READ_SELECTOR || _wakeStep==WakeStep::DRAIN_SELECTOR ||
       (pin && (_wakeStep==WakeStep::PIN_VERIFY || _wakeStep==WakeStep::WRITE_CREDENTIAL || _wakeStep==WakeStep::VERIFY_CREDENTIAL ||
                _wakeStep==WakeStep::HARVEST_APPLY || _wakeStep==WakeStep::APPLY || _wakeStep==WakeStep::VERIFY_CONTROL)) ||
       (address>=0x04A2U && (_wakeStep==WakeStep::VERIFY_CREDENTIAL ||
        _wakeStep==WakeStep::HARVEST_APPLY || _wakeStep==WakeStep::APPLY || _wakeStep==WakeStep::VERIFY_CONTROL)))) {
    _result.mismatch={true,true,31,address,mask,expected,value};
  }
}
void DP8382x::_setWakeStep(WakeStep step) noexcept {
  _pinsWorking=false; _interfaceWorking=false; _qualityWorking=false;
  _wakeWorking=true; _eeeWorking=_powerWorking=_interruptWorking=_cableWorking=_bistWorking=_loopWorking=_configuring=false;
  _wakeStep=step; _operation.nextPollValid=false;
  _operation.phase=step==WakeStep::IDENTIFY?OperationPhase::IDENTIFY:OperationPhase::VERIFY;
  if (step==WakeStep::IDENTIFY) { _wakeExpectedMode=_identity.mode; _wakeWord=0; _wakeCredentialsChanged=false; _wakePinNeedsApply=false; }
}
MdioBatch DP8382x::_wakeBatch() const noexcept {
  switch (_wakeStep) {
    case WakeStep::IDENTIFY: {
      auto batch=wakeRead(2); batch.frameCount=2; batch.frames[1]={FrameKind::READ,3,0,0,NO_READ_INDEX}; return batch;
    }
    case WakeStep::PIN_CLOCK: return _mmdBatch(0x0308,FrameKind::READ,0);
    case WakeStep::PIN_MUX: case WakeStep::PIN_VERIFY: {
      auto batch=_mmdBatch(_wakePin()==1U?0x0304:0x0305,FrameKind::READ,0);
      if (_wakeStep==WakeStep::PIN_MUX && _wakePin()==2U) {
        batch.frames[batch.frameCount++]={FrameKind::READ,0x17,0,0,NO_READ_INDEX};
      }
      return batch;
    }
    case WakeStep::PIN_APPLY: {
      auto batch=_mmdBatch(_wakePin()==1U?0x0304:0x0305,FrameKind::READ,0); batch.frameCount=5;
      // Only the documented GPIO function field belongs to this narrow owner.
      batch.frames[4]={FrameKind::WRITE,0x0E,_wakePinFunction(),0xFFF8,3}; return batch;
    }
    case WakeStep::READ_CONTROL: case WakeStep::VERIFY_DISABLE: case WakeStep::VERIFY_CONTROL:
      return _mmdBatch(0x04A0,FrameKind::READ,0);
    case WakeStep::READ_CREDENTIAL: case WakeStep::VERIFY_CREDENTIAL:
      return _mmdBatch(static_cast<uint16_t>(0x04A2U+_wakeWord),FrameKind::READ,0);
    case WakeStep::WRITE_CREDENTIAL:
      return _mmdBatch(static_cast<uint16_t>(0x04A2U+_wakeWord),FrameKind::WRITE,credentialWord(_wake.desired,_wakeWord));
    case WakeStep::HARVEST_DISABLE: case WakeStep::HARVEST_APPLY: case WakeStep::HARVEST_ACK: {
      auto batch=_mmdBatch(0x04A1,FrameKind::READ,0); batch.frameCount=5;
      batch.frames[4]={FrameKind::READ,0x13,0,0,NO_READ_INDEX}; return batch;
    }
    case WakeStep::DISABLE: return _wakeControlBatch(0x00A1,0,false);
    case WakeStep::APPLY: return _wakeControlBatch(0x17A1,_wakeControl(),false);
    case WakeStep::ACK: return _wakeControlBatch(0,0,true);
    case WakeStep::READ_SELECTOR: return _mmdBatch(0x04A1,FrameKind::READ,0);
    case WakeStep::DRAIN_SELECTOR: return wakeRead(0x13);
  }
  return {};
}

Status DP8382x::_advanceWake(Status status,bool called,const MdioBatch& batch) noexcept {
  const bool ack=_operation.kind==OperationKind::WOL_ACKNOWLEDGE;
  const uint8_t last=static_cast<uint8_t>(batch.frameCount-1U);
  const bool complete=called && _result.failingBatch.completedFrames==batch.frameCount;
  const uint16_t value=complete?_transport.lastBatch.frames[_wakeStep==WakeStep::PIN_MUX?3U:last].value:0;
  // Keep raw history, but never reuse an older word as the result of this
  // requested acquisition. Valid prefixes survive a later write/release error.
  const bool targetRead=called && batch.frameCount>=4U && batch.frames[3].kind==FrameKind::READ &&
      completePrefix(batch,_transport.lastBatch,3);
  if (batch.frameCount>=4U && batch.frames[3].kind==FrameKind::READ && !targetRead) {
    const auto address=batch.frames[1].value;
    if (address==0x04A0U) { _wake.controlValid=false; }
    else if (address>=0x04A2U && address<=0x04A7U) {
      _wake.credentialValidMask&=static_cast<uint8_t>(~(1U<<(address-0x04A2U)));
    } else if (address==0x0304U || address==0x0305U) { _wake.pinControlValid=false; }
    else if (address==0x04A1U) { _wake.rxfsValid=false; }
  }
  if ((_wakeStep==WakeStep::HARVEST_DISABLE || _wakeStep==WakeStep::HARVEST_APPLY ||
       _wakeStep==WakeStep::HARVEST_ACK || _wakeStep==WakeStep::DRAIN_SELECTOR) && !complete) {
    _wake.misr2Valid=false;
  }
  if (called && batch.frames[last].kind==FrameKind::WRITE) {
    const auto& target=_transport.lastBatch.frames[last];
    _result.mutationAttempted|=target.attempted;
    _result.mutationCompleted|=complete;
    _result.mutationPossible|=target.effectPossible || target.completed || target.valueValid || _transport.violation!=TransportViolation::NONE;
  }
  if (targetRead) {
    const auto raw=_transport.lastBatch.frames[3].value;
    if (_wakeStep==WakeStep::READ_CONTROL || _wakeStep==WakeStep::VERIFY_DISABLE || _wakeStep==WakeStep::VERIFY_CONTROL ||
        _wakeStep==WakeStep::DISABLE || _wakeStep==WakeStep::APPLY || _wakeStep==WakeStep::ACK) {
      _retainWakeControl(raw); _wake.observedStale=true;
    }
    if (_wakeStep==WakeStep::READ_CREDENTIAL || _wakeStep==WakeStep::VERIFY_CREDENTIAL) {
      if (_wakeWord<3U) { _wake.rawMac[_wakeWord]=raw; } else { _wake.rawPassword[_wakeWord-3U]=raw; }
      _wake.credentialValidMask|=static_cast<uint8_t>(1U<<_wakeWord);
      if (raw!=credentialWord(_wake.desired,_wakeWord)) {
        _wakeCredentialsChanged=true;
        if (_wakeVerifyOnly || _wakeStep==WakeStep::VERIFY_CREDENTIAL) {
          _wake.trust=ConfigurationTrust::DIRTY;
          if (!_result.mismatch.valid) { _result.mismatch={true,true,31,static_cast<uint16_t>(0x04A2U+_wakeWord),0xFFFF,credentialWord(_wake.desired,_wakeWord),raw}; }
        }
      }
    }
    if (_wakeStep==WakeStep::HARVEST_DISABLE || _wakeStep==WakeStep::HARVEST_APPLY ||
        _wakeStep==WakeStep::HARVEST_ACK || _wakeStep==WakeStep::READ_SELECTOR) {
      _wake.rawRxfs=raw; _wake.rxfsValid=true;
    }
    uint16_t mask=0,expected=0;
    if (_wakeStep==WakeStep::VERIFY_DISABLE) { mask=0x00A1; }
    else if (_wakeStep==WakeStep::VERIFY_CONTROL || (_wakeStep==WakeStep::READ_CONTROL && (_wakeVerifyOnly || ack)) ||
             _wakeStep==WakeStep::ACK) { mask=0x17A1; expected=_wakeControl(); }
    if (mask!=0U && (raw&mask)!=expected) {
      _wake.trust=ConfigurationTrust::DIRTY;
      if (!_result.mismatch.valid) { _result.mismatch={true,true,31,0x04A0,mask,expected,raw}; }
    }
  }
  if (complete && (_wakeStep==WakeStep::HARVEST_DISABLE || _wakeStep==WakeStep::HARVEST_APPLY ||
      _wakeStep==WakeStep::HARVEST_ACK || _wakeStep==WakeStep::DRAIN_SELECTOR)) {
    _wake.rawMisr2=value; _wake.misr2Valid=true;
  }
  if (!status.ok()) { return _finishOperation(status); }
  if (_result.mismatch.valid) { return _finishOperation(Status::Error(Err::CONFIG_MISMATCH,"Wake readback mismatch")); }
  const auto afterPins=[&]() noexcept { _setWakeStep(WakeStep::READ_CONTROL); };
  const auto afterDisable=[&]() noexcept {
    _wakeWord=0;
    _setWakeStep(_wakePinNeedsApply?WakeStep::PIN_APPLY:_wakeCredentialsChanged?WakeStep::WRITE_CREDENTIAL:
        (_wake.rawControl&0x17E1U)==static_cast<uint16_t>(_wakeControl()|(_wake.rawControl&0x0040U))?WakeStep::READ_SELECTOR:WakeStep::HARVEST_APPLY);
  };
  switch (_wakeStep) {
    case WakeStep::IDENTIFY: {
      if (_wakeExpectedMode!=OperatingMode::UNKNOWN && _wakeExpectedMode!=_identity.mode) {
        return _finishOperation(Status::Error(Err::INVALID_CONFIG,"Identity changed during wake procedure",static_cast<int32_t>(ConfigDetail::DEVICE_INVALIDATED)));
      }
      if (!_initialized && (_operation.kind==OperationKind::WOL_CONFIGURE || ack)) { return _finishOperation(wakeConfig("Initialize before wake mutation")); }
      const auto valid=_validateWake(_wake.desired); if (!valid.ok()) { return _finishOperation(valid); }
      if (ack) { afterPins(); }
      else if (_wakePin()==1U && _identity.mode==OperatingMode::ENHANCED) { _setWakeStep(WakeStep::PIN_CLOCK); }
      else if (_wakePin()!=0U) { _setWakeStep(WakeStep::PIN_MUX); }
      else { afterPins(); }
      break;
    }
    case WakeStep::PIN_CLOCK:
      if ((value&1U)==0U) { return _finishOperation(wakeConfig("LED1 clock output remains active")); }
      _setWakeStep(WakeStep::PIN_MUX); break;
    case WakeStep::PIN_MUX:
      if (_wakePin()==2U &&
          (_transport.lastBatch.frames[4].value&0x0020U)!=(_config.interface==MacInterface::RMII?0x0020U:0U)) {
        return _finishOperation(wakeConfig("Observed interface differs from LED2 wake wiring"));
      }
      _wakePinNeedsApply=(value&7U)!=_wakePinFunction();
      if (_wakeVerifyOnly && _wakePinNeedsApply) {
        _result.mismatch={true,true,31,static_cast<uint16_t>(_wakePin()==1U?0x0304:0x0305),7,_wakePinFunction(),value};
        return _finishOperation(Status::Error(Err::CONFIG_MISMATCH,"Wake pin function differs"));
      }
      afterPins(); break;
    case WakeStep::PIN_APPLY: _setWakeStep(WakeStep::PIN_VERIFY); break;
    case WakeStep::PIN_VERIFY:
      if ((value&7U)!=_wakePinFunction()) {
        _result.mismatch={true,true,31,static_cast<uint16_t>(_wakePin()==1U?0x0304:0x0305),7,_wakePinFunction(),value};
        return _finishOperation(Status::Error(Err::CONFIG_MISMATCH,"Wake pin function readback mismatch"));
      }
      _wakePinNeedsApply=false; afterDisable(); break;
    case WakeStep::READ_CONTROL:
      if (ack) { _setWakeStep(WakeStep::HARVEST_ACK); }
      else { _wakeWord=0; _wakeCredentialsChanged=false; _setWakeStep(WakeStep::READ_CREDENTIAL); }
      break;
    case WakeStep::READ_CREDENTIAL:
      if (++_wakeWord<6U) { _setWakeStep(WakeStep::READ_CREDENTIAL); }
      else if (_wakeVerifyOnly || (!_wakePinNeedsApply && !_wakeCredentialsChanged && (_wake.rawControl&0x17E1U)==static_cast<uint16_t>(_wakeControl()|(_wake.rawControl&0x0040U)))) { _setWakeStep(WakeStep::READ_SELECTOR); }
      else if ((_wake.rawControl&0x00A1U)!=0U) { _setWakeStep(WakeStep::HARVEST_DISABLE); }
      else { afterDisable(); }
      break;
    case WakeStep::HARVEST_DISABLE: _setWakeStep(WakeStep::DISABLE); break;
    case WakeStep::DISABLE: _setWakeStep(WakeStep::VERIFY_DISABLE); break;
    case WakeStep::VERIFY_DISABLE: afterDisable(); break;
    case WakeStep::WRITE_CREDENTIAL:
      if (++_wakeWord<6U) { _setWakeStep(WakeStep::WRITE_CREDENTIAL); }
      else { _wakeWord=0; _setWakeStep(WakeStep::VERIFY_CREDENTIAL); }
      break;
    case WakeStep::VERIFY_CREDENTIAL:
      if (++_wakeWord<6U) { _setWakeStep(WakeStep::VERIFY_CREDENTIAL); }
      else if ((_wake.rawControl&0x17E1U)==static_cast<uint16_t>(_wakeControl()|(_wake.rawControl&0x0040U))) { _setWakeStep(WakeStep::READ_SELECTOR); }
      else { _setWakeStep(WakeStep::HARVEST_APPLY); }
      break;
    case WakeStep::HARVEST_APPLY: _setWakeStep(WakeStep::APPLY); break;
    case WakeStep::APPLY: _setWakeStep(WakeStep::VERIFY_CONTROL); break;
    case WakeStep::HARVEST_ACK: _setWakeStep(WakeStep::ACK); break;
    case WakeStep::ACK: _setWakeStep(WakeStep::VERIFY_CONTROL); break;
    case WakeStep::VERIFY_CONTROL: _setWakeStep(WakeStep::READ_SELECTOR); break;
    case WakeStep::READ_SELECTOR:
      if (_wake.desired.enabled && (_wake.rawRxfs&0x1000U)==0U) {
        _wake.trust=ConfigurationTrust::DIRTY;
        if (!_result.mismatch.valid) { _result.mismatch={true,true,31,0x04A1,0x1000,0x1000,_wake.rawRxfs}; }
        return _finishOperation(Status::Error(Err::CONFIG_MISMATCH,"WoL source selector differs"));
      }
      // Never relabel a pending event with a selector proved only afterward.
      _interrupts.selector=InterruptSharedSource::UNKNOWN;
      _setWakeStep(WakeStep::DRAIN_SELECTOR); break;
    case WakeStep::DRAIN_SELECTOR:
      _interrupts.selector=(_wake.rawRxfs&0x1000U)!=0U?InterruptSharedSource::WAKE:InterruptSharedSource::POLARITY;
      if (!ack) { _wake.trust=ConfigurationTrust::VERIFIED; }
      _wake.observedStale=false; _wake.timeValid=true; _wake.observedAtUs=_lastTimeUs+_config.transport.bounds.clockLagUs;
      _wake.lastStatus=Status::Ok(); _result.persistentRestored=true;
      if (_operation.kind!=OperationKind::VERIFY) { _initialized=true; }
      return _finishOperation(Status::Ok());
  }
  return _operation.status;
}

Status DP8382x::readWakeStatus(WakeSnapshot& snapshot,const IoBudget& budget) noexcept {
  if (isA(_config.variant)) { return _unsupportedFeature(); }
  if (!_bound) { return Status::Error(Err::NOT_BOUND,"No transport binding"); }
  if (_active || _polling) { return wakeBusy(BusyDetail::OPERATION_ACTIVE); }
  const auto fail=[&](Status status) noexcept {
    _wake.observedStale=true; _wake.lastStatus=status; return status;
  };
  if (budget.maxFrames<33U) { return fail(Status::Error(Err::OPERATION_TIMEOUT,"Wake observation requires33frames",static_cast<int32_t>(TimeoutDetail::FRAME_BUDGET))); }
  if (_transport.clockInvalid) { return fail(Status::Error(Err::INVALID_CONFIG,"Clock contract failed",static_cast<int32_t>(ConfigDetail::CLOCK_CONTRACT))); }
  if (_stopped) { return fail(wakeBusy(BusyDetail::CONTROLLER_QUARANTINED)); }
  const auto quiet=_checkQuiet(); if (!quiet.ok()) { return fail(quiet); }
  uint64_t raw=0,upper=0;
  _active=true; const auto clock=_sampleTime(raw,upper); _active=false;
  if (!clock.ok()) { return fail(clock); }
  if (raw>=budget.deadlineUs) {
    return fail(Status::Error(Err::OPERATION_TIMEOUT,"Wake observation deadline expired",static_cast<int32_t>(TimeoutDetail::DEADLINE)));
  }
  const auto& bounds=_config.transport.bounds;
  const uint64_t bound=uint64_t{8}*bounds.fixedUs+uint64_t{33}*bounds.frameUs;
  if (upper>=budget.deadlineUs || bound>=budget.deadlineUs-upper) {
    return fail(Status::Error(Err::OPERATION_TIMEOUT,"Wake observation cannot fit",static_cast<int32_t>(TimeoutDetail::BOUND_ADMISSION)));
  }
  IoBudget remaining=budget;
  // This bounded pass samples detector/status words, not the separately owned
  // GPIO mux. Keep its raw historical word without publishing it as refreshed.
  _wake.pinControlValid=false;
  _wake.observedStale=true;
  for (uint8_t phase=0;phase<8U;++phase) {
    auto batch=_mmdBatch(phase==0U?0x04A0:phase==7U?0x04A1:static_cast<uint16_t>(0x04A1U+phase),FrameKind::READ,0);
    if (phase==7U) { batch.frameCount=5; batch.frames[4]={FrameKind::READ,0x13,0,0,NO_READ_INDEX}; }
    if (phase==0U) { _wake.controlValid=false; }
    else if (phase==7U) { _wake.rxfsValid=false; _wake.misr2Valid=false; }
    else { _wake.credentialValidMask&=static_cast<uint8_t>(~(1U<<(phase-1U))); }
    bool called=false;
    const auto status=_execute(batch,remaining,&called);
    if (called && completePrefix(batch,_transport.lastBatch,3)) {
      const auto word=_transport.lastBatch.frames[3].value;
      if (phase==0U) {
        _retainWakeControl(word);
        if (_wake.desiredValid && (word&0x17A1U)!=_wakeControl()) { _wake.trust=ConfigurationTrust::DIRTY; }
      } else if (phase==7U) { _wake.rawRxfs=word; _wake.rxfsValid=true; }
      else {
        const auto index=static_cast<uint8_t>(phase-1U);
        if (index<3U) { _wake.rawMac[index]=word; } else { _wake.rawPassword[index-3U]=word; }
        _wake.credentialValidMask|=static_cast<uint8_t>(1U<<index);
        if (_wake.desiredValid && word!=credentialWord(_wake.desired,index)) { _wake.trust=ConfigurationTrust::DIRTY; }
      }
    }
    if (phase==7U && called && completePrefix(batch,_transport.lastBatch,4)) {
      _wake.rawMisr2=_transport.lastBatch.frames[4].value; _wake.misr2Valid=true;
    }
    _wake.lastStatus=status;
    if (!status.ok()) { return status; }
    remaining.maxFrames-=batch.frameCount;
  }
  _wake.observedStale=false; _wake.timeValid=true; _wake.observedAtUs=_lastTimeUs+_config.transport.bounds.clockLagUs;
  snapshot=wakeSnapshot(); return Status::Ok();
}
WakeSnapshot DP8382x::wakeSnapshot() const noexcept {
  auto snapshot=_wake;
  if (snapshot.desiredValid && _wakePin()==1U && !_led1RouteVerified()) {
    snapshot.trust=ConfigurationTrust::DIRTY; snapshot.observedStale=true;
  }
  return snapshot;
}
} // namespace DP8382x
