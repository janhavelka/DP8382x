# 18 - Implement forced modes and coupled crossover settings

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

Prerequisites: [17](17-negotiated-link-settings.md), [16](16-hard-reset-restart.md), [3](3-silicon-core-ledger.md); only the concrete inputs used below are required.

## Scope, current evidence and work

Complete generic forced 10/100 half/full configuration and all documented
Auto-MDIX, robust Auto-MDIX and manual MDI/MDIX controls. Keep these coupled
settings in one validated procedure; changing BMCR alone does not complete
Force Mode. Extend the same configuration/restore implementation and tests.

Reopen TMP1x2 complete-profile validators/application/readback and PCA9555
ambiguous write/trust tests. Inspect source, public declarations and registered
cases rather than copying their Boolean setting patterns. Reopen SNLS647I
sections 8.3.2 and 8.5.1.10, relevant BMCR/PHYCR definitions and 10BASE-Te
requirements. Preserve task 17's next-page/pause/advertisement contract.

Construct a legal combination matrix for forced/negotiated mode, speed, duplex,
interface and automatic/robust/manual crossover. Robust Auto-MDIX is required
in Force Mode by the cited exact-part source. Validate the full request before
MDIO or committing partial desired values. Reject contradictory manual and
automatic controls and unsupported mode/interface encodings explicitly.
Retain full negotiated capability when switching back to autonegotiation.

Implement exact ordered coupled writes, readback and restart/transition behavior
under the existing aggregate operation deadline. Preserve reserved bits, use
command-neutral composition and apply the `0x030E=0x4A40` 10BASE-Te sequence
where required. Forced configuration does not invent negotiated pause evidence,
partner policy or link completion. Symmetric/asymmetric advertisement stays
available for a later return to negotiated operation under the declared profile
semantics; document ownership and validity.

Keep desired/observed/trusted evidence independent through partial apply,
cancellation and error. Restore every newly owned crossover/forced field through
the same reset/recover procedure in the appropriate order. A successful read of
one register cannot clear a previous uncertain or mismatched field. No automatic
retry, silent speed fallback or unrequested forced-mode initialization.

Test literal vectors for every forced speed/duplex, all legal crossover
combinations and representative prohibited dependencies. Exercise transitions
AN-to-forced, forced-to-AN and 10-to-100, profile validation without I/O, exact
10BASE-Te sequencing, stable/reserved preservation and each write/readback
failure. Include accepted writes followed by timeout, cancellation before/after
effects and reset restoration under every applicable kind. Keep transaction
atomicity intact with small poll budgets.

Compile a forced-mode owner example and explain why the initial board's
100/full-only AN profile remains separate. Document that hardware partner
compatibility and electrical timing are not proved by these host tests.
Expose all implemented general modes only after their complete tests and
restoration pass.

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
