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

Cancellation before/after each command and incomplete restoration preserve
effects and independent primary/cleanup statuses. Cleanup has the existing finite
allowance; denied/abandoned cleanup remains visible. Local teardown cannot claim
hardware stopped. A callback/adapter user cannot outlive its borrowed resources.
Neither a successful management read nor a reset clears unrelated prior failures
or proves the desired profile restored.

Use independent literal traces per reset kind. Test every trigger, completion,
restore and readback failure, accepted-write timeout, exact waits, wrap, frozen
clock, never-completing command and zero budget. Assert strap/identity/configuration
differences, external notification's zero callbacks, cancellation/end/rebind
evidence and contention without lock retention across waits. Compile the owner
examples and extend the lifecycle/reset-effects guide with exact application
responsibilities and currently supported restoration scope.

## Concrete TI source inputs

Use the [reset audit](../reference/library-ti-core.md#initialization-reset-and-recovery-procedure-audit)
and [timing qualifiers](../reference/library-ti-core.md#timing-silicon-limits-versus-library-policy).
PHYRCR register `0x1F` commands are hard-reset `0x8000` and restart `0x4000`;
Poll `0x1F`; test that out-of-range Clause 22 address `0x8000` is rejected
before a truncating backend could substitute BMCR. SNLS647I
requires external RESET_N low at least 25 us, 2 ms inactivity after deassertion,
and 50 ms after POR before SMI; delayed clock has a separate 100 us stability
condition. The 1500 ms FLP figure is nominal, not a maximum reset deadline.
Recheck exact current documents, software-command applicability and real reset/
clock traces; retain unmeasured physical timing gates and correct source mistakes.

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
