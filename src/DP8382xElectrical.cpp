/// @file DP8382xElectrical.cpp
/// @brief Finite source-specific electrical stimuli and calibration contingency.
#include <DP8382x/DP8382x.h>

namespace DP8382x {
namespace {
Status electricalConfig(const char* text) noexcept {
  return Status::Error(Err::INVALID_CONFIG,text,static_cast<int32_t>(ConfigDetail::SOURCE_GATE));
}
Status electricalBusy(BusyDetail detail) noexcept {
  return Status::Error(Err::BUSY,"Electrical owner unavailable",static_cast<int32_t>(detail));
}
MdioBatch electricalRead(uint8_t reg) noexcept {
  MdioBatch batch; batch.frameCount=1; batch.frames[0]={FrameKind::READ,reg,0,0,NO_READ_INDEX}; return batch;
}
}

Status DP8382x::_admitElectrical(const OperationBudget& budget,bool stop) const noexcept {
  if (isA(_config.variant)) { return _unsupportedFeature(); }
  if (budget.pollIntervalUs==0U || budget.maxStalledPolls==0U) {
    return Status::Error(Err::INVALID_PARAM,"Invalid electrical polling budget",static_cast<int32_t>(ParamDetail::BUDGET_RANGE));
  }
  if (!_bound) { return Status::Error(Err::NOT_BOUND,"No transport binding"); }
  if (_active || _polling || _operation.state==OperationState::RUNNING) { return electricalBusy(BusyDetail::OPERATION_ACTIVE); }
  if (_operation.state==OperationState::RESULT_PENDING) { return electricalBusy(BusyDetail::RESULT_PENDING); }
  if (_transport.clockInvalid) {
    return Status::Error(Err::INVALID_CONFIG,"Repair binding clock before electrical work",static_cast<int32_t>(ConfigDetail::CLOCK_CONTRACT));
  }
  if (_stopped) { return electricalBusy(BusyDetail::CONTROLLER_QUARANTINED); }
  if (!stop) {
    if (_operation.cleanupOwed || _operation.temporaryMayBeActive) { return electricalBusy(BusyDetail::CLEANUP_REQUIRED); }
    if (!_initialized || !capabilities().valid) { return Status::Error(Err::NOT_INITIALIZED,"Initialize before electrical work"); }
    if (_softwarePowerWord()!=0U || (_power.desiredValid && _power.desired.disablePll) ||
        (_interface.desiredValid && _interface.desired.isolationOwned && _interface.desired.isolate)) {
      return electricalConfig("Release owned power-down/isolation before electrical stimuli");
    }
    if (!_otherControlsVerified(OperationKind::NONE)) { return electricalConfig("Verify owned controls before electrical work"); }
    if (_eee.desiredValid && _eee.desired.enabled) { return electricalConfig("Disable EEE before electrical stimuli"); }
  }
  if (budget.io.maxFrames==0U || budget.io.deadlineUs==0U) {
    return Status::Error(Err::OPERATION_TIMEOUT,"Empty electrical allowance",static_cast<int32_t>(
        budget.io.maxFrames==0U?TimeoutDetail::FRAME_BUDGET:TimeoutDetail::DEADLINE));
  }
  return Status::Ok();
}

Status DP8382x::startCompliance(const ComplianceOptions& options,const OperationBudget& budget,OperationToken& token) noexcept {
  if (options.mode>ComplianceMode::COUNT100 ||
      (options.pair!=PairSelection::MDI && options.pair!=PairSelection::MDIX) ||
      (options.mode==ComplianceMode::PATTERN10 && options.pattern>13U) ||
      (options.mode==ComplianceMode::COUNT100 && options.pattern>31U) ||
      (options.mode!=ComplianceMode::PATTERN10 && options.mode!=ComplianceMode::COUNT100 && options.pattern!=0U)) {
    return Status::Error(Err::INVALID_PARAM,"Invalid compliance option",static_cast<int32_t>(ParamDetail::VALUE_WIDTH));
  }
  if (options.mode==ComplianceMode::COUNT100) { return electricalConfig("compt_reserved:100M0428 requires exact-part access evidence"); }
  if (!options.maintenanceAdmitted || !options.macStopped || !options.fixtureAdmitted || !options.hardResetAdmitted) {
    return electricalConfig("Compliance requires stopped MAC, fixture and HARD-reset admission");
  }
  const auto admitted=_admitElectrical(budget,false); if (!admitted.ok()) { return admitted; }
  _beginOperation(OperationKind::COMPLIANCE_START,budget);
  _electrical={}; _electrical.compliance=options; _electrical.complianceSession=true;
  _electricalProfile={}; _electricalProfile.mode=LinkMode::FORCED; _electricalProfile.advertisement.technologyMask=0;
  _electricalProfile.forced={options.mode==ComplianceMode::TX100?LinkSpeed::MBPS_100:LinkSpeed::MBPS_10,Duplex::FULL};
  _electricalProfile.crossover.pairSelection=options.pair; _electricalProfile.crossover.robust=true;
  _result.reset=ResetKind::HARD; _result.commandRequired=true;
  _setElectricalStep(ElectricalStep::PREFLIGHT); token=_operation.token; return _operation.status;
}
Status DP8382x::stopCompliance(const OperationBudget& budget,OperationToken& token) noexcept {
  const auto admitted=_admitElectrical(budget,true); if (!admitted.ok()) { return admitted; }
  if (!_electrical.complianceSession || !_operation.temporaryMayBeActive) { return electricalConfig("No retained compliance stimulus to stop"); }
  _beginOperation(OperationKind::COMPLIANCE_STOP,budget); _electrical.controlsVerified=false;
  _result.reset=ResetKind::HARD; _result.commandRequired=true;
  _setInitStep(InitStep::IDENTIFY); token=_operation.token; return _operation.status;
}
Status DP8382x::startCollisionTest(const CollisionOptions& options,const OperationBudget& budget,OperationToken& token) noexcept {
  if (options.speed!=LinkSpeed::MBPS_10 && options.speed!=LinkSpeed::MBPS_100) {
    return Status::Error(Err::INVALID_PARAM,"Collision requires exact speed",static_cast<int32_t>(ParamDetail::ENUM_VALUE));
  }
  if (!options.maintenanceAdmitted || !options.macStopped || !options.colAvailable || !options.txEnControlled || _config.interface!=MacInterface::MII) {
    return electricalConfig("Collision requires MII COL/TX_EN fixture and stopped MAC");
  }
  const auto admitted=_admitElectrical(budget,false); if (!admitted.ok()) { return admitted; }
  _beginOperation(OperationKind::COLLISION_START,budget);
  _electrical={}; _electrical.collision=options; _electrical.collisionSession=true;
  _setElectricalStep(ElectricalStep::PREFLIGHT); token=_operation.token; return _operation.status;
}
Status DP8382x::stopCollisionTest(const OperationBudget& budget,OperationToken& token) noexcept {
  const auto admitted=_admitElectrical(budget,true); if (!admitted.ok()) { return admitted; }
  if (!_electrical.collisionSession || !_operation.temporaryMayBeActive) { return electricalConfig("No retained collision stimulus to stop"); }
  _beginOperation(OperationKind::COLLISION_STOP,budget); _electrical.controlsVerified=false;
  _setElectricalStep(ElectricalStep::COLLISION_WRITE); token=_operation.token; return _operation.status;
}
Status DP8382x::startCalibrationRecovery(const CalibrationOptions& options,const OperationBudget& budget,OperationToken& token) noexcept {
  if (!options.maintenanceAdmitted || !options.macStopped || !options.hardResetAdmitted) {
    return electricalConfig("Calibration contingency requires stopped MAC and HARD-reset admission");
  }
  const auto admitted=_admitElectrical(budget,false); if (!admitted.ok()) { return admitted; }
  _beginOperation(OperationKind::CALIBRATION_RECOVERY,budget); _electrical={};
  _setElectricalStep(ElectricalStep::CAL_READ); token=_operation.token; return _operation.status;
}
ElectricalSnapshot DP8382x::electricalSnapshot() const noexcept {
  auto snapshot=_electrical; snapshot.temporaryMayBeActive=_operation.temporaryMayBeActive; return snapshot;
}
void DP8382x::_setElectricalStep(ElectricalStep step) noexcept {
  _electricalWorking=true; _electricalStep=step; _configuring=false;
  _operation.nextPollValid=false;
  _operation.phase=step==ElectricalStep::PREFLIGHT?OperationPhase::PREFLIGHT:
      step==ElectricalStep::CAL_PAIR || step==ElectricalStep::COLLISION_WRITE || step==ElectricalStep::STIMULUS_WRITE?
      OperationPhase::COMMAND:OperationPhase::VERIFY;
}
void DP8382x::_beginComplianceActivation() noexcept {
  const auto mode=_electrical.compliance.mode;
  if (mode==ComplianceMode::PRBS10 || mode==ComplianceMode::PATTERN10) { _setElectricalStep(ElectricalStep::STIMULUS_WRITE); }
  else {
    _electricalRestart=true; _result.reset=ResetKind::RESTART; _result.commandRequired=true;
    _setInitStep(InitStep::COMMAND);
  }
}
MdioBatch DP8382x::_electricalBatch() const noexcept {
  MdioBatch batch;
  switch (_electricalStep) {
    case ElectricalStep::PREFLIGHT:
      if (_operation.kind==OperationKind::COLLISION_START) {
        batch=_mmdBatch(0x0305,FrameKind::READ,0);
        constexpr uint8_t addresses[]={0,0x17,0x10,1};
        for (const auto address:addresses) { batch.frames[batch.frameCount++]={FrameKind::READ,address,0,0,NO_READ_INDEX}; }
      } else {
        batch=electricalRead(0); batch.frames[batch.frameCount++]={FrameKind::READ,0x11,0,0,NO_READ_INDEX};
      }
      return batch;
    case ElectricalStep::COLLISION_WRITE:
      batch=electricalRead(0); batch.frameCount=2;
      // BMCR command neutral 8000/0200; do not enable power-down or isolation.
      batch.frames[1]={FrameKind::WRITE,0,static_cast<uint16_t>(_operation.kind==OperationKind::COLLISION_START?0x0080:0),0x717F,0};
      return batch;
    case ElectricalStep::COLLISION_VERIFY: return electricalRead(0);
    case ElectricalStep::STIMULUS_WRITE:
      if (_electrical.compliance.mode==ComplianceMode::PRBS10) {
        batch=electricalRead(0x16); batch.frameCount=2;
        // SNLA266B table 5-4 adapted to table 8-40: bit 8 is observed only.
        batch.frames[1]={FrameKind::WRITE,0x16,0x7001,0x8FE0,0};
      } else {
        batch=_mmdBatch(0x0027,FrameKind::READ,0);
        batch.frames[batch.frameCount++]={FrameKind::WRITE,0x0E,static_cast<uint16_t>(0x0010U|_electrical.compliance.pattern),0xFFE0,3};
      }
      return batch;
    case ElectricalStep::STIMULUS_VERIFY:
      if (_electrical.compliance.mode==ComplianceMode::PATTERN10) { return _mmdBatch(0x0027,FrameKind::READ,0); }
      batch=electricalRead(0);
      if (_electrical.compliance.mode==ComplianceMode::PRBS10) { batch.frames[batch.frameCount++]={FrameKind::READ,0x16,0,0,NO_READ_INDEX}; }
      return batch;
    case ElectricalStep::CAL_READ: return electricalRead(0x1E);
    case ElectricalStep::CAL_PAIR:
      batch.frameCount=2;
      batch.frames[0]={FrameKind::WRITE,0x1E,static_cast<uint16_t>(_electrical.rawCdcr&0x3FFFU),0,NO_READ_INDEX};
      batch.frames[1]={FrameKind::WRITE,0x1F,0x8000,0,NO_READ_INDEX}; return batch;
    case ElectricalStep::COMPLETE: return batch;
  }
  return batch;
}
void DP8382x::_retainElectricalEffects(const MdioBatch& batch,bool called) noexcept {
  if (!called || (_operation.kind!=OperationKind::COMPLIANCE_START && _operation.kind!=OperationKind::COMPLIANCE_STOP &&
      _operation.kind!=OperationKind::COLLISION_START && _operation.kind!=OperationKind::COLLISION_STOP &&
      _operation.kind!=OperationKind::CALIBRATION_RECOVERY)) { return; }
  const bool uncertain=_transport.violation!=TransportViolation::NONE;
  uint16_t selector=0; bool addressMode=false;
  for (uint8_t i=0;i<batch.frameCount;++i) {
    const auto& q=batch.frames[i]; const auto& r=_transport.lastBatch.frames[i];
    if (q.kind!=FrameKind::WRITE) { continue; }
    if (q.registerAddress==0x0D) { selector=q.value; addressMode=(selector&0x4000U)==0U; continue; }
    if (q.registerAddress==0x0E && addressMode) { continue; }
    const bool attempted=r.attempted || r.completed || r.effectPossible || r.valueValid || uncertain;
    _result.mutationAttempted|=r.attempted;
    _result.mutationCompleted|=_result.failingBatch.completedFrames>i;
    _result.mutationPossible|=r.effectPossible || _result.failingBatch.completedFrames>i || uncertain;
    if (_operation.kind==OperationKind::CALIBRATION_RECOVERY && _electricalStep==ElectricalStep::CAL_PAIR && i==0U &&
        (r.completed || r.effectPossible || r.valueValid || uncertain)) {
      _electrical.hardResetOwed=true; _operation.cleanupOwed=true;
    }
    // OP-9 distinguishes pending reset effects from a temporary stimulus.
    // Only a non-reset companion/profile/stimulus write activates the session.
    if (attempted && q.registerAddress!=0x1FU && (_electrical.complianceSession || _electrical.collisionSession)) {
      _operation.temporaryMayBeActive=true; _operation.cleanupOwed=true; _cleanupOwner=_operation.kind;
    }
  }
}
Status DP8382x::_finishElectricalReset() noexcept {
  if (_operation.kind==OperationKind::CALIBRATION_RECOVERY) { _cableCalibration=false; }
  _electrical.resetComplete=true; _electrical.stopped=true; _electrical.controlsVerified=false;
  _electrical.complianceSession=_electrical.collisionSession=false;
  _operation.temporaryMayBeActive=false;
  // HARD stopped the stimulus, but destroyed persistent state. Only a later
  // independently admitted initialization may discharge restoration debt.
  _operation.cleanupOwed=true; _initialized=false;
  return _finishOperation(Status::Ok());
}
Status DP8382x::_advanceElectrical(Status status,bool called,const MdioBatch& batch) noexcept {
  (void)batch;
  const auto& receipt=_transport.lastBatch;
  if (called && _electricalStep==ElectricalStep::CAL_READ && _result.failingBatch.completedFrames>0U) {
    _electrical.rawCdcr=receipt.frames[0].value; _electrical.cdcrValid=true;
    _electrical.recoveryNeeded=(_electrical.rawCdcr&0x4000U)!=0U;
    _cableCalibration=_electrical.recoveryNeeded;
  }
  if (!status.ok()) { return _finishOperation(status); }
  // The shared collector invalidates reset evidence before this phase runs.
  // Do not let the RMW's neutral write hide that observation on later readback;
  // its accepted/possible effect still owns temporary-state restoration debt.
  if (_electricalStep==ElectricalStep::COLLISION_WRITE && called &&
      _result.failingBatch.completedFrames>0U && (receipt.frames[0].value&0x8000U)!=0U) {
    return _finishOperation(Status::Error(Err::INVALID_CONFIG,"Reset invalidated collision control",
        static_cast<int32_t>(ConfigDetail::DEVICE_INVALIDATED)));
  }
  if (_result.mismatch.valid) { return _finishOperation(Status::Error(Err::CONFIG_MISMATCH,"Owned mode changed during electrical work")); }
  switch (_electricalStep) {
    case ElectricalStep::PREFLIGHT:
      if (_operation.kind==OperationKind::COLLISION_START) {
        const auto mux=receipt.frames[3].value&7U;
        const auto bmcr=receipt.frames[4].value,rcsr=receipt.frames[5].value;
        const auto physts=receipt.frames[6].value,bmsr=receipt.frames[7].value;
        const bool an=(bmcr&0x1000U)!=0U;
        const auto speed=an?((physts&2U)!=0U?LinkSpeed::MBPS_10:LinkSpeed::MBPS_100):
            ((bmcr&0x2000U)!=0U?LinkSpeed::MBPS_100:LinkSpeed::MBPS_10);
        if ((mux!=3U && mux!=5U && mux!=6U) || (rcsr&0x0020U)!=0U || (bmcr&0xCC80U)!=0U ||
            (an && ((bmsr&0x0024U)!=0x0024U || (physts&1U)==0U)) || speed!=_electrical.collision.speed) {
          return _finishOperation(electricalConfig("Collision requires live MII COL mux and verified local speed"));
        }
        _setElectricalStep(ElectricalStep::COLLISION_WRITE);
      } else {
        if ((receipt.frames[0].value&0x0C00U)!=0U || (receipt.frames[1].value&0x8000U)!=0U) {
          return _finishOperation(electricalConfig("Release observed power-down/isolation before compliance"));
        }
        _setInitStep(InitStep::IDENTIFY);
      }
      break;
    case ElectricalStep::COLLISION_WRITE: _setElectricalStep(ElectricalStep::COLLISION_VERIFY); break;
    case ElectricalStep::COLLISION_VERIFY: {
      const uint16_t expected=_operation.kind==OperationKind::COLLISION_START?0x0080:0;
      if ((receipt.frames[0].value&0x8E80U)!=expected) {
        return _finishOperation(Status::Error(Err::CONFIG_MISMATCH,"Collision command readback mismatch"));
      }
      _electrical.controlsVerified=true;
      if (_operation.kind==OperationKind::COLLISION_STOP) {
        _electrical.collisionSession=false; _electrical.stopped=true; _operation.temporaryMayBeActive=false;
        _result.persistentRestored=_otherControlsVerified(OperationKind::NONE);
      }
      return _finishOperation(Status::Ok());
    }
    case ElectricalStep::STIMULUS_WRITE:
      if (_electrical.compliance.mode==ComplianceMode::PATTERN10) { _setElectricalStep(ElectricalStep::STIMULUS_VERIFY); }
      else {
        _electricalRestart=true; _result.reset=ResetKind::RESTART; _result.commandRequired=true;
        _setInitStep(InitStep::COMMAND);
      }
      break;
    case ElectricalStep::STIMULUS_VERIFY: {
      bool matches=false;
      if (_electrical.compliance.mode==ComplianceMode::PATTERN10) {
        matches=(receipt.frames[3].value&0x001FU)==(0x0010U|_electrical.compliance.pattern);
      } else {
        const uint16_t bmcr=_electrical.compliance.mode==ComplianceMode::TX100?0x2100:0x0100;
        matches=(receipt.frames[0].value&0xFF80U)==bmcr;
        if (_electrical.compliance.mode==ComplianceMode::PRBS10) { matches&=(receipt.frames[1].value&0x701FU)==0x7001U; }
      }
      if (!matches) { return _finishOperation(Status::Error(Err::CONFIG_MISMATCH,"Compliance stimulus readback mismatch")); }
      _electrical.controlsVerified=true; return _finishOperation(Status::Ok());
    }
    case ElectricalStep::CAL_READ:
      if (!_electrical.recoveryNeeded) { return _finishOperation(Status::Ok()); }
      _result.reset=ResetKind::HARD; _result.commandRequired=true;
      _setElectricalStep(ElectricalStep::CAL_PAIR); break;
    case ElectricalStep::CAL_PAIR: _result.commandAccepted=true; _setInitStep(InitStep::WAIT_COMMAND); return _waitOperation();
    case ElectricalStep::COMPLETE: return _finishOperation(Status::Ok());
  }
  return _operation.status;
}
} // namespace DP8382x
