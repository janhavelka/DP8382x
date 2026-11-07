# 65 - Implement exact DP83822IF/HF fiber behavior

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md),
[FULL_COVERAGE](FULL_COVERAGE.md) and [VERIFICATION](VERIFICATION.md). Required
inputs are source-backed subtype/medium facts from
[62](62-p22-source-subtypes.md), shared reset/owner behavior from
[63](63-p22-copper-core.md) and interface controls from
[64](64-p22-interface-clock.md). Missing optical fixtures limit physical
evidence, not documented portable software scope.

## Current evidence and ownership

Reopen MB85RC exact variant capability/dispatch tests, TMP1x2 variant admission
tests and TUSB32x actual profile validators for incompatible combinations.
Inspect current public/source, SDK MAC admission, CLI/runner medium selection,
tests/check registration and AGENTS. Record revisions, dirty state, exact paths
and adopted/adapted/rejected conventions. Recheck SNLS505H fiber sections and
exact IF/HF source applicability rather than inferring from copper paths.

The lead owns shared medium/configuration types, capability data and
integration.

## Concrete source checks

Recheck [fiber controls and source
conflicts](../reference/library-family.md#fiber-reset-diagnostic-and-power-procedures)
against SNLS505H: CR2 `0x000A[14]` FX, FIBER_GENCFG `0x0465[0]` detect polarity, SOR1
`0x0467` strap fields `[11:10]`/`[9:8]`. Enabling FX must set CR2 bit 14 with
literal mask `0x4000` while preserving unrelated fields; an unshifted value `1`
is not a bit-14 write. Derive the exact update from SNLS505H and test it with
independent literal traces. A software enable does not prove signal-detect
wiring or optical medium. Include exact supported forced duplex modes and
100-Mbit FX behavior without AN, then validate interface independence and
copper-recipe exclusion. Any unresolved essential control meaning remains OPEN;
real optical evidence is reported separately.

## Required medium and fiber observation contract

Add Config medium COPPER/FIBER, per-medium link masks, observed CR2[14] medium
with SOR1 evidence, CLI grammar and adapter admission. SD is strap-only:
`sdActive = SOR1[11:10] in {01,10} AND SOR1[9:8] in {10,11}`. CR2[14] alone
never activates SD; RX_ER remains AMDIX_EN unless FX_EN is strapped. Name
`FIBER_GENCFG` at `0x0465` separately from `GENCFG` at `0x0456`; bit 0 controls
SD polarity only (1 active low). Report FX source and SD state separately.
Without active SD electrical-link loss needs the inferred digital restart path.

SNLA437A 2.3.4 says FX BMSR[2] does not toggle with link changes; infer the same
for PHYSTS[0] from table 8-16 and mark it not change-tracked. Record the gap with
table 8-2 and SD link-drop table 8-83. Provide explicit maintenance-admitted
refresh: literal `0x001F=0x4000`, application-bounded wait (no documented
duration), then two collector BMSR reads, keeping both and using the second.
Never refresh from polling, snapshots or SDK timer. Test fixed BMSR and a
latched-low first sample; physical Q14 remains NOT RUN.

Fiber writes no CR1[6:5], PHYCR[15:14] or CR3[6:5]. Add `0x0040` Fiber Far-End
Fault bits 13/6/5 by preserve-reserved RMW, and LDCTRL[11:8] FX swing with 66.
Cite section 4 for FX parts and 7.4.1.1/.2 for MII/RMII. FX+RGMII is not stated
or excluded; table 7-10 RX_ER mode 3 can encode it, physical NOT RUN.

## Work and acceptance

Implement documented IF/HF 100BASE-FX medium selection, startup/configuration,
legal link modes, status/resolution and reset/restoration. Preserve observed ID
versus board-declared subtype and physical medium evidence. Copper-only I/H
reject unavailable fiber selections before callbacks. Do not infer optical
hardware or suffix from shared PHY IDs.

Use the exact medium/interface applicability above. Expose source-proved
fiber through a supported MAC interface; do not require a P4 RGMII build or
erase generic PHY interfaces because one adapter cannot use them. SNLS505H 7.4.6
excludes 100BASE-FX from autonegotiation. Implement its documented
forced-mode/link semantics explicitly; reject incompatible negotiation requests
and avoid copper advertisement, MDIX, TDR or 10BASE-Te recipes by analogy.
Reconcile interrupt/status/counter, power and diagnostic applicability with the
following feature tasks, preserving source-proved shared routines rather than
duplicating the driver.

Register independent literal SNLS505H vectors for every legal IF/HF
medium/interface/mode row and incompatible subtype/medium/interface selection.
Cover invalid enums, shared/unknown/revision-mismatched IDs, no-link and
incomplete observations. Pure preflight rejection causes zero callbacks; an
identity-based mismatch allows only bounded observation reads. Verify
copper-only recipes never execute in fiber, and trace media changes plus
reset/restoration of owned fields. Apply VERIFICATION's transfer,
partial-configuration, reset, deadline/cancel, cleanup and cached-state cases to
changed and inherited fiber paths. Optical link loss remains separate from MDIO
transport health.

Reconcile adapter admission, CLI help/results and fixture suite eligibility.
Update Doxygen and variants/API/interface/optical-fixture guidance with exact
medium effects and restrictions. Run focused host/CLI/Python/docs checks, real
supported SDK links and existing copper/family regressions. Record exact
untested optical rows and complete COMMON's report and checkpoint.
