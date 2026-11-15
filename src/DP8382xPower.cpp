/// @file DP8382xPower.cpp
/// @brief Explicit power intent and finite control verification; no physical wake claim.
#include <DP8382x/DP8382x.h>

namespace DP8382x {
namespace {
Status powerError(const char* message) noexcept {
  return Status::Error(Err::INVALID_CONFIG,message,static_cast<int32_t>(ConfigDetail::POWER_STATE));
}
MdioBatch powerRead(uint8_t address) noexcept {
  MdioBatch batch; batch.frameCount=1;
  batch.frames[0]={FrameKind::READ,address,0,0,NO_READ_INDEX}; return batch;
}
MdioBatch powerUpdate(uint8_t address,uint16_t preserve,uint16_t value) noexcept {
  auto batch=powerRead(address); batch.frameCount=2;
  batch.frames[1]={FrameKind::WRITE,address,value,preserve,0}; return batch;
}
}

uint16_t DP8382x::_softwarePowerWord() const noexcept {
  return _power.desiredValid && _power.requestedPowerDown ? uint16_t{0x0800} : uint16_t{0};
}
Status DP8382x::_validatePower(const PowerSettings& s) const noexcept {
  if (s.request>PowerRequest::DEEP_POWER_DOWN) {
    return Status::Error(Err::INVALID_PARAM,"Invalid power request",static_cast<int32_t>(ParamDetail::ENUM_VALUE));
  }
  if (s.request==PowerRequest::DEEP_POWER_DOWN) {
    return Status::Error(Err::UNSUPPORTED,"DPD exit and opaque0428 composition need exact-part evidence",
                         static_cast<int32_t>(ConfigDetail::SOURCE_GATE));
  }
  if (!s.maintenanceAdmitted || !s.managementClockAvailable) { return powerError("Power maintenance and management clock must be admitted"); }
  if (s.disablePll && s.request!=PowerRequest::IEEE_POWER_DOWN) { return powerError("PLL disable requires IEEE power-down"); }
  if ((s.request==PowerRequest::IEEE_POWER_DOWN || s.disablePll) && !s.clockLossApproved) {
    return powerError("Application must approve generated-clock loss");
  }
  return Status::Ok();
}
Status DP8382x::startConfigurePower(const PowerSettings& settings,const OperationBudget& budget,OperationToken& token) noexcept {
  if (settings.request>PowerRequest::DEEP_POWER_DOWN) {
    return Status::Error(Err::INVALID_PARAM,"Invalid power request",static_cast<int32_t>(ParamDetail::ENUM_VALUE));
  }
  if (budget.pollIntervalUs==0U || budget.maxStalledPolls==0U) {
    return Status::Error(Err::INVALID_PARAM,"Invalid power operation budget",static_cast<int32_t>(ParamDetail::BUDGET_RANGE));
  }
  if (!_bound) { return Status::Error(Err::NOT_BOUND,"No transport binding"); }
  if (_active || _polling || _operation.state==OperationState::RUNNING) {
    return Status::Error(Err::BUSY,"Power owner active",static_cast<int32_t>(BusyDetail::OPERATION_ACTIVE));
  }
  if (_operation.state==OperationState::RESULT_PENDING) {
    return Status::Error(Err::BUSY,"Power result pending",static_cast<int32_t>(BusyDetail::RESULT_PENDING));
  }
  if (_transport.clockInvalid) {
    return Status::Error(Err::INVALID_CONFIG,"Repair binding clock before power work",static_cast<int32_t>(ConfigDetail::CLOCK_CONTRACT));
  }
  if (_operation.cleanupOwed || _operation.temporaryMayBeActive) {
    return Status::Error(Err::BUSY,"Restore earlier effects first",static_cast<int32_t>(BusyDetail::CLEANUP_REQUIRED));
  }
  if (!_initialized || !capabilities().valid) { return Status::Error(Err::NOT_INITIALIZED,"Initialize before power configuration"); }
  const auto valid=_validatePower(settings); if (!valid.ok()) { return valid; }
  if (_eee.desiredValid && _eee.desired.enabled &&
      (settings.request != PowerRequest::NORMAL || settings.disablePll || settings.activeSleep)) {
    return powerError("Disable EEE before independent low-power policy");
  }
  if (!_otherControlsVerified(OperationKind::POWER_CONFIGURE)) { return powerError("Verify other owned controls first"); }
  if (budget.io.maxFrames==0U || budget.io.deadlineUs==0U) {
    return Status::Error(Err::OPERATION_TIMEOUT,"Empty power budget",static_cast<int32_t>(
        budget.io.maxFrames==0U?TimeoutDetail::FRAME_BUDGET:TimeoutDetail::DEADLINE));
  }
  _beginOperation(OperationKind::POWER_CONFIGURE,budget);
  _power.desired=settings; _power.desiredValid=true;
  _power.requestedPowerDown=settings.request==PowerRequest::IEEE_POWER_DOWN;
  _power.trust=ConfigurationTrust::DIRTY; _power.observedStale=true; _power.state=PowerState::UNKNOWN;
  _powerTailPending=false; _powerVerifyOnly=false; _powerPreparingReset=false;
  _setPowerStep(PowerStep::READ_BMCR);
  token=_operation.token; return _operation.status;
}
void DP8382x::_resumeAfterPowerPreparation() noexcept {
  _powerWorking=false; _powerPreparingReset=false; _powerPreparationDone=true;
  _setInitStep(_result.commandRequired && !_result.commandAccepted ? InitStep::COMMAND : InitStep::APPLY);
}
void DP8382x::_setPowerStep(PowerStep step) noexcept {
  _pinsWorking=false; _interfaceWorking=false;
  _wakeWorking=false;
  _powerWorking=true; _interruptWorking=_cableWorking=_bistWorking=_loopWorking=_configuring=false;
  _powerStep=step; _operation.nextPollValid=false; _operation.phase=OperationPhase::VERIFY;
}
MdioBatch DP8382x::_powerBatch() const noexcept {
  switch (_powerStep) {
    case PowerStep::READ_BMCR: case PowerStep::VERIFY_BMCR: return powerRead(0);
    case PowerStep::READ_PHYSCR: case PowerStep::VERIFY_CLEAR_PLL:
    case PowerStep::VERIFY_POLICY: case PowerStep::VERIFY_PLL: return powerRead(0x11);
    case PowerStep::CLEAR_PLL: return powerUpdate(0x11,0x7FFB,0);
    // Active sleep never owns PLL. Test is neutral zero; other IRQ controls stay intact.
    case PowerStep::POLICY: return powerUpdate(0x11,0x8FFB,_power.desired.activeSleep?0x6000:0);
    // Table8-19: software/pin OR is not software intent. Keep isolation independent.
    case PowerStep::BMCR: return powerUpdate(0,0x75FF,_softwarePowerWord());
    case PowerStep::PLL: return powerUpdate(0x11,0x7FFB,0x8000);
    case PowerStep::BISCR: return powerRead(0x16);
  }
  return {};
}
void DP8382x::_retainPowerWord(uint8_t address,uint16_t value) noexcept {
  if (address==0U) {
    _power.rawBmcr=value; _power.bmcrValid=true; _power.observedBmcrPowerDown=(value&0x0800U)!=0U;
    if ((value&0x8000U)!=0U) { _invalidateResetEvidence(true); }
    if (_power.desiredValid && _power.observedBmcrPowerDown!=_power.requestedPowerDown) { _power.trust=ConfigurationTrust::DIRTY; }
  } else if (address==0x11U) {
    _power.rawPhyscr=value; _power.physcrValid=true;
    const auto expected=static_cast<uint16_t>((_power.desired.activeSleep?0x6000U:0U)|(_power.desired.disablePll?0x8000U:0U));
    if (_power.desiredValid && (value&0xF000U)!=expected) { _power.trust=ConfigurationTrust::DIRTY; }
  }
  else if (address==0x16U) { _power.rawBiscr=value; _power.biscrValid=true; }
}
void DP8382x::_observePower() noexcept {
  _power.observedStale=false; _power.timeValid=true;
  _power.observedAtUs=_lastTimeUs+_config.transport.bounds.clockLagUs;
  if (_power.observedBmcrPowerDown) {
    _power.state=_power.desiredValid && !_power.requestedPowerDown ?
        PowerState::WAKE_BLOCKED_OR_UNCONFIRMED : PowerState::POWER_DOWN_INDICATED;
  } else if ((_power.rawPhyscr&0x8000U)!=0U) { _power.state=PowerState::UNKNOWN; }
  else if ((_power.rawBiscr&0x0100U)!=0U) { _power.state=PowerState::NORMAL_INDICATED; }
  else { _power.state=(_power.rawPhyscr&0x7000U)==0x6000U ? PowerState::ACTIVE_SLEEP_INDICATED : PowerState::UNKNOWN; }
}
Status DP8382x::_advancePower(Status status,bool called,const MdioBatch& batch) noexcept {
  const auto last=static_cast<uint8_t>(batch.frameCount-1U);
  const auto& receipt=_transport.lastBatch;
  const bool complete=called && _result.failingBatch.completedFrames==batch.frameCount;
  if (called) {
    for (uint8_t i=0;i<_result.failingBatch.completedFrames;++i) {
      if (batch.frames[i].kind==FrameKind::READ) { _retainPowerWord(batch.frames[i].registerAddress,receipt.frames[i].value); }
    }
    if (batch.frames[last].kind==FrameKind::WRITE) {
      const auto& frame=receipt.frames[last];
      _result.mutationAttempted|=frame.attempted;
      _result.mutationCompleted|=complete;
      _result.mutationPossible|=frame.completed || frame.effectPossible || frame.valueValid || _transport.violation!=TransportViolation::NONE;
    }
  }
  uint16_t mask=0,expected=0;
  if (complete && batch.frames[last].kind==FrameKind::READ) {
    switch (_powerStep) {
      case PowerStep::VERIFY_BMCR: mask=0x0800; expected=_softwarePowerWord(); break;
      case PowerStep::VERIFY_CLEAR_PLL: mask=0x8000; break;
      case PowerStep::VERIFY_POLICY: mask=0x7000; expected=_power.desired.activeSleep?0x6000:0; break;
      case PowerStep::VERIFY_PLL: mask=0x8000; expected=0x8000; break;
      default: break;
    }
    if (mask!=0U && (receipt.frames[last].value&mask)!=expected && !_result.mismatch.valid) {
      _result.mismatch={true,false,0,batch.frames[last].registerAddress,mask,expected,receipt.frames[last].value};
    }
  }
  if (!status.ok()) { return _finishOperation(status); }
  if (called && _power.bmcrValid && (_power.rawBmcr&0x8000U)!=0U) {
    return _finishOperation(powerError("Reset observed during power procedure"));
  }
  if (_result.mismatch.valid) {
    if (_powerStep==PowerStep::VERIFY_BMCR && !_power.requestedPowerDown && _power.observedBmcrPowerDown) {
      _power.state=PowerState::WAKE_BLOCKED_OR_UNCONFIRMED;
    }
    return _finishOperation(Status::Error(Err::CONFIG_MISMATCH,"Power control readback unconfirmed",static_cast<int32_t>(ConfigDetail::POWER_STATE)));
  }
  switch (_powerStep) {
    case PowerStep::READ_BMCR:
      _setPowerStep(PowerStep::READ_PHYSCR); break;
    case PowerStep::READ_PHYSCR: {
      const auto valid=_validatePower(_power.desired); if (!valid.ok()) { return _finishOperation(valid); }
      if ((_power.rawPhyscr&4U)!=0U) { return _finishOperation(powerError("Unowned interrupt stimulus active")); }
      const bool pll=(_power.rawPhyscr&0x8000U)!=0U;
      if (pll && (!_power.observedBmcrPowerDown || !_power.desired.clockLossApproved)) {
        return _finishOperation(powerError("Existing PLL disable requires IEEE down and clock admission"));
      }
      if (_powerPreparingReset) {
        if (pll) { _setPowerStep(PowerStep::CLEAR_PLL); }
        else { _resumeAfterPowerPreparation(); }
      } else if (_powerVerifyOnly) {
        const auto expected=static_cast<uint16_t>((_power.desired.activeSleep?0x6000U:0U)|(_power.desired.disablePll?0x8000U:0U));
        if ((_power.rawPhyscr&0xF000U)!=expected || _power.observedBmcrPowerDown!=_power.requestedPowerDown) {
          const bool control=(_power.rawPhyscr&0xF000U)!=expected;
          _result.mismatch={true,false,0,static_cast<uint16_t>(control?0x11:0),static_cast<uint16_t>(control?0xF000:0x0800),
                            control?expected:_softwarePowerWord(),control?_power.rawPhyscr:_power.rawBmcr};
          return _finishOperation(Status::Error(Err::CONFIG_MISMATCH,"Owned power controls differ"));
        }
        _setPowerStep(PowerStep::BISCR);
      } else { _setPowerStep(pll && !_power.desired.disablePll?PowerStep::CLEAR_PLL:PowerStep::POLICY); }
      break;
    }
    case PowerStep::CLEAR_PLL: _setPowerStep(PowerStep::VERIFY_CLEAR_PLL); break;
    case PowerStep::VERIFY_CLEAR_PLL:
      if (_powerPreparingReset) { _resumeAfterPowerPreparation(); }
      else { _setPowerStep(PowerStep::POLICY); }
      break;
    case PowerStep::POLICY: _setPowerStep(PowerStep::VERIFY_POLICY); break;
    case PowerStep::VERIFY_POLICY: _setPowerStep(PowerStep::BMCR); break;
    case PowerStep::BMCR: _setPowerStep(PowerStep::VERIFY_BMCR); break;
    case PowerStep::VERIFY_BMCR: _setPowerStep(_power.desired.disablePll?PowerStep::PLL:PowerStep::BISCR); break;
    case PowerStep::PLL: _setPowerStep(PowerStep::VERIFY_PLL); break;
    case PowerStep::VERIFY_PLL: _setPowerStep(PowerStep::BISCR); break;
    case PowerStep::BISCR: {
      const auto expected=static_cast<uint16_t>((_power.desired.activeSleep?0x6000U:0U)|(_power.desired.disablePll?0x8000U:0U));
      const bool control=(_power.rawPhyscr&0xF000U)!=expected;
      if (control || _power.observedBmcrPowerDown!=_power.requestedPowerDown) {
        _result.mismatch={true,false,0,static_cast<uint16_t>(control?0x11:0),static_cast<uint16_t>(control?0xF000:0x0800),
                          control?expected:_softwarePowerWord(),control?_power.rawPhyscr:_power.rawBmcr};
        return _finishOperation(Status::Error(Err::CONFIG_MISMATCH,"Power evidence changed during verification"));
      }
      if (!_otherControlsVerified(OperationKind::POWER_CONFIGURE)) {
        return _finishOperation(powerError("Other owned controls lost verification"));
      }
      _observePower(); _power.trust=ConfigurationTrust::VERIFIED;
      _result.persistentRestored=true;
      if (_operation.kind!=OperationKind::VERIFY) { _initialized=true; }
      return _finishOperation(Status::Ok());
    }
  }
  return _operation.status;
}
Status DP8382x::readPowerStatus(PowerSnapshot& snapshot,const IoBudget& budget) noexcept {
  // _execute enforces the same OP14/raw-read guards and collector ownership.
  if (_active || _polling) { return Status::Error(Err::BUSY,"Power owner active",static_cast<int32_t>(BusyDetail::OPERATION_ACTIVE)); }
  auto batch=powerRead(0); batch.frameCount=3;
  batch.frames[1]={FrameKind::READ,0x11,0,0,NO_READ_INDEX};
  batch.frames[2]={FrameKind::READ,0x16,0,0,NO_READ_INDEX};
  bool called=false;
  const auto status=_execute(batch,budget,&called);
  bool resetObserved=false;
  if (called) {
    _power.observedStale=true; _power.state=PowerState::UNKNOWN;
    for (uint8_t i=0;i<3U;++i) {
      const auto& frame=_transport.lastBatch.frames[i];
      if (!frame.attempted || !frame.completed || !frame.valueValid) { break; }
      _retainPowerWord(batch.frames[i].registerAddress,frame.value);
      resetObserved|=i==0U && (frame.value&0x8000U)!=0U;
    }
  }
  if (resetObserved && _operation.state==OperationState::RUNNING) {
    _finishOperation(Status::Error(Err::INVALID_CONFIG,"Reset invalidated running operation",
                                  static_cast<int32_t>(ConfigDetail::DEVICE_INVALIDATED)));
  }
  _power.lastStatus=status;
  if (!status.ok()) { _power.observedStale=true; _power.state=PowerState::UNKNOWN; return status; }
  if ((_power.rawBmcr&0x8000U)!=0U) {
    _power.observedStale=true; _power.state=PowerState::UNKNOWN;
    _power.lastStatus=powerError("Reset observed during power acquisition");
    return _power.lastStatus;
  }
  _observePower();
  if (_power.desiredValid && ((_power.rawPhyscr&0xF000U)!=
      static_cast<uint16_t>((_power.desired.activeSleep?0x6000U:0U)|(_power.desired.disablePll?0x8000U:0U)) ||
      _power.observedBmcrPowerDown!=_power.requestedPowerDown)) { _power.trust=ConfigurationTrust::DIRTY; }
  snapshot=_power; return Status::Ok();
}
} // namespace DP8382x
