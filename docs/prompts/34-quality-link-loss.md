# 34 - Implement quality and hardware link-loss controls

Follow [COMMON](COMMON.md) and its shared contracts, including
[CONVENTIONS](CONVENTIONS.md), [EVIDENCE](EVIDENCE.md) and
[FULL_COVERAGE](FULL_COVERAGE.md). Begin with the stage-specific rule reminder:
owned paths, fresh relevant sibling audit, selected names, one MDIO owner,
documentation/local checks and the recorded execution branch. Use available
subagents for disjoint work or independent review as COMMON specifies.

Prerequisites: [20](20-event-counter-acquisition.md),
[18](18-forced-crossover-settings.md), [5](5-silicon-power-interface-ledger.md),
[33](33-clock-interface-repeater.md). Verify the concrete inputs below; retain unrelated
open gates under COMMON.

## Scope, current evidence and work

Deliver remaining raw quality/MSE, polarity/jabber, link-loss recovery and
MII Fast Link Drop settings. Reconcile tasks17-20 and33 so controls and collector
have one owner.

Re-open the [FLD/quality source facts](../reference/library-ti-features.md#led-interface-and-link-quality-controls)
and SNLS647I tables 8-29/30/52/53/72. CR3 cause mask `0x040F`
combines energy/SNR/MLT3/RX/descrambler; reject unknown bits before I/O.
FLD_CFG1 `0117` window `[15:10]` has 1=2 clocks, 63=64 clocks at 8 ns,
and threshold `[9:4]` accepts 1..63 errors; both zero codes are reserved.
Qualify how these controls relate to generic 10 us prose. Datasheet maxima are
10 us for modes 1-4 and 11 us for descrambler, excluding host observation latency.
Keep MSE `0218` versus app-note `0225` unresolved until evidence closes it.
Test CR2 bit 1 clear=odd-nibble detection enabled/set=disabled, including mode
constraints, and CR2 bit 2 RX_ER-during-IDLE independently of GENCFG IPG.

Reopen INA228 raw diagnostic validity/destructive reads, TMP1x2 cached
snapshots/configuration verification and current LDC1614 deferred-event source/
tests. Inspect their CLI/error projections and registration. Recheck exact
SNLS647I quality, PHYCR/next-page, event/error and Fast Link Drop feature text;
TI C2000/ESC app notes provide context, not ESP32-P4 master recipes.

Enumerate each remaining programmable control and observable field, its legal
encoding, interface restriction, validity, latch/clear and reset behavior.
Reuse task 20's FCSCR/RECR acquisition and task 17's next-page configuration;
add the missing documented evidence/controls here without a second reader.
Resolve source/scaling conflicts before publishing SNR or quality units from
raw MSE. Source-defined raw data remains usable with explicit validity while
unsupported interpretations stay gated.

Fast Link Drop nonzero cause sets are MII-only and reject on RMII before I/O.
A zero-cause disable and FLDS collection stay legal on RMII. CR3's reset low
nibble is unknown; observe rather than assert a constant. Modes2,3,5 claim their
maxima only with RX-error or energy-loss enabled (footnote5); accept other legal
profiles without that maximum claim. MLT3 selects0117=0417; descrambler selects
0131[5:0]=08. Other thresholds are explicit raw overrides under
`fld_threshold_recipe`. Hardware link recovery is not software retry policy.

Validate complete settings and maintenance requirements before I/O. Use exact
mixed-field composition, preserve consumed events and reserved bits, apply once
and verify stable owned fields. Extend reset/power/recovery restoration and
raw/external invalidation through existing paths. Some policy settings depend
on partner events: successful configuration does not guarantee a timed physical
link transition.

Test independent literal masks and all legal extrema, polarity/jabber/next-page
evidence, raw MSE validity and source-defined interpretations only. Test MII
acceptance/RMII nonzero-cause zero-I/O rejection, simultaneous event acquisition, stale/unknown
samples and counters without treating them as MDIO health failures. Fail every
write/readback/restore phase, accepted-write timeout and destructive read-after-
effect. Preserve primary/cleanup status, partial samples and exact bounds.

Compile a raw-quality/next-page observation example and an explicit MII FLD
configuration example. Document interface constraints, application recovery
ownership and physical latency/quality measurement limits. Unwired or unavailable
MII fixtures remain an open physical gate, not omitted software functionality.

Own `tx100_configure`: PHYSCR[11] scrambler bypass, [9:8] FIFO codes 0/1/2/3
for4/5/6/8 nibbles, andCR2[2]. Own `tx10_configure`:10BTSCR[13] threshold,
[12:9] squelch codes 0..8 for200..600mV in50mV steps (reject9..F),[7]NLP
disable,[0]jabber disable and002A[14] short1/long0 preamble. Every10BTSCR read,
RMW and readback collects bit 4 and its PHYSTS[12] dependency. VOD_CFG3 owned
maskFFC0 keeps the mandatory4A40 contract; ANA_RX10BT_CTRL stays raw under its
gate. Reuse prompt 33's CR2[1] setter and add only interaction tests; CR2[2]
RX_ER-during-IDLE remains separate from GENCFG IPG.

## Documentation, verification and checkpoint

Update public Doxygen, procedure citations, affected API/user/test/tool guides,
coverage, stage report and progress with this behavior. Review README/CHANGELOG
and record relevant no-change reasons. Run the focused registered host tests
above with active Release checks, affected portable-consumer and Doxygen checks,
`python tools/verify_references.py` and `git diff --check`.

Apply COMMON's final maintainability review and evidence reporting. Keep source
gaps, SDK/package results and physical qualification separate. The lead commits
and syncs this complete task and coherent verified units on the same execution
branch, verifies upstream equality, and reports any sync failure. First-set
remote CI runs and status checks belong only to prompt 71.
