# Sibling driver audit

Historical numbering: DP8382x stage numbers in this report refer to the former
34-stage plan. Use the [current 80-prompt index](../prompts/README.md) and
[old-to-new mapping](prompt-workload-audit.md) for execution; its sole first-set
remote-CI checkpoint is now 71. The original findings below retain their dated
scope and are not current execution instructions.

Read-only source review on 2026-10-26 for the DP8382x implementation promptset.
Paths below are relative to the parent `Projects/` directory. This is a source
audit, not a claim that the sibling tests, firmware builds or HIL ran today.
Sibling instructions are evidence of their conventions; their Git permissions,
hardware authorizations and project-specific exceptions do not transfer here.

## Source snapshot and coverage

| Repository | HEAD inspected | Working tree observed | Review depth |
| --- | --- | --- | --- |
| INA228 | `b28c5ea69bcb4d26b351c52c24d3c6fae9fd282b` | Clean | AGENTS, public contracts, job/health source, CLI, runner, layout |
| PCA9555 | `498178e4d9e4acb6fab7d8b816ad5021efe8806a` | Clean | AGENTS, typed transport, passive lifecycle, health/cleanup source, CLI, docs, ownership |
| TCA9548A | `ce425e1984ac269e7934dd811a06ec08b91e307c` | Clean | AGENTS, API/source, common CLI, host consumer, HIL/artifact tests, package checker, full CI |
| MB85RC | `1bf7c293549b1038b2d8f3b7a8d655fa74201e6e` | Clean | AGENTS/CONTRIBUTING, family/lifecycle/transport declarations and binding source, layout |
| BME280 | `d538f2e203ee26d6ddbc86f86d2b0cf4c73a6e98` | Clean | AGENTS, staged jobs/state declarations, timing guard, example/tool layout |
| SCD41 | `c0bf1bba282d12b4aa3196d9f861915fe9b22551` | Clean | AGENTS, operation/result/health declarations and execution source, full CI, tool layout |
| SHT3x-main | `f65f15573e3c9b1ef04cce72ad08f307d021e563` | Clean | AGENTS, cooperative/health declarations, logical/physical health source, layout |
| TMP1x2 | `bafc34067530eb340e55c42dae3fad4eb795b6e9` | Clean | AGENTS, explicit model/address capabilities, shared-bus operation contracts |
| TUSB32x | `d22163546c927a03717322f2f47aeb0798d4b1c6` | Dirty | AGENTS, current README, promptset/partial-implementation structure |

The TUSB32x tree had concurrent modifications to `CHANGELOG.md`,
`docs/audit/execution-7-peers.md`, `docs/design-decisions.md`,
`docs/feature-coverage.md`, `docs/implementation-progress.md`,
`docs/validation.md`, `examples/host/basic/README.md` and `src/TUSB32x.cpp`.
Its observed content is therefore not represented by HEAD alone. Nothing in
any sibling was edited, staged, built or committed during this audit.

An additional manifest/layout inventory found these chip-driver precedents:

| Group | Repositories | Useful comparison; limits of this pass |
| --- | --- | --- |
| Other I2C devices | `24Cxx`, `ADS1115`, `INA3221`, `LDC1614`, `LSM6DS3TR`, `MCP45HVX1`, `OPT4001`, `RV3032-C7`, `SSD1315` | Same public-header family, bring-up CLI and Python/CI pattern; inventory only here, not a behavioral audit |
| SPI devices | `ADS1261_ESP32`, `MAX31865` | Explicit `Transport.h`, `Protocol.h`, optional `ArduinoBackend.h`, separate sampling/maintenance/CLI examples; transport adaptation precedent, not MDIO behavior |
| Other wire protocols | `AT21CS11`, `EE871-E2`, `VTN4xx` | Separate protocol/transport or externally owned service boundaries; no justification to copy timing implementations |
| Broader networking | `SIM7080G-Core` | Task/queue/platform modem architecture; unsuitable as the core ownership template for a PHY |

Manifest descriptions such as "production-grade" were not accepted as
verification evidence. Manifest framework filters also vary: `*`, explicit
Arduino/ESP-IDF lists, or no restriction. Native compilation is a stronger
portability test than any one manifest spelling.

## Conventions worth preserving

| Aspect | Observed evidence | DP8382x application |
| --- | --- | --- |
| Public layout | All seven primary libraries use `include/<Chip>/{CommandTable,Config,Status,<Chip>,Version}.h`; implementation is under `src/` | Preserve `include/DP8382x/` and `src/`; add a file only for an implemented responsibility |
| Naming | Their AGENTS and public declarations use PascalCase types, `camelCase` fields/functions, `_camelCase` members, `CAPS_CASE` enum values/constants | Use the same naming; preserve `DP8382x` spelling; MDIO errors and settings must have PHY meanings |
| Status | `INA228/include/INA228/Status.h`, `PCA9555/include/PCA9555/Status.h`, `TCA9548A/include/TCA9548A/Status.h` define `Status { Err code; int32_t detail; const char* msg; }`, static messages, `ok()`, `is()`, explicit bool, `Ok()`/`Error()` | Reuse the shape and explicit errors, without importing I2C NACK codes or assuming all backends can diagnose absence |
| Non-owning transport | `PCA9555/include/PCA9555/Config.h`, `SCD41/include/SCD41/Config.h`, `TMP1x2/include/TMP1x2/Config.h` | Application owns handles, pins, clocks, locks, timeout/retry/recovery; core borrows callbacks/context with documented lifetime and no re-entry |
| Framework boundary | Primary AGENTS prohibit core Arduino/IDF/RTOS dependencies; native examples use `app_main` and real IDF APIs | Portable core plus an explicitly separate IDF adapter; do not put an Arduino compatibility facade in a native IDF example |
| Side-effect evidence | PCA9555 `TransportResult`/`WriteEffect`; MB85RC write/transfer results; SCD41 retained operation/effect results | Preserve ambiguous writes, partial configuration, cancellation effects and configuration validity; a failed call need not mean hardware was untouched |
| Example ownership | `examples/common/` is explicitly example-only in the primary AGENTS | CLI/console/board glue stays outside the shipped core; a reusable IDF PHY adapter needs its own documented library boundary |
| Host proof | `TCA9548A/test/core_no_arduino/CMakeLists.txt` links only `TCA9548A::TCA9548A`; root CMake supplies includes and C++17 | Build/link an ordinary C++ consumer without frameworks, sibling include paths or test-only framework stubs |
| Versioning | `library.json` is source of truth; generators/checks synchronize `Version.h`, component metadata and Doxygen in mature siblings | Add actual manifests with implementation; do not fabricate repository metadata or hand-edit generated version fields |
| Documentation | `PCA9555/docs/README.md`, `MB85RC/CONTRIBUTING.md`, SCD41 AGENTS separate source API contracts, durable guides, vendor references and evidence | Document lifetimes, units, side effects, timing, failure and ownership at declarations; keep prompt progress separate from release claims |

## Differences that must not be averaged away

There is no single sibling lifecycle or universal health contract:

| Source | Actual distinction | Lesson for a PHY |
| --- | --- | --- |
| `PCA9555/src/PCA9555.cpp`, `include/PCA9555/PCA9555.h` | Passive `bind()`/`begin()`, three states `UNINIT/READY/DEGRADED`, budgeted multi-transfer operations, retained result | Bound callbacks are not proof of device presence; keep admission and physical observation separate |
| `TCA9548A/src/TCA9548A.cpp`, `AGENTS.md` | Bounded synchronous single-register driver; `tick()` is a no-op; first tracked success establishes READY; four observational states | Do not invent a job engine or a no-op lifecycle solely for uniformity; choose bounds for actual PHY work |
| `INA228/include/INA228/Config.h`, `src/INA228.cpp` | Cooperative jobs with bounded synchronous conveniences; separate hardware synchronization; passive default and legacy `LATCH_OFFLINE` option | Keep compatibility behavior out of a new API unless a current consumer needs it |
| `BME280/include/BME280/BME280.h` | Health separate from `ConfigSyncState` and `CalibrationState`; concurrent hardware APIs reject active staged jobs | A readable PHY can have untrusted configuration; unrelated successful MDIO must not repair that trust |
| `SHT3x-main/src/SHT3x.cpp` | Physical callback counters and logical operation completion differ; protocol failure can follow successful transport | Count an indirect access/verified operation at the documented level; successful selector writes are not successful target reads |
| `SCD41/src/SCD41.cpp`, `include/SCD41/SCD41.h` | Separate transfer/protocol/operation health, retained results, no-I/O admission, owner-driven poll | Useful evidence boundaries; command waits and sensor-specific expected NACKs are not transferable PHY semantics |
| `MB85RC/include/MB85RC/MB85RC.h` | Sleep state separate from passive four-state transport health; OFFLINE does not suppress owner operations | Keep power state, readiness, transport health and configuration trust independent |
| `TMP1x2/include/TMP1x2/Config.h`, `BusOperations.h` | Explicit BOM model/capabilities, no fabricated readable identity; shared-bus commands outside per-device state | Family support requires exact part facts; physical/shared effects belong to the appropriate owner |

Counter lifetimes differ too: TCA9548A documents object-lifetime totals,
BME280 health-session totals, and SHT3x logical-session versus physical totals.
Name helpers differ (`errName`, `errorName`, `toString`). Choose and document
one DP8382x contract, including saturation, reset conditions and valid timestamps;
do not create aliases just to reproduce every sibling.

`PCA9555::_i2cWriteCleanupTracked()` records cleanup failures but deliberately
does not let successful cleanup erase the original operation failure. This is
a useful design lesson for restoring PHY diagnostic modes or indirect-access
state. Report primary and cleanup results separately.

For DP8382x specifically, never equate `DriverState::READY`, bound callbacks,
successful MDIO, physical link-up, negotiated mode, IP readiness or EtherCAT
operational state. Those describe different owners and evidence.

## CLI and Python runner precedents

`INA228/docs/CLI.md` and both example front ends define common grammar,
aliases, strict whole-token parsing, full error names and native framework
identity. `version/ver`, `help/?`, `cfg/settings`, `drv/health`, `diag`, `probe`,
and bounded test/status commands are recurring concepts, not a mandatory list
of all aliases. Native-IDF example directory spelling varies across siblings
(`espidf_basic`, `idf/basic`, `esp_idf/basic`); DP8382x should keep its established
`examples/esp_idf/` and document any subdirectories.

Two concrete runner designs were inspected:

- `INA228/docs/CLI.md` and `tools/run_i2c_hil.py` use
  `hilrun <token> <sequence> <inner command>` with matching `HIL_BEGIN` and
  newline-terminated `HIL_END` records. The runner distinguishes command Status
  from PASS/FAIL/UNKNOWN/NOT_RUN verdicts, verifies firmware-reported commit,
  version and framework, and rejects incomplete or mismatched framing.
- `TCA9548A/examples/common/Tca9548aCli.cpp` shares a portable CLI between
  Arduino and IDF. `tools/tca9548a_hil.py`, `test_tca9548a_hil.py` and
  `test_hil_artifacts.py` test strict classification, unique output bundles,
  full transcripts, report/JSON/manifest hashes, firmware identity, output
  failures, interruption and dry-run semantics. A dry run does not open serial
  and is explicitly NOT_RUN hardware evidence.

Adopt one framing contract and test it behaviorally. Require explicit serial
selection, finite commands/frames/buffers/deadlines, malformed-input and stale
output tests, and no inferred flashed identity from the host checkout. Keep
destructive diagnostics/reset/raw writes explicit; examples distinguish cached
health from live status reads. Define how PHY reads with latch/clear side effects
interact with SDK polling. Do not copy an I2C scan or sensor measurement menu.

Python should orchestrate fixture campaigns and preserve incomplete evidence;
it must not turn a timeout or skipped physical test into PASS. Runner parser
tests are host evidence, not HIL. CLI output formatting, counters and shared
command tables can be borrowed conceptually without creating a FieldCore
dependency or a second recovery/configuration owner.

## Verification, packaging and repository ownership

`TCA9548A/.github/workflows/ci.yml` demonstrates registered native driver,
CLI/console/transport suites; standalone CMake compile/link/run; strict warnings;
runner tests; Doxygen; package creation/inspection; Arduino builds; and actual
native-IDF builds. `SCD41/.github/workflows/ci.yml` additionally shows UBSan and
clean extracted-package consumers. Their target/version matrices are evidence
of those repositories' choices, not a P4 compatibility matrix to copy.

`BME280/tools/check_core_timing_guard.py` rejects platform includes/timing calls.
Such text guards supplement compilation and behavioral tests. They cannot prove
bounded backend waits, transaction locking or public API correctness.
`TCA9548A/tools/check_package.py` checks an allowlisted archive, path safety,
duplicate/special members, byte limits and source-matched payloads, then builds
a clean consumer. Package validation must test both content exclusion and actual
consumption; exclude DP8382x vendor archives, generated builds and evidence logs.

Human review ownership is explicit: INA228 and TCA9548A root `CODEOWNERS`, and
PCA9555 `.github/CODEOWNERS`, nominate `info@thymos.cz`; MB85RC uses
`@janhavelka`. This is separate from runtime ownership. Confirm applicable
maintainer metadata for DP8382x rather than inventing an owner or repository URL.

Sibling AGENTS consistently require the lead to own shared Git mutations and
preserve concurrent changes. Their commit/push authorization is local to those
projects. In particular, TUSB32x's branch-per-execution rule and special remote
CI stage are explicit user-specific exceptions. Follow DP8382x and parent rules,
not those exceptions. TUSB32x's promptset is useful structural evidence, while
its current README explicitly says the device driver remains planned.

## Required repeat audit during implementation

Before every implementation stage, revisit the relevant sibling AGENTS, public
declarations, implementation, examples, CLI, tests, Python tools, manifests and
workflow definitions. Record current HEAD/dirty state and exact paths, then state
what is adopted, adapted or rejected and why. Inspect additional siblings when
the feature requires them. Do not re-run sibling hardware or modify siblings to
perform this audit, and do not treat this dated report as a frozen specification.

The implementation promptset should require proof of standalone portability,
one configuration owner per PHY, serialization of complete indirect MDIO access,
bounded SDK adapter calls, honest configuration/link/health snapshots, independent
chip vectors, command/runner failures, and separately reported host, SDK link,
CI and physical-hardware evidence. Family expansion must retain exact variant
gates rather than flattening all DP8382x devices into one assumed register map.
