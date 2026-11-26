/// @file DP8382xAnalog.cpp
/// @brief Bounded owned analog profiles; SNLS647I tables 8-73/79..83/86.
#include <DP8382x/DP8382x.h>

namespace DP8382x {
namespace {
constexpr uint16_t addresses[]={0x030B,0x030C,0x030E,0x0461,0x0302,
                                0x0308,0x030F,0x0311,0x031C,0x0404,0x031F,0x033E};
constexpr uint8_t analogRows=static_cast<uint8_t>(std::size(addresses));
constexpr uint8_t firstDspRow=5;
// SNLS638C tables 7-62/65/66/68/72/69/71. Table 7-69 describes [5:1]
// as wake, despite its flush name. Its semantics are confined to the TI recipe.
constexpr uint16_t dspAllowed[]={0xFCF0,0x00FF,0xE000,0xC07F,0x00E0,0xFFC0,0x0060};
constexpr size_t group(DspControl control) noexcept { return static_cast<size_t>(control); }
bool ownsDsp(const AnalogSettings& settings) noexcept {
  for (const auto mask:settings.dspMasks) { if (mask!=0U) { return true; } }
  return false;
}
Status invalidAnalog(const char* message) noexcept {
  return Status::Error(Err::INVALID_CONFIG,message,static_cast<int32_t>(ConfigDetail::SOURCE_GATE));
}
Status analogBusy(BusyDetail detail) noexcept {
  return Status::Error(Err::BUSY,"Analog owner unavailable",static_cast<int32_t>(detail));
}
Status analogSyntax(const AnalogSettings& s) noexcept {
  if (s.minusMdi<0x28U || s.minusMdi>0x38U || s.minusMdix<0x28U || s.minusMdix>0x38U ||
      s.plusMdi<8U || s.plusMdi>0x18U || s.plusMdix<8U || s.plusMdix>0x18U || s.zeroCode>63U ||
      s.mdiTermination>15U || s.mdixTermination>15U || (s.macSlewCode!=0x10U && s.macSlewCode!=0x11U) ||
      s.ioSlewCode>1U || s.receiverGainCode>31U) {
    return Status::Error(Err::INVALID_PARAM,"Invalid analog field encoding",static_cast<int32_t>(ParamDetail::VALUE_WIDTH));
  }
  return Status::Ok();
}
bool completePrefix(const MdioBatch& batch,const BatchResult& receipt,uint8_t last) noexcept {
  for (uint8_t i=0;i<=last;++i) {
    const auto& f=receipt.frames[i];
    if (!f.attempted || !f.completed || f.valueValid!=(batch.frames[i].kind==FrameKind::READ)) { return false; }
  }
  return true;
}
}

bool DP8382x::_dspModeConflict(FunctionalMode mode,const AnalogSettings& settings) const noexcept {
  if (mode==FunctionalMode::RETAIN || mode==FunctionalMode::DEFAULT) { return false; }
  const auto& masks=settings.dspMasks;
  // Every mode writes 0311/0404 as whole words. Table 6-7 also writes
  // 0308/031C/031F/033E. A later replay cannot prove both owners' intent.
  return masks[group(DspControl::CAGC_MAPPING)]!=0U || masks[group(DspControl::LINE_DRIVER_CURRENT)]!=0U ||
      (mode==FunctionalMode::EXTENDED_REACH_EEE &&
       (masks[group(DspControl::LPI_FREEZE)]!=0U || masks[group(DspControl::TIMING_LOOP)]!=0U ||
        masks[group(DspControl::LPI_FREEZE_GROUPS)]!=0U || masks[group(DspControl::THRESHOLD_SCALING)]!=0U));
}

Status DP8382x::_validateAnalog(const AnalogSettings& s) const noexcept {
  const auto syntax=analogSyntax(s); if (!syntax.ok()) { return syntax; }
  const bool p25 = _config.variant==Variant::DP83825I;
  for (size_t i=0;i<std::size(dspAllowed);++i) {
    const auto mask=s.dspMasks[i], value=s.dspValues[i];
    if ((!p25 && mask!=0U) || (mask & ~dspAllowed[i])!=0U || (value & ~mask)!=0U) {
      return invalidAnalog("DSP controls require an exact DP83825I group and owned encoding");
    }
    if (_analog.desiredValid && (_analog.desired.dspMasks[i] & ~mask)!=0U) {
      return invalidAnalog("Retain all previously owned DSP controls");
    }
  }
  const size_t cagc=group(DspControl::CAGC_MAPPING), current=group(DspControl::LINE_DRIVER_CURRENT);
  const uint16_t gainMask=s.dspMasks[cagc]&0xC000U;
  if ((gainMask!=0U && gainMask!=0xC000U) || (s.dspValues[cagc]&0xC000U)==0xC000U ||
      (s.dspMasks[current]!=0U && s.dspMasks[current]!=0xE0U) ||
      (s.dspValues[current]!=0U && s.dspValues[current]!=0x80U && s.dspValues[current]!=0xE0U)) {
    return invalidAnalog("Unsupported CAGC mapping or line-driver current code");
  }
  if (_dspModeConflict(_functionalMode.desired,s)) { return invalidAnalog("Functional mode owns selected DSP words"); }
  if (s.vodCoarseStep>16U || s.vodFineStep < -2 || s.vodFineStep>2) {
    return Status::Error(Err::INVALID_PARAM,"DP83825I VOD step out of range");
  }
  if ((!s.vodOwned && (s.vodCoarseStep!=8U || s.vodFineStep!=0)) ||
      (!s.impedanceOwned && s.impedance25Ohm)) { return invalidAnalog("Select the corresponding analog owner"); }
  if (p25 && (s.amplitudeOwned || s.terminationOwned || s.ioSlewOwned || s.macSlewOwned)) {
    return invalidAnalog("DP83825I requires its VOD or impedance profile");
  }
  if (!p25 && (s.vodOwned || s.impedanceOwned)) { return invalidAnalog("VOD steps and impedance require DP83825I"); }
  if (_analog.desiredValid && ((_analog.desired.vodOwned && !s.vodOwned) ||
      (_analog.desired.impedanceOwned && !s.impedanceOwned))) { return invalidAnalog("Retain previously owned analog groups"); }
  if (s.vodOwned && _functionalMode.desired!=FunctionalMode::RETAIN && _functionalMode.desired!=FunctionalMode::DEFAULT) {
    return invalidAnalog("Functional mode already owns the coarse VOD words");
  }
  if (s.receiverGainOwned) { return invalidAnalog("ANA_RX10BT_CTRL physical mapping remains source-gated"); }
  if (!s.amplitudeOwned && !s.terminationOwned && !s.macSlewOwned && !s.ioSlewOwned && !s.vodOwned && !s.impedanceOwned && !ownsDsp(s)) {
    return invalidAnalog("Explicit analog ownership is required");
  }
  if (!s.maintenanceAdmitted || !s.macStopped || !s.fixtureAdmitted) {
    return invalidAnalog("Analog changes require maintenance, stopped MAC and identified fixture admission");
  }
  if (s.terminationOwned && !s.terminationOverrideAdmitted) { return invalidAnalog("Termination override requires explicit admission"); }
  if ((!s.amplitudeOwned && (s.minusMdi!=0x30U || s.minusMdix!=0x30U || s.plusMdi!=0x10U || s.plusMdix!=0x10U || s.zeroCode!=0U)) ||
      (!s.terminationOwned && (s.mdiTermination!=4U || s.mdixTermination!=4U || !s.mdiTerminationEnable || !s.mdixTerminationEnable)) ||
      (!s.macSlewOwned && s.macSlewCode!=0x10U) || (!s.ioSlewOwned && s.ioSlewCode!=0U) ||
      (!s.receiverGainOwned && s.receiverGainCode!=0U)) { return invalidAnalog("A requested analog setting requires ownership"); }
  if (_analog.desiredValid && ((_analog.desired.amplitudeOwned && !s.amplitudeOwned) ||
      (_analog.desired.terminationOwned && !s.terminationOwned) || (_analog.desired.macSlewOwned && !s.macSlewOwned) ||
      (_analog.desired.ioSlewOwned && !s.ioSlewOwned))) {
    return invalidAnalog("Replace all previously owned analog groups");
  }
  return Status::Ok();
}

Status DP8382x::startConfigureAnalog(const AnalogSettings& settings,const OperationBudget& budget,OperationToken& token) noexcept {
  const auto syntax=analogSyntax(settings); if (!syntax.ok()) { return syntax; }
  if (budget.pollIntervalUs==0U || budget.maxStalledPolls==0U) {
    return Status::Error(Err::INVALID_PARAM,"Invalid analog operation budget",static_cast<int32_t>(ParamDetail::BUDGET_RANGE));
  }
  if (!_bound) { return Status::Error(Err::NOT_BOUND,"No transport binding"); }
  if (_active || _polling || _operation.state==OperationState::RUNNING) { return analogBusy(BusyDetail::OPERATION_ACTIVE); }
  if (_operation.state==OperationState::RESULT_PENDING) { return analogBusy(BusyDetail::RESULT_PENDING); }
  if (_transport.clockInvalid) { return Status::Error(Err::INVALID_CONFIG,"Repair binding clock",static_cast<int32_t>(ConfigDetail::CLOCK_CONTRACT)); }
  if (_stopped) { return analogBusy(BusyDetail::CONTROLLER_QUARANTINED); }
  if (_operation.cleanupOwed || _operation.temporaryMayBeActive) { return analogBusy(BusyDetail::CLEANUP_REQUIRED); }
  if (!_initialized || !capabilities().valid) { return Status::Error(Err::NOT_INITIALIZED,"Initialize before analog configuration"); }
  const auto valid=_validateAnalog(settings); if (!valid.ok()) { return valid; }
  if (!_otherControlsVerified(OperationKind::ANALOG_CONFIGURE)) { return invalidAnalog("Verify other owned controls before analog changes"); }
  if (budget.io.maxFrames==0U || budget.io.deadlineUs==0U) {
    return Status::Error(Err::OPERATION_TIMEOUT,"Empty analog allowance",static_cast<int32_t>(
        budget.io.maxFrames==0U?TimeoutDetail::FRAME_BUDGET:TimeoutDetail::DEADLINE));
  }
  _beginOperation(OperationKind::ANALOG_CONFIGURE,budget);
  _analog.desired=settings; _analog.desiredValid=true; _analog.trust=ConfigurationTrust::DIRTY;
  _analogTailPending=false; _beginAnalog(false);
  token=_operation.token; return _operation.status;
}

uint16_t DP8382x::_analogVod3() const noexcept {
  if (_config.variant==Variant::DP83825I) { return _vodTrimWord; }
  if (!_analog.desiredValid || !_analog.desired.terminationOwned) { return 0x4A40; }
  const auto& s=_analog.desired;
  return static_cast<uint16_t>((uint16_t{s.mdiTermination}<<12U)|(s.mdiTerminationEnable?0x0800U:0U)|
      (uint16_t{s.mdixTermination}<<7U)|(s.mdixTerminationEnable?0x0040U:0U));
}
bool DP8382x::_analogRowValue(uint8_t row,uint16_t& address,uint16_t& mask,uint16_t& value) const noexcept {
  if (row>=analogRows) { return false; }
  address=addresses[row]; mask=value=0; const auto& s=_analog.desired;
  if (_config.variant==Variant::DP83825I) {
    // Table 8-2's 50% row is the printed 00E0, outside the regular steps.
    if (s.vodOwned && row<3U) {
      mask=0xFFFF;
      if (row==0U) { value=s.vodCoarseStep==0U?0x00E0U:static_cast<uint16_t>(0x0E00U-0x40U*s.vodCoarseStep); }
      else if (row==1U) { value=static_cast<uint16_t>(8U+s.vodCoarseStep); }
      else { value=_vodTrimWord; }
    }
    if (row>=firstDspRow) {
      mask=s.dspMasks[row-firstDspRow]; value=s.dspValues[row-firstDspRow];
    }
    if (s.impedanceOwned && row==4U) { mask=0xC000; value=s.impedance25Ohm?0x4000:0; }
    return mask!=0U;
  }
  switch (row) {
    case 0: if (s.amplitudeOwned) { mask=0x3FFF; value=static_cast<uint16_t>((static_cast<uint16_t>(s.minusMdix>>4U)<<12U)|(uint16_t{s.minusMdi}<<6U)|s.zeroCode); } break;
    case 1: if (s.amplitudeOwned) { mask=0xFFFF; value=static_cast<uint16_t>((static_cast<uint16_t>(s.minusMdix&15U)<<12U)|(uint16_t{s.plusMdix}<<6U)|s.plusMdi); } break;
    case 2: if (s.terminationOwned) { mask=0xFFC0; value=_analogVod3(); } break;
    case 3: if (s.macSlewOwned) { mask=0x001F; value=s.macSlewCode; } break;
    case 4: if (s.ioSlewOwned) { mask=0xC000; value=static_cast<uint16_t>(uint16_t{s.ioSlewCode}<<14U); } break;
    default: break;
  }
  return mask!=0U;
}
void DP8382x::_analogReplayCost(uint32_t& frames,uint32_t& callbacks) const noexcept {
  // One two-frame preflight, then snapshot, apply and verify each owned word.
  frames=_analog.desired.vodOwned?6U:2U; callbacks=1;
  for (uint8_t row=0;row<analogRows;++row) {
    uint16_t address=0,mask=0,value=0;
    if (_analogRowValue(row,address,mask,value)) {
      frames+=_config.variant==Variant::DP83825I && row>=4U?13U:12U;
      callbacks+=3;
    }
  }
  // HARD does not specify every split MDIX amplitude bit. Reserve a bridge
  // write and readback until the post-reset snapshot establishes both halves.
  if (_analog.desired.vodOwned) { frames-=8; callbacks-=2; } // 030E is a write-only recipe exception.
  if (_analog.desired.amplitudeOwned) { frames+=8; callbacks+=2; }
}
void DP8382x::_nextAnalogRow() noexcept {
  uint16_t a=0,m=0,v=0;
  while (_analogRow<analogRows && ((!_analogRowValue(_analogRow,a,m,v)) ||
      (_analog.desired.vodOwned && _analogRow==2U && (_analogVerifyOnly || _analogStep==AnalogStep::SNAPSHOT)))) { ++_analogRow; }
  _analogStep=_analogRow==analogRows?AnalogStep::COMPLETE:(_analogVerifyOnly?AnalogStep::VERIFY:AnalogStep::APPLY);
  _operation.phase=_analogVerifyOnly?OperationPhase::VERIFY:OperationPhase::RESTORE;
}
void DP8382x::_beginAnalog(bool verifyOnly) noexcept {
  _qualityWorking=_electricalWorking=_interfaceWorking=_pinsWorking=_wakeWorking=_eeeWorking=_powerWorking=_interruptWorking=false;
  _cableWorking=_bistWorking=_loopWorking=_configuring=false;
  _analogWorking=true; _analogVerifyOnly=verifyOnly; _analogRow=0;
  _analog.observedStale=true; _operation.nextPollValid=false; _operation.phase=OperationPhase::PREFLIGHT;
  const auto valid=_validateAnalog(_analog.desired);
  if (!valid.ok()) { _finishAnalog(valid); return; }
  _analogStep=AnalogStep::PREFLIGHT;
}
MdioBatch DP8382x::_analogBatch() const noexcept {
  if (_analogStep==AnalogStep::PREFLIGHT) {
    MdioBatch batch; batch.frameCount=2;
    batch.frames[0]={FrameKind::READ,0,0,0,NO_READ_INDEX};
    batch.frames[1]={FrameKind::READ,0x11,0,0,NO_READ_INDEX};
    if (_analog.desired.vodOwned) {
      const auto trim=_mmdBatch(0x0333,FrameKind::READ,0);
      for (uint8_t i=0;i<trim.frameCount;++i) { batch.frames[batch.frameCount++]=trim.frames[i]; }
    }
    return batch;
  }
  const bool bridge=_analogStep==AnalogStep::BRIDGE_WRITE || _analogStep==AnalogStep::BRIDGE_VERIFY;
  const uint8_t row=bridge?uint8_t{1}:_analogRow;
  uint16_t address=0,mask=0,value=0; _analogRowValue(row,address,mask,value);
  if (_analogStep==AnalogStep::APPLY || _analogStep==AnalogStep::BRIDGE_WRITE) {
    if (_config.variant==Variant::DP83825I && row>=4U) {
      // The read supplying unowned fields and the dependent data write share
      // one selector transaction. A different bus participant may have changed
      // unowned bits since our earlier snapshot. 0308[0] is status only.
      auto batch=_mmdBatch(address,FrameKind::READ,0);
      const uint16_t neutral=address==0x0308U?1U:0U;
      const auto preserve=static_cast<uint16_t>(~(mask|neutral));
      batch.frames[batch.frameCount++]={FrameKind::WRITE,0x0E,value,preserve,3};
      return batch;
    }
    const auto word=bridge?_analogWords[1]:static_cast<uint16_t>((_analogWords[row]&~mask)|value);
    return _mmdBatch(address,FrameKind::WRITE,word);
  }
  return _mmdBatch(address,FrameKind::READ,0);
}
void DP8382x::_retainAnalogWord(uint16_t address,uint16_t value) noexcept {
  uint8_t row=0; while (row<analogRows && addresses[row]!=address) { ++row; } if (row==analogRows || (row>=firstDspRow && _config.variant!=Variant::DP83825I)) { return; }
  const uint16_t previous=_analog.raw[row];
  _analog.raw[row]=value; _analog.validMask|=static_cast<uint16_t>(1U<<row);
  if (!_analog.desiredValid) { return; }
  uint16_t a=0,mask=0,expected=0; if (!_analogRowValue(row,a,mask,expected)) { return; }
  bool proved=!_analogWorking;
  if (_analogWorking && _analogStep!=AnalogStep::PREFLIGHT) {
    if (_analogVerifyOnly) { proved=true; }
    else {
      // Interleaved admitted reads must agree with the established current
      // image, including future rows whose snapshots supply later writes.
      // Only the immediate final readback expects the destination value.
      proved=_analogStep!=AnalogStep::SNAPSHOT || row<_analogRow;
      if (_analogStep!=AnalogStep::VERIFY || row!=_analogRow) { expected=static_cast<uint16_t>(_analogWords[row]&mask); }
      if (_analogStep==AnalogStep::BRIDGE_WRITE && row==1U) { expected=previous; }
    }
  }
  if ((value&mask)==expected || !proved) { return; }
  _analog.trust=ConfigurationTrust::DIRTY; _analog.observedStale=true;
  if (_analogWorking && !_result.mismatch.valid) { _result.mismatch={true,true,31,address,mask,expected,value}; }
}
Status DP8382x::_finishAnalog(Status status) noexcept {
  if (status.ok() && _result.mismatch.valid) { status=Status::Error(Err::CONFIG_MISMATCH,"Analog readback mismatch"); }
  if (status.ok() && _operation.kind==OperationKind::ANALOG_CONFIGURE && !_otherControlsVerified(OperationKind::ANALOG_CONFIGURE)) {
    status=invalidAnalog("Another owned policy lost verification during analog application");
  }
  _analogStep=AnalogStep::COMPLETE; _analog.lastStatus=status;
  _analog.trust=status.ok()?ConfigurationTrust::VERIFIED:ConfigurationTrust::DIRTY;
  _analog.observedStale=!status.ok();
  if (status.ok()) { _analog.timeValid=true; _analog.observedAtUs=_lastTimeUs+_config.transport.bounds.clockLagUs; }
  _result.analog=_analog; return status;
}
Status DP8382x::_advanceAnalog(Status status,bool called,const MdioBatch& batch) noexcept {
  const auto& receipt=_transport.lastBatch;
  const bool writing=_analogStep==AnalogStep::APPLY || _analogStep==AnalogStep::BRIDGE_WRITE;
  const uint8_t row=(_analogStep==AnalogStep::BRIDGE_WRITE || _analogStep==AnalogStep::BRIDGE_VERIFY)?uint8_t{1}:_analogRow;
  if (called && writing) {
    const auto last=static_cast<uint8_t>(batch.frameCount-1U);
    const auto& target=receipt.frames[last];
    if (_analog.desired.vodOwned && row==2U && (target.attempted || _transport.violation!=TransportViolation::NONE)) { _vodTrimAccepted=false; }
    _result.mutationAttempted|=target.attempted;
    _result.mutationCompleted|=completePrefix(batch,receipt,last);
    _result.mutationPossible|=target.completed || target.effectPossible || target.valueValid || _transport.violation!=TransportViolation::NONE;
  }
  if (!writing && _analogStep!=AnalogStep::PREFLIGHT && row<analogRows && (!called || !completePrefix(batch,receipt,3))) {
    _analog.validMask&=static_cast<uint16_t>(~(1U<<row));
  }
  if (!status.ok()) {
    if (_analogStep==AnalogStep::SNAPSHOT && _analog.desired.amplitudeOwned && _analogRow<2U) {
      _operation.cleanupOwed=true; _cleanupOwner=OperationKind::ANALOG_CONFIGURE;
    }
    return _finishAnalog(status);
  }
  if (_result.mismatch.valid && !_analogVerifyOnly) { return _finishAnalog(Status::Error(Err::CONFIG_MISMATCH,"Analog evidence changed")); }
  switch (_analogStep) {
    case AnalogStep::PREFLIGHT: {
      if (_analog.desired.vodOwned) {
        const uint16_t trim=receipt.frames[5].value;
        const int c=8-static_cast<int>(trim>>11U)+static_cast<int>((trim>>6U)&31U);
        const int d=(c<0?0:c>15?15:c)+_analog.desired.vodFineStep;
        if (d<0 || d>15) { return _finishAnalog(invalidAnalog("Fine VOD offset exceeds unit trim range")); }
        _vodTrimWord=static_cast<uint16_t>((2*d+1)*2048);
        if (_analogVerifyOnly && (!_vodTrimAccepted || _appliedVodTrim!=_vodTrimWord)) {
          return _finishAnalog(invalidAnalog("Restore the device-specific fine VOD recipe"));
        }
      }
      if ((receipt.frames[0].value&0x8200U)!=0U || (!_analogVerifyOnly && (receipt.frames[0].value&0xCC80U)!=0U)) {
        return _finishAnalog(invalidAnalog("Restore BMCR commands, power, isolation and test state before analog mutation"));
      }
      if (!_analogVerifyOnly && (receipt.frames[1].value&0x8004U)!=0U) { return _finishAnalog(invalidAnalog("Restore PLL and interrupt stimulus first")); }
      _nextAnalogRow(); if (!_analogVerifyOnly && _analogStep!=AnalogStep::COMPLETE) { _analogStep=AnalogStep::SNAPSHOT; } break;
    }
    case AnalogStep::SNAPSHOT: {
      _analogWords[row]=receipt.frames[3].value;
      ++_analogRow; _nextAnalogRow();
      if (_analogStep!=AnalogStep::COMPLETE) { _analogStep=AnalogStep::SNAPSHOT; break; }
      _analogRow=0; _analogStep=AnalogStep::APPLY; _nextAnalogRow();
      if (_analog.desired.amplitudeOwned) {
        const auto current=static_cast<uint8_t>(((_analogWords[0]>>12U)&3U)*16U+(_analogWords[1]>>12U));
        if (current<0x28U || current>0x38U) {
          _operation.cleanupOwed=true; _cleanupOwner=OperationKind::ANALOG_CONFIGURE;
          return _finishAnalog(invalidAnalog("Current split MDIX amplitude is undocumented; restoration must establish a legal code"));
        }
        if ((current>>4U)!=(_analog.desired.minusMdix>>4U)) {
          _analogWords[1]=static_cast<uint16_t>((_analogWords[1]&0x0FFFU)|0x8000U); _analogStep=AnalogStep::BRIDGE_WRITE;
        }
      }
      break;
    }
    case AnalogStep::BRIDGE_WRITE: _analogStep=AnalogStep::BRIDGE_VERIFY; _operation.phase=OperationPhase::VERIFY; break;
    case AnalogStep::APPLY:
      if (_analog.desired.vodOwned && row==2U) {
        // SNLS638C table 8-3 authorizes a computed write. There is no
        // documented 030E read to qualify the applied voltage or trim.
        _appliedVodTrim=_vodTrimWord; _vodTrimAccepted=true; ++_analogRow; _nextAnalogRow();
      } else { _analogStep=AnalogStep::VERIFY; _operation.phase=OperationPhase::VERIFY; }
      break;
    case AnalogStep::BRIDGE_VERIFY:
    case AnalogStep::VERIFY: {
      uint16_t address=0,mask=0,expected=0; _analogRowValue(row,address,mask,expected);
      const bool bridge=_analogStep==AnalogStep::BRIDGE_VERIFY;
      if (bridge) { mask=0xFFFF; expected=_analogWords[1]; }
      const auto value=receipt.frames[3].value;
      if ((value&mask)!=expected) {
        if (!_result.mismatch.valid) { _result.mismatch={true,true,31,address,mask,expected,value}; }
        if (!_analogVerifyOnly) { return _finishAnalog(Status::Error(Err::CONFIG_MISMATCH,"Analog readback mismatch")); }
      }
      _analogWords[row]=value;
      if (bridge) { _analogStep=AnalogStep::APPLY; _operation.phase=OperationPhase::RESTORE; }
      else { ++_analogRow; _nextAnalogRow(); }
      break;
    }
    case AnalogStep::COMPLETE: break;
  }
  return _analogStep==AnalogStep::COMPLETE?_finishAnalog(Status::Ok()):Status::Error(Err::IN_PROGRESS,"Analog pass in progress");
}
} // namespace DP8382x
