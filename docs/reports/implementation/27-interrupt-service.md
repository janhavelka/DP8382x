# Prompt 27 - Bounded interrupt configuration and service

Date: 2026-11-15. Branch `main`, clean/fetched/synchronized entry checkpoint
`87aafc01c959d5f811fb7342a037e26c105acac4`. Final commit and upstream equality
are recorded in the handoff. Remote CI remains prompt 71.

## Rules, scope and selected implementation

Read the original prompt and attachment, COMMON, CONVENTIONS, ENGINEERING,
EVIDENCE, FULL_COVERAGE, operation contracts, README, architecture, variants and
reference index. Rechecked current collector, verification/reset replay, source
ledger and parent/root agent rules. Owned procedures are
`interrupt_configuration` and `interrupt_test`. Existing stage20 consumed-prefix
retention and stage22 reconciliation remain the only implementations of those
responsibilities.

The lead owns shared headers/lifecycle/collector, the new cohesive
`src/DP8382xInterrupts.cpp`, CMake, example integration, ledger/guides/progress
and Git. One application owner serializes all instance calls and complete
controller transactions. ISR only signals that owner. Starts/cancel/cached views
are bus/clock silent. Fixed values and one IRQ phase switch extend the existing
operation slot; no event framework, worker, allocation, retry or second collector.

Delivered complete opt-in source/global/output/polarity/selector policy, live pin
admission, retained test start/stop, finite service, pending counter requests and
shared reset replay. Desired/raw observed/trust/test effects remain distinct.
Configuration and replay are managed; service/test sessions are diagnostic RAW.
The [interrupt guide](../../interrupts.md) records precise bounds, partial
evidence, application pin/ISR obligations and physical limitations.

## Exact source evidence

Independent review used original SNLS647I pages 3, 6, 8, 46, 65-68, 81-83 and
92, including tables8-35..37/74..78/91 and the BASIC/ENHANCED pin tables.
The current official [TI datasheet](https://www.ti.com/lit/ds/symlink/dp83826i.pdf)
still serves revision I, August 2025, 114 pages. Vendor payloads/hashes/licenses
remain unchanged and no vendor implementation was copied.

Selected facts: PHYSCR stable mask000B in ENHANCED,000A in BASIC preserving OE;
separate retained test0004; both full MISR maps with FF00 RC and00FF enables;
whole-word RXFS0000/1000 selector writes and consuming readback. A crossing drain
does not retrospectively label pending events. Contradictory RXFS observations,
WoL writes and reset/external uncertainty withdraw attribution.

BASIC pin21 is strap-selected INT; ENHANCED OE0 exposes a power-down input and
needs explicit board safety. Alternate LED routing does not disconnect pin21.
LED1 live GPIO availability and mux4 are verified; ENHANCED MII LED2 requires
declared unused COL. BASIC LED2 rejects. Prompt32 owns mux writes; LED0/3 mux
remain read-only. No unsupported LED drive conflict was invented from the
pre-mux LED generator. Q9, RXFS SFD and remaining pin-source gates stay open.
If HARD removes an externally established alternate mux, IRQ replay rejects and
retains restoration debt. A regression verifies this boundary; prompt32 owns
the missing mux programming/replay, while pin21 replay is complete here.

## Fresh sibling audit

All reviewed sibling worktrees were clean; no sibling changes or fixture actions.

| Source / revision | Actual inspected declarations, implementation, tests/examples and adaptation |
| --- | --- |
| INA228 `4de84f2564eb6382835ca9a72d8688377657dcb4` | `include/INA228/INA228.h::DiagAlert/DiagAlertSnapshot`; `src/INA228.cpp::readDiagAlert`, `_readDiagAlertTracked`, `_captureDiagAlert`, `_writeDiagAlertConfig`; `test/test_basic.cpp` destructive/raw read cases and registration; CLI `printDiagSnapshot`. Adopt central consumed evidence and passive rendering, not sensor-specific neutral values/timing. |
| PCA9555 `8b461b2afe9910b5b15fe32cc049d52e3b498970` | `src/PCA9555.cpp::readInputsAndClearInterrupt`, `clearInterrupts`, `applyInterruptErrataWorkaround`; `test/test_basic.cpp` bounded/exclusive IRQ helper and failed-read retained-output tests, registration/native setup. Keep primary/restoration distinct; reject I2C pointer/ACK cleanup as a PHY rule. |
| LDC1614 `da104689de49ab8f492e7905f3eb7496f0866624` | `include/LDC1614/Config.h::IntbAssertedFn`; `src/LDC1614.cpp::readDataReady/readIntb`; phase/cancel tests, fake status/INTB and registration; CLI `intb`/`intbconfig`; `tools/test_ldc1614_hil_runner.py` wired/skipped fixture cases. Application pin observation and finite acquisition transfer; pin-gated skipping of status does not: PHY service always reads both MISRs. |
| SCD41 `0b9d3d02c27feacf9b1354dc2a8b83a9e5a04879` | Agent/public ISR rules, source/CLI search, `docs/reference/scd41-protocol.md:106`, fixture interruption tests. It has no IRQ command or dedicated IRQ/data-ready pin; no convention was invented from its repository name. |
| RV3032-C7 `15d7a2782f7d6e136d29ff7339c1bee27bc5b8aa` | `src/RV3032.cpp::enableAlarmInterrupt/setTimerInterruptEnabled`, CLI `cmd_alarm_int` and explicit `clearStatus`; `test/test_native/test_datetime.cpp` readback cases and native registration. Adopt explicit owner commands; RTC clear encodings/timing do not transfer. |

## Delegation, corrections and maintainability review

Three inherited capable-model agents supplied independent destructive-source
reasoning, peer/test implementation and lifecycle/deadline review. `irq27_sources`
and `irq27_lifecycle` were read-only. `irq27_peers` first audited siblings then
owned only `test/interrupts.cpp`, using `build/irq27-tests`. The lifecycle reviewer
compiled independent reproducers under `build/irq27-review`; the lead reviewed
actual contributions/diffs and performed all production edits and Git actions.

Confirmed integration corrections, with independent regressions:

- Contradictory RXFS reads withdraw known attribution centrally, even with a
  later release failure. New setting/readback never relabels prior shared events.
- Stable PHYSCR mismatch during test verification marks policy dirty; a mismatch
  only in transient test does not invent a stable-policy mismatch.
- Half-full requests survive budget-limited/failed service and cleared MISRs.
  The single collector clears them only on valid counter consumption or explicit
  host-history discard. The independent reproducer now executes the owed reads.
- Clock-fault and zero-deadline errors retain their exact contract details.
- A changed BASIC/ENHANCED identity during final IRQ replay rejects before further
  mutation. Earlier configuration proof cannot be promoted back to initialized.
  The independent reproducer now returns DEVICE_INVALIDATED and incomplete restore.
- Clarified that pin permission covers pin21 as well as any alternate selected
  route, and corrected old ledger automatic-cleanup wording to separate OP-9 work.

Final review keeps the procedure in one source file and the shared restore tail.
Attribution and pending work are small retained collector values, not parallel
queues. No source gate was closed by a fake or no-op success. README/CHANGELOG,
public Doxygen, API/architecture/device/event/health/initialization/reconciliation
guides, independent vectors, ledger/feature coverage, audit dispositions, example,
test guide and progress are maintained together. Variant support, version,
SDK/build policy and future CLI/runner architecture are unchanged; no unrelated
family implementation or workflow was added.

## Verification and remaining evidence

Environment: Windows PowerShell, GNU C++15.1, Ninja, C++17 Release, active CHECKs
and registered intentional failure probes.

| Layer / command | Result |
| --- | --- |
| Release configure/build `build/host`, registered CTest | PASS: 154/154 |
| Independent `build/irq27-tests` | PASS: full 154/154, plus affected 10/10 IRQ suites/probe after review corrections |
| Tests-OFF `test/consumer` configure/build in `build/irq27-consumer`, CTest | PASS: 2/2; executes IRQ policy/service/cached views/test stop and cable-HARD IRQ replay |
| `python -m unittest discover -s test -p "test_*.py" -v` | PASS: 103 |
| `python tools/check_doxygen.py` | PASS: 201 HTML pages and two rejected negative fixtures |
| `python tools/verify_ledger.py` | PASS: 97 registers, 52 procedures, 35 features; structural coverage |
| `python tools/verify_references.py` | PASS: vendor hashes, extracted references and local links |
| `python scripts/generate_version.py check`, `git diff --check` | PASS |
| SDK link / exported package | NOT RUN; portable implementation and consumer evidence only |
| Remote CI | NOT RUN; first execution reserves it for prompt71 |
| Q9 physical IRQ latching, electrical role, pin delivery and timing | NOT RUN; identified admitted board/analyzer measurements still required |

Host suites cover all sources/simultaneous events, pin/mode combinations, live
mux mismatches, event arrival between reads, every apply/readback/start/stop/
restore failure and cancel boundary, late/invalid callbacks, sentinel outputs,
pending requests, exact work bounds, collector contention and passive repeated
views. The initial board's unwired IRQ does not remove generic software scope.
WoL's later automatic selector change must preserve this same owner; future LED
and power tasks extend its pin and replay rules. No physical support is implied.
