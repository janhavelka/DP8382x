# Prompt 19 independent source review

Reviewed on 2026-11-10 for `observe_link`, the stage-19 ledger procedure.
Entry checkpoint: `8ee2efd77c27eb03dce069e1a0c98bd7fcb1440c`, branch
`promptset/dp8382x-implementation-20261028`. The lead owns implementation,
shared contracts, sibling audit, integration and Git. This delegated reviewer
owns only this report. The lead selected gpt-6-astra/high for independent silicon,
destructive-read and coherence reasoning under AGENTS/COMMON. No fixture, SDK
build or remote CI operation was performed.

Read parent/root AGENTS, COMMON, prompt 19, EVIDENCE, README, architecture,
variants, reference index, API live-link contract, stage-18 source review,
the `observe_link` ledger and its pause matrix. These are prerequisites and
project policies; original exact-part documents remain silicon authority.

## Sources and provenance

Freshly retrieved the [official TI datasheet](https://www.ti.com/lit/ds/symlink/dp83826i.pdf).
It remains SNLS647I, August 2025: 3,366,322 bytes, SHA-256
`4e440fb8bd1cd44ee9ab0773944bef4f0605884f25c4ab96ea55d0ed859fd9c3`.
Bytes match the [archived original](../../reference/source/TI-DP83826I-E-SNLS647I.pdf).
Rendered and visually inspected original printed pages 54-59, 61, 64-65 and 71,
including access table 8-18 and register tables 8-19/20/23/24/25/29/34/43.
Rechecked the [TI product page](https://www.ti.com/product/DP83826I) and bounded
TI searches for pause, PHYSTS/BMSR and extended-duplex corrections. No newer
applicable correction was identified; this is not proof of absence of unpublished
errata. Vendor archive and provenance files were not changed.

Reopened the [identified IEEE original-document mirror](https://www.my-wire.de/schnittstellenbeschreibung/ethernet.pdf):
IEEE Std 802.3-2012, Section Two, 20,249,231 bytes, SHA-256
`29774aba6d5b82524f4323214b1035603b0aa901c5f971d770049776b0a6c2af`.
This matches the prompt-3/17 pin. Visually inspected original printed page 732,
Table 28B-3; PDF zero-based page 1365. This is an identified IEEE document from
a nonpublisher mirror, not a claim to the latest standard edition. Downloads
were compared in memory; renderings were temporary and outside the repository.
No copyrighted implementation or document image was added to firmware sources.

Re-fetched actual comparison source at immutable commits:

| Source | Inspected path and scope | SHA-256 |
| --- | --- | --- |
| [TI RTOS](https://github.com/TexasInstruments/ethernet-rtos-drivers/tree/94bf4343004732983b0b8e1eec123d88ff70453b), `94bf4343004732983b0b8e1eec123d88ff70453b`; GitHub HEAD recheck unchanged | `rtos_drivers/src/generic_phy.c`, lines 1217-1267, `GenericPhy_isLinkUp` | `61fc91d936a008ec40237c06e1f9681cfda8e463de1e53df2698bfde2e0be447` |
| Same TI revision | `rtos_drivers/src/dp83826.c`, lines 395-450, `Dp83826_getSpeedDuplex` | `600732cd0e3e197cb9e01f26761e2574b9527142ecee34e95361511e153df5ad` |
| Same TI revision | `rtos_drivers/src/dp83826_priv.h`, lines 120-127, PHYSTS constants | `be7802418e285b50de8420953d3c9a9923acb0c489a505a799e61da9fe22764f` |
| [Selected ESP-IDF v6.1](https://github.com/espressif/esp-idf/blob/fff9895c82d744c7237be8847347bdd1b07c6643/components/esp_eth/src/phy/esp_eth_phy_802_3.c), `fff9895c82d744c7237be8847347bdd1b07c6643` | `components/esp_eth/src/phy/esp_eth_phy_802_3.c`, lines 217-305, link and pause projection | `17d57ed4e6dcb71ba6d47d2e4e446ec48c392beeda2524a31b83b921e701790a` |

TI's generic helper still reads BMSR twice only when the first link bit is high;
first-low returns success without assigning the output. Reject that procedure.
TI's chip helper correctly maps PHYSTS polarity but applies it without an
AN-disabled qualification and has no aggregate consistency check. Adopt the
numeric mappings only where the datasheet says they apply. ESP-IDF's selected
generic helper observes one BMSR, updates mode only on a link transition and
projects partner symmetric pause into one Boolean. These are SDK behavior facts,
not sufficient portable evidence. SDK callback ownership remains later work.
The TI files retain BSD-3-Clause notices and the SDK file Apache-2.0; no source
implementation was copied.

## Exact register facts and independent literals

All addresses below are Clause 22 registers, not MMD offsets. RH means hardware
can set/clear a field, not automatically read-to-clear; original field prose can
identify a separate clearing register even when an access cell says RC.

| Register | Required fields and interpretation | Original source |
| --- | --- | --- |
| BMCR `0x00` | AN enable `0x1000`; forced speed `0x2000` means 100 Mbit/s, zero means 10; forced duplex `0x0100` means full, zero means half. Speed/duplex controls apply with AN disabled. | Tables 8-19, pp54-55 |
| BMSR `0x01` | Link `0x0004` is latched history cleared by read; AN complete `0x0020` is RH. Remote fault `0x0010` is consumed by a BMSR read. Always retain first sample before acquiring current sample. | Table 8-20, pp55-56 |
| ANAR `0x04`, ALNPAR `0x05` | Symmetric pause `0x0400`, asymmetric `0x0800`; selector mask `0x001f`, IEEE selector `0x0001`; technologies 100/full `0x0100`, 100/half `0x0080`, 10/full `0x0040`, 10/half `0x0020`. ALNPAR is the datasheet register heading; prose also uses ANLPAR. | Tables 8-23/24, pp57-58 |
| ANER `0x06` | Partner AN ability `0x0001`; page received `0x0002`; parallel detection fault `0x0010`. ANER read clears PHYSTS page-received mirror `0x0100` per the PHYSTS prose, despite ANER's RH access cell. Route through the existing collector. | Tables 8-25/34, pp59/64 |
| CR2 `0x0a` | Extended full duplex `0x0020`; applies when local is AN or forced 100 and partner is forced 100. It is partner-dependent, not unconditional local full-duplex intent. | Table 8-29, p61 |
| PHYSTS `0x10` | Link `0x0001`; AN complete `0x0010`; full duplex `0x0004`; speed `0x0002` means 10, zero means 100. Bits 2:1 have forced-mode strap/fixed-one caveats and cannot be used as universal forced results. | Table 8-34, pp64-65 |
| PHYCR `0x19` | RX pause `0x2000`, TX pause `0x1000`, both positive indications. They describe eligible full-duplex AN pause; they are not inverted and are not configuration commands. | Table 8-43, p71 |

PHYSTS has its own RC fields at bits 10/9 (`0x0600`). Its mirrors do not all
clear on reading PHYSTS: receive error clears through RECR, polarity through
10BTSCR, false carrier through FCSCR, page through ANER, interrupt through MISR,
remote fault through BMSR; jabber is explicitly not cleared by PHYSTS. Preserve
these ownership distinctions and raw words, even if mode decoding ignores them.

AN-qualified PHYSTS low literals `0x0001`, `0x0005`, `0x0003`, `0x0007` mean
100/half, 100/full, 10/half, 10/full respectively. Add `0x0010` for complete AN.
Forced BMCR `0x2000` with PHYSTS `0x0007` remains local forced 100/half when
CR2 extension is known zero; the PHYSTS forced caveat is why these values differ.

## Ten reads and bounded consistency

The project's exact successful plan is:

1. One eight-read batch: BMSR first, BMSR current, PHYSTS first, BMCR, ANAR,
   ALNPAR, ANER, PHYCR.
2. One two-read batch: BMSR final, PHYSTS final.

This order is a project acquisition contract justified by register semantics,
not a silicon-mandated atomic snapshot. Reserve all ten frames and both bounded
callbacks against one aggregate deadline before entering the first callback.
Every validated consumed BMSR/PHYSTS/ANER sample goes to the existing collector
immediately. A later failure cannot undo its evidence or justify reacquiring
it through another owner. A possible completed destructive failed read retains
history-loss uncertainty. Ordinary output publishes only after all checks pass.

Independent consistency predicates are:

- Compare BMSR current and final under `0x0024` (link plus AN completion).
- Compare PHYSTS first and final under `0x0011` (link plus AN completion).
- In each corresponding pair, compare BMSR link `0x0004` to PHYSTS link
  `0x0001`, and BMSR AN `0x0020` to PHYSTS AN `0x0010`.
- When using AN-resolved PHYSTS mode, also compare first/final PHYSTS under
  `0x0006`, giving a combined PHYSTS mask `0x0017`. Pending/down/forced states
  do not qualify these mode fields and must not derive validity from them.

Do not compare complete register words: unrelated event fields can legitimately
change or clear during the prescribed reads. Do not include BMSR first in the
current-equality rule: first-low/current-high is the purpose of latch clearing.
Current-high/final-low detects a loss after the current read even if the cable
has recovered by PHYSTS final. Current-low/final-low plus PHYSTS disagreement
is uncertainty, not proof of a stable down state. Same-valued endpoints cannot
prove no unobserved transitions occurred between reads. No two-batch software
plan can provide simultaneous sampling; document the bounded consistency claim.

Detected inconsistency returns `BUSY/OBSERVATION_CHANGED`, leaves caller output
unchanged and marks previous cached observation stale with its old time. There
is no automatic retry. The ledger entry at review start still said boundedly
reacquire; reconcile it to this explicit API/prompt contract. Read completion
time must be a valid observation time, never a zero/default promoted to fresh
data. Timestamp overflow/regression are software validation cases, not PHY
timing values. No silicon minimum/maximum was inferred for this read plan.

## Resolution gates

Current link is independent of resolved mode. Down and pending AN can be valid
observations with unknown mode; successful cable-down transport remains healthy.
For normal completed AN, qualify PHYSTS mode using the coherent completion/link
evidence and applicable partner/base-page evidence. ANER parallel-detection fault
is insufficient resolution evidence. For normal negotiated pause, require a
full-duplex common technology and valid IEEE base-page interpretation; stale
partner bits must not manufacture a pause result for a non-AN partner.

With AN disabled, use observed BMCR for local speed/duplex, never PHYSTS2:1.
The forced100/half exception remains unresolved if CR2[5] is requested one or
whole-profile trust is not VERIFIED: speed can be 100, duplex UNKNOWN, mode
invalid. A desired false alone is not known-zero hardware. Because the ten-read
plan does not include CR2, only prior valid verified configuration evidence can
establish this dependency; do not silently add an eleventh read. Forced full or
forced 10 is not affected by this specific ambiguity. All forced pause is invalid.
This says local mode only; partner duplex is not established. Q11 still owns
physical strap/readback and forced-PHYSTS qualification.

For the extension path with AN enabled, verified CR2[5]=1, ANER[0]=0 and coherent
complete 100/full PHYSTS evidence, report local 100/full, pause invalid. ANER[0]=0
means no exchanged partner AN base page. A PHYSTS full indication alone cannot
be relabeled standard negotiated full duplex; retain the extension dependency.
CR2's text limits this feature to 100 Mbit/s, so PHYSTS 10/full does not qualify
the extension. When extension state is unverified, retain unknown resolution
rather than turning desired intent into observed hardware state.

## Independent pause vectors

The following is a factual expansion of original Table 28B-3. Row is local
ANAR pause/asymmetric bits; column is partner ALNPAR bits. Cell is the expected
positive PHYCR RX13/TX12 mask. It agrees with the existing ledger matrix.

| Local / partner | `0x0000` | `0x0400` | `0x0800` | `0x0c00` |
| --- | --- | --- | --- | --- |
| `0x0000` | `0x0000` | `0x0000` | `0x0000` | `0x0000` |
| `0x0400` | `0x0000` | `0x3000` | `0x0000` | `0x3000` |
| `0x0800` | `0x0000` | `0x0000` | `0x0000` | `0x1000` |
| `0x0c00` | `0x0000` | `0x3000` | `0x2000` | `0x3000` |

Both symmetric bits select RX+TX regardless of asymmetric bits. Otherwise both
asymmetric bits must be set; local symmetric then selects RX only, partner
symmetric selects TX only. The two asymmetric-only sides select neither.
The original table makes RX/TX perspective explicit: local `0x0800` against
partner `0x0c00` transmits pause; the reciprocal case receives pause.

Use all 16 literal expected cells as independent tests, not a second copy of
production conditionals. Cross-check PHYCR under `0x3000`; disagreement leaves
mode intact but invalidates pause and preserves both raw advertisement words
plus PHYCR. Half duplex, forced mode, parallel detection and pending AN never
gain negotiated-pause validity. Zero/zero is a valid negotiated result only
after these prerequisites pass; numerical defaults are not validity.

## Review disposition

Source review PASS for exact masks, original-pause interpretation, ten-read
bounded consistency, forced-mode exception and qualified extension path. The
lead must integrate the precise coherence masks and remove the stale ledger
automatic-reacquisition wording. No new essential source blocker was found.
Q11 physical evidence remains NOT RUN. Host, portable/package consumer and SDK
link tests are NOT RUN by this source reviewer and must be reported by their
executing owner; remote CI is NOT RUN under the first-execution prompt-71 rule.
This report is not an implementation or physical qualification result.

## Implementation review integration

The subsequent read-only review checked the actual new source/header/example
against the original literals and OP-14. It found four issues, all corrected
and regression-tested by the lead/test owner: highest-common full-duplex pause
qualification, conservative age from acquisition start, failed-budget refresh
staleness, and link invalidation when another public read detects a clock fault.
Independent final review found no further blocking defect in the fixed 8+2 plan,
overflow/reentry checks, forced gates or collector retention. The executing
[stage report](19-link-observation.md) owns final test results.
