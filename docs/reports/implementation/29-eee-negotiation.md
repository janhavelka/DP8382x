# Prompt29: exact EEE negotiation and status

Implemented `eee_initialize` and `eee_negotiation_status` using the existing
operation slot, shared HARD primitive, configuration replay and destructive
collector. [EEE guide](../../eee.md) owns API/use/failure details. Explicit legacy
MAC TX LPI sessions remain prompt30; Q1 is physical-check pending.

## Rules, scope and checkpoint

Started on clean fetched `main` at `ce3df832c334c68e23a824333c9e5c86908b5ab8`,
equal to `origin/main`. Re-read the full attachment and repository prompt29,
COMMON, shared API/OP contracts, evidence/coverage conventions, architecture,
variants and current source/feature records. Prerequisites consumed: stage18
complete AN/forced profiles, stage20 single collector, stage5 EEE source
associations/actions and stage28 owned power/shared restoration.

Names follow existing PascalCase/camelCase and Status conventions. The lead owns
public/shared headers, integration, guides, progress and Git. `DP8382xEee.cpp`
is a cohesive recipe/status responsibility feeding the existing phase switch;
it introduces no transaction engine, allocation, scheduler or second collector.
One application owner serializes all instance calls and the entire shared MDIO
bus. Vendor payloads and sibling trees remained unchanged. No fixture operation,
SDK implementation, remote CI dispatch/status or workflow change was performed.

## Current source audit

Independent reviewer visually inspected SNLS647I p26's eighteen-row recipe,
p27 bypass/LPI prose, p35 MMD association, relevant BMCR/reset tables and nominal
timing columns. Fresh official TI PDF downloads exactly match archived bytes:

| Document | Revision | SHA256 |
| --- | --- | --- |
| SNLS647I | Aug2025 | `4e440fb8bd1cd44ee9ab0773944bef4f0605884f25c4ab96ea55d0ed859fd9c3` |
| SNLS638C | Apr2026 | `ec16ad2e7f01ede18aea5fbc39349cf4e6bceabc3541e6d3d43fc7bf17e3cb06` |
| SNLS505H | Jul2026 | `d73d83c018e6dab3f935ea45ca1771bbc34c030b6b24de45fccd1dfa6998a2d3` |
| SNLS783A | Oct2025 | `5015197a74de3c5269acd09fcb7de8dc83412a49db1a87a9b71f0e1ea2f86b8f` |
| SNLS603D | Apr2025 | `a13ecacc0d2d28c404c4e719fc76d665a4472723fceafbe7b5e5a49baecfb627` |
| SNLA328 | Oct2019 | `090228c89427dd1a4f018b1bc37a37af6d9e50e174137e65acff9b07e93ff426` |

The selected low-offset interpretation remains source-implied: SNLS638C p77
low-offset prose and pp80-81 tables, SNLS505H p37 worked indirect trace and
SNLS603D p47 notation analogy support it. Exact26 printed high-prefix addresses,
AI/AE's different recipe/MMD7 notation and SNLA328 full-address wording remain
recorded counter-readings. `eee_wire` and `eee_control_semantics` retain their
IDs and Q1 physical-check status. The ledger owns one selected tuple table;
[original-source vectors](../../register-vectors.md) independently spell every
recipe transaction and neutral composition. No generic digit stripping is used.

Current official source comparison remains pinned:

| Source | Commit and inspected file | Per-file license |
| --- | --- | --- |
| TI ethernet-rtos-drivers main | `94bf4343004732983b0b8e1eec123d88ff70453b`, `rtos_drivers/src/dp83826.c` | BSD-3-Clause |
| TI mcupsdk-core next | `4b9a66f4e18e02a834961f5f834155c846cab041`, `source/board/ethphy/icss_emac/ethphy_dp83826e.c` | BSD-3-Clause |
| TI ti-ethernet-software main | `450af1144b8caeb58724bcbae081b409f6bf5c3a`, Zephyr `phy_ti_dp83826x.c` | Apache-2.0 |

Those inspected drivers do not implement the complete recipe or qualify the
wire inference. Targeted current TI/E2E search found no inspected new exact-part
mapping/disable clarification; this bounded negative search proves no absence.
No source implementation was copied. Nominal EEE timings remain nominal.

## Fresh sibling audit

All three worktrees were clean; paths are relative to their repository roots.

| Reference/revision | Reopened declarations, source, cases and rendering |
| --- | --- |
| TMP1x2 `afd50889312f6e218a6a34eca687e3e8547d8b40` | `include/TMP1x2/TMP1x2.h`, `src/TMP1x2Configuration.cpp` validators/apply/verify; `test/test_core.cpp` verificationPreservesMismatchEvidence and `test/jobs/test_jobs.cpp` cancellationAndTeardown; CMake core/jobs/cli/HIL registration; `examples/common/Tmp1x2CliOutput.cpp`; `tools/test_hil_tmp1x2_runner.py`; AGENTS. |
| MB85RC `3ace168eb01d687ab60e9fe88cc72322e5cae260` | Public SleepState, `src/MB85RC.cpp` enterSleep/wake/_ensureAwakeForI2c; registered sleep/short/ambiguous-write cases in `test/test_basic.cpp`; CLI signed errors/health; `tools/test_hil_runner.py` restoration verdicts; AGENTS. |
| PCA9555 `8b461b2afe9910b5b15fe32cc049d52e3b498970` | Public settings/operation contract; `test/test_basic.cpp` ambiguous apply/short success/cancel/cleanup and registrations; CLI cached settings versus live reads; `tools/test_run_i2c_hil_parser.py`; AGENTS/native setup. |

Adopt pure preflight, retained intent/effects, independent trust/transport,
terminal failure and passive rendering. Reject sensor settling, memory tREC/
no-clock AWAKE claims, I2C ACK and pointer cleanup as PHY procedure evidence.

## Implementation and independent review

Three inherited capable-model agents supplied source reasoning, disjoint source
implementation and independent tests. `eee29_sources` first reviewed original
PDFs, then owned only ledger/vectors, and finally reviewed production read-only.
`eee29_peers` audited siblings, owned only `src/DP8382xEee.cpp`, then reviewed
shared integration read-only. `eee29_tests` owned only `test/eee.cpp`, deriving
literal expectations from original sources. Separate ignored build directories
were used; lead reviewed every contribution and performed all Git work.

The eighteen-row prefix executes once. Whole-word pre-HARD capability and
post-row2 advertisement checks fail closed; opaque rows cannot pass by guessed
readback. Desired fields are applied before the final owned-profile restart.
Disable and owned-policy initialization use one effective HARD; result.reset
reports the explicit upgrade from NONE/PCS/RESTART. Final shared verification
cannot declare restoration while another owned policy is dirty. Known pin,
next-page, diagnostic and power conflicts reject before callbacks.

The source minimum is 74 recipe/self-check frames, plus one reset poll and
subsequent shared initialization. Status acquisition is fixed at 31 frames in
eight callbacks. DEVAD-qualified collector keys prevent aliasing with C22/vendor
status, preserving partial destructive words and possible history loss. Valid
contradictions survive later transport errors. Ordinary outputs remain staged.

Confirmed review corrections integrated with regressions:

- Replace unsupported directional bypass names with honest per-bit controls.
- Preserve opaque EEECFG3 bit1; only mask1009 is owned, forced TX LPI stays clear.
- Keep initialized false until all EEE restoration proof is complete.
- Withdraw EEE trust on contradictory valid prefixes despite later failure.
- Withdraw cached negotiated validity after link loss/change or contradictory
  raw partner/advertisement evidence; preserve historical words/time.
- Reject negotiated inference with reset/power-down/loopback/isolation/restart
  evidence or unknown capability bits; changing bookends reject the aggregate.
- Check all owned policy proof after the final shared tail, including a raw EEE
  contradiction during later IRQ restoration.

The final maintainability review removed an unused tail-mode flag and redundant
status checks. Existing access tests now use the expanded literal collector mask;
all original rejection and unchanged-output assertions remain active. No silent
retry, implicit EEE activation, fabricated inverse or hardware qualification claim.

## Documentation and verification

Updated public Doxygen, README/CHANGELOG, API/init/architecture/feature guides,
ledger/vectors, test guide and [portable EEE example](../../../examples/portable/Eee.h).
The tests-OFF consumer compiles and executes explicit EEE disable/shared replay
and status/cached usage. Enable's full recipe executes in the independent host
fake with distinct DEVAD banks. Variant admission is unchanged; no new family
support is implied. Tool invocation/version/SDK contracts need no change.

Windows, GNU15.1, Ninja, C++17 Release with assertions and strict warnings active:

- PASS: `cmake -S . -B build/host -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON`,
  build and all 171 registered CTests, including nine EEE suites plus its deliberate
  failure-path test. Independent EEE suite reported 697,193 checks, zero failures.
- PASS: separately configured/built tests-OFF `build/eee29-consumer`, both CTests.
- PASS: 103 Python tests; ledger validation (97 registers, 52 procedures, 35 features);
  reference/local-link check; Doxygen (209 generated pages, two negative fixtures);
  deterministic version check; `git diff --check`.
- NOT RUN: SDK/controller and installed-package qualification; SDK support is a
  later owner, ordinary portable consumers are the layer exercised here.
- NOT RUN: remote CI (prompt71 only); physical Q1 wire/control/interoperability,
  timing and energy qualification. Existing unrelated DPD and other gates remain.

All stage29 software outputs and local checks are complete. Final commit and
upstream equality are recorded in the handoff, avoiding a self-referencing hash.
