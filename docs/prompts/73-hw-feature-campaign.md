# 73 - Execute admitted feature, medium and timing qualification

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md), [FULL_COVERAGE](FULL_COVERAGE.md)
and [VERIFICATION](VERIFICATION.md). Inputs are implemented feature paths, explicit
runner suites/fixtures, linked firmware and the applicable fixture admission
record from [72](72-hw-link-campaign.md) or an equivalent freshly verified record.
An unrelated missing fixture does not block an available independently authorized
case. Required physical evidence remains open until actually obtained.

## Current evidence and ownership

Reopen TCA9548A `tools/test_hil_artifacts.py` and actual runner behavior, MB85RC
runner strict-failure/restoration tests, plus current PHY CLI/firmware and Python
suite admission. Inspect AGENTS, relevant registration, exact part/board/netlist
and instrument setup. Record revisions, dirty state, paths and adopted/adapted/
rejected conventions. Read exact TI procedure restrictions and equipment limits,
not only a previous passing campaign report.

The lead owns fixture safety/admission and physical interpretation. A useful
delegation is read-only trace/artifact review or a disjoint host failure regression
with explicit paths, acceptance and an available model suited to complexity.
Review all contributions; agents do not mutate Git or operate fixtures without
separately authorized scope. No automatic flashing, servo motion or incidental
loading-machine operation follows from this prompt.

## Work and acceptance

Run applicable BIST/generator/checker/loopback, TDR/cable, interrupt, LED/GPIO,
EEE/LPI, WoL/filtering and PHY power/wake campaigns on explicitly named exact
parts/revisions/interfaces/media. Define finite duration, initial/owned pre-state,
partner/equipment configuration, correlated request identity, required observations
and restoration verdict before execution. Test supported success, no-link and
inconclusive diagnostic outcomes without converting an inconclusive TDR result
into a cable length. An absent IRQ, optical or wake fixture is BLOCKED/NOT RUN,
never a pass inherited from another board.

For reset/strap/clock/RMII/RGMII timing claims, retain instrument configuration,
traces and measured limits. Packets passing or LEDs toggling do not prove signal
compliance, fixed delay accuracy, deterministic latency or EtherCAT cycle/servo
qualification. Fiber qualification needs actual suitable optics and a supported
MAC-side interface; do not imply P4 RGMII evidence. Temperature/package or another
suffix/revision does not inherit a pass. Electrical compliance needs its own
explicit setup and claimed limits. PHY low-power controls remain in scope;
board power delivery and PoE remain outside the library.

Retain actual firmware identity, SDK, part/mode/fixture data, bounded raw and
structured artifacts and manifest/hash in unique run directories. Preserve
failed retries, interruptions and skipped/missing cases. Reject stale/duplicate/
truncated output, identity mismatch and artifact failure. Record primary test
and restoration separately; failed restoration leaves the result failed/untrusted
even if the primary measurement passed. Fix confirmed defects with executing
host regressions, Doxygen/contracts and affected authorized physical reruns.

Missing equipment/permission leaves a precise physical gate with next fixture/
case, while readiness/host work and unrelated stages can finish. Reconcile exact
feature-matrix rows, README qualification, CHANGELOG, hardware/fixture guides,
report, validation and progress; record inspected no-change reasons. Run focused
local checks and check final diff. Commit/sync verified units and available work
on the recorded branch, keeping BLOCKED evidence visible. Preserve prompt 71's
historical CI SHA; no remote dispatch/poll/wait occurs here.
