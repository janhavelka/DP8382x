/// @file DP8382x.h
/// @brief Portable bounded Clause 22 access and passive transport evidence.
#pragma once

#include <DP8382x/Config.h>

/// @brief Portable PHY driver and copied transport observations.
namespace DP8382x {

/// @brief First detected inconsistency in the last backend report.
enum class TransportViolation : uint8_t {
  NONE,             ///< Report meets the terminal contract.
  STATUS_CODE,      ///< Illegal primary status or detail.
  NULL_MESSAGE,     ///< Primary message is null.
  FRAME_REPORT,     ///< Frame flags, values, count or timestamp contradict the batch.
  RELEASE_REPORT,   ///< Illegal release status or inconsistent reuse report.
  CONTROLLER_STATE  ///< Release succeeded but controller idle is not established.
};

/// @brief Copied all-source diagnostics, separate from future managed health.
struct TransportSnapshot {
  uint32_t totalBatches{0};    ///< Callback invocations, saturating at UINT32_MAX.
  uint32_t attemptedFrames{0};///< Independently valid prefix attempts, saturating.
  uint32_t completedFrames{0};///< Independently valid prefix completions, saturating.
  bool lastBatchValid{false}; ///< Whether a callback receipt has been retained.
  BatchResult lastBatch{};    ///< Exact report, including invalid codes and possible effects.
  Status lastStatus{Err::OK, 0, "No transport attempt"}; ///< Normalized batch/time/release outcome.
  TransportViolation violation{TransportViolation::NONE}; ///< Last report defect.
  bool late{false};           ///< Raw post-call time reached deadline; invalid if clockInvalid.
  bool clockInvalid{false};   ///< Latched timeline fault; repair requires end/rebind.
  bool controllerReusable{false}; ///< Last validated report; never authorizes shared access.
};

/// @brief Noncopyable, application-serialized DP83826I direct-access binding.
/// @pre Serialize every call, including cached access. The borrowed context and
/// callbacks outlive calls. No callback may retain buffers or re-enter this object.
/// @note No allocation, SDK, retry, identity acceptance or hardware initialization.
class DP8382x {
public:
  /// @brief Construct an unbound object without callbacks.
  DP8382x() noexcept = default;
  /// @brief Destroy locally without callbacks or implicit controller cleanup.
  /// @pre All callers are quiescent; copy any needed receipt before destruction.
  ~DP8382x() noexcept = default;
  /// @brief Copying would create another owner and is forbidden.
  DP8382x(const DP8382x&) = delete;
  /// @brief Copy assignment is forbidden, even while unbound.
  DP8382x& operator=(const DP8382x&) = delete;
  /// @brief Moving would invalidate callback ownership and is forbidden.
  DP8382x(DP8382x&&) = delete;
  /// @brief Move assignment is forbidden, even while unbound.
  DP8382x& operator=(DP8382x&&) = delete;

  /// @brief Validate/copy a quiescent binding without clock or MDIO callbacks.
  /// @param[in] config Exact part, board interface, address and borrowed transport.
  /// @return OK, INVALID_CONFIG, UNSUPPORTED for RGMII, or BUSY during a call.
  /// @note Failure preserves the old binding/evidence. Success clears local
  /// receipts, never the application's shared controller quarantine.
  /// Repair a clock/report-contract fault and stop affected controller users
  /// before using a fresh binding to resume I/O.
  Status bind(const Config& config) noexcept;
  /// @brief Detach locally and clear receipts; no release/reset/power callback.
  /// @pre Owner has copied required evidence and retains responsibility for
  /// controller faults; repair is required before a new binding resumes I/O.
  /// @return OK (also unbound), or BUSY when a synchronous call is live.
  Status end() noexcept;
  /// @brief Inspect local binding without I/O or clock access.
  /// @return True for a valid declaration, not proof of identity or bus readiness.
  bool isBound() const noexcept { return _bound; }

  /// @brief Read one nonconsuming Clause 22 word using one terminal owned batch.
  /// @param[in] registerAddress Five-bit address, validated before narrowing.
  /// @param[out] value Assigned only on complete valid in-budget success.
  /// @param[in] budget Original exclusive microsecond deadline and frame allowance.
  /// @return INVALID_PARAM for width/access, NOT_BOUND, BUSY, clock/configuration
  /// error, transport failure or OPERATION_TIMEOUT; never IN_PROGRESS.
  /// @note SNLS647I tables 8-19..48. Selectors, holes, consuming reads and CDCR
  /// (resistor-calibration follow-up obligation) are rejected before callbacks.
  /// Status words may change asynchronously; a raw word is not a coherent link result.
  Status readClause22(uint32_t registerAddress, uint16_t& value,
                      const IoBudget& budget) noexcept;
  /// @brief Write the safe maintenance subset in one terminal owned batch.
  /// @param[in] registerAddress Only LEDCR (0x18) is currently admitted.
  /// @param[in] value Sixteen-bit word; only mask 0x0692 may be nonzero.
  /// @param[in] budget Original exclusive microsecond deadline and frame allowance.
  /// @pre Application establishes exact part/address and safe LED0 electrical use,
  /// and serializes maintenance. Binding alone does not verify those facts.
  /// @return Same terminal errors as readClause22; OK is frame completion, not
  /// verified pin state. A failed/late write may have applied; never auto-retried.
  /// @note SNLS647I table 8-42: blink, polarity, drive enable/value; reserved zero.
  /// No cached setting is claimed. Copy transportSnapshot before another batch.
  Status writeClause22(uint32_t registerAddress, uint32_t value,
                       const IoBudget& budget) noexcept;
  /// @brief Copy retained raw/normalized evidence without clock or MDIO callbacks.
  /// @return Passive receipt; raw invalid timestamps/statuses must not be trusted.
  TransportSnapshot transportSnapshot() const noexcept { return _transport; }

private:
  Config _config{};
  TransportSnapshot _transport{};
  bool _bound{false};
  bool _active{false};
  bool _stopped{false};
  bool _timeValid{false};
  uint64_t _lastTimeUs{0};
  Status _sampleTime(uint64_t& raw, uint64_t& upper) noexcept;
  Status _transfer(FrameKind kind, uint8_t address, uint16_t writeValue,
                   uint16_t& readValue, const IoBudget& budget) noexcept;
};

} // namespace DP8382x
