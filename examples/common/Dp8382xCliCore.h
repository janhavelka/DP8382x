/// @file Dp8382xCliCore.h
/// @brief Owner routes for an application with a portable core binding.
#pragma once

#include "Dp8382xCli.h"

namespace DP8382x::Cli {

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
  using Maintenance = Status (*)(void* context);

  /// @brief Attach to an existing serialized binding without calling it.
  /// @param[in] driver Borrowed portable driver.
  /// @param[in] maintenance Optional application grant for disruptive work.
  /// @param[in] context Borrowed argument for the grant callback.
  explicit CoreOwner(DP8382x& driver, Maintenance maintenance = nullptr,
                     void* context = nullptr) noexcept
      : _driver(driver), _maintenance(maintenance), _context(context) {}

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
      default: return Status::Error(Err::INVALID_PARAM, "Invalid snapshot selector");
    }
    return Status::Ok();
  }

  /// @brief Route a command to one public core call under existing ownership.
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
      case Action::POLL:
        return _driver.pollOperation(request.token, request.budget.io.maxFrames);
      case Action::CANCEL:
        return _driver.cancel(request.token);
      case Action::TAKE:
        return takeResult(request.token, receipt);
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
    if (status.ok()) {
      receipt.result = result.status;
      receipt.mutationCompleted = result.mutationCompleted;
      receipt.mutationPossible = result.mutationPossible;
      receipt.cleanupOwed = result.cleanupOwed;
      receipt.temporary = result.temporaryMayBeActive;
      receipt.negotiationComplete = result.negotiationComplete;
      receipt.nextPages = result.nextPage;
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
};
} // namespace DP8382x::Cli
