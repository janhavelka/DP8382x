/// @file Status.h
/// @brief Portable result codes and signed diagnostic details.
#pragma once

#include <cstdint>

/// @brief Portable DP8382x value types and result helpers.
namespace DP8382x {

/// @brief Library outcome; these values are not an SDK or I2C ABI.
enum class Err : uint8_t {
  OK,                   ///< Complete success.
  IN_PROGRESS,          ///< Accepted work has not reached a terminal result.
  INVALID_PARAM,        ///< Invalid call argument or prohibited direct access.
  INVALID_CONFIG,       ///< Inconsistent configuration or unavailable prerequisite.
  NOT_BOUND,            ///< No transport binding exists.
  NOT_INITIALIZED,      ///< Current PHY initialization has not been accepted.
  UNSUPPORTED,          ///< The declared silicon lacks a source-proved capability.
  DEVICE_NOT_FOUND,     ///< Complete identity words are implausible.
  DEVICE_MISMATCH,      ///< Complete identity conflicts with the declaration.
  MDIO_ERROR,           ///< Terminal controller or callback failure.
  MDIO_TIMEOUT,         ///< Controller or lock attempt timed out.
  OPERATION_TIMEOUT,    ///< Whole operation or admitted work budget expired.
  CONFIG_MISMATCH,      ///< Stable owned bits disagree after valid readback.
  BUSY,                 ///< Current owner, controller or retained result blocks work.
  CANCELLED,            ///< Owner cancelled accepted work.
  RESULT_NOT_AVAILABLE, ///< No terminal result can be taken.
  TOKEN_MISMATCH        ///< A result belongs to a different operation token.
};

/// @brief Copyable status with a full signed backend detail when applicable.
/// @note msg must be non-null, immutable, static-lifetime text. The value never
/// owns or frees that pointer. Consumers branch on code, not message text.
struct Status {
  Err code{Err::OK};      ///< OK alone is success; IN_PROGRESS is not success.
  int32_t detail{0};     ///< Backend detail for MDIO errors, otherwise code-local.
  const char* msg{""};   ///< Borrowed static-lifetime diagnostic message.

  /// @brief Test for complete success without I/O.
  /// @return True only for OK.
  constexpr bool ok() const noexcept { return code == Err::OK; }
  /// @brief Compare codes without validating the expected enum value.
  /// @param[in] expected Code to compare with this value.
  /// @return True when the stored code equals expected.
  constexpr bool is(Err expected) const noexcept { return code == expected; }
  /// @brief Test whether accepted work remains unfinished.
  /// @return True only for IN_PROGRESS.
  constexpr bool inProgress() const noexcept { return code == Err::IN_PROGRESS; }
  /// @brief Convert to true only for complete success.
  /// @return True only for OK.
  constexpr explicit operator bool() const noexcept { return ok(); }
  /// @brief Construct a successful value with static message storage.
  /// @return OK with detail zero and a static message.
  static constexpr Status Ok() noexcept { return {Err::OK, 0, "OK"}; }
  /// @brief Construct a non-success value without touching hardware.
  /// @param[in] error Known non-OK code.
  /// @param[in] message Non-null static-lifetime message.
  /// @param[in] backendDetail Full signed backend detail or code-local reason.
  /// @return Copyable value; arguments are not validated or changed.
  static constexpr Status Error(Err error, const char* message,
                                int32_t backendDetail = 0) noexcept {
    return {error, backendDetail, message};
  }
};

/// @brief Return a static uppercase name or UNKNOWN for an invalid enum value.
/// @param[in] error Code to name; no hardware or allocation is used.
/// @return Static text, including for invalid values.
inline constexpr const char* errorName(Err error) noexcept {
  switch (error) {
    case Err::OK: return "OK";
    case Err::IN_PROGRESS: return "IN_PROGRESS";
    case Err::INVALID_PARAM: return "INVALID_PARAM";
    case Err::INVALID_CONFIG: return "INVALID_CONFIG";
    case Err::NOT_BOUND: return "NOT_BOUND";
    case Err::NOT_INITIALIZED: return "NOT_INITIALIZED";
    case Err::UNSUPPORTED: return "UNSUPPORTED";
    case Err::DEVICE_NOT_FOUND: return "DEVICE_NOT_FOUND";
    case Err::DEVICE_MISMATCH: return "DEVICE_MISMATCH";
    case Err::MDIO_ERROR: return "MDIO_ERROR";
    case Err::MDIO_TIMEOUT: return "MDIO_TIMEOUT";
    case Err::OPERATION_TIMEOUT: return "OPERATION_TIMEOUT";
    case Err::CONFIG_MISMATCH: return "CONFIG_MISMATCH";
    case Err::BUSY: return "BUSY";
    case Err::CANCELLED: return "CANCELLED";
    case Err::RESULT_NOT_AVAILABLE: return "RESULT_NOT_AVAILABLE";
    case Err::TOKEN_MISMATCH: return "TOKEN_MISMATCH";
  }
  return "UNKNOWN";
}

/// @brief INVALID_PARAM detail values; never replace backend diagnostics.
enum class ParamDetail : int32_t {
  ENUM_VALUE = 1, ///< Invalid numeric enum representation.
  ADDRESS_RANGE,  ///< Address lies outside its protocol width.
  VALUE_WIDTH,    ///< Numeric data exceeds its wire width.
  ACCESS_POLICY,  ///< A direct call cannot access this register or effect.
  BUDGET_RANGE    ///< Invalid caller work allowance.
};

/// @brief INVALID_CONFIG detail values.
enum class ConfigDetail : int32_t {
  TRANSPORT = 1,      ///< Required callback or truthful bound is absent.
  HEALTH_THRESHOLD,   ///< Passive OFFLINE threshold is zero.
  LINK_COMBINATION,   ///< Typed link settings disagree or omit required intent.
  SOURCE_GATE,        ///< Essential chip procedure evidence remains unresolved.
  BOARD_PREREQUISITE, ///< Required board condition was not established.
  CLOCK_CONTRACT,     ///< Monotonic clock or its bound was violated.
  ENUM_VALUE,         ///< Binding contains an invalid enum representation.
  ADDRESS_RANGE,      ///< Binding PHY address exceeds Clause 22 range.
  INTERFACE_REQUIRED, ///< No MAC interface was declared.
  DEVICE_INVALIDATED  ///< Current device evidence was invalidated externally.
};

/// @brief BUSY detail values.
enum class BusyDetail : int32_t {
  OPERATION_ACTIVE = 1, ///< An operation already owns the instance.
  RESULT_PENDING,      ///< A terminal result still needs delivery.
  OWNER_ADMISSION,     ///< Another owner has not admitted the request.
  CONTROLLER_QUARANTINED, ///< Shared controller is not proved reusable.
  CLEANUP_REQUIRED,    ///< Hardware cleanup still needs a safe route.
  OBSERVATION_CHANGED, ///< Aggregate observation changed while sampled.
  TOKEN_EXHAUSTED,     ///< Lifetime token counter cannot advance.
  RETIRE_REQUIRED      ///< Binding must end before reuse.
};

/// @brief DEVICE_NOT_FOUND or DEVICE_MISMATCH detail values.
enum class IdentityDetail : int32_t {
  ZERO_ID = 1,          ///< Required identity word is zero.
  ONES_ID,              ///< Required identity word is all ones.
  WRONG_OUI,            ///< Complete OUI differs from accepted evidence.
  WRONG_MODEL,          ///< Complete model is not accepted.
  REVISION_NOT_ACCEPTED,///< Revision is outside the declared policy.
  MODE_MISMATCH         ///< Observed mode differs from required mode.
};

/// @brief OPERATION_TIMEOUT detail values.
enum class TimeoutDetail : int32_t {
  DEADLINE = 1,    ///< Exclusive deadline reached.
  FRAME_BUDGET,    ///< Physical frame allowance is insufficient.
  BOUND_ADMISSION, ///< Complete worst-case work cannot fit.
  POLL_BUDGET      ///< Finite cooperative poll allowance exhausted.
};

/// @brief Source-proved absent silicon capability detail values.
enum class UnsupportedDetail : int32_t {
  INTERFACE = 1, ///< Declared MAC interface is absent.
  LINK_MODE,     ///< Requested link mode is absent.
  FEATURE        ///< Requested programmable feature is absent.
};

} // namespace DP8382x
