# 30 - Execute the available physical qualification matrix

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-29 software/fixture outputs for the tested part; exact
hardware and authorized operations must be available.

At entry apply CONVENTIONS' names, Status/health, layout and simplicity rules;
reopen current relevant sibling sources. Keep source Doxygen and affected
README/CHANGELOG/guides current, verify locally and checkpoint/sync on this
execution's recorded branch.

## Fresh audit and owned work

Recheck peer HIL evidence classification, runner/firmware identity and cleanup handling.
Re-open the exact board/netlist, part/revision documents and current SDK build identity.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Work

Run stage 21's campaigns on explicitly identified hardware when it is available and
within the task scope. Establish exact PHY part/revision/interface/address,
board/netlist, reset/straps/clock, link peer/cable, firmware SHA/dirty state and SDK.
Verify identity from firmware rather than the host checkout.

Prioritize DP83826IRHBR with P4: boot/reset/restart, 100/full-only autonegotiation
policy, resolved-policy rejection, disconnect/reconnect, peer reset, sustained ordinary
traffic, bounded recovery and diagnostic restoration. Test generic 10 Mbit behavior
using a suitable configuration/partner, not an implicit drive-port fallback. Cover legal
duplex/mode differences with real peer configuration evidence.

Use instrumentation for timing/clock/strap claims and exact fixtures for
BIST/TDR/IRQ/LED/wake/power cases. Ordinary PHY low-power operations do not include
board PoE/power delivery. No incidental servo motion or loading-machine control.
EtherCAT cycle/servo qualification belongs to the later application, not this stage's
PHY pass.

Run only supported variant suites on actual parts and keep every family's physical row
separate. Preserve failures, interrupted runs, missing fixtures and restored/untrusted
state. Fix confirmed library/runner defects with regression tests and rerun affected
cases.

If no hardware is attached, complete readiness documentation and case inventory, record
this stage BLOCKED/NOT RUN and continue independent documentation/review work. Do not
invent a physical result or mark the entire promptset complete.

## Concrete fixture admission and evidence

Reopen `../TCA9548A/tools/test_hil_artifacts.py::ArtifactTests` (dry run remains
NOT RUN, distinct output directories, actual firmware response, artifact failures),
the corresponding `tools/tca9548a_hil.py` behavior and
`../MB85RC/tools/hil_runner.py::payload_failure_reason/strict_failure_reasons`
with tests. Adopt finite explicit evidence and restoration requirements, not
memory scratch-window procedures. Inspect current CLI/firmware identity, Python
runner, tests, AGENTS and relevant registration; record revisions/dirty state.
Use CONVENTIONS' Status/health language and separate link/config/application truth.

The lead owns fixture admission and interpretation. Delegate read-only artifact
review or disjoint host regression fixes when useful; choose available models
for complexity and explicit writable/forbidden paths. No agent flashes, resets,
drives fixtures or operates a machine without that separately authorized scope.
The stage number and routine Git authorization do not authorize physical action.

Use `docs/hardware-validation.md` and the existing stage-21 manifest format,
without a parallel fixture schema. Record fixture ID, exact BOM/orderable
part/revision, board/netlist, medium/interface/address, reset/straps/clock
direction/source, partner/cable, instruments and allowed operations before
opening hardware. Record host runner revision separately from firmware-reported
SHA/dirty/SDK/target/part. A matching local checkout does not prove flashed identity.

For every case state initial conditions, command/request token, finite duration,
required observations, restoration checks and pass limits before running it.
Record negotiated and forced partner settings independently; a nominal "100/full"
label is insufficient. Include incomplete negotiation/no compatible partner,
disconnect/reconnect and flap behavior, application-policy mismatch, controlled
MDIO fault where feasible, reset/restart and recovery within defined deadlines.
Exercise generic 10 Mbit and supported duplex modes on suitable peers separately
from the first-board 100/full profile. Keep raw MAC packets, IP traffic and PHY
counters separate; successful serial command text is not traffic evidence.

Diagnostics must retain pre-state/owned configuration, primary result and
restoration outcome. Test supported BIST/TDR/loopback/IRQ/LED/wake/power features
with their required apparatus and exact source limits; do not infer a cable
length from an inconclusive diagnostic or report an absent IRQ fixture as PASS.
Clock/strap/reset timing claims need traces and instrument configuration. Packet
success is not analog compliance or EtherCAT cycle/servo qualification.

Preserve bounded raw transcript, structured results and artifact manifest/hash in
unique run directories; reference retained evidence from the report. Never
overwrite a failed run with a passing retry. Reject stale/duplicate/truncated
results, firmware identity mismatch, missing cases, skipped-only campaigns and
artifact persistence failure. A diagnostic whose primary test passes but cannot
restore remains failed/untrusted under the runner contract.

If a fixture or permission is absent, record exact missing dependency and next
reproducible case; leave physical evidence BLOCKED/NOT RUN. Complete useful
readiness/host work and continue eligible docs/review stages with the gate open.
Do not auto-flash or create successful hardware placeholders.

Update Doxygen/comments and regression tests with any discovered behavior fix;
update README qualification limits, CHANGELOG Unreleased for real changes,
hardware/fixture guides, feature matrix, validation/progress and
`docs/reports/implementation/30-hardware-qualification.md`. Run focused local
checks and rerun affected authorized physical cases after fixes. Retain stage
29's CI-tested SHA; do not dispatch, poll or wait for remote CI here.

## Acceptance and handoff

Retain bounded raw/structured artifacts, identity, per-case verdict and cleanup
evidence. Distinguish serial protocol, real packets, electrical measurements and
application timing. Review failures and remaining unqualified variants explicitly; a
skipped-only campaign cannot pass.

Expected output: Actual available hardware evidence or a precise unclosed fixture gate.
Include source documentation, relevant guides, feature/evidence matrix and progress
updates. Apply COMMON's completion rules; report actual commands, results and remaining
gates. Continue only when the concrete required inputs exist; COMMON explains how
unrelated gates stay open.

Before reporting completion or starting another prompt, commit and sync this
prompt's checked work on the recorded execution branch. Also checkpoint coherent
verified units during the prompt. Follow root AGENTS' first-set CI exception;
do not create empty commits or include unrelated work.
