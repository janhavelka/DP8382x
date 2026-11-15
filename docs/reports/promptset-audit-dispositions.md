# Independent promptset audit: current dispositions

Reviewed 2026-11-15 on `main`, starting from clean, synchronized
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
remains 1-80 and completed stages remain 1-20. No future-stage implementation is
claimed by this maintenance.

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
| G1-5 | Corrected: P13/ENGINEERING distinguish W0C, W1S, self-clearing R/W and description-only read effects. No invented W1C field; SNLS647I table8-18. |
| G1-6 | Implemented: identity methods and two-frame staged _readIdentity, partial diagnostics and OP14 guards; identity/reset tests. |
| G1-7 | Implemented: EXTERNAL_POWER_ON distinct from EXTERNAL_RESET; enhanced POR-only strap distinction and silent reset tests. |
| G1-8 | Planned33: current initialization does not write RCSR; P15 explicitly defers collector-routed replay. |
| G1-9 | Implemented25 / planned35: CDCR14 preflight rejection, calibration_recovery owner35, raw CDCR refused; table8-47 requires clear then HARD. |
| G1-10 | Corrected: hard_reset_straps retains the table8-48/8.4.1 ambiguity, possible resample and Q5. |
| G1-11 | Corrected: P15/P16 anchor external readiness to final supplies and clock-qualified reset; no inferred PCS completion maximum. |
| G1-12 | Implemented through G11-20: no token-exhaustion machinery or unused legacy status details. |
| G1-13 | Implemented: literal reset commands, separate already-active checks, independent reset traces. |
| G1-14 | Implemented: abandonCleanup, invalidation, snapshot/take/cancel/end/bind; operations/reset lifecycle tests. |
| G1-15 | Implemented/source-qualified through G4-3: modifyWakeControl harvests MISR2/RXFS, skips stable no-op writes and uses W0C. Q3 remains open. |
| G2-1 | Implemented: DP8382xLink.cpp resolves forced mode from BMCR, gates100half on known-zero extension, retains raw PHYSTS; link forced tests/Q11. |
| G2-2 | Implemented: typed NextPageExchange and finite Clause28 service; negotiation tests, Q10 remains physical. |
| G2-3 | Implemented: exchange-local NP, CLEAR_NP without restart on completion, immediate failure/barrier and NP0 replay. |
| G2-4 | Implemented: forced robust required, manual+robust legal, automatic preserves PHYCR14; table8-28 and forced matrix. Part relocation planned56. |
| G2-5 | Implemented: extension opt-in without invented peer evidence, CR2 mask0020, forced10 rejection; parallel full resolution has invalid pause. |
| G2-6 | Implemented: forced profiles reject AN intent and bypass ANAR; explicit complete AN profile on return. |
| G2-7 | Historical boundary satisfied: P17 scopes pre18 trust; current P18 applies/verifies crossover and extension. |
| G2-8 | Implemented: advertisementWord sets selector1; negotiation tests overwrite non-default selector. |
| G2-9 | Implemented through G1-2: unconditional initialization VOD. |
| G2-10 | Implemented: coupled CR3 mask0060 and matching swap settings, table8-30; forced vectors. |
| G2-11 | Implemented: IEEE28B-3 pause plus highest-common full-duplex qualification and PHYCR cross-check; link pause tests. |
| G2-12 | Implemented20/27: one collector, owned selector-at-consumption and retained ambiguous crossing history; ANER/BTSCR/RXFS mirror order preserved. |
| G2-13 | Implemented raw evidence, physical gates retained: RECR arithmetic absent, RCSR polarity unknown; events/access tests, Q16/Q17. |
| G2-14 | Implemented through G11-22: consistent snapshot names. |
| G2-15 | Planned22: startVerify has no mutation or escalation; explicit initialization replay and OP9 barrier. |
| G2-16 | Planned23: loopback restoration/crossover masks and tests moved to actual temporary-session owner. |

## Findings G3-G5: diagnostics, power and electrical controls

| ID | Disposition and current evidence |
| --- | --- |
| G3-1 | Implemented25, independent literal tests: SNLA428 table2-1 six runs/gains, channel A, exceptional GENCFG and HARD/replay; ledger tdrRecipe. |
| G3-2 | Implemented25: matching-run threshold/formula, whole-word sign, NO_PEAK and MATLAB conflicts. Corrected section2.2 citation here; tables2-4/5. |
| G3-3 | Implemented23/24 (physical Q8 NOT RUN): script-only reverse/analog restart retained as conflict; 030B3380 unadopted outside table8-79 range; SNLA423A pp18/20. |
| G3-4 | Implemented27: both MISRs, bounded morePossiblyPending, retained counter requests and no dependence on a new edge; Q9 unmeasured. |
| G3-5 | Implemented25/26: CDCR and result words0180/0185/018A are non-destructive; tables8-47/61/66/71. |
| G3-6 | Implemented26: autorun disable/readback is policy, not quiescence; one autorun cannot stand in for six-run interpretation. |
| G3-7 | Implemented27 through G2-12: dependent mirrors and exact-part RXFS selector evidence. |
| G3-8 | Implemented27 PHYSCR/pin admission and ENHANCED-only OE power meaning; LED1/2 mux writes remain32, LED0/3 read-only. |
| G3-9 | Corrected: ledger/test owners interrupt27, autorun26, TDR calibration25; COV agrees. |
| G4-1 | Source-implied policy implemented for three safe raw tuples; P29 owns full recipe. EEE_REGISTERS, mmd tests, 825 table7-92, 8227.4.2.9; Q1 remains. |
| G4-2 | Planned30 start/stop/application dwell already correct; corrected stale COV readiness wording here. 8268.3.4 provides no readiness event. |
| G4-3 | Implemented neutral W0C subset; planned31 full wake. 8268-90/A8-91 differ from8228-87; no-op suppression/collector retain evidence, Q3 remains. |
| G4-4 | Corrected here in current profile preflight/atomic composition, with POWER_STATE and forced_power_pin tests. P28 implements owned software intent across BMCR writers and reset replay; table8-19/Q6. |
| G4-5 | Planned31/59/67: identical asymmetric MAC/password wire packing established in prompts/catalogue; labels do not imply reversed words. |
| G4-6 | Planned31: reset108X is not disabled WoL; unknown bit0 stays unknown and explicit disable is neutral; table8-90. |
| G4-7 | Implemented28: PHYSCR15 separate PLL control, IEEE-down/clock-loss admission, live pre-reset preparation and control verification; physical Q6 remains open. |
| G4-8 | Planned30: exact826 bit12 only, no imported825 bits14:13; Q2 measures108B. |
| G5-1 | Planned35: fourteen COMPT exceptions already present; corrected stale nextEvidence here. Table8-50 admits0..D; upper bits/100M0428 remain gated. |
| G5-2 | Planned34: tx10/tx100 fields, squelch bounds/FIFO/preamble and collector-routed 10BTSCR explicit. |
| G5-3 | Corrected: calibration recovery35 and link recovery34 in ledger/COV. |
| G5-4 | Planned34: RMII zero-cause disable/FLDS allowed, nonzero causes reject, unknown CR3 reset retained. |
| G5-5 | Planned34: conditional maxima and literal MLT3/descrambler settings retained; no unconditional detector deadline. |
| G5-6 | Planned35: PHYCR owned fields and BISCR701F/7001, no reserved script copy; forced robust difference/Q19 explicit. |
| G5-7 | Implemented32, gate retained and rechecked in audit32-33: PHYCR5 exists but semantics incomplete, outside typed profile; Q12. |
| G5-8 | Planned35: hard reset destroys state, later finite replay restores it; no impossible entry-preservation promise. |
| G5-9 | Planned35: CDCR14 clear then HARD, reset completion only, not fabricated calibration completion; table8-47. |
| G5-10 | Implemented32 and audited32-33: per-mode mux/polarity and live RCSR admission; LED3 MII/read-only and pin31 code conflicts remain gated under tables8-76/77 and pin_fields. |
| G5-11 | Corrected: P35 directly requires24; other originally cited omissions were transitive, not blockers. |

## Findings G6-G7: native owner, CLI and runners

| ID | Disposition and current evidence |
| --- | --- |
| G6-1 | Corrected here: public SDK route remains, but RX lifetime now gates destructive teardown in DD/P37/P42. Callback count/fence alone cannot authorize free. |
| G6-2 | Planned37/38: actual pinned driver host layer plus P4 link; corrected invented seven-header count to hashed dependency closure. |
| G6-3 | Planned37: bare wrapper, one Phase/journal, reset/pwrctl reject, one terminal callback rather than nested operation. |
| G6-4 | Planned37/38: static unconditional raw ioctl rejection with zero MAC frames. |
| G6-5 | Planned38: even owner-task raw hooks reject; typed owner reaches core/direct MAC batch, no recursive mediator route. |
| G6-6 | Planned42: TX buffer borrowed/queued not transmitted, RX free on every return; pinned esp_eth.c input/transmit paths. |
| G6-7 | Planned40 pins correct; clarified board snapshot RX signal names here, with final-netlist caveat. |
| G6-8 | Corrected here with G6-1: P42 no longer orders freeing despite known pre-callback RX race; scheduled lifetime test required. |
| G6-9 | Planned37/DD2: PhyHandle first member/static_assert, no generic IEEE base dependency. |
| G6-10 | Planned39: real MAC timing bounds, no inherited blocking base reset/AN loops. |
| G6-11 | Planned38/DD2: immutable address, identical set_addr only. |
| G6-12 | Planned39/DD5: factory-specific P4 eventLost handling versus other-MAC uncertainty, no blind repeated UP. |
| G6-13 | Planned future SDK re-audit through G6-1: d2ed96d2 may change timer fence; it does not resolve RX lifetime by itself. |
| G7-1 | Planned44: ID-only finite explicit range, strap defaults, no selection/broadcast inference; all-one/zero evidence not physical absence. |
| G7-2 | Planned47: bounded whole-line log queue. Corrected CLI_CONTRACT to separate CONFIG_LOG_VERSION_1 from USB/UART transport. |
| G7-3 | Planned43/48: boot/request/nonce, one outstanding request and passive resync, no hidden reconfigure. |
| G7-4 | Planned48/52: exactly txt/json artifacts, hash/length, atomic finalization and worst-case verdict. |
| G7-5 | Planned43/48: real-core CLI fixture and proper dependencies; fake protocol producer alone insufficient. |
| G7-6 | Planned44/47/53/55: one COV matrix with field-owner tests and future CLI columns, not a false current completion claim. |
| G7-7 | Planned43: static command/config tables, no dynamic registry. |
| G7-8 | Corrected here: shared clock/silence/quarantine admission handles elapsed cached flag and failed anchor; P44-46/CLI tests specified. |
| G7-9 | Planned49: finite serial-process join and explicit inconclusive result, no unbounded flush/drain/retry. |
| G7-10 | Planned50-52: systematic evidence mutation. Clarified P50/51 acceptance to every required field in turn. |
| G7-11 | Planned43: freeze grammar/schema only; later stages append executing commands. |
| G7-12 | Planned43/47: column-zero CRLF records, parser strips nothing; bounded console ownership. |
| G7-13 | Planned53/54: map named existing tests and add gaps/cross-entry cases, not another duplicated suite. |
| G7-14 | Planned47/49/50: console/baud/DTR/RTS, minimum partner and IRQ-blocked versus polling-eligible fixtures explicit. |

## Findings G8-G9: later exact parts

| ID | Disposition and current evidence |
| --- | --- |
| G8-1 | Planned56/57: suffix-identical software records shared, declared part/temperature/physical evidence separate. |
| G8-2 | Planned56: PartTable and map-keyed policy/robust/VOD, no premature family support; P60/61 exceptions explicit. |
| G8-3 | Planned57-59: listed A field/default/27-row EEE deltas assigned explicitly; A tables8-86/90/91 and8.3.3.1. |
| G8-4 | Planned61: conservative consuming Register_12/MISR/FLDS, conflicting R type versus clear prose and unknown latch relationship retained. |
| G8-5 | Planned60: declared RMII role, SOR1 and collected RCSR, no role write;825 table7-26/strap evidence. |
| G8-6 | Planned60/61: Leader clock-loss opt-in for mux/isolation/DPD; source versus inference and Q15 separate. |
| G8-7 | Planned61: typed mode recipes/allowlists, no826 VOD on825, conflicting bypass descriptions retained. |
| G8-8 | Planned61: SecureON exact-address source exception under SNLA261A, no generic raw access; Q15. |
| G8-9 | Planned57/58: conditional Strap11, Strap8 conflict and040F preserving mirror; SDAA067A/A datasheet. |
| G8-10 | Planned56 through G8-2: robust rule part-keyed, not imposed on825. |
| G8-11 | Planned31/59 through G4-5: concrete common packing, per-part command semantics. |
| G8-12 | Planned60: conservative20MHz and24MHz prose conflict, not a claimed resolution. |
| G9-1 | Planned65: SOR1 encoded FX/SD sets mapped explicitly;822 tables7-10/8-84. |
| G9-2 | Planned65: stale FX evidence and maintenance refresh, no interrupt-driven invention; SNLA437A2.3.4 and Q14. |
| G9-3 | Planned62/63: five literal reset namespaces/domains, strap conflict retained rather than ambiguous reset labels. |
| G9-4 | Planned67: GENCFG000A exception, optional WoL aid/neutral composition/restart and PTP ownership. |
| G9-5 | Planned63/65: no crossover writes in FX, mode UNKNOWN, part-keyed robust rule. |
| G9-6 | Planned64: sourced4300 transition, owner checks, inferred reapplication/reverse route;822 tables8-23/81, physical pending. |
| G9-7 | Planned62/67: explicit separate namespace and0007/003D/4007 trace;8227.4.2.9. |
| G9-8 | Planned67:822 W1S acknowledgement0800, neutral zero on other writes and CRC preservation; not826 W0C. |
| G9-9 | Historical incorrect reset claim corrected:822 RXFS12 resets0 versus826/8251; table8-88/P63/P67. |
| G9-10 | Planned67/gated: Bit Mask Flag has no defined semantics; preserve, no invented setter;8228-87. |
| G9-11 | Planned66: seven-bit amplitudes and SNLA253 scale conflict retained;8228-60..64. |
| G9-12 | Planned66: start clear AND Done, initial Done1 test; this freshness minimum is not physical qualification. |
| G9-13 | Planned64: GPIO1 clock field and inferred SD conflict;8228-81. |
| G9-14 | Planned65/66: Fiber Far-End Fault and FX swing explicitly included. |
| G9-15 | Planned62/64: fixed RGMII shifts distinct from MII TXCPSR, field-width conflict retained;8228-36. |
| G9-16 | Planned65: FIBER_GENCFG0465 distinct from GENCFG0456. |
| G9-17 | Planned62/64/65: MAC versus medium qualified separately, FX+RGMII not stated/physical pending, correct source sections. |
| G9-18 | Planned65: medium enum, per-medium capabilities, observed medium, CLI and adapter admission assigned. |
| G9-19 | Planned62/67: shared822 map with exact temperature/fiber capabilities and separate physical evidence. |

## Findings G10: packages and verification

| ID | Disposition and current evidence |
| --- | --- |
| G10-1 | Implemented provisioning: idf.cmd/idf.ps1 checks installed pinned checkout; no global environment change. P37 owns actual SDK build. |
| G10-2 | Planned71: push/PR/manual triggers and exact commit run discovery, no pre71 dispatch/status check. Main policy makes old merge concern moot. |
| G10-3 | Corrected here: six actual fields including authors, matching generate_version.py FIELDS/AUTHORS; P69 preserves ownership. |
| G10-4 | Planned70/77: PlatformIO explicitly unsupported this execution, no unnecessary wrapper. |
| G10-5 | Planned69: IDF branch first, portable install/export/config helpers and generated version; no FILE_SET above minimum. Current missing install is expected. |
| G10-6 | Planned69/70: reuse P10 tests, add new outputs/CLI rejection, missing-Git fixture belongs70. |
| G10-7 | Planned69/72-80: explicit CONTRIBUTING owner, stage reports/tested SHA, no nonexistent validation.md/manifest requirement. |
| G10-8 | Corrected: progress/index and AGENTS/VERIFICATION list runnable commands; historical baseline counts stay historical. |
| G10-9 | Planned55/68/71: UBSan trap locally, other toolchains/sanitizers separate. Current GNU accepts the stated flag; no invented flag defect. |
| G10-10 | Planned68: real two-instance application separate from registered assertions/tests-OFF consumer. |
| G10-11 | Planned: one matrix, gap-based53/54, sampled75/78, delta79, changed-input80; sibling audit rule retained. |
| G10-12 | Planned72/73/80: physical BLOCKED/NOT RUN and final set PARTIAL, even if final report work completes. |
| G10-13 | Corrected: gitignore SDK/dependency/log patterns and fixture exception present; revisit if committing sdkconfig.ci or registry locks. |
| G10-14 | Planned69-71: directory export-ignore, HEAD tar.gz, containment/local-path validation, extracted consumer and first-Linux failures. |

## Findings G11-G13: contracts, structure and access

| ID | Disposition and current evidence |
| --- | --- |
| G11-1 | Planned37-42: unmodified public SDK route replaces fork requirement, but new native_rx_quiescence gate prevents unsafe destruction; see G6-1. |
| G11-2 | Implemented OP9 barrier/no CLEANUP engine; corrected P21 restoration accounting drift here. |
| G11-3 | Implemented: abandonCleanup/external disposition, no QuiescenceDisposition or RETIRE_REQUIRED machinery. |
| G11-4 | Implemented: clockLagUs quantization/source latency at read instant; post-callback upper wait anchor. |
| G11-5 | Implemented: short maxFrames yields before complete batch; no split or premature operation timeout; operation/reset/negotiation tests. |
| G11-6 | Implemented: admitReset is single reset look-ahead, ordinary admitBudget; no generic reserve-plan engine. |
| G11-7 | Transport correction implemented; health planned21. Corrected managed-restoration streak contradiction here, preserving lastError/result. |
| G11-8 | Implemented: _yieldOperation counts no-batch/no-time-progress only; frozen/coarse clock regressions. |
| G11-9 | Implemented through G1-3: persistent HARD silence. |
| G11-10 | Implemented through G1-2: mandatory VOD. |
| G11-11 | Implemented current OP14 subset in _execute/_checkMutation/collector/link; pending result reads versus reset WAIT/write denial explicit. |
| G11-12 | Implemented initialize reset kinds; planned22 startVerify. No redundant reset/recover aliases. |
| G11-13 | Implemented BatchSummary/result-free snapshot; later next-page result fields have real consumers, not duplicated receipts. |
| G11-14 | Implemented: three positive transport bounds and widened admission arithmetic. |
| G11-15 | Implemented timestamp microseconds; planned HealthSnapshot agrees, no storedMs conversion. |
| G11-16 | Implemented private exact revision table, no public RevisionPolicy; future part table56. |
| G11-17 | Corrected COMMON authority/read map; historical audit directions now point to this current record. |
| G11-18 | Corrected: shipped declaration links and normative OP section. Reject arbitrary line-count quota as a present defect. |
| G11-19 | Corrected ledger owners/OperationBudget/cleanupOwed and13-frame MMD description; old duration members absent. |
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
| G12-8 | Source-qualified: five-frame RMW labeled inferred from8.3.11.1, not printed; Q4 remains. No unsupported fallback change. |
| G12-9 | Corrected full LEDCR-write Doxygen: four fields,20Hz/active-low consequence, modifyMmd for one field. |
| G12-10 | Implemented recipe offsets uniformly PROCEDURE/ACCESS_POLICY; EEE DEVAD source policy separately constrained. |
| G12-11 | Implemented ledger/address/field/policy tests and every intentional CTest failure-path registration. |
| G12-12 | Implemented distinct DEVAD/REGCR names, direct examples without discarded receipts or redundant staging. |
| G13-1 | Implemented per-target reserved preservation, FFAF/FFDF literals, LEDCR-specific zero policy; power pin is separately fixed under G4-4. |
| G13-2 | Planned35: fourteen COMPT10M actions versus gated100M0428, no premature compliance support. |
| G13-3 | Corrected COMMON owner binding, temporary FIFO23, compatibility33 and tx10/tx10034. |
| G13-4 | Corrected calibration citation: tables8-47/48p73,8.4.1p48. |
| G13-5 | Planned31: rxfs_sfd_reference next evidence assigned31, not29. |
| G13-6 | Planned34/Q18: unknown FLD reset remains, physical evidence73. |
| G13-7 | Corrected RV:6100/4100 are composed examples, duplex is test choice, not printed literals. |

## Additional M/N and contract R findings

| ID | Disposition and current evidence |
| --- | --- |
| M-1 | Corrected: P41/43/68 use separate existing header/source paths; actual named FieldCore/TMP paths checked. |
| N-1 | Source-implied policy corrected in both EEE gates; P29 route-conflict preflight replaces blanket pin gate, physical Q1 retained. |
| N-2 | Re-pin reflected in P40/DD; clarified board RX signal names here, final-netlist validation still required. |
| N-3 | Implemented: Advertisement has no persistent nextPage; typed exchange and NP0 reset replay only. |
| N-4 | Planned40/DD6: default-loop handlers bounded/nonblocking, glue has no own timeout; RX lifetime now additionally gates destruction. |
| N-5 | Implemented25 / recovery35 pending: live CDCR14 rejection before cable mutation, separate clear/HARD contingency. |
| N-6 | Planned62: function10 REGCR contradiction retained; no-post-increment only,8227.4.2.7 versus7.4.2.1. |
| R-1 | Implemented persistent quiet floor across cancel/take/operations; OP12 external owner duty survives end/rebind. |
| R-2 | Implemented reset result distinction; temporary session producer planned23. |
| R-3 | Implemented configure retry _cleanupOwner; planned22 verify cannot clear barrier; temporary stop23. |
| R-4 | Implemented silent validated starts/staged tokens; future verify/session admission remains OP2. |
| R-5 | Implemented D6 unconditional VOD. |
| R-6 | Planned23: owning stop needs complete initialization to restore initialized evidence. |
| R-7 | Planned23: failed session start still admits its owning stop under OP9. |
| R-8 | Implemented current direct write _checkMutation; future session stop/acquisition extension assigned23+. |
| R-9 | Corrected COMMON/OP9: two-call owner-dwell sessions versus self-contained tests restoring before success. |
| R-10 | Implemented link resolution suppression during semantic mutation; temporary producer awaits23+. |
| R-11 | Implemented published busQuietUntilUs and shared-owner duty/two-instance tests; CLI planned guard corrected under G7-8. |
| R-12 | Implemented literal OP6 four-lag inequality,100Hz/exact-expiry timing tests. |
| R-13 | Implemented maxStalledPolls100000 and finite frozen-clock behavior, advancing ticks distinguished. |
| R-14 | Implemented _pollTimeValid and post-callback tracking; no false first-poll stall. |
| R-15 | Implemented running invalidation freezes result, external-reset updates current cleanup evidence, pending historical result retained. |
| R-16 | Implemented persistent cleanupOwed; temporaryMayBeActive producers and owning stop planned23+. |
| R-17 | Implemented NONE/RESTART preservation under OP13; temporary verify rejection planned22/23. |
| R-18 | Shipped API/file migration complete; future features use OP contracts, earlier stage8 reports remain history. |

## Earlier reports and decisions

The correctness report's nine explicit defects are accounted for individually:
W0C by G4-3/G1-15; consumed aggregate evidence by G2-12/G2-13 and current
collector; prohibited local ANAR fault by configuration/tests; sleep-policy
versus state by P28; MII-loopback conflict by P23/table8-19; medium versus MAC
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
transport contract and future native evidence; exact825 identity by P60. None
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
| `cmake -S . -B build/audit-promptset -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON`, build, `ctest --test-dir build/audit-promptset --output-on-failure --no-tests=error` | PASS85/85, including new forced_power_pin and11 intentional rejection probes; public headers compile separately. |
| Same build/test in `build/audit-promptset-ubsan`, CMAKE_CXX_FLAGS `-fsanitize=undefined -fsanitize-trap=undefined` | PASS85/85. |
| `cmake -S test/consumer -B build/audit-promptset-consumer -G Ninja -DCMAKE_BUILD_TYPE=Release`, build, CTest | PASS1/1, tests OFF, real portable library linked. |
| `python -m unittest discover -s test -p "test_*.py" -q` | PASS103 tests. |
| `python tools/check_doxygen.py` | PASS157 generated pages and2 rejected negative fixtures. |
| `python tools/verify_ledger.py` | PASS97 registers,52 procedures,35 features; structural/source-record evidence only. |
| `python tools/verify_references.py` | PASS29 payloads,46,886,437 bytes,27 extracts,1539 local links in191 Markdown files. |
| `python scripts/generate_version.py check` | PASS current0.1.0 generated header; no version change. |
| Disposition inventory comparison | PASS exactly179 unique G/M/N rows plus18 R rows, no omissions or duplicate IDs. |
| `git diff --check` | PASS. |

The new power-pin suite covers55 scenarios across AN and forced10/100 half/full,
initial software/pin-high rejection, all reset replays, and pin assertion only
at the final atomic BMCR read. Its12,991 CHECKs pass. The same test source
linked against the saved pre-fix library produced247 failed assertions, proving
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
NOT RUN in this maintenance. First-execution remote CI remains prompt71 only.
