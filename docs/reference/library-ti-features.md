# TI feature facts and public-driver comparison

Planning evidence reviewed **2026-10-26**; no implementation or measurement is
claimed. This supplements the [complete inventory](dp83826i-feature-inventory.md)
and [public-source index](public-research.md), especially prompts 23-36. Reopen the
exact source and current implementation before using a row. Facts below are
navigation and independent test-vector inputs, not permission to copy a driver,
write reserved fields generally, or assume a sequence has been tested.

Evidence labels: **D** = stated by the archived exact-part datasheet; **C** =
corroborated by another inspected primary source; **O** = observed software
behavior only; **X** = conflicting evidence; **U** = unresolved interpretation or
missing bound. Derived masks/arithmetic are identified as such. D/C is not HIL
qualification. All physical checks remain NOT RUN.

## Pinned sources and audit limits

- **DS**: [SNLS647I, August 2025](source/TI-DP83826I-E-SNLS647I.pdf), original
  DP83826I/E. Section/table references below identify this document. PDF pages
  15, 74, 87 and 88 were visually inspected in this pass to resolve timing
  columns, LED numbering and field widths. Other rows were read from the
  archived derived text; reopen PDF tables whenever interpretation is uncertain.
- **MCU-C/H**: TI MCU+ SDK
  [source](https://github.com/TexasInstruments/mcupsdk-core/blob/4b9a66f4e18e02a834961f5f834155c846cab041/source/board/ethphy/icss_emac/ethphy_dp83826e.c)
  and [header](https://github.com/TexasInstruments/mcupsdk-core/blob/4b9a66f4e18e02a834961f5f834155c846cab041/source/board/ethphy/icss_emac/ethphy_dp83826e.h),
  commit `4b9a66f4e18e02a834961f5f834155c846cab041`, BSD-3-Clause file notices
  at lines 1-31. The repository's default branch **next** still resolved to that
  commit on review; **main** resolved to older
  `517a277791c1de252189bf2d650395726b494dd1`. Do not infer freshness from a
  conventional branch name. This ICSS driver is not a portable implementation.
- **Z-C/B**: TI combined repository's
  [Zephyr source](https://github.com/TexasInstruments/ti-ethernet-software/blob/450af1144b8caeb58724bcbae081b409f6bf5c3a/zephyr_drivers/dp83826a_driver/driver_files/drivers/ethernet/phy/phy_ti_dp83826x.c)
  and [binding](https://github.com/TexasInstruments/ti-ethernet-software/blob/450af1144b8caeb58724bcbae081b409f6bf5c3a/zephyr_drivers/dp83826a_driver/driver_files/dts/bindings/ethernet/phy/ti,dp83826x.yaml),
  commit `450af1144b8caeb58724bcbae081b409f6bf5c3a`, also the observed **main**.
  Both files carry Apache-2.0 identifiers; the repository-wide GPL label is not
  a substitute for per-file licensing. No code is imported. This is a TI-hosted
  example, not evidence of upstream Zephyr support or a tested SDK baseline.
- **DBG**: [SNLA423A](source/TI-DP83826-Troubleshooting-SNLA423A.pdf), June 2025,
  sections 2.3.3 and 2.5. **COMP**:
  [SNLA266B](source/TI-DP8382x-Compliance-SNLA266B.pdf), April 2025, section 5.
  [SNLA328](source/TI-Energy-Efficient-Ethernet-SNLA328.pdf) and
  [SNLA261A](source/TI-Wake-on-LAN-SNLA261A.pdf) are cross-part references, not
  replacements for DS register semantics.
- The [TI core catalogue](library-ti-core.md) records the independently reviewed
  RTOS driver lines and source revision behind the LED mask/pin comparisons.
  Apply the shared [evidence contract](../prompts/EVIDENCE.md) to all these rows.

GitHub connector reads succeeded. Source lines refer to the pinned files above;
the audit covers these feature implementations and associated tables, not every
SDK dependency, board, revision or source on the internet. Archived vendor bytes
and hashes are unchanged. Real MDIO traces, silicon clarification and suitable
measurements must close the unresolved rows before support is claimed.

## Public-driver findings that become negative tests

| Evidence | Inspected source and concrete observation | Required disposition |
| --- | --- | --- |
| O/X | MCU-C 535-546 `enableOddNibbleDetection` sets CR2 bit 1 (`0x0002`) | DS table 8-29 says **0 enables**, 1 disables odd-nibble error detection. Do not preserve the function's advertised meaning; test both literal polarities. |
| O/X | MCU-C 67, 551-563 calls CR2 bit 2 (`0x0004`) enhanced IPG detection | DS calls it RX_ER during IDLE. Keep it separate from GENCFG `0x0456[3]`, which selects 120 ns versus 200 ns minimum IPG (table 8-84). |
| O/X | MCU-C 82-84, 452-467 assigns LED1 shift 12 and LED3 shift 4 | DS table 8-85 says LED3 shift 12 and LED1 shift 4. Independent per-LED vectors must reveal a swap; matching two TI drivers does not resolve the conflict. |
| O | MCU-C 328-346 identifies a part solely by ID1 `0x2000` | Read/decode both ID registers and qualify exact modes/revisions. ID1 agreement alone cannot admit another model. |
| O | MCU-C 349-374 masks FLD input with `0x040F`; 377-518 shifts LED mode/rate without range rejection; LED switches lack default errors | Reject unknown FLD bits, LED index outside 0-3, source `0xF`, and blink codes outside 0-3 before I/O. Masking invalid input or a no-op success is not validation. |
| O | MCU-C 296-309 returns after writing PHYRCR restart `0x4000` | Accepted request is not observed restart completion; no bounded completion procedure is supplied. |
| O/X | Z-C 138-191 selects MMD data mode using `(0x4000 & devad)` | For legal 5-bit DEVAD this is zero. Test the entire indirect wire trace, data-mode selector and whole-transaction exclusion; a per-call mutex is insufficient. |
| O/U | Z-C 202, 272, 539, 597 uses `K_FOREVER`; 550-573 busy-waits 1 ms for RESET_N and 50 ms after GPIO/BMCR reset | These are SDK wait policies, not exact reset-kind completion bounds. Locks must have aggregate deadlines and be released between scheduled waits. This file does not establish a bounded poll algorithm. |
| O/X | Z-B accepts 5000-15000 basis points in steps of 625, default 10000; Z-C 343-407, 884-898 passes those values directly into DAC bitfields and compares them with raw `0x30`/`0x10` defaults | No percentage-to-code conversion occurs in that path. Test independent endpoint/default vectors, not only round trips through the same conversion. |
| O/U | Z-C 53-55 defines VOD_CFG3 `0x030E`, but `config_vod` writes only `0x030B/0x030C` | A declared address does not implement the DS `0x030E=0x4A40` 10BASE-Te requirement or full analog controls. |
| O | Z-C 194-217 consumes MISR1 and discards the event word; 493-526 enables only link-change IRQ | Retain consumed samples, cover both MISRs and preserve already acquired evidence on later failure. This is not full interrupt support. |
| O | Z-C 591-594 rejects forced mode; 851-854 ignores `cfg_link` result during init | Driver limitations are not silicon limitations. Test initialization failure propagation and exact forced speed/duplex paths. |
| O/X | Z-C 33-35 labels `0x2000A131` as NC | Reconcile with the exact-part identity catalogue and pinned TI Linux labels. Never select a variant by that label alone. |

## Loopback and BIST anchors

| Evidence / DS location | Register facts and procedure | Scope and test implication |
| --- | --- | --- |
| D/X, 8.3.14; tables 8-19, 8-40 | BMCR `0x0000[14]` enables MII loopback; BISCR `0x0016[4:0]` selections: PCS input `0x01`, PCS output `0x02`, digital `0x04`, analog `0x08`, reverse `0x10` | PCS input is 10BASE-Te only; digital is 100BASE-TX only. Table 8-19 requires MII companion `0x04` at 100 Mbit/s and `0x01` at 10 Mbit/s, overriding generic prose's universal bit 2 recipe absent stronger evidence. Validate combinations, not independent freely OR-able flags. |
| D, 8.3.14.1; table 8-40 | Disable AN before near-end loopback; external loopback is the stated exception. Analog requires 100-ohm termination. BISCR bit 6 `0x0040` transmits onto the line during MII loopback only at 100 Mbit/s | Do not advertise internal-path success as peer/MAC/cable success. Reject transmit-in-loopback in other modes/speeds. |
| D/C, 8.3.14.4; DBG 2.5.1 | Ordered digital 100-Mbit example: `0000=2100`, `0016=0104`, `0122=2000`, `0123=2000`, `0130=47FF`, `001F=4000` (hex) | Exact recipe includes procedure-only addresses and restart. `0x0123` overlaps the EEE recipe; capture/reconcile ownership and restore. The example's whole-word constants are not general RMW masks. |
| D, 8.3.15; table 8-40 | BISCR `[13:12]`: 0 off; 1 generator single constant-data packet/checker off; 2 checker only; 3 generator plus checker/continuous packets. Bit 14 controls **error counter** single versus wrapping mode | Do not confuse continuous packet generation with error-counter wrapping. Generator-active `0x0200`, checker-lock `0x0800`, sync-loss `0x0400` (RH) are separate evidence. |
| D, tables 8-45/46 | BICSR1 `0x001B[7:0]` IPG is code times 4 bytes; reset `0x7D` = 500 bytes. `[15:8]` is an 8-bit error count; writing bit 15 locks a sample and clears the internal counter. BICSR2 `0x001C[10:0]` is packet length in bytes, reset `0x05EE` = 1518 | Derived masks `0x00FF`, `0xFF00`, `0x07FF`. Preserve IPG when issuing snapshot command; `0x807D` is the default-IPG example. Saturation is `0xFF` in single mode; wrapping cannot prove an aggregate error total. Bit width alone does not prove every length/IPG value generates valid frames. |
| X, DBG 2.5.3 versus DS | DBG uses reverse `0x0020`, PRBS lock at `0x0017[11:10]`, generator `0x5000`, and an analog example expecting `0x3B04` after `0x3108` | DS places reverse at BISCR bit 4 and lock/sync-loss at BISCR bits 11/10, not RCSR. `0x5000` does not select both generator/checker. Preserve these as rejected vectors, not executable recipes. DBG's one-second observation interval is a procedure choice, not a silicon completion maximum. |

## Cable controls and remaining interpretation gates

DS 8.3.16 permits TDR with a disconnected or quiet partner, including configured
autorun on link drop. A link-down bit does not itself prove the far end is quiet.
The host converts position to distance using cable propagation information. The
prose's 1 V pulse and plus/minus 1 m accuracy are source claims, not measurements
of this board. No exact DP83826 maximum TDR-completion bound was established by
these reviewed sources; caller budgets remain software policy.

| Evidence / DS table | Fields and legal-value anchors | Required distinction |
| --- | --- | --- |
| D, 8-28, 8-47 | CR1 `0x0009[8]` = autorun; CDCR `0x001E[15]` = manual start, `[1]` done, `[0]` fail; masks `0x0100`, `0x8000`, `0x0002`, `0x0001` | Start clears on completion; existing done/results need freshness treatment. CDCR reset is `0x0102`, so an initially set done bit is not proof of this session. |
| D, 8-54 | CDSCR `0x0170`: bit 14 same-channel when 1/cross-channel when 0; bit 13 A=0/B=1 TX; bit 12 retains DC-removal state; `[10:8]` averaging codes 0-6 = 1,2,4,8,16,32,64 cycles; 7 reserved | Masks `0x4000`, `0x2000`, `0x1000`, `0x0700`; reject reserved averaging before transfers. |
| D, 8-54 | CDSCR `[6:4]` segment codes 1-5 = 0-10, 10-20, 20-40, 40-80, 80+ metres; 0,6,7 reserved | Do not derive classification or unbounded maximum length from the last interval. |
| D, 8-56/57/60 | For segments 1..5: CDSCR3 `0x0173[15:8]` duration = `0D,0D,1A,34,8F`; `[7:0]` initial skip = `07,14,21,3B,6F`; TDR_175 `0x0175[13:11]` shadow averaging location = 2 each; `[3:0]` forward shadow = `4,4,5,8,B`; TDR_178 `0x0178[2:0]` pulse width = `2,2,2,2,6` | These are exact table entries, not freely interchangeable tuning profiles. Default `0x0173=0x0D04` differs from segment-1 recipe skip `0x07`; test the explicit configuration instead of treating reset as the recipe. |
| D/U, 8-58/59 | TDR_176 `0x0176[4:0]` location threshold reset 5; CDSCR4 `0x0177[12:8]` short-cable threshold reset `0x1E` | Numerical fields are documented, but physical scaling/legal calibration needs more evidence; do not invent metre units. |
| D/U, 8-61/66/71 | First peak: `0x0180[7:0]` location; `0x0185[6:0]` amplitude; `0x018A[11]` polarity and `[5]` cross indication | Raw masks `0x00FF`, `0x007F`, `0x0800`, `0x0020`. Additional named banks `0x0181..184/186..189` are reserved in detailed tables. No fabricated five-peak decoder or good/open/short conversion. |
| D, 8-47 | CDCR bit 14 `0x4000` resistor calibration normally stays low; if read high, clear it then immediately issue PHYRCR `0x001F[15]` hard reset | This is a contingency, not permission to start calibration as a routine diagnostic. Preserve its disruptive effects and restore owned configuration. |

## Interrupts, power, EEE and WoL

| Evidence / DS location | Concrete facts | Acceptance implications |
| --- | --- | --- |
| D/C, tables 8-35..37; Z-C 41-46 | PHYSCR `0x0011`: output enable bit 0, global enable bit 1, test interrupt bit 2. MISR1 `0x0012` and MISR2 `0x0013` have event bits `[15:8]` (RC) and matching enable bits `[7:0]` | Derived mask `0x00FF` for enables and `0xFF00` for event sample. Read-clear evidence is retained even during configuration or after a later callback fails. Test-interrupt stays active while bit 2 stays set. |
| D, table 8-36 | MISR1 events high-to-low: link quality, energy, link change, speed, duplex, AN complete, false-carrier half-full, receive-error half-full | Link enable `0x0020` corresponds to sampled event `0x2000`, not a live link-state value. |
| D, tables 8-37, 8-91 | MISR2 high-to-low: EEE error, AN error, page received, loopback FIFO OF/UF, MDI crossover, sleep, polarity/WoL, jabber. RXFS `0x04A1[12]` selects polarity=0/WoL=1 for the shared source | Do not emit both source labels without the selector evidence. WoL enable automatically selects WoL per the table. |
| D, 8.3.6; table 8-40 | Active-sleep policy PHYSCR `[14:12]=6`, mask/value `0x7000/0x6000`; BISCR bit 8 = 0 sleeping, 1 normal; sleep checks partner with NLP about every 1.4 s | Policy accepted is not asleep. 1.4 s is the documented behavior, not a deadline for external partner appearance. |
| D/C, 8.3.6.2; MCU-C 749-778 | BMCR bit 11 `0x0800` requests IEEE power-down; SMI and internal clocks remain. DPD enters by setting `0x0428[2]` first, then IEEE power-down by pin or BMCR | DPD also stops PLL. The entry order does not establish an arbitrary wake delay or prove a reverse-order exit. Pin ownership and clock availability matter. |
| D/X, 8.3.3.2 | EEE bypass-disabling controls are `0x04D1[0]=1` and `[3]=1` (combined `0x0009`); advertisement is printed MMD7 `0x203C[1]` | DS prints MMD3 `1001/1014/1016`, MMD7 `203C/203D`. Wire-offset translation remains unresolved in the inventory. Do not strip digits or execute this tuple without applicable evidence. |
| D, 8.3.4 | Legacy MAC sequence: enable controls/advertisement, restart AN with BMCR bit 9 `0x0200`, request TX LPI with `0x04D1[12]=1`, stop with bit 12=0 | No PHY frame buffering. Application must coordinate transmit silence/wake; control readback is not proof of partner wake or safe frame delivery. |
| D/U, 7.6 p15 | 100-Mbit EEE sleep **210 us**, quiet **20 ms**, wake Tw_sys_tx **36 us**, refresh **200 us** are in the **NOM** column | Do not present these as worst-case timeout/guaranteed readiness. Actual MAC/partner timing needs appropriate bounds and measurements. |
| D, table 8-90 | RXFCFG1 `0x04A0`: CRC gate bit 12; W0C level acknowledgement bit 11; pulse selector `[10:9]`; level/pulse bit 8; WoL enable bit 7; Secure-ON bit 5; Magic Packet bit 0 | Unrelated writes keep W0C neutral. Pulse codes 0-3 = 8/16/32/64 cycles at 125 MHz, derived 64/128/256/512 ns. Do not promote bit 6's bare bit-mask label into unsupported DP83826 custom pattern matching. |
| D, tables 8-91..97 | RXFS status bits 7/6/5/0 are SFD-error/bad-CRC/password-error/Magic-Packet RCH evidence. MAC destination words `0x04A2/3/4` hold DS byte pairs high:low = 4:5, 2:3, 0:1. Secure-ON `0x04A5/6/7` pairs = 1:0, 3:2, 5:4 | Freeze the application byte-order convention explicitly and use literal vectors from these DP83826 tables. Do not borrow a DP83822 app-note word order. |

The DS 8.3.3.1 EEE recipe is reproduced here as **printed address/value facts**
to make omissions detectable, **not** as executable MDIO wire tuples:

| Order | Printed hex address=value |
| --- | --- |
| 1-6 | `001F=8000`, `203C=0002`, `04D1=008B`, `04D3=4F12`, `04DF=0180`, `033E=A681` |
| 7-12 | `033F=0003`, `0123=0800`, `031B=8848`, `0466=FE00`, `04CF=261D`, `0416=1F30` |
| 13-18 | `04F5=2864`, `04E0=FFF2`, `031F=FE36`, `0308=0000`, `04F4=0800`, `0000=3300` |

The first hard reset invalidates owned settings. Several addresses are absent
from the detailed register catalogue, and the overlapping loopback/clock/profile
controls need reconciliation and restoration. Neither the selected MCU nor
Zephyr driver establishes this recipe, its reverse/disable procedure or the
unresolved EEE wire mapping. A missing recipe in a vendor driver does not remove
EEE from required library scope.

## LED, interface and link-quality controls

| Evidence / DS table | Facts and masks derived from fields | Constraints |
| --- | --- | --- |
| D/C/X, 8-42, 8-49, 8-85; MCU-H LED macros | LED0 source MLEDCR `0x0025[6:3]`, mask `0x0078`; LED1/2/3 at LEDCFG `0x0460[7:4]/[11:8]/[15:12]`, masks `0x00F0/0x0F00/0xF000` | MCU and RTOS source LED1/3 assignments disagree with DS. MLEDCR bit 0=1 routes source; bit 0=0 is reserved. Do not implement a disable control by clearing it. |
| D/C, 8-49/85 | Source codes 0..E: link, RX/TX, TX, RX, collision, 100M speed, 10M speed, full duplex, link/activity, stretch, 100M/full MII link, LPI, MII error, link lost, PRBS error | `0xF` reserved. Link-lost indication remains until BMSR read; PRBS indication remains until error counter clear. Collectors therefore affect visible LED behavior. |
| D/C, 8-42; MCU-C 73,80 | LEDCR `0x0018[10:9]` blink codes 0-3 = 50/100/200/500 ms; mask `0x0600`, shift 9 | Reject out-of-range codes before shifts; values from source are not measured board timing. |
| D/X, 8-74..77 | GPIO mux fields at `0x0303..0306[2:0]`; LED1 allowed table values 0 LED, 2 WoL, 4 IRQ, 5 TX_ER, 6 CLKOUT25M. LED2 0 LED, 2 WoL, 3/5/6 COL, 4 IRQ, 7 high | LED0 and LED3 mux fields are typed R despite selectable descriptions; LED1 clock-select prerequisite mentions mux=1 but that value is reserved in its mux table. Reconcile each pin, not a universal eight-value enum. |
| D/X, 8-89 | LEDCFG2 `0x0469`: LED1 drive-enable/value/polarity bits 0/1/2 and LED2 bits 4/5/6 | Bits 8/9/10 are reserved, so similarly named RTOS LED3 controls are not exact-part authority. |
| D/C, 8-41; Z-C 413-462 | RCSR `0x0017`: RMII enable bit 5; 50-MHz follower versus 25-MHz leader reference bit 7; follower-only TX shift bit 8; RMII revision bit 4 (0=1.2, 1=1.0) | Masks `0x0020`, `0x0080`, `0x0100`, `0x0010`. Check actual wiring/reference first; bit writes cannot create an external clock. |
| D/X, 8-41 | RCSR `[1:0]` elasticity: code 0/1/2/3 = 14/2/6/10 bits, described capacities 16800/2400/7200/12000 bytes at +/-50 ppm; at +/-100 ppm halve length | Non-monotonic encoding needs literal vectors. OF/UF bits 3/2 are RC but table says zero=fault, one=normal; do not assume active-high faults. |
| D, 8-73 | IO_CFG1 `0x0302[8]` selects CRS_DV=0/RX_DV=1 for repeater; bit 6 selects CLKOUT25=0/LED1=1 in ENHANCED mode | Repeater requires two PHYs and board wiring; configuration does not prove forwarding. |
| D/C, 8-30; MCU-C 70,349-374 | CR3 FLD cause mask `0x040F`: bit 0 energy, 1 low SNR, 2 MLT3 errors, 3 RX errors, 10 descrambler. Causes combine by OR | CR3 table cites 32 RX errors or 20 MLT3/SNR occurrences in 10 us. These are detector conditions, not software poll intervals. |
| D/U, 8-52/53 | FLD_CFG1 `0x0117[15:10]` window in 8 ns clocks, endpoint codes 1=2 cycles and 63=64 cycles, 0 reserved; `[9:4]` threshold 1..63 errors, 0 reserved. FLD_CFG2 `0x0131[5:0]` descrambler window, reset 4 | Derived masks `0xFC00`, `0x03F0`, `0x003F`. Do not equate the configurable window with the generic 10-us prose without resolving detector semantics. Descrambler field has no established physical scaling here. |
| D/U, 7.6 | FLD maximum 10 us for modes 1-4; descrambler mode 5 maximum 11 us; table qualifications/footnotes apply | Not an overall host observation bound: MDIO cadence, owner scheduling and transport time add latency. |
| X/U, 8-72 versus DBG 2.3.3 | DS MSE is `0x0218[15:0]`; DBG uses `0x0225` and an SNR formula | Keep address and scaling conflict open. No verified dB result or interchangeable alias from a bare raw register value. |

## Electrical and compliance anchors

| Evidence / DS or COMP location | Concrete facts | Verification requirement |
| --- | --- | --- |
| D/X, tables 8-79/80; Z-C 58-70,343-407 | VOD_CFG1 `0x030B[11:6]` negative MDI; negative MDIX split across `[13:12]` and VOD_CFG2 `0x030C[15:12]`; positive MDIX/MDI at `0x030C[11:6]/[5:0]` | Negative codes `0x28..0x38` map 150% down to 50%, step -6.25%; positive `0x08..0x18` map 50% up to 150%, step +6.25%; nominal codes `0x30/0x10`. Do not feed percentages directly into fields. |
| D, table 8-81 | VOD_CFG3 `0x030E`: MDI termination `[15:12]` enable bit 11; MDIX `[10:7]` enable bit 6. Codes 0..F list 122, 119, 116, 113, 110, 107, 105, 102, 100, 98, 96, 94, 92, 90, 88, 86 | Preserve documented 10BASE-Te baseline `0x4A40` and its source applicability; arbitrary independent controls must reconcile that baseline. Do not transplant COMP section 4's DP83825 analog-tuning recipe. |
| D/U, tables 8-73/83/86 | IO_CFG1 impedance `[15:14]` 0 slow/1 fast; IOCTRL `0x0461[4:0]` only `0x10` fast/`0x11` slow; ANA_RX10BT_CTRL `0x040D[4:0]` describes 200-575 mV in 25-mV steps | IOCTRL prose example `0x0400` conflicts with reserved fields. ANA_RX10BT_CTRL gives 16 physical levels for a 5-bit field without explicit code table; resolve mapping/range, do not assume all 32 values legal. |
| D/X, table 8-50 | COMPT `0x0027[3:0]`: 10-Mbit/s codes 0..D = NLP, single 1, single 0, repeated 1, repeated 0, preamble 10, single 1 + idle, single 0 + idle, repeated 1001, random, idle 00/01/10/11. Prose enables 10-Mbit/s patterns with COMPT bit 4 although the field summary calls it reserved | Feature-specific exception only. Reject undocumented E/F; force correct speed and use finite maintenance sessions/stop/restore. |
| D, table 8-50 | 100-Mbit/s pattern enable `0x0428[4]`; pattern selector is `{0x0428[5], COMPT[3:0]}`: 1..31 zeros after one; 0 clears shift register | Reconfiguration must clear enable then set it again. Restore DPD bit 2 and other owned state at the same procedure-only address; do not use a whole-word write that changes power. |
| C, COMP tables 5-1..5-5 | Test setup uses `001F=8000`, forced BMCR `2100` (100 Mbit/s) or `0100` (10 Mbit/s), PHYCR `0021` MDI or `4021` MDIX, restart `001F=4000`; 10-Mbit/s DP83825/26 adds BISCR `7101`, while DP83822 uses `7108`; harmonic stimulus uses COMPT `0013` | These fixture scripts use whole-word literals. DP83826 PHYCR `[4:0]` is strap-loaded read-only identity, not a programmable address change (DS table 8-43). Compose owned writable controls with the exact variant's loopback and restoration rules. Test patterns do not certify IEEE electrical compliance. |

The LED, odd-nibble, percent/code and loopback contradictions make independent
literal vectors essential. A test that calculates expected values with the same
helper as the implementation can reproduce the source defect. Preserve unknown
write effects and partial acquisition, verify reset/stop/restore, and keep
software bounds, real linked firmware and physical measurements as separate
evidence. No catalogue row closes an implementation or hardware gate.
