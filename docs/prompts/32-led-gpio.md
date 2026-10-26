# 32 - Implement all LED and GPIO controls

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

Prerequisites: [27](27-interrupt-service.md), [31](31-wake-on-lan.md), [5](5-silicon-power-interface-ledger.md); only the concrete inputs used below are required.

## Scope, current evidence and work

Deliver documented LED/GPIO sources, mux, polarity, drive, blink and stretch
controls with exact applicability, conflict validation and verified restoration.
Extend coherent typed configuration and shared pin ownership rules; no class
per pin or field, generic pin registry or alternate raw-setter aliases.

Reopen PCA9555 desired/observed and raw shadow-invalidation code/tests,
LDC1614 deferred interrupt/pin source and relevant SCD41/RV3032-C7 public IRQ
controls. Inspect actual declarations, dispatcher/tests and current registration.
Use peer validation/ownership patterns only; SNLS647I defines PHY pin functions,
electrical encodings and reset behavior.

Make a complete legal-settings table for every documented pin/source/mux/drive/
polarity/blink/stretch field. Distinguish reset-loaded/strap behavior, runtime
programmability and observable hardware-only properties. Unknown or reserved
encodings are rejected, not treated as tuning choices. Unconnected initial-board
pins still belong to the generic software API.

Validate the entire pin profile before I/O, including conflicts with interrupt,
WoL and source-defined clock/interface uses. Preserve each existing user's owned
settings; a later setter cannot silently steal a pin or bypass the IRQ/WoL
collector. For documented runtime transitions, specify admission and ordering.
Source-proved unsafe/unavailable live transitions reject and document the
application's required stopped/reconfigured state.

Apply command-neutral writes once, verify stable owned bits and publish desired,
observed and trusted settings separately. Extend every applicable reset and
power kind's restoration through the common ledger; do not restore status or
resample a POR-only strap by assertion. Preserve unknown effects after accepted
writes fail and invalidate relevant pin evidence after raw/external mutation.

Short settings use bounded direct calls where appropriate; scheduled waits use
the existing operation engine only when required by a real procedure. Keep
aggregate deadlines, complete RMW/MMD ownership and no lock across waits.
Cancellation/end cannot undo a pin mutation; restore failure leaves untrusted
effects visible with independent primary/cleanup status.

Test independent literal masks/encodings for each source and legal extrema,
invalid/reserved values, simultaneous user conflicts and mode restrictions.
Inject failures before/after every write/readback/restore, including ambiguous
pin reassignment. Verify reserved-bit and pending-event preservation, exact
transaction limits, zero-I/O rejection and reset/power replay.

Compile examples for a normal LED profile and a conflict rejection. Document
units, physical pin dependencies and the difference between configured drive/
blink behavior and measured electrical/timing performance. Do not claim actual
visible LEDs, IRQ delivery or external clocks from host register tests.

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
