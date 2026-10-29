# Concrete library conventions

Use these names and responsibilities for DP8382x. Prompts 7-8 establish exact
signatures and behavior in [the API contract](../api-contracts.md); prompt 7's
values/direct calls are now frozen and prompt 8 adds cooperative lifetimes.
Later stages use that contract.
Recheck relevant sibling implementation/tests under COMMON and adapt the baseline
with one coordinated update when source evidence requires a change. Read this
with [ENGINEERING](ENGINEERING.md) and the exact silicon specification.

Prompt 3's [ledger](../registers/dp83826i.json),
[behavior](../device-behavior.md) and [literal vectors](../register-vectors.md)
are the current core research inputs. Future Doxygen must cite exact source
tables for field access/reset and non-obvious procedures, state named unresolved
gates, and distinguish configured settings from independently observed results.

## Source precedents

Paths below are relative to Projects and are read-only references. Reopen the
relevant declarations, implementation and tests at each stage; names alone are
not evidence that two operations mean the same thing.

| Inspected source | Concrete precedent | DP8382x choice |
| --- | --- | --- |
| `TMP1x2/include/TMP1x2/Status.h`, `PCA9555/include/PCA9555/Status.h` | `Err`, `Status`, `errorName`, `ok`, `is`, `inProgress`, `Ok`, `Error` | Preserve value shape and helper spellings; define MDIO errors instead of I2C errors |
| `TMP1x2/include/TMP1x2/TMP1x2.h`, `src/TMP1x2.cpp` | `DriverState`, `HealthSnapshot`, `healthSnapshot`, `track`, separate raw and tracked access | Four passive health states and explicit managed-I/O accounting; keep link and configuration trust separate |
| `PCA9555/include/PCA9555/PCA9555.h`, `src/PCA9555.cpp` | Three health states, `ObservedState`, `RegisterImage`, `pollOperation` | Adopt observation/intent separation and explicit operation naming; do not claim all peers have four states |
| `INA228/include/INA228/INA228.h`, `Config.h` | `HardwareState`, selectable `HealthPolicy`, `pollJob` | Use separate configuration evidence; do not import offline admission policy or a second job vocabulary |
| `TMP1x2/include/TMP1x2/Config.h` | Borrowed callbacks, `Model`, pure `isValidModel` and `isValidAddress` | `Config`, exact `Variant`, `phyAddress`, typed settings and pure validation; no sensor address ranges or GPIO ownership |
| `TMP1x2/src/TMP1x2Operations.cpp` and public header | `startInitialize`, `startConfigure`, `startRecover`, `takeResult`, `cancel`, retained tokens | Small caller-driven operations; one selected vocabulary with PHY-specific cancellation and cleanup |
| `TMP1x2/include/TMP1x2/CommandTable.h` | `cmd::REG_*`, `MASK_*`, documented command-versus-state bits | Named datasheet register/mask constants with access attributes; no generic RMW on destructive fields |
| `TMP1x2/src/`, `examples/common/`, `CMakeLists.txt` | Responsibility-based translation units, one CLI, explicit sources and registered tests | Split when the code has a real responsibility; keep SDK, CLI and tests out of portable production sources |
| `TMP1x2/Doxyfile`, `tools/check_doxygen.py`, public headers | Source Doxygen, generated API checks, documented units and effects | Source documentation from the first implemented header, warning checks and actual generated-page inspection |

## Names and Status

- Namespace `DP8382x`, class `DP8382x`, include `<DP8382x/DP8382x.h>`, CMake
  target `DP8382x`. Use PascalCase for types and camelCase for methods, fields,
  arguments and locals. Private members use `_camelCase`. Scoped enum values,
  register addresses and masks use uppercase names. Python uses snake_case.
- Use descriptive units: `timeoutMs`, `observedAtMs`, `phyAddress`, `registerAddress`,
  `maxTransactions`. Do not call a multi-frame MDIO transaction a transfer in one
  layer and a frame in another. Document the chosen accounting unit.
- `Status` has `Err code`, `int32_t detail`, `const char* msg`. Messages have
  static lifetime. Keep `ok()`, `is(Err)`, `inProgress()`, explicit bool,
  `Status::Ok()`, `Status::Error(Err, const char*, int32_t = 0)` and
  `errorName(Err)`. `OK` alone is success; `IN_PROGRESS` means admitted/ongoing.
  Invalid enum values print `UNKNOWN`; never silently map them to success.
- Use `INVALID_PARAM` for an invalid call argument, `INVALID_CONFIG` for an
  inconsistent configuration, `NOT_BOUND`/`NOT_INITIALIZED` for distinct lifecycle
  failures, `UNSUPPORTED` for a source-proved absent capability, and
  `DEVICE_NOT_FOUND` for missing/plausibility-failed identity evidence under the
  prompt-7 rule. Use `DEVICE_MISMATCH` for a successfully read identity that
  conflicts with the declared part or accepted revision policy, with the reason
  retained in detail. A transport failure remains `MDIO_ERROR`/`MDIO_TIMEOUT`.
  No accepted identity is not proof of a physically absent chip. An unavailable
  fixture or unfinished feature is not unsupported silicon.
- Select `MDIO_ERROR` and `MDIO_TIMEOUT` for terminal transport failures,
  `OPERATION_TIMEOUT` for the whole-operation deadline, `CONFIG_MISMATCH` for
  verified stable-owned-bit disagreement, and `BUSY`, `CANCELLED`,
  `RESULT_NOT_AVAILABLE`, `TOKEN_MISMATCH` for operation state. This is a planned
  vocabulary, not permission to add unused codes. Every shipped code needs a
  reachable meaning, documentation and a test. Do not add a second ambiguous
  `TIMEOUT` alias or peer enum ordinals as a cross-library ABI.
- Preserve backend error detail without truncation. Define locally owned detail
  values by error code, so they cannot overwrite a controller error. Do not
  expose `esp_err_t` in core. Normalize illegal nonterminal callback returns at
  the transport boundary and preserve the original diagnostic evidence.

## Health, link and configuration

The selected passive health vocabulary is `DriverState::{UNINIT, READY,
DEGRADED, OFFLINE}`, printable through `driverStateName`. `OFFLINE` is repeated
managed MDIO failure evidence; it never blocks later attempts or starts recovery.
The application decides retry/backoff and availability. A disconnected cable is
not a management transport failure. `READY` does not prove link, IP reachability,
verified configuration or safe motion.

Use these peer field names in `HealthSnapshot` when their semantics apply:
`state`, `initialized`, `consecutiveFailures`, `totalFailures`, `totalSuccess`,
`lastOkMs`, `lastErrorMs`, `lastOkTimeValid`, `lastErrorTimeValid`, `lastError`.
Freeze widths, saturation, initialization and transitions in prompts 7-8. Timestamp
zero is valid when its validity flag says so. Last error survives a later success.
`resetStatistics()` clears documented cumulative counters only; it must not
repair configuration, erase pending results or declare recovery.

Separate these observations:

| Evidence | Planned interface | Required meaning |
| --- | --- | --- |
| Managed driver health | `driverState()`, `healthSnapshot()` | Cached; no I/O, retry or admission decision |
| Link/negotiation | `LinkSnapshot`, `linkSnapshot()` | Cached validity and time; distinguish link down, pending negotiation and invalid evidence |
| Live link read | `readLinkStatus(...)` | Fallible owner-driven acquisition; publish complete valid evidence only |
| Configuration | `getSettingsSnapshot()` | Desired settings, observed stable bits and trust/dirty evidence stay distinct |
| Operation | `getOperationSnapshot()` | Active/result-pending state and progress; does not advance or consume work |
| All bus activity | Separate transport diagnostics | Include raw/probe/SDK/cleanup frames without using them to claim managed recovery |
| PHY counters | Typed chip diagnostics | Datasheet clear/overflow semantics; not MDIO failure counters |

A successful cleanup or diagnostic probe cannot by itself restore managed health
or configuration trust. Specify which managed attempts count and how one atomic
MMD transaction reports its constituent frames; do not copy peer callback counts
without considering the selected transport boundary. Give read-clear/latch
evidence one collector. Cached views must not consume it again.

## Function and lifecycle vocabulary

Use one method for one meaning. Freeze exact arguments, result types and supported stages in prompts 7-8.

| Purpose | Planned names and constraints |
| --- | --- |
| Local binding | `bind`, `isBound`, `isInitialized`, `end`; no MDIO, reset, power-down or implicit recovery |
| Explicit initialization/reset/configuration | `startInitialize`, `startReset`, `startConfigure`; distinct documented effects, no `init`/`begin`/`startInit` aliases |
| Explicit replay/recovery | `startRecover`; application-admitted and bounded, restores only owned state |
| Operation progress | `pollOperation`, `takeResult`, `cancel`, `OperationToken`, `OperationResult`, `OperationSnapshot`; no parallel `poll`/`pollJob` surface |
| Register access | `readClause22`, `writeClause22`, `readMmd`, `writeMmd`; address spaces explicit, MMD path means the exact part's Clause 22 indirect mechanism |
| External state change | `invalidateDeviceState`; bus-silent, records lost trust without fabricating recovery |
| Pure helpers | `isValidVariant`, `isValidPhyAddress`, typed validators/decoders; no callbacks or hidden global state |

Use explicit reset/diagnostic options instead of an ambiguous Boolean overload.
Short reads/writes use direct bounded calls; do not wrap every field action in
token/result state. Cooperative operations serve actual scheduled waits or cleanup.
Use one active slot and monotonic nonzero tokens with explicit exhaustion rather
than a registry. Operation admission can span polls; the bus lock cannot span
wait intervals or console output. Share the underlying register procedures.
Add typed feature methods only as their behavior is implemented. A blocking
convenience must reuse the same procedure, have a proved aggregate bound and
serve an actual consumer. Do not build a second state machine for it. Do not
retain caller output buffers between polls; retained results belong to the
instance until consumed. Cancellation is local admission/state work; any hardware
restoration needs subsequent owner execution with a finite cleanup budget.

## File responsibilities

These are destinations for real code, not instructions to create empty files.
Keep a small implementation together until a split makes review or reuse easier.
Do not prescribe line-count limits or one file/class per register.

| Intended path | Content |
| --- | --- |
| `include/DP8382x/Status.h` | Error value, static messages and printable names |
| `include/DP8382x/Config.h` | Borrowed transport contract, exact variant and typed desired settings |
| `include/DP8382x/CommandTable.h` | Datasheet register/mask constants in `DP8382x::cmd`; add family-specific sections/files only when justified |
| `include/DP8382x/DP8382x.h` | Driver facade and used passive snapshots/operation values |
| `include/DP8382x/Version.h` | Generated release version only |
| `src/DP8382x.cpp` | Binding/lifetime, private state and common guards |
| `src/DP8382xRegisters.cpp` | Atomic transport/register procedures and side-effect rules |
| `src/DP8382xConfiguration.cpp` | Validated intent, configuration application and verification |
| `src/DP8382xOperations.cpp` | Bounded operation progression, deadline/cancel/result rules |
| `src/DP8382xDiagnostics.cpp` | Implemented BIST/cable/interrupt/power/LED procedures where a separate file helps |
| `include/DP8382x/esp_idf/`, `src/esp_idf/` | Native SDK adapter; explicit separate build selection |
| `examples/common/Dp8382xCli.h`, `.cpp`, `ConsoleBuffer.h` | One portable example CLI and bounded console; separate output/diagnostics only when needed |
| `test/` | Independent register/operation/CLI/runner/consumer cases, grouped by behavior |
| `scripts/generate_version.py`, `tools/` | Version sync/check and focused validation/HIL tools; no unnecessary build framework |

Private helpers stay in implementation or a private internal header when shared.
Do not expose an internal helper just to test it. Test through the smallest real
consumer. Use explicit source lists and keep tests, examples, SDK and archived
reference material out of portable compilation.

Use the public build-tree target for the early portable consumer. Prompts 69-70
add and verify relocatable install/export metadata and a separate installed
`find_package` consumer; use one documented public/import target contract.

## Readable code and source documentation

Use simple engineering English. Name the action, condition and consequence:
"A failed write may have reached the PHY; configuration remains unverified."
Avoid slogans, vague claims and comments that only repeat a function name. Prefer
straight-line steps, named masks, early validation and a small explicit switch
for real operation states. Extract a helper for a repeated rule or a named
responsibility, not for each line. Avoid generic managers, inheritance trees,
macro frameworks, registration systems and layers with no current consumer.

Every changed public declaration needs accurate Doxygen in the same commit:

- Files have `@file` and a useful `@brief`. Types explain purpose, ownership and
  invariants. Enum values and fields explain units, legal values and validity.
- Functions describe inputs/outputs, unchanged output on error where promised,
  state/admission requirements, returned errors and what constitutes progress.
  Use `@param[in]`, `@param[out]`, `@return`/`@retval`, `@pre`, `@note` or
  `@warning` where they convey real information; tags are not a quota.
- Callbacks document context/buffer lifetime, terminal behavior, serialization,
  re-entry restrictions and lock/controller time bounds. Snapshot comments say
  explicitly that they do no I/O.
- Hardware methods document addressed space, side effects, latch/read-clear
  consumption, uncertain writes, total deadline and cleanup. Cite exact TI
  revision/section/table for non-obvious masks, sequences and waits.
- Source files explain private multi-step procedures and why their order matters.
  Keep detailed public behavior authoritative in the header; do not duplicate it
  in a second prose block that can drift. Document Python protocols and limits
  in docstrings, and named test scenarios through clear names and short comments.

For example, a planned cached accessor can be documented as:

```cpp
/// @brief Return cached managed-MDIO health without accessing the PHY.
/// @return A value copy; invalid timestamp flags mean no recorded time.
/// @note READY does not imply link-up or verified hardware configuration.
HealthSnapshot healthSnapshot() const;
```

Implement Doxygen checking with the first source-bearing foundation. Run it
for changed declarations/comments and documentation inputs. Inspect generated
pages and configure source-coverage checks explicitly: `EXTRACT_ALL=YES` alone
does not establish comment coverage. Use a small temporary negative fixture to
prove that missing required documentation and invalid parameter references fail.
Keep vendor archives outside Doxygen input. Record a missing tool as NOT RUN
with the reproducible command; ordinary library builds remain independent of it.

## Documentation and review in every prompt

Each stage reviews `README.md`, `CHANGELOG.md`, relevant example/test/tool READMEs
and the maintained architecture/API/testing/variant/feature guides. Update the
ones affected by that work in the same checkpoint and summarize the review once.
Planning-only stages document decisions and contracts. Add source documentation
when the corresponding public declarations and behavior are implemented.

Before handoff, review for duplicated rules, ambiguous names, hidden I/O,
unnecessary abstractions, stale comments, undocumented outputs and unregistered
tests. Make the simplest change that still meets the full feature and failure
contracts. Do not reduce mandatory functionality to make the code look small.
Then apply COMMON's local verification, progress update and commit/sync rules.
