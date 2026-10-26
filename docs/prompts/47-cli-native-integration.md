# 47 - Integrate the shared CLI in both P4 applications and close coverage

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md),
[CLI_CONTRACT](CLI_CONTRACT.md), [FULL_COVERAGE](FULL_COVERAGE.md) and
[VERIFICATION](VERIFICATION.md). Prerequisites:
[40](40-p4-network-example.md), [42](42-owner-frame-boundary.md) and
[44](44-cli-configuration-registers.md) through
[46](46-cli-diagnostics-controls.md). Essential blocked inputs stay visible.

## Current evidence and ownership

Reopen current TMP1x2/INA228 native console glue, shared CLI source selection,
bounded output and runner expectations. Compare TCA9548A admission/result tests
and 24Cxx host CLI registration with their actual firmware producers. Inspect
relevant workflows as source, not remote runs. Record revisions, dirty state,
exact paths and adopted/adapted/rejected behavior; no I2C target matrix imports.

The lead owns application integration, grammar and coverage closure. Delegate
disjoint transcript cases or read-only guide/source-list parity review after
contracts freeze. Choose available models by integration/ownership complexity;
reviewers do not edit shared dispatch, headers, progress or Git.

## Closed work unit

Attach the same `examples/common/` processor to ordinary networking, owner/raw
frame P4 application and compiled host fixture. Use one exact schema, grammar,
help and dispatch implementation. Board pins, transport/console settings and
resource lifetime remain application-owned. No framework-specific duplicate
parser, CLI-owned PHY or second link poller is permitted.

Implement bounded UART/input/output service with explicit task/callback context,
fixed queue capacities, loss/truncation counters and observable backpressure.
Do not block driver callbacks or link-critical service to print. Console work
cannot starve operation deadlines or required cleanup; terminal completion
remains retained until accepted. Pending results prevent replacement. Show
board/backend/firmware context sufficient to reproduce a session without
secrets or fabricated source identity.

Exercise actual SDK-aware routing for AN/forced configuration and raw BMCR
reconciliation. All base, raw, application and CLI paths share complete MDIO
transaction ownership; frame-level locking alone is insufficient. Active
application sessions reject link-disrupting commands before transfer, while
passive health/help/config views remain bus-silent. Startup performs no hidden
configuration, scan, reset or diagnostic stimulus.

Reconcile the maintained exhaustive ledger against exact variant/mode, public
operation/field, every legal setting, command/help entry, owner route, output
record, host test and physical requirement. Close primary omissions in owning
code with tests/docs; do not shrink the denominator to exported methods. Separate
unsupported silicon, incomplete implementation and unavailable fixtures. Guards
may check grammar/help/runner parity but cannot replace executable dispatch.

## Acceptance and checkpoint

Build/link both complete actual pinned P4 firmware images with the shared source
list. Execute host and meaningful native integration cases for success,
rejection, busy, timeout, cancel, partial failure and cleanup failure. Test
full/slow console, malformed fragmented input, retained terminal records and
cleanup fairness with frame/state invariants. Record linked versus physical
results separately; no timing or HIL claim follows from source inspection.

Update CLI/platform Doxygen, `docs/cli.md`, both example READMEs, root README,
CHANGELOG and feature/evidence mapping. Review other affected guides, run source
docs and focused local checks, record counts/gates, then commit/sync coherent
checked units and completion on the recorded branch. Remote CI stays at 71.
