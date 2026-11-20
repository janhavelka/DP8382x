# DP83826I feature coverage

The [P4 application owner](../examples/esp_idf/owner/README.md) consumes the
verified configuration, management-health and resolved-link evidence from
these features to admit sessions. Its host and SDK verification is recorded
in the [prompt 41 report](reports/implementation/41-owner-admission-example.md).
Its [raw frame handoff](reports/implementation/42-owner-frame-boundary.md)
uses that admission decision for each submission. Chip register coverage is unchanged.

This matrix summarizes the [authoritative ledger](registers/dp83826i.json),
without duplicating its numeric map. All 35 families have complete source
dispositions. All 79 detailed entries, 18 opaque recipe records and five
source-implied printed EEE associations are accounted for; no research row is PENDING.
The 52 procedure contracts have explicit API/CLI/test ownership and host/fixture plans.

Research COMPLETE includes selected interpretations and unresolved source gates.
Software implementation is PARTIAL: prompts 11–13 provide the
[restricted direct/indirect subset](direct-access.md), complete named catalog,
[collector and mixed-register updates](side-effect-access.md), with executing
host tests. Raw access and low-level capture do not complete typed features.
Prompt 14 completes [identity admission and foundational capabilities](identity.md)
with independent host tests. Prompt 15 completes [NONE/PCS initialization](initialization.md),
mandatory VOD verification and retained restoration. Prompt 16 completes HARD/RESTART, persistent quiet floors and external-reset/POR
reconciliation. Prompt 17 completes [negotiated profiles and typed next pages](negotiation.md),
with shared replay and partial-evidence tests. Prompt 18 completes [forced and coupled controls](forced-settings.md) through
the same configuration/replay path. Prompt 19 completes [live/cached link observation](link-observation.md)
with ten-read consistency, forced/pause qualification and retained change evidence.
Prompt 20 completes [typed events/counters](event-counters.md), per-source partial
history and source-qualified FCSCR accumulation. RECR/RMII derived interpretations
remain gated. Prompt 21 completes [passive health](health.md), with separate
managed transaction counts, state projection and retained error evidence. There
is no stage-21 silicon procedure: health is library policy. Prompt 22 adds
[explicit verification and reconciliation](reconciliation.md), reusing the
existing initialization/configuration procedures without another restoration
ledger or automatic recovery. Prompt 23 completes [loopback sessions](loopback.md):
all four ledger procedures use one bounded operation/collector/restoration owner,
with independent host tests. Stages 24-35 implement the admitted typed procedures
described below. Source-gated branches remain incomplete. Prompt 37 adds a real
P4 SDK fixture link and native host tests for the [current adapter subset](esp-idf.md);
hardware qualification is NOT RUN. Ledger tests establish their own research layer.

Prompt 38 maps the existing link/initialization/access families to native
`startConfigure`, `startInitialize`, `reconcileSdkState`, `modifyBasicControl`,
`writeClause22`/`writeMmd`/`modifyMmd` and `acquireEvidence`. All delegate the shared
procedures and collector; raw SDK ioctls still reject. Flow policy calls correctly
typed MAC/PHY entries with separate effects. Native forced 10/100 half/full,
SDK cache/getter semantics, cross-owner batch exclusion, raw invalidation and
public reconciliation have executing pinned-driver evidence. This adds no new
silicon procedure, part support or physical qualification; ledger source gates
remain unchanged. The [stage report](reports/implementation/38-esp-idf-state-arbitration.md)
records exact coverage and limits.

The [independent prompts 37-39 audit](reports/implementation/37-39-independent-audit.md)
adds execution evidence for board/initialization exclusion, discovery transport
accounting and callback/lock bounds. These corrections preserve the same typed
chip procedures, source gates and physical qualification status.

The [prompts 13-18 audit](reports/implementation/13-18-independent-audit.md)
rechecks these completed routes and adds interrupted-verification, malformed
receipt and complete next-page grammar/capacity coverage. No later feature or
physical gate is closed by this software audit.

Prompt 6 completes the [portable/native ownership design](design-decisions.md)
for all these entry families. It introduces no feature support claim. Prompt 37
implements the selected public native lifecycle with terminal resource retention;
reusable teardown, source limitations and physical gates remain separate. Full pause evidence, SDK projection, raw access and the
single collector have explicit owners in that matrix.

The single matrix below maps all 35 feature IDs and all 52 procedure IDs to
configuration routes and observable results. Every row applies to exact
DP83826I. BASIC and ENHANCED are operating modes. The mode column adds
interface restrictions where the operation requires them. The legal-value rule
below covers all 283 fields and the gated or opaque actions in the ledger. Each
multistep operation follows its typed route.

For each feature, legal values are the referenced ledger fields' `legal`,
`prohibited`, `access` and `validity` records plus procedure prerequisites and
gates. Ranges are unshifted encodings excluding prohibited values; they never
authorize writes to read-only fields or supply missing physical interpretations.

Prompt 8 adds the shared operation/cleanup contract used by these owners:
initialize/configure/verify share finite polling, retained effects, cleanupOwed
and an acyclic persistent restore order; restoration is a later bounded operation. Later feature operations
extend that same slot/result with working behavior; their signatures and existing
silicon gates remain explicit. No feature's implementation status changes here.

A named CLI command is available in the shared processor. Pending entries
name the prompt that owns the missing command or record. The host column
names CLI checks when a route exists and core/application checks otherwise.
The [CLI guide](cli.md) defines the grammar and records. The feature transcripts
run the shared processor and its portable owner. `native_cli_features` checks
selected access and admission routes through the pinned native owner. Stage 47
owns P4 console integration. The physical cases remain NOT RUN. See campaigns 72 and 73 and the
[qualification questions](prompts/REMAINING_PROMPTS_AUDIT.md#8-physical-qualification-cases-to-add).

| Ledger ID / responsibility | Mode and restriction | Source and core evidence | Owner route and evidence | Procedures or feature | CLI route | Record | Host evidence | Physical case |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `identity` / Exact identity and capabilities | BASIC/ENHANCED | Research 3 / COMPLETE. Core 14 (SOFTWARE COMPLETE; HOST PASS). Related 14; 37-38 native init host/P4 link PASS; physical NOT RUN. Gates None named. | readIdentity / identitySnapshot / capabilities; raw/decoded IDs and partial/admission evidence | identify | `connection identity`, `connection probe`, `connection discover`, `capabilities` | `result`, `sample` (address and validity) | cli_fixture_transcript, cli_processor_routes, native_cli_discovery, native_cli_shared_silence. Core: identity_scan, identity_policy | 72 cold boot: identify both modes and strapped address |
| `access` / Clause 22 and complete indirect access | BASIC/ENHANCED | Research 3 / COMPLETE. Core 11 (SOFTWARE). Related 11, 12, 13; 38 native host/P4 link PASS. Gates eee_wire. | C22/MMD direct access, stable RMW and collector; native read/write/modify/acquireEvidence and explicit modifyBasicControl; retained frame/effect report | direct_access, mmd_access | `registers read`, `registers write`, `registers modify`, `registers dump`, `registers bmcr`, `test stress`, `test stop`, `test stats`, `diagnostics selftest` | `result`, `sample` (readback, partial evidence and stress counts) | cli_fixture_transcript, cli_selftest_transcript, cli_processor_routes, native_cli_raw_access, native_cli_admission, native_cli_reconciliation. Core: transport_direct, mmd_ownership, access_admission | Q4 and 73: C22/MMD alias readback |
| `reset` / PCS, hard reset, restart and reconciliation | BASIC/ENHANCED | Research 3 / COMPLETE. Core 15-16 (SOFTWARE COMPLETE; HOST PASS). Related 15, 16; 38 native shared-silence host/P4 link PASS. Gates hard_reset_straps, pcs_reset_extent. | Native/core startInitialize reset choices and invalidateDeviceState; completion, lost trust, accepted effects, separate reconcileSdkState | pcs_reset, hard_reset, restart, external_reset | `recovery reset`, `recovery restart`, `recovery reinitialize` | `result` (admission, reset scope, effects and separate restoration) | cli_fixture_transcript, native_cli_lifecycle (PCS). Core: reset_traces, reset_floor, reset_external | Q5 and 72: reset, strap and SMI readiness traces |
| `initialization` / Required 10BASE-Te initialization | BASIC/ENHANCED | Research 3 / COMPLETE. Core 15 (SOFTWARE COMPLETE; HOST PASS). Related 15, 17, 18; 37-38 native host/P4 link PASS. Gates None named. | Shared startInitialize mandatory preparation; verified initialization evidence followed by native SDK reconciliation | initialize_10base | `connection initialize`, `connection end`, `owner start`, `owner poll`, `owner status`, `owner result`, `owner cancel`, `owner abandon` | `result` (one terminal per operation plus separate query replies) | cli_fixture_transcript, native_cli_lifecycle. Core: operation_initialization | 72 cold boot and mandatory VOD readback |
| `negotiation` / Advertisement, forced-compatible negotiation and pause | BASIC/ENHANCED | Research 3 / COMPLETE. Core 17 (SOFTWARE). Related 17, 19; 37-39 native host/P4 link PASS; physical timing unqualified. Gates None named. | LinkSettings/startConfigure, native reconcileSdkState and setFlowControl; desired/observed/trust, SDK cache agreement and separate MAC/PHY effects | configure_an | `configuration desired`, `configuration observed`, `configuration validate`, `configuration apply`, `configuration verify`, `configuration invalidate`, `cached settings` | `result` (admitted/terminal for operations) | cli_fixture_transcript, cli_processor_routes, native_cli_admission, native_cli_reconciliation. Core: negotiation_vectors, negotiation_completion | 72 advertisement readback and compatible link partner |
| `next_page` / Complete next-page exchange | BASIC/ENHANCED | Research 3 / COMPLETE. Core 17 (SOFTWARE). Related 17, 19, 20, 29, 34. Gates next_page_handshake. | Typed local pages and bounded exchange; Partner pages, partial handshake and completion | next_page_exchange | `configuration validate`, `configuration apply` (bounded AN pages) | `result` (admitted/terminal for operations) | cli_fixture_transcript (packed apply and cleanup), cli_processor_routes (decoded pages). Core: negotiation_pages, negotiation_page_failures | Q10: capable partner, exchange, re-AN |
| `crossover` / Automatic, robust and manual crossover / port mirror | BASIC/ENHANCED | Research 3 / COMPLETE. Core 18 (SOFTWARE). Related 18, 19, 34; 38 native forced 10/100 host/P4 link PASS. Gates forced_physts. | ForcedLinkSettings, crossover/mirror/extended duplex through shared startConfigure; native rejects unresolved extended forced half duplex | configure_forced, extended_link_configure | `configuration desired`, `configuration observed`, `configuration validate`, `configuration apply`, `configuration verify`, `configuration invalidate`, `cached settings` | `result` (admitted/terminal for operations) | cli_fixture_transcript, cli_processor_routes, native_cli_admission, native_cli_reconciliation. Core: forced_matrix, forced_ownership | Q11: forced link, automatic crossover and mirror |
| `link` / Live and retained link observations | BASIC/ENHANCED | Research 3 / COMPLETE. Core 19 COMPLETE (SOFTWARE; host PASS). Related 19; 37-38 native publication host/P4 link PASS. Gates forced_physts (physical Q11). | readLinkStatus and cached LinkSnapshot; current/history validity, conservative age, forced/AN pause and change evidence; native framework timer uses same collector | observe_link | `link read`, `link partner`, `link cached`, `cached link`, `link watch`, `test stop` | `result`, `sample` (current, stale and incomplete evidence) | cli_fixture_transcript, native_cli_link_events, cli_processor_cached. Core: link_read_plan, link_transitions | Q11 and 72: partner/link resolution and cable transitions |
| `eee` / EEE capability, initialization and negotiation | BASIC/ENHANCED | Research 5 / COMPLETE. Core 29 (SOFTWARE). Related 29, 37, 38, 39. Gates eee_wire, eee_control_semantics. | EEE initialization/settings/status; Local/partner capability, request and negotiated state | eee_initialize, eee_negotiation_status | `eee configure`, `eee read`, `eee status` | `result` (admitted/terminal, raw negotiation words and retained policy) | cli_control_transcript. Core: eee_recipe, eee_acquisition_failures | Q1: exact DEVAD and EEE partner |
| `lpi` / Legacy MAC LPI sessions | BASIC/ENHANCED | Research 5 / COMPLETE. Core 30 (SOFTWARE). Related 30, 37, 38, 39, 68. Gates eee_wire, eee_control_semantics. | Bounded legacy-MAC LPI start/stop, application-owned dwell; requested/uncertain state and masked control readback, no invented readiness event | legacy_lpi | `lpi start`, `lpi stop`, `lpi status` | `result` (terminal request/uncertain state and separate clear) | cli_control_transcript. Core: lpi_traces, lpi_ownership | Q2: observe LPI on the line or at the partner |
| `wol` / Magic Packet and Secure-ON wake | BASIC/ENHANCED | Research 5 / COMPLETE. Core 31 (SOFTWARE). Related 31. Gates wol_bit_mask, wol_live_reconfigure, rxfs_sfd_reference (TI clarification; 73 WoL/error fixture). | Wake MAC/password, pulse/level/CRC and acknowledgement; Verified settings and retained wake/error evidence | wol_configure, wol_acknowledge | `wol configure`, `wol read`, `wol status`, `wol acknowledge` | `result` (terminal, validity and masked event state without match values) | cli_control_transcript. Core: wol_vectors, wol_routing | Q3: wake frame and acknowledgement |
| `power` / Active-sleep policy, IEEE/deep power-down, PLL | BASIC/ENHANCED | Research 5 / COMPLETE. Core 28 (PARTIAL; deep-power entry/exit BLOCKED). Related 28, 37, 38, 39. Gates deep_power_exit. | IEEE power, active-sleep, PLL and deep-power policy; Desired policy, observed power and exit/trust | power_ieee, power_active_sleep, power_pll_control, power_deep | `power configure`, `power read`, `power status` | `result` (admitted/terminal, current validity and retained policy) | cli_control_transcript. Core: power_policy, power_transitions | Q6: power pin, software state and generated clock |
| `repeater` / RMII repeater arrangements | BASIC/ENHANCED, RMII | Research 5 / COMPLETE. Core 33 (SOFTWARE). Related 33, 68. Gates rmii_rc. | Coordinated two-PHY topology; Both owners' results, partial effects/restoration | repeater_configure | `controls interface`, `controls interface-status` (apply on each admitted side) | `result` (one owner, terminal effects and observed register validity) | cli_pin_transcript. Core: interface_repeater | 73 two PHYs and one qualified RMII reference |
| `clock` / Clock output and reset/power interactions | BASIC/ENHANCED | Research 5 / COMPLETE. Core 33 (SOFTWARE). Related 33. Gates pin_fields. | Clock output selection and dependencies; Configured policy and qualified observed state | clock_configure | `controls interface`, `controls interface-status` | `result` (one owner, terminal effects and observed register validity) | cli_pin_transcript. Core: interface_timing | Q12 and 73: clock rate, pin role and output loading |
| `mii` / MII interface controls | BASIC/ENHANCED, MII controls, isolation also RMII | Research 5 / COMPLETE. Core 33 (SOFTWARE). Related 33, 37, 38, 39. Gates rmii_rc. | MII controls/isolation; Verified interface settings/isolation | mii_configure, isolation_configure | `controls interface`, `controls interface-status` | `result` (one owner, terminal effects and observed register validity) | cli_pin_transcript. Core: interface_vectors, interface_admission | 73 MII edges, pin functions and isolation |
| `rmii` / RMII interface, elasticity and status | BASIC/ENHANCED, RMII | Research 5 / COMPLETE. Core 33 (SOFTWARE). Related 33, 37, 38, 39. Gates rmii_rc. | Leader/follower, revision, elasticity, RX_DV and shift; Verified settings and collected status | rmii_configure | `controls interface`, `controls interface-status` | `result` (one owner, terminal effects and observed register validity) | cli_pin_transcript. Core: interface_vectors, interface_collector | Q17 and 73: RMII clock and status acquisition |
| `tx100` / 100BASE-TX exposed data-path controls | BASIC/ENHANCED | Research 5 / COMPLETE. Core 34 (SOFTWARE). Related 34, 35. Gates None named. | Scrambler and idle RX-error policy; Verified data-path settings | tx100_configure | `controls quality`, `controls quality-status` | `result` (admitted/terminal and verified or partial policy) | cli_pin_transcript. Core: quality_vectors, quality_failures | 73 100M partner and calibrated data-path instrumentation |
| `tx10` / 10BASE-Te receiver, NLP, jabber and preamble | BASIC/ENHANCED | Research 5 / COMPLETE. Core 34 (SOFTWARE). Related 34, 35. Gates analog_rx_gain. | Squelch/threshold/NLP/jabber/preamble/gain; Verified settings with physical interpretation limits | tx10_configure | `controls quality`, `controls quality-status` | `result` (admitted/terminal and verified or partial policy) | cli_pin_transcript. Core: quality_vectors, quality_admission | 73 10M partner and calibrated receiver/pattern checks |
| `loopback` / All documented loopback modes | BASIC/ENHANCED | Research 4 / COMPLETE. Core 23 (SOFTWARE). Related 23. Gates loopback_recipe. | All documented near/digital/reverse/external modes; Active/maybe-active mode and restoration | loopback_near, loopback_digital, loopback_reverse, loopback_external | `diagnostics loopback-start`, `diagnostics loopback-stop`, `diagnostics loopback-status` | `result` (admission, activation and separate stop evidence) | cli_diagnostic_transcript. Core: loopback_matrix, loopback_barriers | Q8: mode routing and restoration |
| `bist` / BIST/PRBS modes and retained acquisition | BASIC/ENHANCED | Research 4 / COMPLETE. Core 24 (SOFTWARE). Related 24. Gates bist_result_semantics. | Generator/checker, packet/IPG/counter modes; Activity/lock, retained finite error sample | bist_run, bist_capture | `diagnostics bist-start`, `diagnostics bist-read`, `diagnostics bist-stop`, `diagnostics bist-status` | `result` (activation, retained capture and separate stop evidence) | cli_diagnostic_transcript. Core: bist_matrix, bist_measurements | 73 instrumented packet and error counters |
| `cable` / Manual and autorun cable capture | BASIC/ENHANCED | Research 4 / COMPLETE. Core 25 (SOFTWARE). Related 25, 26. Gates tdr_reserved, tdr_calibration, tdr_freshness_abort, tdr_recipe. | Manual/autorun settings and finite capture; Freshness-qualified raw peaks/amplitude/polarity | cable_configure, cable_manual, cable_autorun, cable_capture | `diagnostics cable-start`, `diagnostics cable-raw`, `diagnostics cable-read`, `diagnostics cable-stop`, `diagnostics cable-status`, `diagnostics cable-interpret`, `diagnostics cable-autorun`, `diagnostics cable-autorun-read`, `diagnostics cable-autorun-status` | `result` (partial capture, validity and independent stop or interpretation) | cli_diagnostic_transcript. Core: cable_matrix, cable_capture, autorun_policy | Q7: known open, short and good cables |
| `fast_drop` / MII-only Fast Link Drop | BASIC/ENHANCED, MII activation | Research 4 / COMPLETE. Core 34 (SOFTWARE). Related 34. Gates fld_reset_defaults, fld_threshold_recipe. | MII-only causes/thresholds; Verified settings and retained causes | configure_fast_drop | `controls quality`, `controls quality-status` | `result` (admitted/terminal and verified or partial policy) | cli_pin_transcript. Core: quality_vectors, quality_collector | Q18 and 73: MII drop latency and strap readback |
| `link_recovery` / Hardware link-loss recovery | BASIC/ENHANCED | Research 5 / COMPLETE. Core 34 (SOFTWARE). Related 34. Gates None named. | Hardware link-loss recovery; Verified policy | link_recovery_configure | `controls quality`, `controls quality-status` | `result` (admitted/terminal and verified or partial policy) | cli_pin_transcript. Core: quality_vectors, quality_replay | 73 timestamped disturbance and recovery |
| `led` / LED sources, blink, stretch and drive | BASIC/ENHANCED | Research 5 / COMPLETE. Core 32 (SOFTWARE). Related 32. Gates pin_fields. | Per-pin sources/polarity/blink/stretch/manual drive; Verified per-pin values | led_configure | `controls pins`, `controls pins-status` | `result` (attributed terminal effects and stable word validity) | cli_pin_transcript. Core: pins_vectors, pins_failures | Q12: LED source, polarity and drive |
| `pin_mux` / Per-pin GPIO, wake, IRQ and clock mux | BASIC/ENHANCED | Research 5 / COMPLETE. Core 32 (SOFTWARE). Related 32, 33. Gates pin_fields. | Exact per-pin mux and conflicts; Valid roles and read-only observations | pin_configure | `controls pins`, `controls pins-status` | `result` (attributed terminal effects and stable word validity) | cli_pin_transcript. Core: pins_vectors, pins_routing | Q12: pin roles and electrical behavior |
| `interrupts` / Global IRQ and all event enables/service | BASIC/ENHANCED | Research 4 / COMPLETE. Core 27 (SOFTWARE). Related 20, 27. Gates rxfs_sfd_reference (TI clarification; 73 WoL/error fixture). | Global routing/polarity/enables/test/service; Verified masks, retained causes | interrupt_configuration, interrupt_test | `irq configure`, `irq service`, `irq cached-service`, `irq status`, `irq test-start`, `irq test-stop` | `result` (verified policy, retained partial collector evidence and independent stimulus stop) | cli_control_transcript. Core: interrupt_policy, interrupt_stimulus | Q9: IRQ latching and delivery |
| `counters` / Single collector for status and destructive counters | BASIC/ENHANCED | Research 4 / COMPLETE. Core 20 COMPLETE (source-qualified SOFTWARE; host PASS). Related 20, 21. Gates recr_width, rmii_rc, rxfs_sfd_reference (TI clarification; 73 WoL/error fixture), forced_physts. | collectEvents/counterSnapshot, partial evidence, source-qualified FCSCR sums and host-history clear; test/events.cpp | collect_event_counters | `link collect`, `cached events` | `result` (new collector receipts and retained partial samples) | cli_fixture_transcript, native_cli_link_events. Core: events_vectors, events_failures | Q16/Q17: invalid symbols and RMII clock offset |
| `quality` / Raw MSE and supported quality interpretation | BASIC/ENHANCED | Research 4 / COMPLETE. Core 34 (SOFTWARE). Related 34. Gates mse_address. | Raw MSE and source-proved interpretation; Raw value/validity, no invented SNR | signal_quality_sample | `controls mse`, `controls mse-status` | `result` (raw MSE and prior link context) | cli_pin_transcript. Core: quality_raw, quality_collector | 73 instrumented signal and error exposure |
| `compliance` / Finite compliance stimulus procedures | BASIC/ENHANCED | Research 5 / COMPLETE. Core 35 (SOFTWARE). Related 35, 51, 73. Gates compt_reserved. | Collision test and finite admitted stimulus; Applied/maybe-emitting state, stop/cleanup | collision_test, compliance_session | `diagnostics compliance-start`, `diagnostics compliance-stop`, `diagnostics collision-start`, `diagnostics collision-stop`, `diagnostics electrical-status` | `result` (admitted/terminal, uncertain stimulus and separate cleanup) | cli_diagnostic_transcript. Core: electrical_traces, electrical_collision | Q19 and 73: calibrated stimulus and recovery |
| `analog` / Analog amplitude, termination and MAC output | BASIC/ENHANCED | Research 5 / COMPLETE. Core 35 (SOFTWARE). Related 35. Gates ioctrl_example, analog_rx_gain. | Amplitude/termination/slew/output profile; Verified settings and legal intermediate restoration | analog_configure | `controls analog`, `controls analog-status` | `result` (admitted/terminal and verified or partial stable words) | cli_pin_transcript. Core: analog_vectors, analog_failures | 73 calibrated amplitude, termination and rise/fall |
| `compatibility` / IPG and compatibility controls | BASIC/ENHANCED | Research 5 / COMPLETE. Core 33 (SOFTWARE). Related 33, 35. Gates None named. | IPG and compatibility policy; Verified settings | compatibility_configure | `controls interface`, `controls interface-status` | `result` (one owner, terminal effects and observed register validity) | cli_pin_transcript. Core: interface_vectors, interface_replay | 73 MAC traffic, IPG and compatibility |
| `calibration` / Resistor calibration contingency | BASIC/ENHANCED | Research 4 / COMPLETE. Core 35 (SOFTWARE). Related 16, 25, 35. Gates hard_reset_straps. | Calibration contingency recovery; Recovery/reset/reinitialization evidence | calibration_recovery | `diagnostics calibration-recover`, `diagnostics electrical-status` | `result` (CDCR observation, HARD result and replay debt) | cli_diagnostic_transcript. Core: electrical_calibration, electrical_lifecycle | 73 identified hardware and resistor calibration recovery |
| `straps` / POR/pin straps and sampled configuration | BASIC/ENHANCED | Research 3 / COMPLETE. Core 14 (HARDWARE_ONLY). Related 14, 16, 33. Gates hard_reset_straps. | Read-only captured strap observation; Raw/decoded words, mode/reset provenance | Hardware observation; no strap setter | `connection initialize`, `recovery reinitialize` (read-only receipt) | `result` (raw SOR history and validity) | cli_fixture_transcript. Core: identity_decode, reset_external | Q5 and 72: identified straps and reset |
| `outside_phy` / MAC/DMA, stack, board power and scheduling | BASIC/ENHANCED | Research 3 / COMPLETE. Core 6 (OUTSIDE_PHY). Related 6, 37, 40, 41, 42. Gates None named. | Application pins/clocks/power/MAC/stack/scheduling; Application-owned evidence | Outside core | N/A. Platform and stack are application owned | N/A. No PHY record | owner_policy, frame_session (application tests) | N/A. Application owns platform and stack |
| `absent` / Unsupported exact-part interfaces and features | BASIC/ENHANCED | Research 3 / COMPLETE. Core 14 (ABSENT). Related 14. Gates None named. | Exact-part capability validation; Explicit source-backed rejection | No invented operation | `capabilities` | `result` | cli_processor_cached. Core: identity_policy, foundation_link | N/A. Exact part excludes these functions |
| `identify` | BASIC/ENHANCED | Core 14. Gates none named. | readIdentity / identitySnapshot / capabilities | `identity` | `connection identity`, `connection probe`, `connection discover`, `capabilities` | `result`, `sample` (address and validity) | cli_fixture_transcript, cli_processor_routes, native_cli_discovery, native_cli_shared_silence. Core: identity_scan, identity_policy | 72 cold boot: identify both modes and strapped address |
| `direct_access` | BASIC/ENHANCED | Core 11. Gates none named. | readClause22 / writeClause22 / readMmd / writeMmd | `access` | `registers read`, `registers write`, `registers dump`, `registers bmcr` | `result`, `sample` (readback and partial evidence) | cli_fixture_transcript, cli_processor_routes, native_cli_raw_access, native_cli_admission, native_cli_reconciliation. Core: transport_direct, access_admission | 72 ordinary C22 readback and guarded maintenance writes |
| `mmd_access` | BASIC/ENHANCED | Core 12. Gates eee_wire. | readClause22 / writeClause22 / readMmd / writeMmd / modifyMmd | `access` | `registers read`, `registers write`, `registers modify`, `registers dump` | `result`, `sample` (readback and partial evidence) | cli_fixture_transcript, cli_processor_routes, native_cli_raw_access, native_cli_admission. Core: mmd_traces, mmd_ownership | Q4 and 73: C22/MMD alias readback |
| `pcs_reset` | BASIC/ENHANCED | Core 15. Gates pcs_reset_extent. | startInitialize({PCS}) | `reset` | `recovery reset` (PCS) | `result` (admission, terminal and effects) | cli_fixture_transcript, native_cli_lifecycle (PCS). Core: operation_initialization, operation_timing | 72 PCS reset effects and readiness |
| `hard_reset` | BASIC/ENHANCED | Core 16. Gates hard_reset_straps. | startInitialize({HARD}) | `reset` | `recovery reset` (hard) | `result` (admission, terminal, clock validity and effects) | cli_fixture_transcript. Core: reset_traces, reset_floor, reset_external | Q5 and 72: reset, strap and SMI readiness traces |
| `restart` | BASIC/ENHANCED | Core 16. Gates none named. | startInitialize({RESTART}) | `reset` | `recovery restart` | `result` (admission, terminal and effects) | cli_fixture_transcript. Core: reset_traces, native_restart_vectors | 72 separate restart from PCS and hard reset |
| `external_reset` | BASIC/ENHANCED | Core 16. Gates hard_reset_straps. | invalidateDeviceState(EXTERNAL_RESET / EXTERNAL_POWER_ON); later startInitialize | `reset` | `recovery reinitialize` (after owner attestation) | `result` (reinitialization only). External reset and POR attestation remain with the application owner. | cli_fixture_transcript. Core: reset_external | Q5 and 72: instrumented external reset and POR |
| `initialize_10base` | BASIC/ENHANCED | Core 15. Gates none named. | startInitialize | `initialization` | `connection initialize`, `recovery reinitialize` | `result` (admission, terminal, verified restoration and strap receipt) | cli_fixture_transcript. Core: operation_initialization | 72 cold boot and mandatory VOD readback |
| `configure_an` | BASIC/ENHANCED | Core 17. Gates none named. | LinkSettings / startConfigure | `negotiation` | `configuration desired`, `configuration observed`, `configuration validate`, `configuration apply`, `configuration verify`, `configuration invalidate`, `cached settings` | `result` (admitted/terminal for operations) | cli_fixture_transcript, cli_processor_routes, native_cli_admission, native_cli_reconciliation. Core: negotiation_vectors, negotiation_completion | 72 advertisement readback and compatible link partner |
| `configure_forced` | BASIC/ENHANCED | Core 18. Gates forced_physts. | CrossoverSettings / ForcedLinkSettings | `crossover` | `configuration desired`, `configuration observed`, `configuration validate`, `configuration apply`, `configuration verify`, `configuration invalidate`, `cached settings` | `result` (admitted/terminal for operations) | cli_fixture_transcript, cli_processor_routes, native_cli_admission, native_cli_reconciliation. Core: forced_matrix, forced_failures | Q11: forced link, automatic crossover and mirror |
| `next_page_exchange` | BASIC/ENHANCED | Core 17. Gates next_page_handshake. | ConfigureOptions::nextPages / startConfigure | `next_page` | `configuration validate`, `configuration apply` (bounded AN pages) | `result` (admitted/terminal for operations) | cli_fixture_transcript (packed apply and cleanup), cli_processor_routes (decoded pages). Core: negotiation_pages, negotiation_page_failures | Q10: capable partner, exchange, re-AN |
| `observe_link` | BASIC/ENHANCED | Core 19. Gates forced_physts. | readLinkStatus / LinkSnapshot | `link` | `link read`, `link partner`, `link cached`, `cached link`, `link watch` | `result`, `sample` (current, stale and incomplete evidence) | cli_fixture_transcript, native_cli_link_events, cli_processor_cached. Core: link_read_plan, link_transitions | Q11 and 72: partner/link resolution and cable transitions |
| `configure_fast_drop` | BASIC/ENHANCED, MII activation | Core 34. Gates fld_reset_defaults, fld_threshold_recipe. | QualitySettings / startConfigureQuality / qualitySnapshot | `fast_drop` | `controls quality`, `controls quality-status` | `result` (admitted/terminal and verified or partial policy) | cli_pin_transcript. Core: quality_vectors, quality_collector | Q18 and 73: MII drop latency and strap readback |
| `loopback_near` | BASIC/ENHANCED | Core 23. Gates loopback_recipe. | startLoopback / stopLoopback | `loopback` | `diagnostics loopback-start`, `diagnostics loopback-stop`, `diagnostics loopback-status` | `result` (admission, activation and separate stop evidence) | cli_diagnostic_transcript. Core: loopback_matrix, loopback_barriers | Q8: mode routing and restoration |
| `loopback_digital` | BASIC/ENHANCED | Core 23. Gates loopback_recipe. | startLoopback / stopLoopback | `loopback` | `diagnostics loopback-start`, `diagnostics loopback-stop`, `diagnostics loopback-status` | `result` (admission, activation and separate stop evidence) | cli_diagnostic_transcript. Core: loopback_matrix, loopback_barriers | Q8: mode routing and restoration |
| `loopback_reverse` | BASIC/ENHANCED | Core 23. Gates loopback_recipe. | startLoopback / stopLoopback | `loopback` | `diagnostics loopback-start`, `diagnostics loopback-stop`, `diagnostics loopback-status` | `result` (admission, activation and separate stop evidence) | cli_diagnostic_transcript. Core: loopback_matrix, loopback_barriers | Q8: mode routing and restoration |
| `loopback_external` | BASIC/ENHANCED | Core 23. Gates none named. | startLoopback / stopLoopback | `loopback` | `diagnostics loopback-start`, `diagnostics loopback-stop`, `diagnostics loopback-status` | `result` (admission, activation and separate stop evidence) | cli_diagnostic_transcript. Core: loopback_matrix, loopback_barriers | Q8: mode routing and restoration |
| `bist_run` | BASIC/ENHANCED | Core 24. Gates bist_result_semantics. | startBist / startBistAcquire / stopBist / bistSnapshot | `bist` | `diagnostics bist-start`, `diagnostics bist-read`, `diagnostics bist-stop`, `diagnostics bist-status` | `result` (activation, retained capture and separate stop evidence) | cli_diagnostic_transcript. Core: bist_matrix, bist_operation_failures | 73 instrumented packet and error counters |
| `bist_capture` | BASIC/ENHANCED | Core 24. Gates bist_result_semantics. | startBist / startBistAcquire / stopBist / bistSnapshot | `bist` | `diagnostics bist-start`, `diagnostics bist-read`, `diagnostics bist-stop`, `diagnostics bist-status` | `result` (activation, retained capture and separate stop evidence) | cli_diagnostic_transcript. Core: bist_measurements, bist_acquire_failures | 73 instrumented packet and error counters |
| `cable_configure` | BASIC/ENHANCED | Core 25. Gates tdr_calibration, tdr_recipe. | startCableTest / startCableAcquire / stopCableTest / cableSnapshot / interpretCable | `cable` | `diagnostics cable-start`, `diagnostics cable-raw`, `diagnostics cable-read`, `diagnostics cable-stop`, `diagnostics cable-status`, `diagnostics cable-interpret` | `result` (partial capture, validity and independent stop or interpretation) | cli_diagnostic_transcript. Core: cable_matrix, cable_capture, autorun_policy | Q7: known open, short and good cables |
| `cable_manual` | BASIC/ENHANCED | Core 25. Gates tdr_freshness_abort, tdr_calibration. | startCableTest / startCableAcquire / stopCableTest / cableSnapshot / interpretCable | `cable` | `diagnostics cable-start`, `diagnostics cable-raw`, `diagnostics cable-read`, `diagnostics cable-stop`, `diagnostics cable-status`, `diagnostics cable-interpret` | `result` (partial capture, validity and independent stop or interpretation) | cli_diagnostic_transcript. Core: cable_matrix, cable_capture, autorun_policy | Q7: known open, short and good cables |
| `cable_autorun` | BASIC/ENHANCED | Core 26. Gates tdr_freshness_abort. | startConfigureCableAutorun / cableAutorunSnapshot / startCableAutorunAcquire | `cable` | `diagnostics cable-autorun`, `diagnostics cable-autorun-read`, `diagnostics cable-autorun-status` | `result` (verified policy and uncorrelated raw words) | cli_diagnostic_transcript. Core: autorun_policy, autorun_acquisition | Q7: known open, short and good cables |
| `cable_capture` | BASIC/ENHANCED | Core 25. Gates tdr_calibration, tdr_freshness_abort, tdr_reserved. | startCableTest / startCableAcquire / stopCableTest / cableSnapshot / interpretCable | `cable` | `diagnostics cable-start`, `diagnostics cable-raw`, `diagnostics cable-read`, `diagnostics cable-stop`, `diagnostics cable-status`, `diagnostics cable-interpret` | `result` (partial capture, validity and independent stop or interpretation) | cli_diagnostic_transcript. Core: cable_capture, cable_interpretation | Q7: known open, short and good cables |
| `collect_event_counters` | BASIC/ENHANCED | Core 20. Gates recr_width, rmii_rc, forced_physts, rxfs_sfd_reference. | collectEvents / CounterSnapshot / evidenceSnapshot / clearEvidence | `counters` | `link collect`, `cached events` | `result` (new collector receipts and retained partial samples) | cli_fixture_transcript, native_cli_link_events. Core: events_vectors, events_failures | Q16/Q17: invalid symbols and RMII clock offset |
| `interrupt_configuration` | BASIC/ENHANCED | Core 27. Gates rxfs_sfd_reference. | startConfigureInterrupts / interruptSnapshot / serviceInterrupts / interruptServiceSnapshot / startInterruptTest / stopInterruptTest | `interrupts` | `irq configure`, `irq service`, `irq cached-service`, `irq status` | `result` (verified policy or retained partial collector evidence) | cli_control_transcript. Core: interrupt_policy, interrupt_stimulus | Q9: IRQ latching and delivery |
| `interrupt_test` | BASIC/ENHANCED | Core 27. Gates none named. | startInterruptTest / stopInterruptTest / interruptSnapshot / serviceInterrupts | `interrupts` | `irq test-start`, `irq test-stop`, `irq status` | `result` (stimulus state, effects and stop evidence) | cli_control_transcript. Core: interrupt_stimulus, interrupt_ownership | Q9: IRQ latching and delivery |
| `signal_quality_sample` | BASIC/ENHANCED | Core 34. Gates mse_address. | readSignalQuality / signalQualitySnapshot | `quality` | `controls mse`, `controls mse-status` | `result` (raw MSE and prior link context) | cli_pin_transcript. Core: quality_raw, quality_collector | 73 instrumented signal and error exposure |
| `calibration_recovery` | BASIC/ENHANCED | Core 35. Gates hard_reset_straps. | startCalibrationRecovery | `calibration` | `diagnostics calibration-recover`, `diagnostics electrical-status` | `result` (CDCR observation, HARD result and replay debt) | cli_diagnostic_transcript. Core: electrical_calibration, electrical_lifecycle | 73 identified hardware and resistor calibration recovery |
| `led_configure` | BASIC/ENHANCED | Core 32. Gates pin_fields. | configurePins / pinsSnapshot | `led` | `controls pins`, `controls pins-status` | `result` (attributed terminal effects and stable word validity) | cli_pin_transcript. Core: pins_vectors, pins_failures | Q12: LED source, polarity and drive |
| `pin_configure` | BASIC/ENHANCED | Core 32. Gates pin_fields. | configurePins / pinsSnapshot | `pin_mux` | `controls pins`, `controls pins-status` | `result` (attributed terminal effects and stable word validity) | cli_pin_transcript. Core: pins_vectors, pins_routing | Q12: pin roles and electrical behavior |
| `clock_configure` | BASIC/ENHANCED | Core 33. Gates pin_fields. | InterfaceSettings / configureInterface / interfaceSnapshot | `clock` | `controls interface`, `controls interface-status` | `result` (one owner, terminal effects and observed register validity) | cli_pin_transcript. Core: interface_timing | Q12 and 73: clock rate, pin role and output loading |
| `mii_configure` | BASIC/ENHANCED, MII | Core 33. Gates rmii_rc. | InterfaceSettings / configureInterface / interfaceSnapshot | `mii` | `controls interface`, `controls interface-status` | `result` (one owner, terminal effects and observed register validity) | cli_pin_transcript. Core: interface_vectors, interface_admission | 73 MII edges, pin functions and isolation |
| `rmii_configure` | BASIC/ENHANCED, RMII | Core 33. Gates rmii_rc. | InterfaceSettings / configureInterface / interfaceSnapshot | `rmii` | `controls interface`, `controls interface-status` | `result` (one owner, terminal effects and observed register validity) | cli_pin_transcript. Core: interface_vectors, interface_collector | Q17 and 73: RMII clock and status acquisition |
| `repeater_configure` | BASIC/ENHANCED, RMII | Core 33. Gates rmii_rc. | InterfaceSettings / configureInterface / interfaceSnapshot | `repeater` | `controls interface`, `controls interface-status` (apply on each admitted side) | `result` (one owner, terminal effects and observed register validity) | cli_pin_transcript. Core: interface_repeater | 73 two PHYs and one qualified RMII reference |
| `tx100_configure` | BASIC/ENHANCED | Core 34. Gates none named. | QualitySettings / startConfigureQuality / qualitySnapshot | `tx100` | `controls quality`, `controls quality-status` | `result` (admitted/terminal and verified or partial policy) | cli_pin_transcript. Core: quality_vectors, quality_failures | 73 100M partner and calibrated data-path instrumentation |
| `tx10_configure` | BASIC/ENHANCED | Core 34. Gates analog_rx_gain. | QualitySettings / startConfigureQuality / qualitySnapshot | `tx10` | `controls quality`, `controls quality-status` | `result` (admitted/terminal and verified or partial policy) | cli_pin_transcript. Core: quality_vectors, quality_admission | 73 10M partner and calibrated receiver/pattern checks |
| `analog_configure` | BASIC/ENHANCED | Core 35. Gates ioctrl_example, analog_rx_gain. | AnalogSettings / startConfigureAnalog / analogSnapshot | `analog` | `controls analog`, `controls analog-status` | `result` (admitted/terminal and verified or partial stable words) | cli_pin_transcript. Core: analog_vectors, analog_failures | 73 calibrated amplitude, termination and rise/fall |
| `compatibility_configure` | BASIC/ENHANCED | Core 33. Gates none named. | InterfaceSettings / configureInterface / interfaceSnapshot | `compatibility` | `controls interface`, `controls interface-status` | `result` (one owner, terminal effects and observed register validity) | cli_pin_transcript. Core: interface_vectors, interface_replay | 73 MAC traffic, IPG and compatibility |
| `link_recovery_configure` | BASIC/ENHANCED | Core 34. Gates none named. | QualitySettings / startConfigureQuality / qualitySnapshot | `link_recovery` | `controls quality`, `controls quality-status` | `result` (admitted/terminal and verified or partial policy) | cli_pin_transcript. Core: quality_vectors, quality_replay | 73 timestamped disturbance and recovery |
| `extended_link_configure` | BASIC/ENHANCED | Core 18. Gates forced_physts. | LinkSettings / startConfigure | `crossover` | `configuration desired`, `configuration observed`, `configuration validate`, `configuration apply`, `configuration verify`, `configuration invalidate`, `cached settings` | `result` (admitted/terminal for operations) | cli_fixture_transcript, cli_processor_routes, native_cli_admission, native_cli_reconciliation. Core: forced_matrix, forced_ownership | Q11: forced link, automatic crossover and mirror |
| `isolation_configure` | BASIC/ENHANCED | Core 33. Gates none named. | InterfaceSettings / configureInterface / interfaceSnapshot | `mii` | `controls interface`, `controls interface-status` | `result` (one owner, terminal effects and observed register validity) | cli_pin_transcript. Core: interface_vectors, interface_admission | 73 MII edges, pin functions and isolation |
| `collision_test` | BASIC/ENHANCED, MII | Core 35. Gates none named. | startCollisionTest / stopCollisionTest / electricalSnapshot | `compliance` | `diagnostics collision-start`, `diagnostics collision-stop`, `diagnostics electrical-status` | `result` (admitted/terminal, uncertain stimulus and separate cleanup) | cli_diagnostic_transcript. Core: electrical_collision, electrical_lifecycle | 73 MII TX_EN and COL instrumentation |
| `compliance_session` | BASIC/ENHANCED | Core 35. Gates compt_reserved. | startCompliance / stopCompliance / electricalSnapshot | `compliance` | `diagnostics compliance-start`, `diagnostics compliance-stop`, `diagnostics electrical-status` | `result` (admitted/terminal, uncertain stimulus and separate cleanup) | cli_diagnostic_transcript. Core: electrical_traces, electrical_collision | Q19 and 73: calibrated stimulus and recovery |
| `power_ieee` | BASIC/ENHANCED | Core 28. Gates none named. | PowerSettings / startConfigurePower / readPowerStatus / powerSnapshot | `power` | `power configure`, `power read`, `power status` | `result` (admitted/terminal, current validity and retained policy) | cli_control_transcript. Core: power_policy, power_transitions | Q6: power pin, software state and generated clock |
| `power_active_sleep` | BASIC/ENHANCED | Core 28. Gates none named. | PowerSettings / startConfigurePower / readPowerStatus / powerSnapshot | `power` | `power configure`, `power read`, `power status` | `result` (admitted/terminal, current validity and retained policy) | cli_control_transcript. Core: power_policy, power_transitions | Q6: power pin, software state and generated clock |
| `power_pll_control` | BASIC/ENHANCED | Core 28. Gates none named. | PowerSettings / startConfigurePower / readPowerStatus / powerSnapshot | `power` | `power configure`, `power read`, `power status` | `result` (admitted/terminal, current validity and retained policy) | cli_control_transcript. Core: power_policy, power_transitions | Q6: power pin, software state and generated clock |
| `power_deep` | BASIC/ENHANCED | Core 28. Gates deep_power_exit. | PowerSettings / startConfigurePower / readPowerStatus / powerSnapshot | `power` | `power configure`, `power read`, `power status` (deep-power request remains source-gated) | `result` (admitted/terminal, current validity and retained policy) | cli_control_transcript. Core: power_policy (gated rejection) | Q6. Deep-power exit remains source gated |
| `eee_initialize` | BASIC/ENHANCED | Core 29. Gates eee_wire, eee_control_semantics. | startConfigureEee / readEeeStatus / eeeSnapshot | `eee` | `eee configure`, `eee read`, `eee status` | `result` (admitted/terminal, raw negotiation words and retained policy) | cli_control_transcript. Core: eee_recipe, eee_acquisition_failures | Q1: exact DEVAD and EEE partner |
| `eee_negotiation_status` | BASIC/ENHANCED | Core 29. Gates eee_wire, eee_control_semantics. | startConfigureEee / readEeeStatus / eeeSnapshot | `eee` | `eee configure`, `eee read`, `eee status` | `result` (admitted/terminal, raw negotiation words and retained policy) | cli_control_transcript. Core: eee_status, eee_acquisition_failures | Q1: exact DEVAD and EEE partner |
| `legacy_lpi` | BASIC/ENHANCED | Core 30. Gates eee_wire, eee_control_semantics. | startLpi / stopLpi | `lpi` | `lpi start`, `lpi stop`, `lpi status` | `result` (terminal request/uncertain state and separate clear) | cli_control_transcript. Core: lpi_traces, lpi_ownership | Q2: observe LPI on the line or at the partner |
| `wol_configure` | BASIC/ENHANCED | Core 31. Gates wol_bit_mask, wol_live_reconfigure, rxfs_sfd_reference. | WakeSettings / startConfigureWake / readWakeStatus / wakeSnapshot | `wol` | `wol configure`, `wol read`, `wol status` | `result` (terminal and event state without match values) | cli_control_transcript. Core: wol_vectors, wol_routing | Q3: wake frame and acknowledgement |
| `wol_acknowledge` | BASIC/ENHANCED | Core 31. Gates none named. | acknowledgeWake / pollOperation / takeResult | `wol` | `wol acknowledge`, `wol read`, `wol status` | `result` (terminal and retained event evidence) | cli_control_transcript. Core: wol_acquisition, wol_failures | Q3: wake frame and acknowledgement |

All programmable feature families retain SOFTWARE scope and primary owners in
prompts 11–36. Straps are hardware observations; absent capabilities require
explicit rejection; outside-PHY application responsibilities retain their own
scope. Missing board wiring or runtime opt-in never deletes a programmable feature.
Next-page exchange belongs to 17, extended full duplex/crossover to 18,
isolation to 33 and collision testing to 35.

The [prompt-5 report](reports/implementation/5-silicon-power-interface-ledger.md)
records full coverage, independent reviews and 24 unresolved gates. A feature
gate applies to its affected branch, not automatically every operation in the
family: for example IEEE/active-sleep policy and quiesced WoL setup remain
independently specified while deep-power exit and live credential guarantees
need more evidence. Procedure and action gates remain authoritative.

The [fresh prompts 4–5 audit](reports/implementation/4-5-silicon-ledger-audit.md)
retains this source coverage and corrects analog transition/restore ordering.
Exact semantic-field and procedure-mask oracles strengthen the research evidence.

Run the [ledger checks](registers/README.md) on every extension. Retain separate
source, driver-host, package, SDK and physical evidence; campaigns 72–73 own
actual qualification. Future family research is assigned in [variants](variants.md).

WP3 retains research completeness with 97 register records (79 detailed, 18 opaque)
and five source-implied EEE associations. D2's stable reads, collector routes and
fail-closed checks do not qualify physical offsets. D3's SNLA428 transmit-A helper,
D4 forced-mode interpretation, D7 all 14 10M compliance actions and D8 transient
next-page ownership are selected research policies. Their physical questions
remain Q1/Q7/Q11/Q19/Q10 respectively; full feature implementation stays with its
assigned later prompt. The current prompt 13 collector has real destructive-read
and mixed-mutation consumers; it is not deferred by this research update.

Prompt 24 completes `bist_run` and `bist_capture` control/raw-acquisition scope
through [bounded BIST sessions](bist.md), with independent host/consumer tests.
The `bist_result_semantics` gate still blocks PASS/exact-interval claims; SDK
and physical BIST qualification remain NOT RUN.

Prompt 25 completes `cable_configure`, `cable_manual` and `cable_capture` through
[manual cable sessions](cable.md), including raw controls, the selected six-run
recipe, bounded separate HARD restoration and independent interpretation tests.
The cable family was partial at stage 25; stage 26 below completes `cable_autorun`.
CDCR calibration recovery remains prompt 35; Q7 and source conflicts remain open.
SDK linking and physical cable accuracy are NOT RUN.

Prompt 26 completes `cable_autorun`: explicit policy, verified manual handoff,
bounded shared single-run raw acquisition and final shared reset replay. All cable
software procedures now have host/portable evidence; unknown freshness/coherence,
engine quiescence and Q7 physical accuracy remain explicit source/fixture limits.
Calibration recovery still belongs to its separate prompt-35 procedure.

Prompt 27 completes `interrupt_configuration` and `interrupt_test` through
[owned IRQ policy and service](interrupts.md): all source enables, global/output/
polarity, qualified shared-source attribution, explicit test stop, finite service
and shared reset replay. Pending half-full requests survive incomplete passes.
Host tests cover complete procedures; Q9 pin/latching behavior and the RXFS SFD
interpretation gate remain open. The interrupt owner applies/restores the selected
LED1/2 mux=4 route; stage 32 LED profiles validate dependencies on that owner.

## Prompt 28 power implementation

[Power operations](power.md) implement `power_ieee`, `power_active_sleep` and
`power_pll_control`, including profile intent, non-HARD PLL preparation and shared
restoration. `power_deep` has a tested zero-I/O rejection; entry/exit remain
BLOCKED by `deep_power_exit`. Thus the whole power feature remains PARTIAL in the
ledger, despite completion of its independently supported procedures. Host tests
and portable examples do not close Q6 or establish SDK support.

Stage 29 implements `eee_initialize` and `eee_negotiation_status` through the
[EEE guide](eee.md), exact eighteen-row recipe, shared restoration and single
DEVAD-qualified collector. Host coverage is in `test/eee.cpp`; Q1 physical wire,
control and interoperability qualification remains pending. Explicit LPI is implemented separately in stage 30.


Stage 30 implements `legacy_lpi`: two bounded bit 12 calls, persistent OP-9 session
barrier, passive request evidence and separate explicit stop/restoration.
[Guide](lpi.md) and [report](reports/implementation/30-legacy-mac-lpi.md) record
host tests; physical Q1/Q2 and SDK/controller results remain NOT RUN.


Stage 31 implements `wol_configure` and `wol_acknowledge` with exact-part MAC/
Secure-ON matching, CRC/pulse/level controls, typed status, shared collection,
quiesced route/configuration and reset replay. [Guide](wake-on-lan.md) and
[report](reports/implementation/31-wake-on-lan.md) separate host protocol tests
from Q3 live-reconfiguration, W0C/RH-bit and physical wake qualification.


Stage 32 implements `led_configure` and `pin_configure` through one typed
`PinSettings` profile and shared reset/power restoration. All admitted source,
force/polarity, blink/stretch and mux values have independent literal/failure
tests. [The guide](led-gpio.md) lists source-gated routes and
[the report](reports/implementation/32-led-gpio.md) separates host evidence from
Q12. IRQ/WoL source owners retain their own mux writes and collector.


Stage 33 implements `clock_configure`, `mii_configure`, `rmii_configure`,
`repeater_configure`, `compatibility_configure` and `isolation_configure` through
one `InterfaceSettings`/`configureInterface` pass. Independent host vectors and
failure/restore tests are in `test/interface.cpp`; the
[guide](interface-clock.md) and [report](reports/implementation/33-clock-interface-repeater.md)
keep Q12 clock-mux, Q17 raw-RCSR and physical repeater evidence separate.
Clock selectors without an established activation route remain rejected.

Stage 34 completes `configure_fast_drop`, `signal_quality_sample`,
`tx100_configure`, `tx10_configure` and `link_recovery_configure` through
[quality controls](quality.md). `startConfigureQuality` owns explicit grouped
settings; `readSignalQuality` returns unscaled raw MSE. `test/quality.cpp` covers
independent literals, collector retention, failure/cancellation, shared replay
and exact bounds. The typed quality CLI belongs to prompt 46. Physical MII detector
latency, MSE calibration and partner/traffic evidence remain NOT RUN. The
`mse_address`, `fld_threshold_recipe`, `fld_reset_defaults` and `analog_rx_gain`
gates remain visible; no raw interpretation or fixture result is invented.

Prompt 35 implements `analog_configure`, `collision_test`, `compliance_session`
and `calibration_recovery` through the [electrical guide](electrical.md). Source
dispositions include gated receiver gain, IOCTRL example and 100M 0428 access;
software coverage does not close Q19 or qualify hardware.

## Prompt 36 bounded integration review

The [review report](reports/implementation/36-primary-feature-closure.md) samples
the existing rows rather than creating another catalogue. It corrects common
reset observation (`access`, `reset`, `interrupts`) and stale refresh evidence
(`eee`), with registered direct/alias, retained-result and interleaving tests.
Admitted procedures and their physical gates remain distinct.
The [independent audit](reports/implementation/36-independent-audit.md) additionally
checks quarantined link freshness and reset observations inside profile/collision batches.

**Primary closure is BLOCKED**, even though this bounded review and its fixes
are delivered. `power_deep` remains incomplete under owner 28: safe shared-0428
composition, exit order and SMI/clock readiness need exact-part evidence before
a complete finite entry/exit/restoration procedure can be implemented and tested.
Owners 32/33 retain the unresolved `pin_fields` branches; owner 35 retains
`ioctrl_example`, `analog_rx_gain` and the `compt_reserved` 100M branch. Their
exact next evidence remains in the factual ledger and owning guides. Missing
fixtures do not make these controls optional or convert blocked source facts
into absent silicon.

Prompt 55 still owns primary row closure; CLI 44-47, runner 48-52 and SDK 37-42
must consume these dispositions without claiming rejected branches work.
Independent native lifecycle/transport/link integration can use the reviewed
implemented subset while these unrelated gates remain open.
