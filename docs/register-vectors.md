# Independent DP83826I core vectors

These literal expectations were derived from original **SNLS647I** tables before
the independent reviewer read the ledger. They are not generated from ledger
values or vendor macros. See [the original PDF](reference/source/TI-DP83826I-E-SNLS647I.pdf)
and [source review](reports/implementation/3-source-audit.md). The executing
[unit suite](../test/test_ledger.py) compares research records with independent
literals and checks malformed records. It is not a fake driver implementation,
transport test or hardware result. Future driver tests must reuse these oracles
against real executing behavior and inject failures at every frame boundary.

## Identity and masks

Tables 8-21/22, pp56–57: `(0x2000, 0xA111)` gives BASIC;
`(0x2000, 0xA131)` gives ENHANCED. The OUI arithmetic is
`(0x2000 << 6) | ((0xA131 & 0xFC00) >> 10) = 0x080028`.
Model mask `0x03F0` yields `0x11` or `0x13` after shifting four;
revision mask `0x000F` yields 1. Combined IDs are `0x2000A111` and `0x2000A131`.
Zero/all-one words, unknown model, unsupported revision and declared-mode
mismatch are separate cases. A successful first ID read and failed second read
must not publish a new identity. No suffix can be decoded from these shared IDs.

The core masks below are independent arithmetic checks, not a second production
map. Known masks exclude every unestablished bit even when a heading prints a
coarse default.

| Register | Partial reset / known mask | Reserved mask | Source / interpretation |
| --- | --- | --- | --- |
| BMCR | `0000 / CAFF` | `007F` | Table8-19: unknown speed/AN/isolate/duplex `3500`. |
| BMSR | `7849 / FFFF` | `0780` | Table8-20, capability and status defaults. |
| PHYIDR1 | `2000 / FFFF` | `0000` | Table8-21. |
| PHYIDR2 | `A111 / FFDF` | `0000` | Table8-22; common revision-1 bits, mode-dependent bit5 excluded. |
| ANAR | `0001 / FE1F` | `5000` | Table8-23; unknown advertised technology bits `01E0`. |
| ALNPAR | `0000 / FFFF` | `1000` | Table8-24. |
| ANER | `0004 / FFFF` | `FFE0` | Table8-25. |
| ANNPTR | `2001 / FFFF` | `4000` | Table8-26. |
| ANLNPTR | `0000 / FFFF` | `0000` | Table8-27. |
| CR1 | `0000 / FFFF` | `FE5D` | Table8-28. |
| CR2 | `0100 / FFFD` | `FFD9` | Table8-29, unknown odd-nibble bit1. |
| CR3 | `0000 / FFF0` | `FB90` | Table8-30 versus feature/strap text; full FLD nibble conservatively unknown. |
| REGCR | `0000 / FFFF` | `3FE0` | Table8-31. Reset DEVAD0 does not grant access. |
| ADDAR | `0000 / FFFF` | `0000` | Table8-32; selected role controls meaning. |
| PHYSTS | `0002 / FFFF` | `8000` | Table8-34 published reset; dynamic fields are not persistent verification targets. |
| PHYCR | `0000 / 3FE0` | `0740` | Table8-43; unknown crossover/address `C01F`. |
| PHYRCR | `0000 / FFFF` | `3FFF` | Table8-48. |
| VOD_CFG3 | `8400 / FFFF` | `003F` | Table8-81; recipe supersedes initial value. |
| SOR1 | `0000 / F800` | `F800` | Table8-87, fixed reserved bits only; printed zero is not universal straps. |
| SOR2 | `0000 / A040` | `A040` | Table8-88, fixed reserved bits only; printed `0287` is not universal straps. |

## Address and command distinction

Section8.3.11 and tables8-31/32/48 yield:

- C22 integer register range is `0x00..0x1F`; `0x20`, `0x8000`, `0xFFFF`, negative
  and noninteger values fail before I/O. Range success does not permit holes or
  bypass selector/collector policy.
- Hard reset sends a command to **register `0x1F`**, tests **mask `0x8000`**,
  and polls that same register. Read values `0x8000, 0x0000` mean pending, complete.
  `0x4000` clears the hard bit but leaves circuitry restart active; it cannot
  establish overall readiness. An error/timeout is not a fabricated zero read.
- PCS reset polls register `0x00`, mask `0x8000`; circuitry restart polls
  register `0x1F`, mask `0x4000`. These are distinct procedures.
- BMCR command mask is `0x8200`, persistent functional mask `0x7D80`.
  PHYRCR command mask is `0xC000`. Unrequested W1S commands are composed as zero.
  Clearing AN restart only establishes initiation; AN-complete/link remain separate.

## Mandatory VOD transaction

Section8.3.13 p40 and tables8-31/32/81 independently give this literal trace:

```text
write C22 0x0D = 0x001F
write C22 0x0E = 0x030E
write C22 0x0D = 0x401F
write C22 0x0E = 0x4A40
```

Hold one transaction lock over all four frames. The value decomposes into MDI
termination4, MDI enable1, MDIX termination4, MDIX enable1 under masks
`F000/0800/0780/0040`. Table 8-81 marks all four fields **R/W**, so their combined
writable mask is `0xFFC0`; no access exception is needed. Inject failure before and after
each accepted frame in the future driver suite. No later phase may execute after
a failed selector. A failed final write retains possible target effects. Repeat
all selection phases on the next explicitly admitted transaction.

## Negotiation, next page and crossover

Tables8-19/23/26/28/30/43 yield independent useful extremes:

- Advertisement selector is exactly `0x0001`; 100/full only is `0x0101`, all
  four 10/100 abilities are `0x01E1`. Symmetric/asymmetric fields are `0x0400`
  and `0x0800`; both with all speeds give `0x0DE1`.
- Local remote fault `0x2000` is rejected even though R/W. `0x0200` is the
  read-only T4 bit; `0x5000` reserved. Allowed ordinary owned writable mask is
  `0x8DFF`; it does not authorize invalid selectors. Partner remote fault remains
  observable in ALNPAR/BMSR/PHYSTS.
- Next-page owned fields are `0xB7FF`. Hardware toggle `0x0800` and reserved
  `0x4000` cannot be overwritten. Local null message is `0x2001`; unformatted
  code may span `0x0000..0x07FF`, while message interpretation needs IEEE context.
  Reserved message codes cannot be transmitted in message mode. Initial page
  receipt is BASE, not automatically an ANLNPTR payload; a register7 write before
  restart does not prove page-loaded state survives. Continue null message1 with
  MP1/NP0 if local data ends before the peer; require bilateral termination.
  Exact DP83826I event/load sequencing retains `next_page_handshake`.
  Every unformatted series must follow a message page defining its use, with
  referenced pages immediately following in defined order (IEEE Annex 28B.4 p732
  and 28.2.3.4.13g p298).
- Under BMCR selection mask `0x3100`, forced 10-half/full are `0x0000/0x0100`,
  forced 100-half/full `0x2000/0x2100`. Gigabit selection is not supported.
- CR1 robust crossover is `0x0020` and is required for forced operation. PHYCR
  automatic is `0x8000`; manual MDI/MDIX select `0x0000/0x4000` with automatic
  clear. CR3 port mirror combines `0x0020 | 0x0040 = 0x0060`.
- CR2 odd-nibble **disable** is `0x0002`; setting it must not be exposed as
  detection enabled. Extended duplex is independent `0x0020` and opt-in.
- PHYSTS speed bit1 uses zero100/one10, unlike BMCR speed selection. Its
  forced-mode wording remains gated, not normalized away by a test fake.

## Complete pause resolution oracle

Original IEEE802.3-2012 Section Two, printed p732 Table28B-3, independently
visually checked; full provenance and mirror limitation are recorded in the
[source audit](reports/implementation/3-source-audit.md). TI table8-43 applies
the result only to AN's highest common full-duplex technology. Matrix values
are local PHYCR RX/TX status (`RX=2000`, `TX=1000`). Rows are local ANAR
pause/asymmetric fields; columns are partner fields.

| Local / partner | `0000` | `0400` | `0800` | `0C00` |
| --- | --- | --- | --- | --- |
| `0000` | `0000` | `0000` | `0000` | `0000` |
| `0400` | `0000` | `3000` | `0000` | `3000` |
| `0800` | `0000` | `0000` | `0000` | `1000` |
| `0C00` | `0000` | `3000` | `2000` | `3000` |

Forced, half-duplex, incomplete or stale AN does not have a negotiated-pause
result; do not replace invalidity with a claimed successful zero resolution.

## Read effects and failure vectors

Table8-34 independently names clearing relationships: PHYSTS receive error to
RECR, polarity to10BTSCR, false carrier toFCSCR, page toANER, interrupt toMISR,
remote fault toBMSR. Its jabber and current-link mirrors explicitly do not clear
on PHYSTS read. ANER's RH page annotation does not override the clearing prose.
Acquire through one collector, retaining individual samples/timestamps. Test a
first valid BMSR latched-down sample followed by current-up; preserve both rather
than returning only the second. Failure after a consumed event must retain that
event and mark the missing rest, while an ambiguous failed read flags history loss.

Structural negative cases exercise duplicate IDs/address aliases, missing
indirect aliases, overlapping/out-of-width fields, incomplete partitions,
unknown reset bits set in the partial value, wrong command-neutral values,
illegal encodings, missing source/owner, deleted inventory/feature/core-procedure rows,
unproved MMD aliases, procedure masks outside their feature's coverage, and EEE
printed addresses incorrectly promoted to wire tuples. These validate research
structure. Transaction call-count, time/cancellation and physical traces remain
implementation/qualification obligations in the ledger owners.

The fresh access oracle groups every functional bit of all 20 core registers by
its original-table access type. These literal tests are independent of the field
partition tests, which cannot detect a wrong access label. Timing vectors also
preserve the two reset-only nominal signaling/clock intervals without inventing
power-up applicability or an unstated reset edge.

## Prompt-4 independent diagnostic vectors

These literals were derived from the original SNLS647I tables and reviewed
separately from the integrated JSON. The executing research suite is
`test/test_diagnostic_ledger.py`; no simulated register operation is claimed as
driver or hardware evidence. See the [diagnostic review](reports/implementation/4-diagnostic-source-review.md)
and [event review](reports/implementation/4-event-source-review.md) for original
table locations, complete access/reset partitions and conflicts.

- Table 8-19's forced full-duplex MII loopback examples are BMCR `0x6100` at
  100 Mbit/s with BISCR selection `0x04`, and BMCR `0x4100` at 10 Mbit/s with
  selection `0x01`. The generic bit-2 sentence cannot override the latter.
  BISCR loopback selection accepts `0,1,2,4,8,0x10`; combinations such as `3`
  are not independent enable flags. The 10-Mbit path still requires VOD setup.
- Section 8.3.14.4's ordered digital sequence is `0000=2100`, `0016=0104`,
  `0122=2000`, `0123=2000`, `0130=47FF`, `001F=4000` (hex). The last write is
  circuitry restart, not the hard-reset command. Exact recipe offsets do not
  acquire general field/reset definitions from these values.
- BICSR1's desired IPG code `0x7D` gives 500 bytes, maximum `0xFF` gives 1020
  bytes; BICSR2 default `0x05EE` is 1518 bytes and its field ceiling is 2047.
  A freeze with IPG `0x7D` writes `0x807D`. If a read returns `0xFFA5`, preserve
  desired IPG `0xA5` but emit `0x80A5` for the next intentional capture; never
  echo counter bits 14:8. Error byte `0xFF` is saturated in stop-at-maximum mode
  and cannot quantify further errors; continuous mode can wrap through zero.
- CDCR reset `0x0102` already has done `0x0002` set. Start `0x8000`, done
  `0x0002`, fail `0x0001` and reserved `0x3FFC` are distinct. Repeated reset
  word `0x0102` without a fresh trigger/progress history cannot pass measurement.
  A failed trigger write leaves possible running hardware; it is not safe to
  repeat as a new request without reconciliation.
- Segment 1 through 5 words are respectively: CDSCR3 `0D07/0D14/1A21/343B/8F6F`,
  TDR_175 `1004/1004/1005/1008/100B`, TDR_178 `0002/0002/0002/0002/0006` (hex).
  CDSCR3's published reset `0D04` is not segment 1's prescribed `0D07`.
  Average code 7 and segment codes 0/6/7 are prohibited. CDSCR2 and result banks
  CDLRR2–5/CDLAR2–5 are all reserved, despite multi-peak feature prose.
- MISR status lives in the high byte and enables in the low byte. For a raw
  sample `0x0424`, enabled pending evidence is `0x0400`; retain the whole word,
  not just the mask intersection. If MISR1 succeeds then MISR2 fails, the MISR1
  sample remains valid while the aggregate is incomplete. An ambiguous MISR2
  read also flags possible lost history. Mask changes cannot erase this rule.
- PHYSTS `0x2000/0x0800/0x1000/0x0100` correspond to events cleared by RECR,
  FCSCR, 10BTSCR and ANER respectively. A PHYSTS observation cannot replace
  those clearing-source acquisitions. FCSCR has low-byte count; RECR's full-word
  field and `0xFF/0x7F` prose remain contradictory, not silently narrowed.
- FLDS has cause mask `0x01F0`, reserved `0xFE0F`, and read-clear events.
  FLD_CFG1 reset `0x8147` decomposes into window code `0x20`, count `0x14` and
  reserved low nibble `7`. Its prescribed `0x0417` instead selects window code
  1 (two 8-ns clocks) and one error. Preserve the discrepancy with the
  20-errors/10-us prose. FLD_CFG2 mask `0x003F` selects raw code 8 for descrambler
  loss while its reserved upper bits remain protected.
  Original timing table p15 gives MAX 10 us for the strapped 150-metre case
  and modes 1–4, MAX 11 us for mode 5. Footnote 5 on p17 requires RX-error or
  signal/energy-loss detection enabled for the mode 2/3/5 maxima. These physical
  intervals run from the qualifying failure to link-loss indication and do not
  specify host polling latency.

Future driver tests must inject failure and cancellation at each admitted
transaction, including the capture write before its read, and preserve primary
versus cleanup outcomes. Test finite timeout and frozen-clock work limits,
unavailable fixtures, wrong speed/interface, stale done, counter wrap, missing
checker lock and incomplete restoration. These are planned acceptance vectors;
the present tests validate research structure and independently sourced literals.
