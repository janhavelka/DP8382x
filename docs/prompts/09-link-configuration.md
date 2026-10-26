# 09 - Implement generic 10/100 link configuration

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-08.

## Fresh audit and owned work

Inspect BME280/TMP1x2 desired/observed/apply state, LDC1614 validation/readback and raw
invalidation tests. Keep Ethernet policy and negotiation-specific semantics.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Work

Implement advertised modes, autonegotiation enable/restart, supported forced modes and
related supported PHY controls using the common configuration procedure. Represent
10/100 and half/full capabilities correctly for the exact mode/variant. Include
symmetric/asymmetric pause advertisement with its full-duplex restrictions; MAC
flow-control policy is separate. Reject invalid or empty advertisement and contradictory
controls before any writes.

Keep generic defaults documented separately from examples. The first application
preserves autonegotiation and advertises 100/full only; the core must not force this
profile or silently fall back to 10. Forced mode and negotiation policy are distinct,
and peer mismatch is observable.

Apply the DP83826I 10BASE-Te VOD_CFG3 setup per the exact datasheet, including when it
is needed, the extended access sequence and reset reapplication. Do not assume the TI
reference driver already does it. Review Auto-MDIX, robust Auto-MDIX and manual MDI/MDIX
controls: SNLS647I sections 8.3.2 and 8.5.1.10 require robust Auto-MDIX in Force Mode.
Define valid combinations and exact sequencing with forced/negotiated operation rather
than changing BMCR alone. Preserve reserved/stable owned bits and explicit ordering with
reset and negotiation.

Track desired settings, observed values and verified configuration independently.
Partial write/readback mismatch invalidates relevant trust; no false rollback or replay.
Configuration changes and restart can drop link and require application ownership.
Bounded negotiation observation may be a separate operation from successful register
application.

Extend reset/recovery restoration to this owned configuration. Document return status,
pending negotiation, unsupported controls and cached snapshots.

## Acceptance and handoff

Cover every legal advertisement bit combination and representative forced modes,
symmetric/asymmetric pause and half-duplex restrictions, invalid masks, mode conflicts,
Auto-MDIX/robust/manual combinations and Force Mode prerequisites, 10BASE-Te
ordering/value, reset restoration, readback mismatch, transport failure at each phase
and no mutation on validation failure. Test 100/full application profile without
restricting generic core tests.

Expected output: Verified general link settings and a separate board-policy profile.
Include source documentation, relevant guides, feature/evidence matrix and progress
updates. Apply COMMON's completion rules; report actual commands, results and remaining
gates. Continue only when the concrete required inputs exist; COMMON explains how
unrelated gates stay open.

Before reporting completion or starting another prompt, commit and sync this
prompt's checked work on the recorded execution branch. Also checkpoint coherent
verified units during the prompt. Follow root AGENTS' first-set CI exception;
do not create empty commits or include unrelated work.
