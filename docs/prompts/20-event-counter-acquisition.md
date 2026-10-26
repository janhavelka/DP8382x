# 20 - Implement complete event and counter acquisition

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

Prerequisites: [19](19-link-observation.md), [13](13-side-effect-register-access.md), [4](4-silicon-diagnostic-ledger.md); only the concrete inputs used below are required.

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
support them. Unresolved RECR arithmetic remains visibly gated while independently
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
