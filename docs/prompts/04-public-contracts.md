# 04 - Specify API, Status, state and timing contracts

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-03.

## Fresh audit and owned work

Revisit real Status.h, Config.h, health/state and owner-operation declarations in
TMP1x2, INA228, MB85RC, PCA9555 and SHT3x. Identify meaningful naming matches and
documented differences.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Work

Trace every programmable feature in the complete source ledger to a public
typed operation or coherent configuration/field helper and observable result.
Account for all legal settings; a generic raw write is not a substitute for a
stateful command. Keep inactive optional features in the API plan even if the
initial board lacks the pin/interface needed to test them. Complete the design
without declaring unimplemented methods as usable production APIs.

Create `docs/api-contracts.md` with exact proposed declarations and examples before
exposing them in shipped headers. Keep namespace/class/header names from ENGINEERING.
Freeze callback argument ownership, time units, transaction budget and errors. Adopt the
shared Status shape with MDIO-specific errors; preserve backend detail and static
message lifetime. Define invalid enum/status normalization without I2C NACK assumptions.

Define Config/device identity/capabilities, advertisement/forced mode, LinkSnapshot,
HealthSnapshot and configuration trust. Separate bound/initialized, transport health,
link/negotiation and application policy. Name cached vs live methods clearly. All
snapshots carry validity/age semantics; don't use a default numeric speed as valid
evidence.

Specify minimal caller-driven operations needed for reset, configuration, negotiation
waits and diagnostics. Admission, progress, terminal result, consumption, cancellation,
deadlines and busy behavior must be unambiguous. Use retained results/tokens when needed
to prevent stale completion; avoid a generic job framework. Synchronous conveniences
share the same verified procedures. Logical operation budgets must preserve atomic MDIO
batches.

State time domain, overflow/wrap intervals, completion-time handling, zero-budget
behavior, timeout propagation and inability to preempt a blocking backend. Define
configuration invalidation after raw/external changes, cleanup error precedence and
no-I/O bind/end/accessors. Cancellation itself is bus-silent: specify whether later
owner polling performs cleanup, how the terminal result records
pending/restored/uncertain effects, and a reserved or separately admitted finite cleanup
budget that cannot silently extend the primary deadline. Teardown must retain or return
unresolved hardware-effect evidence. Model hardware reset and power-down separately from
local teardown.

Document every planned public operation's inputs, valid states, transaction bound, side
effects, output-on-error and unsupported behavior. Keep chip features unresolved at
stage 02 gated. No production declarations for methods that will only become functional
later.

## Acceptance and handoff

Review scenarios: unplugged cable, dead MDIO, reset in flight, wrong variant, unexpected
writer, partial apply, diagnostic cancellation and stale result. Ensure each has one
Status/state/effect interpretation. Cross-check API examples against the chosen IDF
boundary and peer terminology.

Expected output: Concrete public/state/time specification without successful stubs.
Include source documentation, relevant guides, feature/evidence matrix and progress
updates. Apply COMMON's completion rules; report actual commands, results and remaining
gates. Continue only when the concrete required inputs exist; COMMON explains how
unrelated gates stay open.

Before reporting completion or starting another prompt, commit and sync this
prompt's checked work on the recorded execution branch. Also checkpoint coherent
verified units during the prompt. Follow root AGENTS' first-set CI exception;
do not create empty commits or include unrelated work.
