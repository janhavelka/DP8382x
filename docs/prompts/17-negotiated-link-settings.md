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

Deliver autonegotiated advertisement/pause and IEEE Clause 28 next-page exchange,
which SNLS647I 8.3.1 designates. Implement ledger `next_page_exchange`; freeze
its typed page API here, including message/unformatted data, Acknowledge2,
read-only toggle, partner refusal and finite page/time budgets. Extend prompt
13's collector for ANER page evidence. `next_page_handshake` is physical Q10,
not a software block. `startConfigure` uses the existing operation slot;
forced configuration remains prompt 18.

Reopen TMP1x2 `src/TMP1x2Configuration.cpp::validateSettings`,
`beginApply`, `stepApply`, `verify`, `update` and
`markConfigurationDirty`. Compare PCA9555 shadow invalidation/establishment
and ambiguous-apply/raw-write/readback tests. Inspect actual public Doxygen and
CLI error rendering; sensor settling and expander direction ordering do not
supply PHY sequences.

Reject empty/reserved technology advertisement and contradictory settings before
I/O or desired-state change. Always encode selector `0001`, overwriting and
verifying any prior non-default selector. ANAR bit 13 remains prohibited despite
its R/W label; partner remote-fault observation remains supported. Cover every
legal pause combination; MAC flow-control policy stays outside the core.

Apply the complete admitted profile in source order. `startConfigure` requires
initialized, including prompt 15's unconditional VOD_CFG3 setup; add no second
`030E=4A40` write here. Preserve target-specific reserved bits and command-neutral
values. Do not silently enable EEE/WoL.

Define when admitted desired settings become authoritative and retain desired,
observed and verified state separately. Until prompt 18, trust covers ANAR,
BMCR and stage-15 VOD_CFG3 only. Crossover and extended duplex are not yet applied;
non-default requests reject at preflight. Partial writes, cancellation, late
completion or mismatch keep uncertainty and OP-9's restoration obligation.
Extend the same initialization replay, with no retry or fake rollback.

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

NP=1 belongs only to the typed exchange epoch, never persistent `Advertisement`.
It obliges page service on each AN cycle. On success clear ANAR NP without
restart, because AN has completed. On timeout/cancel/failure terminate at once
with `cleanupOwed` and report NP may remain set; issue no extra write. A later
`startInitialize`, or eligible CONFIGURE retry, replays NP=0 and restarts AN.
Reset replay never writes NP=1. Test re-AN with NP=1 but no page load as explicit
pending/stalled negotiation, never silently down. The ledger is the field
catalogue; use independent original-source vectors for named production masks.

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
