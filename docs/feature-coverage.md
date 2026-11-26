# DP83826 feature coverage

The DP83826AI/AE software covers the 21 procedures assigned to prompt 58.
The exact IDs, grade-specific executing tests and limits are in the
[implementation record](reports/implementation/58-a-core-link.md). Prompt 59
adds 31 diagnostic and optional procedures. The A matrix below records per-grade
checks, exclusions and the remaining physical qualification.

DP83826E shares the original I register map, including the 10BASE-Te sequence
and the prohibition on local remote-fault advertisement. The primary matrix
below remains the I evidence. The E regressions run with their own declared
bindings. Deep-power exit, unresolved pin and electrical procedures and
physical measurements retain the gates recorded by their owners.
[DP83826E qualification](reports/implementation/56-e-support.md)

The [transaction ownership test map](reports/implementation/53-regression-transaction-ownership.md)
ties the indirect-access, reset, calibration, EEE, link and configuration rows to
named CTests. The installed-adapter cases cover reentry at each of the four
read, four write and five masked-update frame boundaries. They record the SDK
cache and possible MAC effects separately from the MDIO trace.
The pinned `native_start_callback_ownership` suite checks that a single owner
start admits at most one AN restart and one initial link observation, including
when another task calls back during start.

The [lifecycle regression tests](reports/implementation/54-regression-lifecycle-deadlines.md)
exercise reset readiness and admission on fine and coarse clocks, retained
event evidence, and separate cancellation and restoration tokens through the CLI.

The [P4 application owner](../examples/esp_idf/owner/README.md) consumes the
verified configuration, management-health and resolved-link evidence from
these features to admit sessions. Its host and SDK verification is recorded
in the [prompt 41 report](reports/implementation/41-owner-admission-example.md).
Its [raw frame handoff](reports/implementation/42-owner-frame-boundary.md)
uses that admission decision for each submission. Chip register coverage is unchanged.

The [hardware validation case table](hardware-validation.md#case-table) maps
identity, initialization, reset, profile, link and 10BASE-Te evidence to
executable campaign cases. Host transcripts and injected serial tests
establish software behavior. Physical case evidence remains open.
The [finite soak](hil-runner.md#finite-soak) repeats an admitted case through
the same owner and verifies per-iteration evidence after interruption.
The feature procedure rows below name their executable
[diagnostic and control cases](hardware-validation.md#feature-fixtures).
Raw collector and independent instrument requirements remain in that guide.

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

The shared CLI routes below cover all 35 feature IDs and 52 procedure IDs.
Feature transcripts execute the shared processor and portable owner.
`native_cli_features` checks selected routes through the pinned SDK owner.
The network and raw-frame P4 applications use that processor and bridge on
their existing owner tasks. `example_cli_network`, `example_cli_slow_stop` and
`owner_cli_integration` cover application admission and retained service.
`cli_native_console_framing`, `cli_native_console_input_gap`,
`cli_native_console_retention` and `cli_native_console_concurrent_logs`
cover the shared console queues. [CLI guide](cli.md),
[network example](../examples/esp_idf/basic/README.md),
[raw-frame owner](../examples/esp_idf/owner/README.md)

Platform and fixture policy can deny individual routes. Source-gated branches
remain gated. Physical cases are NOT RUN, including the IRQ pin cases.
See campaigns 72 and 73 and the
[qualification questions](prompts/REMAINING_PROMPTS_AUDIT.md#8-physical-qualification-cases-to-add).

In the owner column, each command points to the ledger field, write action or printed EEE address it handles. An asterisk matches an ID suffix or substring, and a question mark matches one pin digit. Observation and source-gated paths carry their own qualifications. Campaign names refer to the [hardware case table](hardware-validation.md#case-table). Manual Q10 needs a separately instrumented partner.

| Ledger ID / responsibility | Mode and restriction | Source and core evidence | Owner route and evidence | Procedures or feature | CLI route | Record | Host evidence | Physical case |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `identity` / Exact identity and capabilities | BASIC/ENHANCED | Research 3 / COMPLETE. Core 14 (SOFTWARE COMPLETE; HOST PASS). Related 14; 37-38 native init host/P4 link PASS; physical NOT RUN. Gates None named. | readIdentity / identitySnapshot / capabilities; raw/decoded IDs and partial/admission evidence<br>`connection probe` → `PHYIDR1.*`, `PHYIDR2.*`<br>`link read` → `BMSR.t4`, `BMSR.full*`, `BMSR.half*`, `BMSR.preamble`, `BMSR.an_ability`, `BMSR.extended` (raw BMSR)<br>`registers read` → `PHYCR.phy_address` (PHYCR address readback) | identify | `connection identity`, `connection probe`, `connection discover`, `capabilities`, `link read`, `registers read` | `result`, `sample` (address and validity) | cli_fixture_transcript, cli_processor_routes, native_cli_discovery, native_cli_shared_silence. Core: identity_scan, identity_policy, link_read_plan | 72 cold boot: identify both modes and strapped address. Case `cold-boot` |
| `access` / Clause 22 and complete indirect access | BASIC/ENHANCED | Research 3 / COMPLETE. Core 11–13 COMPLETE for policy-admitted access. Native host/P4 link PASS (38, 53). Gates eee_wire. | C22/MMD direct access, stable RMW and collector; native read/write/modify/acquireEvidence and explicit modifyBasicControl; retained frame/effect report<br>`registers read` → `REGCR.*`, `ADDAR.*` (internal selector sequence for MMD; direct selector access is rejected) | direct_access, mmd_access | `registers read`, `registers write`, `registers modify`, `registers dump`, `registers bmcr`, `test stress`, `test stop`, `test stats`, `diagnostics selftest` | `result`, `sample` (readback, partial evidence and stress counts) | example_cli_network, owner_cli_integration, cli_fixture_transcript, cli_selftest_transcript, cli_processor_run_edges, cli_processor_routes, native_cli_raw_access, native_cli_admission, native_cli_reconciliation. Core: transport_direct, mmd_ownership, access_admission | Q4 and 73: C22/MMD alias readback. Case `cable-autorun`, `observe`, `10base-te` |
| `reset` / PCS, hard reset, restart and reconciliation | BASIC/ENHANCED | Research 3 / COMPLETE. Core 15-16 (SOFTWARE COMPLETE; HOST PASS). Related 15, 16; 38 native shared-silence host/P4 link PASS. Gates hard_reset_straps, pcs_reset_extent. | Native/core startInitialize reset choices and invalidateDeviceState; completion, lost trust, accepted effects, separate reconcileSdkState<br>`recovery reset` → `BMCR.reset`, `PHYRCR.hard_reset`<br>`recovery restart` → `PHYRCR.restart` | pcs_reset, hard_reset, restart, external_reset | `recovery reset`, `recovery restart`, `recovery reinitialize` | `result` (admission, reset scope, effects and separate restoration) | cli_fixture_transcript, native_cli_lifecycle (PCS). Core: reset_traces, reset_floor, reset_external | Q5 and 72: reset, strap and SMI readiness traces. Case `pcs`, `hard`, `restart`, `cold-boot` |
| `initialization` / Required 10BASE-Te initialization | BASIC/ENHANCED | Research 3 / COMPLETE. Core 15 (SOFTWARE COMPLETE; HOST PASS). Related 15, 17, 18; 37-38 native host/P4 link PASS. Gates None named. | Shared startInitialize mandatory preparation; verified initialization evidence followed by native SDK reconciliation<br>`connection initialize`, `controls analog` → `VOD_CFG3.*` (initialization writes 0x4A40 and checks the termination fields at mask 0xFFC0. Replay then restores any explicitly admitted MDI/MDIX termination override) | initialize_10base | `controls analog`, `connection initialize`, `connection end`, `owner start`, `owner poll`, `owner status`, `owner result`, `owner cancel`, `owner abandon` | `result` (one terminal per operation plus separate query replies) | cli_fixture_transcript, native_cli_lifecycle, cli_pin_transcript. Core: operation_initialization, analog_vectors, analog_replay | 72 cold boot and mandatory VOD readback. Case `10base-te`, `cold-boot` |
| `negotiation` / Advertisement, forced-compatible negotiation and pause | BASIC/ENHANCED | Research 3 / COMPLETE. Core 17 (SOFTWARE). Related 17, 19; 37-39 native host/P4 link PASS; physical timing unqualified. Gates None named. | LinkSettings/startConfigure, native reconcileSdkState and setFlowControl; desired/observed/trust, SDK cache agreement and separate MAC/PHY effects<br>`configuration apply` → `BMCR.an_*`, `ANAR.*pause`, `ANAR.full*`, `ANAR.half*`, `ANAR.selector` (IEEE selector and supported advertisement only)<br>`configuration apply` → `ANAR.remote_fault` (always writes zero), `ANAR.t4` (read-only bit preserved)<br>`link partner` → `ALNPAR.*`, `ANER.parallel_fault`, `ANER.partner_an`, `PHYCR.pause_*` (raw partner and ANER words, qualified pause) | configure_an | `configuration desired`, `configuration observed`, `configuration validate`, `configuration apply`, `configuration verify`, `configuration invalidate`, `cached settings`, `link partner` | `result` (admitted/terminal for operations) | example_cli_network, cli_fixture_transcript, cli_processor_routes, native_cli_admission, native_cli_reconciliation. Core: negotiation_vectors, negotiation_completion, link_pause | 72 advertisement readback and compatible link partner. Case `owned-apply`, `100-full-only` |
| `next_page` / Complete next-page exchange | BASIC/ENHANCED | Research 3 / COMPLETE. Core 17 (SOFTWARE). Related 17, 19, 20, 29, 34. Gates next_page_handshake. | Typed local pages and bounded exchange; Partner pages, partial handshake and completion<br>`configuration apply` → `ANAR.next_page`, `ANER.*next`, `ANER.page`, `ANNPTR.*`, `ANLNPTR.*` (bounded exchange owns destructive page indications) | next_page_exchange | `configuration validate`, `configuration apply` (bounded AN pages) | `result` (admitted/terminal for operations) | cli_fixture_transcript (packed apply and cleanup), cli_processor_routes (decoded pages). Core: negotiation_pages, negotiation_page_failures | Manual Q10. No runner case drives a next-page-capable partner through exchange and re-AN. |
| `crossover` / Automatic, robust and manual crossover / port mirror | BASIC/ENHANCED | Research 3 / COMPLETE. Core 18 (SOFTWARE). Related 18, 19, 34; 38 native forced 10/100 host/P4 link PASS. Gates forced_physts. | ForcedLinkSettings, crossover/mirror/extended duplex through shared startConfigure; native rejects unresolved extended forced half duplex<br>`configuration apply` → `BMCR.speed`, `BMCR.duplex`, `CR1.robust_mdix`, `CR2.extended_duplex`, `CR3.*swap`, `PHYCR.*mdix` | configure_forced, extended_link_configure | `configuration desired`, `configuration observed`, `configuration validate`, `configuration apply`, `configuration verify`, `configuration invalidate`, `cached settings` | `result` (admitted/terminal for operations) | example_cli_network, cli_fixture_transcript, cli_processor_routes, native_cli_admission, native_cli_reconciliation. Core: forced_matrix, forced_ownership | Case coverage for paired speed and duplex includes `forced-10-half`, `forced-10-full`, `forced-100-half`, `forced-100-full`. Manual Q11 requires separate crossover, mirror and extended-duplex measurements. |
| `link` / Live and retained link observations | BASIC/ENHANCED | Research 3 / COMPLETE. Core 19 COMPLETE (SOFTWARE; host PASS). Related 19; 37-38 native publication host/P4 link PASS. Gates forced_physts (physical Q11). | readLinkStatus and cached LinkSnapshot; current/history validity, conservative age, forced/AN pause and change evidence; native framework timer uses same collector<br>`link read` → `BMSR.an_complete`, `BMSR.remote_fault`, `BMSR.link`, `BMSR.jabber` (first and current BMSR)<br>`link collect` → `PHYSTS.*` (raw PHYSTS in the collector record)<br>`registers read` → `PHYCR.mii_link` (raw PHYCR bit 11) | observe_link | `link collect`, `registers read`, `link read`, `link partner`, `link cached`, `cached link`, `link watch`, `test stop` | `result`, `sample` (current, stale and incomplete evidence) | example_cli_slow_stop, owner_cli_integration, cli_fixture_transcript, native_cli_link_events, cli_processor_routes, cli_processor_cached, cli_processor_run_edges. Core: link_read_plan, link_transitions | Q11 and 72: partner/link resolution and cable transitions. Case `unplug-replug`, `wrong-policy` |
| `eee` / EEE capability, initialization and negotiation | BASIC/ENHANCED | Research 5 / COMPLETE. Core 29 (SOFTWARE). Related 29, 37, 38, 39. Gates eee_wire, eee_control_semantics. | EEE initialization/settings/status; Local/partner capability, request and negotiated state<br>`eee configure` → `*_eee`, `EEECFG3_enable_bypass_controls`<br>`eee read` → `eee_printed_1014`, `eee_printed_203C`, `eee_printed_203D` (stable D2 capability, local and partner words)<br>`eee read` → `eee_printed_1001`, `eee_printed_1016` (D2 status and errors through the destructive collector) | eee_initialize, eee_negotiation_status | `eee configure`, `eee read`, `eee status` | `result` (admitted/terminal, raw negotiation words and retained policy) | cli_control_transcript. Core: eee_recipe, eee_acquisition_failures | Q1: exact DEVAD and EEE partner. Case `eee` |
| `lpi` / Legacy MAC LPI sessions | BASIC/ENHANCED | Research 5 / COMPLETE. Core 30 (SOFTWARE). Related 30, 37, 38, 39, 68. Gates eee_wire, eee_control_semantics. | Bounded legacy-MAC LPI start/stop, application-owned dwell; requested/uncertain state and masked control readback, no invented readiness event<br>`lpi start` → `EEECFG3_lpi_start`<br>`lpi stop` → `EEECFG3_lpi_stop` | legacy_lpi | `lpi start`, `lpi stop`, `lpi status` | `result` (terminal request/uncertain state and separate clear) | cli_control_transcript. Core: lpi_traces, lpi_ownership | Q2: observe LPI on the line or at the partner. Case `lpi` |
| `wol` / Magic Packet and Secure-ON wake | BASIC/ENHANCED | Research 5 / COMPLETE. Core 31 (SOFTWARE). Related 31. Gates wol_bit_mask, wol_live_reconfigure, rxfs_sfd_reference (TI clarification; 73 WoL/error fixture). | Wake MAC/password, pulse/level/CRC and acknowledgement; Verified settings and retained wake/error evidence<br>`wol configure` → `RXFCFG1_crcGate`, `RXFCFG1_pulseWidth`, `RXFCFG1_levelMode`, `RXFCFG1_enable`, `RXFCFG1_secureOn`, `RXFCFG1_magicPacketEnable`, `RXFPMD*`, `RXFSOP*` (password and MAC bytes withheld from records)<br>`wol configure` → `RXFCFG1_bitMaskFlag` (unknown bit 6 preserved under wol_bit_mask)<br>`wol acknowledge` → `RXFCFG1_levelAcknowledge` (explicit W0C after RXFS and MISR2 harvest) | wol_configure, wol_acknowledge | `wol configure`, `wol read`, `wol status`, `wol acknowledge` | `result` (terminal, validity and masked event state without match values) | cli_control_transcript, native_cli_feature_admission. Core: wol_vectors, wol_routing | Q3: wake frame and acknowledgement. Case `wol-magic`, `wol-secure`, `wol-pulse` |
| `power` / Active-sleep policy, IEEE/deep power-down, PLL | BASIC/ENHANCED | Research 5 / COMPLETE. Core 28 (PARTIAL; deep-power entry/exit BLOCKED). Related 28, 37, 38, 39. Gates deep_power_exit. | IEEE power, active-sleep, PLL and deep-power policy; Desired policy, observed power and exit/trust<br>`power configure` → `BMCR.power_down`, `PHYSCR.disable_pll`, `PHYSCR.power_save*`<br>`power configure` → `RECIPE_0428_dpd_enable` (deep request rejected before I/O under deep_power_exit) | power_ieee, power_active_sleep, power_pll_control, power_deep | `power configure`, `power read`, `power status` | `result` (admitted/terminal, current validity and retained policy) | cli_control_transcript, native_cli_features, native_cli_feature_admission, native_cli_blocked_poll. Core: power_policy, power_transitions | Q6: power pin, software state and generated clock. Case `power-ieee`, `power-sleep`, `power-pll`, `power-deep` |
| `repeater` / RMII repeater arrangements | BASIC/ENHANCED, RMII | Research 5 / COMPLETE. Core 33 (SOFTWARE). Related 33, 68. Gates rmii_rc. | Coordinated two-PHY topology; Both owners' results, partial effects/restoration | repeater_configure | `controls interface`, `controls interface-status` (apply on each admitted side) | `result` (one owner, terminal effects and observed register validity) | cli_pin_transcript. Core: interface_repeater | 73 two PHYs and one qualified RMII reference. Case `repeater-leader`, `repeater-follower` |
| `clock` / Clock output and reset/power interactions | BASIC/ENHANCED | Research 5 / COMPLETE. Core 33 (SOFTWARE). Related 33. Gates pin_fields. | Clock output selection and dependencies; Configured policy and qualified observed state<br>`controls interface` → `IO_CFG1.clock_off`, `CLK_OUT_LED_STATUS.clock_off_status` (selected output and qualified readback)<br>`controls interface` → `LED0_GPIO_CFG.clock_source`, `LED1_GPIO_CFG.clock_source` (PLL/recovered activation is source-gated) | clock_configure | `controls interface`, `controls interface-status` | `result` (one owner, terminal effects and observed register validity) | cli_pin_transcript. Core: interface_timing | Q12 and 73: clock rate, pin role and output loading. Case `interface-mii`, `interface-rmii25`, `interface-rmii50` |
| `mii` / MII interface controls | BASIC/ENHANCED, MII controls, isolation also RMII | Research 5 / COMPLETE. Core 33 (SOFTWARE). Related 33, 37, 38, 39. Gates rmii_rc. | MII controls/isolation; Verified interface settings/isolation<br>`controls interface` → `BMCR.isolate`, `CR1.fast_rxdv`, `CR2.odd_nibble_disable`, `PHYSCR.collision_full_duplex` | mii_configure, isolation_configure | `controls interface`, `controls interface-status` | `result` (one owner, terminal effects and observed register validity) | cli_pin_transcript. Core: interface_vectors, interface_admission | 73 MII edges, pin functions and isolation. Case `interface-mii` |
| `rmii` / RMII interface, elasticity and status | BASIC/ENHANCED, RMII | Research 5 / COMPLETE. Core 33 (SOFTWARE). Related 33, 37, 38, 39. Gates rmii_rc. | Leader/follower, revision, elasticity, RX_DV and shift; Verified settings and collected status<br>`controls interface` → `RCSR.tx_clock_shift`, `RCSR.clock_select`, `RCSR.rmii_mode`, `RCSR.revision`, `RCSR.elasticity`, `IO_CFG1.rx_dv` | rmii_configure | `controls interface`, `controls interface-status` | `result` (one owner, terminal effects and observed register validity) | cli_pin_transcript. Core: interface_vectors, interface_collector | Q17 and 73: RMII clock and status acquisition. Case `interface-rmii25`, `interface-rmii50` |
| `tx100` / 100BASE-TX exposed data-path controls | BASIC/ENHANCED | Research 5 / COMPLETE. Core 34 (SOFTWARE). Related 34, 35. Gates None named. | Scrambler and idle RX-error policy; Verified data-path settings<br>`controls quality` → `CR2.idle_rx_error`, `PHYSCR.scrambler_bypass` | tx100_configure | `controls quality`, `controls quality-status` | `result` (admitted/terminal and verified or partial policy) | cli_pin_transcript. Core: quality_vectors, quality_failures | 73 100M partner and calibrated data-path instrumentation. Case `quality` |
| `tx10` / 10BASE-Te receiver, NLP, jabber and preamble | BASIC/ENHANCED | Research 5 / COMPLETE. Core 34 (SOFTWARE). Related 34, 35. Gates analog_rx_gain. | Squelch/threshold/NLP/jabber/preamble/gain; Verified settings with physical interpretation limits<br>`controls quality` → `10BTSCR.lower_threshold_enable`, `10BTSCR.squelch`, `10BTSCR.nlp_disable`, `10BTSCR.jabber_disable`, `10M_CFG.short_preamble`<br>`controls analog` → `ANA_RX10BT_CTRL.receiver_current_gain` (receiver gain rejected under analog_rx_gain) | tx10_configure | `controls quality`, `controls quality-status`, `controls analog` | `result` (admitted/terminal and verified or partial policy) | cli_pin_transcript. Core: quality_vectors, quality_admission | 73 10M partner and calibrated receiver/pattern checks. Case `quality` |
| `loopback` / All documented loopback modes | BASIC/ENHANCED | Research 4 / COMPLETE. Core 23 (SOFTWARE). Related 23. Gates loopback_recipe. | All documented near/digital/reverse/external modes; Active/maybe-active mode and restoration<br>`diagnostics loopback-start` → `BMCR.loopback`, `PHYSCR.loopback_fifo_depth`, `BISCR_miiLineTransmit`, `BISCR_loopbackMode`, `*_digital` | loopback_near, loopback_digital, loopback_reverse, loopback_external | `diagnostics loopback-start`, `diagnostics loopback-stop`, `diagnostics loopback-status` | `result` (admission, activation and separate stop evidence) | cli_diagnostic_transcript. Core: loopback_matrix, loopback_barriers | Q8: mode routing and restoration. Case `loopback-mii`, `loopback-mii-line`, `loopback-pcs-in`, `loopback-pcs-out`, `loopback-analog`, `loopback-digital`, `loopback-reverse`, `loopback-external` |
| `bist` / BIST/PRBS modes and retained acquisition | BASIC/ENHANCED | Research 4 / COMPLETE. Core 24 (SOFTWARE). Related 24. Gates bist_result_semantics. | Generator/checker, packet/IPG/counter modes; Activity/lock, retained finite error sample<br>`diagnostics bist-start` → `BISCR_errorCounterMode`, `BISCR_prbsMode`, `BICSR1_ipgLength`, `BICSR2_packetLength`<br>`power read` → `BISCR_normalPower` (read-only indication through the collector)<br>`diagnostics bist-read` → `BISCR_checkerLock`, `BISCR_checkerSyncLoss`, `BISCR_generatorActive`, `BICSR1_errorBytes`, `BICSR1_freezeClear` (raw status and command-specific capture) | bist_run, bist_capture | `diagnostics bist-start`, `diagnostics bist-read`, `diagnostics bist-stop`, `diagnostics bist-status`, `power read` | `result` (activation, retained capture and separate stop evidence) | cli_diagnostic_transcript, cli_control_transcript. Core: bist_matrix, bist_measurements | 73 instrumented packet and error counters. Case `bist-off`, `bist-single`, `bist-checker`, `bist-both`, `bist-digital`, `bist-analog`, `bist-external` |
| `cable` / Manual and autorun cable capture | BASIC/ENHANCED | Research 4 / COMPLETE. Core 25 (SOFTWARE). Related 25, 26. Gates tdr_reserved, tdr_calibration, tdr_freshness_abort, tdr_recipe. | Manual/autorun settings and finite capture; Freshness-qualified raw peaks/amplitude/polarity<br>`diagnostics cable-autorun` → `CR1.tdr_auto`<br>`diagnostics cable-start` → `CDCR_start`, `*_tdr_*` (manual recipe)<br>`diagnostics cable-raw` → `CDSCR*`, `TDR_17*` (raw tuning inputs)<br>`diagnostics cable-read` → `CDCR_done`, `CDCR_failed`, `CDLRR1_*`, `CDLAR*` (capture status and result words) | cable_configure, cable_manual, cable_autorun, cable_capture | `diagnostics cable-start`, `diagnostics cable-raw`, `diagnostics cable-read`, `diagnostics cable-stop`, `diagnostics cable-status`, `diagnostics cable-interpret`, `diagnostics cable-autorun`, `diagnostics cable-autorun-read`, `diagnostics cable-autorun-status` | `result` (partial capture, validity and independent stop or interpretation) | cli_diagnostic_transcript. Core: cable_matrix, cable_capture, autorun_policy | Q7: known open, short and good cables. Case `cable-manual`, `cable-raw`, `cable-autorun` |
| `fast_drop` / MII-only Fast Link Drop | BASIC/ENHANCED, MII activation | Research 4 / COMPLETE. Core 34 (SOFTWARE). Related 34. Gates fld_reset_defaults, fld_threshold_recipe. | MII-only causes/thresholds; Verified settings and retained causes<br>`controls quality` → `CR3.descrambler_drop`, `CR3.fast_drop_modes`, `FLD_CFG*` | configure_fast_drop | `controls quality`, `controls quality-status` | `result` (admitted/terminal and verified or partial policy) | cli_pin_transcript. Core: quality_vectors, quality_collector | Q18 and 73: MII drop latency and strap readback. Case `quality` |
| `link_recovery` / Hardware link-loss recovery | BASIC/ENHANCED | Research 5 / COMPLETE. Core 34 (SOFTWARE). Related 34. Gates None named. | Hardware link-loss recovery; Verified policy<br>`controls quality` → `CR1.link_recovery` | link_recovery_configure | `controls quality`, `controls quality-status` | `result` (admitted/terminal and verified or partial policy) | cli_pin_transcript. Core: quality_vectors, quality_replay | 73 timestamped disturbance and recovery. Case `quality` |
| `led` / LED sources, blink, stretch and drive | BASIC/ENHANCED | Research 5 / COMPLETE. Core 32 (SOFTWARE). Related 32. Gates pin_fields. | Per-pin sources/polarity/blink/stretch/manual drive; Verified per-pin values<br>`controls pins` → `LEDCR.*`, `PHYCR.led*`, `MLEDCR.*`, `LEDCFG.*`, `LEDCFG2.*` | led_configure | `controls pins`, `controls pins-status` | `result` (attributed terminal effects and stable word validity) | cli_pin_transcript. Core: pins_vectors, pins_failures | Q12: LED source, polarity and drive. Case `pins`, `pins-basic` |
| `pin_mux` / Per-pin GPIO, wake, IRQ and clock mux | BASIC/ENHANCED | Research 5 / COMPLETE. Core 32 (SOFTWARE). Related 32, 33. Gates pin_fields. | Exact per-pin mux and conflicts; Valid roles and read-only observations<br>`controls pins` → `LED?_GPIO_CFG.mux` (only admitted pin-specific mux values) | pin_configure | `controls pins`, `controls pins-status` | `result` (attributed terminal effects and stable word validity) | cli_pin_transcript. Core: pins_vectors, pins_routing | Q12: pin roles and electrical behavior. Case `pins`, `pins-basic` |
| `interrupts` / Global IRQ and all event enables/service | BASIC/ENHANCED | Research 4 / COMPLETE. Core 27 (SOFTWARE). Related 20, 27. Gates rxfs_sfd_reference (TI clarification; 73 WoL/error fixture). | Global routing/polarity/enables/test/service; Verified masks, retained causes<br>`irq configure` → `PHYSCR.interrupt_*`, `MISR*_*enable`, `RXFS.interrupt_source` (enable bytes, output and source selection)<br>`irq service` → `MISR*_*event` (RC history through one collector)<br>`irq test-start` → `PHYSCR.test_interrupt` (temporary stimulus with explicit stop) | interrupt_configuration, interrupt_test | `irq configure`, `irq service`, `irq cached-service`, `irq status`, `irq test-start`, `irq test-stop` | `result` (verified policy, retained partial collector evidence and independent stimulus stop) | cli_control_transcript. Core: interrupt_policy, interrupt_stimulus | Q9: IRQ latching and delivery. Case `irq` |
| `counters` / Single collector for status and destructive counters | BASIC/ENHANCED | Research 4 / COMPLETE. Core 20 COMPLETE (source-qualified SOFTWARE; host PASS). Related 20, 21. Gates recr_width, rmii_rc, rxfs_sfd_reference (TI clarification; 73 WoL/error fixture), forced_physts. | collectEvents/counterSnapshot, partial evidence, source-qualified FCSCR sums and host-history clear; test/events.cpp<br>`link collect` → `FLDS.causes`, `FCSCR.*`, `RECR.*` (RECR retains raw width evidence)<br>`controls interface` → `RCSR.overflow`, `RCSR.underflow` (collector retains RCSR RC evidence during interface updates)<br>`controls quality` → `10BTSCR.polarity` (collector preserves consumed polarity)<br>`wol read` → `RXFS.sfd_error`, `RXFS.bad_crc`, `RXFS.secure_on_error`, `RXFS.magic_packet` (retained wake/error sample) | collect_event_counters | `link collect`, `cached events`, `controls interface`, `controls quality`, `wol read` | `result` (new collector receipts and retained partial samples) | cli_fixture_transcript, native_cli_link_events, cli_pin_transcript, cli_control_transcript. Core: events_vectors, events_failures, quality_collector, interface_collector | Case `irq` checks a guarded RECR sample of zero. Manual Q16 requires invalid-symbol boundary injection. Manual Q17 requires a controlled RMII clock offset and FIFO polarity, clear and rearm measurements. |
| `quality` / Raw MSE and supported quality interpretation | BASIC/ENHANCED | Research 4 / COMPLETE. Core 34 (SOFTWARE). Related 34. Gates mse_address. | Raw MSE and source-proved interpretation; Raw value/validity, no invented SNR<br>`controls mse` → `MSE_Val.*` | signal_quality_sample | `controls mse`, `controls mse-status` | `result` (raw MSE and prior link context) | cli_pin_transcript. Core: quality_raw, quality_collector | 73 instrumented signal and error exposure. Case `mse` |
| `compliance` / Finite compliance stimulus procedures | BASIC/ENHANCED | Research 5 / COMPLETE. Core 35 (SOFTWARE). Related 35, 51, 73. Gates compt_reserved. | Collision test and finite admitted stimulus; Applied/maybe-emitting state, stop/cleanup<br>`diagnostics collision-start` → `BMCR.collision_test`<br>`diagnostics compliance-start` → `COMPT.pattern`, `compliance_10_*`, `compliance_100_disable`, `compliance_100_enable`<br>`diagnostics compliance-start` → `compliance_100_count_*` (COUNT100 rejected under compt_reserved) | collision_test, compliance_session | `diagnostics compliance-start`, `diagnostics compliance-stop`, `diagnostics collision-start`, `diagnostics collision-stop`, `diagnostics electrical-status` | `result` (admitted/terminal, uncertain stimulus and separate cleanup) | cli_diagnostic_transcript. Core: electrical_traces, electrical_collision | Q19 and 73: calibrated stimulus and recovery. Case `collision-10`, `collision-100`, `compliance-tx100`, `compliance-nlp10`, `compliance-prbs10`, `compliance-pattern10` |
| `analog` / Analog amplitude, termination and MAC output | BASIC/ENHANCED | Research 5 / COMPLETE. Core 35 (SOFTWARE). Related 35. Gates ioctrl_example, analog_rx_gain. | Amplitude/termination/slew/output profile. Explicitly admitted termination settings override the VOD_CFG3 baseline in the initialization row. Readback verifies the settings and replay preserves legal intermediate values<br>`controls analog` → `IO_CFG1.mac_impedance`, `VOD_CFG1.*`, `VOD_CFG2.*`, `IOCTRL.mac_slew` | analog_configure | `controls analog`, `controls analog-status` | `result` (admitted/terminal and verified or partial stable words) | cli_pin_transcript. Core: analog_vectors, analog_failures | 73 calibrated amplitude, termination and rise/fall. Case `analog` |
| `compatibility` / IPG and compatibility controls | BASIC/ENHANCED | Research 5 / COMPLETE. Core 33 (SOFTWARE). Related 33, 35. Gates None named. | IPG and compatibility policy; Verified settings<br>`controls interface` → `GENCFG.min_ipg` | compatibility_configure | `controls interface`, `controls interface-status` | `result` (one owner, terminal effects and observed register validity) | cli_pin_transcript. Core: interface_vectors, interface_replay | 73 MAC traffic, IPG and compatibility. Case `interface-rmii25`, `interface-rmii50` |
| `calibration` / Resistor calibration contingency | BASIC/ENHANCED | Research 4 / COMPLETE. Core 35 (SOFTWARE). Related 16, 25, 35. Gates hard_reset_straps. | Calibration contingency recovery; Recovery/reset/reinitialization evidence<br>`diagnostics calibration-recover` → `CDCR_rescalEnable` (conditional HARD and replay) | calibration_recovery | `diagnostics calibration-recover`, `diagnostics electrical-status` | `result` (CDCR observation, HARD result and replay debt) | cli_diagnostic_transcript. Core: electrical_calibration, electrical_lifecycle | 73 identified hardware and resistor calibration recovery. Case `calibration-recovery` |
| `straps` / POR/pin straps and sampled configuration | BASIC/ENHANCED | Research 3 / COMPLETE. Core 14 (HARDWARE_ONLY). Related 14, 16, 33. Gates hard_reset_straps. | Read-only captured strap observation; Raw/decoded words, mode/reset provenance<br>`connection initialize` → `SOR1.*`, `SOR2.*` (sampled SOR words and validity) | Hardware observation; no strap setter | `connection initialize`, `recovery reinitialize` (read-only receipt) | `result` (raw SOR history and validity) | cli_fixture_transcript. Core: identity_decode, reset_external | Q5 and 72: identified straps and reset. Case `cold-boot`, `hard` |
| `outside_phy` / MAC/DMA, stack, board power and scheduling | BASIC/ENHANCED | Research 3 / COMPLETE. Core 6 (OUTSIDE_PHY). Related 6, 37, 40, 41, 42. Gates None named. | Application pins/clocks/power/MAC/stack/scheduling; Application-owned evidence | Outside core | N/A. Platform and stack are application owned | N/A. No PHY record | example_cli_network, example_cli_slow_stop, owner_cli_integration, owner_policy, frame_session (application tests) | N/A. Application owns platform and stack |
| `absent` / Unsupported exact-part interfaces and features | BASIC/ENHANCED | Research 3 / COMPLETE. Core 14 (ABSENT). Related 14. Gates None named. | Exact-part capability validation; Explicit source-backed rejection | No invented operation | `capabilities` | `result` | cli_processor_cached. Core: identity_policy, foundation_link | N/A. Exact part excludes these functions |
| `identify` | BASIC/ENHANCED | Core 14. Gates none named. | readIdentity / identitySnapshot / capabilities | `identity` | `connection identity`, `connection probe`, `connection discover`, `capabilities` | `result`, `sample` (address and validity) | cli_fixture_transcript, cli_processor_routes, native_cli_discovery, native_cli_shared_silence. Core: identity_scan, identity_policy | 72 cold boot: identify both modes and strapped address. Case `cold-boot` |
| `direct_access` | BASIC/ENHANCED | Core 11. Gates none named. | readClause22 / writeClause22 / readMmd / writeMmd | `access` | `registers read`, `registers write`, `registers dump`, `registers bmcr` | `result`, `sample` (readback and partial evidence) | example_cli_network, cli_fixture_transcript, cli_processor_routes, native_cli_raw_access, native_cli_admission, native_cli_reconciliation. Core: transport_direct, access_admission | 72 ordinary C22 readback and guarded maintenance writes. Case `cable-autorun`, `observe`. CR1 readback in cable autorun and rejected write in observe. Qualify maintenance writes separately. |
| `mmd_access` | BASIC/ENHANCED | Core 12. Gates eee_wire. | readClause22 / writeClause22 / readMmd / writeMmd / modifyMmd | `access` | `registers read`, `registers write`, `registers modify`, `registers dump` | `result`, `sample` (readback and partial evidence) | cli_fixture_transcript, cli_processor_routes, native_cli_raw_access, native_cli_admission. Core: mmd_traces, mmd_ownership | Q4 and 73: C22/MMD alias readback. Case `10base-te`. Compare direct/alias frames separately for Q4. |
| `pcs_reset` | BASIC/ENHANCED | Core 15. Gates pcs_reset_extent. | startInitialize({PCS}) | `reset` | `recovery reset` (PCS) | `result` (admission, terminal and effects) | cli_fixture_transcript, native_cli_lifecycle (PCS). Core: operation_initialization, operation_timing | 72 PCS reset effects and readiness. Case `pcs` |
| `hard_reset` | BASIC/ENHANCED | Core 16. Gates hard_reset_straps. | startInitialize({HARD}) | `reset` | `recovery reset` (hard) | `result` (admission, terminal, clock validity and effects) | cli_fixture_transcript. Core: reset_traces, reset_floor, reset_external | Q5 and 72: reset, strap and SMI readiness traces. Case `hard` |
| `restart` | BASIC/ENHANCED | Core 16. Gates none named. | startInitialize({RESTART}) | `reset` | `recovery restart` | `result` (admission, terminal and effects) | cli_fixture_transcript. Core: reset_traces, native_restart_vectors | 72 separate restart from PCS and hard reset. Case `restart` |
| `external_reset` | BASIC/ENHANCED | Core 16. Gates hard_reset_straps. | invalidateDeviceState(EXTERNAL_RESET / EXTERNAL_POWER_ON); later startInitialize | `reset` | `recovery reinitialize` (after owner attestation) | `result` (reinitialization only). External reset and POR attestation remain with the application owner. | cli_fixture_transcript. Core: reset_external | Q5 and 72: instrumented external reset and POR. Case `cold-boot`. POR receipt only. Capture external RESET_N, SMI floor and later reinitialization separately. |
| `initialize_10base` | BASIC/ENHANCED | Core 15. Gates none named. | startInitialize | `initialization` | `connection initialize`, `recovery reinitialize` | `result` (admission, terminal, verified restoration and strap receipt) | cli_fixture_transcript. Core: operation_initialization | 72 cold boot and mandatory VOD readback. Case `10base-te`, `cold-boot` |
| `configure_an` | BASIC/ENHANCED | Core 17. Gates none named. | LinkSettings / startConfigure | `negotiation` | `configuration desired`, `configuration observed`, `configuration validate`, `configuration apply`, `configuration verify`, `configuration invalidate`, `cached settings` | `result` (admitted/terminal for operations) | example_cli_network, cli_fixture_transcript, cli_processor_routes, native_cli_admission, native_cli_reconciliation. Core: negotiation_vectors, negotiation_completion | 72 advertisement readback and compatible link partner. Case `owned-apply`, `100-full-only` |
| `configure_forced` | BASIC/ENHANCED | Core 18. Gates forced_physts. | CrossoverSettings / ForcedLinkSettings | `crossover` | `configuration desired`, `configuration observed`, `configuration validate`, `configuration apply`, `configuration verify`, `configuration invalidate`, `cached settings` | `result` (admitted/terminal for operations) | example_cli_network, cli_fixture_transcript, cli_processor_routes, native_cli_admission, native_cli_reconciliation. Core: forced_matrix, forced_failures | Case coverage for paired speed and duplex includes `forced-10-half`, `forced-10-full`, `forced-100-half`, `forced-100-full`. Manual Q11 requires separate crossover, mirror and extended-duplex measurements. |
| `next_page_exchange` | BASIC/ENHANCED | Core 17. Gates next_page_handshake. | ConfigureOptions::nextPages / startConfigure | `next_page` | `configuration validate`, `configuration apply` (bounded AN pages) | `result` (admitted/terminal for operations) | cli_fixture_transcript (packed apply and cleanup), cli_processor_routes (decoded pages). Core: negotiation_pages, negotiation_page_failures | Manual Q10. No runner case drives a next-page-capable partner through exchange and re-AN. |
| `observe_link` | BASIC/ENHANCED | Core 19. Gates forced_physts. | readLinkStatus / LinkSnapshot | `link` | `link read`, `link partner`, `link cached`, `cached link`, `link watch` | `result`, `sample` (current, stale and incomplete evidence) | cli_fixture_transcript, native_cli_link_events, cli_processor_cached. Core: link_read_plan, link_transitions | Q11 and 72: partner/link resolution and cable transitions. Case `unplug-replug`, `wrong-policy` |
| `configure_fast_drop` | BASIC/ENHANCED, MII activation | Core 34. Gates fld_reset_defaults, fld_threshold_recipe. | QualitySettings / startConfigureQuality / qualitySnapshot | `fast_drop` | `controls quality`, `controls quality-status` | `result` (admitted/terminal and verified or partial policy) | cli_pin_transcript. Core: quality_vectors, quality_collector | Q18 and 73: MII drop latency and strap readback. Case `quality` |
| `loopback_near` | BASIC/ENHANCED | Core 23. Gates loopback_recipe. | startLoopback / stopLoopback | `loopback` | `diagnostics loopback-start`, `diagnostics loopback-stop`, `diagnostics loopback-status` | `result` (admission, activation and separate stop evidence) | cli_diagnostic_transcript. Core: loopback_matrix, loopback_barriers | Q8: mode routing and restoration. Case `loopback-mii`, `loopback-mii-line`, `loopback-pcs-in`, `loopback-pcs-out`, `loopback-analog` |
| `loopback_digital` | BASIC/ENHANCED | Core 23. Gates loopback_recipe. | startLoopback / stopLoopback | `loopback` | `diagnostics loopback-start`, `diagnostics loopback-stop`, `diagnostics loopback-status` | `result` (admission, activation and separate stop evidence) | cli_diagnostic_transcript. Core: loopback_matrix, loopback_barriers | Q8: mode routing and restoration. Case `loopback-digital` |
| `loopback_reverse` | BASIC/ENHANCED | Core 23. Gates loopback_recipe. | startLoopback / stopLoopback | `loopback` | `diagnostics loopback-start`, `diagnostics loopback-stop`, `diagnostics loopback-status` | `result` (admission, activation and separate stop evidence) | cli_diagnostic_transcript. Core: loopback_matrix, loopback_barriers | Q8: mode routing and restoration. Case `loopback-reverse` |
| `loopback_external` | BASIC/ENHANCED | Core 23. Gates none named. | startLoopback / stopLoopback | `loopback` | `diagnostics loopback-start`, `diagnostics loopback-stop`, `diagnostics loopback-status` | `result` (admission, activation and separate stop evidence) | cli_diagnostic_transcript. Core: loopback_matrix, loopback_barriers | Q8: mode routing and restoration. Case `loopback-external` |
| `bist_run` | BASIC/ENHANCED | Core 24. Gates bist_result_semantics. | startBist / startBistAcquire / stopBist / bistSnapshot | `bist` | `diagnostics bist-start`, `diagnostics bist-read`, `diagnostics bist-stop`, `diagnostics bist-status` | `result` (activation, retained capture and separate stop evidence) | cli_diagnostic_transcript. Core: bist_matrix, bist_operation_failures | 73 instrumented packet and error counters. Case `bist-off`, `bist-single`, `bist-checker`, `bist-both`, `bist-digital`, `bist-analog`, `bist-external` |
| `bist_capture` | BASIC/ENHANCED | Core 24. Gates bist_result_semantics. | startBist / startBistAcquire / stopBist / bistSnapshot | `bist` | `diagnostics bist-start`, `diagnostics bist-read`, `diagnostics bist-stop`, `diagnostics bist-status` | `result` (activation, retained capture and separate stop evidence) | cli_diagnostic_transcript. Core: bist_measurements, bist_acquire_failures | 73 instrumented packet and error counters. Case `bist-off`, `bist-single`, `bist-checker`, `bist-both`, `bist-digital`, `bist-analog`, `bist-external` |
| `cable_configure` | BASIC/ENHANCED | Core 25. Gates tdr_calibration, tdr_recipe. | startCableTest / startCableAcquire / stopCableTest / cableSnapshot / interpretCable | `cable` | `diagnostics cable-start`, `diagnostics cable-raw`, `diagnostics cable-read`, `diagnostics cable-stop`, `diagnostics cable-status`, `diagnostics cable-interpret` | `result` (partial capture, validity and independent stop or interpretation) | cli_diagnostic_transcript. Core: cable_matrix, cable_capture, autorun_policy | Q7: known open, short and good cables. Case `cable-manual`, `cable-raw` |
| `cable_manual` | BASIC/ENHANCED | Core 25. Gates tdr_freshness_abort, tdr_calibration. | startCableTest / startCableAcquire / stopCableTest / cableSnapshot / interpretCable | `cable` | `diagnostics cable-start`, `diagnostics cable-raw`, `diagnostics cable-read`, `diagnostics cable-stop`, `diagnostics cable-status`, `diagnostics cable-interpret` | `result` (partial capture, validity and independent stop or interpretation) | cli_diagnostic_transcript. Core: cable_matrix, cable_capture, autorun_policy | Q7: known open, short and good cables. Case `cable-manual`, `cable-raw` |
| `cable_autorun` | BASIC/ENHANCED | Core 26. Gates tdr_freshness_abort. | startConfigureCableAutorun / cableAutorunSnapshot / startCableAutorunAcquire | `cable` | `diagnostics cable-autorun`, `diagnostics cable-autorun-read`, `diagnostics cable-autorun-status` | `result` (verified policy and uncorrelated raw words) | cli_diagnostic_transcript. Core: autorun_policy, autorun_acquisition | Q7: known open, short and good cables. Case `cable-autorun` |
| `cable_capture` | BASIC/ENHANCED | Core 25. Gates tdr_calibration, tdr_freshness_abort, tdr_reserved. | startCableTest / startCableAcquire / stopCableTest / cableSnapshot / interpretCable | `cable` | `diagnostics cable-start`, `diagnostics cable-raw`, `diagnostics cable-read`, `diagnostics cable-stop`, `diagnostics cable-status`, `diagnostics cable-interpret` | `result` (partial capture, validity and independent stop or interpretation) | cli_diagnostic_transcript. Core: cable_capture, cable_interpretation | Q7: known open, short and good cables. Case `cable-manual`, `cable-raw` |
| `collect_event_counters` | BASIC/ENHANCED | Core 20. Gates recr_width, rmii_rc, forced_physts, rxfs_sfd_reference. | collectEvents / CounterSnapshot / evidenceSnapshot / clearEvidence | `counters` | `link collect`, `cached events` | `result` (new collector receipts and retained partial samples) | cli_fixture_transcript, native_cli_link_events. Core: events_vectors, events_failures | Case `irq` checks a guarded RECR sample of zero. Manual Q16 requires invalid-symbol boundary injection. Manual Q17 requires a controlled RMII clock offset and FIFO polarity, clear and rearm measurements. |
| `interrupt_configuration` | BASIC/ENHANCED | Core 27. Gates rxfs_sfd_reference. | startConfigureInterrupts / interruptSnapshot / serviceInterrupts / interruptServiceSnapshot / startInterruptTest / stopInterruptTest | `interrupts` | `irq configure`, `irq service`, `irq cached-service`, `irq status` | `result` (verified policy or retained partial collector evidence) | cli_control_transcript. Core: interrupt_policy, interrupt_stimulus | Q9: IRQ latching and delivery. Case `irq` |
| `interrupt_test` | BASIC/ENHANCED | Core 27. Gates none named. | startInterruptTest / stopInterruptTest / interruptSnapshot / serviceInterrupts | `interrupts` | `irq test-start`, `irq test-stop`, `irq status` | `result` (stimulus state, effects and stop evidence) | cli_control_transcript. Core: interrupt_stimulus, interrupt_ownership | Q9: IRQ latching and delivery. Case `irq` |
| `signal_quality_sample` | BASIC/ENHANCED | Core 34. Gates mse_address. | readSignalQuality / signalQualitySnapshot | `quality` | `controls mse`, `controls mse-status` | `result` (raw MSE and prior link context) | cli_pin_transcript. Core: quality_raw, quality_collector | 73 instrumented signal and error exposure. Case `mse` |
| `calibration_recovery` | BASIC/ENHANCED | Core 35. Gates hard_reset_straps. | startCalibrationRecovery | `calibration` | `diagnostics calibration-recover`, `diagnostics electrical-status` | `result` (CDCR observation, HARD result and replay debt) | cli_diagnostic_transcript. Core: electrical_calibration, electrical_lifecycle | 73 identified hardware and resistor calibration recovery. Case `calibration-recovery` |
| `led_configure` | BASIC/ENHANCED | Core 32. Gates pin_fields. | configurePins / pinsSnapshot | `led` | `controls pins`, `controls pins-status` | `result` (attributed terminal effects and stable word validity) | cli_pin_transcript. Core: pins_vectors, pins_failures | Q12: LED source, polarity and drive. Case `pins`, `pins-basic` |
| `pin_configure` | BASIC/ENHANCED | Core 32. Gates pin_fields. | configurePins / pinsSnapshot | `pin_mux` | `controls pins`, `controls pins-status` | `result` (attributed terminal effects and stable word validity) | cli_pin_transcript. Core: pins_vectors, pins_routing | Q12: pin roles and electrical behavior. Case `pins`, `pins-basic` |
| `clock_configure` | BASIC/ENHANCED | Core 33. Gates pin_fields. | InterfaceSettings / configureInterface / interfaceSnapshot | `clock` | `controls interface`, `controls interface-status` | `result` (one owner, terminal effects and observed register validity) | cli_pin_transcript. Core: interface_timing | Q12 and 73: clock rate, pin role and output loading. Case `interface-mii`, `interface-rmii25`, `interface-rmii50` |
| `mii_configure` | BASIC/ENHANCED, MII | Core 33. Gates rmii_rc. | InterfaceSettings / configureInterface / interfaceSnapshot | `mii` | `controls interface`, `controls interface-status` | `result` (one owner, terminal effects and observed register validity) | cli_pin_transcript. Core: interface_vectors, interface_admission | 73 MII edges, pin functions and isolation. Case `interface-mii` |
| `rmii_configure` | BASIC/ENHANCED, RMII | Core 33. Gates rmii_rc. | InterfaceSettings / configureInterface / interfaceSnapshot | `rmii` | `controls interface`, `controls interface-status` | `result` (one owner, terminal effects and observed register validity) | cli_pin_transcript. Core: interface_vectors, interface_collector | Q17 and 73: RMII clock and status acquisition. Case `interface-rmii25`, `interface-rmii50` |
| `repeater_configure` | BASIC/ENHANCED, RMII | Core 33. Gates rmii_rc. | InterfaceSettings / configureInterface / interfaceSnapshot | `repeater` | `controls interface`, `controls interface-status` (apply on each admitted side) | `result` (one owner, terminal effects and observed register validity) | cli_pin_transcript. Core: interface_repeater | 73 two PHYs and one qualified RMII reference. Case `repeater-leader`, `repeater-follower` |
| `tx100_configure` | BASIC/ENHANCED | Core 34. Gates none named. | QualitySettings / startConfigureQuality / qualitySnapshot | `tx100` | `controls quality`, `controls quality-status` | `result` (admitted/terminal and verified or partial policy) | cli_pin_transcript. Core: quality_vectors, quality_failures | 73 100M partner and calibrated data-path instrumentation. Case `quality` |
| `tx10_configure` | BASIC/ENHANCED | Core 34. Gates analog_rx_gain. | QualitySettings / startConfigureQuality / qualitySnapshot | `tx10` | `controls quality`, `controls quality-status` | `result` (admitted/terminal and verified or partial policy) | cli_pin_transcript. Core: quality_vectors, quality_admission | 73 10M partner and calibrated receiver/pattern checks. Case `quality` |
| `analog_configure` | BASIC/ENHANCED | Core 35. Gates ioctrl_example, analog_rx_gain. | AnalogSettings / startConfigureAnalog / analogSnapshot | `analog` | `controls analog`, `controls analog-status` | `result` (admitted/terminal and verified or partial stable words) | cli_pin_transcript. Core: analog_vectors, analog_failures | 73 calibrated amplitude, termination and rise/fall. Case `analog` |
| `compatibility_configure` | BASIC/ENHANCED | Core 33. Gates none named. | InterfaceSettings / configureInterface / interfaceSnapshot | `compatibility` | `controls interface`, `controls interface-status` | `result` (one owner, terminal effects and observed register validity) | cli_pin_transcript. Core: interface_vectors, interface_replay | 73 MAC traffic, IPG and compatibility. Case `interface-rmii25`, `interface-rmii50` |
| `link_recovery_configure` | BASIC/ENHANCED | Core 34. Gates none named. | QualitySettings / startConfigureQuality / qualitySnapshot | `link_recovery` | `controls quality`, `controls quality-status` | `result` (admitted/terminal and verified or partial policy) | cli_pin_transcript. Core: quality_vectors, quality_replay | 73 timestamped disturbance and recovery. Case `quality` |
| `extended_link_configure` | BASIC/ENHANCED | Core 18. Gates forced_physts. | LinkSettings / startConfigure | `crossover` | `configuration desired`, `configuration observed`, `configuration validate`, `configuration apply`, `configuration verify`, `configuration invalidate`, `cached settings` | `result` (admitted/terminal for operations) | cli_fixture_transcript, cli_processor_routes, native_cli_admission, native_cli_reconciliation. Core: forced_matrix, forced_ownership | Q11: forced link, automatic crossover and mirror. Case `forced-100-full`. Base forced profile only. Q11 requires separate mirror/crossover and duplex-extension measurements. |
| `isolation_configure` | BASIC/ENHANCED | Core 33. Gates none named. | InterfaceSettings / configureInterface / interfaceSnapshot | `mii` | `controls interface`, `controls interface-status` | `result` (one owner, terminal effects and observed register validity) | cli_pin_transcript. Core: interface_vectors, interface_admission | 73 MII edges, pin functions and isolation. Case `interface-mii` |
| `collision_test` | BASIC/ENHANCED, MII | Core 35. Gates none named. | startCollisionTest / stopCollisionTest / electricalSnapshot | `compliance` | `diagnostics collision-start`, `diagnostics collision-stop`, `diagnostics electrical-status` | `result` (admitted/terminal, uncertain stimulus and separate cleanup) | cli_diagnostic_transcript. Core: electrical_collision, electrical_lifecycle | 73 MII TX_EN and COL instrumentation. Case `collision-10`, `collision-100` |
| `compliance_session` | BASIC/ENHANCED | Core 35. Gates compt_reserved. | startCompliance / stopCompliance / electricalSnapshot | `compliance` | `diagnostics compliance-start`, `diagnostics compliance-stop`, `diagnostics electrical-status` | `result` (admitted/terminal, uncertain stimulus and separate cleanup) | cli_diagnostic_transcript. Core: electrical_traces, electrical_collision | Q19 and 73: calibrated stimulus and recovery. Case `compliance-tx100`, `compliance-nlp10`, `compliance-prbs10`, `compliance-pattern10` |
| `power_ieee` | BASIC/ENHANCED | Core 28. Gates none named. | PowerSettings / startConfigurePower / readPowerStatus / powerSnapshot | `power` | `power configure`, `power read`, `power status` | `result` (admitted/terminal, current validity and retained policy) | cli_control_transcript. Core: power_policy, power_transitions | Q6: power pin, software state and generated clock. Case `power-ieee` |
| `power_active_sleep` | BASIC/ENHANCED | Core 28. Gates none named. | PowerSettings / startConfigurePower / readPowerStatus / powerSnapshot | `power` | `power configure`, `power read`, `power status` | `result` (admitted/terminal, current validity and retained policy) | cli_control_transcript. Core: power_policy, power_transitions | Q6: power pin, software state and generated clock. Case `power-sleep` |
| `power_pll_control` | BASIC/ENHANCED | Core 28. Gates none named. | PowerSettings / startConfigurePower / readPowerStatus / powerSnapshot | `power` | `power configure`, `power read`, `power status` | `result` (admitted/terminal, current validity and retained policy) | cli_control_transcript. Core: power_policy, power_transitions | Q6: power pin, software state and generated clock. Case `power-pll` |
| `power_deep` | BASIC/ENHANCED | Core 28. Gates deep_power_exit. | PowerSettings / startConfigurePower / readPowerStatus / powerSnapshot | `power` | `power configure`, `power read`, `power status` (deep-power request remains source-gated) | `result` (admitted/terminal, current validity and retained policy) | cli_control_transcript. Core: power_policy (gated rejection) | Q6. Deep-power exit remains source gated. Case `power-deep` (BLOCKED) |
| `eee_initialize` | BASIC/ENHANCED | Core 29. Gates eee_wire, eee_control_semantics. | startConfigureEee / readEeeStatus / eeeSnapshot | `eee` | `eee configure`, `eee read`, `eee status` | `result` (admitted/terminal, raw negotiation words and retained policy) | cli_control_transcript. Core: eee_recipe, eee_acquisition_failures | Q1: exact DEVAD and EEE partner. Case `eee` |
| `eee_negotiation_status` | BASIC/ENHANCED | Core 29. Gates eee_wire, eee_control_semantics. | startConfigureEee / readEeeStatus / eeeSnapshot | `eee` | `eee configure`, `eee read`, `eee status` | `result` (admitted/terminal, raw negotiation words and retained policy) | cli_control_transcript. Core: eee_status, eee_acquisition_failures | Q1: exact DEVAD and EEE partner. Case `eee` |
| `legacy_lpi` | BASIC/ENHANCED | Core 30. Gates eee_wire, eee_control_semantics. | startLpi / stopLpi | `lpi` | `lpi start`, `lpi stop`, `lpi status` | `result` (terminal request/uncertain state and separate clear) | cli_control_transcript. Core: lpi_traces, lpi_ownership | Q2: observe LPI on the line or at the partner. Case `lpi` |
| `wol_configure` | BASIC/ENHANCED | Core 31. Gates wol_bit_mask, wol_live_reconfigure, rxfs_sfd_reference. | WakeSettings / startConfigureWake / readWakeStatus / wakeSnapshot | `wol` | `wol configure`, `wol read`, `wol status` | `result` (terminal and event state without match values) | cli_control_transcript. Core: wol_vectors, wol_routing | Q3: wake frame and acknowledgement. Case `wol-magic`, `wol-secure`, `wol-pulse` |
| `wol_acknowledge` | BASIC/ENHANCED | Core 31. Gates none named. | acknowledgeWake / pollOperation / takeResult | `wol` | `wol acknowledge`, `wol read`, `wol status` | `result` (terminal and retained event evidence) | cli_control_transcript. Core: wol_acquisition, wol_failures | Q3: wake frame and acknowledgement. Case `wol-magic`, `wol-secure` |

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
remain Q1/Q7/Q11/Q19/Q10 respectively. The procedure rows identify the
implemented branches and the open source and fixture gates.

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
and exact bounds. The typed quality CLI exposes these operations. Physical MII detector
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

The CLI, offline campaigns and SDK adapter consume the procedure and field dispositions above.
Independent native lifecycle/transport/link integration can use the reviewed
implemented subset while these unrelated gates remain open.

## DP83826AI and DP83826AE

AI and AE share SNLS783A revision 4. Each test stem below runs twice, with
the `ai_` and `ae_` prefixes. The CLI records use the same commands and
terminal, sample and cleanup fields described above. The compiled fixture
executes both declarations. Native admission also requires the board owner's
feature grant. AI physical qualification is **NOT RUN**. AE physical
qualification is **NOT RUN**, including temperature-dependent behavior.
Both grades need identified boards for Strap11 and the FLD/EMC measurements.

### Procedures

| Procedure | Feature | Typed route | CLI commands | Grade test stem |
| --- | --- | --- | --- | --- |
| `identify` | `identity` | readIdentity / identitySnapshot / capabilities | `connection identity`, `connection probe`, `connection discover`, `capabilities` | `identity_policy` |
| `direct_access` | `access` | readClause22 / writeClause22 / readMmd / writeMmd | `registers read`, `registers dump`, `registers bmcr` (raw LEDCR writes fenced) | `reset_access` |
| `mmd_access` | `access` | readClause22 / writeClause22 / readMmd / writeMmd / modifyMmd | `registers read`, `registers dump` (raw LEDCR writes and RMW fenced) | `reset_access` |
| `pcs_reset` | `reset` | startInitialize({PCS}) | `recovery reset` (PCS) | `reset_traces` |
| `hard_reset` | `reset` | startInitialize({HARD}) | `recovery reset` (hard) | `reset_traces` |
| `restart` | `reset` | startInitialize({RESTART}) | `recovery restart` | `reset_traces` |
| `external_reset` | `reset` | invalidateDeviceState(EXTERNAL_RESET / EXTERNAL_POWER_ON), then startInitialize | `recovery reinitialize` (after owner attestation) | `reset_traces` |
| `initialize_10base` | `initialization` | startInitialize | `connection initialize`, `recovery reinitialize` | `operation_initialization` |
| `configure_an` | `negotiation` | LinkSettings / startConfigure | `configuration desired`, `configuration observed`, `configuration validate`, `configuration apply`, `configuration verify`, `configuration invalidate`, `cached settings` | `negotiation_vectors` |
| `configure_forced` | `crossover` | CrossoverSettings / ForcedLinkSettings | `configuration desired`, `configuration observed`, `configuration validate`, `configuration apply`, `configuration verify`, `configuration invalidate`, `cached settings` | `forced_matrix` |
| `next_page_exchange` | `next_page` | ConfigureOptions::nextPages / startConfigure | `configuration validate`, `configuration apply` (bounded AN pages) | `negotiation_pages` |
| `observe_link` | `link` | readLinkStatus / LinkSnapshot | `link read`, `link partner`, `link cached`, `cached link`, `link watch` | `link_coherence` |
| `configure_fast_drop` | `fast_drop` | QualitySettings / startConfigureQuality / qualitySnapshot | `controls quality`, `controls quality-status` | `quality` |
| `loopback_near` | `loopback` | startLoopback / stopLoopback | `diagnostics loopback-start`, `diagnostics loopback-stop`, `diagnostics loopback-status` | `loopback_matrix` |
| `loopback_digital` | `loopback` | startLoopback / stopLoopback | `diagnostics loopback-start`, `diagnostics loopback-stop`, `diagnostics loopback-status` | `loopback_matrix` |
| `loopback_reverse` | `loopback` | startLoopback / stopLoopback | `diagnostics loopback-start`, `diagnostics loopback-stop`, `diagnostics loopback-status` | `loopback_matrix` |
| `loopback_external` | `loopback` | startLoopback / stopLoopback | `diagnostics loopback-start`, `diagnostics loopback-stop`, `diagnostics loopback-status` | `loopback_matrix` |
| `bist_run` | `bist` | startBist / startBistAcquire / stopBist / bistSnapshot | `diagnostics bist-start`, `diagnostics bist-read`, `diagnostics bist-stop`, `diagnostics bist-status` | `bist_measurements` |
| `bist_capture` | `bist` | startBist / startBistAcquire / stopBist / bistSnapshot | `diagnostics bist-start`, `diagnostics bist-read`, `diagnostics bist-stop`, `diagnostics bist-status` | `bist_acquire_failures` |
| `cable_configure` | `cable` | startCableTest / startCableAcquire / stopCableTest / cableSnapshot / interpretCable | `diagnostics cable-start`, `diagnostics cable-raw`, `diagnostics cable-read`, `diagnostics cable-stop`, `diagnostics cable-status`, `diagnostics cable-interpret` | `cable_matrix` |
| `cable_manual` | `cable` | startCableTest / startCableAcquire / stopCableTest / cableSnapshot / interpretCable | `diagnostics cable-start`, `diagnostics cable-raw`, `diagnostics cable-read`, `diagnostics cable-stop`, `diagnostics cable-status`, `diagnostics cable-interpret` | `cable_matrix` |
| `cable_autorun` | `cable` | startConfigureCableAutorun / cableAutorunSnapshot / startCableAutorunAcquire | `diagnostics cable-autorun`, `diagnostics cable-autorun-read`, `diagnostics cable-autorun-status` | `autorun_failures` |
| `cable_capture` | `cable` | startCableTest / startCableAcquire / stopCableTest / cableSnapshot / interpretCable | `diagnostics cable-start`, `diagnostics cable-raw`, `diagnostics cable-read`, `diagnostics cable-stop`, `diagnostics cable-status`, `diagnostics cable-interpret` | `cable_capture` |
| `collect_event_counters` | `counters` | collectEvents / CounterSnapshot / evidenceSnapshot / clearEvidence | `link collect`, `cached events` | `events_failures` |
| `interrupt_configuration` | `interrupts` | startConfigureInterrupts / interruptSnapshot / serviceInterrupts / interruptServiceSnapshot / startInterruptTest / stopInterruptTest | `irq configure`, `irq service`, `irq cached-service`, `irq status` | `interrupt_a_routes` |
| `interrupt_test` | `interrupts` | startInterruptTest / stopInterruptTest / interruptSnapshot / serviceInterrupts | `irq test-start`, `irq test-stop`, `irq status` | `interrupt_stimulus` |
| `signal_quality_sample` | `quality` | readSignalQuality / signalQualitySnapshot | `controls mse`, `controls mse-status` | `quality_raw` |
| `calibration_recovery` | `calibration` | startCalibrationRecovery | `diagnostics calibration-recover`, `diagnostics electrical-status` | `electrical_calibration` |
| `led_configure` | `led` | configurePins / pinsSnapshot | `controls pins`, `controls pins-status` | `pins_a_pins` |
| `pin_configure` | `pin_mux` | configurePins / pinsSnapshot | `controls pins`, `controls pins-status` | `pins_a_pins` |
| `clock_configure` | `clock` | InterfaceSettings / configureInterface / interfaceSnapshot | `controls interface`, `controls interface-status` | `interface` |
| `mii_configure` | `mii` | InterfaceSettings / configureInterface / interfaceSnapshot | `controls interface`, `controls interface-status` | `interface` |
| `rmii_configure` | `rmii` | InterfaceSettings / configureInterface / interfaceSnapshot | `controls interface`, `controls interface-status` | `interface` |
| `repeater_configure` | `repeater` | InterfaceSettings / configureInterface / interfaceSnapshot | `controls interface`, `controls interface-status` (apply on each admitted side) | `interface` |
| `tx100_configure` | `tx100` | QualitySettings / startConfigureQuality / qualitySnapshot | `controls quality`, `controls quality-status` | `quality_vectors` |
| `tx10_configure` | `tx10` | QualitySettings / startConfigureQuality / qualitySnapshot | `controls quality`, `controls quality-status` | `quality_vectors` |
| `analog_configure` | `analog` | AnalogSettings / startConfigureAnalog / analogSnapshot | `controls analog`, `controls analog-status` | `analog_vectors` |
| `compatibility_configure` | `compatibility` | InterfaceSettings / configureInterface / interfaceSnapshot | `controls interface`, `controls interface-status` | `interface` |
| `link_recovery_configure` | `link_recovery` | QualitySettings / startConfigureQuality / qualitySnapshot | `controls quality`, `controls quality-status` | `quality` |
| `extended_link_configure` | `crossover` | LinkSettings / startConfigure | `configuration desired`, `configuration observed`, `configuration validate`, `configuration apply`, `configuration verify`, `configuration invalidate`, `cached settings` | `forced_matrix` |
| `isolation_configure` | `mii` | InterfaceSettings / configureInterface / interfaceSnapshot | `controls interface`, `controls interface-status` | `interface` |
| `collision_test` | `compliance` | startCollisionTest / stopCollisionTest / electricalSnapshot | `diagnostics collision-start`, `diagnostics collision-stop`, `diagnostics electrical-status` | `electrical_collision` |
| `compliance_session` | `compliance` | startCompliance / stopCompliance / electricalSnapshot | `diagnostics compliance-start`, `diagnostics compliance-stop`, `diagnostics electrical-status` | `electrical_traces` |
| `power_ieee` | `power` | PowerSettings / startConfigurePower / readPowerStatus / powerSnapshot | `power configure`, `power read`, `power status` | `power_policy` |
| `power_active_sleep` | `power` | PowerSettings / startConfigurePower / readPowerStatus / powerSnapshot | `power configure`, `power read`, `power status` | `power_policy` |
| `power_pll_control` | `power` | PowerSettings / startConfigurePower / readPowerStatus / powerSnapshot | `power configure`, `power read`, `power status` | `power_policy` |
| `power_deep` | `power` | PowerSettings / startConfigurePower / readPowerStatus / powerSnapshot | `power configure`, `power read`, `power status` (deep-power request remains source-gated) | `power_policy` |
| `eee_initialize` | `eee` | startConfigureEee / readEeeStatus / eeeSnapshot | `eee configure`, `eee read`, `eee status` | `eee_recipe` |
| `eee_negotiation_status` | `eee` | startConfigureEee / readEeeStatus / eeeSnapshot | `eee configure`, `eee read`, `eee status` | `eee_status` |
| `legacy_lpi` | `lpi` | startLpi / stopLpi | `lpi start`, `lpi stop`, `lpi status` | `lpi_traces` |
| `wol_configure` | `wol` | WakeSettings / startConfigureWake / readWakeStatus / wakeSnapshot | `wol configure`, `wol read`, `wol status` | `wol_a_wake` |
| `wol_acknowledge` | `wol` | acknowledgeWake / pollOperation / takeResult | `wol acknowledge`, `wol read`, `wol status` | `wol_a_wake` |

### Features and qualification cases

The named gates carry the exact evidence limits in the [A procedure ledger](registers/dp83826a-procedures.json). Deep-power exit and the 100M pattern session that touches 0428 remain blocked. The TX100 compliance script has a distinct admitted path. IRQ and wake use the single destructive collector. Pin 31 has a POR observation and gated mux codes. The two original reserved-field writers have no A route.

| Feature | Procedures | Gates | Fixture cases or application boundary |
| --- | --- | --- | --- |
| `identity` | `identify` | None | 72 cold boot: identify both modes and strapped address. Case `cold-boot` |
| `access` | `direct_access`, `mmd_access` | `eee_wire` | Q4 and 73: C22/MMD alias readback. Case `cable-autorun`, `observe`, `10base-te` |
| `reset` | `pcs_reset`, `hard_reset`, `restart`, `external_reset` | `hard_reset_straps`, `pcs_reset_extent` | Q5 and 72: reset, strap and SMI readiness traces. Case `pcs`, `hard`, `restart`, `cold-boot` |
| `initialization` | `initialize_10base` | None | 72 cold boot and mandatory VOD readback. Case `10base-te`, `cold-boot` |
| `negotiation` | `configure_an` | None | 72 advertisement readback and compatible link partner. Case `owned-apply`, `100-full-only` |
| `next_page` | `next_page_exchange` | `next_page_handshake` | Manual Q10. No runner case drives a next-page-capable partner through exchange and re-AN. |
| `crossover` | `configure_forced`, `extended_link_configure` | `forced_physts` | Case coverage for paired speed and duplex includes `forced-10-half`, `forced-10-full`, `forced-100-half`, `forced-100-full`. Manual Q11 requires separate crossover, mirror and extended-duplex measurements. |
| `link` | `observe_link` | `forced_physts` | Q11 and 72: partner/link resolution and cable transitions. Case `unplug-replug`, `wrong-policy` |
| `eee` | `eee_initialize`, `eee_negotiation_status` | `eee_wire`, `eee_control_semantics` | Q1: exact DEVAD and EEE partner. Case `eee` |
| `lpi` | `legacy_lpi` | `eee_wire`, `eee_control_semantics` | Q2: observe LPI on the line or at the partner. Case `lpi` |
| `wol` | `wol_configure`, `wol_acknowledge` | `wol_bit_mask`, `wol_live_reconfigure`, `rxfs_sfd_reference` | Q3: wake frame and acknowledgement. Case `wol-magic`, `wol-secure`, `wol-pulse` |
| `power` | `power_ieee`, `power_active_sleep`, `power_pll_control`, `power_deep` | `deep_power_exit` | Q6: power pin, software state and generated clock. Case `power-ieee`, `power-sleep`, `power-pll`, `power-deep` |
| `repeater` | `repeater_configure` | `rmii_rc` | 73 two PHYs and one qualified RMII reference. Case `repeater-leader`, `repeater-follower` |
| `clock` | `clock_configure` | `pin_fields` | Q12 and 73: clock rate, pin role and output loading. Case `interface-mii`, `interface-rmii25`, `interface-rmii50` |
| `mii` | `mii_configure`, `isolation_configure` | `rmii_rc` | 73 MII edges, pin functions and isolation. Case `interface-mii` |
| `rmii` | `rmii_configure` | `rmii_rc` | Q17 and 73: RMII clock and status acquisition. Case `interface-rmii25`, `interface-rmii50` |
| `tx100` | `tx100_configure` | None | 73 100M partner and calibrated data-path instrumentation. Case `quality` |
| `tx10` | `tx10_configure` | `analog_rx_gain` | 73 10M partner and calibrated receiver/pattern checks. Case `quality` |
| `loopback` | `loopback_near`, `loopback_digital`, `loopback_reverse`, `loopback_external` | `loopback_recipe` | Q8: mode routing and restoration. Case `loopback-mii`, `loopback-mii-line`, `loopback-pcs-in`, `loopback-pcs-out`, `loopback-analog`, `loopback-digital`, `loopback-reverse`, `loopback-external` |
| `bist` | `bist_run`, `bist_capture` | `bist_result_semantics` | 73 instrumented packet and error counters. Case `bist-off`, `bist-single`, `bist-checker`, `bist-both`, `bist-digital`, `bist-analog`, `bist-external` |
| `cable` | `cable_configure`, `cable_manual`, `cable_autorun`, `cable_capture` | `tdr_reserved`, `tdr_calibration`, `tdr_freshness_abort`, `tdr_recipe` | Q7: known open, short and good cables. Case `cable-manual`, `cable-raw`, `cable-autorun` |
| `fast_drop` | `configure_fast_drop` | `fld_reset_defaults`, `fld_threshold_recipe` | Q18 and 73: MII drop latency and strap readback. Case `quality` |
| `link_recovery` | `link_recovery_configure` | None | 73 timestamped disturbance and recovery. Case `quality` |
| `led` | `led_configure` | `pin_fields` | Q12: LED source, polarity and drive. Case `pins`, `pins-basic` |
| `pin_mux` | `pin_configure` | `pin_fields` | Q12: pin roles and electrical behavior. Case `pins`, `pins-basic` |
| `interrupts` | `interrupt_configuration`, `interrupt_test` | `rxfs_sfd_reference` | Q9: IRQ latching and delivery. Case `irq` |
| `counters` | `collect_event_counters` | `recr_width`, `rmii_rc`, `rxfs_sfd_reference`, `forced_physts` | Case `irq` checks a guarded RECR sample of zero. Manual Q16 requires invalid-symbol boundary injection. Manual Q17 requires a controlled RMII clock offset and FIFO polarity, clear and rearm measurements. |
| `quality` | `signal_quality_sample` | `mse_address` | 73 instrumented signal and error exposure. Case `mse` |
| `compliance` | `collision_test`, `compliance_session` | `compt_reserved` | Q19 and 73: calibrated stimulus and recovery. Case `collision-10`, `collision-100`, `compliance-tx100`, `compliance-nlp10`, `compliance-prbs10`, `compliance-pattern10` |
| `analog` | `analog_configure` | `ioctrl_example`, `analog_rx_gain` | 73 calibrated amplitude, termination and rise/fall. Case `analog` |
| `compatibility` | `compatibility_configure` | None | 73 MAC traffic, IPG and compatibility. Case `interface-rmii25`, `interface-rmii50` |
| `calibration` | `calibration_recovery` | `hard_reset_straps` | 73 identified hardware and resistor calibration recovery. Case `calibration-recovery` |
| `straps` | See field disposition | `hard_reset_straps` | Q5 and 72: identified straps and reset. Case `cold-boot`, `hard` |
| `outside_phy` | See field disposition | None | N/A. Application owns platform and stack |
| `absent` | See field disposition | None | N/A. Exact part excludes these functions |

### Register fields

Each row gives the SNLS783A table, its register address, and the field masks grouped by feature owner. The feature and procedure rows above give the typed calls, CLI routes and checks. The [A field ledger](registers/dp83826a.json) specifies access, reset knowledge, legal encodings and read effects for each field. Only selected masks within a feature are writable. PHYCR[5] and IO_CFG1[6] are reserved. The LED3 drive fields belong to the LED owner.

| Register | Space and address | TI table | Feature and field masks |
| --- | --- | --- | --- |
| `BMCR` | C22 0x0000 | 8-20 | reset &mdash; `BMCR.reset=0x8000`<br>loopback &mdash; `BMCR.loopback=0x4000`<br>crossover &mdash; `BMCR.speed=0x2000`, `BMCR.duplex=0x0100`<br>negotiation &mdash; `BMCR.an_enable=0x1000`, `BMCR.an_restart=0x0200`<br>power &mdash; `BMCR.power_down=0x0800`<br>mii &mdash; `BMCR.isolate=0x0400`<br>compliance &mdash; `BMCR.collision_test=0x0080` |
| `BMSR` | C22 0x0001 | 8-21 | identity &mdash; `BMSR.t4=0x8000`, `BMSR.full100=0x4000`, `BMSR.half100=0x2000`, `BMSR.full10=0x1000`, `BMSR.half10=0x0800`, `BMSR.preamble=0x0040`, `BMSR.an_ability=0x0008`, `BMSR.extended=0x0001`<br>link &mdash; `BMSR.an_complete=0x0020`, `BMSR.remote_fault=0x0010`, `BMSR.link=0x0004`, `BMSR.jabber=0x0002` |
| `PHYIDR1` | C22 0x0002 | 8-22 | identity &mdash; `PHYIDR1.oui_high=0xFFFF` |
| `PHYIDR2` | C22 0x0003 | 8-23 | identity &mdash; `PHYIDR2.oui_low=0xFC00`, `PHYIDR2.model=0x03F0`, `PHYIDR2.revision=0x000F` |
| `ANAR` | C22 0x0004 | 8-24 | next_page &mdash; `ANAR.next_page=0x8000`<br>negotiation &mdash; `ANAR.remote_fault=0x2000`, `ANAR.asym_pause=0x0800`, `ANAR.pause=0x0400`, `ANAR.t4=0x0200`, `ANAR.full100=0x0100`, `ANAR.half100=0x0080`, `ANAR.full10=0x0040`, `ANAR.half10=0x0020`, `ANAR.selector=0x001F` |
| `ALNPAR` | C22 0x0005 | 8-25 | negotiation &mdash; `ALNPAR.next_page=0x8000`, `ALNPAR.ack=0x4000`, `ALNPAR.remote_fault=0x2000`, `ALNPAR.asym_pause=0x0800`, `ALNPAR.pause=0x0400`, `ALNPAR.t4=0x0200`, `ALNPAR.full100=0x0100`, `ALNPAR.half100=0x0080`, `ALNPAR.full10=0x0040`, `ALNPAR.half10=0x0020`, `ALNPAR.selector=0x001F` |
| `ANER` | C22 0x0006 | 8-26 | negotiation &mdash; `ANER.parallel_fault=0x0010`, `ANER.partner_an=0x0001`<br>next_page &mdash; `ANER.partner_next=0x0008`, `ANER.local_next=0x0004`, `ANER.page=0x0002` |
| `ANNPTR` | C22 0x0007 | 8-27 | next_page &mdash; `ANNPTR.next_page=0x8000`, `ANNPTR.message=0x2000`, `ANNPTR.ack2=0x1000`, `ANNPTR.toggle=0x0800`, `ANNPTR.code=0x07FF` |
| `ANLNPTR` | C22 0x0008 | 8-28 | next_page &mdash; `ANLNPTR.next_page=0x8000`, `ANLNPTR.ack=0x4000`, `ANLNPTR.message=0x2000`, `ANLNPTR.ack2=0x1000`, `ANLNPTR.toggle=0x0800`, `ANLNPTR.code=0x07FF` |
| `CR1` | C22 0x0009 | 8-29 | cable &mdash; `CR1.tdr_auto=0x0100`<br>link_recovery &mdash; `CR1.link_recovery=0x0080`<br>crossover &mdash; `CR1.robust_mdix=0x0020`<br>mii &mdash; `CR1.fast_rxdv=0x0002` |
| `CR2` | C22 0x000A | 8-30 | crossover &mdash; `CR2.extended_duplex=0x0020`<br>tx100 &mdash; `CR2.idle_rx_error=0x0004`<br>mii &mdash; `CR2.odd_nibble_disable=0x0002` |
| `CR3` | C22 0x000B | 8-31 | fast_drop &mdash; `CR3.descrambler_drop=0x0400`, `CR3.fast_drop_modes=0x000F`<br>crossover &mdash; `CR3.polarity_swap=0x0040`, `CR3.pair_swap=0x0020` |
| `REGCR` | C22 0x000D | 8-32 | access &mdash; `REGCR.function=0xC000`, `REGCR.devad=0x001F` |
| `ADDAR` | C22 0x000E | 8-33 | access &mdash; `ADDAR.address_data=0xFFFF` |
| `FLDS` | C22 0x000F | 8-34 | counters &mdash; `FLDS.causes=0x01F0` |
| `PHYSTS` | C22 0x0010 | 8-35 | link &mdash; `PHYSTS.mdix=0x4000`, `PHYSTS.rx_error=0x2000`, `PHYSTS.polarity=0x1000`, `PHYSTS.false_carrier=0x0800`, `PHYSTS.signal=0x0400`, `PHYSTS.descrambler=0x0200`, `PHYSTS.page=0x0100`, `PHYSTS.interrupt=0x0080`, `PHYSTS.remote_fault=0x0040`, `PHYSTS.jabber=0x0020`, `PHYSTS.an_complete=0x0010`, `PHYSTS.loopback=0x0008`, `PHYSTS.duplex=0x0004`, `PHYSTS.speed=0x0002`, `PHYSTS.link=0x0001` |
| `PHYSCR` | C22 0x0011 | 8-36 | power &mdash; `PHYSCR.disable_pll=0x8000`, `PHYSCR.power_save_enable=0x4000`, `PHYSCR.power_save_mode=0x3000`<br>tx100 &mdash; `PHYSCR.scrambler_bypass=0x0800`<br>loopback &mdash; `PHYSCR.loopback_fifo_depth=0x0300`<br>mii &mdash; `PHYSCR.collision_full_duplex=0x0010`<br>interrupts &mdash; `PHYSCR.interrupt_polarity=0x0008`, `PHYSCR.test_interrupt=0x0004`, `PHYSCR.interrupt_enable=0x0002`, `PHYSCR.interrupt_output_enable=0x0001` |
| `MISR1` | C22 0x0012 | 8-37 | interrupts &mdash; `MISR1.link_quality_changed_event=0x8000`, `MISR1.energy_changed_event=0x4000`, `MISR1.link_changed_event=0x2000`, `MISR1.speed_changed_event=0x1000`, `MISR1.duplex_changed_event=0x0800`, `MISR1.an_complete_event=0x0400`, `MISR1.false_carrier_half_full_event=0x0200`, `MISR1.receive_error_half_full_event=0x0100`, `MISR1.link_quality_changed_enable=0x0080`, `MISR1.energy_changed_enable=0x0040`, `MISR1.link_changed_enable=0x0020`, `MISR1.speed_changed_enable=0x0010`, `MISR1.duplex_changed_enable=0x0008`, `MISR1.an_complete_enable=0x0004`, `MISR1.false_carrier_half_full_enable=0x0002`, `MISR1.receive_error_half_full_enable=0x0001` |
| `MISR2` | C22 0x0013 | 8-38 | interrupts &mdash; `MISR2.eee_error_event=0x8000`, `MISR2.an_error_event=0x4000`, `MISR2.page_received_event=0x2000`, `MISR2.loopback_fifo_event=0x1000`, `MISR2.mdix_changed_event=0x0800`, `MISR2.sleep_event=0x0400`, `MISR2.polarity_or_wol_event=0x0200`, `MISR2.jabber_event=0x0100`, `MISR2.eee_error_enable=0x0080`, `MISR2.an_error_enable=0x0040`, `MISR2.page_received_enable=0x0020`, `MISR2.loopback_fifo_enable=0x0010`, `MISR2.mdix_changed_enable=0x0008`, `MISR2.sleep_enable=0x0004`, `MISR2.polarity_or_wol_enable=0x0002`, `MISR2.jabber_enable=0x0001` |
| `FCSCR` | C22 0x0014 | 8-39 | counters &mdash; `FCSCR.false_carrier_count=0x00FF` |
| `RECR` | C22 0x0015 | 8-40 | counters &mdash; `RECR.receive_error_count=0xFFFF` |
| `BISCR` | C22 0x0016 | 8-41 | bist &mdash; `BISCR_errorCounterMode=0x4000`, `BISCR_prbsMode=0x3000`, `BISCR_checkerLock=0x0800`, `BISCR_checkerSyncLoss=0x0400`, `BISCR_generatorActive=0x0200`, `BISCR_normalPower=0x0100`<br>loopback &mdash; `BISCR_miiLineTransmit=0x0040`, `BISCR_loopbackMode=0x001F` |
| `RCSR` | C22 0x0017 | 8-42 | rmii &mdash; `RCSR.tx_clock_shift=0x0100`, `RCSR.clock_select=0x0080`, `RCSR.rmii_mode=0x0020`, `RCSR.revision=0x0010`, `RCSR.elasticity=0x0003`<br>counters &mdash; `RCSR.overflow=0x0008`, `RCSR.underflow=0x0004` |
| `LEDCR` | C22 0x0018 | 8-43 | led &mdash; `LEDCR.blink=0x0600`, `LEDCR.polarity=0x0080`, `LEDCR.drive_enable=0x0010`, `LEDCR.drive_value=0x0002` |
| `PHYCR` | C22 0x0019 | 8-44 | crossover &mdash; `PHYCR.auto_mdix=0x8000`, `PHYCR.force_mdix=0x4000`<br>negotiation &mdash; `PHYCR.pause_rx=0x2000`, `PHYCR.pause_tx=0x1000`<br>link &mdash; `PHYCR.mii_link=0x0800`<br>led &mdash; `PHYCR.led_stretch=0x0080`<br>identity &mdash; `PHYCR.phy_address=0x001F` |
| `10BTSCR` | C22 0x001A | 8-45 | tx10 &mdash; `10BTSCR.lower_threshold_enable=0x2000`, `10BTSCR.squelch=0x1E00`, `10BTSCR.nlp_disable=0x0080`, `10BTSCR.jabber_disable=0x0001`<br>counters &mdash; `10BTSCR.polarity=0x0010` |
| `BICSR1` | C22 0x001B | 8-46 | bist &mdash; `BICSR1_errorBytes=0xFF00`, `BICSR1_ipgLength=0x00FF` |
| `BICSR2` | C22 0x001C | 8-47 | bist &mdash; `BICSR2_packetLength=0x07FF` |
| `CDCR` | C22 0x001E | 8-48 | cable &mdash; `CDCR_start=0x8000`, `CDCR_done=0x0002`, `CDCR_failed=0x0001`<br>calibration &mdash; `CDCR_rescalEnable=0x4000` |
| `PHYRCR` | C22 0x001F | 8-49 | reset &mdash; `PHYRCR.hard_reset=0x8000`, `PHYRCR.restart=0x4000` |
| `MLEDCR` | MMD 0x0025 | 8-50 | led &mdash; `MLEDCR.polarity_swap=0x0200`, `MLEDCR.source=0x0078`, `MLEDCR.route_enable=0x0001` |
| `COMPT` | MMD 0x0027 | 8-51 | compliance &mdash; `COMPT.pattern=0x000F` |
| `10M_CFG` | MMD 0x002A | 8-52 | tx10 &mdash; `10M_CFG.short_preamble=0x4000` |
| `FLD_CFG1` | MMD 0x0117 | 8-53 | fast_drop &mdash; `FLD_CFG1.window=0xFC00`, `FLD_CFG1.error_count=0x03F0` |
| `FLD_CFG2` | MMD 0x0131 | 8-54 | fast_drop &mdash; `FLD_CFG2.descrambler_window=0x003F` |
| `CDSCR` | MMD 0x0170 | 8-55 | cable &mdash; `CDSCR_crossDisable=0x4000`, `CDSCR_txChannel=0x2000`, `CDSCR_dcNoInit=0x1000`, `CDSCR_averageCycles=0x0700`, `CDSCR_segment=0x0070` |
| `CDSCR2` | MMD 0x0171 | 8-56 | No named fields |
| `CDSCR3` | MMD 0x0173 | 8-57 | cable &mdash; `CDSCR3_segmentDuration=0xFF00`, `CDSCR3_initialSkip=0x00FF` |
| `TDR_175` | MMD 0x0175 | 8-58 | cable &mdash; `TDR_175_shadowAverageLocation=0x3800`, `TDR_175_forwardShadow=0x000F` |
| `TDR_176` | MMD 0x0176 | 8-59 | cable &mdash; `TDR_176_peakLocationThreshold=0x001F` |
| `CDSCR4` | MMD 0x0177 | 8-60 | cable &mdash; `CDSCR4_shortCableThreshold=0x1F00` |
| `TDR_178` | MMD 0x0178 | 8-61 | cable &mdash; `TDR_178_pulseWidth=0x0007` |
| `CDLRR1` | MMD 0x0180 | 8-62 | cable &mdash; `CDLRR1_peakLocation=0x00FF` |
| `CDLRR2` | MMD 0x0181 | 8-63 | No named fields |
| `CDLRR3` | MMD 0x0182 | 8-64 | No named fields |
| `CDLRR4` | MMD 0x0183 | 8-65 | No named fields |
| `CDLRR5` | MMD 0x0184 | 8-66 | No named fields |
| `CDLAR1` | MMD 0x0185 | 8-67 | cable &mdash; `CDLAR1_peakAmplitude=0x007F` |
| `CDLAR2` | MMD 0x0186 | 8-68 | No named fields |
| `CDLAR3` | MMD 0x0187 | 8-69 | No named fields |
| `CDLAR4` | MMD 0x0188 | 8-70 | No named fields |
| `CDLAR5` | MMD 0x0189 | 8-71 | No named fields |
| `CDLAR6` | MMD 0x018A | 8-72 | cable &mdash; `CDLAR6_peakPolarity=0x0800`, `CDLAR6_crossDetect=0x0020` |
| `MSE_Val` | MMD 0x0218 | 8-73 | quality &mdash; `MSE_Val.mean_square_error=0xFFFF` |
| `IO_CFG1` | MMD 0x0302 | 8-74 | analog &mdash; `IO_CFG1.mac_impedance=0xC000`<br>rmii &mdash; `IO_CFG1.rx_dv=0x0100` |
| `LED0_GPIO_CFG` | MMD 0x0303 | 8-75 | clock &mdash; `LED0_GPIO_CFG.clock_source=0x0038`<br>pin_mux &mdash; `LED0_GPIO_CFG.mux=0x0007` |
| `LED1_GPIO_CFG` | MMD 0x0304 | 8-76 | clock &mdash; `LED1_GPIO_CFG.clock_source=0x0038`<br>pin_mux &mdash; `LED1_GPIO_CFG.mux=0x0007` |
| `LED2_GPIO_CFG` | MMD 0x0305 | 8-77 | pin_mux &mdash; `LED2_GPIO_CFG.mux=0x0007` |
| `LED3_GPIO_CFG` | MMD 0x0306 | 8-78 | pin_mux &mdash; `LED3_GPIO_CFG.mux=0x0007` |
| `CLK_OUT_LED_STATUS` | MMD 0x0308 | 8-79 | clock &mdash; `CLK_OUT_LED_STATUS.clock_off_status=0x0001` |
| `VOD_CFG1` | MMD 0x030B | 8-80 | analog &mdash; `VOD_CFG1.minus_mdix_high=0x3000`, `VOD_CFG1.minus_mdi=0x0FC0`, `VOD_CFG1.zero=0x003F` |
| `VOD_CFG2` | MMD 0x030C | 8-81 | analog &mdash; `VOD_CFG2.minus_mdix_low=0xF000`, `VOD_CFG2.plus_mdix=0x0FC0`, `VOD_CFG2.plus_mdi=0x003F` |
| `VOD_CFG3` | MMD 0x030E | 8-82 | initialization &mdash; `VOD_CFG3.mdi_termination=0xF000`, `VOD_CFG3.mdi_enable=0x0800`, `VOD_CFG3.mdix_termination=0x0780`, `VOD_CFG3.mdix_enable=0x0040` |
| `ANA_LD_PROG_SL` | MMD 0x0404 | 8-83 | No named fields |
| `ANA_RX10BT_CTRL` | MMD 0x040D | 8-84 | tx10 &mdash; `ANA_RX10BT_CTRL.receiver_current_gain=0x001F` |
| `GENCFG` | MMD 0x0456 | 8-85 | compatibility &mdash; `GENCFG.min_ipg=0x0008` |
| `LEDCFG` | MMD 0x0460 | 8-86 | led &mdash; `LEDCFG.led3_source=0x0F00`, `LEDCFG.led2_source=0x00F0`, `LEDCFG.led1_source=0x000F` |
| `IOCTRL` | MMD 0x0461 | 8-87 | analog &mdash; `IOCTRL.mac_slew=0x001F` |
| `SOR1` | MMD 0x0467 | 8-88 | straps &mdash; `SOR1.strap11=0x0800`, `SOR1.strap10=0x0400`, `SOR1.strap9=0x0200`, `SOR1.strap8=0x0100`, `SOR1.strap7=0x0080`, `SOR1.strap6=0x0040`, `SOR1.strap5=0x0020`, `SOR1.strap4=0x0010`, `SOR1.strap3=0x0008`, `SOR1.strap2=0x0004`, `SOR1.strap1=0x0002`, `SOR1.strap0=0x0001` |
| `SOR2` | MMD 0x0468 | 8-89 | straps &mdash; `SOR2.isolate=0x4000`, `SOR2.crs_rxdv=0x1000`, `SOR2.led3_polarity=0x0800`, `SOR2.led2_polarity=0x0400`, `SOR2.led0_polarity=0x0200`, `SOR2.fast_drop=0x0100`, `SOR2.auto_mdix=0x0080`, `SOR2.led1_polarity=0x0020`, `SOR2.rmii=0x0010`, `SOR2.follower=0x0008`, `SOR2.an1=0x0004`, `SOR2.an0=0x0002`, `SOR2.an_enable=0x0001` |
| `LEDCFG2` | MMD 0x0469 | 8-90 | led &mdash; `LEDCFG2.led3_polarity=0x0400`, `LEDCFG2.led3_drive_value=0x0200`, `LEDCFG2.led3_drive_enable=0x0100`, `LEDCFG2.led2_polarity=0x0040`, `LEDCFG2.led2_drive_value=0x0020`, `LEDCFG2.led2_drive_enable=0x0010`, `LEDCFG2.led1_polarity=0x0004`, `LEDCFG2.led1_drive_value=0x0002`, `LEDCFG2.led1_drive_enable=0x0001` |
| `RXFCFG1` | MMD 0x04A0 | 8-91 | wol &mdash; `RXFCFG1_crcGate=0x1000`, `RXFCFG1_levelAcknowledge=0x0800`, `RXFCFG1_pulseWidth=0x0600`, `RXFCFG1_levelMode=0x0100`, `RXFCFG1_enable=0x0080`, `RXFCFG1_bitMaskFlag=0x0040`, `RXFCFG1_secureOn=0x0020`, `RXFCFG1_magicPacketEnable=0x0001` |
| `RXFS` | MMD 0x04A1 | 8-92 | interrupts &mdash; `RXFS.interrupt_source=0x1000`<br>counters &mdash; `RXFS.sfd_error=0x0080`, `RXFS.bad_crc=0x0040`, `RXFS.secure_on_error=0x0020`, `RXFS.magic_packet=0x0001` |
| `RXFPMD1` | MMD 0x04A2 | 8-93 | wol &mdash; `RXFPMD1_byte4=0xFF00`, `RXFPMD1_byte5=0x00FF` |
| `RXFPMD2` | MMD 0x04A3 | 8-94 | wol &mdash; `RXFPMD2_byte2=0xFF00`, `RXFPMD2_byte3=0x00FF` |
| `RXFPMD3` | MMD 0x04A4 | 8-95 | wol &mdash; `RXFPMD3_byte0=0xFF00`, `RXFPMD3_byte1=0x00FF` |
| `RXFSOP1` | MMD 0x04A5 | 8-96 | wol &mdash; `RXFSOP1_byte1=0xFF00`, `RXFSOP1_byte0=0x00FF` |
| `RXFSOP2` | MMD 0x04A6 | 8-97 | wol &mdash; `RXFSOP2_byte3=0xFF00`, `RXFSOP2_byte2=0x00FF` |
| `RXFSOP3` | MMD 0x04A7 | 8-98 | wol &mdash; `RXFSOP3_byte5=0xFF00`, `RXFSOP3_byte4=0x00FF` |

### Recipe access

Recipe-only access uses the exact order, DEVAD, value and row in the [A procedure ledger](registers/dp83826a-procedures.json). `eee_recipe` and `eee_failures` check the 27 writes and interrupted prefixes on each grade. The two writes to 04F5 are distinct. 0308 is a script exception. 04D3 is original-only. The five IEEE tuples use their separate DEVAD 3/7 namespace, with the consuming reads under the collector. Raw access to recipe words remains fenced.

| Register | MMD address | A procedure scope |
| --- | --- | --- |
| `EEECFG3` | 0x1F:0x04D1 | EEE recipe and typed LPI control |
| `EEE_04D3` | 0x1F:0x04D3 | Original only |
| `EEE_04DF` | 0x1F:0x04DF | EEE recipe |
| `EEE_033E` | 0x1F:0x033E | EEE recipe |
| `EEE_033F` | 0x1F:0x033F | EEE recipe |
| `RECIPE_0123` | 0x1F:0x0123 | Digital loopback and EEE |
| `EEE_031B` | 0x1F:0x031B | EEE recipe |
| `EEE_0466` | 0x1F:0x0466 | EEE recipe |
| `EEE_04CF` | 0x1F:0x04CF | EEE recipe |
| `EEE_0416` | 0x1F:0x0416 | EEE recipe |
| `EEE_04F5` | 0x1F:0x04F5 | EEE recipe |
| `EEE_04E0` | 0x1F:0x04E0 | EEE recipe |
| `EEE_031F` | 0x1F:0x031F | EEE recipe |
| `EEE_04F4` | 0x1F:0x04F4 | EEE recipe |
| `RECIPE_0428` | 0x1F:0x0428 | Gated shared register (deep power and 100M pattern) |
| `LOOP_0122` | 0x1F:0x0122 | Documented digital loopback |
| `LOOP_0130` | 0x1F:0x0130 | Documented digital loopback |
| `TDR_0411` | 0x1F:0x0411 | D3 cable recipe |
| `EEE_0126` | 0x1F:0x0126 | EEE recipe |
| `EEE_0132` | 0x1F:0x0132 | EEE recipe |
| `EEE_04D4` | 0x1F:0x04D4 | EEE recipe |
| `EEE_0310` | 0x1F:0x0310 | EEE recipe |
| `EEE_0311` | 0x1F:0x0311 | EEE recipe |
| `EEE_0313` | 0x1F:0x0313 | EEE recipe |
| `EEE_033A` | 0x1F:0x033A | EEE recipe |
| `EEE_033C` | 0x1F:0x033C | EEE recipe |
| `EEE_0317` | 0x1F:0x0317 | EEE recipe |

### Exceptional writes

The feature owners below apply the [A write-action records](registers/dp83826a-procedures.json) under their typed procedures. Numbers after @ are ordered EEE recipe positions. Source gates and original-only actions in these records remain binding.

| Feature | Action IDs and recipe positions |
| --- | --- |
| `bist` | `BICSR1_freezeClear` |
| `loopback` | `LOOP_0122_digital`, `RECIPE_0123_digital`, `LOOP_0130_digital` |
| `compliance` | `compliance_100_disable`, `compliance_100_enable`, `compliance_100_count_low`, `compliance_100_count_high`, `compliance_10_pattern_0`, `compliance_10_pattern_1`, `compliance_10_pattern_2`, `compliance_10_pattern_3`, `compliance_10_pattern_4`, `compliance_10_pattern_5`, `compliance_10_pattern_6`, `compliance_10_pattern_7`, `compliance_10_pattern_8`, `compliance_10_pattern_9`, `compliance_10_pattern_A`, `compliance_10_pattern_B`, `compliance_10_pattern_C`, `compliance_10_pattern_D` |
| `eee` | `EEECFG3_eee @ 3`, `EEE_04D3_eee`, `EEE_04DF_eee @ 4`, `EEE_033E_eee @ 5`, `EEE_033F_eee @ 6`, `RECIPE_0123_eee @ 12`, `EEE_031B_eee @ 24`, `EEE_0466_eee @ 27`, `EEE_04CF_eee @ 16`, `EEE_0416_eee @ 18`, `EEE_04F5_eee @ 7,25`, `EEE_04E0_eee @ 8`, `EEE_031F_eee @ 9`, `CLK_OUT_LED_STATUS_eee @ 10`, `EEE_04F4_eee @ 11`, `EEECFG3_enable_bypass_controls`, `EEE_0126_eee @ 13`, `EEE_0132_eee @ 14`, `EEE_04D4_eee @ 15`, `EEE_0310_eee @ 17`, `EEE_0311_eee @ 19`, `EEE_0313_eee @ 20`, `EEE_033A_eee @ 21`, `EEE_033C_eee @ 22`, `EEE_0317_eee @ 23` |
| `lpi` | `EEECFG3_lpi_start`, `EEECFG3_lpi_stop` |
| `power` | `RECIPE_0428_dpd_enable` |
| `cable` | `TDR_0411_tdr_0813`, `TDR_0411_tdr_0815`, `TDR_0411_tdr_0816`, `EEE_0416_tdr_1FA0`, `EEE_0416_tdr_1F90`, `GENCFG_tdr_0600` |

## DP83825I

The revision-0 core covers 18 procedures, including exact identity, default
initialization, PCS/HARD/RESTART, RMII clock-role verification and clock mux
selection, link profiles, next pages, live status and fast link drop.
Both declared RMII roles have compiled fixture and offline replay evidence.
[Prompt 60 verification](reports/implementation/60-p25-source-core.md#phase-d-final-verification)

The ledger assigns 15 further procedures to prompt 61, including functional
mode recipes, power and EEE, event/counter services, BIST, loopback, TDR,
analog/electrical settings, wake and pin profiles. Physical clock, link and
electrical qualification remain open under Q15.
[DP83825I register and procedure ledger](registers/dp83825i.json)

### DP83825I procedures

The API, shared CLI and executing host test below identify the owner for each ledger procedure. Q15 covers board and line measurements. The register table groups printed bit ranges by these same owners. The standalone DSP controls below name the independently programmable fields. A recipe may also write opaque fields at the cited addresses and values.

| Ledger procedure | Core owner | Shared CLI entry | Host registration | Scope |
| --- | --- | --- | --- | --- |
| `identify` | readIdentity | `connection identity` | `p25_identity` | 2000:A140 revision 0 |
| `mdio` | readClause22 / readMmd / modifyMmd | `registers read` | `p25_events` | Separate direct, vendor, PCS and AN namespaces |
| `external_reset` | invalidateDeviceState | `configuration invalidate` | `p25_core` | Owner reports an external event |
| `pcs_reset` | startInitialize | `recovery reset` | `p25_core` | BMCR command and observed clear |
| `hard_reset` | startInitialize | `recovery reset` | `p25_core` | HARD with role recapture and controller silence |
| `restart` | startInitialize | `recovery restart` | `p25_core` | Register-retaining circuitry restart |
| `initialize` | startInitialize | `connection initialize` | `p25_core` | Part startup, role, profile and bounded replay |
| `clock_role` | strapSnapshot | `connection identity` | `p25_core` | SOR1 and RCSR, both roles |
| `rmii` | configureInterface | `controls interface` | `p25_profiles` | Revision, elasticity and RMII-only topology |
| `follower_timing` | configureInterface | `controls interface` | `p25_profiles` | Clock-shift companion has no sourced inverse |
| `clock_loss` | configureInterface / modifyMmd | `controls interface` | `p25_profiles` | Leader pin 2 and isolation need board consent |
| `configure_an` | startConfigure | `configuration apply` | `p25_core` | Local ANAR and peer capability stay separate |
| `configure_forced` | startConfigure | `configuration apply` | `p25_core` | Both speeds and duplex settings |
| `next_page` | startConfigure | `configuration apply` | `p25_negotiation_pages` | One owner per page epoch |
| `observe_link` | readLinkStatus | `link read` | `p25_core` | Current link with latch history |
| `fast_drop` | startConfigureQuality | `controls quality` | `p25_profiles` | CR3 enables and collector-owned FLDS |
| `link_options` | startConfigure | `configuration apply` | `p25_core` | Copper advertisement and pair selection |
| `repeater` | configureInterface | `controls interface` | `p25_profiles` | Two followers with a shared external reference |
| `functional_mode` | startInitialize / functionalModeSnapshot | `connection initialize` | `p25_functional_modes` | Default and tables 6-5 through 6-7 from HARD baseline |
| `eee` | startConfigureEee / readEeeStatus / startInitialize | `eee configure` | `p25_functional_modes` | Table 6-7, six IEEE tuples, PCS clock and reset |
| `legacy_lpi` | startLpi / stopLpi | `lpi start` | `p25_functional_modes` | Negotiated and bilateral application-controlled entry. MAC owns the 36 us exit interval |
| `power` | startConfigurePower / readPowerStatus | `power configure` | `p25_functional_modes` | IEEE down, active sleep, PLL and deep-control readback. Actual current and clock recovery need Q15 |
| `irq` | startConfigureInterrupts / serviceInterrupts | `irq service` | `p25_events` | MISR1/2 delivery only. Pin and stimulus checks in p25_outputs |
| `counters` | collectEvents / acquireEvidence | `link collect` | `p25_events` | Register_12 separate. RECR kept raw under the width conflict |
| `quality` | readSignalQuality | `controls mse` | `p25_diagnostics` | SDAA081 table 2-6 holds two equality gaps |
| `loopback` | startLoopback / stopLoopback | `diagnostics loopback-start` | `p25_diagnostics` | Exact BISCR selectors, AN disabled for reverse |
| `bist` | startBist / startBistAcquire | `diagnostics bist-start` | `p25_diagnostics` | PRBS generator, checker, freeze, capture and stop |
| `tdr` | startCableTest / startCableAcquire | `diagnostics cable-raw` | `p25_diagnostics` | Quiet cable and first peak. CDCR calibration completion remains unresolved |
| `led_pin` | configurePins | `controls pins` | `p25_outputs` | LED0, LED2, pin 2 mux and separate clock permission |
| `wake` | startConfigureWake / acknowledgeWake | `wol configure` | `p25_outputs` | Disable before rekey, direct/IRQ routes and level acknowledgement |
| `vod` | startConfigureAnalog | `controls analog` | `p25_outputs` | 17 coarse rows and unit-specific fine word. Q15 measures voltage |
| `analog` | startConfigureAnalog / startConfigureQuality | `controls analog` | `p25_outputs` | Impedance 50/25 ohms, 001A squelch 0..8 and seven independently masked DSP groups (p25_diagnostics). See the field decisions below |
| `compliance` | startCompliance / startCollisionTest | `diagnostics compliance-start` | `p25_diagnostics` | 10M patterns, 100M count, scrambler and RMII register-only collision. Scope and fixture verify emissions |

### DP83825I register fields

Each bit range joins the named field, access, reset, legal codes and constraints in the [exact register ledger](registers/dp83825i.json). Reserved ranges have no programming route. An analog assignment outside 001A and 0302 is restricted to an explicitly listed table write or left at its device setting. SNLS638C supplies no independent safe board operating range for those settings. The six IEEE rows use their DEVAD-qualified low wire offsets.

| Printed offset | SNLS638C table | Field owner and bits |
| --- | --- | --- |
| `0000` | 7-3 | pcs_reset (15) / loopback (14) / configure_forced (13, 8) / configure_an (12, 9) / power (11) / clock_loss (10) / compliance (7) / reserved (6-0) |
| `0001` | 7-4 | observe_link (15, 14, 13, 12, 11, 6, 5, 4, 3, 2, 1, 0) / reserved (10-7) |
| `0002` | 7-5 | identify (15-0) |
| `0003` | 7-6 | identify (15-10, 9-4, 3-0) |
| `0004` | 7-7 | next_page (15) / reserved (14, 12) / configure_an (13, 11, 10, 9, 8, 7, 6, 5, 4-0) |
| `0005` | 7-8 | next_page (15, 14) / configure_an (13, 11, 10, 9, 8, 7, 6, 5, 4-0) / reserved (12) |
| `0006` | 7-9 | reserved (15-5) / next_page (4, 3, 2, 1, 0) |
| `0007` | 7-10 | next_page (15, 13, 12, 11, 10-0) / reserved (14) |
| `0008` | 7-11 | next_page (15, 14, 13, 12, 11, 10-0) |
| `0009` | 7-12 | reserved (15-10, 9, 7, 6, 4, 3-2, 0) / tdr (8) / configure_forced (5) / link_options (1) |
| `000A` | 7-13 | reserved (15, 14, 13-7, 6, 4, 3, 0) / link_options (5, 2, 1) |
| `000B` | 7-14 | reserved (15-11, 9, 8, 7, 4) / fast_drop (10, 3-0) / configure_forced (6, 5) |
| `000C` | 7-15 | irq (15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0) |
| `000D` | 7-16 | mdio (15-14, 4-0) / reserved (13-5) |
| `000E` | 7-17 | mdio (15-0) |
| `000F` | 7-18 | reserved (15-9, 3-0) / fast_drop (8-4) |
| `0010` | 7-19 | reserved (15) / observe_link (14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0) |
| `0011` | 7-20 | power (15, 14, 13-12) / compliance (11) / reserved (10, 7-5, 4) / loopback (9-8) / irq (3, 2, 1, 0) |
| `0012` | 7-21 | irq (15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0) |
| `0013` | 7-22 | irq (15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0) |
| `0014` | 7-23 | reserved (15-8) / counters (7-0) |
| `0015` | 7-24 | counters (15-0) |
| `0016` | 7-25 | reserved (15, 7, 5) / bist (14, 13, 12, 11, 10, 9) / power (8) / loopback (6, 4-0) |
| `0017` | 7-26 | reserved (15-13, 12, 11, 10, 9, 6, 5) / follower_timing (8) / clock_role (7, 3, 2) / rmii (4, 1-0) |
| `0018` | 7-27 | reserved (15-11, 8, 6-5, 3-2, 0) / led_pin (10-9, 7, 4, 1) |
| `0019` | 7-28 | configure_forced (15, 14) / observe_link (13, 12, 11) / reserved (10-8, 6) / led_pin (7, 5) / identify (4-0) |
| `001A` | 7-29 | reserved (15-14, 8, 6-5, 3-1) / analog (13, 12-9, 7, 0) / counters (4) |
| `001B` | 7-30 | bist (15-8, 7-0) |
| `001C` | 7-31 | reserved (15-11) / bist (10-0) |
| `001E` | 7-32 | tdr (15, 14, 1, 0) / reserved (13-2) |
| `001F` | 7-33 | hard_reset (15) / restart (14) / reserved (13, 12-0) |
| `0025` | 7-34 | reserved (15-10, 8-7, 2-1) / led_pin (9, 6-3, 0) |
| `0027` | 7-35 | reserved (15-4) / compliance (3-0) |
| `0101` | 7-36 | analog (15-8, 7, 4, 3-1, 0) / reserved (6-5) |
| `010A` | 7-37 | analog (15-8, 7-0) |
| `0123` | 7-38 | reserved (15) / analog (14-0) |
| `0130` | 7-39 | reserved (15) / analog (14-12, 11, 10, 9, 8, 7-0) |
| `0170` | 7-40 | reserved (15, 11, 7, 3-0) / tdr (14, 13, 12, 10-8, 6-4) |
| `0171` | 7-41 | reserved (15-0) |
| `0172` | 7-42 | reserved (15-0) |
| `0173` | 7-43 | tdr (15-8, 7-0) |
| `0174` | 7-44 | reserved (15-0) |
| `0175` | 7-45 | reserved (15-14, 10-5, 4) / tdr (13-11, 3-0) |
| `0176` | 7-46 | reserved (15-5) / tdr (4-0) |
| `0177` | 7-47 | reserved (15-13, 7-0) / tdr (12-8) |
| `0178` | 7-48 | reserved (15-3) / tdr (2-0) |
| `0180` | 7-49 | reserved (15-8) / tdr (7-0) |
| `0181` | 7-50 | reserved (15-0) |
| `0182` | 7-51 | reserved (15-0) |
| `0183` | 7-52 | reserved (15-0) |
| `0184` | 7-53 | reserved (15-0) |
| `0185` | 7-54 | reserved (15-7) / tdr (6-0) |
| `0186` | 7-55 | reserved (15-0) |
| `0187` | 7-56 | reserved (15-0) |
| `0188` | 7-57 | reserved (15-0) |
| `0189` | 7-58 | reserved (15-0) |
| `018A` | 7-59 | reserved (15-12, 10-6, 4, 3, 2, 1-0) / tdr (11, 5) |
| `0302` | 7-60 | analog (15-14) / reserved (13, 12-9, 7, 5-0) / repeater (8) / led_pin (6) |
| `0305` | 7-61 | reserved (15-6, 5-3) / led_pin (2-0) |
| `0308` | 7-62 | functional_mode (15-1) / led_pin (0) |
| `030B` | 7-63 | reserved (15-12) / vod (11-6, 5-0) |
| `030C` | 7-64 | reserved (15-6) / vod (5-0) |
| `030F` | 7-65 | reserved (15-11) / analog (10-8, 7, 6, 5, 4-2, 1, 0) |
| `0311` | 7-66 | analog (15-14, 13, 8-1) / reserved (12-9, 0) |
| `0313` | 7-67 | analog (15-8, 7-0) |
| `031C` | 7-68 | analog (15, 14, 13-11, 10-7, 6, 5, 4, 3, 2, 1, 0) |
| `031F` | 7-69 | analog (15-11, 10-6, 5-1) / reserved (0) |
| `033C` | 7-70 | reserved_by_description (15-8) / reserved (7) / analog (6-0) |
| `033E` | 7-71 | analog (15, 14-13, 12-8, 7, 6, 5, 4-0) |
| `0404` | 7-72 | analog (15-0) |
| `040D` | 7-73 | reserved (15-5) / analog (4-0) |
| `0416` | 7-74 | reserved (15-13, 6, 3-2) / analog (12, 7, 5-4, 1, 0) / reserved_by_description (11-8) |
| `0428` | 7-75 | reserved (15-6, 5, 4, 3, 1, 0) / power (2) |
| `0429` | 7-76 | analog (15-8) / reserved (7-0) |
| `0456` | 7-77 | reserved (15-4, 2-0) / link_options (3) |
| `0460` | 7-78 | reserved (15-12, 11-8, 3-0) / led_pin (7-4) |
| `0461` | 7-79 | reserved (15, 14, 13-12, 11, 10-7, 6-5, 4-0) |
| `0467` | 7-80 | reserved (15, 11, 10, 9, 7, 6, 5, 4, 2, 1) / reserved_by_description (14) / clock_role (13, 12, 8, 3, 0) |
| `0468` | 7-81 | reserved (15-13, 12, 10, 9, 8, 6, 5, 4, 3, 2, 1, 0) / repeater (11) / led_pin (7) |
| `0469` | 7-82 | reserved (15-11, 10, 9, 8, 7, 3, 2, 1, 0) / led_pin (6, 5, 4) |
| `04A0` | 7-83 | reserved (15-14, 13, 4, 3, 2, 1) / wake (12, 11, 10-9, 8, 7, 6, 5, 0) |
| `04A1` | 7-84 | reserved (15-13, 11-8, 4, 3, 2, 1) / wake (12, 7, 6, 5, 0) |
| `04A2` | 7-85 | wake (15-8, 7-0) |
| `04A3` | 7-86 | wake (15-8, 7-0) |
| `04A4` | 7-87 | wake (15-8, 7-0) |
| `04CD` | 7-88 | analog (15-8, 7-0) |
| `04CE` | 7-89 | reserved (15-8) / analog (7-0) |
| `04CF` | 7-90 | analog (15-12, 11-8) / reserved_by_description (7-4, 3, 2, 1, 0) |
| `04D0` | 7-91 | reserved (15, 14, 13-7, 6-5, 4-0) |
| `04D1` | 7-92 | reserved (15, 11) / legacy_lpi (14-13, 12) / eee (10, 9, 8, 7-4, 3, 2, 1, 0) |
| `04D2` | 7-93 | analog (15-14, 13, 12, 11, 10, 9-5, 4-0) |
| `04D4` | 7-94 | reserved (15, 11, 7, 3) / analog (14-12, 10-8, 6-4, 2-0) |
| `04D5` | 7-95 | reserved (15-10) / analog (9-7, 6-4, 3-2, 1, 0) |
| `04D6` | 7-96 | reserved (15-10) / analog (9-7, 6-4, 3-2, 1, 0) |
| `04D7` | 7-97 | reserved (15-10) / analog (9-7, 6-4, 3-2, 1, 0) |
| `1000` | 7-98 | eee (15, 10) / reserved (14-11, 9-0) |
| `1001` | 7-99 | reserved (15-12, 7, 5-0) / eee (11, 10, 9, 8, 6) |
| `1014` | 7-100 | reserved (15-3, 0) / eee (2, 1) |
| `1016` | 7-101 | eee (15-0) |
| `203C` | 7-102 | reserved (15-2, 0) / eee (1) |
| `203D` | 7-103 | reserved (15-2, 0) / eee (1) |

### DP83825I address exceptions

| DEVAD 31 offset | Procedure | Allowed action and source |
| --- | --- | --- |
| `0042` | `follower_timing` | Write exactly 0014 after 0017[8]=1 on a confirmed follower. Inverse and readback unspecified. (5.6 footnote 2) |
| `0218` | `quality` | Read only with powered PHY and connected partner; retain equality at either threshold as unclassified. (SDAA081 2.3.3 table 2-6) |
| `030E` | `vod` | Write only the computed fine tuning word, with admissible Offset_x; no unconditional startup value. (8-3) |
| `0333` | `vod` | Read unit-specific trim bits 15:11 and 10:6 to compute C; no write. (8-3) |
| `031B` | `functional_mode` | Write 0048 in the cited position of the selected mode. (6-5/6/7) |
| `033A` | `functional_mode` | Write 579C in the cited position of the selected mode. (6-5/6/7) |
| `033D` | `functional_mode` | Write 8110 in the cited position of the selected mode. (6-5/6/7) |
| `04A5` | `wake` | Secure-ON password word; only under the authenticated wake configuration owner. Verify on Q15. (SNLA261A table 2-1 and 2.2.2) |
| `04A6` | `wake` | Secure-ON password word; only under the authenticated wake configuration owner. Verify on Q15. (SNLA261A table 2-1 and 2.2.2) |
| `04A7` | `wake` | Secure-ON password word; only under the authenticated wake configuration owner. Verify on Q15. (SNLA261A table 2-1 and 2.2.2) |

### DP83825I standalone DSP controls and evidence gaps

The analog owner accepts complete documented fields and preserves unowned bits under the controller lock. The CLI names each group in `controls analog`. Mode-owned words cannot also have an independent writer. The following limits follow SNLS638C tables 7-62 through 7-74.

- 0308 exposes RX-disable, pre-LPI, counter and adaptation freeze controls and LPI MSE bypass. Bit 0 is observed, bit 1 is fixed, bits 3/9 are unspecified. Standalone RX clock selection and the 176/192 us timer polarity remain open.
- 030F exposes the eight documented equalizer controls. 0311 permits coefficient selection 0/1/2 and the two coefficient sources. A standalone forced-coefficient profile lacks a documented admissible range.
- 031C exposes gain enables, freeze choices, polarity, input and pre-cursor selection. Independent forced-gain calibration remains open. 031F exposes per-group freeze controls for the LPI freeze and wake states. Bits 5:1 have contradictory flush/wake descriptions and remain recipe-only.
- 033E exposes bits 6:5. 0404 permits only current codes 0/4/7 (12.7/15.7/19.5 mA). Debug and bias codes lack documented operating bounds, and bit 0 has conflicting unused/burn-in descriptions.
- 040D's 200..575 mV interval in 25 mV increments describes 16 levels for a five-bit code, with bit 3 inverted on PG1.1. Standalone threshold programming remains open pending an identified revision and code map. 0461 is reserved.
- 0130 names bypass while its description enables the block. Its step/trigger/clear selector polarities and timer scales are unspecified. 0123 gives no standalone tuning bounds for the convergence threshold. 0101/010A, 0313, 033C, 0416, 0429, 04CD..04CF, 04D2 and 04D4..04D7 identify internal settings without a qualified standalone range. 033E[15] has no operational description, [7] is read-only and [14:13] has no coefficient-limit map. The threshold offset and coefficient limit lack a qualified standalone tuning range.
- 04D1[2:1] controls next-page reception and analog RX shutdown during LPI Quiet. Both fields are R/W in table 7-92. The EEE owner applies the selected policy after table 6-7 writes 00C2. Bits 10:4 are named without independent field behavior and stay at the recipe values. Only bit 0 carries the R marking that conflicts with the cited recipe.
