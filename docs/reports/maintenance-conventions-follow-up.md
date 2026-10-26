# Family, packaging and maintenance convention follow-up

Historical numbering: DP8382x stage numbers in this report refer to the former
34-stage plan. Use the [current 80-prompt index](../prompts/README.md) and
[old-to-new mapping](prompt-workload-audit.md) for execution; its sole first-set
remote-CI checkpoint is now 71. The original findings below retain their dated
scope and are not current execution instructions.

This is a planning-maintenance audit performed on 2026-10-26. It deepens stages
23-26 and 28-34; it neither executes those stages nor implements a PHY driver,
build, test suite, package, CI workflow or hardware qualification. Siblings were
read-only. The lead owns Git and shared contracts; this contribution changes
only those numbered planning files and this report.

## Current source snapshot

The audit used `git rev-parse HEAD`, `git status --short`, `rg --files`, targeted
symbol searches and direct source/test/workflow reads, not remote run queries.

| Sibling | Inspected HEAD | Working tree at initial inspection |
| --- | --- | --- |
| TMP1x2 | `bafc34067530eb340e55c42dae3fad4eb795b6e9` | clean |
| MB85RC | `1bf7c293549b1038b2d8f3b7a8d655fa74201e6e` | clean |
| 24Cxx | `936aa85e21b40b61c6f9cf0306d64c3beb6eaf6e` | clean |
| SCD41 | `c0bf1bba282d12b4aa3196d9f861915fe9b22551` | clean |
| TCA9548A | `ce425e1984ac269e7934dd811a06ec08b91e307c` | clean |
| TUSB32x | `dd82a796e51dcfefa77db36a921d69aec0b12346` | concurrent modifications/untracked files; see scope below |

TUSB32x had tracked modifications in CMake, the host example, component/library
metadata, `CommandTable.h`, private protocol source/header, test registration,
planned contract fixture and protocol tests. New codec source/header/tests and
an audit report were present. This report treats the inspected working files
as a live snapshot, not contents proved by that HEAD alone. No sibling changes
were staged, committed, reverted or synchronized by this audit.

Read parent/root AGENTS, README, architecture, variants, reference index,
prompt index/COMMON/ENGINEERING/FULL_COVERAGE/SOURCE_MAP/VERIFICATION and the
maintained CONVENTIONS baseline. Read relevant AGENTS in all six peers and
CONTRIBUTING in MB85RC/TUSB32x. Parent recurring CI rules remain subject to this
repository's explicit first-set remote-CI-only-at-29 exception.

## Family selection, values and health

Paths below are relative to Projects. Inspection depth is stated to avoid
turning an inventory or declaration into an implementation claim.

| Inspected paths/symbols | Observed fact | DP8382x disposition |
| --- | --- | --- |
| `TMP1x2/include/TMP1x2/Config.h`: `Model`, `isValidModel`, `modelAddressMin/Max`, `isValidAddress`, `hasAlertOutput`; `src/TMP1x2Configuration.cpp::validateConfig`; complete `test/variants/test_variants.cpp` | Explicit package selection; pure capability checks; test exhausts address ranges and proves absent ALERT pin does not disable internal thermostat controls | Adapt to exact `Variant`, interface/medium/feature preflight and zero callbacks; absent first-board wiring cannot erase software features |
| `MB85RC/include/MB85RC/CommandTable.h`: `DeviceVariant`, `cmd::VariantInfo`, `KNOWN_VARIANTS`, `findVariantByProductId`; targeted source around `parseDeviceId`, `variantForExpected`, `isSupportedRuntimeVariant`, `_selectVariant` | Static exact-device metadata distinguishes readable identity, documented capability and implemented runtime support; unknown manufacturer/product errors preserve detail; selection change invalidates device-specific cached state | Adapt a small explicit variant table/switch if useful; retain declaration versus observation and invalidate PHY-specific evidence on rebinding/identity changes |
| `MB85RC/include/MB85RC/MB85RC.h`: `DriverState`, `SettingsSnapshot`, passive accessors; complete `test/test_public_contracts.cpp` | Four-state passive health and value/ownership compile contracts; historical compatibility aliases and transport ordinals also exist | Adopt non-owning/fixed values and compile checks; reject copying old aliases/ordinals or treating compile fixtures as behavior/hardware evidence |
| `24Cxx/include/EEPROM24Cxx/Config.h`, `Types.h`: `DeviceVariant`, `Geometry`, `TransportResult`, `WriteCommit`, `DriverState` | Generic geometry presets explicitly do not detect exact silicon; terminal transfer effects and failure uncertainty are separate from Status | Adapt exact-part and accepted-write uncertainty discipline; reject EEPROM geometry, NACK/write-cycle semantics and a speculative second PHY transport API |
| `TUSB32x/include/TUSB32x/Config.h`: `Variant`, `DriverConfig`, `DeviceProfile`, callback/validator Doxygen; complete `src/TUSB32x.cpp`; first validator-test sections in `test/test_validators.cpp` | Invalid enums are checked before capability restrictions; pure validators preserve inputs and attempt no I/O; semantic enums are not register encodings; some later behavior remains planned/gated | Adopt invalid-versus-unsupported distinction and precise Doxygen; keep DP8382x's selected `Config`/stage-04 names instead of copying every peer type name |
| `TMP1x2/include/TMP1x2/Status.h`, public health declarations, targeted `src/TMP1x2.cpp` and configuration procedures | Structured `Err/code/detail/msg`, static messages, `ok/is/inProgress/Ok/Error`, four-state health and separate raw/tracked access | Adopt selected CONVENTIONS shape and vocabulary; reject I2C error names, sensor readiness aliases, and any assumption that link down is MDIO failure |

Variant stages now require per-part source/field/procedure differences, complete
software coverage and independent literal vectors. Shared IDs cannot establish
an orderable suffix, temperature grade, wiring or medium. Original/A generations,
DP83825I and DP83822 subtypes must qualify exact reset, clock, 10 Mbit, optional
control and diagnostic sequences. No new silicon sequence is asserted by this
maintenance audit: archived exact-part datasheets remain authoritative.

## Packages, version tools and executing checks

| Inspected paths/symbols | Observed fact | DP8382x disposition |
| --- | --- | --- |
| `TMP1x2/scripts/generate_version.py`: `_expected_outputs`, `_sync_outputs`, `_get_git_info`, `main`; complete `tools/test_version_sync.py` | `library.json` drives generated header and CMake/component/Doxygen versions; `sync/check/bump/set`; isolated tests mutate a temporary copy; script also contains historical version alias/build metadata | Adopt one release truth, idempotent sync/check and isolated drift tests; reject unnecessary aliases and core-consumer Git/build discovery; firmware provenance stays example metadata |
| `MB85RC/tools/check_metadata_consistency.py` | Explicit metadata comparisons also fix this sibling's framework/target/SDK policy | Adapt coherence checks; reject S2/S3/espressif-only constraints as PHY portability policy |
| Complete `TMP1x2/tools/check_package.py` | Required content checks, basic path/link/duplicate checks and renamed CMake consumer compile/link/run | Adopt real extracted-consumer proof, extend limits and platform alias checks rather than copying its unbounded subprocess helper |
| `TCA9548A/tools/check_package.py`: constants, `expected_files`, complete `check_archive`, targeted `build_consumer`/entry paths | Bounded compressed/content bytes, member count and subprocesses; explicit content; casefold duplicate/path checks; rejects special/sparse/link members and source drift | Adopt bounded pre-extraction allowlist and negative fixtures, adapting required content to actual DP8382x portable/SDK consumers |
| `SCD41/tools/check_package_contents.py`; complete `tools/check_target_package_consumer.py`; `tools/check_clean_consumer_compile.py` consumer and `safe_extract` | Separate payload and native/target consumers; native runs while firmware is compile/link only; extraction uses resolved path containment and a compatibility fallback | Adopt extraction-only public-consumer journey and honest evidence labels; reject inherited S2/S3 matrix and legacy unfiltered extraction fallback |
| Complete `TMP1x2/CMakeLists.txt`; `TUSB32x/test/test_validators.cpp` explicit `CHECK` | Explicit source/test registration, tests OFF, Python required only tests ON, per-suite timeouts; validator checks remain active under NDEBUG | Adopt actual registration/counts and Release-active checks; zero tests is not a pass |

The stage-28 destination is the existing version generator and a focused
`tools/check_package.py`/negative suite, with durable `docs/packaging.md`.
No implementation tool is created now. A general verification dispatcher is
conditional on removing real duplication; it is not a required new framework.

## Source documentation, workflows and evidence

Read complete `TMP1x2/tools/check_doxygen.py` and its regression tests:
`check_html` validates local documents/assets/anchors; `check_api` compares
generated public members/types/inputs and private exposure. `main` uses a finite
generation deadline, temporary XML and constrained replacement of validated HTML.
Failure preserves previous output. Read TMP1x2/MB85RC/TUSB32x Doxyfile inputs and
warning policies. TMP1x2 uses `EXTRACT_ALL=YES` plus generated-content checks;
MB85RC/TUSB32x explicitly enable undocumented-member/enum/parameter warnings.
DP8382x should use useful strict warnings plus generated visibility/link checks,
not blanket extraction to disguise missing docs. Source Doxygen starts with the
first implementation; stage 32 audits residual gaps rather than introducing it.

Read complete `TCA9548A/.github/workflows/ci.yml` and relevant
TMP1x2/SCD41 workflow sections. TCA9548A shows source-pinned actions, read-only
permissions, finite jobs, pinned/hash-checked documentation tools, real native
and package commands and SDK build jobs. SCD41 has separate package/target
consumer registration. Reuse source-based local/CI command parity and pinned
inputs where appropriate. Reject unrelated sensor/mux firmware matrices and
never treat workflow YAML or sibling green claims as execution evidence.

Read `TCA9548A/tools/test_hil_artifacts.py::ArtifactTests` through its dry/live
identity and persistence-failure cases; inspect corresponding runner structure.
Read targeted `MB85RC/tools/hil_runner.py::payload_failure_reason` and
`strict_failure_reasons`: final results need complete diagnostic counts,
restoration and required observed firmware/device/transport facts. Adopt distinct
host/firmware identity, unique retained artifact directories, NOT RUN dry runs,
nonzero artifact failures and restore evidence. Do not reuse memory scratch
operations or claim a full end-to-end serial/physical audit from these reads.

Stages 29-34 now name executing evidence, exact input identities, independent
review questions and final claim layers. Stage 29 alone dispatches/checks/waits
for remote CI in the first execution. Its tested SHA stays distinct from later
CI-evidence documentation and local substantive fixes. No self-referencing hash
update loop, skip marker or disabled check is required. Physical action and
release publication remain separately authorized scopes.

## Maintained prompt changes and checks

Stages 23-26 now specify exact source anchors, responsibility-based shared
implementation, per-part/mode vectors, invalid/no-I/O behavior, ambiguous IDs,
partial-write/cleanup effects and feature closure. Stages 28-30 specify bounded
archive negatives, metadata truth, real clean consumers, executing CI job inputs
and fixture/artifact admission. Stages 31-34 specify durable guide destinations,
source-doc/generated-page checks, finite read-only review assignments and a
final evidence table. Each stage reviews source Doxygen, README/CHANGELOG and
affected guides with the behavior, selects useful available agents by complexity,
and keeps coherent intraprompt plus completed-stage commit/sync on one branch.

Planning checks: `git diff --check` and `python tools/verify_references.py`.
The lead integrates/reruns these for the final shared change and records its
actual checkpoint/sync evidence. No driver host tests, SDK link, package consumer,
Doxygen generation, remote CI or physical qualification were run by this audit;
those implementations do not yet exist in this repository. Live sibling source
must be reopened before execution; this dated report is not a substitute.
