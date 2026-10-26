# 33 - Implement clock, interface and repeater controls

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

Prerequisites: [32](32-led-gpio.md), [18](18-forced-crossover-settings.md), [5](5-silicon-power-interface-ledger.md); only the concrete inputs used below are required.

## Scope, current evidence and work

Deliver every documented DP83826I clock/interface and RMII repeater option,
with exact mode/reset constraints, shared pin validation and restoration. Extend
the existing configuration surface and procedures; the portable library never
owns oscillator wiring, MAC initialization or board pin assignments.

Reopen TMP1x2 configuration/dependency validators, PCA9555 complete-profile/
shadow tests and relevant native sibling example board-adapter boundaries.
Inspect actual source/registration and FieldCore owner adapters as read-only
examples of application responsibility. Recheck SNLS647I clock, MII/RMII,
repeater and compatibility tables and reset/strap descriptions.

Build a legal matrix covering available interfaces, reference-clock direction/
selection/output, interface timing/compatibility and repeater settings with
their exact dependencies. Distinguish hardware straps, POR-only properties,
software-programmable fields and observed mode. A software API cannot change
a hardware-only property merely because a related part offers such a register.
Additional family interfaces are qualified later, not inherited now.

Validate the whole requested profile and pin users before any transfer. Clock/
interface changes may break the active MAC or MDIO access: explicitly identify
permitted runtime changes, required stopped-driver/maintenance state and
application clock/pin sequencing. Reject unsupported live transitions before
writes without declaring the silicon feature absent. No automatic clock source
switch, reset GPIO control or hidden board policy.

Implement source-defined ordered application/readback, stable-owned masks and
command-neutral composition with aggregate deadlines. Keep desired settings,
observed mode and verified configuration distinct. Extend every applicable reset/
power recovery path using the same restoration ledger and currently implemented
pin/IRQ/WoL ownership rules. Preserve hardware effects if an accepted transition
times out or readback becomes unavailable; no guessed rollback/retry.

Use bounded direct operations for simple fields and the existing cooperative
engine only for real waits. Complete transactions remain atomic and the bus lock
does not span application scheduling. Cancellation or local end cannot certify
restored clocks or stopped hardware; failed/denied cleanup retains uncertainty
and separate primary/cleanup status.

Test independent exact encodings and legal combinations for all documented
options, invalid/reserved values, mode mismatches, pin conflicts and unsafe
live-transition admission with zero I/O. Inject every write/readback/restore
failure, including loss of access after an accepted clock-related write.
Check reset-loaded versus runtime retention and exact operation bounds.

Compile owner examples showing the required application sequencing and a
rejected live transition. Document electrical timing/clock-source assumptions
and physical measurement gates. A host register test cannot prove a working MAC
clock, repeater link or compliant board timing.

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
