# 17 - Implement negotiated link settings and verified application

Follow [COMMON](COMMON.md) and its shared contracts, including
[CONVENTIONS](CONVENTIONS.md), [EVIDENCE](EVIDENCE.md) and
[FULL_COVERAGE](FULL_COVERAGE.md). Begin with the stage-specific rule reminder:
owned paths, fresh relevant sibling audit, selected names, one MDIO owner,
documentation/local checks and the recorded execution branch. Use available
subagents for disjoint work or independent review as COMMON specifies.

Prerequisites: [16](16-hard-reset-restart.md), [3](3-silicon-core-ledger.md),
[7](7-value-transport-contracts.md). Verify the concrete inputs below; retain unrelated
open gates under COMMON.

## Scope, current evidence and work

Deliver complete autonegotiated link configuration, including supported
advertisement, pause/asymmetric pause, selector and next-page controls and
restart. Implement the documented next-page transmit/receive handoff and bounded
exchange procedure, including message/unformatted data, Acknowledge2, read-only
toggle, page-received evidence and partner inability. Assign a finite page count
and time budget; a next-page-enable bit alone is not complete exchange support.
Use tables 8-25 through 8-27 and retain unresolved protocol facts as gates.
Extend existing Config/settings and one configuration procedure;
`startConfigure` uses the established operation slot. Forced-mode configuration
remains explicitly unavailable until task 18.

Reopen TMP1x2 `src/TMP1x2Configuration.cpp::validateSettings`,
`beginApply`, `stepApply`, `verify`, `update` and
`markConfigurationDirty`. Compare PCA9555 shadow invalidation/establishment
and ambiguous-apply/raw-write/readback tests. Inspect actual public Doxygen and
CLI error rendering; sensor settling and expander direction ordering do not
supply PHY sequences.

Build the exact legal-settings/dependency table from SNLS647I. Reject empty or
reserved advertisement, invalid selectors and contradictory controls before any
transfer or committed desired-state change. Table 8-23 marks ANAR bit 13 R/W but
prohibits local remote-fault advertisement on this part: reject it while retaining
partner/status remote-fault observation. Include all legal pause combinations and
their duplex meaning; MAC flow-control policy remains outside core.

Apply the whole validated profile with source-defined ordering and restart.
Implement required 10BASE-Te setup from section 8.3.13:
`VOD_CFG3 0x030E = 0x4A40`, with exact MMD access, applicability and reset
reapplication. Neither reference-driver omission nor a successful generic status
read removes this requirement. Preserve stable/reserved fields and use mixed
register procedures; do not silently enable EEE/WoL.

Define when admitted desired settings become authoritative. Track desired values,
observed readback and verified trust separately. Apply once and verify all stable
owned bits before publishing trust. Partial writes, cancellation, late completion
or mismatch preserve uncertainty; no automatic retry or fake rollback. Extend
every applicable reset's restoration through this same procedure.

Successful register application differs from negotiation completion. Any requested
negotiation wait uses an explicit finite owner-driven contract. Provide the
initial 100/full-only advertisement as application data; generic core allows all
documented negotiated 10/100 half/full settings without fallback policy.

Test independent literal ANAR/BMCR vectors across legal subsets, pause and
next-page controls, prohibited bit 13 and invalid masks. Assert atomic validation,
10BASE-Te placement/value, exact restart order, reserved preservation and absence
of optional activation. Fail every write/readback and cancel boundary, including
mismatch followed by transport failure; earlier evidence persists. Reset each
applicable way and verify owned negotiated profiles, including 10 Mbit/s.
Test finite multipage exchange, partner refusal, missing/duplicate page evidence,
read-only toggle preservation and failure after a consumed page. Prompt 20 extends
the same collector; prompt 34 only completes quality-related observation.
Compile generic and 100/full configuration examples and document partial apply.

## Concrete TI source inputs

Reopen [TI negotiation procedures](../reference/library-ti-core.md#negotiation-forced-modes-and-observations)
and current exact datasheet. ANAR technology masks are `0x0100`, `0x0080`,
`0x0040`, `0x0020` for 100/full, 100/half, 10/full, 10/half; selector is `0x001F`,
IEEE selector value 1. Pause/asymmetric pause are `0x0400/0x0800`; next-page is
`0x8000`; local remote-fault `0x2000` remains prohibited. Replace the complete owned selector/advertisement fields and reject an empty
technology set; test non-default prior selector bits and all legal page controls.
BMCR AN enable/restart are `0x1000/0x0200`; restart-clear is not AN-complete.
Derive the four-frame `0x030E=0x4A40` setup vector directly from the PDF, then
compare it with the catalogue. Reconcile current source and real advertisement/link
readbacks; library omissions and nominal timing never waive documented controls.

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
