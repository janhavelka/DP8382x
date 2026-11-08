# Prompt 18 source and procedure review

Reviewed on 2026-11-08 for ledger procedures `configure_forced` and
`extended_link_configure`. Scope is exact DP83826I configuration and persistent
replay. The reviewer used gpt-6-astra/high for independent silicon
and sequencing reasoning under AGENTS/COMMON delegation guidance. Only this
report is writable by the reviewer; implementation, shared contracts, ledger,
sibling repositories and Git remain lead-owned. Entry checkpoint was
`57a06cb12140b8f2666e1b1a46c64ffe80f8035c`, on
`promptset/dp8382x-implementation-20261028`.

## Original specification and current provenance

Reopened the [original archived SNLS647I PDF](../../reference/source/TI-DP83826I-E-SNLS647I.pdf),
not just its extracted text. Rendered and visually inspected printed pages 26,
54, 55, 61, 62 and 71: section 8.3.2 and tables 8-19, 8-28, 8-29, 8-30 and
8-43. Also read original page 40, section 8.3.13, for the 10BASE-Te recipe.
Current [TI PDF](https://www.ti.com/lit/ds/symlink/dp83826i.pdf) was downloaded
fresh: 3,366,322 bytes, SHA-256
`4e440fb8bd1cd44ee9ab0773944bef4f0605884f25c4ab96ea55d0ed859fd9c3`,
identical to the archive. Revision remains SNLS647I, August 2025.

Reopened the [TI product page](https://www.ti.com/product/DP83826I) and searched
current TI support/manufacturer material for forced mode, robust Auto-MDIX,
port mirror and extended duplex. No applicable newer exact-part correction was
identified in this bounded search. This does not establish absence of all
errata. PDF renders stayed in system temporary storage. Vendor archives and
their notices/hashes were unchanged.

GitHub API HEAD checks reconfirmed all three prior stage-17 pins:

| Repository | Current immutable revision and reopened files |
| --- | --- |
| [TI RTOS](https://github.com/TexasInstruments/ethernet-rtos-drivers/tree/94bf4343004732983b0b8e1eec123d88ff70453b) | `94bf4343004732983b0b8e1eec123d88ff70453b`; `rtos_drivers/src/dp83826.c`, `dp83826_priv.h`, `generic_phy.c`, and `rtos_drivers/include/dp83826.h`, `generic_phy.h`. Their file headers retain BSD-3-Clause notices. |
| [TI combined software](https://github.com/TexasInstruments/ti-ethernet-software/tree/450af1144b8caeb58724bcbae081b409f6bf5c3a) | `450af1144b8caeb58724bcbae081b409f6bf5c3a`; `zephyr_drivers/dp83826a_driver/driver_files/drivers/ethernet/phy/phy_ti_dp83826x.c`, Apache-2.0. |
| [TI MCU+](https://github.com/TexasInstruments/mcupsdk-core/tree/4b9a66f4e18e02a834961f5f834155c846cab041) | `4b9a66f4e18e02a834961f5f834155c846cab041`; HEAD check only, not a new file-level review. |

RTOS `dp83826.c` lines 229-253 controls PHYCR Auto-MDIX and CR1 robust mode,
forces manual MDI when automatic mode is disabled, and ignores those write
results. Its ordering and policy are comparisons, not a complete forced-profile
contract. `generic_phy.c` lines 1117-1214 checks capabilities and updates speed/
duplex using a generic mask that includes gigabit bit 6. It does not establish
the exact DP83826I coupled profile or disable AN by itself. DP83826I BMCR bit 6
is reserved and must not be treated as a gigabit selector. The private header
corroborates CR1/CR2 bit 5 and PHYCR bits 15/14; it lacks a complete mirror
procedure. Current TI Zephyr lines 584-593 rejects AN-disabled configuration.
No vendor implementation was copied; no GPL payload entered project sources.

## Independently derived literal acceptance matrix

All addresses in this table are Clause 22. Literals were derived from the
original tables before reviewing the stage-18 implementation; they must not be
computed from production constants in test expectations.

| Field | Expected composition and comparison |
| --- | --- |
| BMCR `00`, table 8-19 | Forced 10/half `0000`, 10/full `0100`, 100/half `2000`, 100/full `2100`; stable owned mask `3100`; composition `(old & 4CFF) | requested`. Reset `8000` and restart `0200` are neutral, never replayed from sampled values. |
| CR1 `09`, table 8-28 | Own `0020`, preserve `FFDF`. Every forced profile sets `0020`, including manual MDI and manual MDIX. Negotiated profiles may select robust off/on independently of manual/automatic routing. |
| PHYCR `19`, table 8-43 and 8.3.2 | Automatic sets `8000` and preserves bit `4000`; use preserve `7FFF`. Manual MDI clears `C000`; manual MDIX writes `4000` under mask `C000`; both preserve `3FFF`. |
| CR2 `0A`, table 8-29 | Extended duplex changes `0020` only, preserving `FFDF`, including reserved bit `0040`. No partner-evidence preflight is specified. Forced 10 plus extension is rejected by the project contract. |
| CR3 `0B`, table 8-30 | Coupled port mirror disabled `0000`, enabled `0060`, preserve `FF9F`; project admission rejects independent `0020`/`0040` requests. Retain all fast-link-down and reserved settings. |

For independent preservation tests, start CR1/CR2 at `A5DF`: disabled remains
`A5DF`, enabled becomes `A5FF`. CR3 `A59F` becomes `A5FF` with mirror enabled.
PHYCR `40E5` becomes `C0E5` for automatic, `00E5` for manual MDI, and `40E5`
for manual MDIX. BMCR `D6A5` becomes `44A5/45A5/64A5/65A5` for the four forced
profiles above: the retained loopback/collision/reserved fields test preservation
only and do not authorize operation of temporary loopback.

The legal forced matrix combines four speed/duplex choices, three pair choices,
and mirror off/on, always with robust enabled. Extension adds an opt-in only
for the two forced-100 rows. Forced-100/half plus extension remains representable
in the portable core, with its documented ambiguity; the later native adapter
rejects that policy. Negotiated profiles combine complete advertisement,
three pair choices, robust off/on, mirror off/on and extension off/on.

CR2 extension deliberately changes the normal IEEE outcome with a forced-100
partner: the datasheet describes full duplex when local mode is AN or forced
100. It does not prove the partner agrees, nor establish pause. With extension
clear, parallel detection cannot acquire this override. Forced mode never yields
negotiated pause or partner-duplex evidence. Q11 remains the physical check for
robust/manual routing, mirror interaction and forced PHYSTS interpretation.

## Ordering and evidence contract

Complete pure validation precedes any I/O. Snapshot/read-modify-write/verification
must keep complete transactions atomic under the same application/controller
ownership and finite aggregate operation budget. A short poll slice cannot split
an RMW. Unrelated writable/reserved fields retain their sampled values; command
fields use their source-specific neutral composition.

Initialization already establishes vendor MMD DEVAD 31, wire offset `030E`,
value `4A40` before profile replay. A configure operation does not repeat that
write. Reinitialize using NONE/PCS/HARD/RESTART follows the existing primitive,
readiness and identification rules, then writes/verifies VOD once, then restores
the persistent profile through the same configuration implementation.

Choose a deterministic dependency order: CR1 robust, PHYCR routing, CR3 mirror,
CR2 extension, then BMCR activation, with stable owned verification. AN also
requires the new complete ANAR profile before AN enable/restart; forced mode
contains no advertisement intent and neither touches nor verifies ANAR. The
part does not prescribe a total order among independent companion registers;
the chosen order is project policy that ensures dependencies precede activation.
For AN with robust disabled, defer the CR1 clear until after AN activation: the
previous state can be forced, where clearing robust first violates table 8-28.
Always deferring this case avoids relying on an untrusted previous profile.
No transient next-page state or reset/restart command is persistent replay state.

Each successfully read field supplies only that observation. A later successful
read must not clear an earlier mismatch, failed readback, or accepted/uncertain
write. Retain desired intent, last observed values, and whole-profile trust
separately. After effects, failure/cancellation preserves the primary result and
OP-9 cleanup obligation; the owner admits restoration later with its own token,
budget and result. Before effects there is no invented cleanup obligation.

## Required reconciliations found at entry

The lead owns the corrections and their verification:

- `configure_forced` ledger step describing independent CR3 pair/polarity choices
  conflicts with the stage-18 coupled mirror admission. Replace it with the
  `0060` coupled setting. Include CR2 `0020` in complete-profile ownership.
- `extended_link_configure.cleanup` still describes an automatic restore pass
  within reserved cleanup duration. Replace it with OP-9 barrier and separately
  admitted restoration; keep primary and restoration results distinct.
- `library-ti-core.md` limited the forced robust requirement to automatic mode;
  table 8-28 requires it for forced mode generally. `device-behavior.md` described
  CR3 swaps as separate controls; the admitted profile exposes only coupled
  mirror. Preserve individual field facts in the ledger while correcting the
  public procedure.
- Do not retain stale full-profile trust after any single field read succeeds.
  Configuration success remains separate from link and physical qualification.

Source review is PASS for these exact-part software facts. Implementation and
registered host/consumer checks are owned by the lead and independent execution
review; this report initially makes no passing implementation claim. SDK link,
remote CI and physical qualification are NOT RUN by this reviewer. No fixture
operation or remote CI status query occurred. Q11 remains open; loopback
restoration belongs to prompt 23.

## Independent implementation findings

Read the stage-18 working implementation in `src/DP8382xConfiguration.cpp`,
the initialization/evidence/operation paths in `src/DP8382x.cpp`, and public
`Config.h`/`DP8382x.h`. Two concrete sequencing findings were returned to the
lead before final integration:

1. Clearing robust first for a requested AN/nonrobust profile can leave the
   previous forced mode temporarily without required CR1[5]. The accepted
   correction is the deferred clear described above; preserve all other
   companion controls before activation and verify the complete final profile.
2. BMCR verification originally waited for restart bit `0200` to clear even
   for forced mode. Table 8-19 states this command is ignored with AN disabled.
   Forced verification should use stable `3100` without an AN completion wait;
   both the wait branch and final completion predicate need mode qualification.
   A forced readback with correct stable bits and `0200` high must terminate
   successfully without claiming AN completion.

No additional confirmed correctness defect was found in the reviewed mask
composition, paired-mirror/forced-profile admission, first-mismatch retention,
reset replay entry, or separately admitted cleanup paths.

Rechecked both corrections in the subsequent working implementation. The
READ_CR2 transition skips the early CR1 write for nonrobust intent; pure
validation restricts that intent to AN. APPLY_BMCR then enters APPLY_CR1, whose
next transition is VERIFY_ANAR, so the clear occurs once after AN activation
and does not form a loop. Robust profiles retain the original dependency order.
VERIFY_BMCR gates restart waiting to AN, and its final completion predicate
explicitly accepts forced mode independently of `0200`. Forced configuration
does not set negotiation-complete or the AN restart-complete result flag.
Both reported findings are resolved on source recheck. Implementation source
review is PASS; executing regression results remain attributed to the lead's
test runs, not to this read-only reviewer.

There is no new essential software source gate from this review. Q11 remains
NOT RUN physical qualification for forced PHYSTS interpretation, robust manual
routing and mirror interaction. Other stages' independent gates remain unchanged.
