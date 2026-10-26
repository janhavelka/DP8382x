# 15 - Implement initialization and complete PCS reset operations

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

Prerequisites: [14](14-identity-capabilities.md), [8](8-operation-contracts.md); only the concrete inputs used below are required.

## Scope, current evidence and work

Deliver initialization and BMCR PCS reset as complete caller-driven operations,
including cancellation, retained results, teardown and bounded restoration of
currently implemented owned state. Use the existing facade and one explicit
operation-kind/phase switch; no standalone generic job engine.

Reopen TMP1x2 `OperationResult`, `OperationSnapshot` and
`src/TMP1x2Operations.cpp::admit`, `poll`, `finishJob`,
`cancel`, `takeResult`. Inspect PCA9555 cancel/required-cleanup,
detach-refusal and shared-remaining-deadline tests; compare 24Cxx retained effects
across waits. Reject EEPROM ACK polling and sensor conversion delays.

Implement `startInitialize`, PCS `startReset`, `pollOperation`,
`takeResult`, `cancel`, `getOperationSnapshot` and necessary local
lifecycle behavior using task 8's exact signatures. Validate admission, binding,
result availability, kind, token and time range without I/O. Admission is not
completion. One active slot and monotonic nonzero tokens with explicit exhaustion
protect against stale results; unconsumed terminal evidence is never overwritten.

Use SNLS647I's exact BMCR effects, completion evidence and waits. Anchor minimum
waits after callback completion, or use a conservative documented caller-time
anchor. Account for quantization, wrap/overflow, exact expiry and frozen-clock
transfer bounds. Return to the owner instead of busy-spinning. The controller
lock covers one transaction and never spans waits/polls.

Accepted reset invalidates appropriate link/configuration/identity knowledge
even if later reads fail. Restore and verify only implemented owned settings
with one source-backed procedure that subsequent feature tasks extend. Initialization
can succeed with cable down. Reset success alone proves neither link nor
electrical qualification.

Cancel/end/cached access remain bus-silent. Cancellation preserves accepted writes;
later owner polling performs cleanup only within the reserved/separately admitted
finite allowance. Never silently extend the primary deadline or replace its error
with cleanup success. Teardown/rebind cannot erase unresolved hardware effects or
release context still used by callbacks/SDK. Later reset kinds are not exposed
as implemented before task 16. Omit their public options until then, or reject
requests before I/O with an explicit software/lifecycle limitation; never call
a known silicon feature unsupported merely because its software is unfinished.

Test literal reset traces, sticky/never-clearing reset, every write/read/restore
failure, no-clock/wrap/overrun boundaries, zero poll budget and cancellation at
each effect boundary. Test wrong/stale tokens, repeated take/end, result-pending
rebind, exhaustion and reuse after consumption. Competing access cannot split
MMD restoration. Add a compiling owner loop and failed-cleanup example.

## Concrete TI source inputs

Re-audit [TI initialization/reset behavior](../reference/library-ti-core.md#initialization-reset-and-recovery-procedure-audit)
against SNLS647I table 8-19: PCS reset uses register `0x00`, command `0x8000`,
and retains vendor registers. TI supplies no reset wait/deadline here; choose
bounds from exact silicon/controller evidence and explicit application policy.
Its empty InitCfg and Auto-MDIX/interface config are not complete initialization;
invalid config can still cause RCSR I/O, and configuration errors are ignored.
Add independent zero-I/O-invalid-input and accepted-write/failure cases. Establish
50 ms POR and external-reset readiness responsibility before initial probing;
do not misapply external-pin timing as a proved PCS-reset completion interval.
Keep actual hardware reset/restoration evidence separate from host sequencing.

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
