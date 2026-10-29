/// @file Config.h
/// @brief Pure link settings and borrowed terminal MDIO transport values.
#pragma once

#include <array>
#include <cstdint>

#include <DP8382x/Status.h>

/// @brief Portable DP8382x configuration and transport values.
namespace DP8382x {

/// @brief Exact declared ordering part; an unknown part is never admitted.
enum class Variant : uint8_t {
  UNKNOWN,  ///< No ordering part has been declared.
  DP83826I ///< Current declared target; suffix is not read from PHY ID.
};

/// @brief Observed PHY identity mode; it is never a software selector.
enum class OperatingMode : uint8_t {
  UNKNOWN, ///< Either valid mode may be accepted when used as a requirement.
  BASIC,   ///< DP83826 basic identity model.
  ENHANCED ///< DP83826 enhanced identity model.
};

/// @brief Application-declared MAC wiring, not an observed strap value.
enum class MacInterface : uint8_t {
  UNKNOWN, ///< Board interface has not been declared.
  MII,     ///< Supported DP83826I MAC interface.
  RMII,    ///< Supported DP83826I MAC interface.
  RGMII    ///< Representable for validation, absent on DP83826I.
};

/// @brief Acceptance rule for complete identity; not a reset-value guess.
enum class RevisionPolicy : uint8_t {
  DP83826I_REV1_ONLY ///< Current evidence accepts revision 1 only.
};

/// @brief API technology bits, distinct from ANAR wire positions.
constexpr uint8_t LINK_10_HALF = 0x01; ///< API bit for 10 Mbit/s half duplex.
constexpr uint8_t LINK_10_FULL = 0x02; ///< API bit for 10 Mbit/s full duplex.
constexpr uint8_t LINK_100_HALF = 0x04; ///< API bit for 100 Mbit/s half duplex.
constexpr uint8_t LINK_100_FULL = 0x08; ///< API bit for 100 Mbit/s full duplex.
/// @brief Union of all currently defined API technology bits.
constexpr uint8_t LINK_TECHNOLOGY_MASK =
    LINK_10_HALF | LINK_10_FULL | LINK_100_HALF | LINK_100_FULL;

/// @brief API interface bits for a future accepted capability observation.
constexpr uint8_t INTERFACE_MII = 0x01; ///< API bit for MII.
constexpr uint8_t INTERFACE_RMII = 0x02; ///< API bit for RMII.
constexpr uint8_t INTERFACE_RGMII = 0x04; ///< API bit for RGMII, absent on DP83826I.

/// @brief Requested link speed; UNKNOWN is not a valid active forced speed.
enum class LinkSpeed : uint8_t {
  UNKNOWN, ///< Not selected.
  MBPS_10, ///< 10 Mbit/s.
  MBPS_100 ///< 100 Mbit/s.
};

/// @brief Requested duplex; UNKNOWN is not a valid active forced duplex.
enum class Duplex : uint8_t {
  UNKNOWN, ///< Not selected.
  HALF,    ///< Half duplex.
  FULL     ///< Full duplex.
};

/// @brief Persistent link policy, separate from current link observation.
enum class LinkMode : uint8_t {
  AUTONEGOTIATED, ///< Advertise capabilities to the partner.
  FORCED          ///< Apply one explicit 10/100 and half/full setting.
};

/// @brief Requested pair routing, distinct from observed cable routing.
enum class PairSelection : uint8_t {
  AUTOMATIC, ///< Automatic crossover.
  MDI,       ///< Manual MDI routing.
  MDIX       ///< Manual MDIX routing.
};

/// @brief Local AN intent; no transport action occurs when this value is made.
struct Advertisement {
  uint8_t technologyMask{0}; ///< Nonempty LINK_* subset for active AN.
  bool symmetricPause{false}; ///< ANAR symmetric pause request.
  bool asymmetricPause{false}; ///< ANAR asymmetric pause request.
  bool nextPage{false};        ///< Requires a later source-gated exchange owner.
};

/// @brief Forced link intent; both fields must be known when active.
struct ForcedLinkSettings {
  LinkSpeed speed{LinkSpeed::UNKNOWN}; ///< Requested Mbit/s mode.
  Duplex duplex{Duplex::UNKNOWN};      ///< Requested half/full duplex.
};

/// @brief Pair and crossover intent; never an observation of cable wiring.
struct CrossoverSettings {
  PairSelection pairSelection{PairSelection::AUTOMATIC}; ///< Routing policy.
  bool robust{true};       ///< Required for every forced mode.
  bool swapPair{false};    ///< CR3 pair swap, separate from PHYCR routing.
  bool swapPolarity{false};///< Polarity swap; mirroring requests both swaps.
};

/// @brief Persistent desired link settings, not verified hardware state.
struct LinkSettings {
  LinkMode mode{LinkMode::AUTONEGOTIATED}; ///< AN or forced request.
  Advertisement advertisement{};        ///< Active only for AN.
  ForcedLinkSettings forced{};           ///< Active only for forced mode.
  CrossoverSettings crossover{};         ///< Coupled routing intent.
  bool extendedFullDuplex{false};        ///< Later partner-coordinated opt-in.
};

/// @brief Check exact supported part membership without I/O or a clock.
/// @param[in] variant Declared ordering part, including possible invalid casts.
/// @return True only for DP83826I.
inline constexpr bool isValidVariant(Variant variant) noexcept {
  return variant == Variant::DP83826I;
}

/// @brief Check Clause 22 PHY-address width without narrowing the input.
/// @param[in] phyAddress Caller value; only 0 through 31 is legal.
/// @return True for a protocol-valid address, not proof of a strapped device.
inline constexpr bool isValidPhyAddress(uint32_t phyAddress) noexcept {
  return phyAddress <= 31U;
}

/// @brief Check typed link intent without I/O, clock access or mutation.
/// @param[in] settings Desired settings; every enum is checked, including inactive fields.
/// @return OK for pure legal intent, INVALID_PARAM/ENUM_VALUE for invalid enum
/// storage, or INVALID_CONFIG/LINK_COMBINATION for inconsistent fields.
/// @note OK does not establish silicon state or clear later source/board gates.
inline constexpr Status validateLinkSettings(const LinkSettings& settings) noexcept {
  if ((settings.mode != LinkMode::AUTONEGOTIATED && settings.mode != LinkMode::FORCED) ||
      (settings.forced.speed != LinkSpeed::UNKNOWN &&
       settings.forced.speed != LinkSpeed::MBPS_10 &&
       settings.forced.speed != LinkSpeed::MBPS_100) ||
      (settings.forced.duplex != Duplex::UNKNOWN &&
       settings.forced.duplex != Duplex::HALF &&
       settings.forced.duplex != Duplex::FULL) ||
      (settings.crossover.pairSelection != PairSelection::AUTOMATIC &&
       settings.crossover.pairSelection != PairSelection::MDI &&
       settings.crossover.pairSelection != PairSelection::MDIX)) {
    return Status::Error(Err::INVALID_PARAM, "Invalid link enum",
                         static_cast<int32_t>(ParamDetail::ENUM_VALUE));
  }

  if ((settings.advertisement.technologyMask &
       static_cast<uint8_t>(~LINK_TECHNOLOGY_MASK)) != 0U) {
    return Status::Error(Err::INVALID_CONFIG, "Invalid technology mask",
                         static_cast<int32_t>(ConfigDetail::LINK_COMBINATION));
  }
  if (settings.mode == LinkMode::AUTONEGOTIATED) {
    if (settings.advertisement.technologyMask == 0U) {
      return Status::Error(Err::INVALID_CONFIG, "Empty AN technology mask",
                           static_cast<int32_t>(ConfigDetail::LINK_COMBINATION));
    }
    return Status::Ok();
  }
  if (settings.forced.speed == LinkSpeed::UNKNOWN ||
      settings.forced.duplex == Duplex::UNKNOWN ||
      !settings.crossover.robust ||
      settings.advertisement.technologyMask != 0U ||
      settings.advertisement.symmetricPause ||
      settings.advertisement.asymmetricPause ||
      settings.advertisement.nextPage) {
    return Status::Error(Err::INVALID_CONFIG, "Invalid forced link combination",
                         static_cast<int32_t>(ConfigDetail::LINK_COMBINATION));
  }
  return Status::Ok();
}

/// @brief Maximum physical frames in one indivisible terminal batch.
constexpr uint8_t MAX_BATCH_FRAMES = 8;
/// @brief Literal-write sentinel for MdioFrame::readIndex.
constexpr uint8_t NO_READ_INDEX = 0xFF;

/// @brief Clause 22 frame direction; invalid numeric casts are not admitted.
enum class FrameKind : uint8_t {
  READ, ///< One complete 16-bit read.
  WRITE ///< One complete 16-bit write.
};

/// @brief Absolute owner budget in the Transport::nowUs clock domain.
struct IoBudget {
  uint64_t deadlineUs{0}; ///< Exclusive microsecond deadline.
  uint32_t maxFrames{0};  ///< Total allowed physical frames, excluding retries.
};

/// @brief One backend frame description; only the driver may construct a batch.
/// @note READ keeps value/preserveMask at zero and readIndex at NO_READ_INDEX.
/// A literal WRITE keeps preserveMask zero and readIndex at NO_READ_INDEX.
/// A dependent WRITE refers to an
/// earlier successful READ in this batch and writes
/// (readValue & preserveMask) | value; value and preserveMask must not overlap.
struct MdioFrame {
  FrameKind kind{FrameKind::READ}; ///< Read or write.
  uint8_t registerAddress{0};     ///< Clause 22 address 0 through 31.
  uint16_t value{0};              ///< Literal write bits; ignored on read.
  uint16_t preserveMask{0};       ///< Earlier read bits retained by dependent write.
  uint8_t readIndex{NO_READ_INDEX}; ///< Earlier READ index or literal sentinel.
};

/// @brief One PHY's complete terminal operation under a controller-wide lock.
struct MdioBatch {
  uint8_t phyAddress{0}; ///< One protocol-valid PHY address.
  uint8_t frameCount{0}; ///< Used frames, 1 through MAX_BATCH_FRAMES.
  std::array<MdioFrame, MAX_BATCH_FRAMES> frames{}; ///< Unused entries stay default.
};

/// @brief Retained evidence for a physical frame, including partial effects.
/// @note A supplied completion timestamp requires completed=true; it is a
/// conservative upper bound in the same clock domain as IoBudget.
struct FrameResult {
  bool attempted{false};      ///< Backend launched or invoked this frame.
  bool completed{false};      ///< Terminal controller success was reported.
  bool valueValid{false};     ///< Complete read word is available.
  uint16_t value{0};         ///< Valid only for a completed read.
  bool effectPossible{false};///< Failed access may still have had hardware effect.
  uint64_t completedAtUs{0}; ///< Conservative completion upper bound if timeValid.
  bool timeValid{false};     ///< Zero timestamp is valid when this is true.
};

/// @brief Terminal primary/release evidence; default is deliberately failure.
struct BatchResult {
  Status status{Err::MDIO_ERROR, 0, "Batch result not supplied"}; ///< Primary result.
  Status releaseStatus{Err::MDIO_ERROR, 0, "Release not established"}; ///< Lock release.
  bool controllerReusable{false}; ///< False when release or controller idle is unknown.
  std::array<FrameResult, MAX_BATCH_FRAMES> frames{}; ///< Attempted/completed prefix.
};

/// @brief Borrowed synchronous callback for one whole terminal batch.
/// @param[in] context Application-owned callback context; null is legal.
/// @param[in] batch Borrowed descriptor; never retained beyond the call.
/// @param[in] budget Original exclusive deadline and physical-frame allowance.
/// @return Complete terminal primary/release/frame evidence, including failure.
/// @note Backend owns controller-wide exclusion, finite execution and release;
/// no reentry, asynchronous completion, hidden retry or retained buffers.
using ExecuteBatch = BatchResult (*)(void* context, const MdioBatch& batch,
                                    const IoBudget& budget) noexcept;

/// @brief Borrowed bounded monotonic microsecond clock callback; no MDIO or reentry.
/// @param[in] context Application-owned callback context; null is legal.
/// @return Nonwrapping time in microseconds for one binding.
using NowUs = uint64_t (*)(void* context) noexcept;

/// @brief Owner-proved conservative bounds for each complete batch segment.
struct TransportBounds {
  uint32_t acquireUs{0}; ///< Controller-lock acquisition upper bound, microseconds.
  uint32_t frameUs{0};   ///< Positive bound per frame including wire gap, microseconds.
  uint32_t releaseUs{0}; ///< Release/error-finalization bound, microseconds.
  uint32_t overheadUs{0};///< Positive clock/core/publication bound, microseconds.
  uint32_t clockLagUs{0};///< Positive maximum sample age on clock return, microseconds.
};

/// @brief Borrowed application transport; core never owns controller resources.
struct Transport {
  void* context{nullptr};          ///< Borrowed through all operations and cleanup.
  ExecuteBatch executeBatch{nullptr}; ///< Required terminal batch callback.
  NowUs nowUs{nullptr};            ///< Required bounded clock callback.
  TransportBounds bounds{};        ///< No guessed platform timing defaults.
};

/// @brief Board and transport declaration; construction performs no I/O.
/// @note Binding will copy this value but still borrow context/callback lifetime.
/// No pins, SDK objects, network policy or hardware reset are owned here.
struct Config {
  Variant variant{Variant::UNKNOWN}; ///< Exact ordering part, required for binding.
  uint32_t phyAddress{0};            ///< Validate before narrowing to 5-bit wire value.
  MacInterface interface{MacInterface::UNKNOWN}; ///< Required board MAC wiring.
  OperatingMode requiredMode{OperatingMode::UNKNOWN}; ///< UNKNOWN accepts either valid mode.
  RevisionPolicy revisionPolicy{RevisionPolicy::DP83826I_REV1_ONLY}; ///< Identity policy.
  Transport transport{};            ///< Borrowed owner callback and bounds.
  uint32_t offlineThreshold{3};     ///< Passive count threshold; legal 1..UINT32_MAX.
};

} // namespace DP8382x
