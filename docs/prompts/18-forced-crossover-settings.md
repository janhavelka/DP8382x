# 18 - Implement forced modes and coupled crossover settings

Follow [COMMON](COMMON.md) and its shared contracts, including
[CONVENTIONS](CONVENTIONS.md), [EVIDENCE](EVIDENCE.md) and
[FULL_COVERAGE](FULL_COVERAGE.md). Begin with the stage-specific rule reminder:
owned paths, fresh relevant sibling audit, selected names, one MDIO owner,
documentation/local checks and the recorded execution branch. Use available
subagents for disjoint work or independent review as COMMON specifies.

Prerequisites: [17](17-negotiated-link-settings.md), [16](16-hard-reset-restart.md),
[3](3-silicon-core-ledger.md). Verify the concrete inputs below; retain unrelated open
gates under COMMON.

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

Build the legal forced/negotiated speed, duplex and routing matrix. Every forced
profile writes CR1[5]=1, including manual MDI/MDIX (table 8-28). PHYCR[15:14]
selects automatic (`8000`, bit 14 preserved), MDI (`0000`) or MDIX (`4000`).
Forced manual plus robust is legal; qualify routing physically. CR2[5] extended
full duplex is an owner opt-in with no invented partner-evidence preflight.
Change only mask `0020`, preserving other bits. Reject forced-10 plus extension.
Document the altered IEEE result against a forced-100 partner. With extension
clear, parallel detection cannot gain it; never infer negotiated pause.

Apply ordered writes/readback under the operation deadline, preserving reserved
bits with source-specific neutral composition. Initialization already established
`030E=4A40`; do not write it a second time here. A forced profile carries no AN
intent and neither writes nor verifies ANAR. Returning to AN requires a new
complete Advertisement. Forced mode provides no negotiated pause or partner
duplex proof.

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

## Concrete TI source inputs

CR3 `0060` is port mirror (table 8-30), not two independently justified corrections;
include it in the matrix, Doxygen and vectors. Physical qualification Q11 covers
robust manual routing and mirror interaction. Loopback restoration tests belong
to prompt 23, when loopback is implemented.

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
