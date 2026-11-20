/// @file Dp8382xCli.h
/// @brief Portable example command processor and borrowed application boundary.
#pragma once

#include "ConsoleBuffer.h"
#include <DP8382x/DP8382x.h>
#include <array>
#include <cstdint>
#include <string_view>

namespace DP8382x::Cli {

/// @brief Cached values available through the application owner.
enum class SnapshotKind {
  CAPABILITIES, ///< Capability evidence from accepted identity.
  HEALTH,       ///< Managed MDIO health.
  LINK,         ///< Last acquired link evidence.
  SETTINGS,     ///< Desired, observed and trusted configuration.
  OPERATION,    ///< Occupied slot and pending restoration evidence.
  TRANSPORT     ///< Attributed batch and physical frame counts.
};

/// @brief Copy destination. The callback only fills the requested member.
struct CachedSnapshot {
  Capabilities capabilities{}; ///< Capability evidence, valid only when flagged.
  HealthSnapshot health{};      ///< Cached state, counts and last managed error.
  LinkSnapshot link{};          ///< Cached link, resolution and freshness flags.
  SettingsSnapshot settings{};  ///< Cached configuration intent and readback.
  OperationSnapshot operation{}; ///< Cached slot state, not a consumed result.
  TransportSnapshot transport{}; ///< Cached totals, including non-managed work.
};

/// @brief Borrowed callbacks called only by the application task that owns the CLI.
/// @pre No callback re-enters the processor. Context stays alive through the last call.
struct Platform {
  void* context{nullptr}; ///< Passed unchanged to all callbacks.
  /// @brief Accept a prefix of the offered bytes, including zero for backpressure.
  /// @note Buffer lifetime is the call. Return promptly without waiting for capacity.
  std::size_t (*write)(void*, const char*, std::size_t){nullptr};
  /// @brief Copy one cached view under existing owner admission, with no PHY I/O.
  /// @note Destination is borrowed for the call. Return a terminal Status.
  Status (*readCached)(void*, SnapshotKind, CachedSnapshot&){nullptr};
  /// @brief Optional nonwrapping firmware clock, in microseconds since its local origin.
  /// @note Called at record creation. Null means the record has no timestamp.
  uint64_t (*nowUs)(void*){nullptr};
};

/// @brief Values declared by the application, separate from observed silicon.
/// @note Strings must be immutable, NUL terminated and alive through all calls.
/// Each has 1 to 64 ASCII letters, digits, underscores, dots, colons or hyphens.
struct Identity {
  uint32_t boot{0}; ///< Application supplies a varying identifier for each boot.
  const char* firmware{"unknown"}; ///< Firmware revision or opaque build identity.
  const char* tree{"unknown"};     ///< clean, dirty or unknown.
  const char* sdk{"unknown"};      ///< SDK version or unknown.
  const char* target{"unknown"};   ///< Firmware target or unknown.
  const char* backend{"unknown"};  ///< Declared board or backend name.
  const char* clock{"unknown"};    ///< Declared clock choice, with units when applicable.
  Variant variant{Variant::UNKNOWN}; ///< Declared part, not an observed ID.
  MacInterface interface{MacInterface::RMII}; ///< Declared MAC interface.
  uint32_t phyAddress{0}; ///< Declared Clause 22 address, 0 to 31.
};

/// @brief One fixed-memory CLI per application stream.
/// @pre The application serializes setup, feed, discardInput and tick, and keeps
/// console writes outside driver callbacks and controller locks.
class Processor {
public:
  static constexpr std::size_t LINE_BYTES = 192; ///< Stored input bytes before delimiter.
  static constexpr std::size_t MAX_TOKENS = 10; ///< Space/tab separated tokens.
  static constexpr std::size_t FEED_BYTES = 128; ///< Maximum bytes consumed by one feed.
  static constexpr std::size_t WRITE_BYTES = 256; ///< Maximum bytes offered by one tick.

  /// @brief Construct a detached processor. One stream owns its sequence and queue.
  Processor() = default;
  /// @brief Pending output and request identity cannot be copied.
  Processor(const Processor&) = delete;
  /// @brief A stream's established identity cannot be assigned to another processor.
  Processor& operator=(const Processor&) = delete;

  /// @brief Attach locally and queue a human prompt. Does not call the PHY or platform.
  /// @param[in] platform Borrowed callbacks and context.
  /// @param[in] identity Declared application identity with borrowed immutable strings.
  /// @return False for invalid input or repeated setup. Existing state is preserved.
  bool setup(const Platform& platform, const Identity& identity);

  /// @brief Accept up to FEED_BYTES, or stop when the terminal record slot is full.
  /// @param[in] bytes Borrowed raw input. The caller retains any unaccepted suffix.
  /// @return Count accepted, including delimiters. Zero before setup or under backpressure.
  std::size_t feed(std::string_view bytes);

  /// @brief Signal an input gap after any older bytes still held by the caller.
  /// Parsing resumes after the next complete line boundary. A queued record survives.
  void discardInput();

  /// @brief Offer one bounded write, then move a retained record into the queue if it fits.
  void tick();

  /// @brief Test whether bytes are queued or retained locally.
  /// @return True for pending local output. False does not prove host receipt.
  bool outputPending() const;

private:
  struct Command;
  struct ConfigOption;
  struct Reply;
  using Args = std::array<std::string_view, MAX_TOKENS>;
  static const Command COMMANDS[];
  static const ConfigOption CONFIG[];
  void finishLine();
  void flushRecord();
  void human(std::string_view bytes);
  void help(const Command&, const Args&, std::size_t, Reply&);
  void identity(const Command&, const Args&, std::size_t, Reply&);
  void configure(const Command&, const Args&, std::size_t, Reply&);
  void console(const Command&, const Args&, std::size_t, Reply&);
  void cached(const Command&, const Args&, std::size_t, Reply&);

  Platform _platform{};
  Identity _identity{};
  ConsoleBuffer _output{};
  std::array<char, LINE_BYTES> _line{};
  std::array<char, ConsoleBuffer::MAX_RECORD_BYTES + 1> _record{};
  std::size_t _lineSize{0};
  std::size_t _recordSize{0};
  uint64_t _request{0};
  const char* _discardReason{nullptr};
  bool _afterCr{false};
  bool _quiet{false};
  bool _color{false};
};
} // namespace DP8382x::Cli
