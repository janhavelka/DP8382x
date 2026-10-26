# 17 - Implement negotiated link settings and verified application

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

Prerequisites: [16](16-hard-reset-restart.md), [3](3-silicon-core-ledger.md), [7](7-value-transport-contracts.md); only the concrete inputs used below are required.

## Scope, current evidence and work

Deliver complete autonegotiated link configuration, including supported
advertisement, pause/asymmetric pause, selector and next-page controls and
restart. Extend existing Config/settings and one configuration procedure;
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
Compile generic and 100/full configuration examples and document partial apply.

## Concrete TI source inputs

Reopen [TI negotiation procedures](../reference/library-ti-core.md#negotiation-forced-modes-and-observations)
and current exact datasheet. ANAR technology masks are `0x0100`, `0x0080`,
`0x0040`, `0x0020` for 100/full, 100/half, 10/full, 10/half; selector is `0x001F`,
IEEE selector value 1. Pause/asymmetric pause are `0x0400/0x0800`; next-page is
`0x8000`; local remote-fault `0x2000` remains prohibited. TI's selector-bit0-only
update and empty-advertisement success are not full valid-profile application.
BMCR AN enable/restart are `0x1000/0x0200`; restart-clear is not AN-complete.
Use the catalogue's exact four-frame `0x030E=0x4A40` vendor transaction as an
independent setup vector. Reconcile current source and real advertisement/link
readbacks; library omissions and nominal timing never waive documented controls.

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
