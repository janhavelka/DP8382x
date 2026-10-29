# DP83826I feature coverage

This matrix summarizes the [authoritative ledger](registers/dp83826i.json),
without duplicating its numeric map. All 35 families have complete source
dispositions. All 79 detailed entries, 17 opaque recipe records and five
blocked printed EEE associations are accounted for; no research row is PENDING.
The 52 procedure contracts have explicit API/CLI/test ownership and host/fixture plans.

Research COMPLETE includes selected interpretations and unresolved source gates.
Software implementation remains PENDING; driver host tests, SDK builds and
hardware qualification are NOT RUN. Ledger tests establish their own research layer.

Prompt 6 completes the [portable/native ownership design](design-decisions.md)
for all these entry families. It introduces no feature support claim. The native
lifecycle correction is a named prompt-37 dependency; existing silicon source
gates remain unchanged. Full pause evidence, SDK projection, raw access and the
single collector have explicit owners in that matrix.

Prompt 7's [complete typed feature map](api-contracts.md#complete-typed-feature-map)
maps all 35 feature IDs and all 52 procedure IDs to planned configuration/actions
and observable results. Its normative legal-value rule includes all 283 field
records and gated/opaque actions without duplicating the ledger. Only foundational
values/direct signatures are frozen; feature-specific signatures arrive with the
owning behavior. No raw API substitutes for a multistep typed procedure.

| Feature / responsibility | Research | Primary implementation owner | Related integration/test stages | Source gates |
| --- | --- | --- | --- | --- |
| identity / Exact identity and capabilities | 3 / COMPLETE | 14 (SOFTWARE) | 14 | None named |
| access / Clause 22 and complete indirect access | 3 / COMPLETE | 11 (SOFTWARE) | 11, 12, 13 | eee_wire |
| reset / PCS, hard reset, restart and reconciliation | 3 / COMPLETE | 15 (SOFTWARE) | 15, 16 | hard_reset_straps, pcs_reset_extent |
| initialization / Required 10BASE-Te initialization | 3 / COMPLETE | 15 (SOFTWARE) | 15, 17, 18 | None named |
| negotiation / Advertisement, forced-compatible negotiation and pause | 3 / COMPLETE | 17 (SOFTWARE) | 17, 19, 37, 38, 39 | None named |
| next_page / Complete next-page exchange | 3 / COMPLETE | 17 (SOFTWARE) | 17, 19, 20, 29, 34 | next_page_handshake |
| crossover / Automatic, robust and manual crossover / port mirror | 3 / COMPLETE | 18 (SOFTWARE) | 18, 19, 34 | forced_physts |
| link / Live and retained link observations | 3 / COMPLETE | 19 (SOFTWARE) | 19 | forced_physts |
| eee / EEE capability, initialization and negotiation | 5 / COMPLETE | 29 (SOFTWARE) | 29, 37, 38, 39 | eee_wire, eee_control_semantics, pin_fields |
| lpi / Legacy MAC LPI sessions | 5 / COMPLETE | 30 (SOFTWARE) | 30, 37, 38, 39, 68 | eee_wire, eee_control_semantics |
| wol / Magic Packet and Secure-ON wake | 5 / COMPLETE | 31 (SOFTWARE) | 31 | wol_bit_mask, wol_live_reconfigure, rxfs_sfd_reference |
| power / Active-sleep policy, IEEE/deep power-down, PLL | 5 / COMPLETE | 28 (SOFTWARE) | 28, 37, 38, 39 | deep_power_exit |
| repeater / RMII repeater arrangements | 5 / COMPLETE | 33 (SOFTWARE) | 33, 68 | rmii_rc |
| clock / Clock output and reset/power interactions | 5 / COMPLETE | 33 (SOFTWARE) | 33 | pin_fields |
| mii / MII interface controls | 5 / COMPLETE | 33 (SOFTWARE) | 33, 37, 38, 39 | rmii_rc |
| rmii / RMII interface, elasticity and status | 5 / COMPLETE | 33 (SOFTWARE) | 33, 37, 38, 39 | rmii_rc |
| tx100 / 100BASE-TX exposed data-path controls | 5 / COMPLETE | 34 (SOFTWARE) | 34, 35 | None named |
| tx10 / 10BASE-Te receiver, NLP, jabber and preamble | 5 / COMPLETE | 34 (SOFTWARE) | 34, 35 | analog_rx_gain |
| loopback / All documented loopback modes | 4 / COMPLETE | 23 (SOFTWARE) | 23 | loopback_recipe |
| bist / BIST/PRBS modes and retained acquisition | 4 / COMPLETE | 24 (SOFTWARE) | 24 | bist_result_semantics |
| cable / Manual and autorun cable capture | 4 / COMPLETE | 25 (SOFTWARE) | 25, 26 | tdr_reserved, tdr_calibration, tdr_freshness_abort |
| fast_drop / MII-only Fast Link Drop | 4 / COMPLETE | 34 (SOFTWARE) | 34 | fld_reset_defaults, fld_threshold_recipe |
| link_recovery / Hardware link-loss recovery | 5 / COMPLETE | 18 (SOFTWARE) | 18, 34 | None named |
| led / LED sources, blink, stretch and drive | 5 / COMPLETE | 32 (SOFTWARE) | 32 | pin_fields |
| pin_mux / Per-pin GPIO, wake, IRQ and clock mux | 5 / COMPLETE | 32 (SOFTWARE) | 32, 33 | pin_fields |
| interrupts / Global IRQ and all event enables/service | 4 / COMPLETE | 20 (SOFTWARE) | 20, 27 | rxfs_sfd_reference |
| counters / Single collector for status and destructive counters | 4 / COMPLETE | 20 (SOFTWARE) | 20, 21 | recr_width, rmii_rc, rxfs_sfd_reference, forced_physts |
| quality / Raw MSE and supported quality interpretation | 4 / COMPLETE | 34 (SOFTWARE) | 34 | mse_address |
| compliance / Finite compliance stimulus procedures | 5 / COMPLETE | 35 (SOFTWARE) | 35, 51, 73 | compt_reserved |
| analog / Analog amplitude, termination and MAC output | 5 / COMPLETE | 35 (SOFTWARE) | 35 | ioctrl_example, analog_rx_gain |
| compatibility / IPG and compatibility controls | 5 / COMPLETE | 33 (SOFTWARE) | 33, 35 | None named |
| calibration / Resistor calibration contingency | 4 / COMPLETE | 15 (SOFTWARE) | 15, 16, 25, 35 | hard_reset_straps |
| straps / POR/pin straps and sampled configuration | 3 / COMPLETE | 14 (HARDWARE_ONLY) | 14, 16, 33 | hard_reset_straps |
| outside_phy / MAC/DMA, stack, board power and scheduling | 3 / COMPLETE | 6 (OUTSIDE_PHY) | 6, 37, 40, 41, 42 | None named |
| absent / Unsupported exact-part interfaces and features | 3 / COMPLETE | 14 (ABSENT) | 14 | None named |

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
