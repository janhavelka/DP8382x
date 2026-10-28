# DP83826I feature coverage

This matrix summarizes the [authoritative ledger](registers/dp83826i.json); it does
not duplicate its numeric map. Research COMPLETE means source decisions and named
gates are recorded, not an implemented driver. All software implementation remains
PENDING; driver host tests, SDK builds and hardware qualification are NOT RUN.
The validator/unit tests exercise the research records only.

| Feature ID / responsibility | Research owner and state | Implementation stages | Open evidence gates |
| --- | --- | --- | --- |
| identity / Exact identity and capabilities | Prompt 3 / COMPLETE | 14 | None named |
| access / Clause 22 and complete indirect access | Prompt 3 / COMPLETE | 11, 12, 13 | None named |
| reset / PCS, hard reset, restart and reconciliation | Prompt 3 / COMPLETE | 15, 16 | hard_reset_straps, pcs_reset_extent |
| initialization / Required 10BASE-Te initialization | Prompt 3 / COMPLETE | 15, 17, 18 | None named |
| negotiation / Advertisement, forced-compatible negotiation and pause | Prompt 3 / COMPLETE | 17, 19, 37, 38, 39 | None named |
| next_page / Complete next-page exchange | Prompt 3 / COMPLETE | 17, 19, 20, 29, 34 | next_page_handshake |
| crossover / Automatic, robust and manual crossover / port mirror | Prompt 3 / COMPLETE | 18, 19, 34 | forced_physts |
| link / Live and retained link observations | Prompt 3 / COMPLETE | 19 | forced_physts |
| eee / EEE capability, initialization and negotiation | Prompt 5 / PENDING | 29, 37, 38, 39 | eee_wire |
| lpi / Legacy MAC LPI sessions | Prompt 5 / PENDING | 30, 37, 38, 39, 68 | eee_wire |
| wol / Magic Packet and Secure-ON wake | Prompt 5 / PENDING | 31 | None named |
| power / Active-sleep policy, IEEE/deep power-down, PLL | Prompt 5 / PENDING | 28, 37, 38, 39 | None named |
| repeater / RMII repeater arrangements | Prompt 5 / PENDING | 33, 68 | None named |
| clock / Clock output and reset/power interactions | Prompt 5 / PENDING | 33 | pin_fields |
| mii / MII interface controls | Prompt 5 / PENDING | 33, 37, 38, 39 | None named |
| rmii / RMII interface, elasticity and status | Prompt 5 / PENDING | 33, 37, 38, 39 | rmii_rc |
| tx100 / 100BASE-TX exposed data-path controls | Prompt 5 / PENDING | 34, 35 | None named |
| tx10 / 10BASE-Te receiver, NLP, jabber and preamble | Prompt 5 / PENDING | 34, 35 | None named |
| loopback / All documented loopback modes | Prompt 4 / PENDING | 23 | loopback_recipe |
| bist / BIST/PRBS modes and retained acquisition | Prompt 4 / PENDING | 24 | None named |
| cable / Manual and autorun cable capture | Prompt 4 / PENDING | 25, 26 | tdr_reserved |
| fast_drop / MII-only Fast Link Drop | Prompt 4 / PENDING | 34 | fld_reset_defaults |
| link_recovery / Hardware link-loss recovery | Prompt 5 / PENDING | 18, 34 | None named |
| led / LED sources, blink, stretch and drive | Prompt 5 / PENDING | 32 | pin_fields |
| pin_mux / Per-pin GPIO, wake, IRQ and clock mux | Prompt 5 / PENDING | 32, 33 | pin_fields |
| interrupts / Global IRQ and all event enables/service | Prompt 4 / PENDING | 20, 27 | None named |
| counters / Single collector for status and destructive counters | Prompt 4 / PENDING | 20, 21 | recr_width, rmii_rc |
| quality / Raw MSE and supported quality interpretation | Prompt 4 / PENDING | 34 | mse_address |
| compliance / Finite compliance stimulus procedures | Prompt 5 / PENDING | 35, 51, 73 | compt_reserved |
| analog / Analog amplitude, termination and MAC output | Prompt 5 / PENDING | 35 | ioctrl_example |
| compatibility / IPG and compatibility controls | Prompt 5 / PENDING | 33, 35 | None named |
| calibration / Resistor calibration contingency | Prompt 4 / PENDING | 15, 16, 25, 35 | None named |
| straps / POR/pin straps and sampled configuration | Prompt 3 / COMPLETE | 14, 16, 33 | hard_reset_straps |
| outside_phy / MAC/DMA, stack, board power and scheduling | Prompt 3 / COMPLETE | 6, 37, 40, 41, 42 | None named |
| absent / Unsupported exact-part interfaces and features | Prompt 3 / COMPLETE | 14 | None named |

Every feature has planned typed API, owner-routed CLI and test responsibilities in
the ledger. Hardware straps, outside-PHY work and source-proved absent capabilities
are classified explicitly; their observations/rejections still need implementation
tests. A board wiring limitation cannot remove a software feature.

Prompt 4 owns diagnostic/event/counter research, including all-reserved TDR entries,
RC acquisition, loopback, BIST, cable, MSE, FLD and calibration. Prompt 5 owns EEE
printed namespaces and reset-bearing recipes, low power/LPI/WoL, per-pin LED/GPIO,
clock/interface, compliance and remaining analog/compatibility controls. Shared
register fields already partitioned in prompt 3 retain these feature procedure owners.

On each extension run the [ledger checks](registers/README.md), add independent
literal vectors, and update this matrix with source/implementation evidence.
Do not equate source inspection, ledger tests, linked SDK images and physical results.
Physical campaigns retain their separate prompts 72–73 owners.
