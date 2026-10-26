# 04 - Specify API, Status, state and timing contracts

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-03.

Read [CONVENTIONS](CONVENTIONS.md) and make a short stage-rule reminder. This
stage owns the exact API decision; later agents do not invent competing names.
Use independent review with an available model suited to state/ownership work.
Keep one execution branch, per-stage source documentation and commit/sync.

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
initial board lacks the pin/interface needed to test them. Complete the feature
mapping and foundational contracts; keep unresolved feature signatures explicit
until their source-backed stage. Do not declare future methods as usable APIs.

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

## Freeze the concrete vocabulary

Keep short register reads, validated writes and pure helpers as direct bounded
calls. Use cooperative start/poll/result/cancel only for procedures that need
scheduled waits or retained cleanup. One active operation slot with a monotonic
nonzero token and explicit exhaustion is enough; no job registry, generic
scheduler or token framework. Both paths share the same register procedures.
Freeze the foundational signatures now; add feature-specific types/methods with
their source-backed implementation and update this contract together.

Use CONVENTIONS' selected `Status` fields/helpers and `errorName`, four-state
passive `DriverState`, `driverStateName`, `HealthSnapshot` and validity fields.
Contrast TMP1x2 four-state telemetry, PCA9555's three states and INA228's selectable
health admission. Record the deliberate choice: this PHY's health is passive,
`OFFLINE` does not suppress I/O, and link/configuration evidence are independent.
Do not silently import a sibling's recovery latch or raw-probe success rule.

Freeze `bind`, `end`, `isBound`, `isInitialized`, `driverState`, `healthSnapshot`,
`linkSnapshot`, `getSettingsSnapshot`, `getOperationSnapshot`, `readLinkStatus`,
`readClause22`/`writeClause22`, `readMmd`/`writeMmd`, `invalidateDeviceState`,
`startInitialize`/`startReset`/`startConfigure`/`startRecover`, `pollOperation`,
`takeResult` and `cancel` as their responsibilities become applicable. Describe
why `pollOperation` is selected from PCA-style explicit naming while retained
results follow TMP's `takeResult`. Do not expose `pollJob`, `state`, `init` or
`begin` aliases without a real compatibility requirement.

Specify the narrow set of `Err` values and code-specific `detail` meaning from
CONVENTIONS. Map every fallible entry point to a state/error table. At minimum:

| Scenario | Required distinction |
| --- | --- |
| Cable absent but MDIO works | Valid link-down evidence; not MDIO failure or automatic recovery |
| Invalid PHY/register/enum/config combination | Validation error before any callback; outputs and health unchanged |
| Callback returns `IN_PROGRESS` | Contract violation normalized to terminal MDIO failure, preserving diagnostic evidence |
| Valid read identifies the wrong part or an unaccepted revision | `DEVICE_MISMATCH` with explicit reason and observed identity; not absent hardware or transport failure |
| No accepted identity after successful reads | `DEVICE_NOT_FOUND` under documented plausibility rules; do not claim this proves physical absence |
| Controller timeout vs operation deadline | `MDIO_TIMEOUT` vs `OPERATION_TIMEOUT`; no ambiguous timeout alias |
| Active work or unconsumed result | `BUSY` with a defined reason; retained result is not overwritten |
| Wrong result token | `TOKEN_MISMATCH`; result/output unchanged |
| No result | `RESULT_NOT_AVAILABLE`; no fabricated success/default sample |
| Stable owned bits disagree after apply | `CONFIG_MISMATCH` and untrusted configuration, even when transport succeeded |
| Write failed or completed too late | Possible hardware effect remains visible; no automatic retry or success publication |
| Part lacks a documented feature | `UNSUPPORTED` before transfer, with exact capability evidence |

Define `OperationToken` allocation, invalid sentinel, wrap/exhaustion and result
retention across cancellation/end/rebind. Do not reuse a token while a stale
result can be mistaken for the new operation. Prefer fixed-size operation/result
values and an explicit operation-kind switch. Do not build a job registry or
generic asynchronous framework. Specify primary status, cleanup status and
hardware-effect/restoration evidence without collapsing secondary failure into
success. Every field needs a consumer and validity rule.
Operation admission is not ownership of the controller lock across polls. Public
aggregate output can remain unchanged on error while the single event collector
retains already consumed read-clear samples and flags possible lost history.

Set health accounting units explicitly for the chosen transport. Define the
`UNINIT`/initializing boundary, failure threshold, saturating counts, last-error
retention, success recovery and what `resetStatistics` does not clear. Separate
managed transactions, physical frame diagnostics and silicon RX/error counters.
Document zero timestamps, clock wrap, freshness and observation invalidation.

Include a plain C++ owner example and IDF callback mapping that use the exact
proposed signatures, including an error path and a retained result. Keep them
as design examples until their operations are implemented and compiled.

## Documentation in this stage

Write planned Doxygen blocks beside proposed declarations in `docs/api-contracts.md`:
parameters and units, borrowed lifetime, valid states, output-on-error, side
effects, timeout/cleanup bounds and exact return meanings. The implementation
must carry those comments into real headers as behavior arrives. Review README,
CHANGELOG, architecture, feature coverage and example plans for matching names.
Stage 32 must not be the first time these contracts receive source documentation.

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
