# Execute an implementation prompt

Read root `AGENTS.md`, parent `../AGENTS.md`, this contract and the selected
numbered prompt. Use [CONVENTIONS](CONVENTIONS.md) for names and layout,
[ENGINEERING](ENGINEERING.md) for behavior and ownership, and
[VERIFICATION](VERIFICATION.md) for relevant checks. Consult
[EVIDENCE](EVIDENCE.md), [FULL_COVERAGE](FULL_COVERAGE.md),
[CLI_CONTRACT](CLI_CONTRACT.md) and [SOURCE_MAP](SOURCE_MAP.md) for the work
being changed. These shared requirements accompany every numbered prompt.

## Start

1. Inspect branch, upstream, staged/working changes, predecessor outputs and
   applicable agent rules. Fetch and compare the configured upstream; preserve
   concurrent work. At the start of a new promptset execution, create a new
   descriptive branch from the synchronized intended base and record branch/base
   in [implementation progress](../implementation-progress.md). Keep that branch
   through all prompts and resumed sessions. Resume an existing execution on its
   recorded branch. Editing the plan alone is repository maintenance.
2. Re-audit current relevant sibling I2C declarations, implementation, examples,
   CLI, tests, Python runners and agent guidance. Inspect workflow definitions
   initially and when affected. Record revision/dirty state, exact paths and the
   conventions adopted or adapted. Inspect live relevant source at each stage;
   reuse unchanged workflow findings with their recorded revision. Siblings and
   FieldCore-node are read-only references.
3. Verify changed chip facts against exact-part TI documents and source under
   EVIDENCE. Select and record an immutable SDK baseline for SDK decisions.
   Resolve discrepancies with source evidence and update affected contracts.
4. List the actual prerequisite outputs, owned files and acceptance checks.
   Give a brief rule reminder covering relevant names, ownership, documentation,
   verification, execution branch and checkpoint. Execute the requested scope.

Use numeric order, 1 through 80. A prerequisite means the concrete artifact or
behavior consumed by this task. Record an unresolved dependency against the
work it blocks. When a whole-set invocation permits further work, continue
independent eligible stages and return to the blocked task. A single-stage
invocation ends with that stage's handoff.

## Implement a complete work unit

Deliver validation, behavior, failure handling, timeout/cancellation, cleanup,
tests and documentation together. Expose APIs as their behavior becomes usable.
Keep one implementation of each chip procedure and extend existing collectors
and operation state. Use small explicit values and straightforward C++17.

Source research can complete with a fully documented conflict and next evidence
needed. The corresponding implementation gate remains open only where essential
behavior is unresolved. Exact-part evidence can support independent software
work while physical qualification remains pending. Feature stages provide
qualification cases and requirements; prompts 72-73 execute admitted fixtures.

Later regression/documentation reviews audit a finite matrix. Assign substantial
missing functionality back to its owning stage, update its status and plan the
remaining work there. Preserve requirements and dependency links when adapting
the plan to newly established workload.

Update public Doxygen and meaningful procedure comments with each code change.
Review README, CHANGELOG and affected API, architecture, example, test and tool
guides; update the files whose behavior or instructions changed. Summarize the
documentation review once in the handoff. Use plain engineering language and
keep current instructions authoritative in their maintained guides.

## Delegate and integrate

Use subagents for disjoint research, implementation/tests or independent review.
Give each a bounded outcome, writable/forbidden paths, frozen shared contracts
and acceptance cases. The lead owns shared headers, architecture, integration,
progress and Git; review agents are read-only. Use separate build directories.

Select from available models: lightweight for bounded inventory/mechanical work,
capable coding for implementation/tests/docs, stronger reasoning for silicon,
state, concurrency and deadline review. Record a short scope/model rationale.
Use the current model when selection is unavailable. Review and integrate every
contribution against the same acceptance requirements.

## Verify and checkpoint

Run focused local checks appropriate to changed behavior and inspect the final
diff for correctness, clear naming, duplication, hidden I/O and stale comments.
Keep host, SDK-link, package, CI and physical evidence separate. Use PASS, FAIL,
NOT RUN or BLOCKED with actual commands, counts, identities and limitations.

For the first execution, remote CI dispatch/status/waiting belongs to **prompt
71 only**. Other stages use local checks, including applicable real SDK/package
builds. Preserve automatic triggers, hooks and required merge/release checks.
Retain the prompt-71 tested SHA as evidence for that commit. Subsequent promptset
executions follow the parent CI policy unless the user changes it.

Commit and sync every completed prompt and each coherent verified intermediate
unit, including its tests and documentation. The lead stages only task changes,
reviews the staged diff, fetches/integrates safely, pushes the working branch and
verifies upstream equality. Preserve shared history and unrelated work. A
no-change review needs no empty commit. Report the exact failure and local SHA
when sync is blocked; retain the checked work and visible unsynced status.

## Record completion

Update implementation progress and `docs/reports/implementation/N-topic.md` with:

- outcome, prerequisite evidence and owned files;
- relevant source/sibling revisions, selected conventions and decisions;
- API/behavior changes, tests, Doxygen and affected guides;
- actual commands/results and distinct evidence-layer statuses;
- unresolved requirements, their affected scope and next evidence/action;
- delegation, execution branch/base and verified checkpoint/sync result.

Record completed commit IDs in the next progress update or final handoff so the
commit need not contain its own hash. Mark a stage COMPLETE when its stated
outputs and required checks exist. Keep unavailable fixtures and essential
source/tool dependencies explicit; the entire set is complete only when all
required gates close. Hardware operation, sibling edits and publication require
their own authorized task scope.
