# 15 - Implement initialization and complete PCS reset operations

Follow [COMMON](COMMON.md) and its shared contracts, including
[CONVENTIONS](CONVENTIONS.md), [EVIDENCE](EVIDENCE.md) and
[FULL_COVERAGE](FULL_COVERAGE.md). Begin with the stage-specific rule reminder:
owned paths, fresh relevant sibling audit, selected names, one MDIO owner,
documentation/local checks and the recorded execution branch. Use available
subagents for disjoint work or independent review as COMMON specifies.

Prerequisites: [14](14-identity-capabilities.md), [8](8-operation-contracts.md). Verify
the concrete inputs below; retain unrelated open gates under COMMON.

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
waits after callback completion, or after a documented upper bound on that
completion. A caller timestamp captured before the reset write cannot by itself
anchor a post-write minimum wait. Account for quantization, wrap/overflow, exact expiry and frozen-clock
transfer bounds. Return to the owner instead of busy-spinning. The controller
lock covers one transaction and never spans waits/polls.

Accepted reset invalidates appropriate link/configuration/identity knowledge
even if later reads fail. Restore and verify only implemented owned settings
with one source-backed procedure that subsequent feature tasks extend. Initialization
can succeed with cable down. At this stage initialized means the implemented
identity/reset/readiness contract is complete; link-profile application arrives
in prompts 17-18 and must then become part of initialization when requested.
Keep managed-health methods unavailable until prompt 21 implements their
transitions. Reset success alone proves neither link nor electrical qualification.

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
and retains vendor registers. Choose reset bounds from exact silicon/controller evidence and explicit
application policy. Propagate every configuration error and validate the complete
request before RCSR or other mutation. Add independent zero-I/O-invalid-input
and accepted-write/failure cases. Establish
50 ms POR and external-reset readiness responsibility before initial probing;
do not misapply external-pin timing as a proved PCS-reset completion interval.
Keep actual hardware reset/restoration evidence separate from host sequencing.

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
