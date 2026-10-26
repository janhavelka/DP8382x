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

Deliver remaining documented raw quality/MSE, polarity/jabber, remaining next-page evidence through task 17's exchange and task 20's collector,
 MII-only Fast Link Drop and hardware link-loss-recovery settings.
Reconcile this group against tasks 17–20 so every field has one implementation
and collector rather than overlapping helpers.

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

SNLS647I section 8.4.1.1 limits Fast Link Drop to MII. Implement its documented
options for that interface and reject RMII use before transfers. Initial RMII
operation cannot qualify the MII feature. Keep silicon link-loss-recovery
configuration separate from the application's retry/backoff/recovery decisions;
no autonomous software reset or policy switch belongs in core.

Validate complete settings and maintenance requirements before I/O. Use exact
mixed-field composition, preserve consumed events and reserved bits, apply once
and verify stable owned fields. Extend reset/power/recovery restoration and
raw/external invalidation through existing paths. Some policy settings depend
on partner events: successful configuration does not guarantee a timed physical
link transition.

Test independent literal masks and all legal extrema, polarity/jabber/next-page
evidence, raw MSE validity and source-defined interpretations only. Test MII
acceptance/RMII zero-I/O rejection, simultaneous event acquisition, stale/unknown
samples and counters without treating them as MDIO health failures. Fail every
write/readback/restore phase, accepted-write timeout and destructive read-after-
effect. Preserve primary/cleanup status, partial samples and exact bounds.

Compile a raw-quality/next-page observation example and an explicit MII FLD
configuration example. Document interface constraints, application recovery
ownership and physical latency/quality measurement limits. Unwired or unavailable
MII fixtures remain an open physical gate, not omitted software functionality.

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
