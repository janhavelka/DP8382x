# Prompt 5: independent pin, interface and electrical source review

Reviewed 2026-10-28 on execution branch
`promptset/dp8382x-implementation-20261028`, predecessor
`2805cfdea3aacd1f870d27998cd99d0270cccb2a`. The lead owns shared ledger,
schema, source/peer integration and Git. This delegated review owns this report
and its temporary integration fragment only. The lead explicitly selected
`gpt-6-astra` with high reasoning and no history fork for conflicting silicon
tables and independent cross-review; no driver or fixture was operated.

Read root and parent agent rules, COMMON, prompt 5, EVIDENCE, entry guides and
ledger schema. Apply one transaction owner, reserved-bit preservation, independent
literal checks, finite primary/cleanup budgets and separate configuration trust.
Remote CI belongs to prompt 71. Current sibling and manufacturer source audits
are integrated by the lead's prompt-5 audit; this report is the independent
original-PDF comparison, not a second claim to have checked those repositories.

Sources are the archived original PDFs, with per-document TI copyright and
unaltered provenance. PyMuPDF read the original page content; original page 82
and 90 renders were also inspected to verify access and reserved columns.

| Source | Revision | SHA-256 |
| --- | --- | --- |
| [SNLS647I](../../reference/source/TI-DP83826I-E-SNLS647I.pdf) | August 2025 | `4e440fb8bd1cd44ee9ab0773944bef4f0605884f25c4ab96ea55d0ed859fd9c3` |
| [SNLA266B](../../reference/source/TI-DP8382x-Compliance-SNLA266B.pdf) | April 2025 | `74af41cba6912b8734097332b7e766105feaa8fb015321f47d345b7596aa13d5` |
| [SNLU262A](../../reference/source/TI-DP83826EVM-SNLU262A.pdf) | June 2023 | `1500afd71523db029c092f456b0d56f934f0c911742605cde31f3dfa9a16f940` |
| [SNLA344C](../../reference/source/TI-DP83826-EtherCAT-SNLA344C.pdf) | Archived revision C | `bd66fc2520bf564b9ec2802308af1b619c7bf3b5bda12f8dcfdee042a284446f` |

## Independent literal oracle, derived before the integration fragment

These values come from tables 8-42, 8-49..51, 8-73..80, 8-82..86 and 8-89.
The known mask excludes unspecified or contradictory strap defaults. A reset
value is observation metadata, never a word to write. Reserved fields remain
reserved even where their access column says R/W.

| Register | Reset known value | Known mask | Reserved mask | Principal legal writable masks |
| --- | --- | --- | --- | --- |
| LEDCR | 0400 | FF7F | F96D | 0600 blink, 0080 polarity, 0010 drive, 0002 value |
| MLEDCR | 0041 | FFFF | FD86 | 0200 swap, 0078 source, 0001 route |
| COMPT | 0000 | FFFF | FFF0 | 000F pattern; bit 4 is exceptional only |
| 10M_CFG | 7998 | FFFF | BFFF | 4000 short preamble |
| IO_CFG1 | 0000 | FEFF | 3EBF | C000 MAC impedance, 0100 RX_DV, 0040 clock disable |
| LED0_GPIO_CFG | 0008 | FFFF | FFC0 | 0038 clock selector; mux 0007 is R |
| LED1_GPIO_CFG | 0008 | FFF8 | FFC0 | 0038 selector, 0007 per-pin mux |
| LED2_GPIO_CFG | 0008 | FFFF | FFF8 | 0007 per-pin mux |
| LED3_GPIO_CFG | 0008 | FFFF | FFF8 | none; mux 0007 is R |
| CLK_OUT_LED_STATUS | 0002 | FFFF | FFFE | none; bit 0 is R, ENHANCED-only status |
| VOD_CFG1 | 3C00 | FFFF | C000 | 3000 MDIX minus high, 0FC0 MDI minus, 003F zero |
| VOD_CFG2 | 0410 | FFFF | 0000 | F000 MDIX minus low, 0FC0 MDIX plus, 003F MDI plus |
| ANA_LD_PROG_SL | 0080 | FFFF | FFFF | none |
| ANA_RX10BT_CTRL | 0008 | FFFF | FFE0 | 001F raw receiver setting; physical code mapping gated |
| GENCFG | 0008 | FFFF | FFF7 | 0008 minimum IPG |
| LEDCFG | 5665 | FFFF | 000F | F000 LED3, 0F00 LED2, 00F0 LED1 |
| IOCTRL | 0010 | FFFF | FFE0 | 001F, only 10/11 documented |
| LEDCFG2 | 0000 | FBBB | FF88 | 0040/0020/0010 LED2; 0004/0002/0001 LED1 |

MLEDCR source values 0..E are documented, F reserved. Thus source E positions
as 0070; source E with required route 1 is 0071. Route encoding 0 is reserved.
LED1 source A at 0460 positions as 00A0, LED3 source E as E000. Starting with
reset 5665, replacing LED1 alone yields 56A5; replacing LED3 alone yields E665.
LEDCFG2 has no documented LED3 control: bits 10, 9 and 8 are reserved, including
the unknown reset of bit 10. Its ordinary writable mask is 0077, not 0777.
LED0 and LED1 clock selectors allow only 5/6 (0028/0030). The reset selector 1
is reserved for active clock output and is not a legal requested clock source.
LED1 mux legal values are 0/2/4/5/6; LED2 allows 0/2/3/4/5/6/7. A single GPIO
enum accepting every pin/function combination would be incorrect.

VOD minus amplitude codes 28..38 represent 150% down to 50%; plus codes 08..18
represent 50% up to 150%. For 150% all four directional nonzero amplitudes,
VOD_CFG1=2A00 and VOD_CFG2=8618 when zero code stays zero. For 50%, values are
3E00 and 8208. The MDIX minus code is split across two words: validate the full
six-bit code before any transfer, then update both under one owner; a first-word
success and second-word failure is partially applied configuration.

CR2 bit 1 set disables odd-nibble detection: starting at 0100, disable gives
0102 and enable returns 0100. The literal 0001 in SNLA344C's example targets a
reserved bit and does not implement this control. GENCFG bit 3 set selects
120 ns IPG, clear selects 200 ns; these are configured spacing values, not
operation deadlines. IOCTRL field 10 is fast, 11 slow; example 0400 touches
reserved bit 10 and is not accepted. IO_CFG1 field 0 is slow and 1 fast;
encodings 2/3 are not documented.

## Source conflicts and permitted independent work

Pin/clock gate remains explicit. Figure 8-10 and section 8.3.8 promise functions
that cannot be selected through the detailed writable fields: LED0/3 mux is R;
LED2/3 clock-select bits are reserved; LED1 clock-select activation requires mux
1, which is reserved in its own table. The text says CLKOUT may be disabled by
selecting an input but supplies no legal input mux code. IO_CFG1 bit 6 does have
a documented ENHANCED clock/LED1 selection, and CLK_OUT_LED_STATUS observes it.
LEDCFG sources and documented LED0/1/2 drive/polarity remain independently usable.
PHYCR bit 5 has no encoding description and cannot acquire arbitrary meaning
from an inherited vendor literal. No LED3 drive/polarity register is invented.
An authorized exact-part pin fixture must resolve each disputed pin/mode with
MDIO and oscilloscope traces before those functions can be claimed.

ANA_RX10BT_CTRL describes a five-bit field but only a 200..575 mV span in 25 mV
steps (16 levels), without a code-to-voltage table. Retain the raw field and
reset 8; block a physical-unit conversion and out-of-profile programming until
TI supplies exact encodings. ANA_LD_PROG_SL is all reserved; no tuning permission
arises from its analog name. SNLA266B section 4 explicitly tunes DP83825 and is
not a DP83826 analog recipe.

COMPT's detailed table marks 15:4 reserved while its description uses bit 4.
SNLA266B table 5-5 independently prescribes exact COMPT=0013 after hard reset
and forced 10 Mb/s for repetitive-one harmonic testing. This licenses that
whole-word exceptional action, not arbitrary reserved writes or other patterns.
Other 10 Mb/s patterns are documented in COMPT's low nibble (0..D), but an
expanded exact enable recipe remains gated. The 100 Mb/s pattern count joins
0428 bit 5 with COMPT bits 3:0; zero clears the shift register and 1..31 selects
the count of zeros following one. Disable 0428 bit 4 before reconfiguration,
set count, then enable bit 4. Keep bit 2 deep-power policy independent. These
are explicit procedure-only bit actions on 0428; field/reset semantics for that
opaque register remain unknown. The 100 Mb/s branch is blocked before even a
preservation read until exact read effects and safe unrelated-bit composition
are established. Setting a documented bit does not establish that the opaque
register can safely be read for RMW. The proposed frame counts apply only after
that source gate closes. No generic whole-word restore is established.

SNLA266B tables 5-1/2/4 supply standard 100 Mb/s, 10 Mb/s link-pulse and 10 Mb/s
PCS/BIST stimuli; table 5-3's analog-loopback recipe is DP83822-only. Its shared
PHYCR literal 0021 includes undocumented LED configuration and read-only PHY
address, so ordinary software uses the already established forced/crossover and
BIST contracts instead of blindly replaying that word. DP83826 10BASE-Te always
retains its exact VOD_CFG3=4A40 prerequisite. Stop/abort is not specified by the
compliance script: cleanup reserves authorized reset and persistent profile
reapply, records possible emission until proven stopped, and keeps primary and
cleanup errors separate. Reset is never restoration of a temporary test mode.
Stop or reset must complete before restoring COMPT or any persistent profile;
restoring its low pattern nibble while exceptional bit 4 may still be enabled
could emit a different stimulus. Source/IRQ/WoL work delegated by pin setup
requires its own reserved finite counts inside the aggregate primary and
cleanup durations, beyond the local mux transfer allowance.

Searches on 2026-10-28 for exact-part IOCTRL/COMPT/LEDCFG2 corrections did not
find authoritative corrections resolving these conflicts. The inspected
[TI support thread](https://e2e.ti.com/support/microcontrollers/arm-based-microcontrollers-group/arm-based-microcontrollers/f/arm-based-microcontrollers-forum/1311884/am2432-ethercat-can-t-be-scanned-if-power-on-first-and-then-connect-network-cable)
directs a user to SNLA344C and repeats its problematic literals; it does not
correct their field placement. No forum code or vendor implementation was copied.
SNLU262A page 11 supports planning two-EVM repeater and clock fixtures; its page
17 schematic demonstrates concrete pin/LED/clock connections and an E-suffix
device, which does not establish measured I-suffix behavior.

## Procedure ownership and acceptance

The fragment adds LED and pin programming (32), clock/MII/RMII/repeater and
compatibility/isolation (33), 100/10 data-path controls (34), analog/compliance
and collision testing (35), and link recovery/extended duplex (18). Existing
next_page_exchange remains owned by 17 and retains its phase/load/freshness gate;
no new procedure duplicates it. Crossover is still source-proved forced-mode
ownership plus robust MDIX, not an inference from extended-duplex enable.

Every procedure records finite aggregate duration and primary/cleanup transfer
counts, cancellation before/after mutation, accepted/uncertain effects, reset
restoration and identified fixture requirements. Ordinary configuration does
one RMW and one verification per affected stable register (maximum 12 MDIO
frames per MMD register with explicit selector setup each time); cleanup permits
one restoration and verification pass, with a separately bounded reset recovery
only where authorized. Mixed RCSR and 10BTSCR reads go through their collector
and retain consumed samples. Polling, if any, has both finite count and time
bounds; nominal electrical timing is not promoted to a deadline.

Host acceptance includes all legal extremes and forbidden encodings, pin/mode
conflicts, zero-I/O rejection, reset-known masks, field composition, failure at
every transfer boundary, partial amplitude updates, collector retention and
separate cleanup verdict. Physical plans require actual part/board revision,
strap mode, firmware/runner identity, pin routing, partner and calibrated
measurement instruments. LED/clock scope traces, malformed MII symbols,
two-PHY repeater traffic, calibrated MDI amplitude and compliance captures are
separate from successful register readback. No fixture actions are authorized
by the research task.

Independent fragment structural checks: PASS for all 18 register field partitions,
reset masks and field-owner backlinks, and all 15 procedure writable masks and
feature backlinks. The check built a merged view in memory without editing the
shared ledger. `git diff --check` on the report: PASS.

Original-PDF review: PASS for stated research scope. Driver host behavior,
portable/package consumers, SDK links, remote CI and physical evidence: NOT RUN.
Ledger/link/tests and final checkpoint/sync are owned by the lead integration.

## Cross-review of power, EEE and WoL integration

Reviewed the integrated power/EEE/LPI/WoL procedures and exceptional actions
against original SNLS647I pages 26-29 and 91-93. Page 92 was also rendered and
visually checked. The power-policy distinction, BISCR state polarity, W0C
acknowledgement neutrality, exact 18-step EEE recipe, nominal timing labels and
deep-power/EEE source gates agree with the original source. Separate finite
cleanup budgets and preservation of accepted or uncertain effects are retained.
Unresolved printed EEE addresses remain separate from wire tuples; status reads
and opaque RMW require prior source clarification. No physical qualification
was added by this cross-review.

One substantive integration defect was reported to the lead: the initial WoL
MAC mapping equated TI byte labels directly with API octet indices, despite
table 8-92 explicitly marking Byte 5 as MSB. With API octet 0 defined as the
first transmitted/canonical most-significant MAC octet, TI Byte 5 maps to API
octet 0 and TI Byte 4 to API octet 1. Consequently MAC
`11:22:33:44:55:66` derives `2211,4433,6655` at 04A2/04A3/04A4;
`02:12:34:56:78:9A` derives `1202,5634,9A78`. The initially integrated
`5566,3344,1122` vector ignored the MSB annotation and must be corrected in
the ledger, tests and guides (or explicitly source-gated if the mapping remains
disputed). This derivation uses the DP83826 original table, independently of
the matching Linux code. Secure-ON table labels are a separate mapping and
must not be reversed merely to match the MAC convention.

The lead corrected the MAC vectors and integrated the other reviewer's opaque
0428 read gate, stop-before-profile-restoration ordering and delegated pin
operation budgets. Final read-only review verified those changes in the ledger.
The original-derived numeric constants in `test/test_remaining_ledger.py`
agree with the reviewed PDF tables: reset/field partitions, LED placements,
reserved pin encodings, VOD extremes, WoL neutral values and corrected octet
order, and EEE/compliance action literals. On the Windows Python 3.12 host,
`python -m unittest discover -s test -p test_remaining_ledger.py -v` passed
all 17 tests. These checks validate research records, not executing PHY behavior.
