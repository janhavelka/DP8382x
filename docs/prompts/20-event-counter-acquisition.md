# 20 - Implement complete event and counter acquisition

Follow [COMMON](COMMON.md) and its shared contracts, including
[CONVENTIONS](CONVENTIONS.md), [EVIDENCE](EVIDENCE.md) and
[FULL_COVERAGE](FULL_COVERAGE.md). Begin with the stage-specific rule reminder:
owned paths, fresh relevant sibling audit, selected names, one MDIO owner,
documentation/local checks and the recorded execution branch. Use available
subagents for disjoint work or independent review as COMMON specifies.

Prerequisites: [19](19-link-observation.md), [13](13-side-effect-register-access.md),
[4](4-silicon-diagnostic-ledger.md). Verify the concrete inputs below; retain unrelated
open gates under COMMON.

## Scope, current evidence and work

Complete the single collector for MISR1/MISR2 events, FCSCR/RECR counters and
source-backed chip diagnostic evidence. Provide typed live acquisition and cached
results with exact validity/overflow semantics. Extend the existing collector,
not a second IRQ, dump or diagnostic polling path.

Reopen INA228 `DiagAlert`/`readDiagAlert` source and tests, TMP1x2
staged snapshot/public validity contracts and PCA9555 cleanup evidence cases.
Inspect their cached CLI behavior and registration. Reopen SNLS647I tables
8-36 through 8-39 and the recorded RECR width/saturation conflict using original
PDF tables; a vendor macro does not settle inconsistent source facts.

For each source specify read-clear/latch/counter overflow, acquisition order,
timestamp and current versus historical meaning. MISR1/MISR2 upper status bits
and FCSCR/RECR clear on read. Store each successful consumed sample immediately;
MISR1 success followed by MISR2 failure must not erase MISR1 evidence. A callback
timeout after hardware clearing but before valid returned data records possible
history loss. It cannot become zero events, zero errors or a successful batch.

Keep complete aggregate publication separate from per-source evidence. Define
counter accumulation/reset, saturation and validity only where exact-part sources
support them. A saturated sample is a lower bound; a wrapped sample cannot give an
exact total without additional evidence. Separate collector-history consumption
from hardware counter clearing and define whether reset starts a new observation
epoch or preserves earlier evidence with its old validity. Bound retained storage
and expose coalescing/overflow instead of claiming lossless event multiplicity. Unresolved RECR arithmetic remains visibly gated while independently
documented raw acquisition stays available. Separate chip receive/error counts
from MDIO frame activity and managed health.

Coordinate live link sampling, explicit diagnostic requests and future IRQ
service through one owner and collector. Cached readers, CLI formatting and
safe dumps cannot consume evidence again. Define event retention/consumption
with small fixed values and bounded work, not an allocated event framework.
No unlimited drain/resampling loop and no invented atomic silicon snapshot.
Preserve primary versus cleanup/acquisition failures.

Use independent literal samples for each event bit and supported counter
boundary/overflow behavior. Test simultaneous sources, new events between reads,
repeated acquisition, stale evidence, read-after-effect timeout and failure at
every phase. Assert aggregate sentinel output, retained partial samples,
history-loss flags, per-source timestamp validity and exact maximum transactions.
Test two cached readers, reset invalidation and contention with MMD diagnostics;
the collector's transaction lock cannot span waits.

Update the diagnostics/link guides with examples of valid partial evidence,
unknown loss and separate counter domains. Compile a consumer that acquires once
and renders multiple cached views without I/O. Host tests prove acquisition
semantics; physical error rates remain unmeasured.

## Concrete TI source inputs

Use the [source/datasheet collector table](../reference/library-ti-core.md#destructive-reads-counters-and-safe-diagnostic-output).
MISR1/MISR2 `0x12/0x13` have high-byte RC events (`0xFF00`) and low-byte
enables (`0x00FF`) with different event meanings. FLDS `0x0F` cause mask is `0x01F0`; include its read-clear causes in the single
collector and exclude it from bus-active safe dumps.
FCSCR `0x14` is an 8-bit saturating RC counter, maximum `0xFF`, interrupt on
exceeding `0x7F`. RECR `0x15` remains a 16-bit-field versus `0xFF` saturation
source conflict; RCSR `0x17` bits3/2 remain an RC/polarity conflict. Retain raw
values and partial consumed samples instead of inventing count or fault semantics.
Reopen current exact tables and retain physical counter/event verification as
a separate qualification gate.

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
