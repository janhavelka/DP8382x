/// @file Dp8382xCliFeatureTypes.h
/// @brief Fixed-size inputs and record fields for typed CLI features.
#pragma once

#include <DP8382x/DP8382x.h>
#include <array>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string_view>

namespace DP8382x::Cli {

/// @brief Feature commands defined in the shared dispatch table.
enum class Feature {
  NONE, ///< No feature selected.
/// @cond INTERNAL
#define FEATURE(id, name, minimum, maximum, mode, usage, summary) id,
#include "Dp8382xCliDiagnosticCommands.inc"
#include "Dp8382xCliControlCommands.inc"
#include "Dp8382xCliPinCommands.inc"
#undef FEATURE
/// @endcond
};

/// @brief Owner access required by a typed feature command.
enum class FeatureMode {
  CACHED, ///< Copy retained evidence without I/O.
  READ, ///< Terminal acquisition under the existing owner.
  WRITE, ///< Terminal mutation under maintenance.
  OBSERVE, ///< Cooperative acquisition under existing ownership.
  START, ///< Cooperative mutation under maintenance.
  RESET ///< Cooperative operation can require a reset reservation.
};

/// @brief Return the owner's required access for a feature.
/// @param[in] feature Command selector.
/// @return Access mode used by dispatch and the platform owner.
constexpr FeatureMode featureMode(Feature feature) noexcept {
  switch (feature) {
/// @cond INTERNAL
#define FEATURE(id, name, minimum, maximum, mode, usage, summary) case Feature::id: return FeatureMode::mode;
#include "Dp8382xCliDiagnosticCommands.inc"
#include "Dp8382xCliControlCommands.inc"
#include "Dp8382xCliPinCommands.inc"
#undef FEATURE
/// @endcond
    case Feature::NONE: break;
  }
  return FeatureMode::CACHED;
}

/// @brief Borrowed application attestation for a feature's independent prerequisites.
/// @note A token supplied by the terminal does not grant permission. The callback
/// checks the current application, wiring or fixture state without I/O or waits.
using FeatureAttestation = bool (*)(void*, Feature, uint32_t);

/// @brief Copied tokens. The input line can be reused before the owner runs.
struct FeatureArgs {
  std::array<char, 192> bytes{}; ///< Packed tokens without delimiters.
  std::array<uint8_t, 8> lengths{}; ///< Token lengths, zero after the last token.
  uint8_t count{0}; ///< Number of tokens.

  /// @brief Return the number of feature tokens.
  /// @return Count, including zero for a query with no arguments.
  std::size_t size() const noexcept { return count; }

  /// @brief Return one token borrowed from this copy.
  /// @param[in] index Zero-based token index.
  /// @return Empty for an absent token.
  std::string_view operator[](std::size_t index) const noexcept {
    if (index >= count || index >= lengths.size()) return {};
    std::size_t offset = 0;
    for (std::size_t i = 0; i < index; ++i) offset += lengths[i];
    if (offset + lengths[index] > bytes.size()) return {};
    return {bytes.data() + offset, lengths[index]};
  }
};

/// @brief Optional ASCII fields from one typed call or its retained result.
struct FeatureFields {
  std::array<char, 401> text{}; ///< Space-prefixed machine fields including the NUL.
  bool valid{true}; ///< False if formatting could not retain all fields.

  /// @brief Replace the field list with formatted bounded fields.
  /// @param[in] format Format string and matching arguments. Use static safe
  /// field labels, numbers or printable enum names. Do not include credentials.
  void set(const char* format, ...) noexcept {
    va_list args;
    va_start(args, format);
    const int written = std::vsnprintf(text.data(), text.size(), format, args);
    va_end(args);
    valid = written >= 0 && static_cast<std::size_t>(written) < text.size();
    if (!valid) text[0] = '\0';
  }
};

/// @brief Parse a whole unsigned decimal or 0x-prefixed token with an explicit maximum.
/// @param[in] token User input with no sign.
/// @param[in] maximum Inclusive upper bound.
/// @param[out] value Assigned on success.
/// @return Whether the complete token is in range.
bool featureNumber(std::string_view token, uint32_t maximum, uint32_t& value) noexcept;

/// @brief Find a case-sensitive whole-token choice in a fixed set.
/// @param[in] token User token.
/// @param[in] choices Static choices separated by vertical bars.
/// @return Zero-based index or -1.
int featureChoice(std::string_view token, std::string_view choices) noexcept;

/// @brief Report malformed typed feature input.
/// @return INVALID_PARAM with a static message.
inline Status featureInvalid() noexcept { return Status::Error(Err::INVALID_PARAM, "Invalid feature arguments"); }

/// @brief Report missing application proof for an otherwise supported feature.
/// @return BUSY with owner-admission detail.
inline Status featureDenied() noexcept {
  return Status::Error(Err::BUSY, "Feature requires application fixture admission",
                       static_cast<int32_t>(BusyDetail::OWNER_ADMISSION));
}

using Core = ::DP8382x::DP8382x; ///< Existing portable implementation used by both owners.
} // namespace DP8382x::Cli
