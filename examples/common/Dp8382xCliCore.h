/// @file Dp8382xCliCore.h
/// @brief Owner routes for an application with a portable core binding.
#pragma once

#include "Dp8382xCli.h"

namespace DP8382x::Cli {

/// @brief Shared internal helpers for the example owners.
namespace owner_detail {
/// @brief Check whether a new request can access an idle operation slot.
/// @param[in] operation Cached progress from the current owner.
/// @return OK when idle, or BUSY with the active or result-pending reason.
inline Status idleSlot(const OperationSnapshot& operation) noexcept {
  if (operation.state == OperationState::RUNNING) {
    return Status::Error(Err::BUSY, "Operation active", static_cast<int32_t>(BusyDetail::OPERATION_ACTIVE));
  }
  if (operation.state == OperationState::RESULT_PENDING) {
    return Status::Error(Err::BUSY, "Operation result pending", static_cast<int32_t>(BusyDetail::RESULT_PENDING));
  }
  return Status::Ok();
}

/// @brief Project a successfully delivered owner result into the CLI receipt.
/// @param[in] result The immutable terminal value returned by takeResult.
/// @param[out] receipt Result projection; existing fields for other calls are untouched.
inline void copyOperationResult(const OperationResult& result, Receipt& receipt) noexcept {
  receipt.result = result.status;
  receipt.mutationCompleted = result.mutationCompleted;
  receipt.mutationPossible = result.mutationPossible;
  receipt.cleanupOwed = result.cleanupOwed;
  receipt.temporary = result.temporaryMayBeActive;
  receipt.negotiationComplete = result.negotiationComplete;
  receipt.nextPages = result.nextPage;
  receipt.reset = result.reset;
  receipt.commandRequired = result.commandRequired;
  receipt.commandAccepted = result.commandAccepted;
  receipt.protocolComplete = result.protocolComplete;
  receipt.persistentRestored = result.persistentRestored;
  receipt.clockInvalid = result.clockInvalid;
  receipt.straps = result.straps;
}

/// @brief Copy this collector call's valid prefix from the audited transport receipt.
/// @param[in,out] receipt Six-frame collector report and unpopulated event fields.
/// @note The first incomplete frame ends the prefix. Cached values are never consulted.
inline void copyEventWords(Receipt& receipt) noexcept {
  if (!receipt.batchValid || receipt.violation != TransportViolation::NONE) return;
  for (std::size_t i = 0; i < receipt.eventWords.size(); ++i) {
    const auto& frame = receipt.batch.frames[i];
    if (!frame.attempted || !frame.completed || !frame.valueValid) break;
    receipt.eventWords[i] = frame.value;
    receipt.eventValidMask |= static_cast<uint8_t>(1U << i);
  }
}

/// @brief Copy the six event words retained by the collector, including historical samples.
/// @param[in] evidence Cached samples with individual validity.
/// @param[out] output The event word array and mask. Invalid words are zeroed.
inline void copyRetainedEventWords(const EvidenceSnapshot& evidence, CachedSnapshot& output) noexcept {
  constexpr std::array<EvidenceSource, 6> sources{EvidenceSource::PHYSTS, EvidenceSource::FLDS,
      EvidenceSource::MISR1, EvidenceSource::MISR2, EvidenceSource::FCSCR, EvidenceSource::RECR};
  output.eventValidMask = 0;
  for (std::size_t i = 0; i < sources.size(); ++i) {
    const auto& sample = evidence.samples[static_cast<std::size_t>(sources[i])];
    output.eventWords[i] = sample.valid ? sample.raw : 0;
    if (sample.valid) output.eventValidMask |= static_cast<uint8_t>(1U << i);
  }
}
} // namespace owner_detail

/// @brief Borrow one already bound portable driver and its maintenance policy.
/// @pre The application serializes these calls with all other access to the
/// driver, including other users of its shared controller. The driver and
/// policy context outlive this object. No callback may reenter either object.
class CoreOwner {
public:
  /// @brief Check existing maintenance permission without changing owner state.
  /// @param[in] context Borrowed application policy context.
  /// @return Terminal OK for permission, or an error denying the request.
  /// @note No PHY I/O or waits. For an admitted operation the owner guarantees
  /// maintenance through the terminal result. A null policy denies mutation.
  /// Initialization can select a hard reset for owned EEE intent. For shared
  /// buses the application coordinates every participant and preserves quiet
  /// time or quarantine evidenced by the operation, even after END or ABANDON.
  using Maintenance = Status (*)(void* context);
  /// @brief Attest that the owner already established the required external disposition.
  /// @param[in] context Borrowed application policy context.
  /// @return True only after isolation, external reset, power removal or ownership transfer.
  /// @note No PHY I/O, clock calls or waiting. A null callback denies ABANDON.
  using ExternalDisposition = bool (*)(void* context);

  /// @brief Attach to an existing serialized binding without calling it.
  /// @param[in] driver Borrowed portable driver.
  /// @param[in] maintenance Optional application grant for disruptive work.
  /// @param[in] context Borrowed argument for the grant callback.
  /// @param[in] externalDisposition Optional independent application approval for ABANDON.
  explicit CoreOwner(DP8382x& driver, Maintenance maintenance = nullptr,
                     void* context = nullptr, ExternalDisposition externalDisposition = nullptr) noexcept
      : _driver(driver), _maintenance(maintenance), _context(context),
        _externalDisposition(externalDisposition) {}

  /// @brief Copy one cached value without bus or clock access.
  /// @param[in] kind Selects the one member to populate.
  /// @param[out] output That member is assigned on OK. Others stay unchanged.
  /// @return OK for a supported snapshot selector, or INVALID_PARAM.
  Status readCached(SnapshotKind kind, CachedSnapshot& output) const noexcept {
    switch (kind) {
      case SnapshotKind::CAPABILITIES: output.capabilities = _driver.capabilities(); break;
      case SnapshotKind::HEALTH: output.health = _driver.healthSnapshot(); break;
      case SnapshotKind::LINK: output.link = _driver.linkSnapshot(); break;
      case SnapshotKind::SETTINGS: output.settings = _driver.settingsSnapshot(); break;
      case SnapshotKind::OPERATION: output.operation = _driver.operationSnapshot(); break;
      case SnapshotKind::TRANSPORT: output.transport = _driver.transportSnapshot(); break;
      case SnapshotKind::IDENTITY: output.identity = _driver.identitySnapshot(); break;
      case SnapshotKind::EVENTS:
        output.counters = _driver.counterSnapshot();
        owner_detail::copyRetainedEventWords(_driver.evidenceSnapshot(), output);
        break;
      default: return Status::Error(Err::INVALID_PARAM, "Invalid snapshot selector");
    }
    return Status::Ok();
  }

  /// @brief Route a command to the public core under existing ownership.
  /// @param[in] request Typed input and original aggregate budget.
  /// @param[out] receipt Reset on entry, with only this call's evidence on return.
  /// @return The core status or application admission failure. Only START and
  /// POLL can return IN_PROGRESS. A successful TAKE delivers the prior result.
  /// @note Raw, probe and discovery receipts use the core's explicit callback
  /// invocation evidence under owner serialization.
  Status execute(const Request& request, Receipt& receipt) {
    receipt = {};
    if (requiresMaintenance(request.action)) {
      if (!_maintenance) {
        return Status::Error(Err::BUSY, "Maintenance not granted",
                             static_cast<int32_t>(BusyDetail::OWNER_ADMISSION));
      }
      const Status admitted = _maintenance(_context);
      if (admitted.inProgress()) {
        return Status::Error(Err::INVALID_CONFIG, "Nonterminal maintenance grant", admitted.detail);
      }
      if (!admitted.ok()) return admitted;
    }

    switch (request.action) {
      case Action::START_CONFIGURE:
        return _driver.startConfigure(request.settings, request.options, request.budget, receipt.token);
      case Action::START_VERIFY:
        return _driver.startVerify(request.budget, receipt.token);
      case Action::START_INITIALIZE:
        return _driver.startInitialize({request.reset}, request.budget, receipt.token);
      case Action::POLL:
        return _driver.pollOperation(request.token, request.budget.io.maxFrames);
      case Action::CANCEL:
        return _driver.cancel(request.token);
      case Action::TAKE:
        return takeResult(request.token, receipt);
      case Action::OBSERVE_LINK:
      case Action::COLLECT_EVENTS:
      case Action::ABANDON:
        return idleAction(request, receipt);
      case Action::END:
        return _driver.end();
      case Action::INVALIDATE:
        return _driver.invalidateDeviceState(InvalidationReason::EXTERNAL_MUTATION);
      case Action::RECONCILE:
        return Status::Ok(); // The portable owner has no SDK cache.
      case Action::DISCOVER:
        return discover(request, receipt);
      default:
        return rawAccess(request, receipt);
    }
  }

private:
  static bool requiresMaintenance(Action action) noexcept {
    switch (action) {
      case Action::START_CONFIGURE:
      case Action::START_INITIALIZE:
      case Action::ABANDON:
      case Action::END:
      case Action::WRITE_C22:
      case Action::WRITE_MMD:
      case Action::MODIFY_MMD:
      case Action::MODIFY_BMCR:
      case Action::INVALIDATE:
        return true;
      default:
        return false;
    }
  }

  Status takeResult(OperationToken token, Receipt& receipt) noexcept {
    OperationResult result{};
    const Status status = _driver.takeResult(token, result);
    if (status.ok()) owner_detail::copyOperationResult(result, receipt);
    return status;
  }

  Status idleAction(const Request& request, Receipt& receipt) {
    const auto available = owner_detail::idleSlot(_driver.operationSnapshot());
    if (!available.ok()) return available;
    if (request.action == Action::OBSERVE_LINK) {
      return _driver.readLinkStatus(receipt.link, request.budget.io);
    }
    if (request.action == Action::ABANDON) {
      if (!_externalDisposition || !_externalDisposition(_context)) {
        return Status::Error(Err::BUSY, "External disposition not established",
                             static_cast<int32_t>(BusyDetail::OWNER_ADMISSION));
      }
      return _driver.abandonCleanup();
    }
    const auto status = _driver.collectEvents(receipt.counters, request.budget.io, &receipt.batchValid);
    if (receipt.batchValid) {
      const auto transport = _driver.transportSnapshot();
      receipt.batch = transport.lastBatch;
      receipt.violation = transport.violation;
      receipt.releaseStatus = transport.lastReleaseStatus;
      owner_detail::copyEventWords(receipt);
    }
    return status;
  }

  Status discover(const Request& request, Receipt& receipt) noexcept {
    uint16_t id1{}, id2{};
    const Status status = _driver.readIdentityAt(request.address, id1, id2,
                                                 request.budget.io, &receipt.batchValid);
    if (receipt.batchValid) {
      const auto transport = _driver.transportSnapshot();
      receipt.batch = transport.lastBatch;
      receipt.violation = transport.violation;
      if (receipt.violation != TransportViolation::NONE) return status;
      const auto& first = receipt.batch.frames[0];
      const auto& second = receipt.batch.frames[1];
      auto& identity = receipt.identity;
      identity.id1Valid = first.attempted && first.completed && first.valueValid;
      identity.id2Valid = identity.id1Valid && second.attempted && second.completed && second.valueValid;
      if (identity.id1Valid) identity.id1 = first.value;
      if (identity.id2Valid) identity.id2 = second.value;
    }
    return status;
  }

  Status rawAccess(const Request& request, Receipt& receipt) noexcept {
    const auto& budget = request.budget.io;
    Status status{};
    switch (request.action) {
      case Action::PROBE:
        status = _driver.readIdentity(receipt.identity, budget, &receipt.batchValid);
        break;
      case Action::READ_C22:
        status = _driver.readClause22(request.address, receipt.value, budget, &receipt.batchValid);
        break;
      case Action::READ_MMD:
        status = _driver.readMmd(request.devad, request.address, receipt.value, budget, &receipt.batchValid);
        break;
      case Action::WRITE_C22:
        status = _driver.writeClause22(request.address, request.value, budget, &receipt.batchValid);
        break;
      case Action::WRITE_MMD:
        status = _driver.writeMmd(request.devad, request.address, request.value, budget, &receipt.batchValid);
        break;
      case Action::MODIFY_MMD:
        status = _driver.modifyMmd(request.devad, request.address, request.mask, request.value,
                                  budget, &receipt.batchValid);
        break;
      case Action::MODIFY_BMCR:
        status = _driver.modifyBasicControl(request.mask, request.value, budget, &receipt.batchValid);
        break;
      default: return Status::Error(Err::INVALID_PARAM, "Invalid owner action");
    }
    if (receipt.batchValid) {
      const auto transport = _driver.transportSnapshot();
      receipt.batch = transport.lastBatch;
      receipt.violation = transport.violation;
      if (request.action == Action::PROBE && !status.ok() &&
          receipt.violation == TransportViolation::NONE && receipt.batch.frames[0].attempted) {
        receipt.identity = _driver.identitySnapshot();
      }
      receipt.releaseStatus = transport.lastReleaseStatus;
    }
    return status;
  }

  DP8382x& _driver;
  Maintenance _maintenance;
  void* _context;
  ExternalDisposition _externalDisposition;
};
} // namespace DP8382x::Cli
