# DP83826I feature coverage

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
Prompt20 completes [typed events/counters](event-counters.md), per-source partial
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
planned configuration/actions and observable results. Its normative legal-value rule includes all 283 field
records and gated/opaque actions without duplicating the ledger. Only foundational
values/direct signatures are frozen; feature-specific signatures arrive with the
owning behavior. No raw API substitutes for a multistep typed procedure.

For each feature, legal values are the referenced ledger fields' `legal`,
`prohibited`, `access` and `validity` records plus procedure prerequisites and
gates. Ranges are unshifted encodings excluding prohibited values; they never
authorize writes to read-only fields or supply missing physical interpretations.

Prompt 8 adds the shared operation/cleanup contract used by these owners:
initialize/configure/verify share finite polling, retained effects, cleanupOwed
and an acyclic persistent restore order; restoration is a later bounded operation. Later feature operations
extend that same slot/result with working behavior; their signatures and existing
silicon gates remain explicit. No feature's implementation status changes here.

| Feature / responsibility | Research | Primary implementation owner | Related integration/test stages | Source gates | Planned typed route / result | Ledger procedures |
| --- | --- | --- | --- | --- | --- | --- |
| identity / Exact identity and capabilities | 3 / COMPLETE | 14 (SOFTWARE COMPLETE; HOST PASS) | 14; SDK/physical NOT RUN | None named | readIdentity / identitySnapshot / capabilities; raw/decoded IDs and partial/admission evidence | identify |
| access / Clause 22 and complete indirect access | 3 / COMPLETE | 11 (SOFTWARE) | 11, 12, 13 | eee_wire | C22/MMD direct access and internal side-effect-aware update; Value or retained frame/effect report | direct_access, mmd_access |
| reset / PCS, hard reset, restart and reconciliation | 3 / COMPLETE | 15-16 (SOFTWARE COMPLETE; HOST PASS) | 15, 16 | hard_reset_straps, pcs_reset_extent | PCS/hard/circuitry reset and external-reset reconciliation; Completion, lost trust, accepted effects, cleanup | pcs_reset, hard_reset, restart, external_reset |
| initialization / Required 10BASE-Te initialization | 3 / COMPLETE | 15 (SOFTWARE COMPLETE; HOST PASS) | 15, 17, 18 | None named | Mandatory 10BASE-Te preparation; Verified initialization evidence | initialize_10base |
| negotiation / Advertisement, forced-compatible negotiation and pause | 3 / COMPLETE | 17 (SOFTWARE) | 17, 19, 37, 38, 39 | None named | LinkSettings, advertisement and AN controls; Desired/observed/trust, negotiation and directional pause | configure_an |
| next_page / Complete next-page exchange | 3 / COMPLETE | 17 (SOFTWARE) | 17, 19, 20, 29, 34 | next_page_handshake | Typed local pages and bounded exchange; Partner pages, partial handshake and completion | next_page_exchange |
| crossover / Automatic, robust and manual crossover / port mirror | 3 / COMPLETE | 18 (SOFTWARE) | 18, 19, 34 | forced_physts | ForcedLinkSettings, crossover/mirror/extended duplex; Configured mode versus observed routing/link | configure_forced, extended_link_configure |
| link / Live and retained link observations | 3 / COMPLETE | 19 COMPLETE (SOFTWARE; host PASS) | 19 | forced_physts (physical Q11) | readLinkStatus and cached LinkSnapshot; current/history validity, conservative age, forced/AN pause and change evidence; test/link.cpp | observe_link |
| eee / EEE capability, initialization and negotiation | 5 / COMPLETE | 29 (SOFTWARE) | 29, 37, 38, 39 | eee_wire, eee_control_semantics | EEE initialization/settings/status; Local/partner capability, request and negotiated state | eee_initialize, eee_negotiation_status |
| lpi / Legacy MAC LPI sessions | 5 / COMPLETE | 30 (SOFTWARE) | 30, 37, 38, 39, 68 | eee_wire, eee_control_semantics | Bounded legacy-MAC LPI start/stop, application-owned dwell; requested/uncertain state and masked control readback, no invented readiness event | legacy_lpi |
| wol / Magic Packet and Secure-ON wake | 5 / COMPLETE | 31 (SOFTWARE) | 31 | wol_bit_mask, wol_live_reconfigure, rxfs_sfd_reference | Wake MAC/password, pulse/level/CRC and acknowledgement; Verified settings and retained wake/error evidence | wol_configure, wol_acknowledge |
| power / Active-sleep policy, IEEE/deep power-down, PLL | 5 / COMPLETE | 28 (PARTIAL; deep-power entry/exit BLOCKED) | 28, 37, 38, 39 | deep_power_exit | IEEE power, active-sleep, PLL and deep-power policy; Desired policy, observed power and exit/trust | power_ieee, power_active_sleep, power_pll_control, power_deep |
| repeater / RMII repeater arrangements | 5 / COMPLETE | 33 (SOFTWARE) | 33, 68 | rmii_rc | Coordinated two-PHY topology; Both owners' results, partial effects/restoration | repeater_configure |
| clock / Clock output and reset/power interactions | 5 / COMPLETE | 33 (SOFTWARE) | 33 | pin_fields | Clock output selection and dependencies; Configured policy and qualified observed state | clock_configure |
| mii / MII interface controls | 5 / COMPLETE | 33 (SOFTWARE) | 33, 37, 38, 39 | rmii_rc | MII controls/isolation; Verified interface settings/isolation | mii_configure, isolation_configure |
| rmii / RMII interface, elasticity and status | 5 / COMPLETE | 33 (SOFTWARE) | 33, 37, 38, 39 | rmii_rc | Leader/follower, revision, elasticity, RX_DV and shift; Verified settings and collected status | rmii_configure |
| tx100 / 100BASE-TX exposed data-path controls | 5 / COMPLETE | 34 (SOFTWARE) | 34, 35 | None named | Scrambler and idle RX-error policy; Verified data-path settings | tx100_configure |
| tx10 / 10BASE-Te receiver, NLP, jabber and preamble | 5 / COMPLETE | 34 (SOFTWARE) | 34, 35 | analog_rx_gain | Squelch/threshold/NLP/jabber/preamble/gain; Verified settings with physical interpretation limits | tx10_configure |
| loopback / All documented loopback modes | 4 / COMPLETE | 23 (SOFTWARE) | 23 | loopback_recipe | All documented near/digital/reverse/external modes; Active/maybe-active mode and restoration | loopback_near, loopback_digital, loopback_reverse, loopback_external |
| bist / BIST/PRBS modes and retained acquisition | 4 / COMPLETE | 24 (SOFTWARE) | 24 | bist_result_semantics | Generator/checker, packet/IPG/counter modes; Activity/lock, retained finite error sample | bist_run, bist_capture |
| cable / Manual and autorun cable capture | 4 / COMPLETE | 25 (SOFTWARE) | 25, 26 | tdr_reserved, tdr_calibration, tdr_freshness_abort, tdr_recipe | Manual/autorun settings and finite capture; Freshness-qualified raw peaks/amplitude/polarity | cable_configure, cable_manual, cable_autorun, cable_capture |
| fast_drop / MII-only Fast Link Drop | 4 / COMPLETE | 34 (SOFTWARE) | 34 | fld_reset_defaults, fld_threshold_recipe | MII-only causes/thresholds; Verified settings and retained causes | configure_fast_drop |
| link_recovery / Hardware link-loss recovery | 5 / COMPLETE | 34 (SOFTWARE) | 34 | None named | Hardware link-loss recovery; Verified policy | link_recovery_configure |
| led / LED sources, blink, stretch and drive | 5 / COMPLETE | 32 (SOFTWARE) | 32 | pin_fields | Per-pin sources/polarity/blink/stretch/manual drive; Verified per-pin values | led_configure |
| pin_mux / Per-pin GPIO, wake, IRQ and clock mux | 5 / COMPLETE | 32 (SOFTWARE) | 32, 33 | pin_fields | Exact per-pin mux and conflicts; Valid roles and read-only observations | pin_configure |
| interrupts / Global IRQ and all event enables/service | 4 / COMPLETE | 27 (SOFTWARE) | 20, 27 | rxfs_sfd_reference | Global routing/polarity/enables/test/service; Verified masks, retained causes | interrupt_configuration, interrupt_test |
| counters / Single collector for status and destructive counters | 4 / COMPLETE | 20 COMPLETE (source-qualified SOFTWARE; host PASS) | 20, 21 | recr_width, rmii_rc, rxfs_sfd_reference, forced_physts | collectEvents/counterSnapshot, partial evidence, source-qualified FCSCR sums and host-history clear; test/events.cpp | collect_event_counters |
| quality / Raw MSE and supported quality interpretation | 4 / COMPLETE | 34 (SOFTWARE) | 34 | mse_address | Raw MSE and source-proved interpretation; Raw value/validity, no invented SNR | signal_quality_sample |
| compliance / Finite compliance stimulus procedures | 5 / COMPLETE | 35 (SOFTWARE) | 35, 51, 73 | compt_reserved | Collision test and finite admitted stimulus; Applied/maybe-emitting state, stop/cleanup | collision_test, compliance_session |
| analog / Analog amplitude, termination and MAC output | 5 / COMPLETE | 35 (SOFTWARE) | 35 | ioctrl_example, analog_rx_gain | Amplitude/termination/slew/output profile; Verified settings and legal intermediate restoration | analog_configure |
| compatibility / IPG and compatibility controls | 5 / COMPLETE | 33 (SOFTWARE) | 33, 35 | None named | IPG and compatibility policy; Verified settings | compatibility_configure |
| calibration / Resistor calibration contingency | 4 / COMPLETE | 35 (SOFTWARE) | 16, 25, 35 | hard_reset_straps | Calibration contingency recovery; Recovery/reset/reinitialization evidence | calibration_recovery |
| straps / POR/pin straps and sampled configuration | 3 / COMPLETE | 14 (HARDWARE_ONLY) | 14, 16, 33 | hard_reset_straps | Read-only captured strap observation; Raw/decoded words, mode/reset provenance | Hardware observation; no strap setter |
| outside_phy / MAC/DMA, stack, board power and scheduling | 3 / COMPLETE | 6 (OUTSIDE_PHY) | 6, 37, 40, 41, 42 | None named | Application pins/clocks/power/MAC/stack/scheduling; Application-owned evidence | Outside core |
| absent / Unsupported exact-part interfaces and features | 3 / COMPLETE | 14 (ABSENT) | 14 | None named | Exact-part capability validation; Explicit source-backed rejection | No invented operation |

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

WP3 retains research completeness with97 register records (79 detailed,18 opaque)
and five source-implied EEE associations. D2's stable reads, collector routes and
fail-closed checks do not qualify physical offsets. D3's SNLA428 transmit-A helper,
D4 forced-mode interpretation, D7 all14 10M compliance actions and D8 transient
next-page ownership are selected research policies. Their physical questions
remain Q1/Q7/Q11/Q19/Q10 respectively; full feature implementation stays with its
assigned later prompt. The current prompt13 collector has real destructive-read
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

Prompt27 completes `interrupt_configuration` and `interrupt_test` through
[owned IRQ policy and service](interrupts.md): all source enables, global/output/
polarity, qualified shared-source attribution, explicit test stop, finite service
and shared reset replay. Pending half-full requests survive incomplete passes.
Host tests cover complete procedures; Q9 pin/latching behavior and the RXFS SFD
interpretation gate remain open. LED1/2 mux programming remains prompt32.

## Prompt 28 power implementation

[Power operations](power.md) implement `power_ieee`, `power_active_sleep` and
`power_pll_control`, including profile intent, non-HARD PLL preparation and shared
restoration. `power_deep` has a tested zero-I/O rejection; entry/exit remain
BLOCKED by `deep_power_exit`. Thus the whole power feature remains PARTIAL in the
ledger, despite completion of its independently supported procedures. Host tests
and portable examples do not close Q6 or establish SDK support.

Stage29 implements `eee_initialize` and `eee_negotiation_status` through the
[EEE guide](eee.md), exact eighteen-row recipe, shared restoration and single
DEVAD-qualified collector. Host coverage is in `test/eee.cpp`; Q1 physical wire,
control and interoperability qualification remains pending. Explicit LPI is implemented separately in stage30.


Stage30 implements `legacy_lpi`: two bounded bit12 calls, persistent OP-9 session
barrier, passive request evidence and separate explicit stop/restoration.
[Guide](lpi.md) and [report](reports/implementation/30-legacy-mac-lpi.md) record
host tests; physical Q1/Q2 and SDK/controller results remain NOT RUN.


Stage31 implements `wol_configure` and `wol_acknowledge` with exact-part MAC/
Secure-ON matching, CRC/pulse/level controls, typed status, shared collection,
quiesced route/configuration and reset replay. [Guide](wake-on-lan.md) and
[report](reports/implementation/31-wake-on-lan.md) separate host protocol tests
from Q3 live-reconfiguration, W0C/RH-bit and physical wake qualification.


Stage32 implements `led_configure` and `pin_configure` through one typed
`PinSettings` profile and shared reset/power restoration. All admitted source,
force/polarity, blink/stretch and mux values have independent literal/failure
tests. [The guide](led-gpio.md) lists source-gated routes and
[the report](reports/implementation/32-led-gpio.md) separates host evidence from
Q12. IRQ/WoL source owners retain their own mux writes and collector.


Stage33 implements `clock_configure`, `mii_configure`, `rmii_configure`,
`repeater_configure`, `compatibility_configure` and `isolation_configure` through
one `InterfaceSettings`/`configureInterface` pass. Independent host vectors and
failure/restore tests are in `test/interface.cpp`; the
[guide](interface-clock.md) and [report](reports/implementation/33-clock-interface-repeater.md)
keep Q12 clock-mux, Q17 raw-RCSR and physical repeater evidence separate.
Clock selectors without an established activation route remain rejected.

Stage34 completes `configure_fast_drop`, `signal_quality_sample`,
`tx100_configure`, `tx10_configure` and `link_recovery_configure` through
[quality controls](quality.md). `startConfigureQuality` owns explicit grouped
settings; `readSignalQuality` returns unscaled raw MSE. `test/quality.cpp` covers
independent literals, collector retention, failure/cancellation, shared replay
and exact bounds. CLI remains owner-routed work in44-46. Physical MII detector
latency, MSE calibration and partner/traffic evidence remain NOT RUN. The
`mse_address`, `fld_threshold_recipe`, `fld_reset_defaults` and `analog_rx_gain`
gates remain visible; no raw interpretation or fixture result is invented.

Prompt35 implements `analog_configure`, `collision_test`, `compliance_session`
and `calibration_recovery` through the [electrical guide](electrical.md). Source
dispositions include gated receiver gain, IOCTRL example and100M0428 access;
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
