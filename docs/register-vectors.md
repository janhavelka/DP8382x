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

## Prompt 19 link observation vectors

Independent original-source review: SNLS647I tables8-19/20/23/24/25/29/34/43 and
IEEE Table28B-3, recorded in the [review](reports/implementation/19-source-review.md).
The production ten-read plan executes these literal addresses in two callbacks:
`R(01), R(01), R(10), R(00), R(04), R(05), R(06), R(19)`;
then `R(01), R(10)`. No production constant derives the test oracle.

- Current/final BMSR comparison is `0024`; first/final PHYSTS comparison is
  `0011`. Cross-register link is BMSR `0004` versus PHYSTS `0001`; AN complete
  is BMSR `0020` versus PHYSTS `0010`. Resolved AN mode compares PHYSTS `0006`.
- First BMSR `0020`, second/final `0024`, PHYSTS `0015` means historical low
  followed by coherent current100/full. First `0024`, second `0020` is a current
  low; it is publishable DOWN only when final BMSR/PHYSTS also agree on low.
  A current/final mismatch rejects publication; all consumed history survives.
- With AN enabled and complete, PHYSTS low words `0001/0005/0003/0007` mean
  100/half,100/full,10/half,10/full. Completion evidence is separate from these
  low words. ANER `0010` invalidates mode through parallel-detection fault.
- BMCR `2000` plus raw PHYSTS `0007` still means forced100/half when verified
  CR2 `0020` is clear. Otherwise duplex is UNKNOWN. BMCR `2100`, `0000`, `0100`
  establish local forced100/full,10/half,10/full respectively. Pause is invalid.
- ANER partner-AN `0001` is absent on parallel detection. Verified CR2 `0020`
  and coherent AN-complete PHYSTS `0015` qualify extended100/full; `0017` does
  not qualify a 10/full extension. Both parallel paths have invalid pause.
- The 16 pause cells above apply only after IEEE selectors `0001` and a
  matching highest-common full-duplex technology. Common `0100` outranks `0080`,
  which outranks `0040`. Local/partner `0141/0041` at10/full qualify; `00C1/00C1`
  at10/full do not because common100/half has priority. Local/partner `0141/0141`
  observed at10/full also do not qualify pause. Mode remains independently raw/
  qualified; no invented pause result repairs inconsistent advertisements.

Clock regression and `raw+lag` overflow retain the old aggregate interval as
stale CLOCK_CONTRACT history, even when detected by another public read. A clock
value of zero is legal with explicit validity. Maximum age uses the start lower
bound, not the completion upper bound; unchanged cached reads consume nothing.

## Read effects and failure vectors

### Prompt 20 event and counter vectors

Independently reviewed original SNLS647I tables8-33/34/36..39/41/44/91 are
identified in the [stage20 review](reports/implementation/20-source-review.md).
The literal default trace is `R(10), R(0F), R(12), R(13), R(14), R(15)` in one
six-frame transaction. No source is read again to prove an empty interrupt bank.

- Each MISR high bit `8000,4000,2000,1000,0800,0400,0200,0100` is an independent
  raw event; low `00FF` enables do not enter its retained event union. MISR2
  `0200` has no polarity/WoL label without owned selector-epoch evidence.
- FLDS causes are `0010,0020,0040,0080,0100`. ANER `0002`, 10BTSCR `0010` and
  RXFS status `0080,0040,0020,0001` retain occurrence through the same collector;
  RXFS `1000` selection does not enter its event union. RCSR `000C` remains raw.
- FCSCR samples `0000,0001,007F,0080,00FE,00FF` are literal boundary cases.
  `0080` first exceeds the half-full threshold; `00FF` is saturation and a lower
  bound. Samples `0003` then `0002` sum to5; after `00FF`, the sum is260 with
  sticky hardware-saturation/lower-bound evidence, not wraparound.
- RECR `0080,0100,8123,FFFF` can be retained raw without asserting effective
  width. Its host arithmetic field stays0 as an explicit gate, not zero errors.
- Host sum `FFFFFFFFFFFFFFFE` plus1 reaches the exact limit without overflow;
  another1 saturates with the overflow flag. Zero cannot erase that flag.
  Sample count `FFFFFFFE` advances to `FFFFFFFF`; another sample leaves the
  count saturated and explicitly sets `samplesSaturated`.
- MISR1 `4000` followed by a MISR2 after-effect timeout retains MISR1's new
  sample and old aggregate, marks aggregate stale and MISR2 possible history loss.
  A following zero cannot prove that no event was lost. An event arriving after
  the MISR1 read is available on the next pass, without an unlimited drain loop.

These literals execute in `test/events.cpp` and the retained `access_counters`
suite. Clock/metadata failure, reset discontinuity, host-history discard and
shared MMD contention are software evidence, not physical event qualification.

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
partner compatibility remain Q11 NOT RUN; loopback restore tests execute at23.

## Executing loopback vectors (stage 23)

Independent SNLS647I 8.3.14/table8-19/table8-40 vectors: BISCR low5 is01 PCS
input10,02 PCS output,04 digital100,08 analog,10 reverse (hex). MII combines
BMCR4000 with01 at10 or04 at100. BISCR0040 line transmission is only MII100.
Table8-40 requires BMCR14 active before BISCR6: from forced100/full and inactive
BISCR4100, enable W(00,6100) then W(16,4144); stop W(16,4100) then W(00,2100).
The companion10/full pair is W(00,4100), W(16,4101), with no line-transmit bit.
External uses no internal selector. Digital's contiguous target sequence is
W(00,2100), W(16,0104), vendor31 W(0122,2000), W(0123,2000), W(0130,47FF),
W(1F,4000); each vendor word uses the independent four-frame selector trace.
No opaque prior-value read or030B3380 write is permitted. Analog/reverse add
W(1F,4000) per the selected SNLA423A scripts; MII/PCS have no such command.

PHYSCR FIFO codes0/1/2/3 own0300, so0B55 becomes0855/0955/0A55/0B55.
Inactive BISCR4100 may retain counter policy4000; digital0104 clears that
policy temporarily, then its HARD exit restores4100 after initialization.
The tail preserves8FA0 while restoring705F, and preservesFCFF around FIFO0300.
HARD may reload BMCR isolation0400: restore ordinary extras0480 with preserve
317F and commands/power neutral0, then verify7D80 against the saved/profile
intent. This is not a general BMCR or opaque-register raw-write permission.

`test/loopback.cpp` independently pins every semantic write for every legal
mode/speed/interface, plus exact bounded frame/callback plans and every injected
frame/release/cancel boundary. It checks manual MDIX and robust/mirror profile
restoration, FIFO variants, physical-effect uncertainty and separate results.
These are host protocol vectors; Q8/Q5/Q11 measured traces remain NOT RUN.

## Executing BIST vectors (stage 24)

SNLS647I tables8-40/45/46 independently give modes0000/1000/2000/3000;
wrap4000 is separate. IPG7D means500bytes, capture807D preserves it, and
rawFF7D means255errorbytes with the same IPG. Packet05EE means1518bytes;
field limits are07FF and gapFF*4=1020bytes. No Ethernet minimum is inferred.
Analog control3108 with lock0800/active0200 reads3B08, not the note typo3B04;
loss0400 gives3F08. RH is hardware set/clear, not RC. Counter80 in807D is
read data, never a pending command. Zero/wrap and unproved repeat-latch intervals
remain inconclusive. `test/bist.cpp` uses independent literals for every mode,
field boundary, indivisible capture, fault and restoration.

## Executing manual cable vectors (prompt 25)

`test/cable.cpp` independently transcribes SNLA428 table 2-1 and SNLS647I
tables 8-54/56..60, rather than deriving expectations from production arrays.
It checks all six rows and subsets, all A/B channel combinations, legal extrema,
prohibited average7 and saved segment0/6/7, reserved preservation and GENCFG
old0008 ->0608 / old0000 ->0600. Datasheet per-segment tuning remains separate.
The complete SNLA428 table 2-5 sample selects run5/open/100m at4.6 ns/m;
strict thresholds, same-row indexing, whole peak word, raw bits11/5 and NO_PEAK
are literal regressions. No host vector establishes physical accuracy.

One-busy/one-done fixtures prove 426 frames/128 callbacks for all six rows,
101/33 for one SNLA428 row, 102/32 RAW; negotiated restoration uses 127/50 or
118/48 respectively. Each additional pending poll consumes one frame/callback.
Failure injection covers every frame and cancellation boundary without retries,
including partial nonconsuming result reads, explicit reread and failed HARD tail.

## Executing autorun vectors (prompt 26)

Independent table8-28 literals in `test/autorun.cpp` verify old00A2 ->01A2 on
enable and old03F3 ->02F3 on disable; preserveFEFF, compare0100 only. Policy
preflight plus one RMW/readback uses10 frames/eight callbacks. No manual trigger
is written. Raw R1E, MMD31 R0180/R0185/R018A, R1E uses14 frames/five callbacks.
Initial0102 done, identical results and equal bookends remain uncorrelated.
Hardware updates at every read/handoff position never become a generation proof.
Owned reset replay adds3 frames/two callbacks last; verify adds one read only.
No policy ownership adds no frames. These are software bounds, not silicon times.

## Prompt 27 independent interrupt vectors

Original SNLS647I tables8-35..37/91 and pin tables, independently checked against
the original PDF by the source reviewer:

- PHYSCR0108 ->010B is active-low global/output enabled; active-high is0103.
  Test010B ->010F ->010B. ENHANCED stablemask000B; BASIC stablemask000A preservesOE
  without claiming a power-down role. UnrelatedE310 plusowned000B becomesE31B.
- MISR1 enable01/02/04/08/10/20/40/80 maps to RECR-half/FCSCR-half/AN-complete/
  duplex/speed/link/energy/quality events0100/0200/0400/0800/1000/2000/4000/8000.
- MISR2 the same enable ladder maps jabber/shared-polarity-or-WoL/sleep/MDIX/FIFO/
  page/AN-error/EEE-error. Read-clearFF00 is never echoed as a control byte.
- RXFS whole-word0000 selects polarity,1000 selects wake. The four-frame write
  is W(0D,001F),W(0E,04A1),W(0D,401F),W(0E,0000or1000); no preceding RXFS read
  builds the word. Both pre-change and ambiguous crossing MISR2 evidence survive.
- Service always R(10),R(12),R(13); MISR1raw0300 requests R(14),R(15) in one
  further callback. A prior exhausted pass still owes those reads when the next
  MISR pair is zero. Five maximum frames, two callbacks, no unlimited drain.

`test/interrupts.cpp` embeds its own literal fake and assertions, not a lookup of
production masks. Q9 must independently qualify wire/pin latching and delivery.

## Prompt 28 independent power vectors

Original SNLS647I p29 section8.3.6, p55 table8-19, p65 table8-35 and p69 table8-40
were reviewed independently of production constants. PHYSCR `0108 -> 6108`
enables active sleep while BISCR `0100` still indicates normal; BISCR `0000`
indicates sleep only in the applicable active-sleep context. With IEEE down
established, PHYSCR `8108 -> E108` changes policy without clearing PLL disable.
The reverse policy change gives `8108`, not `0108`.

BMCR sampled `B700` composed with reset/restart neutral zero and requested
power-down becomes `3D00`; requested normal becomes `3500`. Isolation `0400`
survives both. A valid reset-bearing sampled word also invalidates readiness,
even if the already indivisible write completed. Readback `0800` after a clear
is blocked/unconfirmed, not an instruction to write again. PLL exit orders
PHYSCR bit15 clear/readback before BMCR bit11 clear/readback. `test/power.cpp`
checks independent literal frames, pin assertion during RMW and accepted-write
failure before separate PCS preparation. No DPD inverse vector is invented.


## Prompt29 independent EEE oracles

Re-derived from original SNLS647I pp26-27 and35, with the source-implied
field comparisons identified below. These literals are independent of production
constants and the JSON ledger. They do not qualify Q1 silicon behavior.

The five selected tuples form an explicit allowlist, not an address translation
algorithm. SNLS638C table7-92 p77 names low offsets in prose; SNLS505H7.4.2.9
p37 demonstrates MMD7/003D on the wire. SNLS603D p47 is notation analogy on a
different part. SNLS647I and SNLS783A still print full prefixed addresses, and
SNLA328 tables4-6 print MMD03 beside1001/1014/1016. Those counter-readings remain
recorded under `eee_wire`; host checks cannot resolve physical ambiguity.

| Printed address | Selected DEVAD/wire offset | Literal read selector/address/data-mode | Route and source-implied meaning |
| --- | --- | --- | --- |
| 1001 | 3/0001 | W(0D,0003), W(0E,0001), W(0D,4003), R(0E) | Collector; received-history0800/0400, current0200/0100 |
| 1014 | 3/0014 | W(0D,0003), W(0E,0014), W(0D,4003), R(0E) | Safe raw read;100BASE-TX capability0002 |
| 1016 | 3/0016 | W(0D,0003), W(0E,0016), W(0D,4003), R(0E) | Collector;16-bit RC wake-error word |
| 203C | 7/003C | W(0D,0007), W(0E,003C), W(0D,4007), R(0E) | Safe raw read;100BASE-TX advertisement0002 |
| 203D | 7/003D | W(0D,0007), W(0E,003D), W(0D,4007), R(0E) | Safe raw read;100BASE-TX partner advertisement0002 |

Reject raw writes at every tuple; reject raw reads of latched0001/RC0016,
all other3/7 offsets, full prefixed offsets and DEVAD31 aliases. Capability
preflight accepts whole word0002 only:0000,0001,0003,0006 andFFFF all reject
before the HARD target write. Selector/address writes used to read capability
are access primitives, not EEE target mutations.

All18 rows below come directly from SNLS647I8.3.3.1 p26. Each W/R refers to a
Clause22 frame; indirect rows remain one indivisible four-frame transaction.

| Row | Independent literal wire write |
| --- | --- |
| 1 | W(1F,8000) |
| 2 | W(0D,0007), W(0E,003C), W(0D,4007), W(0E,0002) |
| 3 | W(0D,001F), W(0E,04D1), W(0D,401F), W(0E,008B) |
| 4 | W(0D,001F), W(0E,04D3), W(0D,401F), W(0E,4F12) |
| 5 | W(0D,001F), W(0E,04DF), W(0D,401F), W(0E,0180) |
| 6 | W(0D,001F), W(0E,033E), W(0D,401F), W(0E,A681) |
| 7 | W(0D,001F), W(0E,033F), W(0D,401F), W(0E,0003) |
| 8 | W(0D,001F), W(0E,0123), W(0D,401F), W(0E,0800) |
| 9 | W(0D,001F), W(0E,031B), W(0D,401F), W(0E,8848) |
| 10 | W(0D,001F), W(0E,0466), W(0D,401F), W(0E,FE00) |
| 11 | W(0D,001F), W(0E,04CF), W(0D,401F), W(0E,261D) |
| 12 | W(0D,001F), W(0E,0416), W(0D,401F), W(0E,1F30) |
| 13 | W(0D,001F), W(0E,04F5), W(0D,401F), W(0E,2864) |
| 14 | W(0D,001F), W(0E,04E0), W(0D,401F), W(0E,FFF2) |
| 15 | W(0D,001F), W(0E,031F), W(0D,401F), W(0E,FE36) |
| 16 | W(0D,001F), W(0E,0308), W(0D,401F), W(0E,0000) |
| 17 | W(0D,001F), W(0E,04F4), W(0D,401F), W(0E,0800) |
| 18 | W(00,3300) |

After any attempted row1, apply the shared persistent HARD floor before every
next frame, then poll1F/8000 clear. After row2, repeat its selector/address/
data-mode writes then R(0E), accepting whole word0002 only. Failure prevents
row3 onward and retains the hard-reset/advertisement effects. Arithmetic:
2direct +16*4indirect =66recipe frames; capability/readback adds8 =74before
reset polling, identity, masked verification and persistent restoration. A slice
below4frames cannot split an indirect row. No nominal EEE timing is a deadline.

Exact-part04D1 verification is mask1009/value0009 after recipe008B. Thus008B
passes,108B fails forcedLPI-clear,0083 fails bit3,008A fails bit0. Changes to
opaque bits are not evidence that documented controls mismatch. Only documented
bits0/3/12 permit typed masked composition; no generic recipe snapshot/restore.
The preserve mask is EFF6, the complement of1009. Starting from008B, the
bit0/bit3 bypass combinations produce008B,008A,0083,0082; opaque bit1 remains
one in all four words. EFF4 would unjustifiably clear that bit. These are
capability-bypass controls, not independently sourced TX/RX direction switches.
SNLA328's DP83825 bits0/3=0 polarity is rejected. Disable has one HARD plus
persistent replay and never a synthesized inverse of these15 opaque vendor rows.

SNLS638C tables7-99..103 pp80-81 and SNLA328 tables4-6 pp11-12 supply the
source-implied observations. Status0800 and0400 are distinct historical TX/RX
received indications;0200 and0100 are distinct current TX/RX indications.
FFFF wake-error is saturated/lower-bound, not an exact overflowing interval;
FFFE remains an unsaturated raw count. Successful samples survive a later
failure; an uncertain destructive read means possible history loss. PCS clears
the silicon counter by the cross-part inference, while retained host history
stays explicitly discontinuous. No timing/rate result is inferred.

Capability/advertisement/partner bit1 tuples000/001/010/011/100/101/110 do not
establish eligible100BASE-TX EEE. Tuple111 still needs current applicable full
link, completed AN, verified bypass controls and coherent source evidence;
incomplete/stale/transport-failed acquisition cannot publish negotiated success.
10BASE-Te reduced amplitude from exact-part p26 is a different fact, not an
invented10Mbps EEE advertisement bit. Current link and prior LPI history also
remain different evidence. All physical Q1/Q2 checks remain NOT RUN.


## Stage30 independent forced TX LPI vectors

Derived from original SNLS647I p27 section8.3.4 and p15 NOM timing column,
independently of production constants. DEVAD31, wire04D1:

- Entry: W(0D,001F),W(0E,04D1),W(0D,401F),R(0E)=008B,W(0E)=108B.
- Readback: W(0D,001F),W(0E,04D1),W(0D,401F),R(0E)=108B; compare1000.
- Exit repeats the five-frame RMW with108B ->008B and four-frame readback;
  compare1000==0000 only. PreserveEFFF, including opaque bit1 and bits14:13.
- Other preserved words: EFFF->FFFF->EFFF;0009->1009->0009;
  A00B->B00B->A00B. These are preservation oracles, not alternate recipe authority.
- Stop1080->0080 succeeds for bit12, but observed bits0/3 contradict owned0009:
  persistent EEE trust stays DIRTY and cleanup remains owed.
- For callback bounds8us+4us/frame and clockLag1us, rawNow+53us rejects the
  full9frame/two-callback exclusive admission. No nominal PHY wait is appended.
- Fail each frame0..8, late completion, malformed receipt and release: no retry,
  at most2callbacks; any attempted target mutation retains uncertain state.

[The LPI guide](lpi.md) owns the software/physical evidence distinction and Q2 plan.


## Stage31 independent Wake-on-LAN literals

Original SNLS647I tables8-90..97 pp91-93 establish these vectors independently
of production codecs and the ledger. API octet0 is first transmitted:

- MAC11:22:33:44:55:66 ->04A2=2211,04A3=4433,04A4=6655.
- Secure-ON2A:2B:2C:2D:2E:2F ->04A5=2B2A,04A6=2D2C,04A7=2F2E.
- CRC+level+enabled+magic =1181, Secure-ON adds0020 ->11A1.
  Unrelated write neutral words are1981/19A1; explicit acknowledgement1181/11A1.
- Disable owns00A1, not a reset assumption:1FA1 ->1F00 understable bits,
  with0800 command-neutral retained on the wire. Preserveunknown0040/reservedE01E.
- Pulse code0/1/2/3 contributes0000/0200/0400/0600 and means8/16/32/64cycles;
  at125MHz the independent arithmetic is64/128/256/512ns.
- Stable no-change comparison17E1 includes the preserved unnamed flag; writes
  own17A1 and preserve that flag. W0C0800 is never in stable verification.
- Direct LED mux update owns0007 only: preserveFFF8 and write0002; the owned
  IRQ route uses0004. DetectorA1 must be verified clear before route/match writes.
- Full status-only change111frames/28callbacks; unchanged35/10; explicit
  acknowledgement25/7; live status33/8. No repeat-until-clear loop exists.

[The fixture plan](wake-on-lan.md) retains exact-part W0C/RH-bit0 and live-change
qualification; neither fake acknowledgements nor pulse arithmetic prove host wake.


## Stage32 independent LED/GPIO literals

SNLS647I detailed tables8-42/43/49/73..78/85/89, independently reviewed from the
original PDF, supply the test oracles. `test/pins.cpp` does not derive expectations
from the production field catalogue.

- LED0 sources0..E contribute0000,0008,...,0070 at0025, preservingFF86 and
  forcing route0001; sources8..E contribute0040..0070. SourceF rejects.
- At0460, LED1=A,LED2=B,LED3=C with prior low nibble5 givesCBA5.
  Independent masks00F0/0F00/F000 preserveFF0F/F0FF/0FFF.
- LEDCR owned force/polarity/blink0692 preservesF96D. Blink0/1/2/3 contributes
  0000/0200/0400/0600 and selects50/100/200/500ms. ForceHIGH is0012;LOW0010.
- LEDCFG2 LED1forceHIGH0003,LOW0001;LED2HIGH0030,LOW0010.
  Polarity0004/0040 combines into0077, preservingFF88 including reserved0700.
  No LED3 drive or polarity word is legal.
- MLED swap0200 is separately owned; full0279 preservesFD86. PHYCR stretch0080
  preservesFF7F, including undocumented LED Configuration0020.
- LED1 mux0/2/4 and LED2 mux0/2/3/4/5/6/7 preserveFFF8. LED2 MII LED requires
  explicit write0 and readback, even from reset0. Pin31 codes5/6 reject.
- IO_CFG1 clock release0040 preservesFFBF; verify0040 then observe0308 bit0
  with mask0001. No write to0308 or claim of POR resampling.
- Each MMD update is literal W(0D,001F),W(0E,address),W(0D,401F),R(0E),W(0E),
  then a separately owned four-frame readback. C22 update/readback takes3frames.
  Never split the five-frame RMW under a short poll slice.

The test models BMSR consumption clearing LINK_LOST and BICSR1 capture/clear
changing PRBS indication. These are source-effect oracles, not measured LEDs.


## Stage33 independent interface and isolation literals

Audit32-33 adds the independent pin prerequisite R(17): mask0020 must be0020
for declared RMII or0000 for MII before an LED2/3 profile can mutate. GPIO3's
reset0008 (mux0) does not replace this check. Raw RC000C is consumed once by the
collector. The full pin plan is now85frames/24callbacks; LED2 IRQ/WoL append
R(17) as frame5 after their four-frame mux read, with no additional callback.

Original SNLS647I tables8-19/28/29/35/41/73/84 supply these independent vectors:

- RCSR stable mask01B3, consumed status000C, preserveFE40. OldFFFF with
  follower+shift+revision1.0+elasticity2 writesFFF2. Reserved0040 survives.
  MII owns0020 only; old01FF becomes01D3 after RC consumption and mode clear.
- RMII leader0020/follower00A0; follower shift0100; revision1.0 adds0010.
  Codes0/1/2/3 mean14/2/6/10bits and16800/2400/7200/12000packet bytes at
  +/-50ppm; +/-100ppm halves those lengths without changing electrical limits.
- IO_CFG1 RX_DV0100 plus optional clock route0040 uses0140 and preserveFEBF.
  ENHANCED clock route0 selects CLKOUT availability;1 selects LED1 availability.
  Read0308 mask0001 confirms selection, not frequency or timing.
- CR1fastRXDV0002; CR2odd-nibble DISABLE0002, preserveFFFD. GENCFG IPG0008
  selects120ns when set or200ns when clear; CR2RX_ER-idle0004 is preserved.
  PHYSCRfull-duplexCOL0010 preservesFFEB, neutralizing test0004.
- BMCR isolation RMW uses preserve71FF, owned0400 and software power0800,
  neutral commands8200. With power intent asserted, a sampledBB00 composes3D00
  for isolation and3900 for release. A live active command is preflight-rejected;
  these are composition vectors, not permission to mutate an active reset.
- Default RMII pass32frames/13callbacks; full clock+isolation39/16; full
  read-only pass19/9. MMD RMW is five indivisible frames then four-frame readback.
  RC values0000/000C stay raw history and do not establish a fault count.

## Stage34 independently transcribed quality vectors

Original SNLS647I tables8-28/29/30/35/44/51/52/53/72 give these
independent literals, checked by `test/quality.cpp` without production masks:

- CR3 causes040F preserve mirror0060; unknown0010 rejects.
- FLD_CFG1 minimum codes1/1 yield0410, with reserved7 giving0417;
  maximum63/63 yieldFFF0, reserved7 givesFFF7. Zero codes reject.
- Descrambler old2284 with six-bit8 becomes2288.
- TX100 bypass plus far-end-loopback FIFOcode3 yields owned0B00.
- TX10 lowerthreshold/NLPdisable/jabberdisable and squelch8 yield3081;
  code9 is illegal. Preamble7998 becomes3998 forlong,7998 forshort.
- A consumed10BTSCR0010 polarity sample survives later transport failure;
  its PHYSTS1000 mirror is captured first when requested.
- MSE is literal vendor31:0218, not0225; raw0 remains0 without SNR inference.

Physical maxima10us modes1-4/11us mode5 require footnote5's energy/RX condition
for modes2/3/5 and the source recipe. Override codes are explicit raw settings,
not a closed `fld_threshold_recipe` gate or measured latency.

## Stage35 independent electrical literals

Original SNLS647I tables8-19/47/48/50/73/76/79..83/86 and SNLA266B
tables5-1/2/4/5 independently establish these vectors:

- Negative amplitude28..38 runs150..50%; positive08..18 runs50..150%,
  in6.25% steps. Negative MDIX2F->30 passes2F,28,38,30; reverse passes
  30,38,28,2F. Both2F->3F->30 and30->20->2F are prohibited intermediates.
- VOD1 high2|MDInegative30|zero0 gives3C00 at MDIX30; VOD2 positive10/10
  with MDIXlow0 gives0410. Reserved VOD1C000 is preserved.
- VOD3 mdi4/enable1/mdix4/enable1 gives4A40; codes0..F and both enables
  are explicit tuning, not inferred impedance units. MACslew10/11, IOslew0/1.
- Manual MDI/MDIX updates onlyPHYCRC000; CR1robust0020 applies in forced
  mode. TX100BMCR2100; NLP10/PRBS10BMCR0100. PRBS BISCR701F=7001,
  preserving observed0100 instead of writing script7101 as a constant.
- COMPT10M writes0010..001D under the fourteen named recipe exceptions;
  001E/001F reject. COUNT1000..31 remains typed but source-gated before0428.
- Collision BMCR0080; unrelated commands/power/isolation compose neutral0,
  stop clears0080. Explicit COL mux codes3/5/6 at0305 require MII.
- CDCR4102 becomes0102 then PHYRCR8000 in the same batch; CDCR0102
  requires no reset. Any attempted HARD enforces at least2000us silence
  after completion upper bound. This is no calibration-completion deadline.

Host tests use literal words and independent fake register effects, not values
generated from the production catalogue. Physical qualification remains separate.
