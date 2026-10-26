# 19 - Expose implemented PHY behavior through the examples

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-18.

Before the audit, apply COMMON's short rule reminder: current sibling sources,
selected conventions, owned paths, source/general documentation, focused local
checks and same-branch commit/sync. Choose subagent models by task complexity.

## Fresh audit and owned work

Re-audit INA228/TCA9548A command/result semantics, TMP1x2 diagnostics and MB85RC
restoration reporting, including corresponding Python expectations.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Work

Use the complete chip feature/field/procedure ledger as the coverage denominator,
not only currently exported methods. Every required programmable feature needs
discoverable grouped commands, all legal settings, effect/status reporting and
tests. Track implementation gaps explicitly until they are filled. Raw register
commands do not replace a usable multistep feature command.

Extend the shared CLI to every implemented primary feature and capability-gated
operation: identity, safe discovery, desired/observed configuration,
apply/verify/invalidate, link/partner observation, owner progress/result/cancel, safe
registers, reset/recovery, diagnostics and optional controls. Match CLI_CONTRACT and the
frozen grammar; unsupported commands return explicit capability errors before I/O.

Distinguish read-only selftest from link-disrupting functional tests. Safe dumps avoid
read-clear/side-effect registers unless explicitly requested through the collector. Raw
writes invalidate appropriate cached evidence. Mutating commands require application
maintenance admission; the EtherCAT owner example rejects them while active. No hidden
init/reset on startup.

Finite watch/stress uses the existing owner path with bounded count/duration/output and
stop handling. Track operation result separately from runner test verdict, and primary
error separately from restoration. Transfer assertions must use the correct
logical-operation vs frame counter; an MMD access is not one frame.

Attach the same processor to both native P4 examples and the host fixture. Keep board
transport/console settings outside it; display enough context to reproduce the setup. If
an additional framework is later implemented it uses this processor too, not a second
grammar. Update coverage mapping between API, command, help, runner case and test.

## Concrete dispatch and coverage closure

Apply [CONVENTIONS](CONVENTIONS.md). Re-audit
`TMP1x2/examples/common/Tmp1x2CliDiagnostics.cpp` and
`Tmp1x2CliOutput.cpp` for `startDiagnostic`, `tickDiagnostic`,
`finishDiagnostic` and retained output; inspect actual current definitions.
Compare MB85RC's `tools/hil_runner.py::classify` and associated
restoration tests rather than accepting a human string containing OK as a
completed test. Keep existing stage 18 processor/parser/record code as the one
implementation used by all examples.

Create a reviewable mapping in the maintained feature ledger with columns for
exact variant/mode, public operation/field, legal input, CLI/help entry, owner
route, output record, host case and physical requirement. This includes controls
unused by the P4 board. A blank public operation or CLI cell is required work,
not an unsupported-chip claim. Capability gating reports absent silicon support,
unimplemented software and unavailable board/backend separately.

Implement related commands as direct handler groups for identity/configuration,
link/events, register access, lifecycle and optional diagnostic/control families.
Use stage 04 names and stage 18 grammar; do not invent alternate setters for CLI
convenience. Each handler validates every token, range and cross-field condition
before invoking the owner. Use `Status::ok()`/`inProgress()` deliberately:
acceptance is not successful completion, and completed cancellation is not proof
that restoration succeeded.

Specific execution cases must include:

- Discovery visits only requested legal PHY addresses with a finite total budget,
  identifies evidence explicitly and never binds/applies a profile automatically.
- Safe dump skips side-effect registers; explicit event collection publishes
  cached evidence consumed by subsequent passive views without a second read.
- Valid raw writes use the admitted transaction path and invalidate only the
  evidence they can affect. Reserved/unknown-address writes, malformed values
  and unsupported feature settings perform no I/O.
- Reset/restart/reinitialize remain different actions in grammar/help/results;
  link watch and stress end on count, duration, stop or fault and retain the final
  operation/restoration result. Zero rounds is rejected, not a passing run.
- Loopback/BIST/TDR and opt-in IRQ/LED/clock/power/wake/EEE controls exercise every
  documented legal setting through typed APIs. An active application session,
  busy owner or retained unconsumed result rejects conflicting work before I/O.
- A terminal failure plus cleanup success remains a failed command/test; cleanup
  failure is additionally visible. Frame counts cover all constituent MMD frames
  and any collector/base traffic, separately from managed health attempts.

Use a compiled transcript test for each handler group and boundary input, with
literal independent expected values for representative register procedures.
Exercise a stalled console while the owner advances deadlines and stops accepting
new work; avoid starving mandatory cleanup to print a watch line. Build both
native example applications with the same source list and protocol schema.

Update command Doxygen, `docs/cli.md`, per-example READMEs, root README command
overview and CHANGELOG in this stage. Include legal values/units, maintenance
admission, destructive-read notes and example success/failure records. Run the
established documentation checks and update grammar/help/runner guards. Delegate
handler groups or transcript cases only after grammar/contracts are frozen;
give each agent disjoint writable files and let the lead own shared dispatch.
Choose review depth to match register side effects and concurrency complexity.

## Acceptance and handoff

Run executable CLI transcripts for successful, rejected, busy, timeout, unsupported,
cancelled and cleanup-failure cases. Compare help with real dispatch and host fixture
output. Build both actual IDF examples with the shared CLI linked. Test stalled console
fairness and live MDIO serialization; don't accept a source-text guard alone.

Expected output: Complete primary command coverage and shared CLI in real examples.
Include source documentation, relevant guides, feature/evidence matrix and progress
updates. Apply COMMON's completion rules; report actual commands, results and remaining
gates. Continue only when the concrete required inputs exist; COMMON explains how
unrelated gates stay open.

Before reporting completion or starting another prompt, commit and sync this
prompt's checked work on the recorded execution branch. Also checkpoint coherent
verified units during the prompt. Follow root AGENTS' first-set CI exception;
do not create empty commits or include unrelated work.
