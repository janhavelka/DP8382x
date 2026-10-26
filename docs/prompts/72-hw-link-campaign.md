# 72 - Execute admitted physical boot, link and recovery campaigns

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md), [FULL_COVERAGE](FULL_COVERAGE.md)
and [VERIFICATION](VERIFICATION.md). Required inputs are actual implemented
part-specific paths, linked firmware, runner/fixture suites from prompts 48–52
and an explicitly available authorized fixture. Preserve prompt 71's actual
CI-tested SHA separately from subsequent local or firmware revisions.

## Current evidence and ownership

Reopen `../TCA9548A/tools/test_hil_artifacts.py::ArtifactTests` and its actual
runner, plus `../MB85RC/tools/hil_runner.py::payload_failure_reason/strict_failure_reasons`
and tests. Inspect current CLI/firmware identity, Python runner, registration,
AGENTS and exact board/netlist/SDK sources. Record revisions, dirty state,
paths and adopted/adapted/rejected evidence/restoration rules; memory scratch
procedures are not PHY test requirements.

The lead owns fixture admission and interpretation. Delegate read-only artifact
review or disjoint host regression fixes with explicit writable/forbidden paths
and available models suited to task complexity. Review every contribution.
Routine Git authorization and a numbered prompt do not authorize flashing,
resetting a fixture or operating a machine; use separately authorized operations.
No incidental servo motion or loading-machine control.

## Work and acceptance

Before each case, record exact PHY part/revision/interface/address, board/netlist,
reset/strap/clock arrangement, partner/cable, SDK, actual firmware-reported SHA/
dirty identity, separate host runner revision, initial state, correlated request
token, finite duration,
observations, restoration and pass limits. Recheck wiring before any authorized
flashing; the planning board snapshot is not final netlist authority. Independently
record negotiated versus forced peer settings, rather than trusting a nominal
“100/full” label or the host checkout identity.
Extend the existing hardware-validation guide and runner fixture manifest;
do not create a competing campaign schema.

Prioritize DP83826IRHBR/P4 cold boot, each applicable reset/restart, owned
configuration restoration, 100/full-only advertisement and resolved-policy
rejection, unplug/replug, peer reset, repeated flaps, incompatible partner,
incomplete negotiation and bounded recovery. Exercise generic 10 Mbit and legal
duplex/mode differences on suitable configured peers separately from the first
board profile. Include controlled management-transport faults where feasible.
Distinguish PHY status/counters, raw MAC packet handoff and real IP traffic;
successful serial parsing is not packet evidence. Internet/DNS failure alone
does not establish PHY failure or broaden application scope.
Include a finite sustained ordinary-traffic case with actual duration, packet/
error counts, partner conditions and final restoration evidence.

Run only actual supported exact-part suites. Preserve bounded raw transcript,
structured per-case results and artifact manifest/hashes in unique directories;
failed/interrupted runs survive passing retries. Reject stale/duplicate/truncated
results, firmware mismatch, missing cases, skipped-only campaigns and artifact
persistence failure. Retain restored versus untrusted state and separate
primary/cleanup verdicts. Fix confirmed library/runner defects with host
regressions and rerun affected authorized cases.

Without fixture or authorization, finish useful readiness/case inventory, record
the precise dependency and next reproducible case, and leave this physical gate
BLOCKED/NOT RUN. Do not mark it DONE or the entire set COMPLETE; continue eligible
software/docs work. Update Doxygen/tests with behavior fixes, README qualification,
CHANGELOG, hardware/fixture guides, validation/progress and this prompt's report,
including no-change reasons. Run focused local checks. Check final diff and
commit/sync checked units on the execution branch; do not dispatch/poll/wait
for remote CI here. Timing/features belong to [73](73-hw-feature-campaign.md).
