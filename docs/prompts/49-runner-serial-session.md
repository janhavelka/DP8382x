# 49 - Implement bounded serial sessions without implicit hardware actions

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md),
[CLI_CONTRACT](CLI_CONTRACT.md) and [VERIFICATION](VERIFICATION.md).
Prerequisite: [48](48-runner-protocol-artifacts.md). Reuse its parser,
classification and durable bundles as one canonical runner; no second protocol.

## Current evidence and ownership

Reopen current TMP1x2 `SerialSession`/`EvidenceFiles` and tests, INA228
`run_step`/`read_until_hilrun_end` plus short-write/stale-input tests, and
TCA9548A session/artifact handling. Compare their firmware producers and 24Cxx
identity expectations. Inspect optional serial library open/DTR/RTS behavior and
the chosen platform restrictions. Record exact revisions, paths, adopted/adapted/
rejected conventions and actual dependencies; peer behavior is not PHY evidence.

The lead owns session lifetime, cleanup policy and verdict integration. Delegate
disjoint serial fault fixtures or read-only durability review with frozen entry
points and exact paths. Select available models by coding and deadline/failure
complexity. Shared schema, source contracts and Git remain lead-owned.

## Closed work unit

Extend `tools/dp8382x_hil_runner.py` with explicit port/baud, expected firmware/
board/target context, selected suites, output directory and finite command,
session, duration and cleanup limits. Validate arguments, fixture permissions
and limits before opening anything. `--help` and dry-run need neither pyserial
nor SDK and must never open a port. No port scanning/guessing, flashing,
power-cycle, reset, reconnect or command replay occurs automatically.

Reserve and write provisional incomplete evidence before hardware access.
Audit serial-open DTR/RTS behavior; choose a non-resetting configuration where
supported and report unavoidable restrictions before use rather than causing
an implicit reset. Borrow/close serial resources in a stated order and preserve
open/write/read/close errors in artifacts.

Require complete reported firmware identity and boot/session records before
case evaluation, comparing only explicit expectations. Never substitute runner
checkout SHA for flashed identity. Send one correlated command with a total
write deadline. A partial write fails the attempt and is never retried. Read
under aggregate deadline, maximum record/decoder state and total capture limits;
fragmentation cannot turn an unterminated numeric field into completion.

Use the existing strict session/request/operation terminal correlation and
failure classification. Serial loss, reboot, stale/duplicate/conflicting
evidence, wrong firmware, cancellation and timeout remain unsuccessful. Only
explicitly admitted cleanup runs, under a separate finite budget, with residual
effects visible. Preserve first failure when cleanup succeeds. Finalize summary
and hashes only after actual serial/artifact close; close or manifest failure
cannot become PASS. Keep device and host time claims separate.

## Acceptance and checkpoint

Inject open failures, DTR/RTS limitations, partial/zero writes, fragmented reads,
serial disappearance, cancellation, deadline exhaustion, cleanup timeout and
close/finalization failures. Assert no port access on invalid/dry-run options and
no hidden replay. Drive the compiled CLI fixture with bounded process lifetime
to prove actual producer/parser parity. Run help/offline tests without pyserial.
These tests qualify runner behavior, not a physical PHY.

Ship Python lifetime/limits/error docstrings, real and offline invocations in
`docs/hil-runner.md`, CLI protocol updates where needed, README and CHANGELOG.
Review affected guides and source docs, run focused local checks, record gates,
and commit/sync verified units and completion on the recorded branch. Only 71
dispatches or checks remote CI; fixture execution remains separately scoped.
