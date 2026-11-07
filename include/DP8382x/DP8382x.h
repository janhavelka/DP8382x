/// @file DP8382x.h
/// @brief Portable bounded direct/indirect access and passive transport evidence.
#pragma once

#include <DP8382x/Config.h>

/// @brief Portable PHY driver and copied transport observations.
namespace DP8382x {

/// @brief Assessment of complete identity words under the exact-part policy.
enum class IdentityAcceptance : uint8_t {
  UNASSESSED, ///< No complete successful acquisition has been assessed.
  ACCEPTED,   ///< Historical assessment accepted; stale still forbids capabilities.
  REJECTED    ///< Complete successful reads failed identity admission.
};

/// @brief Latest identity acquisition; partial words never establish usable identity.
struct DeviceIdentity {
  Variant declaredVariant{Variant::UNKNOWN}; ///< Application declaration, never decoded suffix.
  uint16_t id1{0}; ///< Raw PHYIDR1; meaningful only when id1Valid.
  uint16_t id2{0}; ///< Raw PHYIDR2; meaningful only when id2Valid.
  bool id1Valid{false}; ///< This acquisition returned a trusted first word.
  bool id2Valid{false}; ///< This acquisition returned both words in order.
  uint32_t oui{0}; ///< 22-bit OUI; meaningful only when decodedValid.
  uint8_t model{0}; ///< Six-bit model; meaningful only when decodedValid.
  uint8_t revision{0}; ///< Raw revision nibble; not an acceptance claim.
  OperatingMode mode{OperatingMode::UNKNOWN}; ///< Recognized model's mode, not MAC wiring.
  bool decodedValid{false}; ///< Both raw words decoded, even if admission failed.
  IdentityAcceptance acceptance{IdentityAcceptance::UNASSESSED}; ///< Latest complete assessment.
  Status admissionStatus{Err::NOT_INITIALIZED, 0, "No accepted identity"}; ///< Acquisition/admission outcome.
  uint64_t observedAtUs{0}; ///< Conservative complete-acquisition timestamp, microseconds.
  bool timeValid{false}; ///< Timestamp is usable independently of admission.
  bool stale{true}; ///< Invalidation retains historical words but withdraws capabilities.
};

/// @brief Foundational silicon capabilities of a freshly accepted declared part.
/// @note Invalid means unknown, not absent. These fields do not claim that typed
/// features, the SDK or physical fixtures have been implemented or qualified.
struct Capabilities {
  bool valid{false}; ///< All remaining fields require this flag.
  uint8_t linkModeMask{0}; ///< LINK_* source-supported technology bits.
  uint8_t interfaceMask{0}; ///< INTERFACE_* source-supported MAC interface bits.
  bool copper{false}; ///< Copper medium documented for the accepted part.
  bool fiber{false}; ///< Fiber medium documented for the accepted part.
  bool gigabit{false}; ///< Gigabit speed documented for the accepted part.
};

/// @brief First detected inconsistency in the last backend report.
enum class TransportViolation : uint8_t {
  NONE,             ///< Report meets the terminal contract.
  STATUS_CODE,      ///< Illegal primary status or detail.
  NULL_MESSAGE,     ///< Primary message is null.
  FRAME_REPORT,     ///< Frame flags, values, count or timestamp contradict the batch.
  RELEASE_REPORT,   ///< Illegal release status or inconsistent reuse report.
  CONTROLLER_STATE  ///< Release succeeded but controller idle is not established.
};

/// @brief Transport counts and last backend report, separate from managed health.
struct TransportSnapshot {
  uint32_t totalBatches{0};    ///< Callback invocations, saturating at UINT32_MAX.
  uint32_t attemptedFrames{0};///< Valid requested prefix attempts, saturating.
  uint32_t completedFrames{0};///< Valid requested prefix completions, saturating.
  BatchResult lastBatch{};    ///< Exact report, including invalid codes and possible effects.
  TransportViolation violation{TransportViolation::NONE}; ///< Last report defect.
  bool stopped{false};        ///< Malformed report stopped this binding; owner repair required.
  bool late{false};           ///< Raw post-call time reached deadline; invalid if clockInvalid.
  bool clockInvalid{false};   ///< Latched timeline fault; repair requires end/rebind.
  bool controllerReusable{false}; ///< Last validated report; never authorizes shared access.
};

/// @brief Destructive sources, in acquisition order; PHYSTS precedes mirrors.
enum class EvidenceSource : uint8_t {
  PHYSTS, ///< Raw status before dependent clearing reads; table 8-34.
  BMSR, ///< Latched basic status; table 8-20.
  ANER, ///< Page receive consumes PHYSTS mirror; tables 8-25/34.
  FLDS, ///< Latched fast-link-drop causes; table 8-33.
  MISR1, ///< Interrupt events and stable enables; table 8-36.
  MISR2, ///< Interrupt events and stable enables; table 8-37.
  FCSCR, ///< Eight-bit clear-on-read false-carrier sample; table 8-38.
  RECR, ///< Raw clear-on-read word; width/saturation conflict remains; table 8-39.
  RCSR, ///< Raw RMII RC state; polarity/rearm gate remains; table 8-41.
  BTSCR, ///< 10BTSCR polarity history clears despite R type; table 8-44.
  RXFS, ///< WoL/filter history and routing; table 8-91.
  COUNT ///< Number of supported sources, not a readable source.
};

/// @brief Build an acquisition mask; invalid enum values produce zero.
/// @param[in] source Named destructive source.
/// @return One mask bit or zero, without I/O.
constexpr uint32_t evidenceBit(EvidenceSource source) noexcept {
  return source < EvidenceSource::COUNT ? uint32_t{1} << static_cast<uint8_t>(source) : 0U;
}
/// @brief Every currently supported destructive source; no automatic polling.
constexpr uint32_t ALL_EVIDENCE = (uint32_t{1} << static_cast<uint8_t>(EvidenceSource::COUNT)) - 1U;

/// @brief Retained evidence from one destructive source, separate from aggregate success.
struct EvidenceSample {
  uint16_t raw{0}; ///< Latest successfully returned full word, including unknown bits.
  uint16_t observedOnes{0}; ///< Union of raw ones; no implied physical event decoding.
  uint16_t observedZeros{0}; ///< Union of raw zeros, retaining latched-low observations.
  uint32_t samples{0}; ///< Successful consumed words, saturating; not an event count.
  uint64_t sampledSum{0}; ///< FCSCR low-byte or RECR raw-word arithmetic sum; not a qualified physical total.
  bool sumSaturated{false}; ///< Sticky arithmetic overflow; sampledSum is then a lower bound.
  uint64_t observedAtUs{0}; ///< Conservative completion bound in the binding clock domain.
  bool valid{false}; ///< At least one independently valid word has been retained.
  bool timeValid{false}; ///< Timestamp is usable; zero is a valid timestamp.
  bool stale{true}; ///< Failed refresh or known external/configuration change.
  bool historyLost{false}; ///< Sticky: a possible consumption returned no trusted word.
};

/// @brief Passive destructive evidence; binding/end begin a new local history.
struct EvidenceSnapshot {
  std::array<EvidenceSample, static_cast<uint8_t>(EvidenceSource::COUNT)> samples{}; ///< Indexed by EvidenceSource.
  uint32_t lastRequested{0}; ///< Sources requested by the latest admitted acquisition.
  Status lastStatus{Err::RESULT_NOT_AVAILABLE, 0, "No evidence acquisition"}; ///< Latest acquisition or admitted mixed update result; comparison-only wake exit leaves it unchanged.
  bool configurationDirty{false}; ///< Semantic raw/external mutation; no verification is implied.
};

/// @brief Owner notification of lost device trust; no action is taken on the PHY.
enum class InvalidationReason : uint8_t {
  EXTERNAL_RESET, ///< Owner reports reset; old samples remain historical.
  EXTERNAL_POWER_ON, ///< Owner reports a power cycle.
  EXTERNAL_MUTATION, ///< Another writer may have changed hardware.
  RAW_MUTATION, ///< Owner performed a raw semantic write.
  AMBIGUOUS_MUTATION ///< A write may have applied without valid completion.
};

/// @brief Noncopyable, application-serialized DP83826I direct-access binding.
/// @pre Serialize every call, including cached access. The borrowed context and
/// callbacks outlive calls. No callback may retain buffers or re-enter this object.
/// @note No allocation, SDK, automatic retry or hardware initialization.
/// Maintenance writes return a cached failed/rejected readIdentity status before
/// any callback. Successful reprobe or explicit rebind clears that restriction.
/// Existing owner-attested maintenance before the first probe remains allowed;
/// stale successful identity withdraws capabilities but permits that maintenance.
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

  /// @brief Acquire and accept identity at the bound address in one two-read batch.
  /// @param[out] identity Assigned only after complete in-budget transport and identity acceptance.
  /// @param[in] budget Aggregate microsecond deadline and at least two frames.
  /// @return Transport/admission errors unchanged; DEVICE_NOT_FOUND for either
  /// required word zero/all-ones; DEVICE_MISMATCH for OUI/model/revision/mode,
  /// checked in that order. No error implies physical absence or an MDIO ACK.
  /// @note SNLS647I tables 8-21/22: declared DP83826I, OUI 080028, model 11/13,
  /// revision 1. Shared IDs cannot identify I/E suffix. Partial/rejected evidence
  /// stays in identitySnapshot; never combine words from different calls.
  /// @pre Bound, serialized owner with clock/power/reset readiness established.
  /// Uses the ordinary raw-read guard; no reset, scan, write or retry is performed.
  Status readIdentity(DeviceIdentity& identity, const IoBudget& budget) noexcept;
  /// @brief Copy latest identity evidence without MDIO or clock access.
  /// @return Historical/partial fields with explicit validity, admission and staleness.
  DeviceIdentity identitySnapshot() const noexcept { return _identity; }
  /// @brief Query accepted source capabilities without MDIO or clock access.
  /// @return Invalid until a fresh identity is accepted; no link/settings/qualification claim.
  Capabilities capabilities() const noexcept;

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
  /// @return Direct-access errors or the retained failed identity status; OK is
  /// frame completion, not verified pin state. Failed/late writes may have applied.
  /// @note SNLS647I table 8-42: blink, polarity, drive enable/value; reserved zero.
  /// No cached setting is claimed. Copy transportSnapshot before another batch.
  /// Replaces all four fields: unset bits select 20 Hz blink and active-low
  /// polarity, overriding the strap. Use modifyMmd to change one field.
  Status writeClause22(uint32_t registerAddress, uint32_t value,
                       const IoBudget& budget) noexcept;
  /// @brief Read one admitted nonconsuming MMD word in one four-frame batch.
  /// @param[in] devad Device selector; vendor31 plus the three source-implied EEE reads.
  /// @param[in] registerAddress Sixteen-bit documented offset; no indirect selectors.
  /// @param[out] value Unchanged unless all frames, release and timing succeed.
  /// @param[in] budget Original exclusive deadline and at least four frames.
  /// @return Direct-access terminal errors; INVALID_PARAM for holes, consuming reads
  /// or unadmitted DEVAD/offsets. EEE mappings remain physically unqualified.
  /// @note SNLS647I 8.3.11.1/.5, tables 8-31/32. Clause 22 aliases inherit the
  /// same policy. Raw TDR/MSE/strap words do not establish freshness or units.
  Status readMmd(uint32_t devad, uint32_t registerAddress, uint16_t& value,
                 const IoBudget& budget) noexcept;
  /// @brief Write one admitted vendor-MMD word in one four-frame batch.
  /// @param[in] devad Device selector; only 31 is admitted for mutation.
  /// @param[in] registerAddress Currently only the LEDCR alias (0x0018).
  /// @param[in] value Sixteen-bit word; reserved bits must be zero.
  /// @param[in] budget Original exclusive deadline and at least four frames.
  /// @pre Same exact-part, maintenance and pin-use preconditions as writeClause22.
  /// @return Terminal status; errors retain selector/possible target effects.
  /// @note SNLS647I 8.3.11.4 and table 8-42. Always reselects the address;
  /// no retry or interpretation of a prior ambiguous write as verified success.
  /// Replaces all four fields: unset bits select 20 Hz blink and active-low
  /// polarity, overriding the strap. Use modifyMmd to change one field.
  Status writeMmd(uint32_t devad, uint32_t registerAddress, uint32_t value,
                  const IoBudget& budget) noexcept;
  /// @brief Modify safe stable fields under one five-frame MMD transaction.
  /// @param[in] devad Device selector; only 31 is admitted for mutation.
  /// @param[in] registerAddress Currently only the LEDCR alias (0x0018).
  /// @param[in] mask Nonzero sixteen-bit subset of LEDCR's documented stable bits.
  /// @param[in] value Sixteen-bit replacement bits; must be a subset of mask.
  /// @param[in] budget Original exclusive deadline and at least five frames.
  /// @pre Owner excludes reset/external writers and meets writeClause22 preconditions.
  /// @return Terminal status; a failed or late dependent write may have applied.
  /// @note Five frames are inferred from SNLS647I 8.3.11.1, not a printed sequence.
  /// Nonincrementing selection persists through
  /// the read and following write. Preserve only stable bits outside mask, force
  /// LEDCR reserved bits zero. Other targets require per-register reserved handling.
  /// Never replay commands or update mixed/destructive fields through generic RMW.
  /// No readback verification, automatic retry or separate public lock scope.
  Status modifyMmd(uint32_t devad, uint32_t registerAddress, uint32_t mask,
                   uint32_t value, const IoBudget& budget) noexcept;
  /// @brief Explicitly acquire destructive words through the sole private collector.
  /// @param[in] sourceMask Nonzero subset of ALL_EVIDENCE; no implicit sources.
  /// @param[out] evidence Assigned only when every requested read/release succeeds on time.
  /// @param[in] budget Aggregate deadline and frames; complete requested count admitted first.
  /// @pre Application owns the instance, exact part/address and diagnostic admission.
  /// @return Terminal transport/validation status; errors retain consumed partial samples.
  /// @note PHYSTS is read first when requested. RXFS uses a complete MMD sequence.
  /// Reads can clear IRQ/history; ambiguous reads set historyLost, never zero events.
  /// A collection spans bounded batches; it is not a simultaneous hardware snapshot.
  Status acquireEvidence(uint32_t sourceMask, EvidenceSnapshot& evidence,
                         const IoBudget& budget) noexcept;
  /// @brief Update only MISR1/2 stable enables while retaining consumed events.
  /// @param[in] registerAddress C22 MISR1 (0x12) or MISR2 (0x13).
  /// @param[in] mask Nonzero low-byte owned mask.
  /// @param[in] value Replacement subset of mask.
  /// @param[in] budget Two-frame atomic read/write budget.
  /// @pre Exact-part maintenance owner coordinates interrupt users and pin routing.
  /// @return Terminal status; partial reads remain in evidenceSnapshot on write failure.
  /// @note Tables 8-36/37. This is register maintenance, not IRQ pin setup/service.
  Status modifyInterruptEnables(uint32_t registerAddress, uint32_t mask,
                               uint32_t value, const IoBudget& budget) noexcept;
  /// @brief Maintain RXFCFG1 with exact W0C semantics and pre-write event capture.
  /// @param[in] mask Stable subset of 0x17A1; zero allowed only for acknowledgement.
  /// @param[in] value Replacement subset of mask; full pulse-width field when changed.
  /// @param[in] acknowledge True writes zero to bit 11; false preserves pending level.
  /// @param[in] budget Aggregate allowance of at least 14 frames; no hidden retries.
  /// @pre Exact DP83826I, quiesced traffic, safe wake wiring and exclusive semantic
  /// configuration owner. This does not configure a MAC/password or qualify WoL.
  /// @return Terminal status; failed writes leave configuration evidence dirty.
  /// @note Table 8-90. Read stable state, skip unchanged unrelated updates, capture
  /// RXFS/MISR2, then perform one atomic five-frame RMW. Unknown bit 6 and reserved
  /// bits are preserved. Full feature admission/restoration belongs to prompt 31.
  Status modifyWakeControl(uint32_t mask, uint32_t value, bool acknowledge,
                          const IoBudget& budget) noexcept;
  /// @brief Read retained samples without MDIO or clock callbacks.
  /// @return Copied history; valid/timeValid/stale/historyLost are independent.
  EvidenceSnapshot evidenceSnapshot() const noexcept { return _evidence; }
  /// @brief Invalidate configuration and mark observations historical without I/O.
  /// @param[in] reason Known external/raw uncertainty; invalid enum is rejected.
  /// @return OK or INVALID_PARAM; BUSY during a callback; samples are never erased.
  Status invalidateDeviceState(InvalidationReason reason) noexcept;
  /// @brief Copy retained raw/normalized evidence without clock or MDIO callbacks.
  /// @return Copied report; raw invalid timestamps/statuses must not be trusted.
  TransportSnapshot transportSnapshot() const noexcept { return _transport; }

private:
  Config _config{};
  TransportSnapshot _transport{};
  EvidenceSnapshot _evidence{};
  DeviceIdentity _identity{};
  bool _bound{false};
  bool _active{false};
  bool _stopped{false};
  bool _timeValid{false};
  uint64_t _lastTimeUs{0};
  Status _sampleTime(uint64_t& raw, uint64_t& upper) noexcept;
  Status _execute(MdioBatch batch, const IoBudget& budget, bool* called = nullptr) noexcept;
  void _retainEvidence(const MdioBatch& batch, const BatchResult& result,
                       bool clockValid, uint64_t before, uint64_t upper) noexcept;
  void _invalidateEvidence() noexcept;
  Status _checkMutation() const noexcept;
};

} // namespace DP8382x
