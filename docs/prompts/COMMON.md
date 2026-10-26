# Contract for every implementation prompt

This is an implementation plan, not implemented software. Read this file, the
selected numbered prompt, root `AGENTS.md`, `../AGENTS.md`, and the shared
contracts linked from [the index](README.md) before each stage. A request to
write or audit this promptset does not start its execution.

[FULL_COVERAGE](FULL_COVERAGE.md) is mandatory: every documented programmable
feature belongs to the final library, including runtime opt-in features unused
by the first board. Research relevant public primary sources throughout, keep
the register/field/procedure ledger complete, and do not close gaps by relabeling
supported-but-unimplemented behavior as unsupported silicon or outside scope.

## Start every stage with current evidence

1. Inspect this checkout's branch, revision, dirty/staged state, applicable agent
   rules, existing workflows, registered checks and predecessor outputs. Preserve
   concurrent work. Continue an existing execution; do not restart completed work.
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
- Use straightforward C++17, fixed-size values, explicit errors and bounded work.
  Avoid generic service frameworks, registries, hidden threads/retries, dependency
  injection frameworks, speculative backends and unnecessary parallel API surfaces.
- The lead may delegate disjoint research, tests or implementation after shared
  contracts are settled. Assign explicit paths; shared headers, architecture,
  progress and Git have one writer. Reviewers are read-only. Select available
  agents for the actual work without hard-coded model/version requirements.
  Integrate and review every agent change; delegation does not waive validation.
- Inspect the current Git setup rather than importing another project's workflow.
  Preserve the current working branch unless the task calls for another. Never
  stash/discard unrelated changes, rewrite shared history, or stage others' work.
  The lead alone performs authorized Git mutations. This promptset does not
  import siblings' blanket commit/push, merge, publication or CI exceptions.

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

Mark a stage COMPLETE only when its stated outputs and required checks exist.
Code-complete does not mean hardware-qualified. A missing fixture or tool may
leave a required gate BLOCKED; continue independent work whose prerequisites
are satisfied and return to that gate. Never silently convert an unverified
feature into supported behavior or call the entire set complete with open gates.

Apply the parent CI policy when implementation/integration begins: inspect and
reuse workflows, run focused local checks, and verify the exact final commit for
major changes through applicable remote CI once available. Report unavailable
remote access or tooling honestly. Do not borrow TUSB32x's local one-CI-stage
exception. Small documentation-only changes need relevant local checks.

Finish each invocation with changed behavior, actual verification, remaining
limits and the next eligible stage. Do not start another stage when the user
requested only one. Physical fixture operation, sibling edits and publication
need their own task scope; a planning request does not authorize them.
