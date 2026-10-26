# 63 - Implement DP83822 copper identity, reset and link behavior

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md), [FULL_COVERAGE](FULL_COVERAGE.md)
and [VERIFICATION](VERIFICATION.md). Required inputs are resolved applicable
[62](62-p22-source-subtypes.md) source dispositions and the existing atomic MDIO,
identity, reset, link and collector contracts. This task covers copper core
differences, not complete DP83822 family support or physical qualification.

## Current evidence and ownership

Reopen current MB85RC variant metadata/dispatch tests, TMP1x2 variant and
configuration tests, and TUSB32x declared/observed profile validators. Inspect
relevant public source, CLI/runner admission, registration and AGENTS. Record
revisions, dirty state, paths and adopted/adapted/rejected conventions. Derive
changed behavior directly from exact SNLS505H tables/sections.

The lead owns shared selection, reset/link contracts, capabilities and integration.
Delegate independent literal tests or read-only sequence review after contracts
freeze, assigning exact writable/forbidden files and acceptance. Choose available
models suited to coding or silicon/ownership complexity and review results.
Agents do not mutate Git, shared contracts or siblings.

## Work and acceptance

Implement documented copper identity/revision admission, startup/10 Mbit,
initialization, reset/default/strap behavior, owned-state restoration, forced and
advertised 10/100/duplex settings, negotiation/next-page/pause, MDIX and link
resolution. Use exact per-subtype applicability; preserve declared I/H/IF/HF when
registers cannot distinguish them. Do not transfer another family's vendor writes
by address resemblance. Source-proved absent interface/medium combinations reject
before I/O; unknown source facts remain gates rather than unsupported silicon.

Implement copper link/status/event/counter and interrupt-register differences
through the existing collector and typed paths. One collector owns destructive
reads; retain consumed samples when later reads fail while aggregate validity
becomes incomplete. Use exact neutral values for mixed command/status writes and
preserve reset domains. Keep generic 10/100 separate from board 100/full policy,
transport health separate from no-link and configuration trust. Interface timing
and fiber-specific behavior belong to their following prompts; do not publish
complete family support from this core task.

Register explicit host rows for every applicable exact copper part and legal
mode using independent literal SNLS505H vectors and Release-active checks. Cover
shared/unknown/all-zero/all-one identity, wrong declaration/mode/revision,
invalid enum/mask, reset-loaded values, incompatible selections, link flaps,
negotiation/pause and unavailable registers. Pure preflight failures make zero
callbacks; observed identity mismatch allows bounded identity reads but no writes
or later feature I/O. Verify complete startup/configuration/reset traces and
every changed read/write/cleanup boundary, accepted-write/error, reset mid-operation,
cancellation and aggregate/cleanup deadline exhaustion. Check output sentinels,
preserved backend detail, primary/cleanup errors, finite restoration and unresolved
effects. Cached snapshots/capabilities make zero callbacks.

Ship Doxygen exact-subtype restrictions, units, effects and sequence citations;
update README partial evidence, CHANGELOG, variants/API/feature and fixture guides
or record inspected no-change reasons. Run affected host/CLI/Python/docs checks,
original-family regressions and real SDK links for changed adapter inputs. Record
counts, source audit and separate evidence in report, validation and progress.
Check final diff against entry rules; commit/sync verified units and prompt work
on the recorded execution branch. Remote CI execution/status stays only in 71.
