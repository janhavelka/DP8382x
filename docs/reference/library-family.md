# Public library facts for DP83826E/A, DP83825I and DP83822

Inspected **2026-10-26**, as planning evidence only. No driver, API, tests or
silicon qualification is implemented here. Register numbers and observed source
choices are facts to recheck against the exact archived datasheet, current source
and real device. They are neither initialization prescriptions nor proof that a
vendor library covers a whole part. Keep the complete register/field/procedure
ledger required by [FULL_COVERAGE](../prompts/FULL_COVERAGE.md); this catalogue is
a dense supplement to [public research](public-research.md), not its denominator.

Evidence labels below mean **OBSERVED** in pinned code, **CORROBORATED** by the
named exact-part document, **CONFLICT** between identified sources, or **OPEN**
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
| Linux DP83825 labels I/S/CM/CS | `0x2000A150` / `0x2000A140` / `0x2000A160` / `0x2000A170` | OBSERVED mirror:18-21. **CONFLICT:** SNLS638C 7.3/7.4, table 7-6 gives exact DP83825I ID1 `0x2000`, ID2 `0xA140`, model `0x14`, revision `0`. Original PDF page 42 visually checked. Do not accept either mapping without resolving exact silicon/revision evidence. S/CM/CS names alone establish no public orderable part. |
| Linux original DP83826 C / NC labels | `0x2000A130` / `0x2000A110` | OBSERVED mirror:22-23; BASIC/ENHANCED model meaning must come from SNLS647I, not ordering suffix assumptions. |
| Combined Linux revision-B enhanced/basic labels | `0x2000A131` / `0x2000A111` | OBSERVED combined:22, 24. These are software labels, not a complete acceptance policy; its MDIO table uses mask `0xFFFFFFF0`. |
| Combined Linux A basic/enhanced labels | `0x2000A114` / `0x2000A134` | OBSERVED combined:25-26; CORROBORATED SNLS783A table 8-23: model `0x11` BASIC, `0x13` ENHANCED, default revision `4`. AI/AE suffix stays declared. Original generation default revision is `1`; no invented A-only model. |
| Combined TI Zephyr NC label | `0x2000A131` | CONFLICT: source:34 uses the enhanced revision-1 ID under NC, while both Linux sources use NC `0x2000A110`. Test rejection/mapping explicitly rather than copying the name. |
| A 10BASE-Te programming | `0x030E = 0x4A40` | CORROBORATED SNLS783A 8.3.13 and electrical note; retained original-generation value is not an invented A recipe. Determine reset/order/applicability from exact source. Defining VOD_CFG3 in Zephyr does not establish executing this write. |
| Local ANAR remote fault | bit 13, mask `0x2000` | SNLS647I table 8-23, SNLS783A table 8-24 and SNLS638C table 7-7 prohibit application setting despite R/W type. Supported partner/status observation is a separate path. Do not generalize the prohibition to DP83822 without its own source. |
| A FLD/EMC setting | CR3 `0x000B = 0x0001`; Strap11 HIGH | SDAA067A 2 and 3.1 describe register or strap alternatives for signal/energy-loss FLD; 3.2 says Strap11 MUST be HIGH for best EMC. **CONFLICT:** programmable FLD selection does not establish register-only EMC equivalence. Keep physical guidance unresolved until authoritative clarification/measurement. |

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
| RXFS `0x04A1`; destination `0x04A2..0x04A4`; Secure-ON `0x04A5..0x04A7` | Code uses vendor MMD 31. Destination and password pair application-array byte 0 in low byte, byte 1 high; example `02:12:34:56:78:9A` encodes `0x1202,0x5634,0x9A78`. This is library-observed packing, not an independent datasheet vector. Define array indices versus TI's Byte 5/MSB numbering, then verify exact-part words. Treat command-neutral bit 11 and output-on-error independently from byte packing. |

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
after SFD. RXFCFG bit 6 selects bit-mask behavior; table 8-87 must determine its
exact interaction rather than guessing. RXFCFG CRC gate bit 12 defaults one;
bit 11 is W1S, not retained configuration. Pulse selector bits 10:9 encode
8/16/32/64 cycles of 125 MHz, i.e. 64/128/256/512 ns. No DP83826 custom-pattern
support follows from this family map; see SNLA261A table 2-1.

## Interface, clock, analog and LED facts

| Part / source | Concrete fields or values | Disposition / acceptance seed |
| --- | --- | --- |
| DP83825I RCSR `0x17`, SNLS638C table 7-26 | bit 7 `0x0080`: 0=25 MHz crystal/CMOS, 1=50 MHz CMOS; bit 8 `0x0100` TX shift applies only to follower; bit 4 RMII 1.0 versus 1.2; bits 3/2 FIFO overflow/underflow RC; bits 1:0 elasticity | CORROBORATED clock polarity against Zephyr:257-272. RMII-only. Table heading reset `0x0061` conflicts with bits 6/5 listed reserved reset zero: OPEN; preserve reserved fields pending exact clarification rather than normalizing the header. |
| DP83822 RCSR `0x17`, SNLS505H table 8-23 | bit 12 RX shift 1=enabled; bit 11 TX shift 0=enabled; both fixed 3.5 ns. bit 10 shared-clock synchronization; bit 9 RGMII; bit 8 RMII TX shift; bit 7 25/50 MHz selection; bit 6 recovered-clock async FIFO (0=bypass); bit 5 RMII; bit 4 RMII revision; bits 3/2 RC FIFO status; bits 1:0 elasticity | CORROBORATED; RX/TX shift defaults zero mean RX off, TX on. Independent combinations `(off,off)=0x0800`, `(on,off)=0x1800`, `(off,on)=0x0000`, `(on,on)=0x1000` before other fields. Test changing either direction from both prior states. |
| DP83822 RMII mode transition | After changing RCSR bit 5 to one: write `0x0462=0x4300`, then `0x001F=0x4000` | CORROBORATED table 8-23 note, original PDF page 70 visually checked. Implement the complete sequence with exact reset/owned-state effects and finite restoration, not merely a mode-bit setter. |
| DP83822 Linux RCSR access | Mirror:563-575 and 647-651 use vendor MMD 31 address `0x17` | CORROBORATED by SNLS505H 7.4.2.1, page 35: standard registers 0..31 permit direct or indirect access; REGCR `0x0D` and ADDAR `0x0E` are direct-only and ignore indirect access. DEVAD 31 is the general vendor namespace. Keep whole indirect-transaction ownership; actual hardware traces remain NOT RUN. Do not infer another part's aliasing from this DP83822 statement. |
| DP83822 GPIO clock selection | `0x0462[14:12]` GPIO3; `0x0463[6:4]` GPIO2 | SNLS505H tables 8-81/82: selector 0=MAC IF, 1=XI, 2=25 MHz internal, 3 reserved, 4=50 MHz RMII leader reference, 5 reserved, 6=125 MHz free-running, 7=125 MHz recovered. Linux mirror:135-145, 825-845 implements GPIO2 subset. Validate pin mux/interface conflicts before I/O. |
| DP83822 LED mux/functions | MLED `0x0025[6:3]` and routing `[1:0]`; LED1 `0x0460[11:8]`, LED3 `[7:4]`; GPIO1 `0x0462[2:0]`, GPIO3 `[10:8]`, GPIO2 `0x0463[2:0]` | OBSERVED mirror:113-161, 474-514. Function codes 0..14: link, RX/TX, TX, RX, collision, 100-link, 10-link, full-duplex, link/activity, stretch, MII-link, LPI, RX/TX-error, link-lost, PRBS-error. Recheck each exact mux value/table rather than accepting reserved 15. |
| Linux mux admission | MLED routes to LED0 or COL/GPIO2, not both; GPIO2 LED conflicts with clock; RX_D3/GPIO3 LED admitted only in RMII | OBSERVED mirror:781-800; exact pin/interface applicability requires datasheet verification. These are useful independent zero-transfer negative cases. |
| DP83822 TX amplitude | LDCTRL `0x0403[7:4]`, reset code 12; codes 0..15: `1600,1633,1667,1700,1733,1767,1800,1833,1867,1900,1933,1967,2000,2033,2067,2100` mVpp | CORROBORATED SNLS505H table 8-68. Linux's integer percentages `80,82,83,85,87,88,90,92,93,95,97,98,100,102,103,105` are rounded properties, not exact voltage definitions. Preserve exact code/value semantics. |
| DP83822 fiber/10BASE-Te swing | `0x0403[11:8]` FX, `[3:0]` 10BASE-Te | Table 8-68 gives FX codes 0..15 `533,567,600,633,667,700,733,767,800,833,867,900,933,976,1000,1033` mVpp; default 15. Code 13's documented 976 differs from uniform-step expectation: do not silently substitute 967. 10BASE-Te codes 0..15 `3200,3233,3267,3300,3333,3367,3400,3433,3467,3500,3533,3567,3600,3633,3667,3700` mVpp, default 15. Treat measurement tolerance separately. |
| DP83822 MAC termination | IOCTRL `0x0461[4:1]`, reset code 8 | Table 8-80 exact ohms by code 0..15: `99.25,91.13,84.24,78.31,73.17,68.65,64.66,61.11,58.07,55.18,52.57,50.20,48.03,46.04,44.20,42.51`. Linux mirror:747-749 uses rounded `99,91,84,78,73,69,65,61,58,55,53,50,48,46,44,43`. Do not label these rounded integers exact silicon impedances. |

The older combined Linux delay setter only sets requested bits; newer mirror
uses a mask covering both shift controls. Independent prior-state tests must
catch a stale enabled RX shift or disabled TX shift. Linux's `phy_get_internal_delay`
positive/zero policy does not create a programmable delay range on this PHY.

## Fiber, reset, diagnostic and power procedures

| Scope | Facts and source | Required interpretation / test |
| --- | --- | --- |
| DP83822 fiber | SNLS505H 7.4.4/7.4.6: IF/HF 100BASE-FX; independent MII/RMII/RGMII; no AN. CR2 `0x000A[14]` FX enable; GENCFG `0x0465[0]` signal-detect low polarity. Linux SOR1 `0x0467[11:10]` modes 1/2 enable FX, `[9:8]` modes 2/3 enable signal detection (zero-based codes). | OBSERVED strap decode mirror:926-950; exact wiring/straps remain required. **CONFLICT:** mirror:582-583 and combined:424-425 pass clear mask `0x4000` but set value `1` to `phy_modify`; do not copy this as a valid FX-enable write. Check helper semantics and silicon, use independent literal bit 14 vectors. Copper-only MDIX/TDR/10BASE-Te recipes must never leak into FX. |
| Reset control `0x001F` | New mirror:52-53 calls bit 15 software reset, bit 14 digital restart; older combined:47-48 calls bit 15 hardware reset and bit 14 software reset; combined reset:490 uses bit 14, new mirror:734 uses bit 15. | CONFLICT in labels/procedure semantics. Exact datasheet reset domains and accepted-write uncertainty govern. Do not turn either name into a universal operation, or assume immediate config restoration is safe without bounded completion. |
| DP83825I timing | SNLS638C 5.6: hardware reset pulse minimum 25 us; reset-to-SMI maximum 2 ms; POR-to-SMI maximum 50 ms; reset-to-FLP nominal 1500 ms; reset-to-RMII-leader-clock nominal 0.2 ms. | Zephyr:337 uses 1 ms pulse and 343/353 waits 50 ms after GPIO/BMCR reset. OBSERVED conservative framework policy is not a silicon minimum. Do not hold controller lock during waits or use 50 ms as a universal operation timeout. Nominal FLP timing is not a guaranteed maximum. |
| DP83826A timing | SNLS783A 7.6: reset pulse minimum 25 us; reset-to-SMI maximum 2 ms; POR-to-SMI maximum 50 ms; reset-to-FLP nominal 1500 ms; leader-clock nominal 0.2 ms. | Recheck original PDF columns and selected mode before encoding min/max; a GPIO reset specification is not automatically a BMCR/PHYRCR deadline. Its 100M EEE nominal timings are sleep 210 us, quiet 20 ms, system TX wake 36 us and refresh 200 us; these are not software operation deadlines. |
| DP83822 timing | SNLS505H 6.6-6.8: power-up register readiness wait up to 200 ms; hardware reset pulse minimum 10 us, XI stable at least 1 us during low; post-reset SMI maximum 2 ms; reset configuration latch typical 120 ns, output transition typical 64 ns, reset-to-FLP typical 1.5 s. | Original PDF page 11 columns independently rechecked: table 6.8 T3/T4/T5 are TYP, not MAX. Separate readiness, reset completion and link acquisition. These nominal values establish no guaranteed FLP deadline; do not import 25/26 timing or turn missing FLPs into a transport-health fault. |
| DP83822 TDR | SNLA253 2.3: set `0x0171[3:0]=0xC`, `0x0173[15:8]=0xFF`, `0x0177[12:8]=0x18`; auto-run `0x0009[8]` or manual start `0x001E[15]`; status `0x001E[1:0]`; results `0x0180..0x018A`. | Preserve other fields. The printed `000011000` value for a five-bit field equals 0x18; confirm original rather than miscounting digits. Five TX/RX peaks: locations 0180..0184, amplitudes 0185..0189, polarity 018A[15:6] (0 positive, 1 negative), excess-peak flags[3:2]. Account for documented invalid 0xC samples and inconclusive/terminated cables. |
| DP83822 TDR conversion | SNLA253 2.3.2: intermediate `D*0.8621-8`; meters `intermediate+(70-intermediate)*0.01`; example D=`0x33`=51 yields about 36.31 m. Amplitude code divided by 256. | Source calibration is DP83822-specific, not interchangeable with DP83825. Preserve raw values and qualify cable/conditions, unit/rounding/invalid-sample behavior independently. No unmeasured universal accuracy claim. |
| DP83822 deep power-down | SNLS505H table 8-76: DPDWN `0x0428[2]` enable, bit 3 exit-speed 1=50 ms, 0=100 ms; requires prior IEEE power-down BMCR `0x0000[11]` or INT/PWDN_N. | Complete entry/wake/restore with admission and finite deadlines. Same register bits 5/4 also participate in TX idle-pattern test with `0x0027[3:0]`; exact field preservation prevents an accidental test mode. Linux generic suspend is not complete deep-power support. |
| DP83822 SFD indication | SNLS505H 7.3 SFD discussion: after muxing RX/TX SFD, example `0x04A0=0x1080`, then `0x001F=0x4000`. | WoL enable improves SFD-detection accuracy per document, not only packet wake. Reconcile owned RXFCFG, reset and mux state; never clobber wake/filter controls by blindly replaying a whole-register example. |

## Corrections and independent acceptance obligations

Keep identity `A140/A150`, DP83825 RCSR reset/reserved values,
shifted FX enable, reset labels/domains and Strap11/CR3 EMC differences
as named source issues with exact evidence and next steps. None is resolved by
this catalogue. Later TI corrections or observed hardware may resolve them;
record the actual evidence and update affected prompts/contracts/tests together.

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
deadline and cleanup exhaustion. Source-derived literal traces must be independent
of production constants. New substantive evidence can require correcting this
plan; do so before implementing guessed behavior. Missing hardware is a visible
qualification gate while eligible documented software work continues.
