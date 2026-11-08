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
mandatory VOD verification and retained restoration. HARD/RESTART and remaining
feature procedures are PENDING; SDK builds and hardware qualification
are NOT RUN. Ledger tests establish their own research layer.

Prompt 6 completes the [portable/native ownership design](design-decisions.md)
for all these entry families. It introduces no feature support claim. The selected public native lifecycle route remains prompt-37 implementation
work; source limitations and physical gates remain separate. Full pause evidence, SDK projection, raw access and the
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
| reset / PCS, hard reset, restart and reconciliation | 3 / COMPLETE | 15 (PCS COMPLETE; HOST PASS), 16 (PENDING) | 15, 16 | hard_reset_straps, pcs_reset_extent | PCS/hard/circuitry reset and external-reset reconciliation; Completion, lost trust, accepted effects, cleanup | pcs_reset, hard_reset, restart, external_reset |
| initialization / Required 10BASE-Te initialization | 3 / COMPLETE | 15 (SOFTWARE COMPLETE; HOST PASS) | 15, 17, 18 | None named | Mandatory 10BASE-Te preparation; Verified initialization evidence | initialize_10base |
| negotiation / Advertisement, forced-compatible negotiation and pause | 3 / COMPLETE | 17 (SOFTWARE) | 17, 19, 37, 38, 39 | None named | LinkSettings, advertisement and AN controls; Desired/observed/trust, negotiation and directional pause | configure_an |
| next_page / Complete next-page exchange | 3 / COMPLETE | 17 (SOFTWARE) | 17, 19, 20, 29, 34 | next_page_handshake | Typed local pages and bounded exchange; Partner pages, partial handshake and completion | next_page_exchange |
| crossover / Automatic, robust and manual crossover / port mirror | 3 / COMPLETE | 18 (SOFTWARE) | 18, 19, 34 | forced_physts | ForcedLinkSettings, crossover/mirror/extended duplex; Configured mode versus observed routing/link | configure_forced, extended_link_configure |
| link / Live and retained link observations | 3 / COMPLETE | 19 (SOFTWARE) | 19 | forced_physts | readLinkStatus and cached LinkSnapshot; Link/current-history validity, age and resolution | observe_link |
| eee / EEE capability, initialization and negotiation | 5 / COMPLETE | 29 (SOFTWARE) | 29, 37, 38, 39 | eee_wire, eee_control_semantics, pin_fields | EEE initialization/settings/status; Local/partner capability, request and negotiated state | eee_initialize, eee_negotiation_status |
| lpi / Legacy MAC LPI sessions | 5 / COMPLETE | 30 (SOFTWARE) | 30, 37, 38, 39, 68 | eee_wire, eee_control_semantics | Finite legacy-MAC LPI session; Accepted/maybe-active state, wake/exit readiness | legacy_lpi |
| wol / Magic Packet and Secure-ON wake | 5 / COMPLETE | 31 (SOFTWARE) | 31 | wol_bit_mask, wol_live_reconfigure, rxfs_sfd_reference | Wake MAC/password, pulse/level/CRC and acknowledgement; Verified settings and retained wake/error evidence | wol_configure, wol_acknowledge |
| power / Active-sleep policy, IEEE/deep power-down, PLL | 5 / COMPLETE | 28 (SOFTWARE) | 28, 37, 38, 39 | deep_power_exit | IEEE power, active-sleep, PLL and deep-power policy; Desired policy, observed power and exit/trust | power_ieee, power_active_sleep, power_pll_control, power_deep |
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
| counters / Single collector for status and destructive counters | 4 / COMPLETE | 20 (SOFTWARE) | 20, 21 | recr_width, rmii_rc, rxfs_sfd_reference, forced_physts | Single destructive acquisition owner; Per-sample time/validity, partial counts/history loss | collect_event_counters |
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
