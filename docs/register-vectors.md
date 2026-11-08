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

The following remains an initialization-owned transaction; `writeMmd` is not
an arbitrary bootstrap route to VOD configuration.

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

## Atomic stable MMD modification

Prompt 12's independent original-PDF review of 8.3.11.1/.4/.5 and table 8-42
derives five physical frames: `W0D=001F`, `W0E=0018`, `W0D=401F`, `R0E`, `W0E`.
The selected nonincrementing data address persists under one controller lock.
No second control/address setup is needed inside this transaction; every next
explicit call must nevertheless establish its complete selection anew.

Literal LEDCR test vectors, independent of production constants:

- Injected prior word `FFFF`, mask `0012`, replacement `0002`: preserve mask
  `0680`, final word `0682`. Reserved poison bits must never be written back.
- Prior word `0492`, mask `0600`, replacement `0200`: preserve mask `0092`,
  final word `0292`.
- Zero mask, reserved mask `0001`, replacement `0010` outside mask `0002`,
  or any value/mask above `FFFF` reject before callbacks.
- Four available frames cannot admit the five-frame modification; equality of
  deadline to clock upper bound plus full transaction bound also rejects.
- Failed phase index `k` admits no later frame: attempts `k+1`, completions `k`.
  Successful setup frames alone do not establish target mutation; only the final
  write may do so, including on ambiguous error. A new explicit operation emits
  all setup frames again and cannot validate the earlier write retrospectively.

`test/mmd.cpp` executes these protocol/failure rules through production APIs.
Its simulated register storage does not establish electrical timing or silicon behavior.

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

- Composed from table 8-19, the forced full-duplex MII loopback vectors are BMCR `0x6100` at
  100 Mbit/s with BISCR selection `0x04`, and BMCR `0x4100` at 10 Mbit/s with
  selection `0x01`. These words are not printed examples; duplex is the test's
  choice. The generic bit-2 sentence cannot override the latter.
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

## Prompt 5 independent power, wake and interface vectors

The [power review](reports/implementation/5-power-source-review.md) and
[pin review](reports/implementation/5-pin-source-review.md) derive these literals
from original SNLS647I pages 15, 26–29, 69–75 and 81–93, plus SNLA266B table 5-5.
They were checked before ledger integration. Registered
`test_remaining_ledger.py` independently transcribes every new detailed
reset/access partition and the exceptional actions; it does not generate
expected values from the JSON. These are research tests, not executing PHY code.

- MLEDCR address `0x0025`, selector mask `0x0078`: source E gives
  `0x0070`, with required route bit `0x0071`; F and route 0 are reserved.
  LEDCFG `0x0460` has LED1 at `0x00F0` and LED3 at `0xF000`.
  From `0x5665`, LED1=A gives `0x56A5`; LED3=E gives `0xE665`.
  LEDCFG2 `0x0469` writable mask is `0x0077`, reserved `0xFF88`,
  known reset mask `0xFBBB`: bit 10 is unknown **and reserved**.
- CR2 bit 1 set disables odd-nibble detection:
  `0x0100 → 0x0102` disables, `0x0102 → 0x0100` enables.
  Extended full duplex uses bit 5 only, `0x0020`; bit 6 stays reserved.
  Its primary owner is 18, next page 17, isolation 33 and collision test 35.
- Clock selectors on LED0/1 admit 5/6, positioned `0x0028/0x0030`;
  the reset selector 1 does not license an active clock request.
  LED1 mux admits 0/2/4/5/6; LED2 admits 0/2/3/4/5/6/7.
  LED0/3 mux fields remain R, and contradictory clock routes remain gated.
- IO_CFG1 impedance codes 0/1 mean slow/fast. IOCTRL codes
  `0x10/0x11` mean fast/slow; `0x0400` touches a reserved bit.
  VOD minus code `0x28..0x38` spans 150% down to 50%; plus
  `0x08..0x18` spans 50% up to 150%. With zero code unchanged at zero,
  all-direction 150% gives VOD_CFG1/2 `0x2A00/0x8618`, and 50%
  gives `0x3E00/0x8208`. The split MDIX-minus code must be validated
  as a whole before either write, including every intermediate state.
  For `0x2F → 0x30`, high-first gives undocumented `0x3F` and low-first
  gives undocumented `0x20`. Stage through low nibble 8:
  `0x2F → 0x28 → 0x38 → 0x30`; restoration uses
  `0x30 → 0x38 → 0x28 → 0x2F`. Verify each write before continuing.
  Independent arithmetic covers all 289 legal endpoint pairs and restoration
  from every modeled failure prefix. These are legal-code transitions, not a
  claim of glitch-free hardware output. ANA_RX10BT_CTRL's five bits versus sixteen
  stated voltage levels do not establish a physical conversion.
- Active-sleep policy uses mask/value `0x7000/0x6000`; BISCR
  `0x0100` means normal, zero under that mask means asleep.
  PHYSCR=`0x6108`, BISCR=`0x0100` is a valid enabled-but-awake result.
  IEEE request is BMCR `0x0800`, reset/restart neutral mask `0x8200`;
  preserve isolation `0x0400`. DPD sets `0x0428[2]` before the
  IEEE request, but inverse exit and opaque composition remain gated.
- RXFCFG1 reserved `0xE01E`, known reset `0x1080/0xFFFE`,
  stable writable `0x17E1`. W0C bit `0x0800` has neutral **one**.
  CRC-on, level, WoL+Magic policy uses ordinary write `0x1981`,
  explicit acknowledgement `0x1181`; Secure-ON gives
  `0x19A1/0x11A1`. CRC-off ordinary update `0x0981` must not
  acknowledge. Pulse codes 0/1/2/3 select 8/16/32/64 cycles at 125 MHz,
  derived 64/128/256/512 ns, not a wake deadline.
- **MAC numbering correction from independent cross-review:** table 8-92
  labels TI byte 5 MSB. API octet 0 is the first conventional MAC octet,
  so TI byte 5 maps to API 0, not API 5. MAC `11:22:33:44:55:66`
  yields `04A2=2211, 04A3=4433, 04A4=6655`.
  TI high:low byte labels remain 4:5, 2:3, 0:1.
  Password `2A 2B 2C 2D 2E 2F` uses its separately numbered tables:
  `04A5=2B2A, 04A6=2D2C, 04A7=2F2E`.
  All-zero/all-FF credentials give zero/FFFF words, but only asymmetric
  literals expose a swapped mapping. No DP83822 callback is the oracle.
- EEE original printed sequence (selected wire mapping recorded separately):
  `001F=8000,203C=0002,04D1=008B,04D3=4F12,04DF=0180,033E=A681,
  033F=0003,0123=0800,031B=8848,0466=FE00,04CF=261D,0416=1F30,
  04F5=2864,04E0=FFF2,031F=FE36,0308=0000,04F4=0800,0000=3300`.
  Final BMCR changes stable mask `0x7D80` and issues restart
  `0x0200`; owned mask is `0x7F80`, not just the set bits.
  EEECFG3 bypass bits are set with mask/value `0x0009/0x0009`;
  LPI start/stop uses `0x1000/0x1000` and `0x1000/0`.
  Opaque recipe actions grant no generic read/snapshot permission; exact-part
  EEECFG3 bits0/3/12 permit bounded masked control and readback.
- D2 selects MMD3 wire 0001/0014/0016 and MMD7 wire 003C/003D as
  source-implied, not physically qualified. Only 0014/003C/003D allow safe raw
  reads; 0001/0016 belong to the collector. Reject high-prefix wire addresses.
  EEE sleep 210 us, wake 36 us, quiet 20 ms and refresh 200 us are all
  **NOM**, not MAX; no partner-ready deadline follows from them.
- COMPT ordinary mask is `0x000F`; 10M codes 0..D are described and
  E/F are not 10M patterns. In 100M, all nibbles combine with
  `0x0428[5]` into count 0..31; zero clears the shift register.
  Disable/enable actions touch only `0x0010`, count actions only
  `0x0020`, preserving DPD `0x0004` once composition is proved.
  Table8-50 admits fourteen actions with mask `0x001F`, values `0x0010..0x001D`;
  `0x0013` is SNLA266B's repeated-one example. Preserve bits15:5. Stop/reset
  must precede profile restore.

Future host acceptance must reject unsupported pin/mode/code combinations with
zero I/O, preserve each accepted or uncertain multiword prefix, retain consumed
events, and test every primary/cleanup failure and cancellation boundary.
Freeze the fake clock to prove count bounds independently of elapsed time.
Unresolved source gates reject before unknown reads or mutation. Qualification
requires identified peers, wiring, MDIO/clock traces and calibrated instruments;
successful register reads or generated traffic alone are not physical evidence.

## WP3 independent source regressions

These literals were reopened from original PDFs; they are not generated from
production constants or the ledger. Host arithmetic does not qualify silicon.

- SNLS638C tables 7-99..103 and SNLS505H 7.4.2.9 support the selected EEE
  inference: wire read `(7,003D)` is `0D=0007,0E=003D,0D=4007,read0E`.
  Capability `(3,0014)` must equal whole word `0002`; `FFFF` must fail even
  though bit1 is set. Advertisement `(7,003C)=0002` must read back `0002`.
  These add eight frames to the 66-frame printed recipe floor: 74 before
  reset polls/restoration. EEECFG3 LPI `008B -> 108B -> 008B` preserves bypass.
- D4: AN-disabled BMCR `2100` with link up gives 100/full; `0100` gives10/full.
  BMCR `2000` with CR2 extended bit known zero gives100/half. With extended
  enabled or unverified, retain100, duplexUNKNOWN and mode invalid. Forced
  pause remains invalid. Ordinary ANAR ownership is `0DFF`; next-page `8000`
  is transient and clears on successful exchange without another restart.
- SNLA423A reverse/analog scripts require `001F=4000`; MII/PCS do not.
  Analog script `030B=3380` decodes low field `(3380 & 0FC0)>>6 = 0E`,
  outside the detailed legal interval `28..38`; reject it.

SNLA428 table2-1 p3, columns **1A, 1B, 2, 3, 4, 5**, in write order:

| Address | 1A | 1B | 2 | 3 | 4 | 5 |
| --- | --- | --- | --- | --- | --- | --- |
| 0170 | 5C12 | 5C12 | 5C22 | 5E32 | 5E42 | 5E52 |
| 0173 | 0D00 | 0D0C | 0D13 | 1A20 | 343A | 8F6E |
| 0175 | 1007 | 1007 | 1007 | 1007 | 100A | 100D |
| 0178 | 0002 | 0002 | 0002 | 0002 | 0002 | 0006 |
| 0416 | 1FA0 | 1FA0 | 1FA0 | 1FA0 | 1FA0 | 1F90 |
| 0411 | 0813 | 0813 | 0813 | 0815 | 0816 | 0816 |
| 0456 | 0608 | 0608 | 0608 | 0608 | 0608 | 0608 |
| 001E | 8102 | 8102 | 8102 | 8102 | 8102 | 8102 |

Keep the different datasheet segment literals above; this is an explicit recipe
selection, not a correction of those tables. GENCFG owns exception `0600`:
old `0008 -> 0608`; old `0000 -> 0600` is inferred composition pending Q7.
Never claim opaque 0411/0416 readback or inverse restoration.

SNLA428 table2-5 p6 gives rows2/3/4 zero, row5 `(78,11,0000)`,
row1B `(0C,06,0800)`, row1A `(06,44,0000)` in location/amplitude/peak-word hex.
Scan2,3,4,5,1B,1A; amplitude masked7F must exceed decimal10 (rows2..5) or24
(rows1A/1B). Row5 qualifies first. Its whole peak word is zero (source-implied
open), and `abs((0x78-5)*8/(2*4.6)) = 100 m` within1e-9. Use the same row's
location and peak word; do not use the previous row or bit11 alone. Preserve
raw bit11/bit5 separately. Positive caller propagation delay is mandatory;
Table2-4 gives5.35ns/m Cat5/5e,5 Cat6,4.6 Cat7. No peak is NO_PEAK, not zero
metres or good cable. This derives only transmit-A; raw B/cross capture is separate.
The note's MATLAB indexing, undefined temporary and5.2/5.35 discrepancy are not
copied as implementation. Q7 fixtures must compare recipes and physical distance.

## Stage 15 independent initialization regressions

Original SNLS647I table 8-19 (pp54–55), 8.3.13 (p40), table 8-81 (p86) and
8-87/88 (pp89–90) were independently reopened for prompt 15. These literal
expectations originated in `test/operations.cpp`, not generated from driver masks;
stage16 adds the PHYRCR precheck described below:

- PCS command is exactly `W(00,8000)` after a separate BMCR check. Completion
  requires a later `R(00)` with bit `8000` clear. Sticky `8000` ends by caller
  limits; `3100` is a clear example, not an identity oracle.
- Mandatory vendor write: `W(0D,001F) W(0E,030E) W(0D,401F) W(0E,4A40)`.
  Verification reselects completely and reads `0E`. Table 8-81's four stable
  fields independently sum to mask `F000|0800|0780|0040=FFC0` and configured
  word `4000+0800+0200+0040=4A40`. Readback `4A7F` passes; `8400` and `4A00` fail.
- Capture uses complete vendor reads of `0467` and `0468`, retaining both raw
  words without interpreting BASIC mode through ENHANCED strap labels.
- NONE with no active reset takes `2+4+4+1+4+4=19` frames. PCS clear on its first
  completion observation adds command/read and renewed ID/straps:
  `2+4+4+1+1+1+2+4+4+4+4=31` frames. Selector failure cannot imply target effect.

Independent OP-6 policy arithmetic is separate from silicon facts. With fixed
50 us, per-frame 100 us, lag 10000 us and poll interval 1000 us, one-frame bound
is 150 us and reset reservation is `40000+150+1000+150=41300` us. Deadline equal
to raw+41300 rejects; one us more admits. After command return at raw10000,
completion upper bound is20000 and next due21000: raw20000 cannot poll,
raw30000 can. Tests also advance the pre-callback clock after the earlier poll
sample to prove reset look-ahead is rechecked at actual submission.

No arithmetic vector establishes controller timing or physical reset domains.
The source/implementation review and qualification limits are in the
[stage report](reports/implementation/15-initialize-pcs-reset.md).

## Stage 16 independent reset vectors

Original SNLS647I table 8-48 (p73) independently gives HARD `W(1F,8000)` and
RESTART `W(1F,4000)`, reserved13:0 neutral zero. Read `1F`, never address8000;
requested bit clear with the other command set is not readiness. C22 address8000
still rejects before callbacks. Full current no-wait traces are NONE20 and
PCS/HARD/RESTART32 frames: stage16 adds one PHYRCR precheck to stage15's historical
19/31 counts above. `test/reset.cpp` compares actual literals and all failure positions.

For fixed50us/frame100us/lag10000us with requested polling1000us, HARD OP-6
reservation is `40000+150+2000+150=42300us`; equality rejects, one us more admits.
RESTART retains the PCS41300us arithmetic and has no HARD floor. For HARD
completion raw10000 and lag10000, the floor is22000; raw21000 cannot issue any
frame, even after cancel/take/abandon or through another owner-routed PHY. A clock
regression or unrepresentable floor retains uncertainty and closes timed access.

Faulty reports cannot convert an unreported command into proof of no effects.
A success receipt missing its command frame retains possible reset and the quiet
floor. A valid zero-attempt BUSY/error remains effect-free. These are transport
contract regressions, not physical reset measurements. Q5 remains NOT RUN.

## Stage 17 independent negotiation and page vectors

Original SNLS647I tables 8-19/23 independently map the four API choices to
wire 0020/0040/0080/0100. Selector is always0001. For 100/full alone the four
pause cases are0101/0501/0901/0D01; all four technologies without pause are01E1.
Prior ANAR F21F becomes5301 for100/full: preserve5200, replace selector, clear
prohibited2000 and transient8000. BMCR prior217F becomes127F on restart, then
stable readback107F; owned3100 compares1000, commands8200 are not sampled/replayed.
The tests independently enumerate all15 nonempty subsets times4 pause choices.

Mandatory initialization remains the literal sequence
`W(0D,001F) W(0E,030E) W(0D,401F) W(0E,4A40)`, verified underFFC0.
Configure issues no such sequence. Initialization replay adds the same negotiated
procedure after mandatory setup for NONE/PCS/HARD/RESTART, always with NP0.

IEEE802.3-2012 Figure28-18 and original tables8-25..27 distinguish the first
fresh ANER event (read partner BASE at05) from subsequent NEXT events (read08).
With BASE toggle0, first accepted NEXT toggle is1. For a remote-fault message4
followed by unformatted3 with ACK2, local register7 words are B804 then1803 when
hardware toggle is0800 then0. Writes preserve4800 and ownB7FF. Generated null
is2001 plus hardware-owned toggle. A duplicate toggle consumes a retained event
slot but cannot advance the local page. MP5/OUI and MP6/PHYID require four ordered
unformatted pages. Null followed by substantive information is rejected as a
semantic inference from Clause28's end-of-information meaning.

Bilateral last pages still require BMSR0020 before epoch success; then ANAR8101
becomes0101 without another BMCR restart. Refusal retains its own flag and waits
for ordinary AN completion. Timeout/cancel/read-after-effect failure retains
consumed evidence and possible NP without an extra write. Independent literal
controller tests prove software traces, not physical Clause28 handshake or wire
timing. [The source review](reports/implementation/17-source-review.md) records
original evidence and Q10 qualification boundaries.

## Stage 18 independent forced/crossover vectors

Original SNLS647I tables8-19/28/29/30/43 yield forced BMCR owned3100 values
0000/0100/2000/2100 for10half/10full/100half/100full. Preserve4CFF, neutralize
reset/restart8200, and compare only3100. The ignored AN-restart readback bit does
not impose a forced completion wait. No forced frame targets ANAR04.

CR1 robust0020 is required for all forced routes. Automatic PHYCR changes8000
only (preserve7FFF); manual changesC000 to0000/4000 (preserve3FFF). From40E5,
automatic yieldsC0E5, MDI00E5 and MDIX40E5. CR3 mirror owns0060 (preserveFF9F),
not separate arbitrary corrections. CR2 extension owns0020 (preserveFFDF), never
0060; A5DF becomesA5FF on enable and preserves reserved0040. Both mirror flags
must agree; forced10 plus extension and forced with any AN intent reject.

A forced profile has five saved reads, five two-frame atomic RMWs and five
stable readbacks:20frames. Ordinary AN has six of each:24frames. Save order is
BMCR/CR1/PHYCR/CR3/CR2, with ANAR first only forAN. Apply routing/mirror/extension
before activation. If AN requests robust=false, defer the CR1 clear until after
BMCR enables AN; no write may clear robust during the previous forced interval.
Verify ANAR when active, thenCR1/PHYCR/CR3/CR2/BMCR. Stable mismatch evidence is
retained even when a later field read fails. These counts exclude optional AN
waits/pages and the separate initialization/VOD procedure.

`test/forced.cpp` independently checks36 forced and24 AN routing profiles,
transitions, every-frame faults/cancellation, reset replay, actual observed
mismatches and owner exclusion. Existing next-page vectors remain unchanged;
additional owned controls increase the two-page trace from23 to39frames and the
duplicate-page trace from25 to41. Physical robust-manual/mirror interaction and
partner compatibility remain Q11 NOT RUN; loopback restore tests arrive at23.
