# DP83826I full feature and register inventory

Reviewed on **2026-10-26** for exact **DP83826I**, including DP83826IRHBR,
against [TI SNLS647I](source/TI-DP83826I-E-SNLS647I.pdf), August 2025.
This is a planning and source-audit checklist. No listed operation, API, CLI,
test or chip support is implemented by this document. DP83826E and A-generation
or other family members still require their own comparison and qualification.

The scope is the chip's complete documented software-controllable functionality,
not only the initial ESP32-P4 RMII/100-full application. Optional activation at
runtime does not make a documented capability optional implementation scope.
Each software feature needs a usable public operation, CLI coverage, validation,
failure/restore tests and a physical qualification row. A generic raw-register
escape hatch alone does not complete a feature. Hardware-only behavior needs
accurate configuration/observation documentation, without invented setters.

An unresolved source conflict remains a visible feature gate and research task;
it is not permission to silently drop the feature or implement guessed behavior.
Field names and register addresses below are factual navigation aids, not a
replacement register specification or frozen public API.

The [extracted public-library catalogues](library-evidence.md) provide additional
numeric inputs and rejected source behavior. Reconcile them into this inventory
and the implementation ledger using [EVIDENCE](../prompts/EVIDENCE.md), rather
than maintaining independent production register maps from vendor headers.

Prompt 3 reconciled the core rows into the authoritative
[JSON ledger](../registers/dp83826i.json) on 2026-10-28. Its
[coverage matrix](../feature-coverage.md) names prompt-4/5 owners for every
remaining feature and recipe offset. This catalogue remains source navigation;
current numeric/access decisions belong in that ledger and its independent tests.

## Source basis and applicability

- Primary sections 8.3, 8.4 and 8.5: [SNLS647I](source/TI-DP83826I-E-SNLS647I.pdf).
  Register tables on PDF pages 52-53, the EEE recipe on page 26 and compliance
  control on page 75 were visually inspected. Previously reviewed pages 49,
  65 and 68 support strap, status and counter caveats.
- [SNLA423A](source/TI-DP83826-Troubleshooting-SNLA423A.pdf): exact DP83826
  troubleshooting, MDI/MII/RMII checks, MSE interpretation and loopback/PRBS
  procedures. Some addresses and procedure labels conflict with the datasheet;
  see the conflict list below.
- [SNLA266B](source/TI-DP8382x-Compliance-SNLA266B.pdf): section 5 explicitly
  distinguishes DP83825/26 and DP83822 10BASE-Te test recipes. Section 4's VOD
  tuning example is DP83825-specific; do not transplant it as DP83826 authority.
- [SNLA261A](source/TI-Wake-on-LAN-SNLA261A.pdf): table 2-1 explicitly supports
  Magic Packet and Secure-ON on DP83826, but **not custom pattern matching**.
  Its DP83822 pattern registers are not additional DP83826 capabilities.
- [SNLA328](source/TI-Energy-Efficient-Ethernet-SNLA328.pdf): EEE background
  and DP83825I implementation example. Use SNLS647I for DP83826 programming;
  apparent differences in bypass-bit polarity must not be flattened away.
- [SNLA330A](source/TI-Cable-Diagnostics-SNLA330A.pdf): its title is generic,
  but the implementation section is DP83825I TDR. It is not a verified DP83826
  sequence. [SNLA253](source/TI-DP83822-TDR-SNLA253.pdf) is DP83822-specific.
- [SNLA344C](source/TI-DP83826-EtherCAT-SNLA344C.pdf),
  [SNLA494](source/TI-DP83826-EtherCAT-Frame-Errors-SNLA494.pdf) and
  [SNLA495](source/TI-DP83826-EtherCAT-Link-Down-SNLA495.pdf): application
  context and targeted procedures; ESC/C2000 registers are not P4 master APIs.
- [Pinned source references](source-code.md) supplement these sources. They
  cannot establish a field's meaning by themselves or override exact-part
  datasheet evidence. Public/manufacturer research should record any later
  clarification separately without replacing archived bytes.

## Feature checklist and implementation-stage ownership

All rows start **NOT IMPLEMENTED / NOT QUALIFIED**. Stage numbers refer to the
[promptset](../prompts/README.md). Prompts 3-8 establish exact evidence and
contracts; 43-52 add CLI/runner cases, 53-55 review cross-module regressions,
and 72-73 record applicable physical evidence. Each feature prompt closes its
own working behavior, tests and documentation. Prompt 36 audits complete primary
coverage; it is not a catch-all for missing feature implementation.

| Feature family | Complete scope to account for | Exact source | Main stages |
| --- | --- | --- | --- |
| Identity and capabilities | Raw IDs, OUI/model/revision, BASIC/ENHANCED distinction, declared exact part vs observable identity, address and capability evidence | 8.5.1.2-4, .25; 8.4.1 | 14, 19 |
| Standard control | Software PCS reset, forced speed/duplex, negotiation enable/restart, MII isolation and its RMII-clock effect, collision test | 8.5.1.1 | 15-18, 23, 33, 35 |
| Advertisement and negotiation | All documented 10/100 half/full abilities; selector, pause/asymmetric pause; reject prohibited local ANAR remote-fault bit 13 while preserving partner/status observation; partner abilities, completion and parallel-detection faults | 8.3.1; 8.5.1.1-7, .25 | 17, 19, 37-39 |
| Next-page exchange | Local next-page/message/code/acknowledge2 settings; partner page, toggle/acknowledgement and page-received evidence; interaction with EEE negotiation | 8.5.1.7-9; 8.3.3.2 | 17, 19-20, 29, 34 |
| MDI/MDIX | Automatic crossover, robust automatic crossover in forced operation, manual MDI/MDIX, observed pair routing and port-mirror/polarity-swap controls | 8.3.2; 8.5.1.10, .12, .16, .25 | 18-19, 34 |
| EEE capability/negotiation | Actual DP83826 enable procedure, local/partner ability, bypass configuration, negotiated validity, LPI evidence and wake-error counters using the documented MMD spaces | 8.3.3.1-2; 8.3.11.1; additional EEE register descriptions need source closure | 29, 37-39 |
| EEE with legacy MAC | Explicit enter/exit TX LPI control, MAC/application synchronization, no transmit while PHY is in LPI, absent PHY frame buffering, cancellation/restore | 8.3.4 | 30, 37-39, 68 |
| WoL | Magic Packet MAC-address match, Secure-ON password enable/data, CRC gate, enable/disable, pulse length vs level indication, level acknowledgement, wake/status/interrupt routing and error evidence | 8.3.5.1-3; 8.5.1.19, .56-.59, .72-.79; SNLA261A | 31 |
| PHY power states | Normal operation; active-sleep enable policy and automatic partner-dependent state; explicit IEEE/deep power-down and PLL state, applicable timing, retained configuration and clock effects | 8.3.6.1-3; 8.5.1.1, .17, .22 | 15-16, 28, 37-39 |
| RMII repeater | Leader/follower and follower/follower arrangements; CRS_DV vs RX_DV selection, configuration validation and observable state; physical two-PHY wiring remains application-owned | 8.3.7; 8.3.10; 8.4.1; 8.5.1.23, .55 | 33, 68 |
| Clock output | MAC-interface, XI pass-through, PLL/free-running and recovered-clock options where supported by each pin/mode; clock disable/multiplexing and status; reset/power interactions | 8.3.8, 8.3.18; 8.5.1.55-.60 | 32-33 |
| MII interface | Full 10/100 and half/full operation, CRS/COL/RX_ER behavior, odd-nibble handling, fast RX_DV and full-duplex collision signaling controls | 8.3.9; 8.5.1.10-.11, .17 | 17-18, 33, 37-39 |
| RMII interface | Leader/follower clock source, MII/RMII selection, revision 1.0/1.2 behavior, follower TX-clock shift, receive elasticity buffer, underflow/overflow observation, CRS_DV/RX_DV output selection | 8.3.10; 8.5.1.23, .55 | 19-20, 33, 37-39 |
| Management interface | Clause 22 transactions, PHY addressing, preamble/suppression constraints, MDC/MDIO timing; vendor and EEE indirect address/read/write operations; namespace validation | 8.3.11.1-6; 7.6; 8.5.1.13-.14 | 11-14 |
| 100BASE-TX data path | Coding/scrambling/NRZI/MLT-3 and receive processing are hardware; account for exposed scrambler bypass, error detection, signal/descrambler status and analog controls rather than invent software encoding APIs | 8.3.12.1-2; 8.5.1.11, .16-.17, .61-.65 | 19-20, 34-35 |
| 10BASE-Te data path | Required VOD_CFG3 initialization; receiver threshold/squelch and gain controls, NLP transmit control, jabber enable/disable/status, polarity evidence and preamble-length mode | 8.3.13.1-4; 8.5.1.26, .33, .63, .65 | 17-19, 34-35 |
| All loopback modes | MII, PCS input/output, digital, analog and reverse/far-end; exact 10/100 restrictions, termination, required companion writes, far-end FIFO depth and 100-Mbit transmit-during-MII-loopback option | 8.3.14.1-6; 8.5.1.1, .17, .22 | 23 |
| BIST/PRBS | Generator/checker disabled, single-packet generation, checker-only and continuous combined modes; packet length/IPG, generator active, lock/sync loss, count mode and explicit counter latch/clear acquisition | 8.3.15; 8.5.1.22, .27-.28 | 24 |
| Cable diagnostics | Manual start and hardware autorun-on-link-down configuration; done/fail; A/B transmit and cross/same-pair capture; segment, averaging, DC-removal and shadow/threshold/pulse controls; available peak position/amplitude/polarity/cross evidence; calibrated distance/fault interpretation | 8.3.16.1; 8.5.1.10, .29, .36-.53 | 25-26 |
| Fast Link Drop | Supported MII conditions: RX error, MLT-3 error, low-SNR threshold, energy loss and descrambler-loss mechanisms; window/count/threshold settings, latched cause and mode limitations | 8.3.17; 8.4.1.1; 8.5.1.12, .15, .34-.35 | 19-20, 34 |
| Link-loss recovery | Explicit hardware link-loss-recovery control and semantics, distinct from application retry/reset policy; source interpretation and compatible FLD settings | 8.5.1.10 | 18, 34 |
| LED presentation | Every documented source choice for each LED, activity/error/link-loss/LPI indications, common blink rate/stretch bypass, per-LED polarity and forced-drive value/enable | 8.3.18; 8.5.1.24-.25, .31, .67, .71 | 32 |
| Pin multiplexing | LED/GPIO constant output, WoL, interrupt, TX_ER/COL/CRS and supported clock routing; pin-specific restrictions and clock-vs-LED ownership | 8.3.18; 8.5.1.55-.60 | 27, 31-33 |
| Interrupts | Global/output enable and polarity, software test interrupt, all MISR1/MISR2 sources and enables, read-clear collection, shared WoL/polarity source selection and deferred service | 8.5.1.17-.19, .73 | 20, 27 |
| Status/counters | Current and latched link, speed/duplex, signal/descrambler, receive error, false carrier, polarity, remote fault, jabber, MDI routing, page/IRQ state and physical error counters; validity and destructive-read ownership | 8.5.1.2, .7, .15-.16, .18-.21, .23, .25-.28, .73 | 19-21, 34 |
| Signal quality | Raw mean-square-error observation and source-supported SNR/quality interpretation; resolve address/scaling conflict before claiming derived units | 8.5.1.54; SNLA423A 2.3.3 | 34 |
| Compliance stimulus | Documented 10BASE-Te pattern selections, 100BASE-TX zero-run patterns, enable/reconfigure/disable sequences, forced-mode prerequisites and cleanup; fixture ownership separate from normal traffic | 8.5.1.32; SNLA266B section 5 | 23, 35, 51, 73 |
| Analog and MAC output controls | Documented MDI/MDIX transmit-amplitude components, 10-Mbit termination controls, receive comparator/gain, MAC slew/impedance controls and supported combinations; no arbitrary reserved-bit tuning | 8.5.1.55, .61-.65, .68 | 35, 51, 73 |
| Timing and compatibility controls | Minimum IPG control, extended full-duplex behavior with forced link partners, RX_ER during idle, odd-nibble handling and mode-specific compatibility behavior | 8.5.1.10-.11, .66 | 17-18, 33, 35 |
| Reset and strap evidence | BMCR PCS reset vs PHYRCR hard reset vs restart, external reset/POR, all loaded strap values and decoded state, POR-only straps, exact reapplication and observation | 8.4.1.1-2; 8.5.1.1, .30, .69-.70; 7.6 | 14-16, 33 |
| Resistor calibration contingency | CDCR calibration-enable observation and the documented deassert/hard-reset response, as an explicit admitted operation with restoration, never an unbounded automatic recovery loop | 8.5.1.29 | 15-16, 25, 35 |

MISR1 covers link quality, energy detect, link, speed, duplex, negotiation
completion, false-carrier half-full and receive-error half-full events. MISR2
covers EEE error, negotiation error, next page, loopback FIFO, MDI crossover,
sleep, polarity/WoL and jabber. These sources need separate names, enable masks
and event-preservation tests, even if the initial board polls without an IRQ pin.

Port mirroring/pair inversion is a PHY wiring aid, not an Ethernet switch port
mirror. RMII repeater mode is not a software packet bridge. WoL keeps the PHY
able to receive its matching frame; it must not be equated to every PHY
power-down state. Board power delivery and PoE remain outside this inventory.

## Complete register-summary catalogue

SNLS647I table 8-17 contains **79 entries: 30 direct Clause 22 addresses and
49 extended entries**. The `Section` column below abbreviates `8.5.1.N` as `N`.
Names follow the source, including `ALNPAR`; internal naming may correct spelling
only with an explicit source-name mapping. This catalogue does not declare every
bit readable, writable or useful: each field's access/reset semantics still need
the detailed table. In particular, some named registers contain only reserved
fields in this revision.

Direct addresses use ordinary Clause 22 access. Section 8.3.11 permits indirect
access to the standard set as well, **except REGCR and ADDAR themselves**, which
must use direct access. Extended entries use the vendor MMD namespace, DEVAD 31,
through REGCR/ADDAR. All multi-frame indirect operations require one serialized
transaction. Native Clause 45 is not implied.

| Direct address | Source name | Section | Coverage |
| --- | --- | --- | --- |
| `0x0000` | BMCR | 1 | Standard control/reset/isolation/test |
| `0x0001` | BMSR | 2 | Capabilities, negotiation and latched status |
| `0x0002` | PHYIDR1 | 3 | Identity |
| `0x0003` | PHYIDR2 | 4 | Identity/mode/revision |
| `0x0004` | ANAR | 5 | Advertisement |
| `0x0005` | ALNPAR | 6 | Link-partner abilities |
| `0x0006` | ANER | 7 | Negotiation expansion/page/fault |
| `0x0007` | ANNPTR | 8 | Local next page |
| `0x0008` | ANLNPTR | 9 | Partner next page |
| `0x0009` | CR1 | 10 | TDR autorun, recovery, robust MDIX, fast RX_DV |
| `0x000A` | CR2 | 11 | Extended duplex, idle error, odd nibble |
| `0x000B` | CR3 | 12 | FLD, pair/polarity swap |
| `0x000D` | REGCR | 13 | Indirect access control |
| `0x000E` | ADDAR | 14 | Indirect address/data |
| `0x000F` | FLDS | 15 | FLD cause |
| `0x0010` | PHYSTS | 16 | Live/latched PHY observations |
| `0x0011` | PHYSCR | 17 | Power, scrambler, loop FIFO, collision, IRQ |
| `0x0012` | MISR1 | 18 | Interrupt status/enables |
| `0x0013` | MISR2 | 19 | Interrupt status/enables |
| `0x0014` | FCSCR | 20 | False-carrier counter |
| `0x0015` | RECR | 21 | Receive-error counter |
| `0x0016` | BISCR | 22 | BIST/loopback/power observation |
| `0x0017` | RCSR | 23 | RMII controls/status |
| `0x0018` | LEDCR | 24 | LED blink/polarity/drive |
| `0x0019` | PHYCR | 25 | MDIX/pause/LED/address observation |
| `0x001A` | 10BTSCR | 26 | 10-Mbit receiver/NLP/polarity/jabber |
| `0x001B` | BICSR1 | 27 | BIST error acquisition and IPG |
| `0x001C` | BICSR2 | 28 | BIST packet length |
| `0x001E` | CDCR | 29 | Cable-test/calibration control/status |
| `0x001F` | PHYRCR | 30 | Hard reset/restart |

The direct holes are `0x000C` and `0x001D`. They are not general scratch
registers or valid destinations for ordinary write/dump commands.

| Vendor extended address (DEVAD 31) | Source name | Section | Coverage |
| --- | --- | --- | --- |
| `0x0025` | MLEDCR | 31 | LED0 source/routing/polarity |
| `0x0027` | COMPT | 32 | Compliance patterns |
| `0x002A` | 10M_CFG | 33 | 10-Mbit preamble mode |
| `0x0117` | FLD_CFG1 | 34 | MLT-3 FLD window/count |
| `0x0131` | FLD_CFG2 | 35 | Descrambler FLD threshold |
| `0x0170` | CDSCR | 36 | TDR channel/averaging/segment |
| `0x0171` | CDSCR2 | 37 | Detailed table is all reserved |
| `0x0173` | CDSCR3 | 38 | TDR duration/skip |
| `0x0175` | TDR_175 | 39 | TDR shadow controls |
| `0x0176` | TDR_176 | 40 | TDR peak-location threshold |
| `0x0177` | CDSCR4 | 41 | TDR short-cable threshold |
| `0x0178` | TDR_178 | 42 | TDR pulse width |
| `0x0180` | CDLRR1 | 43 | First TDR peak location |
| `0x0181` | CDLRR2 | 44 | Detailed table is all reserved |
| `0x0182` | CDLRR3 | 45 | Detailed table is all reserved |
| `0x0183` | CDLRR4 | 46 | Detailed table is all reserved |
| `0x0184` | CDLRR5 | 47 | Detailed table is all reserved |
| `0x0185` | CDLAR1 | 48 | First TDR peak amplitude |
| `0x0186` | CDLAR2 | 49 | Detailed table is all reserved |
| `0x0187` | CDLAR3 | 50 | Detailed table is all reserved |
| `0x0188` | CDLAR4 | 51 | Detailed table is all reserved |
| `0x0189` | CDLAR5 | 52 | Detailed table is all reserved |
| `0x018A` | CDLAR6 | 53 | TDR peak polarity/cross indication |
| `0x0218` | MSE_Val | 54 | Raw MSE; address conflict below |
| `0x0302` | IO_CFG1 | 55 | MAC output, RX_DV, clock mux |
| `0x0303` | LED0_GPIO_CFG | 56 | LED0/GPIO/clock mux |
| `0x0304` | LED1_GPIO_CFG | 57 | LED1/GPIO/clock mux |
| `0x0305` | LED2_GPIO_CFG | 58 | LED2/GPIO mux |
| `0x0306` | LED3_GPIO_CFG | 59 | LED3/GPIO mux |
| `0x0308` | CLK_OUT_LED_STATUS | 60 | Clock-vs-LED availability |
| `0x030B` | VOD_CFG1 | 61 | Transmit-amplitude components |
| `0x030C` | VOD_CFG2 | 62 | Transmit-amplitude components |
| `0x030E` | VOD_CFG3 | 63 | 10-Mbit termination setup |
| `0x0404` | ANA_LD_PROG_SL | 64 | Detailed table is all reserved |
| `0x040D` | ANA_RX10BT_CTRL | 65 | 10-Mbit receive comparator/gain |
| `0x0456` | GENCFG | 66 | Minimum IPG |
| `0x0460` | LEDCFG | 67 | LED1/2/3 sources |
| `0x0461` | IOCTRL | 68 | MAC slew/impedance; conflict below |
| `0x0467` | SOR1 | 69 | Raw strap observations |
| `0x0468` | SOR2 | 70 | Loaded configuration observations |
| `0x0469` | LEDCFG2 | 71 | LED1/2 drive/polarity |
| `0x04A0` | RXFCFG1 | 72 | WoL configuration/acknowledgement |
| `0x04A1` | RXFS | 73 | WoL source/status/errors |
| `0x04A2` | RXFPMD1 | 74 | WoL MAC match bytes |
| `0x04A3` | RXFPMD2 | 75 | WoL MAC match bytes |
| `0x04A4` | RXFPMD3 | 76 | WoL MAC match bytes |
| `0x04A5` | RXFSOP1 | 77 | Secure-ON bytes |
| `0x04A6` | RXFSOP2 | 78 | Secure-ON bytes |
| `0x04A7` | RXFSOP3 | 79 | Secure-ON bytes |

All other vendor offsets are holes in this table. Important examples include
`0x0020-0x0024`, `0x0026`, `0x0028-0x0029`, `0x0172`, `0x0174`,
`0x0179-0x017F` and gaps between the listed blocks. A range containing documented
endpoints is not an implied valid register bank. The procedure-only addresses
below require separate exact-source treatment rather than broad range access.

## Documented procedure-only addresses and namespaces

The table-8-17 warning about unlisted addresses conflicts with several explicit
procedures elsewhere in the same datasheet. Preserve both facts. Build an
allowlist of **exact part/revision + feature + address namespace + operation +
permitted mask/value/sequence** after source review. A recipe exception does not
make an otherwise reserved address an arbitrary public writable register.
Conversely, absence from table 8-17 alone must not silently delete an expressly
documented chip function.

| Feature/source | Addresses beyond the main catalogue | Required treatment |
| --- | --- | --- |
| EEE initialization, 8.3.3.1, page 26 | Vendor offsets `0x04D1`, `0x04D3`, `0x04DF`, `0x033E`, `0x033F`, `0x0123`, `0x031B`, `0x0466`, `0x04CF`, `0x0416`, `0x04F5`, `0x04E0`, `0x031F`, `0x04F4`; the recipe also names EEE address `0x203C` and already catalogued `0x001F`, `0x0308`, `0x0000` | Audit the entire ordered recipe, hard-reset/negotiation effects, register namespaces, restoration and every failure boundary. Do not expose undocumented internal fields as freely tunable settings. |
| EEE control/status, 8.3.3.2 and 8.3.11.1 | Printed MMD 3 addresses `0x1001`, `0x1014`, `0x1016`; printed MMD 7 addresses `0x203C`, `0x203D`; vendor DEVAD 31 `0x04D1` (EEECFG3). Exact on-wire EEE offsets/possible aliases remain unresolved | Preserve the printed namespace/address separately from verified wire offsets. The cited DP83826 sections identify these EEE addresses but do not prove an alias translation or provide a complete detailed field catalogue; close this from exact applicable official evidence. Never convert them to vendor offsets or native Clause 45 merely by name. |
| Legacy-MAC LPI, 8.3.4 | Vendor `0x04D1` control fields; printed MMD7 address `0x203C`, subject to the same unresolved wire-offset gate | Typed enter/exit and enable/advertise procedures need explicit application/MAC coordination. No PHY TX buffering is available in LPI. |
| Deep power-down, 8.3.6.3 | Vendor `0x0428`, bit 2, followed by IEEE power-down | Resolve relationship to PHYSCR PLL-disable and define exit/clock restoration before implementing. |
| Digital loopback, 8.3.14.4; SNLA423A 2.5 | Vendor `0x0122`, `0x0123`, `0x0130` in addition to the base loopback/reset registers | Allow only the sourced loopback preparation values/ordering, validate speed/mode, and reconcile overlap with the EEE recipe at `0x0123`. |
| Compliance stimulus, 8.5.1.32 | Vendor `0x0428`, bits 4/5; COMPT `0x0027` bit 4 is used by prose despite being grouped as reserved in its table | Resolve and implement exact enable/reconfigure/stop procedure; normal low-level RMW must not automatically authorize these reserved-field exceptions. |
| Signal quality, SNLA423A 2.3.3 | `0x0225` in the app note vs `0x0218` in SNLS647I | Conflict requiring clarification, not two automatically equivalent APIs. Record raw evidence and withhold derived SNR until address/scaling are resolved. |

The bare address `0x203C` in the EEE recipe is insufficient to select DEVAD;
section 8.3.3.2 supplies the printed MMD7 association, not a proved wire-offset translation. A future implementation must check
the exact host/MMD access convention, including any vendor tooling translation,
instead of interpreting the printed hexadecimal number as a Clause 22 address.

## Source conflicts and limitations requiring explicit closure

1. **Register-summary holes vs feature recipes:** the exact-source allowlist
   above is necessary. General arbitrary writes to reserved fields remain invalid.
2. **MSE address:** SNLS647I 8.5.1.54 defines `0x0218` and refers to SNLA423;
   SNLA423A table 2-6 gives `0x0225`. Units and SNR computation require closure.
3. **RECR saturation:** table 8-39 describes a 16-bit field and an `0xFF` maximum
   with `0x7F` half-full indication. Do not infer a 16-bit cumulative counter.
4. **PHYSTS forced mode:** table 8-34's ENHANCED-mode speed/duplex annotations
   require reconciliation with BMCR and actual forced-mode behavior.
5. **Loopback/PRBS and reset guide errors:** SNLA423A contains differing reverse
   loopback values, PRBS lock references to RCSR, and BMCR described as hard reset.
   Compare to BISCR/PHYRCR detailed definitions and exact mode-specific sequences.
   Its general all-speed loopback sentence cannot override individual restrictions.
6. **MMD IOCTRL example vs detailed fields:** 8.3.11.6 uses `0x0461 = 0x0400`
   for an impedance example; 8.5.1.68 groups that high bit as reserved and describes
   a low-bit slew selection. Do not promote the example to an ordinary setting.
7. **Clock/GPIO diagram vs field tables:** 8.3.8/8.3.18 describe clock routing
   options that are not uniformly represented by 8.5.1.56-.59; some GPIO controls
   are marked read-only and some diagrammed clock fields reserved. Resolve per pin
   and BASIC/ENHANCED mode; no universal GPIO-function enum without evidence.
8. **COMPT enable bit:** detailed table and procedure prose disagree about bit 4.
   SNLA266B provides feature-specific corroboration, not a blanket reserved-bit rule.
9. **TDR result banks:** general prose describes multiple peaks, but several
   CDLRR/CDLAR detailed tables are entirely reserved. Support actual documented
   results; do not decode five independent peaks from a name alone. Exact
   DP83826 cable classification/calibration still needs sufficient procedure evidence.
10. **EEE cross-part guidance:** SNLA328 is DP83825I-oriented and describes bypass
    values differently from the DP83826 datasheet. Audit each part separately,
    including reset and fixed writes. Do not hide incomplete evidence by dropping EEE.
11. **RMII status polarity:** RCSR overflow/underflow fields are marked read-clear
    yet descriptions encode zero as a fault and one as normal. Preserve the exact
    table and verify interpretation rather than applying a generic active-high rule.
12. **Reserved/all-reserved and minimally described fields:** a named register or
    `R/W` marking does not authorize changing fields labeled reserved. The WoL
    bit-mask flag does not establish custom pattern matching on DP83826; SNLA261A
    explicitly excludes that feature.

## Corrections from the prompt audit

The correctness review adds these specific constraints to the source ledger:

- **W0C and mixed registers:** SNLS647I table 8-18 defines W0C/W1S. RXFCFG1
  bit 11 is W0C (table 8-90): zero acknowledges level indication. Unrelated
  updates must use command-neutral composition. SNLA261A table 2-4 is a DP83822
  example with different acknowledgement and MAC-word ordering; use DP83826
  tables 8-90 and 8-92 through 8-94 for literal acknowledgement/byte-order vectors.
- **Consumed event evidence:** MISR1/MISR2 and FCSCR/RECR have clear-on-read
  behavior (tables 8-36 through 8-39). Retain each valid consumed sample even if
  the aggregate acquisition fails; ambiguous failed reads can lose event history.
- **Active sleep:** 8.3.6.1 enables a hardware policy through PHYSCR[14:12]=110;
  actual sleep/wake depends on partner detection. BISCR[8] observes current state.
  Successful policy configuration does not require the PHY to enter sleep.
- **MII loopback:** the general recipe in 8.3.14.2 conflicts with the explicit
  speed-dependent BMCR table 8-19: BISCR[4:0]=00100 at 100BASE-TX and 00001 at
  10BASE-Te. Use the detailed definition absent stronger exact-part evidence and
  retain the discrepancy plus independent vectors for both speeds.
- **EEE printed addresses versus wire offsets remains open:** SNLS647I prints
  MMD3 1001/1014/1016 and MMD7 203C/203D; SNLS783A and SNLA328 also explicitly
  pair these full numbers with MMD selectors. Conversely, SNLS638C table 7-92
  refers to offsets 0014 in MMD3 and 003C/003D in MMD7 despite similar full-number
  headings. Do not silently strip address digits or assert a translation. Obtain
  an exact applicable access trace/clarification before freezing the EEE wire
  tuples; ordinary proved vendor-register addressing remains separate.

## Corrections from public-library extraction

The [TI core](library-ti-core.md) and [feature](library-ti-features.md) catalogues
record exact source locations and independent datasheet comparisons:

- TI RTOS reset completion uses mask `0x8000` as an address. PHYRCR is C22
  `0x1F`; the mask and address must remain distinct inputs.
- MLEDCR `0x0025[6:3]` needs mask `0x0078`, not TI's `0x0038`. Test a mode
  using bit 6. Bit 0's zero encoding is reserved; do not invent a disable switch.
- LEDCFG `0x0460` places LED1 at `[7:4]` and LED3 at `[15:12]`; TI RTOS and
  MCU+ labels swap them. LEDCFG2 `0x0469[10:8]` is reserved on this exact part,
  despite vendor definitions for LED3 drive/polarity.
- MCU+ calls setting CR2 bit 1 odd-nibble detection enable; table 8-29 says
  that value disables detection. Derive the public Boolean from the field table.
- RECR `0x15` is described as a 16-bit RC field, but its prose gives maximum
  `0xFF` and half-full `0x7F`. Preserve the raw sample and record this conflict;
  do not invent counter width, overflow or saturation semantics.

## Hardware-only scope and closure rule

BASIC/ENHANCED pinout selection, oscillator/crystal and board wiring, voltages,
magnetics, physical strap resistor choices and their sample timing are hardware
properties. The library can validate declared configuration, read documented strap
state and expose software-overridable controls; it cannot change a POR-only strap
by pretending to perform a register write. Strap1/Strap6 resampling limits remain
part of the reset contract. DP83826I has MII/RMII, not RGMII; later DP83822 support
must independently include its documented RGMII capabilities.

Full coverage requires every feature row and every non-reserved field to map to
maintained source evidence, a public operation/value, allowed states, timing and
ownership rules, CLI help/dispatcher, host failure tests, runner cases and a
physical evidence status. Record hardware-only, truly absent and source-blocked
items explicitly. A first-board wiring limitation may block physical testing or
that board's activation, but cannot remove the generic software capability.

The catalogue itself proves coverage of the archived document's listed addresses,
not correctness of a driver, current silicon clarification, electrical compliance
or support for another part. Implementation and qualification remain future work.
