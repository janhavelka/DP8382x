# 54 - Close lifecycle, deadline and retained-evidence regressions

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md),
[ENGINEERING](ENGINEERING.md) and [VERIFICATION](VERIFICATION.md).
Prerequisites: [53](53-regression-transaction-ownership.md), implemented
lifecycle/link/health and [52](52-hil-soak-interruption.md). Focused tests and
documentation already belong to each behavior task; this closes cross-module
invariants.

## Current evidence and ownership

Reopen current peer operation-clock/cancellation tests and the production
paths they exercise, allocation guards, side-effect collectors and retained
CLI results. Compare TMP1x2 explicit operation/source registration and 24Cxx
CLI/ Python fixture tests. Recheck exact TI reset, 10BASE-Te and
latch/read-clear sections. Record source revisions, paths, dirty state and
adopted/adapted/rejected conventions; copied timing assumptions are not
evidence.

Assign paths and select subagent models under COMMON. Keep shared contracts
and Git with the lead; use independent review for ownership and failure
semantics.

## Source requirements and regression vectors

Use [SDK timing evidence](../reference/library-sdk.md) under
[EVIDENCE](EVIDENCE.md). Cover reset budgets below 10 ms, accepted writes
before expiry, AN-control success with negotiation incomplete, nominal 100
us/1000 us controller loops and 2 ms reset readiness with coarse ticks. Assert
elapsed intervals and preserved effects; nominal FLP/EEE values are not
maxima.

## Implement

Extend the established fixtures with zero budget, exhausted aggregate
deadline, exact timeout edges, quantized/wrapping/frozen clocks and illegal
callback progress. No extra transfers, unbounded busy loops or fresh full
timeout at each step are allowed. Distinguish primary and finite cleanup
budgets; a late callback does not prove preemption or permission to extend an
expired deadline.

Exercise repeated bind/initialize/end, lifecycle reuse, active cancellation,
pending retained results and context teardown. Include partial SDK install,
start, stop and uninstall failures, callback drain while another task needs
the owner, same-link mode changes and failed partial notification. Check the
actual instance/snapshot synchronization contract; cached reads are not
inherently thread-safe. Reject stale-session queued frames after link
loss/readmission. Admission and bus lock are different lifetimes. Bus-silent
end/cancel cannot undo accepted writes or release borrowed context still
referenced by cleanup. Preserve residual effects, primary/cleanup errors and
token identity; consuming/outputting one result may not overwrite another
pending completion.

Verify reset kinds, owned restoration and 10BASE-Te ordering with independent
expected values. Failed stable-bit verification never restores trust.
Latch-low, read-clear and counter observations have one collector despite
multiple passive consumers, and successfully consumed partial samples survive
later read failure. Potentially lost history remains explicit rather than
becoming zero events.

Distinguish cable loss, negotiation pending, stale/invalid link, policy
mismatch, configuration uncertainty and backend transport fault. Argument
rejection and passive getters do not count as MDIO attempts; successful
unrelated reads do not repair configuration. Exercise saturating health
counters, valid timestamp zero and independent instances without hidden
platform timing or hot-path allocation.

## Acceptance

Execute core/CLI/Python boundary tests with full/slow output and fragmented or
malformed records. Cover attach-after-boot session handshake, partial input
followed by cancellation, session replacement with pending work, late
duplicate terminal records and payload corruption after manifest creation.
Owner cleanup must remain bounded and fair while terminal output is retained;
zero-case, skip-only and incomplete campaigns cannot pass. Track actual
allocations after caller setup, supplementing source checks. Rebuild affected
actual SDK images when lifecycle/public paths change; separate host, link and
physical claims and retain missing-tool gates.

Update Doxygen, API/timing/ownership/testing guides and `test/README.md` with
corrected lifecycle, scheduler, result and partial-effect contracts.

Review README/CHANGELOG and affected guides. Run focused local and
source-documentation checks (Doxygen where applicable), then update
evidence/progress under COMMON. Commit/sync checked units and completion on
the execution branch; first-set remote CI belongs only to 71.
