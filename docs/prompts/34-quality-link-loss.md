# 34 - Implement quality and hardware link-loss controls

Read [COMMON](COMMON.md), [ENGINEERING](ENGINEERING.md),
[CONVENTIONS](CONVENTIONS.md), [FULL_COVERAGE](FULL_COVERAGE.md) and
[VERIFICATION](VERIFICATION.md). Use [SOURCE_MAP](SOURCE_MAP.md) for current
source locations; historical reports do not replace a fresh audit.

At entry record owned paths, applicable rules, current source revisions/dirty
state, selected names, one MDIO owner, local checks and the recorded execution
branch. Resume that branch. Use disjoint implementation/test work or independent
read-only review when useful; choose available models/effort by task complexity
and record the rationale. The lead owns shared contracts, integration and Git.
Siblings remain read-only.

Prerequisites: [20](20-event-counter-acquisition.md), [18](18-forced-crossover-settings.md), [5](5-silicon-power-interface-ledger.md), [33](33-clock-interface-repeater.md); only the concrete inputs used below are required.

## Scope, current evidence and work

Re-open the [FLD/quality source facts](../reference/library-ti-features.md#led-interface-and-link-quality-controls)
and SNLS647I tables 8-29/30/52/53/72. CR3 cause mask `0x040F`
combines energy/SNR/MLT3/RX/descrambler; reject unknown bits before I/O.
FLD_CFG1 `0117` window `[15:10]` has 1=2 clocks, 63=64 clocks at 8 ns,
and threshold `[9:4]` accepts 1..63 errors; both zero codes are reserved.
Qualify how these controls relate to generic 10 us prose. Datasheet maxima are
10 us for modes 1-4 and 11 us for descrambler, excluding host observation latency.
Keep MSE `0218` versus app-note `0225` unresolved until evidence closes it.
Include the MCU odd-nibble polarity bug (setting CR2 bit 1 disables detection)
and RX_ER-during-IDLE versus IPG naming as independent regression cases.

Deliver remaining documented raw quality/MSE, polarity/jabber, next-page
evidence, MII-only Fast Link Drop and hardware link-loss-recovery settings.
Reconcile this group against tasks 17–20 so every field has one implementation
and collector rather than overlapping helpers.

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

Ship accurate public Doxygen and procedure citations with this behavior: units,
legal states, ownership, output validity, I/O effects, deadlines and cleanup.
Update API contracts, affected guides, restoration/feature evidence and the stage
report/progress. Review README, CHANGELOG and relevant example/test/tool guides;
update affected claims and record why others need no change. Future APIs and
physical qualification remain explicitly unimplemented/unverified.

Run registered focused host tests with active Release checks, relevant portable
consumer/source-documentation/link checks, the reference checker and
`git diff --check`. Record actual commands and separate PASS/FAIL/NOT RUN/BLOCKED
for host, SDK, packages and hardware where relevant. Missing source evidence
blocks the affected requirement; it cannot become unsupported silicon or close
a required gate. Continue independent eligible work.

Review the final diff for simple control flow, stale comments, duplicated
procedures and undocumented errors. Commit and sync checked tests/docs/code on
the same execution branch after this task and each coherent verified unit.
The lead fetches, safely integrates, pushes and verifies upstream equality;
report exact sync failures. First-execution remote CI dispatch, polling and
waiting belong only to checkpoint **71**, never this task.
