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
