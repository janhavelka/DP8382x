# Current driver API, state and ownership comparison

Prompt 1 source audit, 2026-10-28. Paths in the source and test tables are
relative to `Projects/`. The sibling repositories and FieldCore-node were read
only. Their code and registered tests were inspected, not executed. This report
records observed behavior and decisions for the **planned** DP8382x library;
it is not evidence of an implemented driver, SDK link or physical PHY.

## Source identity and audit boundary

All listed worktrees were clean at inspection. Each was on its existing branch;
none was fetched or built for this audit. Their root `AGENTS.md` files and the
parent `Projects/AGENTS.md` were read before using their conventions. `rg --files`
located the cited current headers, sources and tests.

| Repository | Branch and exact HEAD | Inspected scope |
| --- | --- | --- |
| TMP1x2 | `main`, `bafc34067530eb340e55c42dae3fad4eb795b6e9` | `include/TMP1x2/`, `src/`, root `CMakeLists.txt`, registered core/jobs/bus tests |
| INA228 | `main`, `b28c5ea69bcb4d26b351c52c24d3c6fae9fd282b` | `include/INA228/`, `src/INA228.cpp`, native Unity tests |
| PCA9555 | `main`, `498178e4d9e4acb6fab7d8b816ad5021efe8806a` | `include/PCA9555/`, `src/PCA9555.cpp`, native Unity tests |
| TCA9548A | `main`, `ce425e1984ac269e7934dd811a06ec08b91e307c` | `include/TCA9548A/`, `src/TCA9548A.cpp`, driver tests |
| MB85RC | `main`, `1bf7c293549b1038b2d8f3b7a8d655fa74201e6e` | `include/MB85RC/`, `src/MB85RC.cpp`, native tests |
| BME280 | `main`, `d538f2e203ee26d6ddbc86f86d2b0cf4c73a6e98` | `include/BME280/`, `src/BME280.cpp`, native tests |
| SHT3x-main | `main`, `f65f15573e3c9b1ef04cce72ad08f307d021e563` | `include/SHT3x/`, `src/SHT3x.cpp`, native tests |
| FieldCore-node | `rodtester`, `5ad9292dc8481cf08be9a61f662495a6474d1da0` | INA228 module, owner contracts/adapters, composition and native module tests |

FieldCore-node's `config/build_profiles.json` pins INA228 `v3.0.4`; the local
annotated tag peels to `0b14044cac46eba07015a59640756b05bb82acc7`.
The current INA228 sibling HEAD above is **different**. FieldCore observations
below come from its current module source; current INA228 behavior is not
silently attributed to that pinned build. The direct module test includes current
module source, while a complete pinned dependency build is outside prompt 1.

## Status and transport evidence

| Live path / symbol | Observed source behavior | Decision and planned DP8382x destination |
| --- | --- | --- |
| `TMP1x2/include/TMP1x2/Status.h::Err`, `Status`, `errorName`; `PCA9555/include/PCA9555/Status.h` same symbols | Both have `Err : uint8_t`, `Status { Err code; int32_t detail; const char* msg; }`, `ok()`, `is()`, `inProgress()`, explicit bool, `Ok()` and `Error(Err, const char*, int32_t = 0)`. The constructor orders detail before message. Message storage is borrowed and must have static lifetime. Unknown enums print `UNKNOWN`. | **Adopt shape and spellings** in planned `include/DP8382x/Status.h`; document static message lifetime and preserve full signed backend detail. `OK` alone succeeds; `IN_PROGRESS` is admitted work. |
| Same `Err` lists; `PCA9555/src/PCA9555.cpp::_mapTransportResult`; `TMP1x2/src/TMP1x2.cpp::terminalTransportStatus` | TMP has sensor aliases, I2C NACK/bus codes and a generic `TIMEOUT`; PCA fixes its own ordinal ABI, retains reserved `OFFLINE`, distinguishes callback timeout, short-success protocol error and uncertain write effects. TMP normalizes callback `IN_PROGRESS` to terminal I2C error while retaining detail. | **Adapt**, not copy ordinals or I2C meanings. Planned `MDIO_ERROR`, `MDIO_TIMEOUT` and `OPERATION_TIMEOUT` distinguish transport from aggregate deadline. `DEVICE_NOT_FOUND` requires specified identity plausibility evidence; a controller error stays a controller error. Define local detail values per code without overwriting controller detail. Invalid terminal callback returns need normalization and retained diagnostics in `include/DP8382x/Config.h` and `src/DP8382xRegisters.cpp`. |
| `TMP1x2/include/TMP1x2/Config.h` callbacks; `PCA9555/include/PCA9555/Config.h::TransportResult` | Both borrow callback and context lifetime; PCA also exposes completed bytes and `WriteEffect`, and forbids retained buffers/reentry. A failed side-effecting callback can leave a write accepted or uncertain. Neither I2C byte counts nor address NACK prove a PHY's MDIO effects. | **Adopt borrowed, bounded terminal callbacks** and conservative effects in planned `Config.h`/register procedures. Application owns controller, callback lifetime and locking. Exact MDIO frame completion and effect contract belongs to prompts 7–8; do not import byte-count or NACK semantics. |

The Status shape is retained in [CONVENTIONS](prompts/CONVENTIONS.md). Its
selected error names are planned vocabulary; unused codes are not to be emitted
as empty placeholders.

## Passive health, link and configuration

| Live path / symbol | Observed source behavior and disagreement | Decision and planned DP8382x destination |
| --- | --- | --- |
| `TMP1x2/include/TMP1x2/TMP1x2.h::HealthSnapshot`, `DriverState`; `src/TMP1x2.cpp::track`, `healthSnapshot` | Four states `UNINIT/READY/DEGRADED/OFFLINE`. `track` counts selected physical transfer failures, saturates totals, retains last error on later success and uses explicit timestamp-validity flags. Configuration mismatch and invalid calls are not transport failures. Snapshot copies cache without I/O. OFFLINE is passive and does not gate access. | **Adopt four passive states** and applicable field names in planned `DP8382x.h`; freeze exact managed-attempt attribution, widths and threshold in prompts 7–8. Timestamp zero remains valid with a validity flag. `OFFLINE` never starts recovery or rejects an owner-requested attempt. |
| `PCA9555/include/PCA9555/PCA9555.h::DriverState`, `SettingsSnapshot`; `src/PCA9555.cpp::_updateHealth`, `_i2cWriteCleanupTracked`; `examples/common/HealthDiag.h::printHealthDiag` | Exactly three states `UNINIT/READY/DEGRADED`. Passive `bind` sets READY before physical presence. Raw probe is health-neutral. A failed input read remains visible after successful pointer cleanup; cleanup failure is counted. Its timestamp fields lack explicit validity flags; the example treats zero as never. | **Reject three-state limit and timestamp-zero sentinel**. Preserve primary error and separate cleanup result; cleanup success alone cannot restore managed health. Planned health and all-source MDIO diagnostics have separate accounting. READY says nothing about cable, configuration or app readiness. |
| `INA228/include/INA228/Config.h::HealthPolicy`; `src/INA228.cpp::_updateHealth`, `_ensureNormalI2cAllowed` | `PASSIVE` observes initialized transfer outcomes; legacy `LATCH_OFFLINE` can suppress ordinary access. `HardwareState` is separate from health. | **Adopt passive default only** and **reject LATCH_OFFLINE admission** for a new PHY core. Recovery and retry remain owner policy. Planned `healthSnapshot()` is cached; a successful cleanup or raw probe cannot fabricate managed recovery. |
| `TCA9548A/src/TCA9548A.cpp::begin`, `_updateHealth`, `getSettings`; `include/TCA9548A/TCA9548A.h` | `begin` performs a presence read; a failed read can leave a binding in UNINIT. Tracked success/failure changes four-state telemetry; raw probe is untracked. `getSettings` is cached. Totals survive `end()` for object lifetime. | **Adapt passive telemetry**, not its active `begin` or counter lifetime by default. DP8382x `bind` stays bus-silent and counters need a specified lifetime. Presence/identity is explicit managed work. |
| `MB85RC/src/MB85RC.cpp::bind`, `begin`, `end`, `_updateHealth`; `include/MB85RC/MB85RC.h` | Sleep state and transport health are separate. `bind` is local, `begin` uses the bus, and `end` resets health-session totals. This conflicts with TCA's object-lifetime totals. | **Adopt orthogonal state**: PHY power mode, transport health and link/configuration each have their own evidence. Freeze one DP8382x counter lifetime; do not infer health from sleep or link. |
| `BME280/include/BME280/BME280.h::SettingsSnapshot`, `ConfigSyncState`, `CalibrationState`; `src/BME280.cpp::_updateHealth`, `getSettings` | Cached settings, hardware configuration trust and calibration state are distinct. Raw/tracked wrappers differ; health counters saturate. A stale sample or failed resync is represented separately. | **Adapt separate trust/observation** in planned `getSettingsSnapshot()` and `LinkSnapshot`. Successful unrelated MDIO does not validate owned settings. |
| `SHT3x-main/src/SHT3x.cpp::_updateHealth`, `getSettings`, `readSettings`; `include/SHT3x/SHT3x.h` | Physical callback success and logical operation completion have separate counts. CRC/protocol failures can follow successful transport. `getSettings` is cached; `readSettings` may do I/O. | **Adapt explicit accounting unit**. An MMD selector frame is not a successful target read. Count complete managed transactions and physical frames separately. Name live acquisition separately from cached health/link/configuration views. |

For the PHY, link-down is a valid link observation, not an MDIO failure. A
transport-successful read can still yield unresolved negotiation or invalid
configuration evidence. Application readiness includes policy and higher layers
outside the driver. Destructive latch/read-clear evidence has one collector;
cached views never consume it.

## Lifecycle, operations, effects and bus ownership

| Live path / symbol | Observed source behavior | Decision and planned DP8382x destination |
| --- | --- | --- |
| `TMP1x2/src/TMP1x2.cpp::bind`, `begin`, `end`, `unbind` | `bind` is local but an invalid rebind first clears the prior binding. `begin` calls bind then active recovery; `end` is local deinitialization that retains binding, configuration and dirty evidence. `unbind` releases them. | **Select explicit meanings**: planned `bind` validates before commit and performs no MDIO; `end` is bus-silent local teardown subject to pending effect/result rules. Use `startInitialize`/`startRecover` for hardware. No ambiguous `begin` alias. Exact teardown contract belongs to prompt 8. |
| `PCA9555/src/PCA9555.cpp::bind`, `detach`; `include/PCA9555/PCA9555.h::begin`, `end` | `bind` is passive and validates before replacing state; `begin` aliases it. `end` aliases bus-silent detach. Detach retains cancellation result but refuses active required pointer cleanup. | **Adopt validation-before-rebind and owed-cleanup guard**. A PHY instance must retain pending/uncertain effects until the owner can complete or explicitly acknowledge finite cleanup. No hardware action is hidden in teardown. |
| `TMP1x2/src/TMP1x2Operations.cpp::startInitialize`, `poll`, `takeResult`; `PCA9555/src/PCA9555.cpp::pollOperation` | TMP admission is local, then bounded polls perform work; token/result is retained until matching `takeResult`. PCA polls with a transaction budget and retains primary/cleanup status, partial masks, write effect and cleanup-required flag. Names differ, and a zero budget performs no transfer. | **Adopt one vocabulary**: planned `startInitialize`/`startReset`/`startConfigure`/`startRecover`, `pollOperation`, `takeResult`, `cancel`. Use one active slot and retained result with exact token. Polling owns finite work; cancellation can mark a request locally, while hardware restoration needs separate finite owner execution. Short operations use direct bounded calls. |
| `INA228/src/INA228.cpp::bind`, `startInitialize`, `pollJob`, `takeJobResult`, `_cancelJob`, `SAMPLE_RESTORE_ADC` | Bind and job admission are bus-silent. Cancellation/timeout preserve `NONE/PARTIAL/INDETERMINATE` effects and dirty hardware state without cleanup I/O. A normal diagnostic path restores ADC configuration and verifies it. INA uses a different job vocabulary. | **Adapt retained effect and finite restore**, **reject parallel `pollJob` names**. PHY post-write waits need completion-based anchors and aggregate bounds; cancel/teardown cannot erase accepted or uncertain effects. Planned `DP8382xOperations.cpp` holds this procedure, with actual feature cleanup defined when implemented. |
| `SHT3x-main/src/SHT3x.cpp::bind`, `begin`, `pollJob`, `end` | Passive bind and active begin differ; request/cancel can be local while poll progresses I/O. Begin's name alone is not evidence of bus silence. | **Keep explicit PHY lifecycle names**; each public method must say whether it acquires the bus, stores a result or only changes local state. |
| `PCA9555/include/PCA9555/Config.h`, `TMP1x2/include/TMP1x2/Config.h`; their tracked/raw wrappers | They borrow application transport and do not supply a controller-wide lock. A callback can serialize one I2C transfer, but neither peer proves a complete multi-frame indirect MDIO or read/modify/write lock. | **Extend ownership for MDIO**: one application owner serializes every instance call, including snapshots; one controller-wide mechanism covers each complete indirect or RMW transaction across core, SDK base, ioctl, CLI and other participants. Release the short lock before waits/output. Guard raw selector and read-clear access through the same owner/collector. Planned home: `Config.h`, `DP8382xRegisters.cpp`, SDK adapter and owner examples. |

`PCA9555/include/PCA9555/PCA9555.h::RegisterImage` is desired writable
state; `ObservedState` carries independently valid/mismatched/uncertain pairs
and observation time. `src/PCA9555.cpp::readObservedState` returns only the
current call's partial observations after failure, while driver-wide write
uncertainty survives. `INA228/include/INA228/INA228.h::HardwareState` is
`UNBOUND/UNKNOWN/SYNCHRONIZED/RESYNC_REQUIRED`; its job failure paths preserve
touched registers and unresolved effect. **Adapt these evidence boundaries**:
planned `getSettingsSnapshot()` separates desired persistent settings, observed
stable bits, trust/dirty cause and timestamp. A write that failed or completed
late may have reached hardware. Diagnostic temporary settings remain temporary;
restoration cannot silently replace persistent intent. Exact field masks and
replay order require silicon prompts 3–5 and operation contracts 7–8.

## FieldCore-node: application owner boundary

The current `FieldCore-node/src/devices/i2c/Ina228Module.h/.cpp` holds a private
INA228 object and injected owner transport. It is an application wrapper, not
an Ethernet module or DP8382x dependency.

| Live path / symbol | Observed behavior | DP8382x responsibility decision |
| --- | --- | --- |
| `src/devices/i2c/Ina228Module.cpp::bind`, `prepareLibraryConfig`; `include/TunnelMonitor/i2c/I2cOwnerTransport.h` | Validates a fixed product profile and borrowed transport, requires `HealthPolicy::PASSIVE`, then wires library callbacks/time context. Same owner rebind is idempotent; different owner is busy. No bus access. | Core validates portable transport/settings. Application chooses board address, clocks, profile, callback context and owner lifetime. |
| `Ina228Module.cpp::start`, `startInitialJob`, `poll`, `takeResult` | Start admits a copied command/deadline without I/O. Poll probes address, selects initialize/reinitialize/verify from hardware trust, grants one library transfer per poll, checks deadline before and after completion, and returns `NeedsBusRecovery` to owner. Module result is copied and consumed once. | Core owns chip procedure and retained operation evidence; application owns admission, scheduling, recovery policy and publication. Whole indirect MDIO transaction must be indivisible despite an I2C peer's one-transfer cadence. |
| `Ina228Module.cpp::i2cWrite`, `i2cWriteRead`, `mapLibraryStatus`, `mapJobEffect`; `src/i2c/I2cTask.cpp::transferOnce`, `physicalTransferOnce` | Adapter forwards one bounded transfer under original owner deadline, retains completion time and translates backend status; mapping distinguishes identity-phase absence, transport timeout, operation deadline and partial effect. Owner clips each transfer timeout to the original deadline and owns controller recovery. | MDIO adapter must preserve original deadline and backend detail across SDK calls, report late/uncertain effects and keep SDK state coherent. Product-specific status translation remains outside the portable core. |
| `Ina228Module.cpp::refreshStatus`, `snapshot`, `copyStatus`; `src/i2c/I2cTask.cpp::refreshBindingStatusCaches`; `src/product/tunnelmonitor/TunnelMonitorRuntimeComposition.cpp::publishI2cTypedStatuses`, `copyPowerStatus` | Refresh ages cached status using supplied time and clears stale value validity without bus I/O. Snapshot/copyStatus return cached copies; composition publishes a typed copy for consumers. | Planned `healthSnapshot`, `linkSnapshot` and settings snapshot are no-I/O; application publishes synchronized copies and decides stale/application readiness policy. |
| `include/TunnelMonitor/i2c/I2cDeviceBinding.h::makeI2cDeviceBinding`; `src/product/tunnelmonitor/TunnelMonitorRuntimeComposition.cpp`, `src/product/co2control/Co2ControlRuntimeComposition.cpp` | Adapters route bind/start/poll/takeResult/cancel/invalidation/refresh/snapshot to the module; both compositions create actual power-module bindings. | A future application owner may wrap DP8382x this way. Do not import TunnelMonitor contracts, product profile or task topology into this library. |

## Representative tests and actual registration

These tests exercise the conventions above. Registration was checked in the
test entry point and its configured native path; finding a test filename alone
would not establish that it is runnable. No sibling test was executed here.

| Repository | Exercising case and registration evidence |
| --- | --- |
| TMP1x2 | `test/test_core.cpp::lifecycle`, `health`, `dirtyRecovery`, `protocolAndClockGuards` are in its called `main` table; `test/jobs/test_jobs.cpp::budgetsAndHealth` is in its called table. Root `CMakeLists.txt` explicitly lists `src/TMP1x2.cpp`, `TMP1x2Configuration.cpp`, `TMP1x2Measurement.cpp`, `TMP1x2Operations.cpp`, `BusOperations.cpp`, and registers `core`, `jobs`, `bus` plus other real CTest targets. |
| PCA9555 | `test/test_basic.cpp::test_status_and_typed_value_helpers`, `test_bind_and_begin_are_passive_for_all_valid_addresses`, `test_read_observed_state_failure_returns_only_current_partial_evidence`, `test_input_read_failures_survive_successful_cleanup_in_health_and_validity`, `test_detach_refuses_to_abandon_required_pointer_cleanup` have `RUN_TEST` calls in `main`. `platformio.ini` `[env:native]` builds source; `.github/workflows/ci.yml` invokes `pio test -e native`. Root CMake registers only an IDF component, not these host tests. |
| INA228 | `test/test_basic.cpp::test_cooperative_bind_is_zero_i2c_and_validates_contract`, `test_cancel_and_timeout_are_bus_silent_with_precise_effects`, `test_passive_health_never_suppresses_owner_requested_transport`, `test_wait_origin_newer_than_caller_timestamp_does_not_skip_wait` have `RUN_TEST` calls. `platformio.ini` native environment and CI `pio test -e native` select them. |
| TCA9548A | `test/test_driver/test_driver.cpp::test_failures_before_initialization_keep_uninit` and `test_settings_snapshot_is_io_free_and_truthful` have `RUN_TEST` calls; `platformio.ini` native `test_filter = test_driver` and CI native test command select the suite. |
| MB85RC | `test/test_basic.cpp::test_begin_success_sets_ready_and_health` has a `RUN_TEST` call; `platformio.ini` native environment and CI `pio test -e native` select it. `test/test_public_contracts.cpp` has compile-time assertions, which are not a separate runtime result. |
| BME280 | `test/test_basic.cpp::test_cancel_result_is_zero_i2c_exactly_once_and_blocks_until_retrieved` and `test_invalidate_device_state_is_zero_i2c_and_recovery_reloads_state` have `RUN_TEST` calls; `platformio.ini` native environment and CI `pio test -e native` select them. |
| SHT3x-main | `test/test_basic.cpp::test_bind_rebind_and_end_are_zero_i2c_local_operations`, `test_probe_status_crc_is_counter_neutral`, `test_observe_only_health_continues_i2c_and_counters_saturate`, `test_end_clears_runtime_state` have `RUN_TEST` calls; `platformio.ini` native environment and CI `pio test -e native` select them. |
| FieldCore-node | `test/native/test_component_ina228/test_ina228_module.cpp` cases for staged atomic sample, identity/effect/stale failure, deadline/cancel/recovery and two instances are invoked through `suite_main.cpp`'s `TUNNELMONITOR_NATIVE_RUN_CASE` (`RUN_TEST`) calls. `config/native_suite_links.json` selects `component:ina228`/`test_component_ina228`; `scripts/verify.py` runs selected native suites and checks registration. |

## Source boundaries and next contracts

TMP1x2's explicit CMake source list demonstrates useful responsibility
boundaries: `TMP1x2.cpp` owns lifetime/transport/health; configuration,
measurement and operations have separate procedures; `BusOperations.cpp`
handles bus-level helpers. **Adapt this split as behavior appears** in planned
`src/DP8382x.cpp`, `DP8382xRegisters.cpp`, `DP8382xConfiguration.cpp` and
`DP8382xOperations.cpp`; separate diagnostics or SDK files only for real
responsibilities. No class-per-register structure or empty scaffolding follows.

Future public declarations need Doxygen at introduction: file and type purpose,
units/validity, legal states, borrowed context/buffer lifetime, synchronization,
I/O and read-clear side effects, output validity on failure, terminal errors,
aggregate bounds, cancellation/cleanup and exact TI section for non-obvious
sequences. Private multi-step source comments should explain order. Prompt 9
establishes source-documentation checking; this audit adds no fake header or
unregistered test.

Prompt 2 owns the broader examples/CLI, Python tooling, packaging and workflow
inventory. Prompt 6 owns the operation-by-operation portable/SDK ownership
table; prompts 7–8 freeze exact API signatures and time/transaction contracts.
The present comparison closes only prompt 1's source, API, state and ownership
decision scope.
