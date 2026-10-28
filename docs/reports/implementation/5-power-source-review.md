# Prompt 5 power, EEE, LPI and wake source review

Independent research on 2026-10-28, for exact DP83826I / SNLS647I. No driver,
SDK image or fixture was executed. The assigned scope owns this report and a
temporary integration fragment only; the lead owns the shared ledger, checker,
guides, progress and Git. The execution branch is
`promptset/dp8382x-implementation-20261028`; entry checkpoint is
`2805cfdea3aacd1f870d27998cd99d0270cccb2a`. The lead explicitly selected
`gpt-6-astra` with high reasoning effort and `fork_turns=none` for this review's
mixed-command, address and cross-part conflicts.

Root/parent AGENTS, COMMON, EVIDENCE, ENGINEERING, FULL_COVERAGE, prompt 5 and
future owning prompts 28-31 were read. README, architecture, variants, reference
index, TI feature catalogue and the existing schema/core/diagnostic ledger were
checked. All future primary software ownership below is within prompts 11-36.
Application ownership, whole indirect/RMW transaction exclusion, bounded terminal
callbacks, distinct primary/cleanup results and read-clear collection apply.
Archives and siblings remain unmodified. The lead integrates the fresh sibling
API/CLI/test/runner audit with this silicon review.

The relevant MB85RC power path was independently re-audited read-only at
`1bf7c293549b1038b2d8f3b7a8d655fa74201e6e` (clean working tree):
`include/MB85RC/MB85RC.h` SleepState and sleep/wake contracts,
`src/MB85RC.cpp` enterSleep/wake and capability checks,
`test/test_basic.cpp` uncertain-command and failed-wake/no-retry tests,
`examples/common/DiagnosticCore.h` owner recovery sequencing, and
`tools/hil_runner.py` HIL-017/017A capability/result correlation. Its AGENTS was
read. Adopt explicit capability rejection, separate power/health and retained
uncertainty; do not copy FRAM wake time, I2C commands or its no-clock immediate
AWAKE policy into PHY observations. The PHY still needs finite frame counts
alongside time bounds and separate requested policy/current state.

## Original documents and current sources

- [SNLS647I](../../reference/source/TI-DP83826I-E-SNLS647I.pdf), August 2025:
  original PDF pages 15, 26, 27, 29 and 91-93 visually inspected, with sections
  8.3.3-8.3.6 and tables 8-19, 8-35, 8-40, 8-90..97 cross-checked. Page 15
  establishes the **NOM** column; page 91 establishes **W0C**, not W1C.
- [SNLA261A](../../reference/source/TI-Wake-on-LAN-SNLA261A.pdf), March 2024:
  original pages 7, 9 and 10 inspected. Its DP83822 examples use a different
  MAC pairing and set bit 11 to acknowledge. Neither rule overrides DP83826.
  Page 7 defines the first listed octet as byte 0. Table 2-7 additionally has
  inconsistent byte-pair descriptions; its example is not a DP83826 oracle.
- [SNLA328](../../reference/source/TI-Energy-Efficient-Ethernet-SNLA328.pdf),
  October 2019: original pages 10-11 inspected. Its implementation explicitly
  targets **DP83825**. Bypass controls are cleared there, versus set in
  SNLS647I. Its status/counter fields and minimum 36-us MAC inter-frame example
  cannot establish DP83826 register semantics or guaranteed exit timing.
- [SNLU262A](../../reference/source/TI-DP83826EVM-SNLU262A.pdf), June 2023:
  sections 2.2 and 3.2/4.1, page 11, describe MSP430/USB-2-MDIO low-power
  demonstrations and external supply measurements. They provide no missing
  EEE wire trace, safe live-update rule or DPD exit sequence. The EVM is a
  possible future fixture, not an identified fixture available in this work.
- The [TI DP83826I product page](https://www.ti.com/product/DP83826I) was checked
  on 2026-10-28: it still lists Rev. I and EEE/WoL. Targeted searches for
  `site.e2e.ti.com DP83826 EEE 203C 003C`, `DP83826 WOL 4A0 0800`, and quoted
  DP83826 EEE/Wake returned datasheets/product material, but no inspected exact
  clarification resolving these gates. This is a bounded search result.
- TI MCU+ SDK `next` was re-resolved through GitHub to
  `4b9a66f4e18e02a834961f5f834155c846cab041`. The complete
  [ethphy_dp83826e.c](https://github.com/TexasInstruments/mcupsdk-core/blob/4b9a66f4e18e02a834961f5f834155c846cab041/source/board/ethphy/icss_emac/ethphy_dp83826e.c)
  file was read, including command dispatch and IEEE-power enable/disable
  helpers. These perform BMCR RMW and return the write result; they do not
  establish physical wake, command-neutral reset/restart handling or complete
  EEE/WoL/DPD procedures. Its file notice is BSD-3-Clause.
- TI combined Ethernet repository `main` was re-resolved to
  `450af1144b8caeb58724bcbae081b409f6bf5c3a`. The complete
  [phy_ti_dp83826x.c](https://github.com/TexasInstruments/ti-ethernet-software/blob/450af1144b8caeb58724bcbae081b409f6bf5c3a/zephyr_drivers/dp83826a_driver/driver_files/drivers/ethernet/phy/phy_ti_dp83826x.c)
  was retrieved and searched for EEE/WoL/LPI/power-down implementation; no
  corresponding complete feature procedure was found. The file is Apache-2.0,
  despite the repository-wide GPL label. No reference implementation was copied.

Original PDFs retain archive hashes and TI notices. Mutable product/search
pages are navigation and bounded negative-search evidence, not immutable silicon
authority. All numerical decisions below derive from archived originals.

## Independent literal oracles, derived before the fragment

RXFCFG1 table 8-90 partitions `0xFFFF` into reserved `0xE01E` and documented
fields `0x1FE1`: CRC `0x1000`, acknowledgement `0x0800`, pulse `0x0600`, level
`0x0100`, enable `0x0080`, named mask flag `0x0040`, Secure-ON `0x0020` and
Magic Packet enable `0x0001`. Stable writable mask is `0x17E1`. Known reset is
value `0x1080`, mask `0xFFFE`: bit 0 is explicitly unknown. Acknowledgement is
W0C with unshifted neutral **1**, positioned neutral **0x0800**; all other fields
have no command-neutral value. Ordinary updates must compose that neutral even
if the read value of bit 11 was zero. Readback checks exclude the command bit.

With CRC on, Magic Packet/WoL enabled and level indication, the stable policy is
`0x1181`. An unrelated policy write is **0x1981**; explicit level acknowledgement
is **0x1181**. With Secure-ON enabled those vectors are **0x19A1/0x11A1**.
An unrelated CRC-off update of the first policy is **0x0981**, and must not
acknowledge. A generic all-command-zero composer is wrong. Pulse codes 0/1/2/3
produce positioned values `0x0000/0x0200/0x0400/0x0600`: 8/16/32/64 cycles at
125 MHz, derived 64/128/256/512 ns. These are pulse widths, not wake deadlines.

API byte 0 means the first transmitted octet of the conventionally displayed
MAC address. Exact table 8-92 calls **TI byte 5 MSB**: TI MAC byte 5 maps to API
octet 0, TI byte 4 to API octet 1, continuing down to TI byte 0/API octet 5.
Therefore MAC `11:22:33:44:55:66` gives **04A2=2211, 04A3=4433, 04A4=6655**.
Secure-ON `2A 2B 2C 2D 2E 2F` gives **04A5=2B2A, 04A6=2D2C, 04A7=2F2E**:
password octets retain their explicitly numbered sequence and do not inherit
the MAC's numeric significance convention. The MAC tables pair high:low **TI**
bytes **4:5, 2:3, 0:1**; password tables pair **1:0, 3:2, 5:4**.

Independent pin-review cross-check found an error in the first power fragment:
it equated TI MAC byte0 with API octet0 and produced5566/3344/1122. Re-reading
the original MSB label confirms that those expected words were wrong. The
SNLA261A generic byte0-first explanation must not override the exact DP83826
table's explicit significance label. The lead owns the corresponding ledger,
guide and independent-test correction to2211/4433/6655; this is a source-review
correction, not hardware qualification. The final shared ledger's six MAC field
descriptions and `wol_configure` were re-read and confirm this correction. The
temporary integration fragment is obsolete and must not replace the corrected
shared records.
All six words reset to zero, all 16 bits are ordinary RW, and each octet admits
0..255. Extreme vectors all-zero/all-FF produce 0000/FFFF in every word; the
asymmetric vector, not round-trip conversion, detects swapped octets/words.
The actual packet begins with six FF sync octets followed by 16 complete MAC
repetitions and optional six password octets; FCS/packet framing are MAC-owned.

PHYSCR active-sleep mask/value is **7000/6000**; BISCR observation **0100** means
normal and **0000 under mask0100** means asleep. Thus PHYSCR=6108 and BISCR=0100
is a valid successful enable while awake. BMCR IEEE request mask is **0800**;
compose reset/restart bits **8200** neutral zero and preserve isolation0400.
PHYSCR PLL disable is **8000**, conditional on IEEE power-down. The separate DPD
recipe sets **0428 bit0004 before BMCR0800**; compliance bits0030 must not change.
No complete inverse DPD sequence or guaranteed wake completion is documented.

EEE's 18 printed pairs, in order, are:

`001F=8000,203C=0002,04D1=008B,04D3=4F12,04DF=0180,033E=A681,`
`033F=0003,0123=0800,031B=8848,0466=FE00,04CF=261D,0416=1F30,`
`04F5=2864,04E0=FFF2,031F=FE36,0308=0000,04F4=0800,0000=3300`.

The first and last are C22 controls; **203C remains printed notation**, not a
wire offset. Full-word exceptional actions record the other 15 words only for
this recipe, never generic writable fields. EEECFG3 prose supports set bits0/3
(`mask0009/value0009`) and TX-LPI start/stop (`mask1000/value1000/0000`). It does
not document their general read semantics, remaining fields, reserved mask,
reset word or a safe arbitrary-word RMW policy. Recipe values are not defaults.

Page 15's four 100-Mbit EEE values are all **NOM**: sleep **210 us**, quiet
**20 ms**, wake **36 us**, refresh **200 us**. No MAX is supplied. The table does
not define precise measurement edges; retain those missing qualifiers instead
of assigning a write-to-complete deadline. Active-sleep NLP **1.4 s** is a stated
interval, not maximum partner discovery, entry or wake time.

## Coverage and unresolved source gates

Seven detailed WoL rows become complete field records. Thirteen opaque EEE rows
become RECIPE with unknown reset/reserved masks and no invented fields; the
already researched shared RECIPE_0123 gains only a separate EEE action. The lead
owns shared RECIPE_0428 and the clock record. Their source exceptions must remain
bounded by the exact action masks and participating owners.

Five printed associations are researched but source-blocked: MMD3
1001/1014/1016 and MMD7 203C/203D. Exact DP83826 prose explicitly establishes
203C bit1 advertisement only. Capability, partner, LPI indication and error-count
roles remain required scope, but their complete DP83826 field/access/reset maps
are absent from its detailed register catalogue. SNLA328's DP83825 table4 names
1001 bits11/10 latched high and9/8 current; table5 names1014 bits2/1 capability;
section2.3.3 describes an RC saturating wake-error counter cleared by PCS reset.
These remain comparison facts. They do not authorize DP83826 reads, clear
assumptions, digit stripping, 1000-Mbit support or hidden aliases.

`eee_wire` requires exact-part selector/address/data traces or authoritative TI
clarification for all five associations, including offsets above04D1 in the
recipe. `eee_control_semantics` requires exact control/read/reset/disable facts
and safe handling of unspecified EEECFG3 bits. `deep_power_exit` requires the
complete DPD exit and clock/SMI-ready behavior, plus safe shared0428 composition.
`wol_bit_mask` blocks interpreting the merely named RXFCFG1 bit6 as a custom
pattern engine. `wol_live_reconfigure` retains the absence of a documented atomic
live credential-update/pending-indication guarantee: quiesced/disabled setup
can proceed independently, but continuous live-match guarantees cannot.
The existing RXFS SFD-reference and pin conflicts remain in their owning gates.

## Whole procedures and later acceptance

| Procedure / owner | Required behavior and bounds | Independent acceptance / fixture plan |
| --- | --- | --- |
| power_ieee /28 | One composed BMCR RMW then stable readback; at most3 direct target frames for that change, plus explicitly budgeted preflight/cleanup. Clearing the request cannot override an asserted ENHANCED PWRDN pin. | Test isolation retained, command bits neutral, accepted-write failures, asserted external pin and no false awake result. Future identified board observes clocks/power and MDIO under both pin roles. |
| power_active_sleep /28 | Apply7000=6000, verify policy, take one BISCR sample. No wait-until-sleep loop; partner presence can keep normal state indefinitely. | Enable while linked/awake succeeds; later BISCR0/1 changes are observations. Future fixture records NLP, partner insertion and current draw without claiming1.4s wake bound. |
| power_pll_control /28 | Only admitted while IEEE power-down; one PHYSCR8000 RMW/readback. Clock dependency belongs to application. | Reject unmet prerequisites before mutation; no inference of PLL running from software setter alone. Measure supplied/generated clock paths separately. |
| power_deep /28 | Source-gated0428 bit2 then IEEE request, preserving compliance; no invented readback/inverse. Separate finite reset/reinitialize recovery if explicitly admitted. | Zero I/O until essential gate closes; test every accepted/uncertain prefix and failed cleanup. Future fixture captures exact MDIO and clock/rail behavior. |
| eee_initialize /29 | Gate before first destructive reset; exact18-step recipe, acyclic replay of other owned settings. Nominal recipe floor is2 direct+16 indirect targets=66 C22 frames only after mapping is proved; reset polls, checks and cleanup are additional bounded work. | Literal order/all18 values, overlap0123, clock0308 and reset invalidation; every failure/cancel boundary; no arbitrary opaque readback or guessed disable. Future EEE-capable/noncapable peers,10/100 and MII/RMII, identified traces and power measurements. |
| eee_negotiation_status /29 | Bypass0009 set, proven advertisement route, AN restart through sole negotiation owner, separate capability/request/partner/result validity. Unknown status reads remain blocked. | All capability/peer combinations, stale epochs, destructive evidence preservation after clarification, no LPI if negotiation unsupported/incomplete. |
| legacy_lpi /30 | MAC stops/drains TX; qualified EEE first; set1000, finite session, clear1000, establish readiness before releasing TX. No PHY buffering. | Fail/cancel at each side effect; preserve TX-may-remain-inhibited until proven exit, even after teardown. Future waveform/traffic captures assess real peer timing;36us NOM is never sufficient proof alone. |
| wol_configure /31 | Whole request validated; quiesce/disable, preserve pending collector evidence, write/verify six credentials and stable policy, establish IRQ/GPIO route, enable last with W0C neutral. At most40 logical target accesses in proposed base profile; delegated pin/IRQ work consumes the same finite aggregate budget. | Literal MAC/password/CRC/pulse/level combinations and invalid lengths/codes; every target failure; RXFS/MISR partial samples retained. Future valid/bad CRC/wrong password/malformed packets at10/100, pulse widths and MCU wake measured separately. |
| wol_acknowledge /31 | Retain collector status first; explicit0 only at bit11 preserving stable settings. One composed target write; any RMW/status prerequisites count separately. Never acknowledge during unrelated updates. | Pending event survives1981/19A1;1181/11A1 acknowledge. Repeated cached views perform no I/O. New events during ack remain an ordering uncertainty, not a lossless queue claim. |

All procedures use finite primary and cleanup **duration and frame/transaction
counts**, bounded callbacks and owner-admission time. Count C22 versus complete
indirect transactions separately; admit each whole transaction before starting.
An indirect target normally costs4 C22 frames; atomic RMW must include both
directions under the same controller lock. Release that lock before scheduled
waits or output. Zero work budget performs no I/O. Do not refresh the overall
deadline on each poll; counts terminate even if a clock freezes.

Late/failed terminal callbacks preserve accepted or uncertain effects and every
successfully consumed event. Cancellation stops new primary work and retains
the original result; subsequent cleanup has its own finite allowance and cannot
erase the primary failure. No implicit reset, retry or power cycling. Persistent
desired policy is separate from observed bits and configuration trust. Temporary
LPI/diagnostic/ack commands are not replayed. Hard reset invalidates old
observations and requires an acyclic identity/clock/interface/persistent-state
restore; PCS reset and restart keep their separately researched domains. No
opaque recipe word can be restored by an invented readable snapshot.

## Evidence status and handoff

The subsequent read-only cross-review visually checked SNLS647I pages75,81-83
and SNLA266B page13. Pin-specific mux/access/reset facts and COMPT's10M codes0..D
versus100M count0..31 agreed with the originals. Three procedure findings were
sent to the lead and their final shared-ledger corrections were verified:

- `compt_reserved` and `compliance_session` now block the0428 branch **before
  any preservation read**, pending exact read effects and safe unknown-bit
  composition. The source-described bit writes do not grant opaque reads.
- Compliance cleanup now stops or completes the authorized hard reset **before
  restoring any COMPT nibble or persistent profile**, retaining possible emission
  if stop cannot be established. Temporary stimuli are never restored.
- `pin_configure` now states its24+16 frame allowance is local work only;
  source/IRQ/WoL operations and cleanup reserve additional finite counts under
  the same aggregate durations, with nonrecursive dependency coordination.

All three review findings are **resolved in the research contracts**. Their
executing implementation and physical acceptance remain future work.

Original-PDF research and independent literal derivation: **PASS** for the
identified pages and facts. Fragment JSON parsing and all seven complete field
partitions/reset-mask checks: **PASS**, Windows/PowerShell, Python3.12. The
fragment contains20 registers,9 procedures,19 actions,5 timing records and5
blocked printed associations. `python tools/verify_references.py`: **PASS**,
27 vendor artifacts,25 extracts,1209 links in142 Markdown files at this review
checkpoint (the lead may add more documents). Host driver, portable/package
consumer and SDK link: **NOT RUN**
(no implementation in this stage). Remote CI: **NOT RUN**, reserved for prompt71.
Physical evidence: **NOT RUN**, no authorized fixture actions. Essential EEE/DPD
source-dependent operations remain **BLOCKED** by the named gates. This does not
block the independent IEEE policy, active-sleep policy or established WoL field
work. Lead-owned final ledger/tests/reference/diff checks and commit/upstream
equality determine the completed prompt checkpoint.
