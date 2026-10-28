# Current driver API, state, ownership and tooling comparison

Prompt 5's [fresh source review](reports/implementation/5-coverage-source-review.md)
reopens TMP1x2 and MB85RC model/capability declarations, actual rejection paths,
CLI/runner contracts and TUSB32x planning at their recorded current revisions.
Its PHY adaptations retain declared part versus readable identity, capability
rejection before I/O and separate evidence layers. This supplements the broader
prompt-1/2 comparison below without claiming new sibling test runs.

Prompts 1–2 source audits, 2026-10-28. Paths in the source and test tables are
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

Passive admission does not imply the same state transition in each peer:
`INA228` with `HealthPolicy::PASSIVE` remains `DEGRADED` after repeated failures,
while `SHT3x-main` with `HealthPolicy::OBSERVE_ONLY` can report `OFFLINE` and
still permits another owner-requested transfer. Their registered passive-health
tests exercise these different outcomes. DP8382x selects passive four-state
telemetry without using `OFFLINE` to gate access.

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
The section above closes prompt 1's source, API, state and ownership decisions.
The following prompt 2 audit extends the same comparison to tooling and consumers.

## Prompt 2 source identity and depth

The prompt 1 revisions above were rechecked unchanged and clean for TMP1x2,
INA228, PCA9555, TCA9548A and SHT3x-main. The additional deep sources were:

| Repository | Branch, exact HEAD and state | Scope |
| --- | --- | --- |
| 24Cxx | `main`, `936aa85e21b40b61c6f9cf0306d64c3beb6eaf6e`, clean | Shared CLI, runner, CMake, tests, package workflow |
| SCD41 | `main`, `c0bf1bba282d12b4aa3196d9f861915fe9b22551`, clean | Extracted-package consumer checks and workflow |
| MB85RC | `main`, `1bf7c293549b1038b2d8f3b7a8d655fa74201e6e`, clean | Metadata/package checkers and workflow; core covered above |
| FieldCore-node | `rodtester`, `5ad9292dc8481cf08be9a61f662495a6474d1da0`, clean | Prompt organization, guidelines, implementation plan and verifier |
| TUSB32x | `promptset/tusb32x-implementation-20261023`, `7f23b901b97325fa3c49f3a1e4f26bd05d4231f0`, **dirty** | Promptset/progress/AGENTS and selected adapter contract only |

TUSB32x had modified `examples/arduino/BoardConfig.h`, `ExampleApp.cpp`,
`Tusb32xWire.cpp/.h`, `examples/esp_idf/Tusb32xIdf.cpp/.h` and
`test/arduino_wire/test_wire.cpp`, plus untracked `test/arduino_app/` at
inspection. Its HEAD is not a complete description of those working files;
this audit does not use them as a released implementation baseline. Applicable
local `AGENTS.md` files were read. The repositories remained read-only; no
sibling fetch, build, test or remote CI run was performed.

## Shared CLI and owner routing

| Actual source and exercising check | Observed behavior / incompatibility | Decision and planned owner |
| --- | --- | --- |
| `TMP1x2/examples/common/Tmp1x2Cli.h::Cli`; `Tmp1x2Cli.cpp::feed`, `processCommand`; `test/cli/test_cli.cpp` through root CMake `add_test(NAME cli ...)` | One instance-based processor serves examples. `feed` bounds a 160-byte line, handles CR/LF and backspace, discards overflow/control-corrupted lines. `processCommand` caps five tokens and checks complete decimal/hex/finite values. The compiled CLI test calls both paths and checks invalid/cached commands against transfer counts. | **Adopt** one portable example dispatcher in `examples/common/Dp8382xCli.*`, strict whole-token parsing and zero-I/O rejection for malformed input. Application owns console, clock and command admission. Prompt 43 freezes exact grammar/capacities; do not copy sensor command meanings. |
| `TMP1x2/examples/common/Tmp1x2Cli.cpp::setup`; `Tmp1x2CliDiagnostics.cpp::startOperation`, `pollOperation`, `finishOperation`, `tick`; registered `test/cli/test_cli.cpp` | Setup makes no immediate bus call but admits initialization for a later tick. Tick usually grants one transfer, keeps manual conversion pending after a stopped watch, and `takeResult` moves a matched terminal result into CLI cache. A method's immediate bus silence does not mean attaching the CLI is inert over time. | **Adapt** cooperative scheduling and correlated results; **reject** automatic initialization on DP CLI attach. A P4 or portable application explicitly admits initialization and maintenance. CLI attachment is local, and output backpressure must not consume terminal evidence. |
| `TMP1x2/examples/common/ConsoleBuffer.h::format`, `append`, `drain`; registered `test/console/test_console.cpp` | Fixed formatting and queue, one bounded backend write per drain, whole-record drop with counters/loss marker. Its sent count means backend acceptance, not host receipt. The queue has no reserved terminal-result capacity. | **Adapt** bounded output and truthful counters in planned `examples/common/ConsoleBuffer.h`; reserve or retain terminal evidence until emission, so a stalled console cannot silently lose a result. Do not hold the MDIO lock during output. |
| `24Cxx/examples/common/Eeprom24CxxCli.cpp::feed`, `processCommand`, `beginSession`, `finishSession`, `tick`; `test/test_cli/`, `test/cli_host/main.cpp`, root CTest `cli`/`cli_transcript` | One shared CLI uses canonical aliases, strict bounded direct-call scan, 34-token cap, session admission/result records and output-ready backpressure. CTest compiles the dispatcher and runs a transcript against a host CLI executable. | **Adopt** command catalogue/help agreement, explicit accepted versus terminal records and a compiled host transcript fixture. **Adapt** session capacity and routing to PHY owner; EEPROM scratch/write-protect and ACK-poll policy do not transfer. |
| `SHT3x-main/examples/common/Sht3xCli.cpp::CliString::assign`, `processCommand`, `tick`; `tools/check_cli_contract.py`, `test_cli_health_rate.py` | Namespace-global CLI uses whole-token numeric helpers and one-transfer tick with result correlation, but direct input is silently truncated at 159 bytes. The cited tools inspect source/help or extracted expressions; no inspected `test/` harness executes the shared dispatcher. | **Reject** silent truncation and global mutable CLI state. Keep explicit overflow rejection, instance ownership and executing dispatcher tests. Source-pattern checks alone cannot prove command behavior. |

Aliases such as `help/?`, `version/ver` and `drv/health` can be familiar where
their meanings agree, but a sensor `begin`, I2C scan, EEPROM scratch operation or
global CLI object cannot be copied as PHY behavior. The CLI reads cached health,
link and configuration without MDIO; explicit live commands route through the
application owner and the register/event access policy in [CLI_CONTRACT](prompts/CLI_CONTRACT.md).

## Python HIL protocol, verdict and artifact comparisons

| Actual source and exercising check | Observed behavior / limit | Decision and planned owner |
| --- | --- | --- |
| `INA228/tools/run_i2c_hil.py::hilrun_end_re`, `read_until_hilrun_end`, `strip_hilrun_frame`, `run_step`, `run_serial`; `tools/test_run_i2c_hil_parser.py::main`, CI parser commands | `hilrun` uses a fresh token and increasing sequence; its parser finds a matching begin-token substring followed by a matching newline-terminated `HIL_END`. Tests reject wrong token/sequence, truncated frames, stale end-before-begin and expected-rejection false passes, and check unexpected trailing output. The parser does not require a line-anchored begin or unique begin/end pair. Firmware version/commit/status/framework are checked. One `read(in_waiting)` can exceed the configured byte limit before checking it, drain has no byte cap, and UNKNOWN can exit zero unless `--fail-on-unknown`. | **Adapt** token/sequence correlation, identity and failure precedence; require anchored, uniquely matched begin/end records, pre-read byte caps and non-PASS exit on missing required evidence in `tools/dp8382x_hil_runner.py`. A new boot/session schema and bounded close/finalization are required by the DP contract. |
| `24Cxx/tools/hil_runner.py::Session`, `terminal_evidence`; `tools/hil_identity.py`; root CTest `hil_runner` and compiled `cli_transcript` | Runner checks matched accepted/completed session records, reboot/stale/duplicate evidence and short-write framing loss; it avoids more commands after uncertain framing. Exclusive run directories and close failures are retained. | **Adapt** session monotonicity, startup marker, partial transcript retention and primary/cleanup separation. Add DP-specific PHY identity, feature, fixture and required manifest/case agreement. |
| `SHT3x-main/tools/run_sht3x_hil.py::parse_command_output`, `classify`, `run_serial`; CI parser test | Validates required firmware/result fields and short writes, but completion uses expected text, prompt and idle timing rather than unique command/session records. | **Adapt** field validation only; **reject** prompt/idle as sufficient terminal correlation for DP diagnostics and reset. |
| `PCA9555/tools/run_i2c_hil.py::run_serial_command`, `final_verdict`; CI `test_run_i2c_hil_parser.py` | It distinguishes review-required and dry-run evidence from physical success, but its serial command path ignores `write()` count and calls `flush()` without a proved deadline. Completion is mainly prompt/text based. | **Adopt** honest review/physical classification; **reject** unbounded flush and unchecked partial write. DP runner must retire the session on uncertain framing. |
| `TCA9548A/tools/tca9548a_hil.py::SerialRunner`, `prepare_artifacts`, `write_bundle`; `tools/test_tca9548a_hil.py`, `test_hil_artifacts.py` registered in CI | Serial path checks full writes, caps each read by remaining response capacity, rejects stale/prompt/reboot ambiguity and retires a lost session. Artifact paths, including an empty manifest, are reserved before the run. The manifest content is written and closed after the other payloads with byte counts and hashes; write/close failure prevents a parseable PASS manifest. | **Adapt** bounded transport and final manifest content; DP will write its manifest last as `CLI_CONTRACT` requires, then check required members/schema and case-verdict agreement. Fixture tests need identified authorization and exact firmware/board conditions. |

These protocols are incompatible: INA228 uses token/sequence frames, 24Cxx uses
accepted/result session records, SHT3x and PCA9555 rely more on prompt/text
completion, and TCA9548A uses a synchronized marker and finite serial session.
DP8382x will define one own protocol in prompt 43 and one runner entry point in
`tools/dp8382x_hil_runner.py`; no peer transcript is treated as PHY evidence.

## Build, version, documentation and package checks

| Actual source / registration | What is checked, and what is not | Decision and planned DP8382x owner |
| --- | --- | --- |
| `TMP1x2/CMakeLists.txt` `TMP1X2_SOURCES`, `add_test`; `.github/workflows/ci.yml` | Explicit production sources, tests OFF for consumers, eight C++ CTest executables and four Python checks (version, IDF contract, Doxygen checker, HIL runner). CI configures/builds/runs host tests and checks plus package and S2/S3 Arduino/IDF images. This source matrix does not prove ESP32-P4 MAC/PHY support. | **Adopt** explicit sources, registered executing tests and separate evidence layers. Planned root CMake will build a framework-free C++17 target, with real host tests when code exists; native P4 adapter must link under a selected IDF SDK, not inherit sensor target names. |
| `TMP1x2/scripts/generate_version.py::main`; `library.json`, `include/TMP1x2/Version.h`, `idf_component.yml`, `CMakeLists.txt`, `Doxyfile`; `tools/test_version_sync.py` through CTest | `library.json` drives reproducible version outputs. The script's module-level `main([])` runs on import/PlatformIO hook, while direct CLI execution exits through its `__main__` call first. Import-time mutation is intentional there but is a surprising contract for an ordinary generator. | **Adapt** one explicit, deterministic `scripts/generate_version.py` with nonmutating check mode, `library.json` as release-version truth and separate firmware Git identity. Consumers must build from packaged generated files without Python/Git. Do not copy import-side effects or hard-code peer release numbers. |
| `TMP1x2/Doxyfile`, `tools/check_doxygen.py`, `tools/test_check_doxygen.py`; registered CTest `doxygen_checker` | `EXTRACT_ALL` exposes declarations but does not prove meaningful comments. The custom checker runs bounded Doxygen and checks warnings, required input/public XML types and generated HTML links/assets; its own tests exercise checker failures. | **Adapt** a source-document checker from the first real public header in prompt 9. Check warnings, required public comment coverage and negative fixtures; inspect representative generated pages. No Doxyfile is created for this evidence-only stage. |
| `24Cxx/CMakeLists.txt` and `.github/workflows/ci.yml` | CTest registers core, CLI, transport, helpers, diagnostics, detection, console, Python HIL discovery and a compiled CLI transcript. Workflow packages, validates, extracts and CMake-builds the extracted library. The existing `test/core_no_arduino/` is **not** registered, and extracted build does not compile/link a separate consumer executable. | **Adopt** actual CLI/runner registration; **reject** treating a package library build or unregistered source as consumer proof. DP installed/exported `find_package` and relocated consumers need their own linked and executed cases in prompts 69–70. |
| `SCD41/tools/check_clean_consumer_compile.py`, `check_target_package_consumer.py`; `.github/workflows/ci.yml` package steps | The clean consumer checker compiles/links/runs a zero-I/O C++ app against source or extracted archive. Target checker builds copied-package native and S2/S3 projects; only native executes. | **Adapt** clean extracted and relocated consumer proof, plus actual SDK link for DP. Native run does not establish target execution or a physical PHY. |
| `MB85RC/tools/check_package_contents.py`, `check_metadata_consistency.py`; `.github/workflows/ci.yml` | Package checker validates required/forbidden archive paths, SemVer and Markdown links; metadata checker compares library version and capabilities with generated files/readme. The inspected package job does not link a consumer. | **Adopt** allowlisted contents, required licenses/links and version consistency; **add** a real DP consumer build/run. Reference PDFs/ZIPs and HIL logs stay outside release archives. |
| `PCA9555/tools/check_package.py`, `package_consumer.cpp`; `TCA9548A/tools/check_package.py`, `test/core_no_arduino/CMakeLists.txt`; their CI workflows | Both provide package and native checks with different registration layouts. TCA additionally invokes a standalone CMake consumer and strict-warning direct compile/run. PCA's host tests are PlatformIO Unity, not root CTest. | **Adapt** the useful portability/package gates, inspecting the actual registered command and artifact. No one sibling workflow is a DP8382x CI matrix. |

No current DP8382x `CMakeLists.txt`, `library.json`, `idf_component.yml`,
`Doxyfile`, `platformio.ini` or `.github/workflows/` exists. Its only executable
checker is `tools/verify_references.py`: it verifies provenance-required fields,
TI URL host, size/SHA-256, PDF signatures, ZIP member inventory/CRC, derived
text hashes and repository-local Markdown links. It does **not** verify online
document freshness, chip interpretation, C++ behavior, SDK link or hardware.
`test/README.md` and root `README.md` register
`python tools/verify_references.py` and `git diff --check` as current commands.
Prompt 2 inspected workflow *source* in siblings; no remote badge, run or
green status is adopted as verification of this repository.

## Native example resource ownership

| Live example implementation | Observed resource/cleanup behavior | Planned DP8382x decision |
| --- | --- | --- |
| `TMP1x2/examples/esp_idf/basic/main/main.cpp::app_main` | Application creates I2C bus/device handles and attempts to remove acquired handles after setup failures; normal path enters its service loop. Some removal outcomes are not retained in the example's reported status. | **Adopt** application-owned controller setup, **strengthen** partial cleanup reporting and callback quiescence in P4 examples. |
| `INA228/examples/esp_idf/basic/main/Ina228IdfI2cTransport.cpp::removePendingTemporaryDevice`, `init`, `deinit` | Temporary device removal failure retains its handle for retry; failed bus deletion retains the bus. | **Adopt** resource ownership ledger and separate primary/cleanup outcomes. Translate only after checking native Ethernet SDK lifecycle semantics. |
| `PCA9555/examples/espidf_basic/main/main.cpp::ensureDevice`, `initBus` | `ensureDevice` ignores remove result and clears a handle; write mapping conservatively reports possible commit. | **Reject** lost-handle cleanup; **adopt** uncertain write reporting. The IDF PHY adapter must preserve resources and affected SDK state on partial failure. |
| `TCA9548A/examples/espidf_basic/main/main.cpp::prepareAddress`, bus setup | Address replacement is registered before retiring the old handle. Failed old-handle removal attempts replacement cleanup; if that cleanup also fails, it retains the replacement and faults registration. | **Adapt** partial-state accounting, not I2C address-handle behavior. ESP32-P4 MAC, mediator and PHY state have their own lifecycle and must be audited in prompt 6/37. |

These are I2C examples. Their Arduino and ESP32-S2/S3 build matrices neither
provide a DP83826I driver nor establish an ESP32-P4 Ethernet backend. The
planned ordinary P4 Ethernet example must link against the selected SDK and
reuse its MAC/PHY framework; the application owns board pins, clocks, address,
reset, network policy and teardown. An EtherCAT owner example demonstrates a
MAC frame/maintenance boundary, not an EtherCAT stack or servo cycle.

## Prompt workflow and delegation precedents

| Current source | Observed scope and limitation | DP8382x decision |
| --- | --- | --- |
| `FieldCore-node/AGENTS.md`, `docs/repository_map.md`, `docs/guidelines/implementation_plan.md`, `docs/prompts/README.md`, `scripts/verify.py` | Repository map links owners/contracts/tests; guidelines own architecture, implementation plan owns order/status and reports retain history. RodTester host/software work is distinct from target and physical gates. Deferred prompt presence does not dispatch work. Focused verification and a larger verifier are separate. | **Adopt** explicit prerequisites, path ownership, local checks and completion evidence in `docs/prompts/`, `docs/implementation-progress.md` and reports. **Reject** importing product tasks/contracts or assuming its board/fixture authorization. |
| `TUSB32x/docs/prompts/README.md`, `COMMON.md`, `25-idf-transport.md`, `docs/implementation-progress.md` at the dirty revision above | 38-stage promptset names source audits, owners, verification and handoffs. Progress records 1–25 complete with stated SDK-link limits and 26–38 unstarted; physical qualification is separate. Its current adapter worktree is dirty, so prompt prose is not proof of an immutable adapter implementation. | **Adapt** precise dependency/owner/deadline handoffs; **reject** treating planned CLI/HIL or dirty adapter source as completed DP behavior. Keep this execution's branch, checkpoints and first-set CI rule from DP8382x instructions, not sibling permissions. |

Read-only reviewers handled disjoint CLI/runner, mechanical build/package and
owner/prompt-inventory questions. The lead reconciles the contracts and owns
this comparison, architecture, progress, staging, commits and sync. No sibling
push, merge, fixture action or publication authority is inherited. This
repository's configured `origin` is
`https://github.com/janhavelka/DP8382x.git`; local Git author is Jan Havelka.
There is no `CODEOWNERS` or `CONTRIBUTING.md` in this repository at this audit.

## Broader Projects inventory (path discovery only)

`rg --files` located the following repository paths. This is an inventory of
present source/layout, **not** a behavioral or test audit of these drivers. All
listed Git worktrees were clean at inspection. Exact HEADs are included to make
the limited observation reproducible; no source behavior is attributed to a
filename alone.

| Repository and exact HEAD | Located source boundary / scope limit |
| --- | --- |
| ADS1115 `235af3e8c722661a89a44766a4ccd903a654f6fe` | `include/ADS1115/`, `src/ADS1115.cpp`; inventory only |
| INA3221 `41b6184b75f3c152611d4236f7ea41a5ce77bc2c` | `include/INA3221/`, `src/INA3221.cpp`; inventory only |
| LDC1614 `1db38b63cb93a561b6e0bea51017ab94102ebd24` | `include/LDC1614/`, `src/LDC1614.cpp`; inventory only |
| LSM6DS3TR `362b140990bc4ba2bc7f32cef50f8d9f30b4d3cc` | `include/LSM6DS3TR/`, `src/LSM6DS3TR.cpp`; repository is `LSM6DS3TR`, chip label includes `-C` |
| OPT4001 `dab818e575fc25a0949d962bc0df600eb2ed5200` | `include/OPT4001/`, `src/OPT4001.cpp`; inventory only |
| RV3032-C7 `aabd8d9204e75072403fefb60cf8d2679a7f56de` | `include/RV3032/`, `src/RV3032.cpp`; repository and include directory differ |
| MCP45HVX1 `c6c7feefd79b9fc16ddc7b7786ad187e5185fde6` | `include/MCP45HVX1/`, `src/MCP45HVX1.cpp`; inventory only |
| SSD1315 `9586b61363b2b70966f84e218f786bcfe9906e76` | `include/ssd1315/`, compatibility `include/SSD1315.h`, `src/SSD1315.cpp`; casing differs |
| MAX31865 `eb44f3463328a0a893b82102fcdfee4f7f9a39d1` | `include/MAX31865/Transport.h`, `src/MAX31865_Protocol.cpp`, `src/platform/arduino/`; sampled contract below |
| ADS1261_ESP32 `c4420ae197ce7da7c884c6e5b39acba133fa13f4` | `include/ADS1261_ESP32/Transport.h`, `src/ADS1261_Protocol.cpp`, `src/platform/arduino/`; sampled contract below |
| MotorControl-RS `bd831c859daa9c8c07b8d27a2dd879bb4e750511` | `include/MotorControlRS/`, multiple `src/` units; other-protocol inventory only |
| MotorControl-CAN `aa8a3bc2613843174ec25eab195f27ebcb596fcf` | No current `include/`, `src/`, `examples/` or `test/` located; planned repository, no implemented behavior inferred |
| VTN4xx `c825c125b87b4f7f2b47bf4352d76367322447b4` | `include/VTN4XX/`, `src/VTN4XX.cpp`; other-protocol inventory only |
| NextionNX3224T028_UART (no Git metadata found) | Standalone `src/NextionDisplay.h/.cpp`, `src/main.cpp`; no revision or shared library contract claimed |

The deeper sample `MAX31865/include/MAX31865/Transport.h` explicitly borrows
SPI callbacks/context, separates controller lock, chip select and transfer, and
documents callback lifetime. `ADS1261_ESP32/include/ADS1261_ESP32/Transport.h`
also separates callback roles and platform backend. **Adapt** the explicit
transport/platform boundary where appropriate, but SPI framing, CS timing and
pin roles are not MDIO rules. The other inventory rows are not additional
sources for selected DP behavior.

## Tool availability and current verification boundary

Observed in the Windows PowerShell environment on 2026-10-28, without
installing or building anything:

| Tool / source | Observed availability | Interpretation |
| --- | --- | --- |
| Python | 3.12.10, `C:/Users/Honza/AppData/Local/Programs/Python/Python312/python.exe` | Runs current standard-library reference checker. |
| C++ compiler | MinGW-w64 `g++` 15.1.0; `clang++` and MSVC `cl` not on PATH | Available future host compiler; no DP core exists to compile now. |
| CMake / Ninja | 4.0.1 / 1.12.1 on PATH | Available future host build tools; repository has no CMake project yet. |
| Doxygen | 1.13.2 on PATH | Available for source documentation after real declarations; no DP Doxyfile yet. |
| PlatformIO | Managed `C:/Users/Honza/.platformio/penv/Scripts/platformio.exe`, Core 6.1.19; another `pio` is on PATH | If used on Windows, a future repository wrapper must select the managed installation as root AGENTS requires. No DP PlatformIO project exists. |
| ESP-IDF | `idf.py` not on PATH; `IDF_PATH`/`IDF_TOOLS_PATH` unset; no framework-espidf under the checked managed PlatformIO packages; checked `C:/Espressif` and `C:/Users/Honza/esp` absent | **Not available in this shell/checked locations** for a native SDK link. This is not proof that no SDK exists elsewhere. Pin and install/select the actual SDK when prompt 6/37 requires it. |

The configured repository remote is the owner-specific URL above. Prompt 2's
source review and local link checker prove only documentation integrity and
observed source behavior. Host driver, portable consumer, SDK link, remote CI
and physical PHY evidence retain separate NOT RUN status until their actual
implementation/checks exist.

## Planned file responsibilities and consumers

The map names destinations for real work, not files to create now. Existing
paths are the root/readme/reference/test/tool directories already in this
workspace; future source and manifests remain absent.

| Planned path | Responsibility and consumer |
| --- | --- |
| `include/DP8382x/` | Framework-neutral public Status, typed Config/transport, exact variant/command values, facade and passive snapshots. A plain C++ MCU owner includes these without SDK headers. |
| `src/` | Actual shared chip procedures: binding/health, complete atomic C22/MMD access, configuration, bounded operations and diagnostics, split by responsibility as behavior appears. |
| `include/DP8382x/esp_idf/`, `src/esp_idf/` | Separately selected native ESP-IDF PHY adapter and supported MAC/mediator integration. No IDF type reaches the portable core. |
| `examples/common/Dp8382xCli.h`, `.cpp`, `ConsoleBuffer.h` | One example-only strict CLI, bounded console, owner admission and correlated terminal presentation; host fixture and native examples share it. Additional files only for real responsibilities. |
| `examples/esp_idf/` | Linked ordinary ESP32-P4 Ethernet application and application-owned maintenance/EtherCAT boundary example. Board pins, clock, PHY address, scheduling, network/EtherCAT stack and power delivery stay with the app. |
| `test/` | Registered host protocol/failure tests, compiled CLI fixture, framework-free and extracted/installed consumers, actual SDK link fixtures and offline runner/parser/artifact checks. Each evidence layer states what ran. |
| `tools/dp8382x_hil_runner.py` | Canonical bounded serial session, strict identity/correlation and complete artifacts. Fixture operation requires identified authorized scope; dry run stays NOT RUN hardware. |
| `scripts/generate_version.py`, `library.json`, generated `include/DP8382x/Version.h`, build manifests | One release-version truth, reproducible generated outputs and nonmutating checks. Firmware Git identity is separate; exported consumers need no Git/Python. |
| `Doxyfile`, `tools/check_doxygen.py` | Source-document coverage from the first real public header, warnings, representative pages and negative fixtures; ordinary consumers do not need Doxygen. |

The plain C++ MCU consumer proves the public core link; the ordinary P4
Ethernet example proves an actual SDK image link; the EtherCAT owner example
shows owner admission and MAC frame handoff without claiming a stack or cycle
timing. Hardware qualification remains a separate exact-board/fixture gate.
