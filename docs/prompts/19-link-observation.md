# 19 - Implement live and cached link observations

Follow [COMMON](COMMON.md) and its shared contracts, including
[CONVENTIONS](CONVENTIONS.md), [EVIDENCE](EVIDENCE.md) and
[FULL_COVERAGE](FULL_COVERAGE.md). Begin with the stage-specific rule reminder:
owned paths, fresh relevant sibling audit, selected names, one MDIO owner,
documentation/local checks and the recorded execution branch. Use available
subagents for disjoint work or independent review as COMMON specifies.

Prerequisites: [18](18-forced-crossover-settings.md),
[17](17-negotiated-link-settings.md), [13](13-side-effect-register-access.md),
[4](4-silicon-diagnostic-ledger.md). Verify the concrete inputs below; retain unrelated
open gates under COMMON.

## Scope, current evidence and work

Deliver `readLinkStatus`, passive `linkSnapshot` and retained change
evidence usable by a later adapter. Extend the existing collector/driver or one
cohesive status source. No notification scheduler, SDK callbacks or rendering
belongs in core.

Reopen TMP1x2 `readSnapshot` and `RegisterSnapshot`/
`SettingsSnapshot`, INA228 `SettingsSnapshot` and actual
`readDiagAlert`, plus FieldCore cached `copyStatus`. Inspect their
tests and CLI rendering. Staged software publication is useful; sensor freshness
and CONFIG bookends cannot prove simultaneous PHY sampling.

Specify the exact read plan from SNLS647I with latch-low/current/destructive
semantics, ordering and validity dependencies. Keep current link separate from
consumed historical loss evidence through task 13's single collector.
Distinguish down, negotiating, resolved, forced, stale/unknown observation and
MDIO failure. Numeric defaults never imply valid speed/duplex. Old values after
failure keep old time with explicit invalidity/staleness.

Resolve speed/duplex from exact mode evidence. PHYSTS forced-mode ambiguity
remains a named gate if stronger official evidence does not resolve it; retain
documented raw evidence without inventing resolution. Derive RX/TX pause from
both advertisements and documented ANAR/ANLPAR/PHYCR semantics, valid only under
the required full-duplex negotiation conditions. Forced, half-duplex or incomplete
negotiation cannot imply resolved pause.

Define coherence if link changes between reads. Use a fixed maximum for any
resample or publish uncertainty; never retry until stable. Preserve each consumed
sample immediately even when aggregate acquisition later fails. Task 20 expands
the event/counter acquisition surface; this task must already honor shared
collector ownership and history loss.

Define first-valid, changed, repeated, stale-failure and regained-validity
evidence without duplicate event consumption. Portable applications drive
sampling; the later IDF adapter follows the selected single polling/notification
owner. Cached getters and rendering do no I/O. A valid 10/half link may fail
application 100/full policy without becoming MDIO failure. DHCP/IP/EtherCAT and
machine readiness are not PHY snapshot fields.

Test literal latch-low/current-up/down and negotiation transitions, forced
resolution/gates and independent pause truth-table vectors. Change link between
every read; fail every phase and verify no falsely current aggregate output.
Test zero/missing/wrapped timestamps, age, repeated readers, reset invalidation,
change evidence and cable-down successful transport accounting. Add a compiling
cached-status example and a guide table separating link, freshness, transport
and application policy.

## Concrete TI source inputs

Use the [link helper audit](../reference/library-ti-core.md#negotiation-forced-modes-and-observations).
PHYSTS `0x10` low-bit vectors `0x0001/0x0005/0x0003/0x0007` decode 100/half,
100/full, 10/half, 10/full only where the mode's status semantics are proved;
do not use them to close the ENHANCED forced-mode ambiguity. For BMSR link bit `0x0004`, implement the documented latch-low/current read
plan. Test first-low/second-high, persistent low, first-high/second-low and
second-read failure with previous output true. Every success assigns an explicit
result; preserve consumed first-read history and render only valid fields.
Check current source/datasheet and actual mode/partner traces before asserting
current link, pause or speed; hardware evidence cannot be replaced by literals.

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
