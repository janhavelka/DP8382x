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
  TRANSPORT,    ///< Attributed batch and physical frame counts.
  IDENTITY      ///< Retained identity, including stale and incomplete words.
};

/// @brief Copy destination. The callback only fills the requested member.
struct CachedSnapshot {
  Capabilities capabilities{}; ///< Capability evidence, valid only when flagged.
  HealthSnapshot health{};      ///< Cached state, counts and last managed error.
  LinkSnapshot link{};          ///< Cached link, resolution and freshness flags.
  SettingsSnapshot settings{};  ///< Cached configuration intent and readback.
  OperationSnapshot operation{}; ///< Cached slot state, not a consumed result.
  TransportSnapshot transport{}; ///< Cached totals, including non-managed work.
  DeviceIdentity identity{};     ///< Cached identity, not a fresh failed-call result.
};

/// @brief One terminal owner call or one step of the existing core operation.
enum class Action {
  PROBE,           ///< Acquire accepted identity at the bound address.
  DISCOVER,        ///< Acquire raw ID words at an explicit address in one batch.
  READ_C22,        ///< Read one safe Clause 22 register.
  READ_MMD,        ///< Read one safe MMD register in a complete indirect batch.
  WRITE_C22,       ///< Write the admitted LEDCR maintenance subset.
  WRITE_MMD,       ///< Write the admitted vendor LEDCR alias.
  MODIFY_MMD,      ///< Atomically update selected vendor LEDCR bits.
  MODIFY_BMCR,     ///< Maintain AN/speed/duplex bits under stopped ownership.
  INVALIDATE,      ///< Report external mutation without touching hardware.
  START_CONFIGURE, ///< Admit typed profile application under maintenance.
  START_VERIFY,    ///< Admit read-only verification.
  POLL,            ///< Advance the admitted operation by at most one batch.
  TAKE,            ///< Copy a terminal result and release that operation's slot.
  RECONCILE        ///< Apply verified intent to stopped SDK cache where applicable.
};

/// @brief Selected owner inputs, copied at command admission.
struct Request {
  Action action{Action::PROBE}; ///< Selects the applicable fields.
  OperationBudget budget{};   ///< Exclusive absolute deadline in owner microseconds and remaining frames.
  uint32_t address{0};        ///< PHY address for discovery, register offset for access.
  uint32_t devad{0};          ///< MMD device address for MMD access.
  uint32_t mask{0};           ///< Owned replacement mask for RMW.
  uint32_t value{0};          ///< Literal or masked write value, validated before narrowing.
  OperationToken token{};     ///< Token returned by the selected owner, for poll/take.
  LinkSettings settings{};    ///< Typed link intent for START_CONFIGURE.
  ConfigureOptions options{}; ///< Bounded negotiation policy for START_CONFIGURE.
};

/// @brief Evidence for this call only. Unavailable evidence remains defaulted.
struct Receipt {
  DeviceIdentity identity{}; ///< Fresh probe or raw discovery prefix; flags govern the values.
  BatchResult batch{};       ///< Fresh raw transport receipt when batchValid.
  bool batchValid{false};    ///< This call acquired the receipt, never historical.
  TransportViolation violation{TransportViolation::NONE}; ///< Core audit of that receipt.
  uint16_t value{0};         ///< Ordinary register output only on successful read.
  OperationToken token{};    ///< Accepted start token only on IN_PROGRESS.
  Status result{};           ///< Core operation status when TAKE returned OK.
  bool mutationCompleted{false}; ///< Operation retained a completed semantic mutation.
  bool mutationPossible{false};  ///< Operation retained uncertain semantic mutation.
  bool cleanupOwed{false};   ///< Retained operation requires later restoration.
  bool temporary{false};     ///< Retained operation may have left temporary state.
  bool negotiationComplete{false}; ///< Explicit AN completion from the retained procedure.
  NextPageResult nextPages{}; ///< Collected next-page progress, including partial reads.
  int32_t sdkError{0};       ///< Separate SDK error on RECONCILE; zero when absent.
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
  /// @brief Optional live command route; requires the same clock used by the owner.
  /// @note Application grants maintenance for disruptive actions before any I/O.
  /// START/POLL may return IN_PROGRESS; TAKE preserves the original core result.
  /// A portable RECONCILE is a local success. Native reconciliation uses public
  /// stopped SDK controls and supplies the independent SDK error.
  Status (*execute)(void*, const Request&, Receipt&){nullptr};
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

  /// @brief Offer one bounded write, move pending records and advance at most one owner call.
  /// @note Core polling can continue under output backpressure. Result delivery
  /// waits for room to keep all correlated output.
  void tick();

  /// @brief Test whether bytes are queued or retained locally.
  /// @return True for pending local output. False does not prove host receipt.
  bool outputPending() const;

  /// @brief True while a live command owns the CLI service slot.
  /// @return Indicates work for a later tick, including deferred result delivery.
  bool workPending() const;

private:
  struct Command;
  struct ConfigOption;
  struct Reply;
  enum class Work { IDLE, CALL, DISCOVERY, DUMP, POLL, TAKE, RECONCILE };
  struct Job {
    Work step{Work::IDLE};
    Request request{};
    Receipt result{};
    std::string_view command{};
    Status firstError{};
    Status stop{};
    uint64_t requestId{0};
    uint64_t lastUs{0};
    uint32_t end{0};
    uint32_t rows{0};
    uint32_t errors{0};
    uint32_t skipped{0};
    uint32_t attempted{0};
    bool attemptedKnown{true};
  };
  using Args = std::array<std::string_view, MAX_TOKENS>;
  static const Command COMMANDS[];
  static const ConfigOption CONFIG[];
  void finishLine();
  void record(std::string_view name, uint64_t req, const Reply& reply);
  void flushRecord();
  void serviceJob();
  void sequenceStep();
  void human(std::string_view bytes);
  void help(const Command&, const Args&, std::size_t, Reply&);
  void identity(const Command&, const Args&, std::size_t, Reply&);
  void configure(const Command&, const Args&, std::size_t, Reply&);
  void console(const Command&, const Args&, std::size_t, Reply&);
  void cached(const Command&, const Args&, std::size_t, Reply&);
  void route(const Command&, const Args&, std::size_t, Reply&);
  bool parseProfile(const Args&, std::size_t count, Request&);

  Platform _platform{};
  Identity _identity{};
  ConsoleBuffer _output{};
  Job _job{};
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
