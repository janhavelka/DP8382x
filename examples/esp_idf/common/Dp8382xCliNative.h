/// @file Dp8382xCliNative.h
/// @brief Application bridge from the portable CLI to the installed ESP-IDF PHY owner.
#pragma once

#include "../../common/Dp8382xCliCore.h"
#include <DP8382x/esp_idf/Phy.h>

namespace DP8382x::Cli {

/// @brief Borrow a single native owner and the application's console and maintenance policy.
/// @pre Keep the bridge, PHY and callback context alive through CLI service.
/// @note The application task serializes this bridge with its other owner calls.
/// The bridge uses esp_timer for the same microsecond clock as the native PHY.
struct NativeOwner {
  EspIdf::Phy* phy{nullptr}; ///< Installed adapter owned by the application.
  void* context{nullptr}; ///< Borrowed console and application policy context.
  ConsoleBuffer::WriteSome write{nullptr}; ///< Nonblocking console sink.
  /// @brief Check already-granted maintenance permission without side effects.
  /// @note Return false during active traffic. Null denies disruptive actions.
  /// The application owns quiescence and lifecycle changes. This check does not
  /// open maintenance, stop the MAC or perform MDIO.
  bool (*maintenanceAllowed)(void*){nullptr};
  /// @brief Independently attest external disposition before abandoning cleanup.
  /// @note Return true only after isolation, external reset, power removal or ownership transfer.
  /// A local check under application serialization. A null callback denies ABANDON.
  bool (*externalDisposition)(void*){nullptr};
  /// @brief Run the application's bounded quiescence and release route for the installed owner.
  /// @note Receives context and an absolute deadline in esp_timer microseconds. Zero means the
  /// CLI could not supply a trusted deadline. Return terminal status and keep storage reachable
  /// by SDK or CLI callbacks alive. Null means the application has no END route.
  Status (*end)(void*, uint64_t deadlineUs){nullptr};
  FeatureAttestation featureAttestation{nullptr}; ///< Independent application proof for typed feature prerequisites.

  /// @brief Build the processor's borrowed platform callbacks without I/O.
  /// @return Platform with a null writer if the bridge has no console sink.
  Platform platform() noexcept {
    return {this, write ? &writeSome : nullptr, &readCached, &nowUs, &execute};
  }

private:
  static std::size_t writeSome(void* opaque, const char* bytes, std::size_t size) {
    auto& bridge = *static_cast<NativeOwner*>(opaque);
    return bridge.write(bridge.context, bytes, size);
  }

  static uint64_t nowUs(void*) { return static_cast<uint64_t>(esp_timer_get_time()); }

  static Status missing() {
    return Status::Error(Err::NOT_BOUND, "The application has no native PHY");
  }

  static Status readCached(void* opaque, SnapshotKind kind, CachedSnapshot& output) {
    auto* phy = static_cast<NativeOwner*>(opaque)->phy;
    if (!phy) return missing();
    switch (kind) {
      case SnapshotKind::CAPABILITIES: return phy->capabilitiesSnapshot(output.capabilities);
      case SnapshotKind::HEALTH: return phy->healthSnapshot(output.health);
      case SnapshotKind::LINK: return phy->linkSnapshot(output.link);
      case SnapshotKind::SETTINGS: return phy->settingsSnapshot(output.settings);
      case SnapshotKind::OPERATION: return phy->operationSnapshot(output.operation);
      case SnapshotKind::TRANSPORT: return phy->transportSnapshot(output.transport);
      case SnapshotKind::IDENTITY: return phy->identitySnapshot(output.identity);
      case SnapshotKind::EVENTS: {
        EvidenceSnapshot evidence{};
        const auto status = phy->counterSnapshot(output.counters, &evidence);
        if (status.ok()) owner_detail::copyRetainedEventWords(evidence, output);
        return status;
      }
    }
    return Status::Error(Err::INVALID_PARAM, "Unknown native snapshot");
  }

  static bool requiresMaintenance(Action action) {
    switch (action) {
      case Action::WRITE_C22: case Action::WRITE_MMD: case Action::MODIFY_MMD:
      case Action::MODIFY_BMCR: case Action::INVALIDATE: case Action::START_CONFIGURE:
      case Action::RECONCILE: case Action::START_INITIALIZE: case Action::ABANDON: case Action::END:
        return true;
      default: return false;
    }
  }

  static Status discover(EspIdf::Phy& phy, const Request& request, Receipt& receipt) {
    if (!isValidPhyAddress(request.address))
      return Status::Error(Err::INVALID_PARAM, "Discovery address exceeds Clause 22 width",
                           static_cast<int32_t>(ParamDetail::ADDRESS_RANGE));
    EspIdf::DiscoverySnapshot output{};
    EspIdf::DiscoverySnapshot fresh{};
    const auto status = phy.discover(uint32_t{1} << request.address, output, request.budget.io, &fresh);
    // A failed local admission must not turn an older same-address scan into
    // fresh evidence. Only the caller-specific fresh destination is examined.
    if (fresh.lastBatchValid) {
      receipt.batch = fresh.lastBatch;
      receipt.batchValid = true;
      receipt.violation = fresh.lastBatchViolation;
      const auto& entry = fresh.entries[request.address];
      receipt.identity.id1 = entry.id1;
      receipt.identity.id2 = entry.id2;
      receipt.identity.id1Valid = entry.id1Valid;
      receipt.identity.id2Valid = entry.id2Valid;
    }
    return status;
  }

  static Status take(EspIdf::Phy& phy, OperationToken token, Feature feature, Receipt& receipt) {
    OperationResult result{};
    const auto taken = phy.takeResult(token, result);
    if (taken.ok()) {
      owner_detail::copyOperationResult(result, receipt);
      if (feature != Feature::NONE) owner_detail::describeFeatureResult(feature, result, receipt.featureFields);
    }
    return taken;
  }

  static Status raw(EspIdf::Phy& phy, const Request& request, Receipt& receipt) {
    EspIdf::RawBatchReceipt evidence{};
    const auto& budget = request.budget.io;
    Status status{};
    switch (request.action) {
      case Action::PROBE: status = phy.readIdentity(receipt.identity, budget, &evidence); break;
      case Action::READ_C22: status = phy.readClause22(request.address, receipt.value, budget, &evidence); break;
      case Action::READ_MMD: status = phy.readMmd(request.devad, request.address, receipt.value, budget, &evidence); break;
      case Action::WRITE_C22: status = phy.writeClause22(request.address, request.value, budget, &evidence); break;
      case Action::WRITE_MMD: status = phy.writeMmd(request.devad, request.address, request.value, budget, &evidence); break;
      case Action::MODIFY_MMD:
        status = phy.modifyMmd(request.devad, request.address, request.mask, request.value, budget, &evidence);
        break;
      case Action::MODIFY_BMCR: status = phy.modifyBasicControl(request.mask, request.value, budget, &evidence); break;
      case Action::COLLECT_EVENTS: status = phy.collectEvents(receipt.counters, budget, &evidence); break;
      default: return Status::Error(Err::INVALID_PARAM, "Unknown raw native action");
    }
    if (evidence.valid) {
      receipt.batch = evidence.batch;
      receipt.batchValid = true;
      receipt.violation = evidence.violation;
      receipt.releaseStatus = evidence.releaseStatus;
      if (request.action == Action::COLLECT_EVENTS) owner_detail::copyEventWords(receipt);
      if (request.action == Action::PROBE && !status.ok() && evidence.violation == TransportViolation::NONE) {
        const auto& first = evidence.batch.frames[0];
        const auto& second = evidence.batch.frames[1];
        // A started probe committed fresh identity evidence, including partial
        // words or a rejected decoded identity. A denied batch may keep old data.
        if (first.attempted && phy.identitySnapshot(receipt.identity).ok()) return status;
        receipt.identity.id1Valid = first.attempted && first.completed && first.valueValid;
        receipt.identity.id2Valid = receipt.identity.id1Valid && second.attempted &&
            second.completed && second.valueValid;
        if (receipt.identity.id1Valid) receipt.identity.id1 = first.value;
        if (receipt.identity.id2Valid) receipt.identity.id2 = second.value;
        if (receipt.identity.id1Valid) receipt.identity.admissionStatus = status;
      }
    }
    return status;
  }

  static Status endOwner(NativeOwner& bridge, uint64_t deadlineUs) {
    if (!bridge.end) return Status::Error(Err::INVALID_CONFIG, "No application release route");
    OperationSnapshot operation{};
    const auto copied = bridge.phy->operationSnapshot(operation);
    if (!copied.ok()) return copied;
    const auto available = owner_detail::idleSlot(operation);
    if (!available.ok()) return available;
    if (operation.cleanupOwed) {
      return Status::Error(Err::BUSY, "Cleanup is still required",
                           static_cast<int32_t>(BusyDetail::CLEANUP_REQUIRED));
    }
    const auto status = bridge.end(bridge.context, deadlineUs);
    if (status.inProgress()) {
      return Status::Error(Err::INVALID_CONFIG, "Nonterminal application release", status.detail);
    }
    return status;
  }

  static Status execute(void* opaque, const Request& request, Receipt& receipt) {
    auto& bridge = *static_cast<NativeOwner*>(opaque);
    receipt = {};
    if (!bridge.phy) return missing();
    const bool maintenance = requiresMaintenance(request.action) ||
        (request.action == Action::FEATURE && owner_detail::featureMaintenance(request.feature));
    if (maintenance &&
        (!bridge.maintenanceAllowed || !bridge.maintenanceAllowed(bridge.context))) {
      return Status::Error(Err::BUSY, "Application maintenance is unavailable",
                           static_cast<int32_t>(BusyDetail::OWNER_ADMISSION));
    }
    auto& phy = *bridge.phy;
    switch (request.action) {
      case Action::DISCOVER: return discover(phy, request, receipt);
      case Action::INVALIDATE: return phy.invalidateDeviceState(InvalidationReason::EXTERNAL_MUTATION);
      case Action::START_CONFIGURE:
        return phy.startConfigure(request.settings, request.options, request.budget, receipt.token);
      case Action::START_VERIFY: return phy.startVerify(request.budget, receipt.token);
      case Action::START_INITIALIZE: return phy.startInitialize({request.reset}, request.budget, receipt.token);
      case Action::POLL: return phy.pollOperation(request.token, request.budget.io.maxFrames);
      case Action::CANCEL: return phy.cancel(request.token);
      case Action::TAKE: return take(phy, request.token, request.feature, receipt);
      case Action::FEATURE: return owner_detail::executeFeature(phy, request, receipt, bridge.featureAttestation, bridge.context);
      case Action::OBSERVE_LINK: return phy.readLinkStatus(receipt.link, request.budget.io);
      case Action::ABANDON:
        if (!bridge.externalDisposition || !bridge.externalDisposition(bridge.context)) {
          return Status::Error(Err::BUSY, "External disposition not established",
                               static_cast<int32_t>(BusyDetail::OWNER_ADMISSION));
        }
        return phy.abandonCleanup();
      case Action::END: return endOwner(bridge, request.budget.io.deadlineUs);
      case Action::RECONCILE: {
        const auto status = phy.reconcileSdkState(request.budget.io.deadlineUs);
        receipt.sdkError = status.sdkError;
        return status.coreStatus;
      }
      default: return raw(phy, request, receipt);
    }
  }
};
} // namespace DP8382x::Cli
