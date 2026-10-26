# 26 - Implement cable autorun and shared result acquisition

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

Prerequisites: [25](25-cable-manual.md), [20](20-event-counter-acquisition.md), [4](4-silicon-diagnostic-ledger.md); only the concrete inputs used below are required.

## Scope, current evidence and work

Deliver explicitly configured silicon cable autorun on link down, exact
readback/status/result behavior and interaction with manual capture. Reuse task
25's verified controls, decoding, collector and restoration; no autonomous
software recovery loop or duplicate diagnostic acquisition path.

Reopen TMP1x2 operation result-retention/public lifetime contracts and fixture
runner/parser cases; compare PCA9555 cancel/cleanup and deferred observation
tests. Inspect current registrations and examples. Reopen all SNLS647I hardware
autorun, link-down trigger, cable status and shadow/result descriptions. Do not
infer generations or timing guarantees the device does not expose.

Define typed autorun policy configuration with every source-supported setting,
supported mode/peer constraints and explicit ownership. Validate pure inputs
before I/O; acquire any required live precondition through bounded reads before
mutation. Apply once, verify stable owned controls and preserve desired/observed/trusted
state through partial errors. Enabled autorun is an explicit opt-in persistent
setting; initialization/recovery may replay it only when the application
requested and owns it. Extend reset/power restoration as applicable.

Keep successful policy configuration separate from a physical trigger, active
capture, completed valid evidence and consumed history. Reuse the collector for
manual sessions, IRQ/poll status and autorun results. Specify arbitration while
a manual session is active and what happens when another hardware event changes
results during acquisition. A link-down observation alone cannot prove a fresh
completed cable measurement. Where freshness/generation is not observable,
publish that limit and retain raw evidence without declaring a new PASS.

Bound acquisition and any manual-session handoff. Never drain until stable or
poll until the environment triggers. Stop/reconfigure with explicit maintenance
admission where disruptive; preserve accepted settings and partial samples on
failure. Cancellation and cleanup keep their separate finite budget, retained
effects and status rules. Hardware autorun does not authorize software reset,
link recovery or application scheduling policy.

Test literal enable/disable/readback sequences, legal settings and invalid
zero-I/O rejection. Simulate link triggers, no trigger, old results, changing
results between reads, manual-session contention and simultaneous status
consumption. Fail every mutation/acquisition/restore phase, including a
destructive read-after-effect timeout. Assert stale/unknown evidence cannot
become a measured length, exact transfer limits, no background MDIO and
cached views without new reads.

Extend the diagnostics guide with manual-versus-autorun ownership, persistence,
freshness limits and fixture scenarios. Compile an owner example that explicitly
configures policy and later acquires evidence. Physical trigger timing and
distance accuracy remain separate from host protocol tests.

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
