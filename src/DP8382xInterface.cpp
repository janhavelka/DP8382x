/// @file DP8382xInterface.cpp
/// @brief Bounded exact-part interface/clock profiles and shared restoration.
#include <DP8382x/DP8382x.h>

namespace DP8382x {
namespace {
constexpr uint8_t INTERFACE_ROWS=8;
Status interfaceConfig(const char* message) noexcept {
  return Status::Error(Err::INVALID_CONFIG,message,static_cast<int32_t>(ConfigDetail::SOURCE_GATE));
}
Status interfaceBusy(BusyDetail detail) noexcept {
  return Status::Error(Err::BUSY,"Interface owner unavailable",static_cast<int32_t>(detail));
}
Status interfaceTimeout(TimeoutDetail detail) noexcept {
  return Status::Error(Err::OPERATION_TIMEOUT,"Interface allowance exhausted",static_cast<int32_t>(detail));
}
Status interfaceSyntax(const InterfaceSettings& settings) noexcept {
  if (settings.interface>MacInterface::RGMII || settings.rmiiClock>RmiiClock::FOLLOWER_50MHZ ||
      settings.clockOutput>ClockOutput::RECOVERED || settings.elasticityCode>3U) {
    return Status::Error(Err::INVALID_PARAM,"Invalid interface encoding",static_cast<int32_t>(ParamDetail::ENUM_VALUE));
  }
  return Status::Ok();
}
int interfaceWord(uint16_t address) noexcept {
  constexpr uint16_t addresses[]={0x0009,0x000A,0x0011,0x0456,0x0302,0x0017,0x0000,0x0308,0x0303};
  for (unsigned i=0;i<9U;++i) { if (addresses[i]==address) { return static_cast<int>(i); } }
  return -1;
}
// SNLS783A fig. 8-13 and tables 8-13/89. Compare the sampled interface
// selection to the raw strap evidence. RCSR is separately programmable.
Status checkStartupInterface(const StrapEvidence& straps,OperatingMode mode) noexcept {
  if (!straps.sor1Valid || !straps.sor2Valid) { return interfaceConfig("Current interface strap samples required"); }
  bool rmii=false, follower=false;
  if (mode==OperatingMode::BASIC) {
    rmii=(straps.sor1&0x0010U)!=0U;
    follower=(straps.sor1&0x0400U)!=0U;
    if (!rmii && follower) { return interfaceConfig("Reserved BASIC Strap10/4 interface selection"); }
  } else {
    rmii=(straps.sor1&0x0102U)==0x0102U;
    follower=(straps.sor1&0x0020U)!=0U;
  }
  if (rmii!=((straps.sor2&0x0010U)!=0U) || (rmii && follower!=((straps.sor2&0x0008U)!=0U))) {
    return interfaceConfig("Raw interface straps disagree with sampled SOR2 mode or clock role");
  }
  return Status::Ok();
}
bool interfacePrefix(const MdioBatch& batch,const BatchResult& receipt,uint8_t last) noexcept {
  for (uint8_t i=0;i<=last;++i) {
    const auto& frame=receipt.frames[i];
    if (!frame.attempted || !frame.completed || frame.valueValid!=(batch.frames[i].kind==FrameKind::READ)) { return false; }
  }
  return true;
}
}

Status DP8382x::_validateInterface(const InterfaceSettings& settings) const noexcept {
  const auto syntax=interfaceSyntax(settings); if (!syntax.ok()) { return syntax; }
  const bool p22 = isDp83822(_config.variant);
  if (p22 && settings.interface != MacInterface::RGMII) { return _dp83822FeatureGate(); }
  if (!p22 && (settings.rgmiiRxShift || settings.rgmiiTxShift)) {
    return interfaceConfig("Fixed RGMII shifts require a DP83822 RGMII profile");
  }
  if (!p22 && settings.interface!=MacInterface::MII && settings.interface!=MacInterface::RMII) {
    return Status::Error(Err::UNSUPPORTED,"This interface procedure supports declared MII/RMII only");
  }
  if (settings.interface!=_config.interface) { return interfaceConfig("Profile must match application-declared interface wiring"); }
  if (!settings.maintenanceAdmitted || !settings.macStopped || !settings.pinsAvailable ||
      !settings.referenceClockStable || !settings.managementClockAvailable || !settings.clockChangeApproved) {
    return interfaceConfig("Interface and clock transition requires stopped consumers and stable admitted clocks");
  }
  if (settings.isolate && !settings.isolationOwned) { return interfaceConfig("Isolation requires explicit ownership"); }
  if (p22) {
    if (settings.referenceClockMHz != 25U || settings.rmiiClock != RmiiClock::LEADER_25MHZ ||
        settings.txClockShift || settings.revision10 || settings.elasticityCode != 0U ||
        settings.maxFrameBytes != 1518U || settings.clockTolerancePpm != 50U ||
        settings.rxDv || settings.repeater || settings.repeaterWiringAdmitted || settings.fastRxDv ||
        settings.oddNibbleDisable || settings.collisionFullDuplex || settings.shortIpg ||
        settings.clockOutput != ClockOutput::UNOWNED || settings.isolationOwned || settings.observePin31) {
      return interfaceConfig("DP83822 fixed shifts require a 25 MHz RGMII shift-only profile");
    }
    return _dp83822BoardStatus();
  }
  const bool a=isA(_config.variant);
  if (_config.variant==Variant::DP83825I) {
    // SNLS638C figure 6-4 feeds both repeater PHYs from the same oscillator.
    if (settings.repeater && settings.rmiiClock!=RmiiClock::FOLLOWER_50MHZ) {
      return interfaceConfig("DP83825I repeater requires a shared external 50 MHz reference");
    }
    if (settings.rmiiClock!=_config.rmiiClock || _straps.stale || !_straps.rcsrValid ||
        !_straps.sor1Valid || _clockRoleMismatch().valid) {
      return interfaceConfig("DP83825I requires its observed board clock role");
    }
    // SNLS638C tables 7-60/62 expose CLKOUT25 disable and its status.
    if (_config.rmiiClock==RmiiClock::LEADER_25MHZ && !_config.clockLossApproved &&
        (settings.isolate || settings.clockOutput==ClockOutput::LED1)) {
      return interfaceConfig("DP83825I clock route or isolation not admitted");
    }
    if (!settings.txClockShift && settings.rmiiClock==RmiiClock::FOLLOWER_50MHZ &&
        ((_straps.rcsr & 0x0100U)!=0U || (_interface.desiredValid && _interface.desired.txClockShift))) {
      return interfaceConfig("Default follower timing requires a reset baseline");
    }
  }
  if (a) {
    const auto startup=checkStartupInterface(_straps,_identity.mode);
    if (!startup.ok()) { return startup; }
    if (settings.clockOutput==ClockOutput::LED1 || settings.clockOutput==ClockOutput::CLKOUT25) {
      return interfaceConfig("A-generation pin 31 mux programming remains source-gated");
    }
    const auto role=_pins.desiredValid?_pins.desired.pins[0].function:PinFunction::UNOWNED;
    if (role==PinFunction::CLOCK && settings.clockOutput==ClockOutput::UNOWNED) {
      return interfaceConfig("Release the dependent pin 30 clock profile before dropping its source");
    }
    if (settings.clockOutput!=ClockOutput::UNOWNED &&
        ((role!=PinFunction::UNOWNED && role!=PinFunction::CLOCK) ||
         (_interrupts.desiredValid && _interrupts.desired.pin==InterruptPin::LED0) ||
         (_wake.desiredValid && _wake.desired.route==WakeRoute::LED0))) {
      return interfaceConfig("Pin 30 clock conflicts with an owned LED0 GPIO route");
    }
    if (settings.observePin31 && _identity.mode!=OperatingMode::ENHANCED) {
      return interfaceConfig("Pin 31 route status requires ENHANCED mode");
    }
  } else if (settings.observePin31 || settings.clockOutput==ClockOutput::PLL || settings.clockOutput==ClockOutput::RECOVERED) {
    return interfaceConfig("Requested pin clock operation is unavailable for this part");
  }
  if (settings.interface==MacInterface::MII) {
    if (settings.rmiiClock!=RmiiClock::LEADER_25MHZ || settings.txClockShift || settings.revision10 ||
        settings.elasticityCode!=0U || settings.rxDv || settings.repeater) {
      return interfaceConfig("RMII options do not apply to MII");
    }
    if (settings.referenceClockMHz!=25U) { return interfaceConfig("MII requires the declared 25MHz reference"); }
  } else {
    const bool follower=settings.rmiiClock==RmiiClock::FOLLOWER_50MHZ;
    if (settings.referenceClockMHz!=(follower?50U:25U) || (settings.txClockShift && !follower)) {
      return interfaceConfig("RMII reference direction/frequency or TX shift is incompatible");
    }
    if (settings.collisionFullDuplex) { return interfaceConfig("Full-duplex COL output applies only to MII"); }
    if (settings.clockTolerancePpm!=50U && settings.clockTolerancePpm!=100U) {
      return interfaceConfig("RMII packet bound requires the documented 50ppm or 100ppm case");
    }
    // SNLS647I table 8-41: encoding order is 14, 2, 6, 10 bits, not ascending.
    constexpr uint32_t maximumBytes[]={16800,2400,7200,12000};
    const auto maximum=maximumBytes[settings.elasticityCode]/(settings.clockTolerancePpm==100U?2U:1U);
    if (settings.maxFrameBytes==0U || settings.maxFrameBytes>maximum) { return interfaceConfig("Declared packet exceeds RMII elasticity allowance"); }
    if (settings.repeater && (!settings.rxDv || !settings.repeaterWiringAdmitted)) {
      return interfaceConfig("Repeater requires explicit crossed RX_DV wiring and clock ownership");
    }
  }
  if (!a && settings.clockOutput!=ClockOutput::UNOWNED) {
    if (_config.variant!=Variant::DP83825I && _identity.mode!=OperatingMode::ENHANCED) { return interfaceConfig("Runtime CLKOUT/LED1 selection requires ENHANCED mode"); }
    if (_pins.desiredValid && _pins.desired.led1ClockOverride) {
      return interfaceConfig("Release the pin owner's clock override before interface ownership");
    }
  }
  if (settings.clockOutput==ClockOutput::CLKOUT25 &&
      ((_pins.desiredValid && _pins.desired.pins[1].function!=PinFunction::UNOWNED) ||
       (_interrupts.desiredValid && _interrupts.desired.pin==InterruptPin::LED1) ||
       (_wake.desiredValid && _wake.desired.route==WakeRoute::LED1))) {
    return interfaceConfig("CLKOUT conflicts with an owned LED1 GPIO/source route");
  }
  if (settings.collisionFullDuplex && _pins.desiredValid &&
      _pins.desired.pins[2].function!=PinFunction::UNOWNED &&
      _pins.desired.pins[2].function!=PinFunction::COLLISION3 &&
      _pins.desired.pins[2].function!=PinFunction::COLLISION5 &&
      _pins.desired.pins[2].function!=PinFunction::COLLISION6) {
    return interfaceConfig("Requested COL output conflicts with LED2 pin ownership");
  }
  if (settings.collisionFullDuplex &&
      ((_interrupts.desiredValid && _interrupts.desired.pin==InterruptPin::LED2) ||
       (_wake.desiredValid && _wake.desired.route==WakeRoute::LED2))) {
    return interfaceConfig("Requested COL output conflicts with LED2 IRQ/WoL routing");
  }
  if (_eee.desiredValid && _eee.desired.enabled &&
      ((_config.variant!=Variant::DP83825I && settings.clockOutput!=ClockOutput::UNOWNED) || (settings.isolationOwned && settings.isolate))) {
    return interfaceConfig("EEE clock recipe conflicts with clock-route ownership or isolation");
  }
  return Status::Ok();
}

bool DP8382x::_interfaceRowValue(uint8_t row,uint16_t& address,uint16_t& mask,uint16_t& value) const noexcept {
  const auto& settings=_interface.desired; mask=value=0;
  if (isDp83822(_config.variant)) {
    if (row != 5U) { return false; }
    // SNLS505H table 8-23. Verification includes the required live RGMII mode.
    address = 0x0017;
    mask = 0x1A00;
    value = static_cast<uint16_t>(0x0200U | (settings.rgmiiRxShift ? 0x1000U : 0U) |
                                  (settings.rgmiiTxShift ? 0U : 0x0800U));
    return true;
  }
  // SNLS647I 8-19/28/29/35/41/73/84. Fields are owned independently of
  // power, link routing, IRQ, and LED masks in the same registers.
  switch (row) {
    case 0: address=0x0009; mask=2; value=settings.fastRxDv?2:0; break;
    case 1: address=0x000A; mask=2; value=settings.oddNibbleDisable?2:0; break;
    case 2: address=0x0011; if (_config.variant==Variant::DP83825I) { break; } mask=0x0010; value=settings.collisionFullDuplex?0x0010:0; break;
    case 3: address=0x0456; mask=8; value=settings.shortIpg?8:0; break;
    case 4:
      address=0x0302;
      if (settings.interface==MacInterface::RMII) { mask=0x0100; value=settings.rxDv?0x0100:0; }
      if (!isA(_config.variant) && settings.clockOutput!=ClockOutput::UNOWNED) {
        mask|=0x0040; if (settings.clockOutput==ClockOutput::LED1) { value|=0x0040; }
      }
      break;
    case 5:
      address=0x0017;
      if (_config.variant==Variant::DP83825I) {
        mask=settings.rmiiClock==RmiiClock::FOLLOWER_50MHZ ? 0x0113 : 0x0013;
        value=static_cast<uint16_t>(settings.elasticityCode | (settings.revision10?0x10:0) |
                                    (settings.txClockShift?0x0100:0));
        break;
      }
      mask=0x0020;
      if (settings.interface==MacInterface::RMII) {
        mask=0x01B3; value=static_cast<uint16_t>(0x0020U|settings.elasticityCode|
            (settings.rmiiClock==RmiiClock::FOLLOWER_50MHZ?0x0080U:0U)|
            (settings.txClockShift?0x0100U:0U)|(settings.revision10?0x0010U:0U));
      }
      break;
    case 6: address=0; if (settings.isolationOwned) { mask=0x0400; value=settings.isolate?0x0400:0; } break;
    case 7:
      address=0x0303;
      if (isA(_config.variant) && settings.clockOutput!=ClockOutput::UNOWNED) {
        // SNLS783A table 8-75: enable the pin 30 clock mux and select its
        // source in one RMW. Pin 31 and the RMII RX_DV route are independent.
        mask=0x003F; value=settings.clockOutput==ClockOutput::PLL?0x0029:0x0031;
      }
      break;
    default: return false;
  }
  return mask!=0U;
}

void DP8382x::_nextInterfaceRow() noexcept {
  uint16_t address=0,mask=0,value=0;
  while (_interfaceRow<INTERFACE_ROWS && !_interfaceRowValue(_interfaceRow,address,mask,value)) { ++_interfaceRow; }
  if (_interfaceRow<INTERFACE_ROWS) {
    _interfaceStep=_interfaceVerifyOnly?InterfaceStep::VERIFY:InterfaceStep::APPLY;
  } else if (_interface.desired.observePin31 && _interfaceStep!=InterfaceStep::CLOCK_STATUS) {
    _interfaceStep=InterfaceStep::CLOCK_STATUS;
  } else {
    _interfaceStep=InterfaceStep::COMPLETE;
  }
}
void DP8382x::_beginInterface(bool verifyOnly) noexcept {
  _pinsWorking=_wakeWorking=_eeeWorking=_powerWorking=_interruptWorking=false;
  _cableWorking=_bistWorking=_loopWorking=_configuring=false;
  _qualityWorking=false;
  _interfaceWorking=true; _interfaceVerifyOnly=verifyOnly; _interfaceResult={}; _interfaceRow=0;
  _interface.observedStale=true;
  const auto valid=_validateInterface(_interface.desired);
  if (!valid.ok()) { _finishInterface(valid); return; }
  _interfaceStep=InterfaceStep::PREFLIGHT;
  if (isDp83822(_config.variant)) { _nextInterfaceRow(); }
}
MdioBatch DP8382x::_interfaceBatch() const noexcept {
  if (_interfaceStep==InterfaceStep::PREFLIGHT) {
    MdioBatch batch; batch.frameCount=2;
    batch.frames[0]={FrameKind::READ,0,0,0,NO_READ_INDEX};
    batch.frames[1]={FrameKind::READ,0x11,0,0,NO_READ_INDEX};
    if (_interface.desired.clockOutput==ClockOutput::RECOVERED) {
      batch.frames[batch.frameCount++]={FrameKind::READ,0x10,0,0,NO_READ_INDEX};
    }
    return batch;
  }
  if (_interfaceStep==InterfaceStep::FOLLOWER_TIMING) {
    // SNLS638C 5.6 footnote 2 supplies this whole word only for shifted timing.
    return _mmdBatch(0x0042,FrameKind::WRITE,0x0014);
  }
  if (_interfaceStep==InterfaceStep::CLOCK_STATUS) { return _mmdBatch(0x0308,FrameKind::READ,0); }
  uint16_t address=0,mask=0,value=0; _interfaceRowValue(_interfaceRow,address,mask,value);
  MdioBatch batch;
  if (isDp83822(_config.variant) && _interfaceStep == InterfaceStep::APPLY) {
    // Sample the live mode and compose its two fixed shifts under one hold.
    // Emit zero for the reserved read-only bits and read-clear FIFO flags.
    batch = _mmdBatch(address, FrameKind::READ, 0);
    batch.frames[4] = {FrameKind::WRITE, 0x0E, static_cast<uint16_t>(value & 0x1800U),
                       0x07F3, 3, 0x0200, 0x0200};
    batch.frameCount = 5;
    return batch;
  }
  if (address<=0x1FU) { batch.frameCount=1; batch.frames[0]={FrameKind::READ,static_cast<uint8_t>(address),0,0,NO_READ_INDEX}; }
  else { batch=_mmdBatch(address,FrameKind::READ,0); }
  if (_interfaceStep==InterfaceStep::APPLY) {
    // RCSR RC samples belong to the collector and are never replayed. BMCR
    // power reads include a hardware OR: supply owned software intent instead.
    uint16_t neutral=address==0x0017U?0x000C:address==0x0011U?0x0004:address==0U?0x8A00:0;
    if (address==0U) { value|=_softwarePowerWord(); }
    const auto read=static_cast<uint8_t>(batch.frameCount-1U);
    batch.frames[batch.frameCount++]={FrameKind::WRITE,address<=0x1FU?static_cast<uint8_t>(address):uint8_t{0x0E},
        value,static_cast<uint16_t>(~(mask|neutral)),read};
  }
  return batch;
}

void DP8382x::_retainInterfaceWord(uint16_t address,uint16_t value) noexcept {
  const int index=interfaceWord(address);
  if (index<0 || (index==8 && !isA(_config.variant))) { return; }
  _interface.raw[index]=value; _interface.validMask|=static_cast<uint16_t>(1U<<index);
  if (address==0x0017U) {
    // SNLS505H table 8-23 gives RGMII priority even when RMII_EN is set.
    if (isDp83822(_config.variant) && (value & 0x0200U) != 0U) {
      _interface.observedInterface = MacInterface::RGMII;
    } else {
      _interface.observedInterface = (_config.variant == Variant::DP83825I || (value & 0x0020U) != 0U)
          ? MacInterface::RMII : MacInterface::MII;
    }
    _interface.modeValid=true;
    // LED2 source owners share the same mode prerequisite even without a pin
    // profile. A later collector observation cannot leave their route verified.
    if (_interface.observedInterface!=_config.interface) {
      const bool irq=_interrupts.desiredValid && (_interrupts.desired.pin==InterruptPin::LED2 || _interrupts.desired.pin==InterruptPin::LED3);
      const bool wake=_wake.desiredValid && (_wakePin()==2U || _wakePin()==3U);
      if (irq) { _interrupts.trust=ConfigurationTrust::DIRTY; _interrupts.observedStale=true; }
      if (wake) { _wake.trust=ConfigurationTrust::DIRTY; _wake.observedStale=true; }
      const bool proved=(irq && _interruptWorking && _interruptStep!=InterruptStep::IDENTIFY && _interruptStep!=InterruptStep::PIN_MUX) ||
          (wake && _wakeWorking && _wakeStep!=WakeStep::IDENTIFY && _wakeStep!=WakeStep::PIN_MUX);
      if (proved && !_result.mismatch.valid) {
        _result.mismatch={true,false,0,0x17,0x0020,static_cast<uint16_t>(_config.interface==MacInterface::RMII?0x0020:0),value};
      }
    }
  }
  if (!_interface.desiredValid) { return; }
  uint16_t target=0,mask=0,expected=0;
  bool proved=!_interfaceWorking;
  if (index==7) {
    if (isA(_config.variant) || _interface.desired.clockOutput==ClockOutput::UNOWNED) { return; }
    mask=1; expected=_interface.desired.clockOutput==ClockOutput::LED1?1:0;
    proved|=_interfaceRow>4U;
  } else {
    const uint8_t row=index==8?7:static_cast<uint8_t>(index);
    if (!_interfaceRowValue(row,target,mask,expected)) { return; }
    proved|=_interfaceVerifyOnly || row<_interfaceRow || (row==_interfaceRow && _interfaceStep==InterfaceStep::VERIFY);
  }
  if ((value&mask)==expected) { return; }
  _interface.trust=ConfigurationTrust::DIRTY; _interface.observedStale=true;
  if (index==8 && proved && _pins.desiredValid && _pins.desired.pins[0].function==PinFunction::CLOCK) {
    _pins.trust=ConfigurationTrust::DIRTY;
    _pins.observedStale=true;
  }
  if (_interfaceWorking && proved && !_interfaceResult.mismatch.valid) {
    _interfaceResult.mismatch={true,address>0x1FU,static_cast<uint8_t>(address>0x1FU?31:0),address,mask,expected,value};
  }
}
Status DP8382x::_finishInterface(Status status) noexcept {
  if (status.ok() && _interfaceDirect && !_otherControlsVerified(OperationKind::INTERFACE_CONFIGURE)) {
    status=interfaceConfig("Another owned policy lost verification during interface application");
  }
  _interfaceStep=InterfaceStep::COMPLETE;
  _interfaceResult.status=status; _interface.lastStatus=status;
  _interfaceResult.verified=status.ok();
  _interface.trust=status.ok()?ConfigurationTrust::VERIFIED:ConfigurationTrust::DIRTY;
  _interface.observedStale=!status.ok();
  if (status.ok()) { _interface.timeValid=true; _interface.observedAtUs=_lastTimeUs+_config.transport.bounds.clockLagUs; }
  _interfaceResult.interfaceState=_interface;
  return status;
}
Status DP8382x::_advanceInterface(Status status,bool called,const MdioBatch& batch) noexcept {
  const auto& receipt=_transport.lastBatch;
  const auto last=static_cast<uint8_t>(batch.frameCount-1U);
  const auto readIndex=static_cast<uint8_t>(batch.frameCount>=4U?3U:0U);
  const auto address=batch.frameCount>=4U?batch.frames[1].value:uint16_t{batch.frames[0].registerAddress};
  const bool readValid=called && interfacePrefix(batch,receipt,readIndex);
  const int index=interfaceWord(address);
  if (index>=0 && !readValid) { _interface.validMask&=static_cast<uint16_t>(~(1U<<index)); }
  if (address==0x0017U && !readValid) { _interface.modeValid=false; }
  if (_interfaceStep==InterfaceStep::PREFLIGHT && (!called || !interfacePrefix(batch,receipt,1))) {
    _interface.validMask&=static_cast<uint16_t>(~0x04U);
  }
  if (called) {
    ++_interfaceResult.transactions; _interfaceResult.primary=receipt.status; _interfaceResult.release=receipt.releaseStatus;
    bool prefix=true;
    for (uint8_t i=0;i<batch.frameCount;++i) {
      const auto& frame=receipt.frames[i]; const bool attempted=prefix && frame.attempted;
      if (attempted) { ++_interfaceResult.attemptedFrames; }
      prefix=attempted && frame.completed && frame.valueValid==(batch.frames[i].kind==FrameKind::READ);
      if (prefix) { ++_interfaceResult.completedFrames; }
    }
    if (batch.frames[last].kind==FrameKind::WRITE) {
      const auto& target=receipt.frames[last];
      const bool guardPassed = !readValid || batch.frames[last].acceptsRead(receipt.frames[readIndex].value);
      _interfaceResult.mutationAttempted|=target.attempted;
      _interfaceResult.mutationCompleted|=guardPassed && interfacePrefix(batch,receipt,last);
      _interfaceResult.mutationPossible|=target.completed || target.effectPossible || target.valueValid || _transport.violation!=TransportViolation::NONE;
      if (!guardPassed && isDp83822(_config.variant)) {
        _interfaceResult.mismatch = {true, true, 31, address, 0x0200, 0x0200, receipt.frames[readIndex].value};
      }
    }
  }
  if (_config.variant==Variant::DP83825I && _clockRoleMismatch().valid) {
    _interfaceResult.mismatch=_clockRoleMismatch();
  }
  if (!status.ok()) { return _finishInterface(status); }
  if (_interfaceResult.mismatch.valid) { return _finishInterface(Status::Error(Err::CONFIG_MISMATCH,"Interface readback mismatch")); }
  switch (_interfaceStep) {
    case InterfaceStep::PREFLIGHT: {
      const auto bmcr=receipt.frames[0].value,physcr=receipt.frames[1].value;
      if (_interface.desired.clockOutput==ClockOutput::RECOVERED && (receipt.frames[2].value&1U)==0U) {
        return _finishInterface(interfaceConfig("Recovered 125 MHz clock requires an observed active partner link"));
      }
      if ((bmcr&0x8200U)!=0U) { return _finishInterface(interfaceConfig("Reset/restart is active")); }
      if (!_interfaceVerifyOnly && (physcr&4U)!=0U) { return _finishInterface(interfaceConfig("Unowned interrupt stimulus is active")); }
      if (!_interfaceVerifyOnly && !_power.desiredValid && (bmcr&0x0800U)!=0U) { return _finishInterface(interfaceConfig("Unowned power request cannot be echoed into interface writes")); }
      if (!_interfaceVerifyOnly && (physcr&0x8000U)!=0U) { return _finishInterface(interfaceConfig("Restore PLL before interface/clock mutation")); }
      _nextInterfaceRow(); break;
    }
    case InterfaceStep::APPLY:
      if (_config.variant==Variant::DP83825I && address==0x0017 && _interface.desired.txClockShift) {
        _p25TimingApplied=false;
        _interfaceStep=InterfaceStep::FOLLOWER_TIMING;
      } else { _interfaceStep=InterfaceStep::VERIFY; }
      break;
    case InterfaceStep::FOLLOWER_TIMING:
      _p25TimingApplied=true;
      _interfaceStep=InterfaceStep::VERIFY; break;
    case InterfaceStep::VERIFY: {
      uint16_t target=0,mask=0,expected=0; _interfaceRowValue(_interfaceRow,target,mask,expected);
      if (_config.variant==Variant::DP83825I && target==0x0017 && _interface.desired.txClockShift && !_p25TimingApplied) {
        return _finishInterface(interfaceConfig("Follower setup-time companion write is unproved"));
      }
      if ((receipt.frames[readIndex].value&mask)!=expected) {
        _interfaceResult.mismatch={true,target>0x1FU,static_cast<uint8_t>(target>0x1FU?31:0),target,mask,expected,receipt.frames[readIndex].value};
        return _finishInterface(Status::Error(Err::CONFIG_MISMATCH,"Interface readback mismatch"));
      }
      ++_interfaceRow;
      if (!isA(_config.variant) && target==0x0302U && _interface.desired.clockOutput!=ClockOutput::UNOWNED) { _interfaceStep=InterfaceStep::CLOCK_STATUS; }
      else { _nextInterfaceRow(); }
      break;
    }
    case InterfaceStep::CLOCK_STATUS: {
      const uint16_t expected=_interface.desired.clockOutput==ClockOutput::LED1?1:0;
      if (!isA(_config.variant) && (receipt.frames[readIndex].value&1U)!=expected) {
        _interfaceResult.mismatch={true,true,31,0x0308,1,expected,receipt.frames[readIndex].value};
        return _finishInterface(Status::Error(Err::CONFIG_MISMATCH,"Clock route status mismatch"));
      }
      _nextInterfaceRow(); break;
    }
    case InterfaceStep::COMPLETE: break;
  }
  return _interfaceStep==InterfaceStep::COMPLETE?_finishInterface(Status::Ok()):Status::Error(Err::IN_PROGRESS,"Interface pass in progress");
}

void DP8382x::_interfacePassCost(bool verifyOnly,uint32_t& frames,uint32_t& callbacks) const noexcept {
  if (isDp83822(_config.variant)) {
    frames = verifyOnly ? 1U : 6U;
    callbacks = verifyOnly ? 1U : 2U;
    return;
  }
  const auto& settings=_interface.desired;
  frames=settings.clockOutput==ClockOutput::RECOVERED?3U:2U; callbacks=1;
  for (uint8_t row=0;row<INTERFACE_ROWS;++row) {
    uint16_t address=0,mask=0,value=0;
    if (_interfaceRowValue(row,address,mask,value)) {
      const uint32_t readFrames=address<=0x1FU?1U:4U;
      frames+=verifyOnly?readFrames:readFrames*2U+1U;
      callbacks+=verifyOnly?1U:2U;
    }
  }
  if (_config.variant==Variant::DP83825I && settings.txClockShift && !verifyOnly) { frames+=4; ++callbacks; }
  if (settings.observePin31 || (!isA(_config.variant) && settings.clockOutput!=ClockOutput::UNOWNED)) {
    frames+=4; ++callbacks;
  }
}

Status DP8382x::configureInterface(const InterfaceSettings& settings,InterfaceResult& result,const IoBudget& budget) noexcept {
  const auto syntax=interfaceSyntax(settings); if (!syntax.ok()) { return syntax; }
  if (!_bound) { return Status::Error(Err::NOT_BOUND,"No transport binding"); }
  if (_active || _polling || _operation.state==OperationState::RUNNING) { return interfaceBusy(BusyDetail::OPERATION_ACTIVE); }
  if (_operation.state==OperationState::RESULT_PENDING) { return interfaceBusy(BusyDetail::RESULT_PENDING); }
  if (_transport.clockInvalid) { return Status::Error(Err::INVALID_CONFIG,"Clock contract failed",static_cast<int32_t>(ConfigDetail::CLOCK_CONTRACT)); }
  if (_operation.cleanupOwed || _operation.temporaryMayBeActive) { return interfaceBusy(BusyDetail::CLEANUP_REQUIRED); }
  if (!_initialized || !capabilities().valid) { return Status::Error(Err::NOT_INITIALIZED,"Initialize before interface configuration"); }
  const auto valid=_validateInterface(settings); if (!valid.ok()) { return valid; }
  if (!_otherControlsVerified(OperationKind::INTERFACE_CONFIGURE)) { return interfaceConfig("Verify other owned controls before interface changes"); }
  if (_stopped) { return interfaceBusy(BusyDetail::CONTROLLER_QUARANTINED); }
  const auto quiet=_checkQuiet(); if (!quiet.ok()) { return quiet; }
  // Compute this finite complete pass before replacing retained desired intent.
  const auto old=_interface; _interface.desired=settings;
  uint32_t frames=0,callbacks=0;
  _interfacePassCost(false,frames,callbacks);
  _interface=old;
  if (budget.maxFrames==0U || budget.maxFrames<frames) { return interfaceTimeout(TimeoutDetail::FRAME_BUDGET); }
  uint64_t raw=0,upper=0;
  _active=true; const auto clock=_sampleTime(raw,upper); _active=false;
  if (!clock.ok()) { return clock; }
  if (raw>=budget.deadlineUs) { return interfaceTimeout(TimeoutDetail::DEADLINE); }
  const auto& bounds=_config.transport.bounds;
  const uint64_t bound=uint64_t{callbacks}*bounds.fixedUs+uint64_t{frames}*bounds.frameUs;
  if (upper>=budget.deadlineUs || bound>=budget.deadlineUs-upper) { return interfaceTimeout(TimeoutDetail::BOUND_ADMISSION); }
  _interface.desired=settings; _interface.desiredValid=true; _interface.trust=ConfigurationTrust::DIRTY;
  _interfaceDirect=true; _beginInterface(false);
  IoBudget remaining=budget;
  while (_interfaceStep!=InterfaceStep::COMPLETE) {
    const auto batch=_interfaceBatch(); const auto before=_interfaceResult.attemptedFrames;
    const auto reserve=_interfaceResult.transactions==0U?bound:0U;
    bool called=false;
    const auto traffic=isDp83822(_config.variant)?Traffic::DP83822_MANAGED:Traffic::MANAGED;
    const auto status=_execute(batch,remaining,&called,false,traffic,32,reserve);
    _advanceInterface(status,called,batch);
    remaining.maxFrames-=_interfaceResult.attemptedFrames-before;
  }
  _interfaceWorking=_interfaceDirect=false;
  if (_interfaceResult.transactions==0U && _interfaceResult.status.is(Err::OPERATION_TIMEOUT)) {
    _interface=old;
  }
  if (!_interfaceResult.status.ok() && _interfaceResult.mutationPossible) {
    _operation.cleanupOwed=true; _cleanupOwner=OperationKind::INTERFACE_CONFIGURE;
  }
  if (_interfaceResult.transactions!=0U || _interfaceResult.status.ok()) { result=_interfaceResult; }
  return _interfaceResult.status;
}
bool DP8382x::_led1RouteVerified() const noexcept {
  // On A the interface clock owner selects pin 30. Pin 31 has a separate POR observation.
  if (isA(_config.variant)) { return true; }
  const bool pinRoute=!_pins.desiredValid || !_pins.desired.led1ClockOverride ||
      _pins.trust==ConfigurationTrust::VERIFIED;
  const bool interfaceRoute=!_interface.desiredValid || _interface.desired.clockOutput==ClockOutput::UNOWNED ||
      (_interface.desired.clockOutput==ClockOutput::LED1 && _interface.trust==ConfigurationTrust::VERIFIED);
  return pinRoute && interfaceRoute;
}
} // namespace DP8382x
