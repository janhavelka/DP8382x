/// @file DP8382xQuality.cpp
/// @brief Owned quality/TX/link-loss profiles and unscaled MSE observations.
#include <DP8382x/DP8382x.h>

namespace DP8382x {
namespace {
constexpr uint8_t QUALITY_ROWS=9;
Status qualityConfig(const char* message) noexcept {
  return Status::Error(Err::INVALID_CONFIG,message,static_cast<int32_t>(ConfigDetail::SOURCE_GATE));
}
Status qualityBusy(BusyDetail detail) noexcept {
  return Status::Error(Err::BUSY,"Quality owner unavailable",static_cast<int32_t>(detail));
}
Status qualitySyntax(const QualitySettings& settings) noexcept {
  const auto& fld=settings.fastDrop;
  if ((fld.causeMask&0xFBF0U)!=0U || fld.windowCode==0U || fld.windowCode>63U ||
      fld.errorThreshold==0U || fld.errorThreshold>63U || fld.descramblerThreshold>63U ||
      settings.tx100.fifoDepthCode>3U || settings.tx10.squelchCode>8U) {
    return Status::Error(Err::INVALID_PARAM,"Invalid quality field encoding",static_cast<int32_t>(ParamDetail::VALUE_WIDTH));
  }
  return Status::Ok();
}
int qualityWord(uint16_t address) noexcept {
  constexpr uint16_t addresses[]={0x000B,0x0117,0x0131,0x0011,0x000A,0x001A,0x002A,0x0009};
  for (unsigned i=0;i<8U;++i) { if (addresses[i]==address) { return static_cast<int>(i); } }
  return -1;
}
bool qualityPrefix(const MdioBatch& batch,const BatchResult& receipt,uint8_t last) noexcept {
  for (uint8_t i=0;i<=last;++i) {
    const auto& frame=receipt.frames[i];
    if (!frame.attempted || !frame.completed || frame.valueValid!=(batch.frames[i].kind==FrameKind::READ)) { return false; }
  }
  return true;
}
}

Status DP8382x::_validateQuality(const QualitySettings& settings) const noexcept {
  const auto syntax=qualitySyntax(settings); if (!syntax.ok()) { return syntax; }
  const auto& fld=settings.fastDrop;
  if (_config.variant == Variant::DP83825I &&
      (fld.rawThresholdOverride || settings.tx10.owned || settings.recoveryOwned ||
       settings.tx100.scramblerBypass || settings.tx100.fifoDepthCode != 0U)) {
    return _unsupportedFeature();
  }
  if (isA(_config.variant)) {
    if ((fld.causeMask&0x0406U)!=0U && (fld.causeMask&0x0009U)==0U) {
      return qualityConfig("A-generation MLT3, SNR and descrambler detectors require RX error or energy detection");
    }
    if (fld.owned && !_straps.sor1Valid) {
      return qualityConfig("Fast Link Drop requires current SOR1 strap evidence");
    }
  }
  if (!fld.owned && !settings.tx100.owned && !settings.tx10.owned && !settings.recoveryOwned) {
    return qualityConfig("Explicit quality ownership is required");
  }
  if (!settings.maintenanceAdmitted || !settings.macStopped) {
    return qualityConfig("Quality mutation requires maintenance admission and stopped MAC traffic");
  }
  if (fld.causeMask!=0U && (!fld.owned || (_config.variant!=Variant::DP83825I && _config.interface!=MacInterface::MII))) {
    return qualityConfig("Nonzero Fast Link Drop causes require owned MII policy");
  }
  if (!fld.rawThresholdOverride && (fld.windowCode!=1U || fld.errorThreshold!=1U || fld.descramblerThreshold!=8U)) {
    return qualityConfig("Nonrecipe FLD thresholds require explicit raw override under fld_threshold_recipe");
  }
  if (fld.rawThresholdOverride && !fld.owned) { return qualityConfig("Raw FLD thresholds require ownership"); }
  if ((!settings.tx100.owned && (settings.tx100.scramblerBypass || settings.tx100.fifoDepthCode!=0U || settings.tx100.rxErrorDuringIdle)) ||
      (!settings.tx10.owned && (settings.tx10.lowerThreshold || settings.tx10.squelchCode!=0U || settings.tx10.nlpDisable ||
          settings.tx10.jabberDisable || settings.tx10.shortPreamble)) ||
      (!settings.recoveryOwned && settings.linkRecovery)) {
    return qualityConfig("A requested control requires its explicit owner");
  }
  if (settings.tx10.owned && !settings.partnerPreambleAdmitted) {
    return qualityConfig("10M preamble policy requires partner admission");
  }
  if (_quality.desiredValid &&
      ((_quality.desired.fastDrop.owned && !fld.owned) || (_quality.desired.tx100.owned && !settings.tx100.owned) ||
       (_quality.desired.tx10.owned && !settings.tx10.owned) || (_quality.desired.recoveryOwned && !settings.recoveryOwned))) {
    return qualityConfig("Replace all previously owned quality controls; do not abandon persistent ownership");
  }
  return Status::Ok();
}

Status DP8382x::startConfigureQuality(const QualitySettings& settings,const OperationBudget& budget,OperationToken& token) noexcept {
  const auto syntax=qualitySyntax(settings); if (!syntax.ok()) { return syntax; }
  if (budget.pollIntervalUs==0U || budget.maxStalledPolls==0U) {
    return Status::Error(Err::INVALID_PARAM,"Invalid quality operation budget",static_cast<int32_t>(ParamDetail::BUDGET_RANGE));
  }
  if (!_bound) { return Status::Error(Err::NOT_BOUND,"No transport binding"); }
  if (_active || _polling || _operation.state==OperationState::RUNNING) { return qualityBusy(BusyDetail::OPERATION_ACTIVE); }
  if (_operation.state==OperationState::RESULT_PENDING) { return qualityBusy(BusyDetail::RESULT_PENDING); }
  if (_transport.clockInvalid) {
    return Status::Error(Err::INVALID_CONFIG,"Repair binding clock before quality work",static_cast<int32_t>(ConfigDetail::CLOCK_CONTRACT));
  }
  if (_stopped) { return qualityBusy(BusyDetail::CONTROLLER_QUARANTINED); }
  if (_operation.cleanupOwed || _operation.temporaryMayBeActive) { return qualityBusy(BusyDetail::CLEANUP_REQUIRED); }
  if (!_initialized || !capabilities().valid) { return Status::Error(Err::NOT_INITIALIZED,"Initialize before quality configuration"); }
  const auto valid=_validateQuality(settings); if (!valid.ok()) { return valid; }
  if (!_otherControlsVerified(OperationKind::QUALITY_CONFIGURE)) { return qualityConfig("Verify other owned controls before quality changes"); }
  if (budget.io.maxFrames==0U || budget.io.deadlineUs==0U) {
    return Status::Error(Err::OPERATION_TIMEOUT,"Empty quality allowance",static_cast<int32_t>(
        budget.io.maxFrames==0U?TimeoutDetail::FRAME_BUDGET:TimeoutDetail::DEADLINE));
  }
  _beginOperation(OperationKind::QUALITY_CONFIGURE,budget);
  _quality.desired=settings; _quality.desiredValid=true;
  _quality.trust=ConfigurationTrust::DIRTY; _quality.observedStale=true;
  _qualityTailPending=false;
  _beginQuality(false);
  token=_operation.token; return _operation.status;
}

bool DP8382x::_qualityRowValue(uint8_t row,uint16_t& address,uint16_t& mask,uint16_t& value,bool readback) const noexcept {
  const auto& settings=_quality.desired;
  const auto& fld=settings.fastDrop;
  mask=value=0;
  if (_config.variant == Variant::DP83825I) {
    // SNLS638C 6.3.16.2 and table 7-14 permit every combination. Only CR3
    // belongs to this procedure; the DP83826 threshold recipe does not apply.
    if (row==8 && fld.owned) {
      address=0x000B; mask=0x040F; value=fld.causeMask;
      return true;
    }
    // CR2[2] controls idle RX errors. PHYSCR[11,9:8] belong to the
    // compliance and loopback procedures in SNLS638C table 7-20.
    if (row!=4) { return false; }
  }
  // SNLS647I 8.3.17: disable detectors before their threshold recipes; restore
  // only explicitly owned stable fields, then enable the selected cause set.
  switch (row) {
    case 0: address=0x000B; if (fld.owned && fld.causeMask!=0U) { mask=0x040F; } break;
    case 1:
      address=0x0117;
      if (fld.owned && (fld.causeMask&4U)!=0U) {
        mask=0xFFF0; value=static_cast<uint16_t>((uint16_t{fld.windowCode}<<10U)|(uint16_t{fld.errorThreshold}<<4U));
        // SNLS783A 8.3.17 requires the complete documented threshold word.
        // Include the read-only nibble in every proof while preserving it on writes.
        if (readback && isA(_config.variant) && !fld.rawThresholdOverride) { mask=0xFFFF; value=0x0417; }
      }
      break;
    case 2:
      address=0x0131;
      if (fld.owned && (fld.causeMask&0x0400U)!=0U) { mask=0x003F; value=fld.descramblerThreshold; }
      break;
    case 3:
      address=0x0011;
      if (settings.tx100.owned) {
        mask=0x0B00; value=static_cast<uint16_t>((settings.tx100.scramblerBypass?0x0800U:0U)|(uint16_t{settings.tx100.fifoDepthCode}<<8U));
      }
      break;
    case 4: address=0x000A; if (settings.tx100.owned) { mask=4; value=settings.tx100.rxErrorDuringIdle?4:0; } break;
    case 5:
      address=0x001A;
      if (settings.tx10.owned) {
        mask=0x3E81;
        value=static_cast<uint16_t>((settings.tx10.lowerThreshold?0x2000U:0U)|(uint16_t{settings.tx10.squelchCode}<<9U)|
            (settings.tx10.nlpDisable?0x0080U:0U)|(settings.tx10.jabberDisable?1U:0U));
      }
      break;
    case 6: address=0x002A; if (settings.tx10.owned) { mask=0x4000; value=settings.tx10.shortPreamble?0x4000:0; } break;
    case 7: address=0x0009; if (settings.recoveryOwned) { mask=0x0080; value=settings.linkRecovery?0x0080:0; } break;
    case 8: address=0x000B; if (fld.owned) { mask=0x040F; value=fld.causeMask; } break;
    default: return false;
  }
  return mask!=0U;
}

void DP8382x::_nextQualityRow() noexcept {
  uint16_t address=0,mask=0,value=0;
  while (_qualityRow<QUALITY_ROWS && !_qualityRowValue(_qualityRow,address,mask,value)) { ++_qualityRow; }
  _qualityStep=_qualityRow<QUALITY_ROWS?(_qualityVerifyOnly?QualityStep::VERIFY:QualityStep::APPLY):QualityStep::COMPLETE;
  _operation.phase=_qualityStep==QualityStep::APPLY?OperationPhase::RESTORE:OperationPhase::VERIFY;
}
void DP8382x::_qualityPassCost(bool verifyOnly,uint32_t& frames,uint32_t& callbacks) const noexcept {
  // Five direct preflight samples share one acquisition. Each selected
  // direct/MMD row then requires a read, or an atomic RMW plus readback.
  frames=5; callbacks=1;
  for (uint8_t row=verifyOnly?1:0;row<QUALITY_ROWS;++row) {
    uint16_t address=0,mask=0,value=0;
    if (_qualityRowValue(row,address,mask,value)) {
      const uint32_t readFrames=address>0x1FU?4U:address==0x001AU?2U:1U;
      frames+=verifyOnly?readFrames:readFrames*2U+1U;
      callbacks+=verifyOnly?1U:2U;
    }
  }
}
void DP8382x::_beginQuality(bool verifyOnly) noexcept {
  _interfaceWorking=_pinsWorking=_wakeWorking=_eeeWorking=_powerWorking=_interruptWorking=false;
  _cableWorking=_bistWorking=_loopWorking=_configuring=false;
  _qualityWorking=true; _qualityVerifyOnly=verifyOnly; _qualityRow=verifyOnly?1:0;
  _quality.observedStale=true; _quality.fldMaximumApplicable=false;
  _operation.nextPollValid=false; _operation.phase=OperationPhase::PREFLIGHT;
  const auto valid=_validateQuality(_quality.desired);
  if (!valid.ok()) { _finishQuality(valid); return; }
  _qualityStep=QualityStep::PREFLIGHT;
}

MdioBatch DP8382x::_qualityBatch() const noexcept {
  MdioBatch batch;
  if (_qualityStep==QualityStep::PREFLIGHT) {
    // Preserve mirror/status before destructive FLDS and RCSR observations.
    constexpr uint8_t addresses[]={0,0x11,0x17,0x10,0x0F};
    batch.frameCount=5;
    for (uint8_t i=0;i<5U;++i) { batch.frames[i]={FrameKind::READ,addresses[i],0,0,NO_READ_INDEX}; }
    return batch;
  }
  uint16_t address=0,mask=0,value=0; _qualityRowValue(_qualityRow,address,mask,value);
  if (address>0x1FU) { batch=_mmdBatch(address,FrameKind::READ,0); }
  else {
    if (address==0x001AU) { batch.frames[batch.frameCount++]={FrameKind::READ,0x10,0,0,NO_READ_INDEX}; }
    batch.frames[batch.frameCount++]={FrameKind::READ,static_cast<uint8_t>(address),0,0,NO_READ_INDEX};
  }
  if (_qualityStep==QualityStep::APPLY) {
    // PHYSCR test stimulus is neutral zero.10BTSCR polarity is consumed by the
    // collector; do not echo its read-clear bit into an unrelated stable write.
    const uint16_t neutral=address==0x0011U?uint16_t{4}:address==0x001AU?uint16_t{0x10}:uint16_t{0};
    const auto read=static_cast<uint8_t>(batch.frameCount-1U);
    batch.frames[batch.frameCount++]={FrameKind::WRITE,address<=0x1FU?static_cast<uint8_t>(address):uint8_t{0x0E},
        value,static_cast<uint16_t>(~(mask|neutral)),read};
  }
  return batch;
}

void DP8382x::_retainQualityWord(uint16_t address,uint16_t value) noexcept {
  if (_config.variant!=Variant::DP83825I && address==0x0017U && _quality.desiredValid && _quality.desired.fastDrop.owned &&
      _quality.desired.fastDrop.causeMask!=0U && (value&0x0020U)!=0U) {
    _quality.trust=ConfigurationTrust::DIRTY; _quality.observedStale=true; _quality.fldMaximumApplicable=false;
    if (_qualityWorking && !_result.mismatch.valid) {
      _result.mismatch={true,false,0,0x0017,0x0020,0,value};
    }
  }
  const int index=qualityWord(address); if (index<0) { return; }
  _quality.raw[static_cast<unsigned>(index)]=value; _quality.validMask|=static_cast<uint8_t>(1U<<index);
  if (!_quality.desiredValid) { return; }
  // The source recipe prints the whole word 0417. Preserve its RO nibble even
  // when unexpected, but do not claim that a different word met that recipe.
  if (address==0x0117U && value!=0x0417U && (_quality.desired.fastDrop.causeMask&4U)!=0U) {
    _quality.fldMaximumApplicable=false;
  }
  // CR3 row 0 is an intentional transient disable. Its final proof is row 8;
  // later observations during threshold work must instead retain disable proof.
  const uint8_t row=index==0?uint8_t{8}:static_cast<uint8_t>(index);
  uint16_t target=0,mask=0,expected=0;
  if (!_qualityRowValue(row,target,mask,expected,true)) { return; }
  bool proved=!_qualityWorking || (_qualityStep!=QualityStep::PREFLIGHT &&
      (_qualityVerifyOnly || row<_qualityRow || (row==_qualityRow && _qualityStep==QualityStep::VERIFY)));
  if (index==0 && _qualityWorking && !_qualityVerifyOnly && _qualityStep!=QualityStep::PREFLIGHT &&
      (_qualityRow>0U && _qualityRow<8U)) { expected=0; proved=true; }
  if ((value&mask)==expected) { return; }
  _quality.trust=ConfigurationTrust::DIRTY; _quality.observedStale=true; _quality.fldMaximumApplicable=false;
  if (_qualityWorking && proved && !_result.mismatch.valid) {
    _result.mismatch={true,address>0x1FU,static_cast<uint8_t>(address>0x1FU?31:0),address,mask,expected,value};
  }
}

Status DP8382x::_finishQuality(Status status) noexcept {
  if (status.ok() && _result.mismatch.valid) { status=Status::Error(Err::CONFIG_MISMATCH,"Quality readback mismatch"); }
  if (status.ok() && _operation.kind==OperationKind::QUALITY_CONFIGURE && !_otherControlsVerified(OperationKind::QUALITY_CONFIGURE)) {
    status=qualityConfig("Another owned policy lost verification during quality application");
  }
  _qualityStep=QualityStep::COMPLETE;
  _quality.lastStatus=status; _quality.trust=status.ok()?ConfigurationTrust::VERIFIED:ConfigurationTrust::DIRTY;
  _quality.observedStale=!status.ok();
  const auto& fld=_quality.desired.fastDrop;
  // 7.6 footnote 5: SNR/MLT3/descrambler maxima also require RX-error or energy.
  _quality.fldMaximumApplicable=_config.variant!=Variant::DP83825I && status.ok() && fld.owned && fld.causeMask!=0U && !fld.rawThresholdOverride &&
      ((fld.causeMask&0x0406U)==0U || (fld.causeMask&0x0009U)!=0U) &&
      ((fld.causeMask&4U)==0U || _quality.raw[1]==0x0417U);
  if (status.ok()) { _quality.timeValid=true; _quality.observedAtUs=_lastTimeUs+_config.transport.bounds.clockLagUs; }
  _result.quality=_quality;
  return status;
}

Status DP8382x::_advanceQuality(Status status,bool called,const MdioBatch& batch) noexcept {
  const auto& receipt=_transport.lastBatch;
  if (called && batch.frames[batch.frameCount-1U].kind==FrameKind::WRITE) {
    const auto last=static_cast<uint8_t>(batch.frameCount-1U);
    const auto& target=receipt.frames[last];
    _result.mutationAttempted|=target.attempted;
    _result.mutationCompleted|=qualityPrefix(batch,receipt,last);
    _result.mutationPossible|=target.completed || target.effectPossible || target.valueValid || _transport.violation!=TransportViolation::NONE;
  }
  uint16_t address=0,mask=0,expected=0;
  uint8_t readIndex=0;
  if (_qualityStep==QualityStep::PREFLIGHT) {
    if (!called || !qualityPrefix(batch,receipt,1)) { _quality.validMask&=0xF7U; }
  } else {
    _qualityRowValue(_qualityRow,address,mask,expected,true);
    readIndex=address>0x1FU?uint8_t{3}:address==0x001AU?uint8_t{1}:uint8_t{0};
    const int index=qualityWord(address);
    if (index>=0 && (!called || !qualityPrefix(batch,receipt,readIndex))) {
      _quality.validMask&=static_cast<uint8_t>(~(1U<<index));
    }
  }
  if (!status.ok()) { return _finishQuality(status); }
  if (_result.mismatch.valid && !_qualityVerifyOnly) { return _finishQuality(Status::Error(Err::CONFIG_MISMATCH,"Quality evidence mismatch")); }
  switch (_qualityStep) {
    case QualityStep::PREFLIGHT: {
      const auto bmcr=receipt.frames[0].value,physcr=receipt.frames[1].value;
      if ((bmcr&0x8200U)!=0U) { return _finishQuality(qualityConfig("Reset/restart is active")); }
      if (!_qualityVerifyOnly && (physcr&0x8004U)!=0U) { return _finishQuality(qualityConfig("Restore PLL or interrupt stimulus before quality mutation")); }
      if (_config.variant==Variant::DP83825I && _clockRoleMismatch().valid) {
        return _finishQuality(Status::Error(Err::CONFIG_MISMATCH,"RMII strap evidence changed"));
      }
      if (_config.variant!=Variant::DP83825I && _quality.desired.fastDrop.causeMask!=0U && (receipt.frames[2].value&0x0020U)!=0U) {
        return _finishQuality(qualityConfig("Observed RMII mode cannot enable Fast Link Drop"));
      }
      _nextQualityRow(); break;
    }
    case QualityStep::APPLY:
      _qualityStep=QualityStep::VERIFY; _operation.phase=OperationPhase::VERIFY; break;
    case QualityStep::VERIFY:
      if ((receipt.frames[readIndex].value&mask)!=expected) {
        if (!_result.mismatch.valid) {
          _result.mismatch={true,address>0x1FU,static_cast<uint8_t>(address>0x1FU?31:0),address,mask,expected,receipt.frames[readIndex].value};
        }
        if (!_qualityVerifyOnly) { return _finishQuality(Status::Error(Err::CONFIG_MISMATCH,"Quality readback mismatch")); }
      }
      ++_qualityRow; _nextQualityRow(); break;
    case QualityStep::COMPLETE: break;
  }
  return _qualityStep==QualityStep::COMPLETE?_finishQuality(Status::Ok()):Status::Error(Err::IN_PROGRESS,"Quality pass in progress");
}

Status DP8382x::readSignalQuality(SignalQualitySnapshot& snapshot,const IoBudget& budget) noexcept {
  if (_config.variant == Variant::DP83825I) { return _unsupportedFeature(); }
  if (!_bound) { return Status::Error(Err::NOT_BOUND,"No transport binding"); }
  if (_active || _polling || (_operation.state==OperationState::RUNNING &&
      !_qualityWorking && !_configuring && !_cableWorking && !_interruptWorking && !_powerWorking &&
      !_eeeWorking && !_wakeWorking && !_pinsWorking && !_interfaceWorking && _operation.phase==OperationPhase::WAIT)) {
    return qualityBusy(BusyDetail::OPERATION_ACTIVE);
  }
  auto fail=[this](Status status) noexcept {
    _signalQuality.stale=true; _signalQuality.lastStatus=status; return status;
  };
  if (_stopped) { return fail(qualityBusy(BusyDetail::CONTROLLER_QUARANTINED)); }
  if (!capabilities().valid) { return fail(Status::Error(Err::NOT_INITIALIZED,"Accepted identity required for raw MSE")); }
  const auto context=_link;
  const auto identity=_identity;
  const auto status=_execute(_mmdBatch(0x0218,FrameKind::READ,0),budget,nullptr,false,Traffic::MANAGED);
  if (!status.ok()) { return fail(status); }
  SignalQualitySnapshot next;
  next.rawMse=_transport.lastBatch.frames[3].value;
  next.valid=true; next.stale=false; next.timeValid=true;
  next.observedAtUs=_lastTimeUs+_config.transport.bounds.clockLagUs;
  next.linkContext=context; next.identity=identity; next.lastStatus=Status::Ok();
  _signalQuality=next; snapshot=next; return Status::Ok();
}
} // namespace DP8382x
