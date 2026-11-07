# Public library facts for DP83826E/A, DP83825I and DP83822

Initially inspected **2026-10-26**; G8/G9 corrections rechecked against archived
original PDFs **2026-11-07**, as planning evidence only. No driver, API, tests or
silicon qualification is implemented here. Register numbers and observed source
choices are facts to recheck against the exact archived datasheet, current source
and real device. They are neither initialization prescriptions nor proof that a
vendor library covers a whole part. Keep the complete register/field/procedure
ledger required by [FULL_COVERAGE](../prompts/FULL_COVERAGE.md); this catalogue is
a dense supplement to [public research](public-research.md), not its denominator.

Evidence labels below mean **OBSERVED** in pinned code, **CORROBORATED** by the
named exact-part document, **INFERRED** for an explicitly selected interpretation,
**CONFLICT** between identified sources, or **OPEN**
until the missing interpretation is established. A source conflict is not an
unsupported-silicon verdict. Reopen original tables when extraction is ambiguous,
correct the plan and dependent tests explicitly, and never invent a value or
silently select whichever recipe makes a test pass. Host traces prove software
behavior; actual clocks, analog output, optical link and restoration require
separate hardware evidence.

## Immutable source catalogue

All linked files were read through the connected GitHub source tool. Facts are
restated independently; GPL implementation is not copied into MIT source.

| Pin / license | Files and inspected line anchors | Useful scope and limit |
| --- | --- | --- |
| TI RTOS `94bf4343004732983b0b8e1eec123d88ff70453b`; per-file BSD-3-Clause | [dp83822.c](https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/rtos_drivers/src/dp83822.c#L52), [private header](https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/rtos_drivers/src/dp83822_priv.h#L58), [public header](https://github.com/TexasInstruments/ethernet-rtos-drivers/blob/94bf4343004732983b0b8e1eec123d88ff70453b/rtos_drivers/include/dp83822.h#L75) | Entire files: identity, MAC modes, MDIX, PHYSTS. Empty extended configuration and null PTP callbacks establish no complete-feature or timestamping support. |
| TI Linux `97becb13b81c314c1d17fcf2226aac9a32086f3d`; GPL-2.0 | [drivers/net/phy/dp83822.c](https://github.com/TexasInstruments/ti-linux-kernel/blob/97becb13b81c314c1d17fcf2226aac9a32086f3d/drivers/net/phy/dp83822.c#L17) | Entire file: IDs, IRQ, WoL, RMII/RGMII, fiber, LED/GPIO, clock output, amplitude and termination. Later sections also register driver callbacks; registration is not exact-subtype proof. |
| TI combined `450af1144b8caeb58724bcbae081b409f6bf5c3a`; Linux file GPL-2.0 | [linux_drivers/dp83822.c](https://github.com/TexasInstruments/ti-ethernet-software/blob/450af1144b8caeb58724bcbae081b409f6bf5c3a/linux_drivers/dp83822.c#L16) | Entire file: additional revision/A IDs and an older reset/delay path. Its model-matching macro masks revision bits despite separate revision constants. |
| Same TI combined pin; Zephyr file Apache-2.0, repository root GPL | [phy_ti_dp83826x.c](https://github.com/TexasInstruments/ti-ethernet-software/blob/450af1144b8caeb58724bcbae081b409f6bf5c3a/zephyr_drivers/dp83826a_driver/driver_files/drivers/ethernet/phy/phy_ti_dp83826x.c#L33) | Constants, indirect access, static configuration and reset/identity paths inspected. Its NC identity label conflicts with Linux; indirect selector is defective. Per-file terms still govern any actual reuse. |
| Upstream Zephyr `bc5d59d1b6657cfeef30124b818c8fbd69da68ac`; Apache-2.0 | [phy_ti_dp83825.c](https://github.com/zephyrproject-rtos/zephyr/blob/bc5d59d1b6657cfeef30124b818c8fbd69da68ac/drivers/ethernet/phy/phy_ti_dp83825.c#L27) | Entire file: RMII reference selection, IRQ, reset, AN-only configuration and monitoring. Infinite mutex waits, held locks during waits and framework work queues are not the portable contract. |

Existing [source-code.md](source-code.md) pins the TI original DP83826 sources
used for E comparison. A mutable branch or SDK `latest` page must be resolved to
an immutable version during execution; these snapshots do not claim current-head
identity or a tested SDK range. Original vendor bytes under `source/` stay intact.

PDF figures/tables were also visually checked at SNLS638C pages 8/42,
SNLS783A page 16, SNLS505H pages 11/35/70/83, and SNLA253 page 5. In particular,
nominal timing values must not be recast as maxima or minimum holdoffs.

## Identity and A-generation comparison

| Exact observation | Value | Evidence and disposition |
| --- | --- | --- |
| TI RTOS DP83822 OUI / model / revision equality check | `0x080028` / `0x24` / `0` | OBSERVED c:52-54, 129-143; CORROBORATED SNLS505H tables 8-3/8-4. Strict reference revision equality is a source policy, not a universal future-revision rule. |
| Linux DP83822 ID | `0x2000A240` | OBSERVED mirror:17; does not distinguish I/H/IF/HF, temperature or optical wiring. |
| Linux DP83825 labels I/S/CM/CS | `0x2000A150` / `0x2000A140` / `0x2000A160` / `0x2000A170` | OBSERVED mirror:18-21. **CONFLICT:** SNLS638C 7.3/7.4, table 7-6 gives exact DP83825I ID1 `0x2000`, ID2 `0xA140`, model `0x14`, revision `0`. Original PDF page 42 visually checked. Use the exact datasheet for source-backed admission/tests; retain the Linux-label discrepancy and qualify actual device IDs separately. An alternate accepted identity requires exact-part/revision authority. S/CM/CS names alone establish no public orderable part. |
| Linux original DP83826 C / NC labels | `0x2000A130` / `0x2000A110` | OBSERVED mirror:22-23; BASIC/ENHANCED model meaning must come from SNLS647I, not ordering suffix assumptions. |
| Combined Linux revision-B enhanced/basic labels | `0x2000A131` / `0x2000A111` | OBSERVED combined:22, 24. These are software labels, not a complete acceptance policy; its MDIO table uses mask `0xFFFFFFF0`. |
| Combined Linux A basic/enhanced labels | `0x2000A114` / `0x2000A134` | OBSERVED combined:25-26; CORROBORATED SNLS783A table 8-23: model `0x11` BASIC, `0x13` ENHANCED, default revision `4`. AI/AE suffix stays declared. Original generation default revision is `1`; no invented A-only model. |
| Combined TI Zephyr NC label | `0x2000A131` | CONFLICT: source:34 uses the enhanced revision-1 ID under NC, while both Linux sources use NC `0x2000A110`. Test rejection/mapping explicitly rather than copying the name. |
| A 10BASE-Te programming | `0x030E = 0x4A40` | CORROBORATED SNLS783A 8.3.13 and electrical note; retained original-generation value is not an invented A recipe. Determine reset/order/applicability from exact source. Defining VOD_CFG3 in Zephyr does not establish executing this write. |
| Local ANAR remote fault | bit 13, mask `0x2000` | SNLS647I table 8-23, SNLS783A table 8-24 and SNLS638C table 7-7 prohibit application setting despite R/W type. Supported partner/status observation is a separate path. Do not generalize the prohibition to DP83822 without its own source. |
| A FLD/EMC setting | CR3 owned mask `0x040F`; Strap11 HIGH if FLD is used | SDAA067A sections 2/3.1 give register/strap alternatives; 3.2 requires Strap11 HIGH for best EMC if FLD is used. Preserve CR3 pair/polarity bits 6:5 instead of replaying whole word `0x0001`; observe Strap11 through SOR1[11]. **CONFLICT:** SDAA067A table 3-3 option 5 says Strap8 HIGH while Figure 3-2 and SNLS783A table 8-5 say LOW; the datasheet governs. Programmable FLD is not register-only EMC equivalence; physical qualification remains open. |

I/E in SNLS647I and AI/AE in SNLS783A share their respective register/procedure
sets without suffix-conditioned software behavior; ordering differences are
temperature and TA-keyed electrical limits. Retain one cited map per generation,
parameterized admission tests and separate physical evidence. DP83822 I/H/IF/HF
likewise share one map, with temperature and fiber-capability attributes. Shared
IDs do not identify those attributes; DP83822 operating mode remains UNKNOWN.

A-generation differences are more than revision admission. SNLS783A tables
8-86/8-90 move LED1/2/3 to LEDCFG [3:0]/[7:4]/[11:8] (reset `0x5565`), reserve
[15:12], and define LEDCFG2 bits 8/9/10 for LED3 enable/value/polarity. Tables
8-75..8-78 make mux `0x0303[2:0]` and `0x0306[2:0]` R/W instead of original
R; `0x0304[2:0]` and `0x0305[2:0]` are RH/W with strap-dependent defaults.
Table 8-74 reserves IO_CFG1[6]; table 8-44 reserves PHYCR[5]. Table 8-88 adds
SOR1[11] on pin 19. Do not reuse original LED masks across generations.

Further A deltas: FLD_CFG2 table 8-54 reset `0x0288` versus `0x2284`, reserved
[15:6] `0x0A` versus `0x8A`, threshold 8 versus 4; CR1[5]/CR3[10] RH/W X
(BASIC 0, ENHANCED strap); PHYSCR[1:0] RH/W X (ENHANCED 0, BASIC 1, register
reset `010Xh`). RXFCFG1 table 8-91 reset `10X1h` replaces `108Xh`; bit 7 is
BASIC strap/ENHANCED 1, bit 0 reset 1 with strap wording. FLDS table 8-34 is
RCH instead of RC but remains consuming. FLD tables 8-5/8-6 depend on Strap11;
section 8.3.17 requires MLT3, low SNR and descrambler with energy loss or RX error.

SNLS783A 8.3.3.1 has 27 ordered EEE rows versus original 18: drop
`04D3=4F12`, change `0416` `1F30` to `1830` and `0466` `FE00` to `FD00`; add
`0126=5D23`, `0132=0183`, `04D4=3322`, `0310=76E1`, `0311=01FC`, `0313=06E3`,
`033A=579B`, `033C=EC00`, `0317=1000` and a second `04F5=2864`. `0466` is last,
after `0000=3300`; duplicates and ordering cannot be collapsed into a map.
These are inputs for prompts 57-59, not an implemented A support claim.

## TI RTOS DP83822 concrete behavior

| Register / mask | Observed interpretation | Source and independent verification needed |
| --- | --- | --- |
| CR1 `0x09`, `0x0020` | Robust Auto-MDIX enable | private:58-68; c:214-219. Check current SNLS505H exact mode applicability. |
| PHYCR `0x19`, `0x8000` / `0x4000` | Auto-MDIX enable / forced crossover; forced MDI is zero | private:61-74; c:191-226 enables Auto-MDIX and robust mode except near-end loopback, where it disables auto and forces MDI. It does not clear robust mode in that branch. Verify required pre-state and restoration; no hidden link-policy change. |
| PHYSTS `0x10`, bits 0/1/2 | Link / 10-Mbit flag / full duplex; speed bit clear means 100 Mbit/s | private:64-83; c:264-318. Literal linked rows: `0x0001` 100/half, `0x0005` 100/full, `0x0003` 10/half, `0x0007` 10/full. No-link is separate from read failure and is not a transport-health fault. |
| MAC-mode predicate | MII, RMII and RGMII admitted | c:146-167; public header calls it an RMII PHY while implementation permits all three. Configuration c:170-201 only adjusts MDIX; the predicate is not evidence that it programs each interface. |
| Reset and extended access | Generic callbacks | c:85-88; audit actual generic implementation and exact reset domains. No dedicated DP83822 timing guarantee follows from a callback name. |

The MDIX helper returns `void` and discards read-modify-write callback statuses
(c:203-226); configuration can therefore report success after transport failure.
Use independent failure injection, preserve backend detail and configuration
uncertainty. Its register dump also ignores read results and consumes status.
Do not reproduce that pattern or allow a dump to bypass the single collector.

## Interrupt and wake facts from TI Linux

OBSERVED at mirror:25-111, 212-427; exact per-part applicability and access
semantics still require SNLS647I/SNLS783A/SNLS638C/SNLS505H. Common labels do not
authorize all registers on every part.

| Address / mask | Observed meaning |
| --- | --- |
| PHYSCR `0x11`, bits 0/1 | Interrupt output enable `0x0001`, global enable `0x0002` |
| MISR1 `0x12`, enables bits 0..7 | RX-error half-full `01`, false-carrier half-full `02`, AN complete `04`, duplex change `08`, speed change `10`, link change `20`, energy detect `40`, link quality `80` (hex) |
| MISR2 `0x13`, enables bits 0..7 | Jabber `01`, WoL packet `02`, sleep `04`, MDI crossover `08`, loopback FIFO `10`, next page `20`, AN error `40`, EEE error change `80` (hex) |
| MISR1/MISR2 status | Upper byte 15:8 corresponds to lower-byte enable 7:0; Linux tests enabled-pending overlap. Reading can clear evidence: retain MISR1 samples if MISR2 fails. Never use a raw RMW/dump as an independent collector. |
| RXFCFG/WoL configuration `0x04A0` | Magic bit 0 `0x0001`, Secure-ON bit 5 `0x0020`, enable bit 7 `0x0080`, level indication bit 8 `0x0100`, clear-indication command bit 11 `0x0800` |
| RXFS `0x04A1`; destination `0x04A2..0x04A4`; SecureON `0x04A5..0x04A7` | Code uses vendor MMD 31. The application MAC is wire octet order: `02:12:34:56:78:9A` packs as `0x1202,0x5634,0x9A78`; `11:22:33:44:55:66` packs as `0x2211,0x4433,0x6655`. SNLA261A sections 2.2.1/.2 and the exact RXFPMD/RXFSP tables corroborate the words despite differing Byte 5/MSB labels. Password `2A:2B:2C:2D:2E:2F` similarly gives `0x2B2A,0x2D2C,0x2F2E`. Command acknowledgement is separately part-specific, not implied by packing. |

Linux configures the destination first, optionally password, reads MISR2, then
commits configuration/enable/clear. Several intermediate writes/read results are
unchecked; getters and suspend/resume also have unchecked failures. This is a
source-observed ordering lead, not an approved transaction or error contract.
The newer mirror saves desired WoL state for configuration restoration, while
the older combined `config_init` disables WoL. Test desired-versus-effective
state, partial writes and reset/restore rather than inheriting either policy.

Linux exposes only Magic/Secure-ON. **DP83822 additionally documents pattern
matching**: SNLS505H tables 8-87..8-131 define RXFCFG bit 1, 64 bytes in
`0x04A8..0x04C7`, four byte masks `0x04C8..0x04CB` (one means ignore byte),
and start offset `0x04CC[5:0]`, default `12`, beginning at the thirteenth byte
after SFD. RXFCFG bit 6 is only named Bit Mask Flag in table 8-87: semantics
are OPEN, preserve it and expose no typed setter. RXFCFG CRC gate bit 12
defaults one. DP83822 bit 11 is W1S: acknowledgement writes owned config OR
`0x0800`; unrelated writes keep it zero (SNLA261A table 2-4 step 6). In contrast,
SNLS647I table 8-90 and SNLS783A table 8-91 define W0C, neutral one. RXFS[12]
resets zero on DP83822 (table 8-88), versus one on DP83826; enabling WoL
automatically selects WoL for MISR2[9]. Pulse selector bits 10:9 encode
8/16/32/64 cycles of 125 MHz, i.e. 64/128/256/512 ns. No DP83826 custom-pattern
support follows from this family map; see SNLA261A table 2-1.

DP83825 Register_12 `0x000C` is consuming. MISR1/2 and FLDS must also be
collector-owned despite R markings where SNLS638C prose describes read-clear
or latching (tables 7-19/7-21 and FLDS definition). Exclude all four from
ordinary reads; acquire IRQ events through MISR1/2 only until the `0x000C`
latch relationship is established. Retain a consumed first sample if the next
read fails. SecureON addresses `0x04A5..0x04A7` are absent from SNLS638C's
summary map but SNLA261A table 2-1 and 2.2.2 explicitly include DP83825: an
exact-address allowlist is source-supported, with physical Q15 still NOT RUN.

## Interface, clock, analog and LED facts

| Part / source | Concrete fields or values | Disposition / acceptance seed |
| --- | --- | --- |
| DP83825I RCSR `0x17`, SNLS638C table 7-26 | bit 7 `0x0080`: 0=25 MHz crystal/CMOS, 1=50 MHz CMOS; bit 8 TX shift follower-only; bit 4 RMII revision; bits 3/2 RC FIFO status; bits 1:0 elasticity | Role is board-declared LEADER/FOLLOWER and strap-determined, not an independent clock selector. Observe SOR1[3] directly and RCSR[7] through the collector; mismatch makes no role write. Table 7-26 marks bit 7 R/W/reset zero, but its description and SDAA081 2.4 use the strap as authority; retain the conflict and strap-parameterized vectors. Header reset `0x0061` also conflicts with reserved bits 6/5 reset zero: preserve reserved readback. Zephyr writes remain OBSERVED only. |
| DP83822 RCSR `0x17`, SNLS505H table 8-23 | bit 12 RX shift 1=enabled; bit 11 TX shift 0=enabled; both fixed 3.5 ns. bit 10 shared-clock synchronization; bit 9 RGMII; bit 8 RMII TX shift; bit 7 25/50 MHz selection; bit 6 recovered-clock async FIFO (0=bypass); bit 5 RMII; bit 4 RMII revision; bits 3/2 RC FIFO status; bits 1:0 elasticity | CORROBORATED; RX/TX shift defaults zero mean RX off, TX on. Independent combinations `(off,off)=0x0800`, `(on,off)=0x1800`, `(off,on)=0x0000`, `(on,on)=0x1000` before other fields. Test changing either direction from both prior states. |
| DP83822 RMII mode transition | Software RCSR[5] 0-to-1: literal `0x0462=0x4300`, then `0x001F=0x4000`; strap-selected RMII has no such recipe | SNLS505H table 8-23/7.4.1.2; the word selects RX_D3 50 MHz output and LED_1 tri-state. Read `0x0462` first and reject conflicting owners of [14:12]/[10:8]. Reapplying nonzero [6:4]/[2:0] after restart is INFERRED, physical NOT RUN. OPEN: the literal selects Leader reference even in Follower. Leaving RMII returns [10:8] to zero then restarts, INFERRED from table 5-1 data-pin requirements; no reverse recipe is documented. |
| DP83822 Linux RCSR access | Mirror:563-575 and 647-651 use vendor MMD 31 address `0x17` | CORROBORATED by SNLS505H 7.4.2.1, page 35: standard registers 0..31 permit direct or indirect access; REGCR `0x0D` and ADDAR `0x0E` are direct-only and ignore indirect access. DEVAD 31 is the general vendor namespace. Keep whole indirect-transaction ownership; actual hardware traces remain NOT RUN. Do not infer another part's aliasing from this DP83822 statement. |
| DP83822 GPIO clock selection | `0x0462[14:12]` GPIO3, `[6:4]` GPIO1; `0x0463[6:4]` GPIO2 | SNLS505H tables 8-81/82: 0=MAC IF, 1=XI, 2=25 MHz internal, 3 reserved, 4=50 MHz RMII Leader, 5 reserved, 6=125 MHz free-running, 7=125 MHz recovered. GPIO1 clock is active with `0x0462[2:0]=011`; conflict with FX SD on pin 24 is INFERRED. Linux mirror:135-145, 825-845 covers only a GPIO2 subset; validate exact mux/interface ownership. |
| DP83822 LED mux/functions | MLED `0x0025[6:3]` and routing `[1:0]`; LED1 `0x0460[11:8]`, LED3 `[7:4]`; GPIO1 `0x0462[2:0]`, GPIO3 `[10:8]`, GPIO2 `0x0463[2:0]` | OBSERVED mirror:113-161, 474-514. Function codes 0..14: link, RX/TX, TX, RX, collision, 100-link, 10-link, full-duplex, link/activity, stretch, MII-link, LPI, RX/TX-error, link-lost, PRBS-error. Recheck each exact mux value/table rather than accepting reserved 15. |
| Linux mux admission | MLED routes to LED0 or COL/GPIO2, not both; GPIO2 LED conflicts with clock; RX_D3/GPIO3 LED admitted only in RMII | OBSERVED mirror:781-800; exact pin/interface applicability requires datasheet verification. These are useful independent zero-transfer negative cases. |
| DP83822 TX amplitude | LDCTRL `0x0403[7:4]`, reset code 12; codes 0..15: `1600,1633,1667,1700,1733,1767,1800,1833,1867,1900,1933,1967,2000,2033,2067,2100` mVpp | CORROBORATED SNLS505H table 8-68. Linux's integer percentages `80,82,83,85,87,88,90,92,93,95,97,98,100,102,103,105` are rounded properties, not exact voltage definitions. Preserve exact code/value semantics. |
| DP83822 fiber/10BASE-Te swing | `0x0403[11:8]` FX, `[3:0]` 10BASE-Te | Table 8-68 gives FX codes 0..15 `533,567,600,633,667,700,733,767,800,833,867,900,933,976,1000,1033` mVpp; default 15. Code 13's documented 976 differs from uniform-step expectation: do not silently substitute 967. 10BASE-Te codes 0..15 `3200,3233,3267,3300,3333,3367,3400,3433,3467,3500,3533,3567,3600,3633,3667,3700` mVpp, default 15. Treat measurement tolerance separately. |
| DP83822 MAC termination | IOCTRL `0x0461[4:1]`, reset code 8 | Table 8-80 exact ohms by code 0..15: `99.25,91.13,84.24,78.31,73.17,68.65,64.66,61.11,58.07,55.18,52.57,50.20,48.03,46.04,44.20,42.51`. Linux mirror:747-749 uses rounded `99,91,84,78,73,69,65,61,58,55,53,50,48,46,44,43`. Do not label these rounded integers exact silicon impedances. |

The older combined Linux delay setter only sets requested bits; newer mirror
uses a mask covering both shift controls. Independent prior-state tests must
catch a stale enabled RX shift or disabled TX shift. Linux's `phy_get_internal_delay`
positive/zero policy does not create arbitrary RGMII RCSR delay taps. Separate
MII TXCPSR `0x0042` exists (table 8-36): bit 4 is W1S enable, [3:0] phase value
with 4 ns difference steps; the description's [4:0] wording conflicts with those
field boundaries and remains explicit.

## Fiber, reset, diagnostic and power procedures

| Scope | Facts and source | Required interpretation / test |
| --- | --- | --- |
| DP83822 fiber | Section 4 identifies IF/HF FX parts; 7.4.1.1/.2 establishes MII/RMII and 7.4.6 excludes FX AN. CR2[14] enables FX; `FIBER_GENCFG 0x0465[0]` sets SD active-low polarity only, distinct from `GENCFG 0x0456`. | SD is strap-only: SOR1[11:10] must be 01 or 10 AND [9:8] must be 10 or 11 (table 7-10); CR2[14] alone does not activate it. RX_ER keeps AMDIX_EN meaning without FX_EN strap. FX+RGMII is not stated or excluded; table 7-10 RX_ER mode 3 can encode it, physical NOT RUN. Linux mirror:582-583/combined:424-425 incorrectly use set value 1 for bit 14; use literal mask `0x4000`. FX writes no copper CR1[6:5], PHYCR[15:14] or CR3[6:5], and runs no copper TDR/10BASE-Te recipe. |
| DP83822 reset mechanisms | POR/RESET_N reset all and latch straps; `0x001F=0x8000` is pin-equivalent; `0x001F=0x4000` digital restart retains registers; BMCR[15] resets PCS while retaining vendor registers and not relatching; printed MMD3 `0x3000[15]` resets MMD3/MMD7/PCS, retaining vendor registers. | Tables 8-1/8-30, section 7.5.1 and SNLA437A 2.2.6: treat PHYRCR[15] as relatching despite table 8-1 saying only POR/pin latch. BMCR PCS membership in `0x0000..0x001F` is unspecified: reread owned fields. Prose software reset often means literal `0x4000`; section 7.4.4.2 soft reset is interpreted the same (INFERRED). Linux labels differ; names never select a universal reset recipe. |
| DP83825I timing | SNLS638C 5.6: hardware reset pulse minimum 25 us; reset-to-SMI maximum 2 ms; POR-to-SMI maximum 50 ms; reset-to-FLP nominal 1500 ms; reset-to-RMII-leader-clock nominal 0.2 ms. | Zephyr:337 uses 1 ms pulse and 343/353 waits 50 ms after GPIO/BMCR reset. OBSERVED conservative framework policy is not a silicon minimum. Do not hold controller lock during waits or use 50 ms as a universal operation timeout. Nominal FLP timing is not a guaranteed maximum. |
| DP83826A timing | SNLS783A 7.6: reset pulse minimum 25 us; reset-to-SMI maximum 2 ms; POR-to-SMI maximum 50 ms; reset-to-FLP nominal 1500 ms; leader-clock nominal 0.2 ms. | Recheck original PDF columns and selected mode before encoding min/max; a GPIO reset specification is not automatically a BMCR/PHYRCR deadline. Its 100M EEE nominal timings are sleep 210 us, quiet 20 ms, system TX wake 36 us and refresh 200 us; these are not software operation deadlines. |
| DP83822 timing | SNLS505H 6.6-6.8: power-up register readiness wait up to 200 ms; hardware reset pulse minimum 10 us, XI stable at least 1 us during low; post-reset SMI maximum 2 ms; reset configuration latch typical 120 ns, output transition typical 64 ns, reset-to-FLP typical 1.5 s. | Original PDF page 11 columns independently rechecked: table 6.8 T3/T4/T5 are TYP, not MAX. Separate readiness, reset completion and link acquisition. These nominal values establish no guaranteed FLP deadline; do not import 25/26 timing or turn missing FLPs into a transport-health fault. |
| DP83822 TDR | SNLA253 2.3: `0x0171[3:0]=0xC`, `0x0173[15:8]=0xFF`, `0x0177[12:8]=0x18`; auto `0x0009[8]` or manual `0x001E[15]`; results `0x0180..0x018A`. | Preserve other fields. Done resets one: require start bit 15 clear AND done bit 1 set; fail is bit 0. Tables 8-55..8-59 give eight-bit locations, tables 8-60..8-64 seven-bit amplitudes (`0x7F`; bits 15/7 reserved), overriding SNLA253 eight-bit wording. Keep exact TD/RD peak placement, polarity `018A[15:6]` (0 positive/1 negative), excess flags [3:2], invalid samples and inconclusive outcomes. |
| DP83822 TDR conversion | SNLA253 2.3.2: intermediate `D*0.8621-8`; meters `intermediate+(70-intermediate)*0.01`; D=`0x33` gives about 36.31 m. Its amplitude text divides an eight-bit code by 256, conflicting with seven-bit detailed tables. | Mask raw amplitude to `0x7F` per exact tables; do not invent a replacement physical amplitude scale from a convenient width. Keep the conversion source conflict and raw result distinct. Cable conditions, invalid-sample behavior, calibration and accuracy require exact physical evidence; no transfer to DP83825/26. |
| DP83822 deep power-down | SNLS505H table 8-76: DPDWN `0x0428[2]` enable, bit 3 exit-speed 1=50 ms, 0=100 ms; requires prior IEEE power-down BMCR `0x0000[11]` or INT/PWDN_N. | Complete entry/wake/restore with admission and finite deadlines. Same register bits 5/4 also participate in TX idle-pattern test with `0x0027[3:0]`; exact field preservation prevents an accidental test mode. Linux generic suspend is not complete deep-power support. |
| DP83822 SFD indication | SNLS505H 7.3.3: after IOCTRLx RX/TX SFD mux, literal `0x0456=0x000A`, optionally enable WoL, then `0x001F=0x4000`. | Exact `0x000A` is an exceptional allowlist because table 8-78 reserves bit 1; bit 3 remains one, so reject an owner requiring zero. WoL is an optional accuracy aid: compose RXFCFG1[7] with owned fields, preserve [12], use neutral [11]=0. Enabling WoL sets RXFS[12], making MISR2[9] WoL; default selector is zero. PTPPSEL `0x003E`/PTPCFG `0x003F` also belong to prompt 67. |

DP83822 FX link observation needs a separate contract. SNLA437A 2.3.4 says
BMSR[2] does not toggle with FX link changes and requires literal
`0x001F=0x4000` before rereading. PHYSTS[0] not being change-tracked is INFERRED
from table 8-16; table 8-2 has no fiber exception and table 8-83 says SD drops
link, so preserve that source gap. Explicit maintenance-admitted refresh may
restart, wait under an application budget (no duration documented), then read
BMSR twice through the collector, retaining the first and using the second.
Never reset from polling, cached reads or an SDK timer. Without active SD,
section 7.4.4.2 says electrical-link-loss recovery needs soft reset, interpreted
as that digital restart. Physical Q14 remains NOT RUN. Include far-end fault
`0x0040[13,6,5]` from table 8-35 with reserved-preserving writes and FX swing
LDCTRL[11:8]; copper/fiber is a declared medium with observed CR2/SOR1 evidence.

DP83822 standard aliases exclude REGCR/ADDAR themselves. Preserve a second
source conflict: 7.4.2.1 calls REGCR function 10 invalid, while table 8-13 and
7.4.2.6/.7 use it. Select no-post-increment 01 only until qualified. Section
7.4.2.9 explicitly maps printed `0x703D` to DEVAD 7/offset `0x003D`: literal
writes `0x0D=0x0007`, `0x0E=0x003D`, `0x0D=0x4007`, then read `0x0E`.
Printed `0xDnnn` uses DEVAD D/offset `0x0nnn` here; DP83826's printed scheme
must not inherit that conclusion by analogy.

DP83825 whole-word functional modes come from SNLS638C tables 6-5..6-7.
Its `0x04D1` bypass uses sections 6.3.3.2/6.3.4 and table 6-7 `0x00C2`
(bits 0/3 clear), against table 7-92's R marking and reversed bit 3 wording.
Recipe addresses `0x030E,0x0333,0x0218,0x031B,0x033A,0x033D` have exact
allowlist evidence in those tables, table 8-3 and SDAA081 2.3.3 despite summary
reserved wording; no generic writes or DP83826 `0x030E=0x4A40` recipe follow.
For declared Leader, mux `0x0305[2:0] != 0`, BMCR isolation and DPDWN[2]
require explicit clock-loss opt-in before mutation; a later SOR1 role mismatch
blocks them. Isolation follows table 7-3, while mux/deep-power clock loss is
INFERRED from table 7-61/6.3.8, physical Q15. SNLS638C T4 specifies 20 MHz MDC
maximum versus 24 MHz prose: select the conservative 20 MHz backend limit.

## Source dispositions and independent acceptance

Keep identity `A140/A150`, DP83825 RCSR reset/reserved values, FX enable,
reset domains and Strap11/CR3 EMC differences as named source records. Use the
documented exact-part interpretation where it is established, including the
DP83825I datasheet identity policy and corrected FX bit composition. Keep
essential authoritative conflicts and physical qualification separately open.
Record later applicable evidence and update affected contracts/tests together.

In the TI A Zephyr indirect helpers, source:151/180 combines data mode with
`0x4000 & devad`, yielding zero for DEVAD 31. Required C22 indirect phases must
be independently checked against exact datasheets; address `0x001F` and data
`0x401F` selector literals are a negative-test seed. A test generated from the
same mistaken helper does not validate it. Linux writes or generic helpers that
ignore errors likewise cannot define successful completion.

For prompts 56..67, verify exact parts and all legal settings; test invalid enum,
reserved code, incompatible medium/interface/pin assignment and prohibited ANAR
before I/O. Separately test bounded identity reads with no writes after mismatch,
mixed RC/command registers, partial effect followed by error, reset/restore,
aggregate deadline and separate finite restoration failure. Source-derived literal traces must be independent
of production constants. New substantive evidence can require correcting this
plan; do so before implementing guessed behavior. Missing hardware is a visible
qualification gate while eligible documented software work continues.
