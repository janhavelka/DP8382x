# 09 - Implement generic 10/100 link configuration

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-08.

Read [CONVENTIONS](CONVENTIONS.md) and restate the applicable stage rules before
coding. Re-audit live sibling source; use simple engineering language, source
Doxygen, affected guides and local checks. Keep the same execution branch and
coherent-unit/per-prompt commit+sync; first-set remote CI belongs only to 29.

## Fresh audit and owned work

Inspect BME280/TMP1x2 desired/observed/apply state, LDC1614 validation/readback and raw
invalidation tests. Keep Ethernet policy and negotiation-specific semantics.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Concrete source pass and implementation boundaries

Read [CONVENTIONS](CONVENTIONS.md). Reopen
`TMP1x2/src/TMP1x2Configuration.cpp::validateSettings`, `beginApply`,
`stepApply`, `verify`, `update` and `src/TMP1x2.cpp::markConfigurationDirty`.
Compare `PCA9555/src/PCA9555.cpp::_invalidateShadowPair`,
`_establishShadowPair` and tests for ambiguous apply, matching verify and raw
invalidation. Adopt desired/observed/verified separation, not sensor settling or
expander safe-direction ordering. Read public Doxygen and CLI diagnostics so
apply failure has one interpretation across source, owner and future console.

Extend existing typed Config/link values and one configuration implementation,
such as `src/DP8382xConfiguration.cpp` when warranted. Small field validators/
encoders need actual consumers; a policy engine or per-field service is not
needed. `startConfigure` uses 08's operation engine; conveniences share its
sequence/deadline. `getSettingsSnapshot` returns desired settings and trust
without reads. Use unit-bearing fields and explicit enums; avoid booleans that
conflate negotiation with forced mode. Stage 04 owns exact signatures.

## Decisions and independent test oracles

1. Make a dependency table for advertisement, pause/asymmetric pause, forced
   speed/duplex, Auto-MDIX/robust/manual crossover and interface. Include selector/
   remote-fault fields and next-page controls owned here; assign remaining
   next-page evidence to 10/14 explicitly. Map every legal encoding to a typed
   input, validator and test so smaller reference drivers do not limit coverage.
2. Validate the whole proposed profile without transfer or committed desired
   mutation. Define when desired settings become authoritative after admission/
   first write and apply that rule on cancellation. Sequential local validation
   must not leave half a proposed profile committed. MAC policy stays separate.
3. Derive ordered writes from SNLS647I, including 10BASE-Te `VOD_CFG3` at
   `0x030E = 0x4A40`, robust Auto-MDIX Force Mode requirements and restart order.
   Record why persistent fields are owned/preserved/reset. Reference drivers
   supply comparisons, not authority for missing procedures or guessed masks.
4. Apply once, verify stable owned bits and publish trust only after complete
   proof. An intermediate match cannot clear earlier mismatch/uncertainty. Never
   automatically replay an ambiguous write or call cache reversal rollback.
5. Extend reset/recover with this same procedure and profile, including the
   10BASE-Te requirement. Keep register application distinct from negotiation
   completion; any requested wait has its own bounded owner-driven semantics.
6. Provide 100/full-only autonegotiation as application data in guide/test
   consumers. Generic core still accepts every documented 10/100 half/full mode
   and does not silently change policy when a partner is incompatible.

Enumerate legal advertisement subsets and invalid/reserved bits with independent
literal ANAR/BMCR expectations, pause combinations and forced modes. Check robust/
manual Auto-MDIX dependencies, 10BASE-Te placement/value, reserved-bit preservation
and no optional EEE/WoL activation. Inject failure after every write and each
readback, checking desired/observed/trusted states separately. Test mismatch
followed by transport failure so mismatch evidence persists. Reset each supported
way and prove the correct owned profile is restored, including 10 Mbit/s.

## Documentation shipped in this stage

Doxygen every field/enum and apply/verify path with legal modes, units, default,
disruption, trust/error behavior and time bounds. Update API/configuration guides
with generic and initial 100/full profiles, a forced-mode example and partial-
apply recovery. Compile examples against implemented declarations. README,
CHANGELOG, ledger and validation distinguish host proof from interoperability.
Run Doxygen/warning/link checks. Keep the same execution branch and coherent-
unit/per-prompt commit+sync; remote CI is only at 29 for this first set.

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
