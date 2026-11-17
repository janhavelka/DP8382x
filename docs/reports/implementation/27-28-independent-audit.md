# Independent audit of prompts 27 and 28

## Scope and checkpoint

Fresh audit began on clean, fetched `main`, equal to `origin/main`, at
`cd0420e2846a218b0b2a934f7f2074d33a4568a5`. Re-read both full original
prompts, COMMON and the shared ownership, evidence, coverage and OP-1 through
OP-17 contracts. Compared current code and the stage changes, not prior summaries.
Prompts 1-28 keep their existing implementation scope; this audit does not start 29.

Lead owns production source, public headers, shared guides, progress and Git.
The application remains the sole instance/bus owner. No new manager, background
work, SDK behavior, allocated storage, hardware operation or timing claim was added.
Siblings and original vendor payloads were read-only. Local verification only;
first-set remote CI remains prompt 71.

## Independent reviews and original evidence

Three inherited capable-model reviewers worked in parallel, selected for source,
ownership and partial-failure reasoning. `audit2728_irq` reviewed interrupt code,
original PDF tables and tests, then edited only `test/interrupts.cpp`.
`audit2728_power` reviewed power/reset code and original PDF evidence, then edited
only `test/power.cpp`. `audit2728_coverage` independently checked prompt coverage,
simplicity, shared tails and cross-policy reproducers without tracked edits.
Reviewers used separate ignored `build/audit2728-*` directories; the lead checked
each confirmed finding against actual code/diffs and integrated all production fixes.

The current official [TI datasheet](https://www.ti.com/lit/ds/symlink/dp83826i.pdf)
remains SNLS647I, August 2025, 114 pages. Reviewers reopened original PDF pp28-29,
55, 65-69, 82-83 and 91-92: power prose, BMCR/PHYSCR/BISCR, MISR, pin and RXFS
access. Independent literals still agree with active-sleep policy/state separation,
software/pin power-down OR, reserved preservation, PLL permission, RC consumption
and IRQ selector attribution. No new silicon discrepancy or authorization was found.
`deep_power_exit`, Q6 and Q9 remain open; no wake deadline or physical pin result
was inferred from host callbacks. The existing ledger and literal source vectors
need no numeric correction.

Fresh sibling source/header/CLI/test/runner registration reviews retained the
actual paths documented in [27](27-interrupt-service.md#fresh-sibling-audit) and
[28](28-power-states.md#fresh-sibling-audit). Lead reconfirmed clean worktrees at:

| Reference | Revision |
| --- | --- |
| MB85RC | `3ace168eb01d687ab60e9fe88cc72322e5cae260` |
| INA228 | `4de84f2564eb6382835ca9a72d8688377657dcb4` |
| PCA9555 | `8b461b2afe9910b5b15fe32cc049d52e3b498970` |
| LDC1614 | `da104689de49ab8f492e7905f3eb7496f0866624` |
| SCD41 | `0b9d3d02c27feacf9b1354dc2a8b83a9e5a04879` |
| RV3032-C7 | `15d7a2782f7d6e136d29ff7339c1bee27bc5b8aa` |

Retain staged outputs, passive rendering, explicit permissions and independent
primary/restoration evidence. I2C ACK rules, sensor waits, RTC clear encodings
and sibling wake behavior remain unsuitable as PHY procedure evidence.

## Confirmed findings and corrections

| Finding verified in current implementation | Correction and regression |
| --- | --- |
| Power work lacked an admitted identity snapshot, so a changed identity between polls could later be treated as initialized. | Seed every operation result with its admitted identity. Test changed BASIC/ENHANCED identity at every power boundary. |
| A valid reset word in an inter-poll power observation invalidated readiness without stopping pending IRQ/power work. | Freeze the operation as DEVICE_INVALIDATED immediately. Preserve the separate observation call's later MDIO failure/detail and retained partial words. |
| Final power completion could promote a contradicted earlier policy after verifying only the PLL bit. | Compare the entire retained owned PHYSCR/BMCR tuple before trust promotion. Test final PLL and inter-poll control contradictions. |
| Interrupt test RMW discarded valid contradictory PHYSCR reads when a later write/release failed. | Retain every valid read prefix, dirty stable-policy trust and preserve first mismatch beside the transport error. |
| RXFS contradiction during the UNKNOWN crossing interval could leave a stale selector proof usable by later polls. | Invalidate the pending proof, preserve raw evidence and freeze independent observations immediately. Assert next poll performs no callback/frame. |
| Standalone IRQ/power completion could claim persistentRestored while another owned policy was dirty; test stop also ignored such debt. | Use one small cross-owner proof predicate at admission/completion, with shared reset tails retaining their existing ordered restoration. Both directions reject silently; owning test stop can clear stimulus without claiming restoration. |
| IRQ/power source checks or empty allowance could precede the frozen OP-2 lifecycle/error order. | Separate syntax/budget shape, lifecycle, capability/source and zero allowance. Test unbound, pending-result, denied-policy and invalid-budget combinations without I/O/token changes. |

The tests extend existing registered suites; no new production abstraction or
alternate recovery path was introduced. The redundant second reset check in
power READ_BMCR was removed because the common validated-prefix check already
handles it. Existing complete literal traces, RC retention, failure phases,
finite budgets, cancellation, ownership, reset replay and examples remain tested.
No additional confirmed gap remains in the audited supported procedures.

Public Doxygen and the [interrupt](../../interrupts.md) and
[power](../../power.md) guides now describe inter-poll invalidation, distinct call
errors and cross-policy restoration. CHANGELOG/progress are updated. README,
architecture, variants, ledger and feature counts need no change: supported
features, API signatures, source gates and platform boundaries are unchanged.

## Verification

Windows, GNU 15.1, Ninja, C++17 Release with active test assertions:

- PASS: configured/built `build/host`; all 161 registered CTests.
- PASS: separately configured tests-OFF `build/audit2728-consumer`; both linked
  portable consumers, including compiled/executed interrupt and power examples.
- PASS: 103 Python tests; ledger validator (97 registers, 52 procedures, 35
  features); reference/local-link check (1,679 links in 210 Markdown files);
  Doxygen (205 generated pages and two rejected negative fixtures);
  deterministic version check; `git diff --check`.
- PASS: independent reviewer strict builds and cross-policy reproducer.
- NOT RUN: SDK/controller link and installed-package qualification (unchanged
  boundaries; this audit changes portable behavior only).
- NOT RUN: remote CI (reserved for prompt 71), Q6 physical power/clock and Q9
  interrupt delivery. DPD entry/exit remains BLOCKED by exact-source evidence.

The final commit and verified upstream equality are recorded in the handoff,
avoiding a self-referencing commit ID.

## Current-code response — 2026-11-17

Rechecked all seven findings against `main` entry
`77c544f415edb26a0e23de76591a0163b9b73147`, current prompts 27/28 and OP-2/OP-9.
The original checkpoint and tests remain historical.

| Finding | Current verdict and evidence |
| --- | --- |
| Power operation lacked admitted identity | **Confirmed implemented.** `src/DP8382x.cpp::_beginOperation` seeds `_result.identity` for every operation; `readIdentity` freezes a changed-context operation. `test/power.cpp::auditInterleavedEvidence` covers mode changes at each power boundary. |
| Inter-poll reset left IRQ/power running | **Confirmed implemented.** `_retainEvidence` invalidates valid BMCR/PHYRCR evidence and freezes independent running work before another poll. `test/power.cpp::auditInterleavedEvidence` and `test/interrupts.cpp::auditRegressions` retain a later raw-call error independently from DEVICE_INVALIDATED. |
| Final PLL proof concealed earlier policy contradiction | **Confirmed implemented.** `src/DP8382xPower.cpp::_advancePower`, BISCR completion, checks the full retained PHYSCR F000/BMCR 0800 tuple before trust promotion. `auditInterleavedEvidence` injects contradictory final PLL/independent control observations. |
| IRQ test RMW lost contradictory read prefix | **Confirmed implemented.** `src/DP8382xInterrupts.cpp::_advanceInterrupt` retains each completed PHYSCR read and first mismatch before its error guard. `test/interrupts.cpp::auditRegressions` covers later write and release failures. |
| UNKNOWN crossing window retained invalid selector proof | **Confirmed implemented.** `_retainEvidence` explicitly checks RXFS during VERIFY_SELECT through VERIFY_CONTROL, keeps raw contradiction, dirties trust and freezes an inter-poll pass. `auditRegressions` requires no later callback and ambiguous MISR2 attribution. |
| IRQ/power completion or IRQ stop erased another policy's debt | **Confirmed implemented.** `_otherControlsVerified` is used at admission and completion in both owners; owning IRQ stop can clear test stimulus without reporting persistent restoration. `auditRegressions` and `auditInterleavedEvidence` exercise both directions. |
| Source/zero-budget checks preceded lifecycle | **Confirmed implemented.** `startConfigurePower` and `_admitInterrupt` separate syntax/budget shape, lifecycle, capability/source and nonzero allowance. `test/power.cpp::auditAdmissionOrder` and interrupt audit cases check rejection without token/I/O changes. |
| Complete DPD support inferred from guarded rejection | **Still open.** `_validatePower` returns UNSUPPORTED for DEEP_POWER_DOWN before I/O. Prompt 28 explicitly assigns remaining entry/exit/restoration to `deep_power_exit`; this is not completed software support. |

Original SNLS647I sections 8.3.6 and tables 8-19/35/36/37/40/91 were reopened
directly. They confirm active-sleep 7000/6000 versus BISCR8 indication, BMCR11's
software/pin OR, PLL permission only during IEEE power-down, and RXFS selector
with consuming MISR evidence. They supply no complete DPD exit/0428 preservation
recipe, so reversing entry writes would be unjustified.

A current prompt inconsistency was **corrected here**: prompt 27 previously
assigned mux=4 writes to prompt 32. It now matches `_interruptBatch::PIN_APPLY`
and `_advanceInterrupt::PIN_MUX`, which give narrow IRQ routing/restoration to the
IRQ source owner; prompt 32 owns the LED profile and dependency validation.
`test/interrupts.cpp::pins` and `liveModeRouting` verify this single-writer
arrangement. The shared prompt correction was reviewed against those paths.

Fresh host verification: Windows/GNU C++ 15.1/Ninja, C++17 Release,
`build/audit-current-response-silicon`: **PASS 236/236 CTests**. SDK/package/CI and
physical checks were **NOT RUN** by this review; Q6/Q9 remain open. No new
production defect was confirmed in these supported procedures.
