# Prompt 4 diagnostic source review

Reviewed 2026-10-28 for exact DP83826I. This is source research, not an executed
driver or physical test. Lead owns shared ledger/contracts/Git. Writable scope:
this report and temporary `.prompt4-diagnostics.json`; siblings and vendor
payloads are read-only. Execution branch:
`promptset/dp8382x-implementation-20261028`, entry checkpoint
`42a908b28d9ad471a60b5ca220127eb9e7dbf9ec`.

Root/parent AGENTS, COMMON, prompt 4 and EVIDENCE govern this review. Strong
reasoning was selected for contradictory silicon tables, destructive counter
actions and result freshness: gpt-6-astra with high reasoning effort. Keep
address spaces explicit, uncertain effects visible, cleanup finite and temporary
diagnostic values outside persistent desired configuration.

## Sources and original-PDF inspection

Primary: [SNLS647I, August 2025](../../reference/source/TI-DP83826I-E-SNLS647I.pdf),
8.3.14-16, tables 8-19, 8-28, 8-35, 8-40, 8-45..47, 8-54..71. SHA-256:
`4e440fb8bd1cd44ee9ab0773944bef4f0605884f25c4ab96ea55d0ed859fd9c3`.
The [current official PDF](https://www.ti.com/lit/ds/symlink/dp83826i.pdf)
opened on the review date still identifies SNLS647I. Pages 72, 73, 77, 78, 80
and 81 were rasterized directly from the archived original in memory and
visually inspected: access/reset columns, reserved banks, segment words and the
BICSR1 overlapping write exception were confirmed. No images or vendor changes
were retained. Other relevant pages were inspected as original-derived text.

[SNLA423A, June 2025](../../reference/source/TI-DP83826-Troubleshooting-SNLA423A.pdf),
2.5 pp18-20, was compared with its
[official PDF](https://www.ti.com/lit/an/snla423a/snla423a.pdf).
Archived SHA-256:
`6ed711daa960bc7f9530cd1298960e75fed049b49eb0285ed7b8e0d19812cae1`.
It corroborates digital-loopback and counter-capture actions but contains
reverse/status/reset/readback discrepancies below. Its one-second observation
suggestion is not a maximum silicon completion bound.

[SNLU262A, June 2023](../../reference/source/TI-DP83826EVM-SNLU262A.pdf),
3.2/table 3-1 p11, describes actual EVM MII/RMII copper, PRBS, external MAC
loopback and MSP430 register access possibilities. It is fixture guidance, not
an executed test. Original URL: [TI SNLU262](https://www.ti.com/lit/pdf/snlu262).

[SNLA330A, January 2020](../../reference/source/TI-Cable-Diagnostics-SNLA330A.pdf),
2-3 pp3-4, explicitly targets DP83825I. Its
[official source](https://www.ti.com/lit/pdf/SNLA330) is not exact DP83826I
calibration authority. SNLS647I independently gives segment3 `0173=1A21`,
`0175=1005`; the DP83825 note prints different words and adds analog settings
at0456/0411/0416/0190. Do not transfer those settings or its no-peak/fault-length
algorithm. SNLS647I has no ALCD register/active-link cable-length facility:
CDLAR means cable diagnostic amplitude result, not ALCD.

[SNLA344C, October 2023](../../reference/source/TI-DP83826-EtherCAT-SNLA344C.pdf)
introduction/section2, [SNLA494, February 2026](../../reference/source/TI-DP83826-EtherCAT-Frame-Errors-SNLA494.pdf)
abstract/sections1-2 and [SNLA495, February 2026](../../reference/source/TI-DP83826-EtherCAT-Link-Down-SNLA495.pdf)
abstract/sections1-3 establish EtherCAT ESC/C2000 context. Original URLs:
[SNLA344](https://www.ti.com/lit/pdf/SNLA344),
[SNLA494](https://www.ti.com/lit/pdf/SNLA494),
[SNLA495](https://www.ti.com/lit/pdf/SNLA495). ESC loopback/error policy, reset
wiring and MII-only Fast Link Drop do not transfer to an ESP32-P4 RMII master.
Detailed event/Fast Link Drop review belongs to the other delegated scopes.

All TI documents retain TI copyright. The report adds factual observations and
original analysis; archive provenance is unchanged. Review-date searches for
exact DP83826 TDR, BIST counter and 10-Mbit/s loopback TI E2E clarification found
older datasheets/general results without stronger exact-part conflict resolution.
That is a bounded search result, not proof that no clarification exists. No
third-party implementation was copied. Current primary-driver comparison is in
the lead's stage-4 source audit; detailed exact-part tables govern this review.

## Independent aggregate register oracle

Literals were transcribed from original tables before the integration fragment;
they are independent of proposed fields and future production constants. Values
below are hex, register-aligned. Every ordinary row has reset-known maskFFFF.
Access columns describe the actual table/read effects, not generic permissions.
C22 words have the established DEVAD31 aliases; extended offsets use DEVAD31.

| Register/offset | Reset | Reserved | Ordinary RW | R | RH | Explicit write exception |
| --- | --- | --- | --- | --- | --- | --- |
| BISCR C22/0016 | 0100 | 80A0 | 705F | 0B00 | 0400 | none |
| BICSR1 C22/001B | 007D | 0000 | 00FF | FF00 | 0000 | write8000 snapshot/clear, overlaps read counter |
| BICSR2 C22/001C | 05EE | F800 | 07FF | 0000 | 0000 | none |
| CDCR C22/001E | 0102 | 3FFC | C000 | 0003 | 0000 | RW start8000 self-clears; neutral0 |
| CDSCR/0170 | 0C12 | 888F | 7770 | 0000 | 0000 | none |
| CDSCR2/0171 | C850 | FFFF | 0000 | 0000 | 0000 | none; reserved despite tableR/W |
| CDSCR3/0173 | 0D04 | 0000 | FFFF | 0000 | 0000 | none |
| TDR_175/0175 | 1004 | C7F0 | 380F | 0000 | 0000 | none |
| TDR_176/0176 | 0005 | FFE0 | 001F | 0000 | 0000 | none |
| CDSCR4/0177 | 1E00 | E0FF | 1F00 | 0000 | 0000 | none |
| TDR_178/0178 | 0002 | FFF8 | 0007 | 0000 | 0000 | none |
| CDLRR1/0180 | 0000 | FF00 | 0000 | 00FF | 0000 | none |
| CDLRR2/0181 | 0000 | FFFF | 0000 | 0000 | 0000 | none |
| CDLRR3/0182 | 0000 | FFFF | 0000 | 0000 | 0000 | none |
| CDLRR4/0183 | 0000 | FFFF | 0000 | 0000 | 0000 | none |
| CDLRR5/0184 | 0000 | FFFF | 0000 | 0000 | 0000 | none |
| CDLAR1/0185 | 0000 | FF80 | 0000 | 007F | 0000 | none |
| CDLAR2/0186 | 0000 | FFFF | 0000 | 0000 | 0000 | none |
| CDLAR3/0187 | 0000 | FFFF | 0000 | 0000 | 0000 | none |
| CDLAR4/0188 | 0000 | FFFF | 0000 | 0000 | 0000 | none |
| CDLAR5/0189 | 0000 | FFFF | 0000 | 0000 | 0000 | none |
| CDLAR6/018A | 0000 | F7DF | 0000 | 0820 | 0000 | none |

No above register has a documented read-clear effect. BICSR1 clearing happens
on its explicit write action, not read. Recipe-only0122,0123,0130 have no known
reset/field masks and no ordinary writable permission; only exact literal
actions2000,2000,47FF respectively are established for digital loopback.

BISCR fields: counter mode4000, joint PRBS mode3000, checker lock0800,
hardware sync-loss0400, generator0200, normal-power0100, MII-line transmit0040,
loopback001F. Low-five legal codes0,1,2,4,8,10(hex) only; combinations are
unsupported. PRBS bits13:12 jointly encode0 off,1 single constant packet with
checker off,2 checker only,3 continuous generator plus checker. Bit14 separately
selects counter saturation/rollover; it does not select traffic duration.
SNLA423A5000 thus selects single generator plus rollover, not both paths.

BICSR1 high byte is errored-byte count: saturates255 or wraps by selected mode.
Low IPG code7D means500 bytes; FF means1020. BICSR2 raw field07FF is packet
bytes; reset05EE=1518. Zero is field-representable but no minimum useful/accepted
packet length or valid arbitrary-gap traffic policy is specified. Raw bounds
do not certify all combinations as valid Ethernet frames.

Capture with desired IPG7D writes807D; desiredA5 writes80A5. ReadingFFA5 must
not cause an FFA5 write: high bits are counter data and write15 is destructive.
Preserve desired low byte and send zero bits14:8. Read15 is counterMSB, not
command state. Unknown capture/read effects may lose a window; blind repetition
cannot recover it. Full repeat-capture latch lifecycle remains unestablished.

CDCR start8000 is RW/self-clearing, with neutral0 outside deliberate trigger.
Done0002 is already1 after reset0102; it cannot establish a fresh measurement.
Reserved reset field13:2 code40 produces0100. Fail0001=0 without fresh done is
not a pass. Rescal4000 normally0; if observed1, deassert then immediately
PHYRCR hard reset. Other reviewer owns this recovery; cable code cannot bypass it.

CDSCR cross-disable4000 and TX-channel2000 give combined bits14:13:
A->B=0, B->A=1, A->A=2, B->B=3. DC-no-init1000 preserves DC-removal state,
but preparation/settling prerequisites are unspecified. Average0700 codes0..6
mean1,2,4,8,16,32,64 cycles;7 reserved. Segment0070 codes1..5 mean0-10,10-20,
20-40,40-80,80+ metres;0,6,7 reserved.

| Segment | 0173 duration/skip | 0175 shadow | 0178 pulse code |
| --- | --- | --- | --- |
| 1 | 0D07 | 1004 | 2 |
| 2 | 0D14 | 1004 | 2 |
| 3 | 1A21 | 1005 | 2 |
| 4 | 343B | 1008 | 2 |
| 5 | 8F6F | 100B | 6 |

CDSCR3 reset0D04 differs from recipe0D07; preserve both. Duration fieldFF00,
skip00FF; shadow average3800 always code2, forward000F codes4,4,5,8,B.
TDR176001F has a name/default5 without physical scaling. CDSCR41F00 defaults
1E and compensates strong short-cable reflections, without physical scaling.
TDR1780007 has no established ns/code. The duration expression uses length
times2 times5.2 divided by8; it does not establish a calibrated returned
location conversion. The feature's1-V pulse does not scale amplitude codes.

CDLRR1 low8 and CDLAR1 low7 are raw first peak location/amplitude. CDLAR6
polarity0800 lacks a defined positive/negative bit encoding; cross0020 describes
cross reflection/short between TD+ and TD-. Do not infer calibrated fault type
from that wording. All other result banks are wholly reserved despite names.

## Procedures and conflicts

One admitted application owner serializes driver/collector/raw access and
quiesces MAC/SDK normal activity before disruptive modes. Locks cover complete
MDIO selector/RMW transactions, and are released between waits and before output.
Primary and cleanup deadlines include admission, locks, terminal callbacks and
polling. No nominal or example duration becomes a guaranteed completion maximum.

MII loopback uses BMCR14 with BISCRlow5=4 at100 or1 at10, per table8-19;
forced-full BMCR literals6100/4100 respectively. Qualified10BASE-Te setup still
applies. All-speed bit2 prose does not override this. MII-line transmit bit6 is
100-only. PCS input is10-only/code1; PCS output code2; analog code8 supports
10/100 with100-ohm termination. Disable AN before internal near-end modes.
The name MII loopback does not exclude a configured RMII MAC interface.

Standalone digital is100-only, exact ordered sequence: BMCR2100, BISCR0104,
MMD31/0122=2000,/0123=2000,/0130=47FF,PHYRCR4000. Last action is digital
restart preserving registers. Opaque words are exact actions, never general
field permissions;0123 is also used by EEE. Stopping selector alone does not
restore opaque settings. Explicitly admitted reset/reinitialize/persistent
replay is a conservative cleanup plan; otherwise restoration stays unverified.

Reverse code10 returns partner traffic at the MAC interface, ignoring local
MAC TX. Save/restore requested PHYSCR FIFO depth, observe finite partner payloads
and clear selector. External cable loopback has no internal selector, requires
an identified authorized fixture and is exempt from internal AN-disable rule.
Observe correlated MAC/partner traffic; configuration readback is not path proof.

SNLA423A reverse0020 writes reserved bit5; status address0017 is RCSR, not
BISCR0016. Analog script3108 plus lock/generator status would retain selector8
and read3B08 under those conditions, not its printed3B04. Its030B3380 aid is
not imported as generic prerequisite; reconcile VOD ownership/previous settings
before any such diagnostic action. BMCR15 is PCS reset, not PHYRCR hard reset.

BIST uses15-bit PRBS and independent generator/checker paths. Admit legal
loopback/fixture, safely set packet/IPG, record baseline capture, select mode,
observe finite window, capture error count, stop both paths and restore settings.
Retain lock, sync-loss, generator, coverage, capture failure and wrap/saturation
independently. RH sync-loss is hardware-set/cleared, not RC; neither explicit
clear nor complete interval-history retention is established. Lock, running
packets or zero after possible wrap cannot alone establish PASS. Raw finite
results can exist while pass criteria and capture lifecycle remain gated.

Manual cable test requires disconnected/quiet partner, no competing
BIST/loopback/reset and disabled autorun while setting finite segment/channel
iterations. Preserve threshold defaults absent exact-part tuning evidence.
Set CDCR15 once, retain acceptance/uncertainty, then poll with released lock.
Freshness needs observed post-trigger start/busy or done0 followed by
start0/done1, or stronger exact-part correlation evidence. If very fast completion
hides progress, report freshness unknown rather than reuse reset/previous done.
No diagnostic completion MAX is provided.

Retain completed request/event generation, fail/status and per-word validity
for0180/0185/018A. Failed later reads preserve prior words but invalidate the
complete composite. Include segment/channel/average/DC context and reset identity.
Do not decode reserved banks, import DP83825 zero-amplitude/no-fault rules or
claim metres, volts, polarity/fault classes or active-link cable diagnosis.

Autorun CR1bit8 enables TDR on link-down; enable is not a trigger or sample.
Link flaps may replace results; before/after equal status cannot prove atomic
capture or no intervening run. Exact latching/retrigger behavior remains needed.
Cancellation stops primary work between callbacks and retains partial results.
Cleanup disables start/autorun, verifies controls, restores persistent settings,
then persistent autorun. Clear start alone is not an acknowledged analog abort.
Keep unknown quiescence/cleanup failure separate. Reset invalidates the generation
and requires finite identity/strap/configuration verification and replay.

## Sibling evidence and acceptance

Read-only sibling HEADs were clean: TMP1x2
`bafc34067530eb340e55c42dae3fad4eb795b6e9`, PCA9555
`498178e4d9e4acb6fab7d8b816ad5021efe8806a`, MB85RC
`1bf7c293549b1038b2d8f3b7a8d655fa74201e6e`; their AGENTS were reopened.
TMP1x2 `include/TMP1x2/TMP1x2.h` OperationResult/OperationSnapshot and
`examples/common/Tmp1x2CliDiagnostics.cpp` result consumption support retained
correlation. PCA9555 `include/PCA9555/PCA9555.h` OperationResult,
`src/PCA9555.cpp` POINTER_PARK/pollOperation and `test/test_basic.cpp`
cancel/deadline/cleanup-failure cases establish separate primary/cleanup status
and partial validity. MB85RC `include/MB85RC/MB85RC.h` TransferResult and
`src/MB85RC.cpp` reconciliation/result sites preserve original uncertainty.
Relevant runner timeout/identity paths in TMP1x2 `tools/hil_tmp1x2_runner.py`
and MB85RC `tools/hil_runner.py` were reopened; broader review is the lead's.
Adopt retention/validity/owner policy; adapt to whole MDIO transactions; reject
sensor/memory timing or pointer parking as PHY facts.

Host cases: independent matrix/segment literals; prohibited combined/speed
loopbacks; counter echo prevention; saturation/wrap; stale reset done/missed busy;
partial reads; accepted-trigger cancellation; ambiguous capture; finite primary
and cleanup expiry; autorun retrigger; reset generation; opaque restore failure.
These are source-oracle cases, not executing PHY behavior.

Physical acceptance needs identified DP83826I board/revision, firmware/runner,
MDIO trace, clocks/interface, MAC/partner payload counters,100-ohm termination
and calibrated segment/open/short/cross fixtures with quiet partner. Cover both
10/100 and MII/RMII where legal. Exact TI clarification or identified measurements
must establish calibration, counter latch, result freshness and abort behavior.

Fragment:22 ordinary registers,three recipe-only records,10 procedures,five gates,
four write actions and independent vectors. Local fragment partition/reset checks:
PASS. Lead integration owns ledger/unit/reference/diff checks and final Git sync.
Host driver, portable/package consumer, SDK link, remote CI and physical:
NOT RUN in this source-only review. Remote CI remains reserved for prompt71.

## Independent event-review cross-check

After integration, independently compared the event review's nine-register
matrix and five procedure contracts against original PDF pages64-70 and92,
rendered directly in memory, plus previously inspected72/73/81. PASS: PHYSCR
reserved04E0; MISR RCFF00/RW00FF; FCSCRRC00FF; RECRRCFFFF with unresolved width;
10BTSCR polarityR0010 with explicit destructive-read prose; RCSR
reset0041/knownFF5F/reservedFE40; RXFSreservedEF1E/RCH00E1; MSErawFFFF.
Collector preserves partial and late-success consumed words, distinguishes
selector failure from possible target consumption, retains source/reset epochs
and avoids destructive retry as proof. Calibration deassert/reset adjacency,
trigger neutral0, retained primary error and finite owed cleanup are consistent
with table8-47. No blocking numeric or procedure defect found. Suggested a
wording correction to clarify that retained host history survives reset until
explicit consumption/acknowledgement; the lead owns that shared contract edit.
