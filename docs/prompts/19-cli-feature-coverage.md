# 19 - Expose implemented PHY behavior through the examples

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-18.

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
