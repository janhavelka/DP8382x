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

For a coherent pass with BMCR[12]=0 and current link up, resolve local forced
speed from BMCR[13] (1=100) and duplex from BMCR[8], with pause invalid. Exception:
100/half with CR2[5] not known zero (extension desired or trust not VERIFIED)
has UNKNOWN duplex and unresolved mode. Never use PHYSTS[2:1] with AN disabled;
keep it raw. This describes local mode, not partner duplex; strap-forced BMCR
readback stays source-implied until Q11. No extra read is needed in the existing
10-read plan. With AN enabled, CR2[5]=1, ANER[0]=0 and PHYSTS[2]=1, report 100/full
with pause invalid. Compute RX/TX pause from ANAR/ANLPAR using Table 28B-3;
PHYCR[13:12] is a cross-check and disagreement invalidates pause while retaining
both raw words.

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
Test zero/missing timestamps, uint64 overflow and clock regression, age, repeated readers, reset invalidation,
change evidence and cable-down successful transport accounting. Add a compiling
cached-status example and a guide table separating link, freshness, transport
and application policy.

## Concrete TI source inputs

PHYSTS `10` low bits `0001/0005/0003/0007` decode 100/half, 100/full,
10/half, 10/full only for applicable AN status. A forced 100/half BMCR with PHYSTS
low bits `0007` still reports 100/half when CR2[5] is known zero. Keep
`forced_physts` as a nonblocking raw-value question. BMSR `0004` uses the
latch-low/current two-read plan; retain the first sample even if the second fails.
Test first-low/second-high, first-high/second-low, persistent low and sentinel
output on failure. Extend the single collector, never create another.

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
