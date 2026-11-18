# Prompt 28 — power policy and explicit states

Completed the independently source-supported active-sleep, IEEE power request/
clear and separate PLL-control procedures. Deep-power-down remains **BLOCKED** by
`deep_power_exit`: its zero-I/O rejection is implemented and tested, but silicon
entry/exit is not claimed. The whole power feature therefore remains PARTIAL in
the ledger. SDK and physical power/clock qualification remain NOT RUN.

## Rules, ownership and checkpoint

Started on clean, fetched `main`, equal to `origin/main`, at
`ced8b1ce5418aec044eb156b507ea78573302576` (prompt 27). Read the full attached
prompt and repository prompt 28, COMMON, shared contracts, API, architecture,
variants and reference index. Prior stage 22 reconciliation and stage 5 ledger
are the consumed prerequisites. Current procedures owned here:
`power_ieee`, `power_active_sleep`, `power_pll_control`, `power_deep`.

Lead owns shared headers, integration, guides, ledger, progress and Git.
`DP8382xPower.cpp` is one cohesive explicit phase switch in the existing operation
slot, transport and collector. Names follow existing Status/PascalCase/camelCase
contracts. The application owns instance serialization, all-bus atomic RMW,
clocks/pins and scheduling. No SDK, allocations, background work or new framework.
Siblings and archived vendor bytes were read-only. No remote CI dispatch/status
was performed; first-set CI remains prompt 71 and no workflow was added.

## Exact source and independent vectors

Independent reviewer reopened original SNLS647I pp28-29 section 8.3.6, p55
table 8-19, p65 table 8-35 and p69 table 8-40, plus p3/p6 mode/pin tables and p75
table 8-50 for the opaque DPD/compliance overlap. The current official TI
`dp83826i.pdf` is byte-identical to the archived RevI August 2025, 114-page PDF:
SHA256 `4e440fb8bd1cd44ee9ab0773944bef4f0605884f25c4ab96ea55d0ed859fd9c3`.
Targeted current TI/E2E searches found no inspected exact-part DPD exit
clarification; this is a bounded negative search, not proof of nonexistence.

The reviewer derived active-sleep, software/pin OR and PLL arithmetic directly
from original tables. `B700 -> 3D00/3500` preserves isolation and neutralizes
commands; reset-bearing samples still invalidate readiness. `8108 -> E108`
preserves PLL during a policy update. BISCR 0100 means normal despite enabled
policy. No 1.4-second partner deadline or PLL-ready delay was imported.
The [vectors](../../register-vectors.md), [power guide](../../power.md) and ledger
record the interpretation. No unlicensed implementation was copied.

## Fresh sibling audit

All four sibling worktrees were clean at inspection. Paths below are relative
to `Projects`; they are evidence, not build dependencies.

| Sibling/revision | Reopened declarations, implementation and executing cases |
| --- | --- |
| MB85RC `3ace168eb01d687ab60e9fe88cc72322e5cae260` | `include/MB85RC/MB85RC.h`, `Config.h`; `src/MB85RC.cpp` enterSleep/wake/_ensureAwakeForI2c; `test/test_basic.cpp` sleep variant, incomplete transfer, wake timeout and no-clock cases; ESP-IDF/Arduino CLI; `tools/hil_runner.py`, `tools/test_hil_runner.py`; platformio native registration. |
| SCD41 `0b9d3d02c27feacf9b1354dc2a8b83a9e5a04879` | `src/SCD41.cpp` explicit power/wake phases; `test/test_basic.cpp` failed wake/reattach, cancel between phases, accepted effects and bus-silent end; `examples/01_basic_bringup_cli/main.cpp`; `tools/scd41_hil_runner.py`, `tools/test_scd41_hil_runner.py`; native/UBSan registration. |
| RV3032-C7 `15d7a2782f7d6e136d29ff7339c1bee27bc5b8aa` | `include/RV3032/RV3032.h`, `src/RV3032.cpp` backup admission/observation; `test/test_native/test_datetime.cpp` backup fault/timing/evidence, activation reconciliation and charger permission; CLI backup status; `tools/hil_cli_runner.py`, `tools/test_hil_cli_runner.py`; native registration. |
| PCA9555 `8b461b2afe9910b5b15fe32cc049d52e3b498970` | `test/test_basic.cpp` ambiguous apply/shadow invalidation and primary/cleanup cases; `examples/01_basic_bringup_cli/main.cpp` cached getSettings display; `tools/test_run_i2c_hil_parser.py`; native/RUN_TEST registration. |

Adopt explicit permission, staged output, preserved partial evidence and uncertain
effects. Reject FRAM tREC/I2C ACK rules and no-clock AWAKE claim, SCD41 expected
wake NACK/30ms timing, RTC activation/charger policy, and automatic pointer cleanup
after cancellation. DP8382x OP-9 always uses a later restoration operation. RTC
backup status is live I/O; PCA9555 settings rendering is cached—these were checked
in the actual CLI implementations, not inferred from names.

## Implementation and review findings

`startConfigurePower` is bus/clock silent. Polling validates live controls,
performs source-specific neutral RMW/readback and samples BISCR through the
existing collector. Desired software intent is separate from BMCR OR readback.
A blocked wake terminates after one clear. `readPowerStatus` stages ordinary
output while retaining partial raw words; cached queries do no I/O.

All existing BMCR writers now compose explicit owned software intent, including
profile and diagnostic restoration. Unowned active-profile preflight still
requires observed power-down zero. Owned down is retained; negotiation waits and
page exchange reject while down. Initialization replays power last. Non-HARD
initialization first checks live power controls and restores PLL control while
IEEE down still holds; its preparation is bounded and shares the same phases.

Parallel reviewers used the available capable model for independent original-PDF,
sibling/ownership and executing-test work. Scopes were disjoint: source/sibling
reviewers read-only, test agent writable only `test/power.cpp`; no agent used Git.
Separate ignored `build/power28-review` and `build/power28-tests` builds checked
reproducers and tests independently. Lead reviewed every integration against
actual source/diff, not just summaries.

Confirmed and fixed during review:

- Partial contradictory BMCR/PHYSCR evidence could leave power trust VERIFIED.
  Each valid contradictory word now dirties trust immediately.
- Reset indications in live/verification/RMW reads could leave identity/readiness
  valid. Valid reset evidence now invalidates knowledge before terminal-error
  handling, including later read/release failure.
- PCS clears BMCR while retaining vendor PLL disable. Added live PLL preparation
  before non-HARD reset/replay. It checks even when an accepted PLL write timed
  out and the cached word still shows zero; a per-operation completion flag avoids
  repeated preparation.
- Executing the power example after cable HARD restoration exposed owner-example
  bookkeeping that discarded a captured silence floor on later profile COMMAND
  phases. The helper now recognizes only the first command phase per reset
  attempt; the complete portable sequence exercises the corrected behavior.

Final maintainability review kept one power source, fixed-size values and explicit
phase transitions. No retry engine, clock policy or duplicated collector was
introduced. The tests remove unused inherited fake state. Public Doxygen, API,
architecture, initialization/reconciliation, health, vectors, feature ledger,
README/CHANGELOG, test guide and progress were updated. SDK/tool wrappers and
version files need no changes: this stage adds no SDK dependency or version bump.

## Verification

Environment: Windows PowerShell, GNU 15.1, Ninja, C++17 Release, Python 3.12.

| Layer/check | Result |
| --- | --- |
| Host configure/build: `cmake -S . -B build/host -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON`; build; `ctest --test-dir build/host --output-on-failure --no-tests=error` | PASS 161/161, including six power suites and the Release failure probe. |
| Separate portable consumer: `cmake -S test/consumer -B build/power28-consumer -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=OFF`; build; CTest | PASS 2/2. Executes policy, IEEE/PLL down, explicit wake and repeated cached views after diagnostic restoration. |
| `python -m unittest discover -s test -p "test_*.py" -v` | PASS 103. |
| `python tools/verify_ledger.py` | PASS 97 registers, 52 procedures, 35 features; structure/source coverage only. |
| `python tools/check_doxygen.py` | PASS 205 HTML pages and two rejected negative fixtures. |
| `python tools/verify_references.py`, version check, `git diff --check` | PASS; archived payloads unchanged, local links checked, generated version current. |
| SDK link / installed-exported package | NOT RUN; no SDK or packaging changes in this portable stage. Separate tests-OFF source consumer is PASS. |
| Remote CI | NOT RUN; reserved for prompt 71. |
| Q6 power/pin/generated-clock qualification | NOT RUN; no fixture operation authorized or claimed. |

Power tests cover all transition frame/release failures, accepted writes followed
by timeout, every PCS initialization/preparation phase, HARD power-tail failures,
invalid zero-I/O requests, readback mismatches, stale/partial evidence, reset
invalidation, pin races, all reset kinds, PLL retention, cancellation at every
PLL entry/exit callback, cleanup denial, separate restoration results, exact
frame counts, contention, zero/small slices, frozen/regressing clocks and overflow.

The lead commits checked task files and synchronizes `main`, then records actual
SHA/upstream equality in the final handoff. Remaining gates are DPD exact exit/
composition evidence, Q6 physical qualification and later SDK integration.
