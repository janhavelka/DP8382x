# Contract for every implementation prompt

This is an implementation plan, not implemented software. Read this file, the
selected numbered prompt, root `AGENTS.md`, `../AGENTS.md`, and the shared
contracts linked from [the index](README.md) before each stage. A request to
write or audit this promptset does not start its execution.
Root `AGENTS.md` owns the user-authorized commit/sync workflow and the first
promptset's remote-CI exception; those override earlier unqualified parent-policy
references in historical planning reports.
Read [CONVENTIONS](CONVENTIONS.md) for concrete names, source paths and the chosen
PHY adaptations. Keep simple engineering language in code, docs and handoffs.

[FULL_COVERAGE](FULL_COVERAGE.md) is mandatory: every documented programmable
feature belongs to the final library, including runtime opt-in features unused
by the first board. Research relevant public primary sources throughout, keep
the register/field/procedure ledger complete, and do not close gaps by relabeling
supported-but-unimplemented behavior as unsupported silicon or outside scope.

## Start every stage with current evidence

1. Inspect this checkout's branch, revision, dirty/staged state, applicable agent
   rules, existing workflows, registered checks and predecessor outputs. Preserve
   concurrent work. Continue an existing execution; do not restart completed work.
   Fetch/compare upstream before editing. For every new promptset execution,
   create a new execution branch from the synchronized intended base and record
   branch/base before its first stage. Do not use main or a prior set's branch.
   Keep it for the entire set and every resumed session; resuming the same set
   never creates another branch. No per-prompt or per-agent branches.
2. **Always re-audit the relevant sibling I2C libraries before implementing a
   stage.** Use [SOURCE_MAP](SOURCE_MAP.md) and that stage's focused sources.
   Read current public declarations and actual implementation, examples/CLI,
   tests, Python runners, build/workflow registration and `AGENTS.md` as relevant.
   Record repository revisions, dirty state, exact paths, and what is adopted,
   adapted or rejected. Previous reports and sibling README claims are leads,
   not a substitute for inspecting current behavior. If a reference is unavailable,
   name that limit and use available evidence without inventing a comparison.
3. Re-open the exact TI document sections for behavior being implemented. Inspect
   original PDF tables when extraction is ambiguous. Check current official
   SDK documentation/source when choosing a supported adapter baseline; record
   immutable versions. Do not replace archived vendor bytes silently.
4. Confirm the stage's prerequisites and list owned files, shared contracts and
   consumers affected by a change. Resolve routine choices from evidence.
   Correct conflicting plans explicitly before coding; update all affected
   prompts/contracts rather than adding a second interpretation or alias.
5. Write a short entry reminder in the work plan/report: applicable root rules,
   selected convention names and current source evidence, owned paths, one MDIO
   owner, documentation/checks, execution branch and commit/sync checkpoint.
   Recheck it against the final diff. Keep this brief and specific to the stage.

Prerequisite ranges refer to the **software outputs actually needed** by the
selected work, not a requirement to close every unrelated evidence gate first.
Record those concrete inputs before proceeding. An unavailable cable diagnostic,
variant, hardware fixture or remote CI result does not block unrelated adapter,
CLI, package, documentation or review work. It remains an explicit open gate;
dependent code must reject the unavailable capability. A source-proved absent or
inapplicable feature can have a completed disposition. Missing essential evidence
is not that disposition and cannot become implemented/supported by declaration.
Leave affected required work incomplete until resolved. Stage 34 can review a
partial result but cannot mark the whole set COMPLETE with required gates open.

## Work and ownership

- Public portable headers live in `include/DP8382x/`, implementations in `src/`.
  Keep platform-only subdirectories excluded from the portable build. Add files
  for real responsibilities, not to populate an aspirational directory tree.
- Siblings and FieldCore-node are read-only references. No dependency on their
  sources, contracts, include paths, services, tasks or build trees is permitted.
- Keep board wiring, MAC/frame ownership, scheduling, locks, link policy,
  recovery decisions, EtherCAT, IP networking and power delivery outside core.
- Deliver each behavior with source documentation, failure-path tests and relevant
  user guidance in the same stage. Later audit stages do not defer this work.
  Do not expose successful stubs, unimplemented factories or placeholder APIs.
- Follow CONVENTIONS' source documentation contract: public Doxygen with actual
  parameters, units, ownership, errors, output validity and side effects; source
  comments explain non-obvious procedures. Review README/CHANGELOG and affected
  architecture/API/example/test/tool guides every stage, updating what changed
  and recording a reason for no change elsewhere. Run affected Doxygen/link checks.
- Use straightforward C++17, fixed-size values, explicit errors and bounded work.
  Avoid generic service frameworks, registries, hidden threads/retries, dependency
  injection frameworks, speculative backends and unnecessary parallel API surfaces.
- The lead may delegate disjoint research, tests or implementation after shared
  contracts are settled. Assign explicit paths; shared headers, architecture,
  progress and Git have one writer. Reviewers are read-only. Select available
  models and reasoning effort for task complexity: lightweight for bounded
  inventory/mechanical work, capable coding for scoped implementation/docs/tests,
  stronger reasoning for state/ownership/silicon review. Record chosen model and
  rationale with the task's scope. Use the inherited model when selection is not
  available; never require a particular provider/version or relax acceptance.
  Integrate and review every agent change; delegation does not waive validation.
- Follow root AGENTS' single execution branch and authorized checkpoint workflow.
  Commit and sync every completed prompt and coherent verified units during a
  prompt. The lead alone owns Git mutations. Never stash/discard unrelated work,
  stage others' changes, rewrite shared history or force-push. Sync includes
  fetch/integration/push and verified upstream equality, not just a local commit.
  Missing access is an explicit unsynced state; continue independent useful work.

## Completion and handoff

Run focused checks from [VERIFICATION](VERIFICATION.md) and the current repository
commands, inspect the full diff, and fix confirmed defects without weakening
tests. Keep implementation, host tests, linked firmware, package consumers,
remote CI and physical qualification as separate evidence.

Update [implementation progress](../implementation-progress.md) and the stage
report `docs/reports/implementation/NN-topic.md` when executing a stage. Record:

- objective, prerequisite evidence, revision/dirty state and files owned;
- fresh sibling/source audit, adopted conventions and justified differences;
- decisions, changed API/behavior, affected consumers and migration if any;
- commands, results, test counts and actual build/framework/board identities;
- PASS, FAIL, NOT RUN or BLOCKED for each required evidence layer;
- unresolved issues with exact dependency, next step and qualification limit.
- the entry-rule reminder, model/delegation choices, Doxygen/general-doc changes
  or relevant no-change reasons, and maintainability findings resolved;
- execution branch/base, completed commit IDs and actual sync result; record
  completed IDs in the next update rather than a self-referencing commit loop.

Mark a stage COMPLETE only when its stated outputs and required checks exist.
Code-complete does not mean hardware-qualified. A missing fixture or tool may
leave a required gate BLOCKED; continue independent work whose prerequisites
are satisfied and return to that gate. Never silently convert an unverified
feature into supported behavior or call the entire set complete with open gates.

For this first implementation promptset, **remote CI belongs to prompt 29 only**.
Prompts 01-28 and 30-34 do not dispatch, poll, wait for or require remote results,
including after substantive changes or commit/sync. Inspect checked-in workflows
when relevant and keep all focused local checks, SDK builds and package tests.
Prompt 29 checks its actual completed commit; later work retains that SHA and
uses local evidence without claiming the old run validates a newer commit.
Preserve automatic triggers, hooks and required merge/release checks. After the
first execution, parent CI policy applies unless the user changes it.

Before reporting a prompt complete or starting the next one, commit and sync the
checked work, including docs/tests/progress, on the same execution branch. Also
checkpoint coherent verified units during a prompt. Do not create empty commits
for no-change reviews or include broken/unrelated work. Routine commits and
pushes are authorized; report exact sync failures rather than claiming success.

Finish each invocation with changed behavior, actual verification, remaining
limits and the next eligible stage. Do not start another stage when the user
requested only one. Physical fixture operation, sibling edits and publication
need their own task scope; a planning request does not authorize them.
