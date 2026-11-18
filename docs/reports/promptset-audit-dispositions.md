# Independent promptset audit: current dispositions

The [2026-11-17 follow-up addendum](#follow-up-addendum-2026-11-17) supersedes
the named rows below. Historical table rows are preserved for traceability.

Originally reviewed 2026-11-15 on `main`, starting from clean, synchronized
`f576dee3b067e16f51e3a8e51d9ffe17b35924ab`. This review checks the
[remaining-prompts audit](../prompts/REMAINING_PROMPTS_AUDIT.md), including all
172 G findings, M-1, N-1 through N-6 and the 18 R contract-review mappings.
The earlier [correctness](prompt-correctness-audit.md),
[workload](prompt-workload-audit.md) and
[public-source](public-library-data-audit.md) reports were also checked for
requirements not covered by those IDs. Historical reports remain evidence of
their dated baseline, not instructions to undo subsequent implementation.

The user authorized prompt corrections, small direct code fixes, and assignment
of larger changes to existing or new prompts. Existing owners suffice; numbering
remained 1-80 and that maintenance left completed stages at 1-20.

Revalidated 2026-11-17 at synchronized `main`
`77c544f415edb26a0e23de76591a0163b9b73147`. The per-ID tables below now describe
current code and prompts through the supported stage 39 subset. The original
review method, corrections and verification sections retain their dated evidence;
the [current response](#current-code-response--2026-11-17) records this pass.
Future-stage requirements and unresolved source/physical gates remain open.

## Review method and evidence

Three disjoint reviewers checked actual files: inherited capable models reviewed
core/state/transport and SDK/tooling; `gpt-6-astra` with high reasoning reviewed
silicon and family differences. Reviewers initially had read-only scope. The
SDK reviewer later implemented only `test/forced.cpp` under an explicit disjoint
assignment. The lead owns production changes, shared contracts, this record,
verification and Git. No sibling, vendor payload or SDK file was modified.

The core review reopened implementation, headers, registered tests and original
SNLS647I access/reset tables. The silicon review reopened archived PDF text with
PyMuPDF, including SNLS647I, SNLS783A, SNLS638C, SNLS505H, SNLA428, SNLA423A,
SNLA437A and SNLA261A. It did not claim fresh visual inspection of every figure.
The lead visually checked original SNLS647I table 8-19, pp54-55, for the changed
BMCR behavior and checked the SNLA428 section numbering. The archive identity
checker protects all 29 original payloads and 27 extracts.

SDK findings were checked against the actual installed unmodified checkout
`C:/esp/v6.1/esp-idf`, commit
`fff9895c82d744c7237be8847347bdd1b07c6643`, especially
`components/esp_eth/src/esp_eth.c`, `src/mac/esp_eth_mac_esp.c`, public headers,
and logging/input paths. Those inspected SDK files retain Apache-2.0; none was
copied into this project's MIT implementation. The board snapshot was compared
with the read-only board document at `ee16c0ed32356cd87e8efec2d6fd5aa5988d2096`;
this is not a fresh netlist/electrical qualification. Relevant FieldCore/TMP
example paths were checked for existence. This maintenance changes no sibling
convention and does not claim a new implementation-stage sibling audit.

Evidence abbreviations below: Pn means the numbered file in
[the prompt index](../prompts/README.md); AC is [API contracts](../api-contracts.md),
DD is [design decisions](../design-decisions.md), COV is
[feature coverage](../feature-coverage.md), RV is
[register vectors](../register-vectors.md). Ledger means
[dp83826i.json](../registers/dp83826i.json). Source tables are in the original
[TI archive](../reference/README.md). Code/test paths are repository-relative.

Dispositions distinguish **implemented**, **planned** (correct future acceptance,
not shipped), **corrected here**, and **superseded**. A source-implied decision
remains an inference, with its physical gate; passing host tests does not promote
it to measured silicon behavior.

## Corrections made

- **G4-4, current code:** BMCR's software/pin OR was copied by
  `BMCR_PRESERVE=0x4CFF` in `DP8382xConfiguration.cpp`. Live preflight now rejects
  a high power-down indication with `INVALID_CONFIG/POWER_STATE` before profile
  writes, retaining the raw word. A low preflight proves software request zero
  under exclusive ownership; the final atomic RMW excludes bit 11 (`0x44FF`),
  so a later pin assertion cannot latch itself into software intent. Neither
  rejection alone nor blindly clearing bit 11 would suffice. The shared reset
  replay retains any earlier effects/barrier on rejection. P28 now explicitly
  owns extension of this existing writer to full software-power intent.
- **G6-1/G6-8, planned code:** the old native teardown sequence disclosed a
  known RX lifetime race yet still ordered driver destruction. DD/P37/P42 now
  require the `native_rx_quiescence` gate before freeing any referenced storage.
  Until a public-API quiescence route is demonstrated, close admission, keep
  driver/glue/MAC/PHY/netif/context alive and require restart. A scheduled
  pre-callback RX test is required. This is a software lifetime gate; Q13 stress
  or a linked image cannot substitute for it. Independent native work continues.
- **G7-8:** CLI discovery must enforce shared clock/silence/quarantine admission.
  A cached valid deadline is compared with a fresh conservative raw timestamp;
  the cached flag alone neither stays busy forever nor admits an invalid reset
  anchor. P44-46 and CLI_CONTRACT require expiry/fault/rebind regressions.
- **G11-2/G11-7:** P21 now distinguishes managed restoration, which can reset
  the failure streak, from raw/session-stop/release activity, which cannot.
  Neither may erase the earlier result or last error.
- **G10-3:** P69 preserves the current six-field metadata schema and owner
  `authors`. P77 follows `main` until version 0; COMMON/index stop instructing
  a blind replay of already-applied pre-13 work packages.
- **G3-2/G4-2/G5-1:** corrected SNLA428 algorithm citations to section 2.2,
  removed invented LPI readiness/dwell claims from COV, and narrowed the stale
  COMPT gate to the still-unresolved fields/100M path.
- Clarified exact RX pin names, actual SDK header dependency closure,
  CONFIG_LOG_VERSION_1 versus console transport, and every-field runner mutation
  acceptance. README, CHANGELOG, public Doxygen, API/user/test guides and progress
  are updated. Version and supported part set are unchanged.

## Findings G1 and G2: primary core

| ID | Disposition and current evidence |
| --- | --- |
| G1-1 | Superseded by explicit user P13 scope: acquireEvidence, modifyInterruptEnables and modifyWakeControl are real consumers of the single collector. Removing them would break current intent. |
| G1-2 | Implemented: pollOperation always applies four-frame VOD 030E=4A40 then verifies FFC0; configuration adds no duplicate. operations/reset/forced tests exercise replay. |
| G1-3 | Implemented: _anchorQuiet/_quietAt enforce persistent HARD floor after attempted writes, including cancel/take; reset tests cover two PHYs. Q5 remains physical. |
| G1-4 | Superseded catalog-size advice; consistency implemented. User requested complete named fields, checked against the ledger by test_access_policy.py. No second factual authority. |
| G1-5 | Corrected: P13/ENGINEERING distinguish W0C, W1S, self-clearing R/W and description-only read effects. No invented W1C field; SNLS647I table 8-18. |
| G1-6 | Implemented: identity methods and two-frame staged _readIdentity, partial diagnostics and OP14 guards; identity/reset tests. |
| G1-7 | Implemented: EXTERNAL_POWER_ON distinct from EXTERNAL_RESET; enhanced POR-only strap distinction and silent reset tests. |
| G1-8 | Implemented 33: owned interface replay uses the RCSR collector in DP8382xInterface.cpp; interface_replay/collector cover it. Unowned initialization does not write RCSR. Current initialization-guide baseline wording corrected in this response pass. |
| G1-9 | Implemented 25/35: cable preflight rejects live CDCR 14; explicit calibration recovery clears 14 then HARD with retained debt. DP8382xElectrical.cpp CAL_PAIR and electrical_calibration/lifecycle; table 8-47. No calibration-complete claim. |
| G1-10 | Corrected: hard_reset_straps retains the table 8-48/8.4.1 ambiguity, possible resample and Q5. |
| G1-11 | Corrected: P15/P16 anchor external readiness to final supplies and clock-qualified reset; no inferred PCS completion maximum. |
| G1-12 | Implemented through G11-20: no token-exhaustion machinery or unused legacy status details. |
| G1-13 | Implemented: literal reset commands, separate already-active checks, independent reset traces. |
| G1-14 | Implemented: abandonCleanup, invalidation, snapshot/take/cancel/end/bind; operations/reset lifecycle tests. |
| G1-15 | Implemented/source-qualified through G4-3: modifyWakeControl harvests MISR2/RXFS, skips stable no-op writes and uses W0C. Q3 remains open. |
| G2-1 | Implemented: DP8382xLink.cpp resolves forced mode from BMCR, gates 100 half on known-zero extension, retains raw PHYSTS; link forced tests/Q11. |
| G2-2 | Implemented: typed NextPageExchange and finite Clause 28 service; negotiation tests, Q10 remains physical. |
| G2-3 | Implemented: exchange-local NP, CLEAR_NP without restart on completion, immediate failure/barrier and NP0 replay. |
| G2-4 | Implemented: forced robust required, manual+robust legal, automatic preserves PHYCR 14; table 8-28 and forced matrix. Part relocation planned 56. |
| G2-5 | Implemented: extension opt-in without invented peer evidence, CR2 mask 0020, forced 10 rejection; parallel full resolution has invalid pause. |
| G2-6 | Implemented: forced profiles reject AN intent and bypass ANAR; explicit complete AN profile on return. |
| G2-7 | Historical boundary satisfied: P17 scopes pre 18 trust; current P18 applies/verifies crossover and extension. |
| G2-8 | Implemented: advertisementWord sets selector 1; negotiation tests overwrite non-default selector. |
| G2-9 | Implemented through G1-2: unconditional initialization VOD. |
| G2-10 | Implemented: coupled CR3 mask 0060 and matching swap settings, table 8-30; forced vectors. |
| G2-11 | Implemented: IEEE 28B-3 pause plus highest-common full-duplex qualification and PHYCR cross-check; link pause tests. |
| G2-12 | Implemented 20/27: one collector, owned selector-at-consumption and retained ambiguous crossing history; ANER/BTSCR/RXFS mirror order preserved. |
| G2-13 | Implemented raw evidence, physical gates retained: RECR arithmetic absent, RCSR polarity unknown; events/access tests, Q16/Q17. |
| G2-14 | Implemented through G11-22: consistent snapshot names. |
| G2-15 | Planned 22: startVerify has no mutation or escalation; explicit initialization replay and OP9 barrier. |
| G2-16 | Planned 23: loopback restoration/crossover masks and tests moved to actual temporary-session owner. |

## Findings G3-G5: diagnostics, power and electrical controls

| ID | Disposition and current evidence |
| --- | --- |
| G3-1 | Implemented 25, independent literal tests: SNLA428 table 2-1 six runs/gains, channel A, exceptional GENCFG and HARD/replay; ledger tdrRecipe. |
| G3-2 | Implemented 25: matching-run threshold/formula, whole-word sign, NO_PEAK and MATLAB conflicts. Corrected section 2.2 citation here; tables 2-4/5. |
| G3-3 | Implemented 23/24 (physical Q8 NOT RUN): script-only reverse/analog restart retained as conflict; 030B3380 unadopted outside table 8-79 range; SNLA423A pp18/20. |
| G3-4 | Implemented 27: both MISRs, bounded morePossiblyPending, retained counter requests and no dependence on a new edge; Q9 unmeasured. |
| G3-5 | Implemented 25/26: CDCR and result words 0180/0185/018A are non-destructive; tables 8-47/61/66/71. |
| G3-6 | Implemented 26: autorun disable/readback is policy, not quiescence; one autorun cannot stand in for six-run interpretation. |
| G3-7 | Implemented 27 through G2-12: dependent mirrors and exact-part RXFS selector evidence. |
| G3-8 | Implemented 27 with one IRQ mux writer: DP8382xInterrupts.cpp PIN_MUX/PIN_APPLY/PIN_VERIFY save, apply and restore LED1/2 mux 4. P32 owns LED profile/dependency validation; P27 wording corrected here. LED0/3 read-only and ENHANCED-only OE semantics remain. |
| G3-9 | Corrected: ledger/test owners interrupt 27, autorun 26, TDR calibration 25; COV agrees. |
| G4-1 | Implemented 29 under source-implied D2: DP8382xEee.cpp and eee_* tests execute both full-word self-checks, exact recipe and shared reset/replay. Three safe raw tuples remain separate from destructive collector tuples; Q1 remains open. |
| G4-2 | Implemented 30: startLpi/stopLpi share the EEE owner, bounded bit 12 updates and readback, application dwell and uncertain effects; lpi_* tests. SNLS647I 8.3.4 supplies no readiness event or bounded dwell. |
| G4-3 | Implemented 31: DP8382xWake.cpp uses exact 826 W0C, harvest-before-write and stable no-op suppression; wol_vectors/routing/acquisition. Family write-1 recipes remain comparison evidence; Q3 is open. |
| G4-4 | Implemented 28: DP8382xPower.cpp and all BMCR writers use owned software intent, separate pin-OR observation and fail-closed preflight; power_* and forced_power_pin tests. The original correction remains valid; Q6 is open. |
| G4-5 | Implemented 31 for 826: DP8382xWake.cpp packing and asymmetric wol_vectors match SNLA261A/table 8-91 onward. A59/82267 remain planned; identical wire words do not imply identical control semantics. |
| G4-6 | Implemented 31: reset 108X is not disabled WoL; raw validity and observed enabled state remain separate. DP8382xWake.cpp and wol_vectors/acquisition preserve unknown bit 0 and neutral explicit disable. |
| G4-7 | Implemented 28: PHYSCR 15 separate PLL control, IEEE-down/clock-loss admission, live pre-reset preparation and control verification; physical Q6 remains open. |
| G4-8 | Implemented 30: EEE owner uses exact 826 bit 12 only, not 825 bits 14:13. lpi_* tests establish software effects; Q2 line observation remains NOT RUN. |
| G5-1 | Implemented 35: DP8382xElectrical.cpp accepts all fourteen COMPT10M patterns with literal vectors; electrical_traces/admission. Upper bits and 100M 0428 remain source-gated under compt_reserved. |
| G5-2 | Implemented 34: DP8382xQuality.cpp owns tx10/tx100 fields, squelch/FIFO/preamble and collector-routed 10BTSCR. quality_vectors/collector/replay verify the source-backed fields. |
| G5-3 | Implemented ownership: link recovery in DP8382xQuality.cpp 34 and calibration in DP8382xElectrical.cpp 35; ledger/COV owners match; no automatic recovery escalation. |
| G5-4 | Implemented 34: zero-cause RMII disable/FLDS remain legal, nonzero RMII causes reject before I/O, unknown CR3 reset stays unknown; quality_admission/collector. |
| G5-5 | Implemented 34: literal MLT3/descrambler settings and conditional maxima in quality procedures/tests; unsupported threshold overrides remain source-gated. No unconditional hardware deadline. |
| G5-6 | Implemented 35: electrical sessions compose owned PHYCR and BISCR 701F/7001, retain forced robust policy and do not copy script reserved bits. electrical_traces; Q19 remains open. |
| G5-7 | Implemented 32, gate retained and rechecked in audit 32-33: PHYCR 5 exists but semantics incomplete, outside typed profile; Q12. |
| G5-8 | Implemented 35: electrical sessions reject incompatible power/isolation and restore persistent settings through the shared finite HARD/replay path. electrical_lifecycle; no entry-preservation promise across reset. |
| G5-9 | Implemented 35: CAL_PAIR clears CDCR 14 with 15 neutral then issues HARD; result reports reset completion and retained cleanup debt, never invented calibration completion. electrical_calibration/lifecycle. |
| G5-10 | Implemented 32 and audited 32-33: per-mode mux/polarity and live RCSR admission; LED3 MII/read-only and pin 31 code conflicts remain gated under tables 8-76/77 and pin_fields. |
| G5-11 | Corrected: P35 directly requires 24; other originally cited omissions were transitive, not blockers. |

## Findings G6-G7: native owner, CLI and runners

| ID | Disposition and current evidence |
| --- | --- |
| G6-1 | Implemented 37-39 public SDK route, with safe terminal retention. The original proposed destructive order is rejected: callback count/fence cannot close the pre-callback RX window. Phy.cpp retire/destroy and raw/glue RX tests enforce native_rx_quiescence. |
| G6-2 | Implemented 37-39: test/esp_idf compiles hash-checked real driver/glue and 45-file selected SDK dependency closure at 1000/100Hz; 63 current host cases and actual P4 fixture link. Seven hand-picked headers/shims are not the evidence boundary. |
| G6-3 | Implemented 37: PhyHandle wrapper, Phase/journal, local installer callbacks, reset/pwrctl rejection and terminal admitted controls in Phy.cpp/PhyControl.cpp; no nested operation owner. |
| G6-4 | Implemented 37: static raw read/write hooks reject unconditionally; real pinned_driver_raw_ioctl/native_raw_routing tests observe zero MAC frames. |
| G6-5 | Implemented 38: even owner-task raw ioctls reject; native typed owner calls core policy/direct MAC batches. Native boundary/arbitration tests cover every MMD/RMW frame and second PHY. |
| G6-6 | Planned 42: TX buffer borrowed/queued not transmitted, RX free on every return; pinned esp_eth.c input/transmit paths. |
| G6-7 | Planned 40 pins correct; clarified board snapshot RX signal names here, with final-netlist caveat. |
| G6-8 | Unsafe original deletion proposal superseded. Implemented 37 retained resources and scheduled raw/glue RX capture tests protect the pre-handoff window. P42 must retain the same gate; a hypothetical one-buffer leak does not describe a safe lifetime. |
| G6-9 | Implemented 37: PhyHandle first-member esp_eth_phy_t, static_assert and owner pointer; no generic base header or storage. source-code.md former reuse advice corrected in this response pass. |
| G6-10 | Implemented 39 conformance and conditional timing table in esp-idf.md; Phy.cpp uses no base reset/AN loop. MAC nominal loops are not preemption-proof bounds; native_wall_time remains unqualified. |
| G6-11 | Implemented 37/38: set_addr accepts only the configured value; native_forbidden_lifecycle covers retarget rejection. Installed deletion is still gated, so delete/recreate is not an available installed retarget shortcut. |
| G6-12 | Implemented 39: factory-selected P4 eventLost versus generic publication uncertainty, terminal receipt and explicit event retry. native_lost_link_event/notification_* tests preserve accepted fields without replay. |
| G6-13 | Planned future SDK re-audit through G6-1: d2ed96d2 may change timer fence; it does not resolve RX lifetime by itself. |
| G7-1 | Planned 44: ID-only finite explicit range, strap defaults, no selection/broadcast inference; all-one/zero evidence not physical absence. |
| G7-2 | Planned 47: bounded whole-line log queue. Corrected CLI_CONTRACT to separate CONFIG_LOG_VERSION_1 from USB/UART transport. |
| G7-3 | Planned 43/48: boot/request/nonce, one outstanding request and passive resync, no hidden reconfigure. |
| G7-4 | Planned 48/52: exactly txt/json artifacts, hash/length, atomic finalization and worst-case verdict. |
| G7-5 | Planned 43/48: real-core CLI fixture and proper dependencies; fake protocol producer alone insufficient. |
| G7-6 | Planned 44/47/53/55: one COV matrix with field-owner tests and future CLI columns, not a false current completion claim. |
| G7-7 | Planned 43: static command/config tables, no dynamic registry. |
| G7-8 | Corrected planned 44-46: use shipped bound-core readIdentityAt or native discover, not a second probe binding. CLI_CONTRACT retains aggregate budget, shared admission, exact quiet-floor expiry, receipt freshness and cumulative accounting. No direct transport bypass. |
| G7-9 | Planned 49: finite serial-process join and explicit inconclusive result, no unbounded flush/drain/retry. |
| G7-10 | Planned 50-52: systematic evidence mutation. Clarified P50/51 acceptance to every required field in turn. |
| G7-11 | Planned 43: freeze grammar/schema only; later stages append executing commands. |
| G7-12 | Planned 43/47: column-zero CRLF records, parser strips nothing; bounded console ownership. |
| G7-13 | Planned 53/54: map named existing tests and add gaps/cross-entry cases, not another duplicated suite. |
| G7-14 | Planned 47/49/50: console/baud/DTR/RTS, minimum partner and IRQ-blocked versus polling-eligible fixtures explicit. |

## Findings G8-G9: later exact parts

| ID | Disposition and current evidence |
| --- | --- |
| G8-1 | Planned 56/57: suffix-identical software records shared, declared part/temperature/physical evidence separate. |
| G8-2 | Planned 56: PartTable and map-keyed policy/robust/VOD, no premature family support; P60/61 exceptions explicit. |
| G8-3 | Planned 57-59: listed A field/default/27-row EEE deltas assigned explicitly; A tables 8-86/90/91 and 8.3.3.1. |
| G8-4 | Planned 61: conservative consuming Register_12/MISR/FLDS, conflicting R type versus clear prose and unknown latch relationship retained. |
| G8-5 | Planned 60: declared RMII role, SOR1 and collected RCSR, no role write; 825 table 7-26/strap evidence. |
| G8-6 | Planned 60/61: Leader clock-loss opt-in for mux/isolation/DPD; source versus inference and Q15 separate. |
| G8-7 | Planned 61: typed mode recipes/allowlists, no 826 VOD on 825, conflicting bypass descriptions retained. |
| G8-8 | Planned 61: SecureON exact-address source exception under SNLA261A, no generic raw access; Q15. |
| G8-9 | Planned 57/58: conditional Strap11, Strap8 conflict and 040F preserving mirror; SDAA067A/A datasheet. |
| G8-10 | Planned 56 through G8-2: robust rule part-keyed, not imposed on 825. |
| G8-11 | Primary 826 packing implemented 31 through G4-5; A59 and 822 67 remain planned, with per-part acknowledgement/CRC semantics. |
| G8-12 | Planned 60: conservative 20MHz and 24MHz prose conflict, not a claimed resolution. |
| G9-1 | Planned 65: SOR1 encoded FX/SD sets mapped explicitly; 822 tables 7-10/8-84. |
| G9-2 | Planned 65: stale FX evidence and maintenance refresh, no interrupt-driven invention; SNLA437A 2.3.4 and Q14. |
| G9-3 | Planned 62/63: five literal reset namespaces/domains, strap conflict retained rather than ambiguous reset labels. |
| G9-4 | Planned 67: GENCFG 000A exception, optional WoL aid/neutral composition/restart and PTP ownership. |
| G9-5 | Planned 63/65: no crossover writes in FX, mode UNKNOWN, part-keyed robust rule. |
| G9-6 | Planned 64: sourced 4300 transition, owner checks, inferred reapplication/reverse route; 822 tables 8-23/81, physical pending. |
| G9-7 | Planned 62/67: explicit separate namespace and 0007/003D/4007 trace; 822 7.4.2.9. |
| G9-8 | Planned 67: 822 W1S acknowledgement 0800, neutral zero on other writes and CRC preservation; not 826 W0C. |
| G9-9 | Historical incorrect reset claim corrected: 822 RXFS 12 resets 0 versus 826/825 1; table 8-88/P63/P67. |
| G9-10 | Planned 67/gated: Bit Mask Flag has no defined semantics; preserve, no invented setter; 822 8-87. |
| G9-11 | Planned 66: seven-bit amplitudes and SNLA253 scale conflict retained; 822 8-60..64. |
| G9-12 | Planned 66: start clear AND Done, initial Done 1 test; this freshness minimum is not physical qualification. |
| G9-13 | Planned 64: GPIO1 clock field and inferred SD conflict; 822 8-81. |
| G9-14 | Planned 65/66: Fiber Far-End Fault and FX swing explicitly included. |
| G9-15 | Planned 62/64: fixed RGMII shifts distinct from MII TXCPSR, field-width conflict retained; 822 8-36. |
| G9-16 | Planned 65: FIBER_GENCFG 0465 distinct from GENCFG 0456. |
| G9-17 | Planned 62/64/65: MAC versus medium qualified separately, FX+RGMII not stated/physical pending, correct source sections. |
| G9-18 | Planned 65: medium enum, per-medium capabilities, observed medium, CLI and adapter admission assigned. |
| G9-19 | Planned 62/67: shared 822 map with exact temperature/fiber capabilities and separate physical evidence. |

## Findings G10: packages and verification

| ID | Disposition and current evidence |
| --- | --- |
| G10-1 | Implemented provisioning and 37-39 link: scripts/idf.cmd/idf.ps1 select exact installed fff9895c SDK without global environment changes; actual adapter_link P4 fixture exists and links. |
| G10-2 | Planned 71: push/PR/manual triggers and exact commit run discovery, no pre 71 dispatch/status check. Main policy makes old merge concern moot. |
| G10-3 | Corrected here: six actual fields including authors, matching generate_version.py FIELDS/AUTHORS; P69 preserves ownership. |
| G10-4 | Planned 70/77: PlatformIO explicitly unsupported this execution, no unnecessary wrapper. |
| G10-5 | IDF source selection implemented 37 in root CMake before project(). Portable install/export/version-config work remains planned 69; a present missing install is not an earlier-stage defect. |
| G10-6 | Planned 69/70: reuse P10 tests, add new outputs/CLI rejection, missing-Git fixture belongs 70. |
| G10-7 | Planned 69/72-80: explicit CONTRIBUTING owner, stage reports/tested SHA, no nonexistent validation.md/manifest requirement. |
| G10-8 | Corrected: progress/index and AGENTS/VERIFICATION list runnable commands; historical baseline counts stay historical. |
| G10-9 | Local GNU native UBSan trap already executes 63 cases; second compiler/other sanitizers remain separate 71 responsibilities. P55/68/71 preserve honest toolchain scope. |
| G10-10 | Planned 68: real two-instance application separate from registered assertions/tests-OFF consumer. |
| G10-11 | Planned: one matrix, gap-based 53/54, sampled 75/78, delta 79, changed-input 80; sibling audit rule retained. |
| G10-12 | Planned 72/73/80: physical BLOCKED/NOT RUN and final set PARTIAL, even if final report work completes. |
| G10-13 | Corrected: gitignore SDK/dependency/log patterns and fixture exception present; revisit if committing sdkconfig.ci or registry locks. |
| G10-14 | Planned 69-71: directory export-ignore, HEAD tar.gz, containment/local-path validation, extracted consumer and first-Linux failures. |

## Findings G11-G13: contracts, structure and access

| ID | Disposition and current evidence |
| --- | --- |
| G11-1 | Implemented 37-39 public unmodified SDK integration and terminal retained resources; native_rx_quiescence still blocks reusable installed destruction. No fork or private cache patch is required. |
| G11-2 | Implemented OP9 barrier/no CLEANUP engine; corrected P21 restoration accounting drift here. |
| G11-3 | Implemented: abandonCleanup/external disposition, no QuiescenceDisposition or RETIRE_REQUIRED machinery. |
| G11-4 | Implemented: clockLagUs quantization/source latency at read instant; post-callback upper wait anchor. |
| G11-5 | Implemented: short maxFrames yields before complete batch; no split or premature operation timeout; operation/reset/negotiation tests. |
| G11-6 | Implemented: admitReset is single reset look-ahead, ordinary admitBudget; no generic reserve-plan engine. |
| G11-7 | Implemented 21: _execute classifies managed health from backend outcome before local clock/deadline publication. Clean late success counts transport success; health_* tests preserve lastError and results during restoration. |
| G11-8 | Implemented: _yieldOperation counts no-batch/no-time-progress only; frozen/coarse clock regressions. |
| G11-9 | Implemented through G1-3: persistent HARD silence. |
| G11-10 | Implemented through G1-2: mandatory VOD. |
| G11-11 | Implemented current OP14 subset in _execute/_checkMutation/collector/link; pending result reads versus reset WAIT/write denial explicit. |
| G11-12 | Implemented 15-22: startInitialize(reset) and startVerify use one shared procedure/slot, with no reset/recover aliases; operations/reset/verify tests. |
| G11-13 | Implemented BatchSummary/result-free snapshot; later next-page result fields have real consumers, not duplicated receipts. |
| G11-14 | Implemented: three positive transport bounds and widened admission arithmetic. |
| G11-15 | Implemented 21: all stored health/evidence times use microseconds with validity; healthSnapshot has real state, not a future placeholder. |
| G11-16 | Implemented private exact revision table, no public RevisionPolicy; future part table 56. |
| G11-17 | Corrected COMMON authority/read map; historical audit directions now point to this current record. |
| G11-18 | Corrected: shipped declaration links and normative OP section. Reject arbitrary line-count quota as a present defect. |
| G11-19 | Corrected ledger owners/OperationBudget/cleanupOwed and 13-frame MMD description; old duration members absent. |
| G11-20 | Implemented: no unused legacy details or practical token-exhaustion branch. POWER_STATE added here has a current tested consumer. |
| G11-21 | Corrected through G10-8: current progress/checks distinct from historical baseline. |
| G11-22 | Implemented consistent snapshot names, uint64 overflow/regression/latch checks instead of modular-wrap promises. |
| G12-1 | Implemented one REGISTER_POLICY, refused rows/preserve masks/static consuming-RMW guard; ledger consistency tests. |
| G12-2 | Implemented common Check/FakeController for shared transport/MMD semantics. Distinct feature wire models remain justified independent oracles, not a mandate for one enormous fake. |
| G12-3 | Implemented admitBudget/auditFrames/classifyReport. Additional current collector work is legitimate;47-line target is not a requirement. |
| G12-4 | Implemented checkMmdCall/directBatch/mmdBatch validation/order helpers. |
| G12-5 | Implemented reusable+quarantine RELEASE_REPORT rejection, stopped evidence and subsequent refusal regression. |
| G12-6 | Implemented removal of misleading transport lastStatus/lastBatchValid; typed evidence statuses have separate real consumers. |
| G12-7 | Implemented stopped in TransportSnapshot, local versus backend quarantine distinguished and tested. |
| G12-8 | Source-qualified: five-frame RMW labeled inferred from 8.3.11.1, not printed; Q4 remains. No unsupported fallback change. |
| G12-9 | Corrected full LEDCR-write Doxygen: four fields, 20Hz/active-low consequence, modifyMmd for one field. |
| G12-10 | Implemented recipe offsets uniformly PROCEDURE/ACCESS_POLICY; EEE DEVAD source policy separately constrained. |
| G12-11 | Implemented ledger/address/field/policy tests and every intentional CTest failure-path registration. |
| G12-12 | Implemented distinct DEVAD/REGCR names, direct examples without discarded receipts or redundant staging. |
| G13-1 | Implemented per-target reserved preservation, FFAF/FFDF literals, LEDCR-specific zero policy; power pin is separately fixed under G4-4. |
| G13-2 | Implemented 35 fourteen COMPT10M actions/tests. Source-gated 100M 0428 remains unavailable; no broad compliance-support claim. |
| G13-3 | Corrected COMMON owner binding, temporary FIFO 23, compatibility 33 and tx10/tx100 34. |
| G13-4 | Corrected calibration citation: tables 8-47/48p73,8.4.1p48. |
| G13-5 | Implemented 31 raw RXFS/SFD-error acquisition. rxfs_sfd_reference remains open because RXFS[7] refers to RXFCFG1[13], which the detailed table marks reserved. No programmable SFD detection or RXFCFG1[13] writes are inferred. Owner 31 replaces the earlier 29 citation; physical qualification remains open. |
| G13-6 | Implemented 34 software FLD policies preserve unknown reset defaults; Q18 remains physical 73 work. quality_vectors/admission/collector cover supported behavior. |
| G13-7 | Corrected RV:6100/4100 are composed examples, duplex is test choice, not printed literals. |

## Additional M/N and contract R findings

| ID | Disposition and current evidence |
| --- | --- |
| M-1 | Corrected: P41/43/68 use separate existing header/source paths; actual named FieldCore/TMP paths checked. |
| N-1 | Implemented 29/30 under source-implied D2: both EEE gates agree, pin/clock conflict preflight before recipe and one collector for adopted fields; Q1/Q2 remain unmeasured. |
| N-2 | Re-pin reflected in P40/DD; clarified board RX signal names here, final-netlist validation still required. |
| N-3 | Implemented: Advertisement has no persistent nextPage; typed exchange and NP0 reset replay only. |
| N-4 | Planned 40/DD6: default-loop handlers bounded/nonblocking, glue has no own timeout; RX lifetime now additionally gates destruction. |
| N-5 | Implemented 25/35: live CDCR 14 rejection before cable writes plus separate explicit clear/HARD calibration route with retained debt; cable_admission/electrical_calibration. |
| N-6 | Planned 62: function 10 REGCR contradiction retained; no-post-increment only, 822 7.4.2.7 versus 7.4.2.1. |
| R-1 | Implemented persistent quiet floor across cancel/take/operations; OP12 external owner duty survives end/rebind. |
| R-2 | Implemented 23-35: reset acceptance/completion remains separate from temporaryMayBeActive; loopback/BIST/LPI/electrical session starts mark possible stimulus at attempted mutation. |
| R-3 | Implemented 22+: startVerify never clears cleanup; retry configuration requires matching owner; owning session stop restores under separate operation. verify_recovery/verify_barrier and session lifecycle tests. |
| R-4 | Implemented silent validated starts/staged tokens across current operations. Admission does not treat passive OFFLINE as an I/O latch; operations/health/session tests. |
| R-5 | Implemented D6 unconditional VOD. |
| R-6 | Implemented 23+: owning stops restore initialized evidence only through complete identity/mandatory/profile replay; DP8382xDiagnostics.cpp and loopback/BIST lifecycle tests. |
| R-7 | Implemented 23+: failed session starts still admit their own stop; loopback/BIST/LPI/electrical lifecycle tests retain possible mutation and cleanup. |
| R-8 | Implemented current direct writes and actual session stop/acquisition entry paths; _checkMutation and owning feature admission preserve the barrier. |
| R-9 | Corrected COMMON/OP9: two-call owner-dwell sessions versus self-contained tests restoring before success. |
| R-10 | Implemented 19/23+: link resolution is suppressed during semantic mutation or possible temporary stimulus; link/session tests retain raw evidence. |
| R-11 | Implemented core quiet floor and native exclusive BusAdmission through result delivery; two-instance/reset/native board-silence tests. Future CLI reuses these routes under corrected G7-8. |
| R-12 | Implemented literal OP6 four-lag inequality, 100Hz/exact-expiry timing tests. |
| R-13 | Implemented maxStalledPolls100000 and finite frozen-clock behavior, advancing ticks distinguished. |
| R-14 | Implemented _pollTimeValid and post-callback tracking; no false first-poll stall. |
| R-15 | Implemented running invalidation freezes result, external-reset updates current cleanup evidence, pending historical result retained. |
| R-16 | Implemented persistent cleanupOwed and actual temporary producers/owning stops; session/restore tests prove failed stop cannot discharge unrelated debt. |
| R-17 | Implemented NONE/RESTART preservation and temporary verification rejection; verify/session/reset tests cover OP13 and independent restoration. |
| R-18 | Shipped API/file migration complete; future features use OP contracts, earlier stage 8 reports remain history. |

## Earlier reports and decisions

The correctness report's nine explicit defects are accounted for individually:
W0C by G4-3/G1-15; consumed aggregate evidence by G2-12/G2-13 and current
collector; prohibited local ANAR fault by configuration/tests; sleep-policy
versus state by P28; MII-loopback conflict by P23/table 8-19; medium versus MAC
by G9-17/18; SDK cache by P38/DD5; raw-ioctl ownership by G6-4/5; narrower SDK
pause by P39/DD5. The report's later family anchors are P57/P59 and P62-67.
EEE inference is now D2 with Q1, not an indefinitely blocked raw tuple. A-generation
EMC remains a source conflict, not a measured software guarantee.

The workload report's split remains valid: 80 complete scopes, no renumbering,
one full-coverage denominator, separate current tests and physical evidence.
Its old new-branch rule is superseded by root AGENTS and corrected in P77.
Its restored health/address/entry requirements remain P21/P37; input-admission
versus live checks remain COMMON/API; sustained physical traffic is P73. Its
token-exhaustion and generic-base simplification advice is superseded by current
OP contracts and bare native wrapper, not a reason to resurrect abandoned APIs.

Public-source report regressions are still covered: reset mask/address confusion
by transport/reset tests; low-first BMSR by link tests; MLEDCR/LED fields and
odd-nibble polarity by ledger/constants/tests; poisoned REGCR and reserved bits
by MMD/SDK poison probes; vendor timing/locking shortcomings by the bounded
transport contract and future native evidence; exact 825 identity by P60. None
requires copying third-party implementation or broadening exact-part admission.

D1 remains unmodified SDK, but its destructive RX order is corrected here.
D2/D3 remain explicitly source-implied EEE and selected TDR exception, with
Q1/Q7. D4/D5/D6/D7/D8 are retained forced/robust/VOD/COMPT/NP decisions checked
above. D9-D11 retain the simpler OP barrier/budget/entry design. D12 now follows
six-field metadata, D13 one matrix and numbering, D14 hard-reset floor, D15
per-target preservation, D16 inferred RMW/W0C qualification, and D17 current
clock/status/snapshot conventions. No physical Q1-Q19 case is marked passed.

## Verification and remaining work

Environment: Windows PowerShell, GNU C++15.1.0/WinLibs, C++17 Release,
strict conversion warnings as errors, active CHECK assertions under NDEBUG.
Final independent core and silicon review found no blocking defect in the
integrated correction. The core reviewer independently passed all nine forced
suites in `build/audit-core-final`; silicon review did not claim test execution.

| Layer / command | Result |
| --- | --- |
| `cmake -S . -B build/audit-promptset -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON`, build, `ctest --test-dir build/audit-promptset --output-on-failure --no-tests=error` | PASS 85/85, including new forced_power_pin and 11 intentional rejection probes; public headers compile separately. |
| Same build/test in `build/audit-promptset-ubsan`, CMAKE_CXX_FLAGS `-fsanitize=undefined -fsanitize-trap=undefined` | PASS 85/85. |
| `cmake -S test/consumer -B build/audit-promptset-consumer -G Ninja -DCMAKE_BUILD_TYPE=Release`, build, CTest | PASS 1/1, tests OFF, real portable library linked. |
| `python -m unittest discover -s test -p "test_*.py" -q` | PASS 103 tests. |
| `python tools/check_doxygen.py` | PASS 157 generated pages and 2 rejected negative fixtures. |
| `python tools/verify_ledger.py` | PASS 97 registers, 52 procedures, 35 features; structural/source-record evidence only. |
| `python tools/verify_references.py` | PASS 29 payloads, 46,886,437 bytes, 27 extracts, 1539 local links in 191 Markdown files. |
| `python scripts/generate_version.py check` | PASS current 0.1.0 generated header; no version change. |
| Disposition inventory comparison | PASS exactly 179 unique G/M/N rows plus 18 R rows, no omissions or duplicate IDs. |
| `git diff --check` | PASS. |

The new power-pin suite covers 55 scenarios across AN and forced 10/100 half/full,
initial software/pin-high rejection, all reset replays, and pin assertion only
at the final atomic BMCR read. Its 12,991 CHECKs pass. The same test source
linked against the saved pre-fix library produced 247 failed assertions, proving
the new cases detect the original behavior. No production test-only hook was
added. These are independent source-derived models, not physical Q6 evidence.

The final maintainability review retained one profile/replay path, one small
live preflight and one changed preservation mask. No new state machine, power
API, registry, automatic retry or alternative collector was introduced. Shared
contracts now point to current dispositions instead of ordering old removals.
Numbered future work stays with existing owners, and the SDK source lifetime
issue is not hidden as a physical-only qualification question.

Native reusable destruction remains BLOCKED on native_rx_quiescence; P37/P42
contain concrete source/test acceptance and preserve independent eligible work.
P28 extends software-power ownership, P44-46 implement the corrected discovery
guard, and other future findings remain at their existing numbered owners.
SDK-link, installed/package consumer, remote CI and physical execution are
NOT RUN in this maintenance. First-execution remote CI remains prompt 71 only.

## Current-code response — 2026-11-17

This pass checked every existing G/M/N/R row against the full original audit,
current code, current owning prompts and source evidence, rather than applying
the historical recommendations mechanically. The tables above are the current
response for all 197 IDs: 172 G, M-1, six N and 18 R. Fifty-two rows now reflect
implemented stages 21-39, the actual IRQ mux owner, retained native RX resources
and the existing discovery route. Historical verification above remains evidence
of its own commit; current checks are recorded in the
[repository review index](current-review-responses.md).

Confirmed maintained-document corrections are the source catalogue's obsolete
generic SDK base recommendation, prompt 27's second IRQ mux writer, initialization's
overly broad no-RCSR/baseline-frame wording, and the future CLI's separate discovery
binding. These now match the actual source and tested owner contracts. No new
production defect was confirmed, and no implementation change is justified merely
to reproduce an old line count, enum name or speculative abstraction.

D1's destructive order and automatic fence-removal suggestion remain rejected;
the current unmodified SDK needs the explicit native_rx_quiescence gate. D2/D3
are still selected source-implied EEE/TDR policies with physical questions.
D4-D11 and D14-D17 match current implementation/contracts; D12 follows the actual
six-field manifest; D13 preserves one matrix and current main-only workflow.
Current OP1-17 and all R mappings have real operation/session consumers. G8/G9
and remaining G7/G10 work stays with future owners, with uncertainty/inference
labels preserved. The old audit's instruction not to re-audit without cause
does not supersede the user's explicit request for this review.

Original PDFs checked include SNLS647I access/reset/IRQ/RCSR/WoL tables,
SNLA428 recipe/derivation, SNLS783A family deltas, SNLS638C role/access/timing
conflicts and SNLS505H FX/reset/RGMII/REGCR/cable fields. The actual unmodified
SDK remains fff9895c82d744c7237be8847347bdd1b07c6643; public source paths and
literal regressions are in the individual source-audit responses. Whole-word
commands, exact-part precedence, validity on failure, one collector and one
transaction owner remain the engineering basis, not a generic best-practice
claim that overrides a datasheet.

Future CLI/runners/packages/CI and family qualification were reviewed as
requirements, not falsely reported as shipped. Source-gated silicon branches,
native wall time and physical Q1-Q19 remain open. This maintenance starts no
new numbered implementation stage and changes no supported part/version.

## Follow-up addendum, 2026-11-17

The supplied follow-up attributed its observations to cb5ffe5 on 2026-11-16.
This maintenance independently checked every item at 955ecf3; it does not
reattribute that older verification to current code. The earlier response missed
the BISCR wire-word defect and duplicated-contract drift. The corrections below
supersede those claims without rewriting the dated table rows.

Commits: **3fe72be** code/regression/helper; **681a10e** contracts/source guides;
**380cd83** prompt acceptance. Each was fetched/pushed/refetched with equal
upstream IDs and zero ahead/behind. The final records commit is in the handoff.
Full item-by-item evidence and checks are in the
[follow-up report](audit-followup-2026-11-17.md).

| Finding / attached observation | Current verification and response | Current owner |
| --- | --- | --- |
| G5-6: BISCR preserve mask omitted RO bit 8 | Confirmed at 955ecf3 despite the earlier implemented claim. 3fe72be changes 8EE0 to 8FE0, retains verify 701F; the RO fake and literal 7101 wire regression fail with the old mask. Original SNLS647I table 8-40 p69 governs. | Fixed; Q19 physical remains 73 |
| G11-18: copied types/feature appendices drifted | Confirmed; remove shipped declaration copies and link headers/guides in 681a10e. Preserve unique decoding/reset-polling rules in maintained guides; remove the incorrect mismatch-priority statement. No arbitrary line quota overrides normative content. | Fixed; future final documentation 76 |
| D10/OP-6 and G11-6: reset called the sole look-ahead | Confirmed;681a10e documents reset including calibration pair and EEE75/71-frame,21/20-callback floors, strict deadline and late-owner limits. | Fixed contract; extraction 53 |
| G12-3: five repeated 12-flag chains; per-kind work in executor | Five exact repetitions confirmed and replaced by one equivalent private helper in 3fe72be. Larger extraction remains assigned by 380cd83, preserving fresh callback-entry resampling, error/effect order and existing regressions. Source-line counts are not correctness targets. | Helper fixed; per-kind admission 53 |
| G1-5: unqualified W1C inventory | No W1C in reviewed four exact-part PDFs;380cd83 removes the unqualified entry while retaining the prohibition on inventing access semantics. | Fixed |
| G12-8: selector persistence stated as proved |681a10e labels the five-frame RMW selection as inferred from SNLS647I 8.3.11.1, not a printed combined sequence; no wire behavior changed. | Physical Q4 remains open |
| G13-5: completed 31 still named as next evidence owner | 681a10e assigns TI clarification and 73 WoL/error fixture; 380cd83 prohibits reserved-bit probing while blocked. Raw RXFS status software is already implemented; this is the reserved RXFCFG1[13] cross-reference, not a timing/scale issue. | TI source clarification; fixture 73 |
| N-2/G6-1/G6-7: stale pins/install/IO_MUX | Management pins and failed-install/installed-retention distinction were already corrected. 681a10e adds all RMII pins, pinned IO_MUX source and removes residual reusable-deletion wording; 380cd83 fixes 40's typo. | Docs fixed; final-netlist/physical qualification still 40/72 |
| G6-13: d2ed96d2 dismissed as unbounded |681a10e explains that blocking stop waits for the active callback; finite wall time depends on qualified callback/backend/scheduler bounds. Future-baseline fence/inactive-stop re-audit and independent RX lifetime gate remain. | Fixed interpretation; future SDK baseline owner 37 |
| G11-13/D17: full by-value result growth |1864B OperationResult measured on GNU 15.1 x64; example 1984B plus local 1864B is 3848B object storage, not measured optimized stack. 681a10e records it;380cd83 assigns compact payload/retained evidence and ABI size ceiling, forbids new embedded feature-result copies in COMMON. | Assigned 76; no current public API change |
| G12-2: duplicated fake/setup harnesses | Current counts are 23 Controller suites and 257 same-line bind CHECKs, not 22/251. 380cd83 requires shared mechanical harness/fixture, documented independent wire oracles and before/after static/executing assertions with no coverage reduction. | Assigned 55 |
| OP-5 scenarios: arithmetic probes are insufficient | Two exact repeated-loop cases still absent. The short-slot DEADLINE test already executes;380cd83 preserves it and assigns the missing BOUND_ADMISSION counterpart plus both real driver-loop cases. | Assigned 54 |

Variant citation, I/E software equivalence with TA-dependent electrical limits,
DP83822 RXFS W1S neutral-zero writes and six-field manifest guidance were also
corrected in 681a10e/380cd83. Future work is assigned, not reported as implemented.
No stage status, supported part, SDK baseline or hardware qualification changed.
