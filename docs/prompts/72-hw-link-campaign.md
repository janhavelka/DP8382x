# 72 - Execute admitted physical boot, link and recovery campaigns

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md),
[FULL_COVERAGE](FULL_COVERAGE.md) and [VERIFICATION](VERIFICATION.md). Required
inputs are actual implemented part-specific paths, linked firmware,
runner/fixture suites from prompts 48–52 and an explicitly available authorized
fixture. Preserve prompt 71's actual CI-tested SHA separately from subsequent
local or firmware revisions.

## Current evidence and ownership

Reopen `../TCA9548A/tools/test_hil_artifacts.py::ArtifactTests` and its actual
runner, plus
`../MB85RC/tools/hil_runner.py::payload_failure_reason/strict_failure_reasons`
and tests. Inspect current CLI/firmware identity, Python runner, registration,
AGENTS and exact board/netlist/SDK sources. Record revisions, dirty state, paths
and adopted/adapted/rejected evidence/restoration rules; memory scratch
procedures are not PHY test requirements.

The lead owns fixture admission and interpretation.

## Concrete public-source inputs

Use [recorded timing/identity conflicts](../reference/library-evidence.md) and
[EVIDENCE](EVIDENCE.md) to define observations. Capture actual raw IDs,
straps/mode, reset readback and MDIO readiness against exact part/revision
requirements. Resolve source conflicts with identified evidence, never silently
change an expected ID or delay until the fixture passes. Typical FLP timing is
not a guaranteed link-up deadline; source/host/SDK and physical results remain
distinct even when the same runner records them.

## Readiness and physical cases

No board is expected for a long time. Complete case inventory/readiness, mark
this stage BLOCKED (software/readiness done, physical NOT RUN), and continue
74-80; absence of hardware does not excuse host or SDK layers. Recheck current
netlist/pins/SDK when fixtures arrive. Define Q5 hard-reset first-valid-read and
strap relatch; Q10 next-page/re-AN; Q11 forced-mode raw PHYSTS, robust/manual
MDI and port-swap; Q13 P4 fenced stop, repeated/never-started stop, STOP delivery,
stub mediator and RX teardown; Q14 FX BMSR/SD and explicit restart refresh;
Q15 DP83825 identity/latch/Leader clock/SecureON. Use the exact observations
and authorization boundaries in REMAINING_PROMPTS_AUDIT section 8; none is
pre-qualified by the readiness record.

## Work and acceptance

Extend the evidence record and fixture selection from 48-52. Before each case
record exact part/revision/interface/address, board/netlist, reset/strap/clock,
partner/cable, SDK, firmware SHA/dirty identity, host runner revision, initial
state, boot/request/nonce correlation, finite duration, observations, restoration
and pass limits. Recheck wiring before authorized flashing and independently
record peer settings. Extend the hardware-validation guide without a second
campaign schema or fixture manifest.

Prioritize DP83826IRHBR/P4 cold boot, each applicable reset/restart, owned
configuration restoration, 100/full-only advertisement and resolved-policy
rejection, unplug/replug, peer reset, repeated flaps, incompatible partner,
incomplete negotiation and bounded recovery. Exercise generic 10 Mbit and legal
duplex/mode differences on suitable configured peers separately from the first
board profile. Include controlled management-transport faults where feasible.
Distinguish PHY status/counters, raw MAC packet handoff and real IP traffic;
successful serial parsing is not packet evidence. Internet/DNS failure alone
does not establish PHY failure or broaden application scope. Include a finite
sustained ordinary-traffic case with actual duration, packet/error counts,
partner conditions and final restoration evidence.

Run only admitted exact-part suites. Each run has exactly `<run>.txt` and
`<run>.json` under the 48-52 contract: preserve failed/interrupted runs, verify
transcript hash/length and worst-case verdict, and reject stale/duplicate/
truncated output, identity mismatch, zero/skipped-only cases and persistence
failure. Keep primary/restoration evidence distinct and rerun only affected
authorized cases after defects are fixed.

Without fixture or authorization, finish useful readiness/case inventory, record
the precise dependency and next reproducible case, and leave this physical gate
BLOCKED/NOT RUN. Do not mark it COMPLETE or the entire set COMPLETE; continue
eligible software/docs work. Update Doxygen/tests with behavior fixes, README
qualification, CHANGELOG, hardware/fixture guides, progress and this
prompt's report, including no-change reasons. Run focused local checks. Check
final diff and commit/sync checked units on the execution branch; do not
dispatch/poll/wait for remote CI here. Timing/features belong to
[73](73-hw-feature-campaign.md).
