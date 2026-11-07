# 16 - Implement hard reset, restart and external-reset reconciliation

Follow [COMMON](COMMON.md) and its shared contracts, including
[CONVENTIONS](CONVENTIONS.md), [EVIDENCE](EVIDENCE.md) and
[FULL_COVERAGE](FULL_COVERAGE.md). Begin with the stage-specific rule reminder:
owned paths, fresh relevant sibling audit, selected names, one MDIO owner,
documentation/local checks and the recorded execution branch. Use available
subagents for disjoint work or independent review as COMMON specifies.

Prerequisites: [15](15-initialize-pcs-reset.md), [3](3-silicon-core-ledger.md). Verify
the concrete inputs below; retain unrelated open gates under COMMON.

## Scope, current evidence and work

Extend the existing lifecycle with complete PHYRCR hard-reset and restart
procedures and bus-silent external-reset/POR notification. This task owns each
kind's full completion, restoration and failure behavior; hardware GPIO/power
operation stays with the application.

Reopen TMP1x2 reset/recovery operation phases, PCA9555 primary/cleanup and detach
tests, TCA9548A bus-silent `end`, and their public lifecycle contracts.
Recheck SNLS647I BMCR/PHYRCR/strap/reset sections and independent reset vectors.
Keep register addresses and command masks distinct in code and tests.

Maintain a reset-effects table with trigger, source-defined terminology, minimum
wait, completion evidence, retained/invalidated settings, identity/mode effects,
strap resampling and restore ownership. Distinguish BMCR PCS reset, PHYRCR hard
reset, PHYRCR restart, external reset and POR, including POR-only straps. No
ambiguous Boolean reset option or inferred interchangeable behavior.

Implement each newly supported software kind through task 15's operation slot,
deadlines/tokens/results and shared register procedures. Validate before I/O,
issue the correct command-neutral composition once, wait cooperatively and verify
completion under one aggregate primary deadline. Release transaction ownership
before waits. Re-read/reselect identity when effects require it; stale mode-derived
capabilities cannot survive a reset that invalidated them.

Restore validated desired owned state in exact reset-specific order even if the
current hardware image was untrusted. Verification compares stable owned fields,
not self-clearing commands/status. New configuration/features later extend this
same restore path rather than creating alternate reset implementations. Explicit
external notification records uncertainty and changed knowledge without touching
pins, resetting hardware again or performing implicit recovery.

Cancellation and incomplete restoration freeze the current result and retain
accepted/possible effects. OP-9 protects them with the barrier; restoration is a
new operation with its own budget and result. Use `startInitialize({HARD})` and
`startInitialize({RESTART})`, never separate reset/recover aliases. Local teardown
cannot claim hardware stopped or outlive borrowed callbacks.

Use independent literal traces per reset kind. Test every trigger, completion,
restore and readback failure, accepted-write timeout, exact waits, uint64 overflow and clock regression, frozen
clock, never-completing command and zero budget. Assert strap/identity/configuration
differences, external notification's zero callbacks, cancellation/end/rebind
evidence and contention without lock retention across waits. Compile the owner
examples and extend the lifecycle/reset-effects guide with exact application
responsibilities and currently supported restoration scope.

## Concrete TI source inputs

Use single literal words `W(1F,8000)` for HARD and `W(1F,4000)` for RESTART,
not sampled-word RMW. For any attempted HARD write, schedule every next frame
no earlier than completion upper bound plus `max(2000, pollIntervalUs)` us.
Publish `busQuietUntilUs` and apply OP-6 admission and OP-14's persistent floor,
including failure, cancellation, take and later operations. The bus owner holds
all other PHYs silent too. Then poll `1F` mask `8000`; 2 ms is minimum silence,
not a completion maximum or proof of the undocumented internal release time.
Q5 qualifies it. PCS and RESTART have no such floor. Test 1000 us polling and
out-of-range C22 address `8000` rejection before truncation.

PHYRCR hard reset's best reading is eligible strap resampling (8.4.1 and tables
8-47/48); the software-reset wording remains arguable. Keep `hard_reset_straps`
open, report possible resample, and qualify in Q5. ENHANCED Strap1/Strap6 are not
re-latched by reset; BMCR reset reloads stored values. `invalidateDeviceState`
adds EXTERNAL_POWER_ON to distinguish POR, which samples all straps, from
EXTERNAL_RESET. Do not assert a POR-only restriction for BASIC.

RESET_N low is at least 25 us without debounce capacitors. Apply prompt 15's
50 ms-from-final-supplies and delayed-clock readiness rule, plus 2 ms after
release. FLP 1500 ms is nominal, not a reset deadline. Keep all physical claims
NOT RUN until measured.

## Documentation, verification and checkpoint

Update public Doxygen, procedure citations, affected API/user/test/tool guides,
coverage, stage report and progress with this behavior. Review README/CHANGELOG
and record relevant no-change reasons. Run the focused registered host tests
above with active Release checks, affected portable-consumer and Doxygen checks,
`python tools/verify_references.py` and `git diff --check`.

Apply COMMON's final maintainability review and evidence reporting. Keep source
gaps, SDK/package results and physical qualification separate. The lead commits
and syncs this complete task and coherent verified units on the same execution
branch, verifies upstream equality, and reports any sync failure. First-set
remote CI runs and status checks belong only to prompt 71.
