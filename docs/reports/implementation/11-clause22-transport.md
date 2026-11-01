# Prompt 11: bounded Clause 22 transport

Executed 2026-11-01 on `promptset/dp8382x-implementation-20261028`.
Clean synchronized entry: `19bf456d472752865ec28bbc01f361d0cf25b229`;
original execution base remains `5e148fffde074b85fadc15d8ff66fe2c77111036`.
Fetched origin and verified equal entry histories before edits. The lead alone
owns integration, shared contracts and Git. Final commit/upstream equality is
recorded in the handoff, avoiding a self-referencing commit.

## Rules and delivered scope

Reread parent/root AGENTS, prompt 11, COMMON, CONVENTIONS, ENGINEERING, EVIDENCE,
FULL_COVERAGE, VERIFICATION and SOURCE_MAP. Inspected README, architecture,
variants, reference index and prerequisite outputs: prompt 6's ownership design,
prompts 7–8's exact batch/Status/lifetime contracts and prompt 9's public values,
consumer, active checks and Doxygen tooling. Prompt 10's committed version stays
unchanged. No root `.github` workflow exists; no trigger was changed and remote
CI dispatch/status checks remain prompt 71 only.

Implemented `bind`, `end`, `isBound`, `readClause22`, `writeClause22` and copied
`transportSnapshot` in the selected namespace/class. One source file owns the
current cohesive binding/one-frame responsibility; no unused MMD/operation
declarations or placeholder implementation. CMake now links the real static
C++17 library with tests OFF by default and no SDK/Python/Git build dependency.
The portable read example compiles and executes in the separate consumer.

One terminal batch callback owns controller exclusion, finite execution and
release. Public width/access checks precede any callback. Ordinary outputs stage
until normalized complete success, valid release/controller state, valid clock
and aggregate publication allowance. All-source counts, original signed primary
and release reports, possible effects and positive lateness evidence are retained.
No retries, guessed deadlines, cached hardware claims or managed-health updates.
Local lifecycle guards reject callback reentry/teardown; real serialization and
resource lifetime remain with the application owner.

Found and resolved a stage-order conflict: the frozen prose required initialization
before every write, but prompt 11 requires a usable write API and initialization
arrives at 15. Independent silicon review identified LEDCR as a complete independent
maintenance subset. The [API contract](../../api-contracts.md) now explicitly
admits its documented controls after binding under exact-part/address and LED0
electrical owner preconditions. No public fake initialized flag or arbitrary write
bypass was introduced. Other mutations still need their typed/collector policy.

Known nonconsuming raw words are readable; selectors, consuming/latch registers,
holes and CDCR are rejected. CDCR denial reflects the compulsory follow-up when
resistor calibration is observed enabled, not an invented read-clear effect.
All exact access policy and backend obligations are in the maintained
[direct-access guide](../../direct-access.md) and public Doxygen.

## Fresh source and sibling audit

All sibling repositories remained read-only and clean at these revisions:

| Reference | Revision | Actual reopened paths and behavior |
| --- | --- | --- |
| PCA9555 | `498178e4d9e4acb6fab7d8b816ad5021efe8806a` | `include/PCA9555/Config.h`; `src/PCA9555.cpp` `_mapTransportResult` (149), `_i2cWriteReadRaw` (206), `_i2cWriteTracked` (256); `test/test_basic.cpp` ambiguous writes (1124), short writes (1217), staged reads (2461), timeout distinctions (2477), aggregate grants/deadlines (2495/2520), RUN_TEST registration (2685–2743). |
| MB85RC | `1bf7c293549b1038b2d8f3b7a8d655fa74201e6e` | `src/MB85RC.cpp::_mapTransportResult` (1449); `test/test_basic.cpp` short/invalid/no-retry (5426), accepted timeout -701 and verify-only reconciliation (5841), registration (6540/6548); `include/MB85RC/Config.h`. |
| TMP1x2 | `bafc34067530eb340e55c42dae3fad4eb795b6e9` | `src/TMP1x2.cpp` normalization (12), `track` (120), staged `read` (140), `write` (152); `test/test_core.cpp` protocolAndClockGuards/formatFailureMatrix; `test/jobs/test_jobs.cpp` expiry and elapsed callback (228), CMake test registration and Release-active CHECK. |

Also reopened each AGENTS and `.github/workflows/ci.yml`; PCA9555/MB85RC
`examples/common/I2cTransport.h`, TMP1x2
`examples/01_basic_bringup_cli/main.cpp` and `examples/common/Tmp1x2CliDiagnostics.cpp`;
PCA9555 `tools/run_i2c_hil.py`, MB85RC `tools/hil_runner.py`, TMP1x2
`tools/hil_tmp1x2_runner.py`. These establish actual borrowed-buffer, finite
deadline, no-replay and retained-result precedents, not PHY timing or SDK proof.
Sibling suites were not run.

Adopted staged output, terminal normalization, signed detail preservation,
explicit failed-write uncertainty and separate attempt accounting. Rejected
I2C byte/ACK, register-pointer and accepted-prefix rules: MDIO writes have no
I2C acknowledgement proving application. A complete frame is not readback
verification. Rejected sibling wraparound millisecond timing in favor of the
frozen nonwrapping uint64 microsecond/lag contract. Complete MDIO transaction
ownership is required across users, not only individual callback exclusion.

TI's [current DP83826I page](https://www.ti.com/product/DP83826I) still identifies
SNLS647I. The original archived PDF was independently reopened for detailed
read/write semantics (tables 8-19..48), LEDCR table 8-42 page 70 and CDCR table
8-47 page 73. Page 16 column positions establish 2.5 MHz NOM and 24 MHz MAX,
0..13 ns output delay and 10 ns minimum setup/hold. Section 8.3.11 and table
8-20 establish preamble/gap/idle-edge/readiness conditions; they are not software
timeout defaults. The [core source catalogue](../../reference/library-ti-core.md#transport-and-mmd-procedure-audit)
remains the detailed comparison, including the rejected address/mask confusion.

The GitHub connector rechecked official `TexasInstruments/ethernet-rtos-drivers`
main at `94bf4343004732983b0b8e1eec123d88ff70453b` and reopened complete
[`rtos_drivers/src/generic_phy.c`](https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/rtos_drivers/src/generic_phy.c).
The inspected direct forwarding at lines 205–235 still lacks local five-bit
validation, staged output and aggregate ownership guarantees. The per-file
BSD-3-Clause notice was inspected; no implementation was copied. Combined TI
repository navigation supplied no stronger transport evidence. Archived vendor
bytes, provenance, hashes and factual ledger numeric definitions remain unchanged.

## Independent reviews and corrections

`transport_peer_audit` used the current capable coding model for live sibling
review and disjoint `test/transport.cpp` implementation. It derived literals from
the frozen contract/PDF review, not production constants. `transport_contract_review`
used available `gpt-6-astra`, high reasoning, for original-PDF access/timing and
ownership/deadline review. Its scope was read-only; neither agent edited siblings
or Git. The lead reviewed each contribution and the actual final diff.

Independent review corrected two contract mismatches before completion: attempted
failed reads may contain nonzero invalid scratch storage, and a latched clock
fault takes precedence over local report-stop admission on later calls. Added
regressions for both. Optional invalid timestamp evidence now prevents publication
without erasing independently valid physical completion counts. Teardown/rebind
documentation explicitly retains owner repair obligations. Review found no need
for a second transaction API, generic register framework or split source files.

Tests cover address bounds before narrowing, every C22 policy position, all
65,536 write words for the legal LEDCR mask, all 256 primary status representations,
short/default/extra reports, signed extremes, unchanged sentinels, accepted failed
and late writes, clock zero/regression/overflow, strict admission/publication
equality, controller contention/release/quarantine, no retry, copy/move restrictions,
reentry and two isolated/shared-controller instances. Host models explicitly do
not prove physical execution bounds.

## Verification and remaining evidence

Environment: Windows PowerShell, Python 3.12.10, GNU C++ 15.1.0 (WinLibs),
CMake/CTest 4.0.1 and installed Doxygen. Local acceptance commands:

```sh
cmake -S . -B build/transport-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON
cmake --build build/transport-release
ctest --test-dir build/transport-release --output-on-failure --no-tests=error
cmake -S test/consumer -B build/transport-consumer -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/transport-consumer
ctest --test-dir build/transport-consumer --output-on-failure --no-tests=error
python tools/check_doxygen.py
python scripts/generate_version.py check
python tools/verify_ledger.py
python -m unittest discover -s test -p "test_*.py" -v
python tools/verify_references.py
git diff --check
```

Final local results: PASS, nine registered Release CTest suites and five standalone
header compilations with strict warnings; the five transport suites execute
333,332 active checks. The deliberately failing CHECK probe exits 1 even under
NDEBUG. PASS, separate tests-OFF linked consumer. PASS, Doxygen generated 53 HTML
pages, validated public member/page/link coverage and rejected both negative
fixtures. PASS, version check; ledger (96 records, 52 procedures, 35 features);
98 Python tests; reference archive hashes (27 payloads, 43,240,385 bytes and
25 extracts), 1,346 local links in 157 Markdown files, and diff whitespace check.
Final independent acceptance review found no remaining blocking gap.
SDK link, installed/exported package qualification and physical timing/hardware
remain NOT RUN; the tests-OFF build-tree consumer is a separate PASS layer.
Remote CI is NOT RUN by the prompt-71 policy. Current silicon gates remain open
for their affected later implementation work; direct raw reads do not resolve
interpreted signal, loopback, EEE, reset or calibration conflicts.

Reviewed README/CHANGELOG, API/architecture/variant/feature guides, public-header
guide, test guide and Doxygen checker. Added the direct-access guide/example and
updated access/LED ledger evidence to PARTIAL, not full feature support. Version
tool instructions need no behavior change: the committed version and generation
workflow are unaffected. CLI/runners/SDK are still future code, so no nonexistent
commands or hardware success claims were added.
