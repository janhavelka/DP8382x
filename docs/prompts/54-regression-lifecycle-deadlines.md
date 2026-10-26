# 54 - Close lifecycle, deadline and retained-evidence regressions

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md), [ENGINEERING](ENGINEERING.md)
and [VERIFICATION](VERIFICATION.md). Prerequisites:
[53](53-regression-transaction-ownership.md), implemented lifecycle/link/health
and [52](52-hil-soak-interruption.md). Focused tests and documentation already
belong to each behavior task; this closes cross-module invariants.

## Current evidence and ownership

Reopen current peer operation-clock/cancellation tests and the production paths
they exercise, allocation guards, side-effect collectors and retained CLI
results. Compare TMP1x2 explicit operation/source registration and 24Cxx CLI/
Python fixture tests. Recheck exact TI reset, 10BASE-Te and latch/read-clear
sections. Record source revisions, paths, dirty state and adopted/adapted/rejected
conventions; copied timing assumptions are not evidence.

The lead owns lifecycle/timing contract corrections and cross-module coverage.
Delegate disjoint fault cases or independent read-only deadline/cleanup review
with frozen clocks, token and result semantics. Select available models by
reasoning complexity; agents do not change shared headers, progress or Git or
share a mutating build directory.

## Concrete public-source inputs

Add the [observed SDK timing hazards](../reference/library-sdk.md) to the
finite matrix and follow [EVIDENCE](EVIDENCE.md): reset budget below 10 ms after
an accepted write, AN expiry returning success, 100 us/1000 us controller loops,
and 2 ms hardware-reset readiness with coarse ticks. Test required elapsed
intervals and preserved effects, not only loop counts. Silicon nominal FLP/EEE
values cannot become maximum deadlines just because the tests use them.

## Closed work unit

Extend the established fixtures with zero budget, exhausted aggregate deadline,
exact timeout edges, quantized/wrapping/frozen clocks and illegal callback
progress. No extra transfers, unbounded busy loops or fresh full timeout at
each step are allowed. Distinguish primary and finite cleanup budgets; a late
callback does not prove preemption or permission to extend an expired deadline.

Exercise repeated bind/initialize/end, lifecycle reuse, active cancellation,
pending retained results and context teardown. Admission and bus lock are
different lifetimes. Bus-silent end/cancel cannot undo accepted writes or release
borrowed context still referenced by cleanup. Preserve residual effects,
primary/cleanup errors and token identity; consuming/outputting one result may
not overwrite another pending completion.

Verify reset kinds, owned restoration and 10BASE-Te ordering with independent
expected values. Failed stable-bit verification never restores trust. Latch-low,
read-clear and counter observations have one collector despite multiple passive
consumers, and successfully consumed partial samples survive later read failure.
Potentially lost history remains explicit rather than becoming zero events.

Distinguish cable loss, negotiation pending, stale/invalid link, policy mismatch,
configuration uncertainty and backend transport fault. Argument rejection and
passive getters do not count as MDIO attempts; successful unrelated reads do not
repair configuration. Exercise saturating health counters, valid timestamp zero
and independent instances without hidden platform timing or hot-path allocation.

## Acceptance and checkpoint

Execute core/CLI/Python boundary tests with full/slow output and fragmented or
malformed records. Owner cleanup must remain bounded and fair while terminal
output is retained; zero-case, skip-only and incomplete campaigns cannot pass.
Track actual allocations after caller setup, supplementing source checks.
Rebuild affected actual SDK images when lifecycle/public paths change; separate
host, link and physical claims and retain missing-tool gates.

Ship clarified public Doxygen and meaningful procedure comments with fixes.
Update API/timing/ownership/testing guides, `test/README.md`, README and
CHANGELOG; record relevant no-change reasons. Run documentation and focused local
checks, record commands/counts/limits and commit/sync coherent verified units
and completion on the execution branch. Remote CI remains task 71 only.
