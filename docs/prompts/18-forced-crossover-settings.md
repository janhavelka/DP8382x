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

Construct a legal combination matrix for forced/negotiated mode, speed, duplex,
interface and automatic/robust/manual crossover. Require robust Auto-MDIX when
Auto-MDIX is used in Force Mode under table 8-28. Review manual pair selection
separately against sections 8.3.2 and 8.5.1.25; preserve any unresolved restriction
instead of treating manual MDI/MDIX as automatic crossover. Validate the full request before
MDIO or committing partial desired values. Reject contradictory manual and
automatic controls and unsupported mode/interface encodings explicitly.
Retain full negotiated capability when switching back to autonegotiation.
Include CR2[5] extended full-duplex ability as an explicit opt-in setting with
its source-defined forced-partner behavior. Standard parallel-detection results
must not silently gain that extension or inferred negotiated pause.

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

## Concrete TI source inputs

Use the [forced-mode source audit](../reference/library-ti-core.md#negotiation-forced-modes-and-observations)
with current SNLS647I tables 8-19/28/30/43. Forced speed/duplex contributions are
`0x0000/0x0100/0x2000/0x2100` for 10/half, 10/full, 100/half, 100/full, after
explicitly disabling AN. Preserve reserved bit 6 and test that no 1000-Mbit encoding can enter this PHY's
profile. PHYCR Auto-MDIX/forced-MDIX masks are `0x8000/0x4000`;
CR1 robust Auto-MDIX is `0x0020`; CR3 pair/polarity swap are `0x0020/0x0040`.
Audit robust behavior separately for automatic versus manual pair selection.
Preserve the requested crossover profile through loopback exit and test every
partial write, including restoration of manual MDIX. Verify mode-dependent
behavior against real part/partner evidence; missing fixtures stay explicit.

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
