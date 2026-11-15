/// @file DP8382xPins.cpp
/// @brief Exact-part LED/GPIO profiles and the shared bounded restoration pass.
#include <DP8382x/DP8382x.h>

namespace DP8382x {
namespace {
constexpr uint8_t PIN_ROWS=10;
Status pinConfig(const char* message) noexcept {
  return Status::Error(Err::INVALID_CONFIG,message,static_cast<int32_t>(ConfigDetail::SOURCE_GATE));
}
Status pinBusy(BusyDetail detail) noexcept {
  return Status::Error(Err::BUSY,"Pin owner unavailable",static_cast<int32_t>(detail));
}
Status pinTimeout(TimeoutDetail detail) noexcept {
  return Status::Error(Err::OPERATION_TIMEOUT,"Pin allowance exhausted",static_cast<int32_t>(detail));
}
Status pinSyntax(const PinSettings& settings) noexcept {
  if (settings.blinkCode>3U || (settings.availableMask&0xF0U)!=0U) {
    return Status::Error(Err::INVALID_PARAM,"Invalid pin/global encoding",static_cast<int32_t>(ParamDetail::ENUM_VALUE));
  }
  for (const auto& pin:settings.pins) {
    if (pin.function>PinFunction::CLOCK || pin.source>LedSource::PRBS_ERROR ||
        pin.polarity>LedPolarity::ACTIVE_HIGH || pin.drive>LedDrive::HIGH) {
      return Status::Error(Err::INVALID_PARAM,"Invalid LED encoding",static_cast<int32_t>(ParamDetail::ENUM_VALUE));
    }
  }
  return Status::Ok();
}
bool collision(PinFunction function) noexcept {
  return function==PinFunction::COLLISION3 || function==PinFunction::COLLISION5 || function==PinFunction::COLLISION6;
}
uint16_t muxValue(PinFunction function) noexcept {
  switch (function) {
    case PinFunction::WAKE: return 2;
    case PinFunction::INTERRUPT: return 4;
    case PinFunction::COLLISION3: return 3;
    case PinFunction::COLLISION5: return 5;
    case PinFunction::COLLISION6: return 6;
    case PinFunction::HIGH: return 7;
    default: return 0;
  }
}
int pinWord(uint16_t address) noexcept {
  constexpr uint16_t addresses[]={0x0018,0x0019,0x0025,0x0460,0x0469,0x0302,0x0303,0x0304,0x0305,0x0306};
  for (unsigned i=0;i<10U;++i) { if (addresses[i]==address) { return static_cast<int>(i); } }
  return -1;
}
bool prefixComplete(const MdioBatch& batch,const BatchResult& receipt,uint8_t last) noexcept {
  for (uint8_t i=0;i<=last;++i) {
    const auto& frame=receipt.frames[i];
    if (!frame.attempted || !frame.completed || frame.valueValid!=(batch.frames[i].kind==FrameKind::READ)) { return false; }
  }
  return true;
}
}

Status DP8382x::_validatePins(const PinSettings& settings) const noexcept {
  const auto syntax=pinSyntax(settings); if (!syntax.ok()) { return syntax; }
  if (!settings.maintenanceAdmitted || !settings.macStopped || !settings.outputsQuiesced) {
    return pinConfig("Pin changes require stopped MAC and quiesced output admission");
  }
  const bool basic=_identity.mode==OperatingMode::BASIC;
  if (_interface.desiredValid) {
    if (settings.led1ClockOverride && _interface.desired.clockOutput!=ClockOutput::UNOWNED) {
      return pinConfig("Interface owner already owns the LED1 clock override");
    }
    if (settings.pins[1].function!=PinFunction::UNOWNED && _interface.desired.clockOutput==ClockOutput::CLKOUT25) {
      return pinConfig("LED1 is owned as a clock output");
    }
    const auto role=settings.pins[2].function;
    if (_interface.desired.collisionFullDuplex && role!=PinFunction::UNOWNED && !collision(role)) {
      return pinConfig("Interface owner requires the MII COL output");
    }
  }
  if (basic && (settings.led0SwapOwned || settings.led1ClockOverride)) {
    return pinConfig("BASIC polarity override and clock selection remain source-gated");
  }
  if (settings.led0SwapOwned && settings.pins[0].function!=PinFunction::LED) {
    return pinConfig("LED0 swap requires ownership of LED0");
  }
  if (settings.led1ClockOverride && settings.pins[1].function==PinFunction::UNOWNED) {
    return pinConfig("Clock override requires ownership of LED1");
  }
  for (uint8_t i=0;i<4U;++i) {
    const auto& pin=settings.pins[i];
    if (pin.function!=PinFunction::LED && (pin.source!=LedSource::LINK ||
        pin.polarity!=LedPolarity::PRESERVE || pin.drive!=LedDrive::NORMAL)) {
      return pinConfig("LED-only controls require the LED function");
    }
    if (pin.function==PinFunction::UNOWNED) { continue; }
    if ((settings.availableMask&(1U<<i))==0U) { return pinConfig("Selected pin still has a board consumer"); }
    if ((basic && i>=2U) || (!basic && _config.interface==MacInterface::MII && i==3U)) {
      return Status::Error(Err::UNSUPPORTED,"Pin is unavailable in the declared mode/interface");
    }
    if (pin.function==PinFunction::TX_ERROR || pin.function==PinFunction::CLOCK) {
      return Status::Error(Err::UNSUPPORTED,"Pin clock/TX_ER interpretation remains source-gated",static_cast<int32_t>(ConfigDetail::SOURCE_GATE));
    }
    if ((i==0U || i==3U) && pin.function!=PinFunction::LED) {
      return Status::Error(Err::UNSUPPORTED,"Read-only pin mux cannot be reassigned",static_cast<int32_t>(ConfigDetail::SOURCE_GATE));
    }
    if ((basic && pin.polarity!=LedPolarity::PRESERVE) ||
        (i==3U && (pin.polarity!=LedPolarity::PRESERVE || pin.drive!=LedDrive::NORMAL))) {
      return pinConfig("Polarity/drive is not programmable for this pin/mode");
    }
    if (collision(pin.function) && (i!=2U || _config.interface!=MacInterface::MII)) {
      return pinConfig("COL mux codes apply only to MII LED2");
    }
    if (pin.function==PinFunction::HIGH && i!=2U) { return pinConfig("Constant-high mux is documented only on LED2"); }
    if (i==1U && !basic && (!settings.led1ClockUnused || (_eee.desiredValid && _eee.desired.enabled))) {
      return pinConfig("LED1 conflicts with a clock consumer or the owned EEE recipe");
    }
    if (i==2U && _config.interface==MacInterface::MII && !collision(pin.function) && !settings.colUnused) {
      return pinConfig("LED2 non-COL routing requires the MII COL consumer stopped");
    }
    const bool irqPin=_interrupts.desiredValid &&
        ((i==1U && _interrupts.desired.pin==InterruptPin::LED1) ||
         (i==2U && _interrupts.desired.pin==InterruptPin::LED2));
    const bool wakePin=_wake.desiredValid &&
        ((i==1U && _wake.desired.route==WakeRoute::LED1) ||
         (i==2U && _wake.desired.route==WakeRoute::LED2));
    if ((irqPin && pin.function!=PinFunction::INTERRUPT) || (wakePin && pin.function!=PinFunction::WAKE)) {
      return pinConfig("Pin profile conflicts with an existing IRQ/WoL owner");
    }
    const bool direct=!_pinsWorking || _pinsDirect;
    if (pin.function==PinFunction::WAKE && (!wakePin || (direct && _wake.trust!=ConfigurationTrust::VERIFIED))) {
      return pinConfig("Configure the matching WoL source before its pin profile");
    }
    if (pin.function==PinFunction::INTERRUPT && (!irqPin || (direct && _interrupts.trust!=ConfigurationTrust::VERIFIED))) {
      return pinConfig("Configure the matching IRQ source before its pin profile");
    }
  }
  return Status::Ok();
}

bool DP8382x::_pinRowValue(uint8_t row,uint16_t& address,uint16_t& mask,uint16_t& value) const noexcept {
  // SNLS647I tables 8-42/49/85/89 define the LED fields; tables 8-73..77
  // define clock and per-pin mux access. Prepare source/value before enabling
  // the output function; all RMW masks exclude reserved and unrelated fields.
  const auto& s=_pins.desired; mask=value=0;
  const bool led0=s.pins[0].function==PinFunction::LED;
  const bool led1=s.pins[1].function==PinFunction::LED;
  const bool led2=s.pins[2].function==PinFunction::LED;
  switch (row) {
    case 0:
      address=0x0025;
      if (led0) { mask=0x0079; value=static_cast<uint16_t>((static_cast<uint16_t>(s.pins[0].source)<<3U)|1U); }
      if (s.led0SwapOwned) { mask|=0x0200; if (s.led0Swap) { value|=0x0200; } }
      break;
    case 1:
      address=0x0460;
      for (uint8_t i=1;i<4U;++i) {
        if (s.pins[i].function==PinFunction::LED) {
          mask|=static_cast<uint16_t>(0xFU<<(i*4U)); value|=static_cast<uint16_t>(static_cast<uint16_t>(s.pins[i].source)<<(i*4U));
        }
      }
      break;
    case 2: address=0x0019; if (s.stretchOwned) { mask=0x0080; value=s.stretchBypass?0x0080:0; } break;
    case 3:
      address=0x0018;
      if (s.blinkOwned) { mask=0x0600; value=static_cast<uint16_t>(uint16_t{s.blinkCode}<<9U); }
      if (led0) {
        mask|=0x0012;
        if (s.pins[0].drive==LedDrive::HIGH) { value|=0x0002; }
        if (_pinsVerifyOnly && s.pins[0].drive!=LedDrive::NORMAL) { value|=0x0010; }
        if (s.pins[0].polarity!=LedPolarity::PRESERVE) {
          mask|=0x0080; if (s.pins[0].polarity==LedPolarity::ACTIVE_HIGH) { value|=0x0080; }
        }
      }
      break;
    case 4:
      address=0x0469;
      for (uint8_t i=1;i<3U;++i) {
        if (s.pins[i].function!=PinFunction::LED) { continue; }
        const auto shift=static_cast<uint8_t>((i-1U)*4U);
        mask|=static_cast<uint16_t>(3U<<shift);
        if (s.pins[i].drive==LedDrive::HIGH) { value|=static_cast<uint16_t>(2U<<shift); }
        if (_pinsVerifyOnly && s.pins[i].drive!=LedDrive::NORMAL) { value|=static_cast<uint16_t>(1U<<shift); }
        if (s.pins[i].polarity!=LedPolarity::PRESERVE) {
          mask|=static_cast<uint16_t>(4U<<shift);
          if (s.pins[i].polarity==LedPolarity::ACTIVE_HIGH) { value|=static_cast<uint16_t>(4U<<shift); }
        }
      }
      break;
    case 5: address=0x0302; if (s.led1ClockOverride) { mask=value=0x0040; } break;
    case 6: address=0x0304; if (s.pins[1].function==PinFunction::LED) { mask=7; } break;
    case 7:
      address=0x0305;
      if (s.pins[2].function==PinFunction::LED || collision(s.pins[2].function) || s.pins[2].function==PinFunction::HIGH) {
        mask=7; value=muxValue(s.pins[2].function);
      }
      break;
    case 8: address=0x0018; if (!_pinsVerifyOnly && led0 && s.pins[0].drive!=LedDrive::NORMAL) { mask=value=0x0010; } break;
    case 9:
      address=0x0469;
      if (!_pinsVerifyOnly) {
        if (led1 && s.pins[1].drive!=LedDrive::NORMAL) { mask|=1; }
        if (led2 && s.pins[2].drive!=LedDrive::NORMAL) { mask|=0x10; }
        value=mask;
      }
      break;
    default: return false;
  }
  return mask!=0U;
}

void DP8382x::_nextPinRow() noexcept {
  uint16_t address=0,mask=0,value=0;
  while (_pinRow<PIN_ROWS && !_pinRowValue(_pinRow,address,mask,value)) { ++_pinRow; }
  _pinStep=_pinRow<PIN_ROWS?(_pinsVerifyOnly?PinStep::VERIFY:PinStep::APPLY):PinStep::COMPLETE;
}
void DP8382x::_beginPins(bool verifyOnly) noexcept {
  _interfaceWorking=false; _qualityWorking=false;
  _pinsWorking=true; _pinsVerifyOnly=verifyOnly; _pinResult={}; _pinRow=0;
  _pins.observedStale=true;
  const auto valid=_validatePins(_pins.desired);
  if (!valid.ok()) { _pinStep=PinStep::COMPLETE; _finishPins(valid); return; }
  if (_pins.desired.pins[2].function!=PinFunction::UNOWNED || _pins.desired.pins[3].function!=PinFunction::UNOWNED) {
    _pinStep=PinStep::INTERFACE;
  } else if (_pins.desired.pins[1].function!=PinFunction::UNOWNED && _identity.mode==OperatingMode::ENHANCED && !_pins.desired.led1ClockOverride) {
    _pinStep=PinStep::CLOCK_STATUS;
  } else if (_pins.desired.pins[0].function==PinFunction::LED) { _pinStep=PinStep::MUX0; }
  else if (_pins.desired.pins[3].function==PinFunction::LED) { _pinStep=PinStep::MUX3; }
  else { _nextPinRow(); }
  if (_pinStep==PinStep::COMPLETE) { _finishPins(Status::Ok()); }
}

MdioBatch DP8382x::_pinsBatch() const noexcept {
  if (_pinStep==PinStep::INTERFACE) {
    MdioBatch batch; batch.frameCount=1;
    batch.frames[0]={FrameKind::READ,0x17,0,0,NO_READ_INDEX}; return batch;
  }
  if (_pinStep==PinStep::CLOCK_STATUS) { return _mmdBatch(0x0308,FrameKind::READ,0); }
  if (_pinStep==PinStep::MUX0 || _pinStep==PinStep::MUX3) { return _mmdBatch(_pinStep==PinStep::MUX0?0x0303:0x0306,FrameKind::READ,0); }
  uint16_t address=0,mask=0,value=0; _pinRowValue(_pinRow,address,mask,value);
  MdioBatch batch;
  if (address<=0x1FU) { batch.frameCount=1; batch.frames[0]={FrameKind::READ,static_cast<uint8_t>(address),0,0,NO_READ_INDEX}; }
  else { batch=_mmdBatch(address,FrameKind::READ,0); }
  if (_pinStep==PinStep::APPLY) {
    const auto read=static_cast<uint8_t>(batch.frameCount-1U);
    batch.frames[batch.frameCount++]={FrameKind::WRITE,address<=0x1FU?static_cast<uint8_t>(address):uint8_t{0x0E},value,static_cast<uint16_t>(~mask),read};
  }
  return batch;
}

void DP8382x::_retainPinWord(uint16_t address,uint16_t value) noexcept {
  const int index=pinWord(address);
  if (index>=0) { _pins.raw[index]=value; _pins.validMask|=static_cast<uint16_t>(1U<<index); }
  if (!_pins.desiredValid) { return; }
  // Read-only mux/clock prerequisites also withdraw profile proof when a later
  // observation contradicts them. They are not writable profile rows.
  uint16_t prerequisiteMask=0,prerequisiteValue=0;
  bool prerequisiteProved=!_pinsWorking;
  if (address==0x0017U && (_pins.desired.pins[2].function!=PinFunction::UNOWNED ||
      _pins.desired.pins[3].function!=PinFunction::UNOWNED)) {
    prerequisiteMask=0x0020; prerequisiteValue=_config.interface==MacInterface::RMII?0x0020:0;
    prerequisiteProved|=_pinStep!=PinStep::INTERFACE;
  } else if (address==0x0303U && _pins.desired.pins[0].function==PinFunction::LED) {
    prerequisiteMask=7;
    prerequisiteProved|=_pinStep==PinStep::MUX3 || _pinStep==PinStep::APPLY ||
        _pinStep==PinStep::VERIFY || _pinStep==PinStep::COMPLETE || (_pinStep==PinStep::CLOCK_STATUS && _pinRow==6U);
  } else if (address==0x0306U && _pins.desired.pins[3].function==PinFunction::LED) {
    prerequisiteMask=7;
    prerequisiteProved|=_pinStep==PinStep::APPLY || _pinStep==PinStep::VERIFY ||
        _pinStep==PinStep::COMPLETE || (_pinStep==PinStep::CLOCK_STATUS && _pinRow==6U);
  } else if (address==0x0308U && _pins.desired.pins[1].function!=PinFunction::UNOWNED &&
             _identity.mode==OperatingMode::ENHANCED) {
    prerequisiteMask=prerequisiteValue=1;
    prerequisiteProved|=_pins.desired.led1ClockOverride ?
        (_pinRow>=6U && _pinStep!=PinStep::CLOCK_STATUS) : _pinStep!=PinStep::CLOCK_STATUS;
  }
  if (prerequisiteMask!=0U && (value&prerequisiteMask)!=prerequisiteValue) {
    _pins.trust=ConfigurationTrust::DIRTY; _pins.observedStale=true;
    if (_pinsWorking && prerequisiteProved && !_pinResult.mismatch.valid) {
      _pinResult.mismatch={true,address>0x1FU,static_cast<uint8_t>(address>0x1FU?31:0),address,prerequisiteMask,prerequisiteValue,value};
    }
  }
  if (index<0) { return; }
  uint16_t owned=0,expected=0,proved=0;
  for (uint8_t row=0;row<PIN_ROWS;++row) {
    uint16_t target=0,mask=0,word=0;
    if (_pinRowValue(row,target,mask,word) && target==address) {
      owned|=mask; expected=static_cast<uint16_t>((expected&~mask)|word);
      if (_pinsVerifyOnly || row<_pinRow || (row==_pinRow && _pinStep==PinStep::VERIFY)) { proved|=mask; }
      else { proved&=static_cast<uint16_t>(~mask); }
    }
  }
  if (owned==0U || (value&owned)==expected) { return; }
  _pins.trust=ConfigurationTrust::DIRTY; _pins.observedStale=true;
  if (_pinsWorking && !_pinResult.mismatch.valid && ((value^expected)&proved)!=0U) {
    _pinResult.mismatch={true,address>0x1FU,static_cast<uint8_t>(address>0x1FU?31:0),address,proved,static_cast<uint16_t>(expected&proved),value};
  }
}

Status DP8382x::_finishPins(Status status) noexcept {
  _pinStep=PinStep::COMPLETE;
  _pinResult.status=status; _pins.lastStatus=status;
  _pinResult.verified=status.ok();
  _pins.trust=status.ok()?ConfigurationTrust::VERIFIED:ConfigurationTrust::DIRTY;
  _pins.observedStale=!status.ok();
  if (status.ok()) {
    _pins.timeValid=_pinResult.transactions!=0U;
    _pins.observedAtUs=_pins.timeValid?_lastTimeUs+_config.transport.bounds.clockLagUs:0U;
  }
  _pinResult.pins=pinsSnapshot();
  return status;
}

Status DP8382x::_advancePins(Status status,bool called,const MdioBatch& batch) noexcept {
  const auto& receipt=_transport.lastBatch;
  const auto last=static_cast<uint8_t>(batch.frameCount-1U);
  const auto readIndex=static_cast<uint8_t>(batch.frameCount>=4U?3U:0U);
  const bool readValid=called && prefixComplete(batch,receipt,readIndex);
  const auto address=batch.frameCount>=4U?batch.frames[1].value:uint16_t{batch.frames[0].registerAddress};
  const int index=pinWord(address);
  if (index>=0 && !readValid) { _pins.validMask&=static_cast<uint16_t>(~(1U<<index)); }
  if (called) {
    ++_pinResult.transactions; _pinResult.primary=receipt.status; _pinResult.release=receipt.releaseStatus;
    bool prefix=true;
    for (uint8_t i=0;i<batch.frameCount;++i) {
      const auto& frame=receipt.frames[i];
      const bool attempted=prefix && frame.attempted;
      if (attempted) { ++_pinResult.attemptedFrames; }
      prefix=attempted && frame.completed && frame.valueValid==(batch.frames[i].kind==FrameKind::READ);
      if (prefix) { ++_pinResult.completedFrames; }
    }
    if (batch.frames[last].kind==FrameKind::WRITE) {
      const auto& target=receipt.frames[last];
      _pinResult.mutationAttempted|=target.attempted;
      _pinResult.mutationPossible|=target.completed || target.effectPossible || target.valueValid || _transport.violation!=TransportViolation::NONE;
    }
  }
  // _execute has already retained every valid prefix through the single
  // evidence boundary, including a successful read before a later write error.
  uint16_t target=0,mask=0,expected=0;
  if (_pinStep==PinStep::VERIFY && readValid) {
    _pinRowValue(_pinRow,target,mask,expected);
    const auto value=receipt.frames[readIndex].value;
    if ((value&mask)!=expected && !_pinResult.mismatch.valid) {
      _pinResult.mismatch={true,target>0x1FU,static_cast<uint8_t>(target>0x1FU?31:0),target,mask,expected,value};
    }
  }
  if (!status.ok()) { return _finishPins(status); }
  if (_pinResult.mismatch.valid) { return _finishPins(Status::Error(Err::CONFIG_MISMATCH,"Pin readback mismatch")); }
  const auto prerequisiteNext=[&]() noexcept {
    if (_pins.desired.pins[0].function==PinFunction::LED) { _pinStep=PinStep::MUX0; }
    else if (_pins.desired.pins[3].function==PinFunction::LED) { _pinStep=PinStep::MUX3; }
    else { _nextPinRow(); }
  };
  switch (_pinStep) {
    case PinStep::INTERFACE:
      // Tables5-1/8-41: a board declaration or read-only mux default cannot
      // prove current LED2/3 availability. RC status enters the sole collector.
      if ((receipt.frames[readIndex].value&0x0020U)!=(_config.interface==MacInterface::RMII?0x0020U:0U)) {
        return _finishPins(pinConfig("Observed interface differs from pin-profile wiring"));
      }
      if (_pins.desired.pins[1].function!=PinFunction::UNOWNED && _identity.mode==OperatingMode::ENHANCED && !_pins.desired.led1ClockOverride) {
        _pinStep=PinStep::CLOCK_STATUS;
      } else { prerequisiteNext(); }
      break;
    case PinStep::CLOCK_STATUS:
      if ((receipt.frames[readIndex].value&1U)==0U) { return _finishPins(pinConfig("LED1 clock output remains selected")); }
      if (_pinRow==6U) { _nextPinRow(); } else { prerequisiteNext(); }
      break;
    case PinStep::MUX0: case PinStep::MUX3:
      if ((receipt.frames[readIndex].value&7U)!=0U) { return _finishPins(pinConfig("Read-only mux does not select the LED")); }
      if (_pinStep==PinStep::MUX0 && _pins.desired.pins[3].function==PinFunction::LED) { _pinStep=PinStep::MUX3; }
      else { _nextPinRow(); }
      break;
    case PinStep::APPLY: _pinStep=PinStep::VERIFY; break;
    case PinStep::VERIFY:
      ++_pinRow;
      if (target==0x0302U) { _pinStep=PinStep::CLOCK_STATUS; } else { _nextPinRow(); }
      break;
    case PinStep::COMPLETE: break;
  }
  return _pinStep==PinStep::COMPLETE?_finishPins(Status::Ok()):Status::Error(Err::IN_PROGRESS,"Pin pass in progress");
}

Status DP8382x::configurePins(const PinSettings& settings,PinResult& result,const IoBudget& budget) noexcept {
  const auto syntax=pinSyntax(settings); if (!syntax.ok()) { return syntax; }
  if (!_bound) { return Status::Error(Err::NOT_BOUND,"No transport binding"); }
  if (_active || _polling || _operation.state==OperationState::RUNNING) { return pinBusy(BusyDetail::OPERATION_ACTIVE); }
  if (_operation.state==OperationState::RESULT_PENDING) { return pinBusy(BusyDetail::RESULT_PENDING); }
  if (_transport.clockInvalid) { return Status::Error(Err::INVALID_CONFIG,"Clock contract failed",static_cast<int32_t>(ConfigDetail::CLOCK_CONTRACT)); }
  if (_operation.cleanupOwed || _operation.temporaryMayBeActive) { return pinBusy(BusyDetail::CLEANUP_REQUIRED); }
  if (!_initialized || !capabilities().valid) { return Status::Error(Err::NOT_INITIALIZED,"Initialize before pin configuration"); }
  const auto valid=_validatePins(settings); if (!valid.ok()) { return valid; }
  if (!_otherControlsVerified(OperationKind::PIN_CONFIGURE)) { return pinConfig("Verify other owned controls before pin changes"); }
  if (_stopped) { return pinBusy(BusyDetail::CONTROLLER_QUARANTINED); }
  const auto quiet=_checkQuiet(); if (!quiet.ok()) { return quiet; }
  // Count the complete fixed pass before changing desired state or invoking MDIO.
  // Temporarily evaluating the value-only plan does not publish new intent.
  const auto old=_pins; const bool oldVerify=_pinsVerifyOnly;
  _pins.desired=settings; _pinsVerifyOnly=false;
  uint32_t frames=0,callbacks=0;
  if (settings.pins[2].function!=PinFunction::UNOWNED || settings.pins[3].function!=PinFunction::UNOWNED) { ++frames; ++callbacks; }
  for (uint8_t row=0;row<PIN_ROWS;++row) {
    uint16_t address=0,mask=0,value=0;
    if (_pinRowValue(row,address,mask,value)) { frames+=address<=0x1FU?3U:9U; callbacks+=2; }
  }
  if (settings.pins[0].function==PinFunction::LED) { frames+=4; ++callbacks; }
  if (settings.pins[3].function==PinFunction::LED) { frames+=4; ++callbacks; }
  if (settings.pins[1].function!=PinFunction::UNOWNED && _identity.mode==OperatingMode::ENHANCED) { frames+=4; ++callbacks; }
  _pins=old; _pinsVerifyOnly=oldVerify;
  if (budget.maxFrames==0U || budget.maxFrames<frames) { return pinTimeout(TimeoutDetail::FRAME_BUDGET); }
  uint64_t raw=0,upper=0;
  _active=true; const auto clock=_sampleTime(raw,upper); _active=false;
  if (!clock.ok()) { return clock; }
  if (raw>=budget.deadlineUs) { return pinTimeout(TimeoutDetail::DEADLINE); }
  const auto& bounds=_config.transport.bounds;
  const uint64_t bound=uint64_t{callbacks}*bounds.fixedUs+uint64_t{frames}*bounds.frameUs;
  if (upper>=budget.deadlineUs || bound>=budget.deadlineUs-upper) { return pinTimeout(TimeoutDetail::BOUND_ADMISSION); }
  _pins.desired=settings; _pins.desiredValid=true; _pins.trust=ConfigurationTrust::DIRTY;
  _pinsDirect=true; _beginPins(false);
  IoBudget remaining=budget;
  while (_pinStep!=PinStep::COMPLETE) {
    const auto batch=_pinsBatch(); const auto before=_pinResult.attemptedFrames;
    bool called=false; const auto status=_execute(batch,remaining,&called,false,Traffic::MANAGED);
    _advancePins(status,called,batch);
    remaining.maxFrames-=_pinResult.attemptedFrames-before;
  }
  _pinsWorking=_pinsDirect=false;
  if (!_pinResult.status.ok() && _pinResult.mutationPossible) {
    _operation.cleanupOwed=true; _cleanupOwner=OperationKind::PIN_CONFIGURE;
  }
  if (_pinResult.transactions!=0U || _pinResult.status.ok()) { result=_pinResult; }
  return _pinResult.status;
}

PinSnapshot DP8382x::pinsSnapshot() const noexcept {
  auto snapshot=_pins;
  if (!snapshot.desiredValid) { return snapshot; }
  if (snapshot.desired.pins[1].function!=PinFunction::UNOWNED && !_led1RouteVerified()) {
    snapshot.trust=ConfigurationTrust::DIRTY; snapshot.observedStale=true;
  }
  for (uint8_t i=1;i<3U;++i) {
    const auto function=snapshot.desired.pins[i].function;
    const bool wake=function==PinFunction::WAKE;
    const bool interrupt=function==PinFunction::INTERRUPT;
    const bool sourceVerified=wake ? _wake.desiredValid && _wake.trust==ConfigurationTrust::VERIFIED &&
        _wake.desired.route==(i==1U?WakeRoute::LED1:WakeRoute::LED2) :
        interrupt ? _interrupts.desiredValid && _interrupts.trust==ConfigurationTrust::VERIFIED &&
        _interrupts.desired.pin==(i==1U?InterruptPin::LED1:InterruptPin::LED2) : true;
    if (!sourceVerified) { snapshot.trust=ConfigurationTrust::DIRTY; snapshot.observedStale=true; }
  }
  return snapshot;
}
} // namespace DP8382x
