# Consumer, CLI and runner convention follow-up

Historical numbering: DP8382x stage numbers in this report refer to the former
34-stage plan. Use the [current 80-prompt index](../prompts/README.md) and
[old-to-new mapping](prompt-workload-audit.md) for execution; its sole first-set
remote-CI checkpoint is now 71. The original findings below retain their dated
scope and are not current execution instructions.

Planning audit on 2026-10-26. This report supports expanded implementation
prompts 15-22 and 27. It does not execute those stages or establish a driver,
SDK build, test result, firmware image, hardware result or new public API.
The shared prompt contracts and stage 04 signature decisions remain authoritative.

## Inspected revisions and limits

Sibling checkouts were read-only. `git rev-parse HEAD` and `git status --short`
were inspected for each; all six reported an empty short status at inspection.
No fetch, branch change, build, serial connection or Git mutation was performed
in those checkouts. Their own AGENTS guidance was read for applicable context;
their CI/Git/model policies do not override DP8382x instructions.

| Repository | Inspected HEAD | Main evidence |
| --- | --- | --- |
| TMP1x2 | `bafc34067530eb340e55c42dae3fad4eb795b6e9` | Shared CLI header, console implementation, CLI test fixture, native-IDF glue, CMake, runner functions/artifact implementation and workflow source |
| 24Cxx | `936aa85e21b40b61c6f9cf0306d64c3beb6eaf6e` | CLI header, identity/artifact implementations and provenance tests, CMake and workflow source |
| INA228 | `b28c5ea69bcb4d26b351c52c24d3c6fae9fd282b` | Native-IDF dispatcher/framing and startup sections, transport mappings, runner frame implementation and exact-completion regression tests, workflow source |
| PCA9555 | `498178e4d9e4acb6fab7d8b816ad5021efe8806a` | Native-IDF `mapI2c`/device lifetime sections, runner-test inventory and fixture/verdict cases, workflow source |
| MB85RC | `1bf7c293549b1038b2d8f3b7a8d655fa74201e6e` | Runner classifier and restoration/payload checks, diagnostic tests, workflow source |
| FieldCore-node | `4b97894f11ba129079262131c353dbfd9bb438fb` | `Ina228Module.h`, selected implementation including cached projection/status mapping, owner/CLI file inventory and repository rules |

Paths below are relative to `Projects/<repository>/`, not build dependencies.
Inspection was focused on named declarations and implementation sections, not
an exhaustive re-audit of every sibling file. Related paths named in future
prompts must be reopened during implementation. Actual pinned Espressif controller,
PHY/base and mediator source inspection belongs to implementing stages 03/15;
this maintenance audit does not invent new SDK behavior or supported versions.

## Concrete adoption and adaptation

| Evidence path and actual symbol | Observed convention | DP8382x disposition |
| --- | --- | --- |
| `TMP1x2/examples/common/Tmp1x2Cli.h`: `Cli::setup`, `feed`, `processCommand`, `tick`, `printHelp` | Small framework-neutral example processor, fixed input and retained operation state | Adapt the simple entry points and responsibility split; use the actual externally owned PHY in IDF mode rather than create a competing CLI-owned driver |
| `TMP1x2/examples/common/ConsoleBuffer.h`: `ConsoleBuffer::format`, `append`, `drain`, `stats` | Fixed queue, whole-record format failure/drop, nonblocking limited drain, observable loss | Adopt bounded work/observable loss; add explicit retained/backpressured terminal completion because loss notification alone is insufficient for DP8382x automation |
| `TMP1x2/examples/common/Tmp1x2CliDiagnostics.cpp`: `startDiagnostic`, `tickDiagnostic`, `finishDiagnostic` | Example diagnostic jobs live apart from output/dispatch when responsibility warrants it | Adapt readable diagnostic grouping; reject file/class per command and avoid copying sensor conversion phases |
| `TMP1x2/test/cli/test_cli.cpp`: `Fixture`, transfer/write counters, controllable clock | Real shared CLI tested through simulated callbacks with observable effects | Adopt executable dispatch assertions for passive/rejected commands; use MDIO frame/logical counts instead of sensor transfer assumptions |
| `24Cxx/examples/common/Eeprom24CxxCli.h`: `discardInput`, `Platform::outputReady`, session fields and retained backup/results | Lost input invalidates the line; output can pause polling; mutating diagnostic state remains explicit | Adapt whole-line rejection and retained results; PHY cleanup deadlines must still advance under console pressure and EEPROM backup/barrier semantics are not copied |
| `INA228/examples/esp_idf/basic/main/main.cpp`: `processCommand`, `HIL_BEGIN`/`HIL_END`, `initExample`, `app_main` | Native-IDF consumer, familiar help/Status output, token/sequence framing | Adapt familiar presentation and correlation; do not copy the large dispatcher, I2C setup or assume an IN_PROGRESS trailer proves terminal completion |
| `INA228/tools/run_i2c_hil.py`: `hilrun_end_re`, `read_until_hilrun_end`, `strip_hilrun_frame` | Exact token/sequence trailer with required terminator; end sought after matching begin | Adopt complete-record/correlation tests; DP8382x additionally carries boot/session and retained operation identity under its frozen schema |
| `INA228/tools/test_run_i2c_hil_parser.py`: `test_frame_completion_requires_a_full_terminated_trailer`, `test_serial_commands_require_full_write_without_host_drain`, `test_stale_input_cannot_supply_framed_verdict`, `test_frame_identity_and_completion_are_exact` | Tests split the real trailer and reject incomplete writes/stale evidence | Adapt all split boundaries and no-replay failures; do not copy sensor-specific status patterns |
| `TMP1x2/tools/hil_tmp1x2_runner.py`: `one_record`, `operation_admission`, `response_complete`, `classify`, `EvidenceFiles`, `SerialSession` | Explicit record completeness, admission/result distinction, exclusive evidence reservation and provisional inconclusive result | Adapt bounded session/artifact flow; preserve DP8382x-specific cleanup/trust and explicit serial-opening policy |
| `24Cxx/tools/hil_identity.py`: `parse_identity`, `check_identity` | Complete identity records, unknown metadata stays unknown, explicit expectations, observed SHA prefix preserved | Adopt provenance separation and unknown rejection for required expectations; firmware identity comes from firmware, not the runner checkout |
| `24Cxx/tools/hil_artifacts.py`: `collect_provenance`, `render_markdown`; `tools/test_hil_artifacts.py::ProvenanceTests` | Bounded rooted Git queries, exact helper hashes, original argument boundaries and reports that preserve interrupted evidence | Adapt small testable artifact/provenance helpers and readable reports; final artifact manifest/flush/close semantics remain explicit DP8382x requirements |
| `PCA9555/examples/espidf_basic/main/main.cpp`: `mapI2c`, `ensureDevice`, `timeoutArg` | Preserve backend detail; ambiguous errors do not become false absence evidence | Adapt honest error/lifetime mapping; reject all I2C NACK/probe/address-range/controller conclusions for MDIO |
| `PCA9555/tools/test_run_i2c_hil_parser.py`: `test_fault_guard_snapshot_invariant`, `test_expected_rejection_classification_is_fail_closed`, `test_aggregate_destructive_failure_runs_recovery_before_stopping` | Expected rejection requires invariants; later recovery must not erase primary failure | Adapt unchanged-state/frame assertions, finite cleanup and truthful aggregate verdicts; GPIO fixtures are not PHY fixtures |
| `MB85RC/tools/hil_runner.py::classify`, payload/restore checks; `tools/test_hil_runner.py::test_diagnostic_prompt_requires_all_counters_and_restore_proof` | Required evidence counters, complete payload and restoration proof; contradictory status fails | Adapt false-PASS regressions for PHY diagnostics and campaign completion; FRAM scratch restoration is not PHY restoration evidence |
| `FieldCore-node/src/devices/i2c/Ina228Module.h/.cpp`: `bind`, `start`, `poll`, `takeResult`, `refreshStatus`, `snapshot`, `copyStatus`, `mapLibraryStatus`, `mapJobEffect` | Owner-local execution and retained result; cached snapshots and explicit application translation | Adopt ownership/projection principles; keep health, link, trust and application policy separate; reject TunnelMonitor dependency, I2C mappings and any existing Ethernet-support claim |
| `TMP1x2/CMakeLists.txt`, `24Cxx/CMakeLists.txt` | Explicit sources, consumer tests default OFF, executing CLI/console/runner registration | Adopt actual registered behavior tests and independent consumer boundary; do not add Python/Git/SDK/Doxygen to ordinary core consumption |
| `TMP1x2/test/core_no_arduino/compile_main.cpp` | Public includes, compile-time value properties and hostile platform macro coverage | Adapt portable-header checks as supplementary evidence; a compile-only fixture is insufficient to prove chip behavior or a real SDK link |

Workflow source inspected: `.github/workflows/ci.yml` in TMP1x2, 24Cxx, INA228,
PCA9555 and MB85RC. The TMP1x2/24Cxx files show explicit native CMake/CTest,
source guards and current SDK matrices; other siblings also pin PlatformIO and
register native/build work. These provide verification organization, not a P4
Ethernet support matrix. No remote workflow was dispatched, polled or required.
The first DP8382x execution keeps remote CI execution/status checking at stage 29.

## Prompt changes and proposed names

Prompts 15-22 and 27 now retain their existing work/acceptance/checkpoint rules
and add concrete implementation flows, paths, ownership, failure cases and
stage-specific documentation requirements. Each links CONVENTIONS and requires
source Doxygen or Python docstrings, applicable guides, root/example README and
CHANGELOG updates in the stage that adds behavior. Subagent assignments follow
frozen contracts and disjoint paths; reasoning complexity guides available agent
selection, without fixed model names or parallel competing implementations.

Proposed responsibility paths are clearly future design recommendations:
`include/DP8382x/esp_idf/Phy.h`, `src/esp_idf/Phy.cpp`,
`examples/common/Dp8382xCli.h/.cpp`, bounded `ConsoleBuffer.h`,
`examples/esp_idf/owner/` and `examples/portable/basic/`. Existing appropriate
modules should be reused; optional output/diagnostic files and a testable owner
helper require a current readability/consumer responsibility. No adapter factory
name or alternate portable operation API is invented here. Stage 04 freezes core
signatures; SDK factory naming and valid methods follow the supported pinned SDK.

Stage-specific additions include:

- 15: exact adapter-method ownership and wait-path audit, factory/lifetime/error
  unwinding, whole-transaction arbitration and real link fixture.
- 16: native application resource order, partial initialization cleanup, event
  servicing and separate management/link/IP/reachability evidence.
- 17: explicit application/maintenance admission table, stale/policy rejection,
  retained completion and frame-buffer ownership, without an EtherCAT stack.
- 18: parser/console modules, strict complete-line rules and single correlated
  machine schema with terminal-record retention.
- 19: full feature-to-command/help/owner/test mapping, typed action dispatch,
  side-effect collectors and legal-value/negative transcripts.
- 20: validate/reserve/open/identify/send/correlate/evaluate/cleanup/finalize
  runner flow and parser/artifact/serial failure injection.
- 21: explicit suite/fixture requirements and complete/interrupted verdicts;
  no host/dry-run promotion to hardware PASS.
- 22: cross-module protocol/owner/deadline/trust/console regressions with debug
  and optimized executing tests and independent vectors.
- 27: standalone tests-OFF C++ consumer, independent two-instance behavior and
  application status translation without sibling/SDK dependencies.

## Verification and handoff

This report records read-only source evidence and planning edits only. Local
`git diff --check` for the owned prompts/report passed. The integrated workspace's
`python tools/verify_references.py` passed with 27 vendor artifacts (43,240,385
bytes), 25 text extracts and 279 local links in 67 Markdown files at that check.
The lead repeats applicable checks after integration and owns checkpoint sync.
Sibling suites, portable driver tests,
SDK linking, remote CI and physical hardware were NOT RUN by this audit; those
results are not claimed for these planning-only edits. No hardware was operated and
no sibling code or Git state was changed.
