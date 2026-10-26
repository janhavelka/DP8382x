# 28 - Implement active-sleep policy and explicit power states

Read [COMMON](COMMON.md), [ENGINEERING](ENGINEERING.md),
[CONVENTIONS](CONVENTIONS.md), [FULL_COVERAGE](FULL_COVERAGE.md) and
[VERIFICATION](VERIFICATION.md). Use [SOURCE_MAP](SOURCE_MAP.md) for current
source locations; historical reports do not replace a fresh audit.

At entry record owned paths, applicable rules, current source revisions/dirty
state, selected names, one MDIO owner, local checks and the recorded execution
branch. Resume that branch. Use disjoint implementation/test work or independent
read-only review when useful; choose available models/effort by task complexity
and record the rationale. The lead owns shared contracts, integration and Git.
Siblings remain read-only.

Prerequisites: [22](22-reconcile-recover.md), [5](5-silicon-power-interface-ledger.md); only the concrete inputs used below are required.

## Scope, current evidence and work

Verify the [power facts](../reference/library-ti-features.md#interrupts-power-eee-and-wol)
against SNLS647I 8.3.6 / table 8-40. Active-sleep configuration is PHYSCR
mask/value `0x7000/0x6000`, while BISCR bit 8 is 0 sleeping/1 normal;
the documented 1.4-second NLP interval is not a guaranteed time to find a partner.
IEEE power-down uses BMCR `0x0800`; DPD entry first sets `0x0428[2]`,
then enables IEEE power-down. DPD stops the PLL. Confirm exit timing/order and
clock availability independently; do not assume reversing the two writes is
a complete wake procedure. Recheck conflicts with compliance controls at 0428
and preserve separate policy, observed state and transition validity.

Deliver active-sleep policy plus documented IEEE power-down, deep-power-down,
disable and wake transitions. Keep policy configuration distinct from observed
power state and partner-dependent behavior. Extend one shared configuration/
operation/restore implementation; local `end` remains bus-silent.

Reopen MB85RC `src/MB85RC.cpp::enterSleep`, `wake`,
`_ensureAwakeForI2c` and public sleep/capability declarations. Inspect
SCD41/RV3032-C7 explicit power command implementations/tests and PCA9555
uncertain-effect/cleanup cases. Preserve transition uncertainty, but do not copy
memory wake timings or no-clock policies into the PHY.

Use SNLS647I section 8.3.6.1 precisely: active sleep is enabled by
`PHYSCR[14:12]=110`; hardware enters/wakes according to partner detection,
and BISCR[8] reports current sleep/normal state. Policy configuration can
complete successfully while linked and awake. Do not wait for sleep entry or
promise partner-dependent wake/link timing as a finite software completion gate.

For explicit IEEE/deep power transitions, document exact prerequisites, owned
state, controller access availability, clock/PLL effects, waits, completion
evidence and retained/reset configuration. Validate capability, requested state,
maintenance and timing before I/O. Apply command-neutral writes once and use
owner-driven bounded waits only where source-defined completion exists.
Unknown write effects cannot become known sleep/awake state.

Preserve desired settings and extend reset/recovery/restoration for each power
kind, using stable source-supported verification. Pin, oscillator, GPIO reset,
power rails and application MAC/link policy remain outside core. Power delivery/
PoE is not a PHY library feature. No default low-power activation or autonomous
power cycling; explicit opt-in settings may be replayed by initialization/
recovery when owned.

Cancellation retains accepted effects and performs no MDIO itself. Later cleanup
uses its finite allowance and reports primary/cleanup status separately; failed
wake or restore leaves uncertain state visible. No full-timeout refresh per read,
lock across waits or inferred successful wake from a local setter.

Test literal policy/power sequences, active-sleep enable while linked and awake,
partner-dependent state changes, invalid/unsupported zero-I/O requests and all
reset/power retention differences. Inject every write/read/restore failure,
accepted-effect timeout, never-completing explicit transition, cancellation and
cleanup denial. Verify exact bounds and no false asleep/awake publication.

Compile policy and explicit-transition owner examples. Document source-bounded
software operations separately from unbounded environmental link/wake outcomes
and unperformed physical clock/power measurements.

## Documentation, verification and checkpoint

Ship accurate public Doxygen and procedure citations with this behavior: units,
legal states, ownership, output validity, I/O effects, deadlines and cleanup.
Update API contracts, affected guides, restoration/feature evidence and the stage
report/progress. Review README, CHANGELOG and relevant example/test/tool guides;
update affected claims and record why others need no change. Future APIs and
physical qualification remain explicitly unimplemented/unverified.

Run registered focused host tests with active Release checks, relevant portable
consumer/source-documentation/link checks, the reference checker and
`git diff --check`. Record actual commands and separate PASS/FAIL/NOT RUN/BLOCKED
for host, SDK, packages and hardware where relevant. Missing source evidence
blocks the affected requirement; it cannot become unsupported silicon or close
a required gate. Continue independent eligible work.

Review the final diff for simple control flow, stale comments, duplicated
procedures and undocumented errors. Commit and sync checked tests/docs/code on
the same execution branch after this task and each coherent verified unit.
The lead fetches, safely integrates, pushes and verifies upstream equality;
report exact sync failures. First-execution remote CI dispatch, polling and
waiting belong only to checkpoint **71**, never this task.
