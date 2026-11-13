# DP8382x engineering and agent guidance

## Scope and entry

This workspace contains reference material and an executing implementation plan.
Read README, `docs/architecture.md`, `docs/variants.md` and the reference index
before implementation. Follow parent `Projects/AGENTS.md`, with the first-set CI
exception below. Prompt maintenance leaves implementation status unchanged.

For each numbered stage use `docs/prompts/COMMON.md`, the selected prompt and
relevant shared contracts. Re-audit current relevant sibling I2C headers, source,
examples/CLI, tests, Python runners and agent rules. Record paths, revisions and
selected conventions. Siblings and FieldCore-node are read-only references.

## Git: work on main until version 0

Routine commits and synchronization are authorized for this repository's prompt
execution and complete planning/maintenance changes.

- Inspect status, staged changes, branch and upstream before edits. Fetch the
  configured remote and compare histories. Fast-forward only when safe and
  preserve local/concurrent work.
- Until version 0 is finished, all work happens directly on `main`: every stage,
  agent, resumed session and planning change. Create no new branch, including
  for a new promptset execution, unless the user asks for one. The earlier
  `promptset/dp8382x-implementation-20261028` branch was fast-forwarded into
  `main` on 2026-11-11; do not resume work on it. Resume recorded progress on
  `main`. Prompt maintenance does not start implementation.
- Commit and sync each completed prompt before reporting completion or starting
  the next. Checkpoint coherent verified intermediate units with their tests/docs
  as they become ready. A no-change review needs no empty commit.
- The lead alone switches branches, stages, integrates, commits and pushes.
  Stage only task changes after reviewing the diff. Preserve unrelated work and
  shared history; avoid stash/discard, force-push and history rewriting.
- Sync means fetch, safely integrate if needed, push to the configured upstream,
  fetch again and verify equal commit IDs with zero ahead/behind. Resolve normal
  merge conflicts within task scope and rerun affected checks.
- Retain checked local work if access/network/protection or an unsafe conflict
  blocks sync. Report exact failure, local SHA and remaining action; retry when
  possible. A commit or fetch alone is not a completed sync.
- Record checkpoint IDs in the final handoff or next progress update, avoiding
  a self-referencing commit loop. Use actual configured repository/owner data.
  Release publication, fixture operation and sibling edits need their own scope.

## Verification and CI

Current runnable checks are `python tools/verify_references.py`,
`python tools/verify_ledger.py`,
`python -m unittest discover -s test -p "test_*.py" -v` and `git diff --check`.
Ledger tests validate research records, not executing PHY behavior. Introduce
implementation commands with their working code.

Portable checks: configure/build with `cmake -S . -B build/host -G Ninja
-DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON`, then `cmake --build build/host`
and `ctest --test-dir build/host --output-on-failure --no-tests=error`.
Build `test/consumer` separately with tests OFF and run its registered CTest.
Also run `python tools/check_doxygen.py` and
`python scripts/generate_version.py check`.

ESP-IDF v6.1 (commit fff9895c) is installed side by side in `C:\esp\v6.1`. Run
`idf.py` only through `scripts\idf.cmd` (see `scripts/README.md`), with a short
build folder such as `C:\esp\build\<name>`. Never install ESP-IDF globally, run
Espressif's GUI installer or set `IDF_PATH`/`IDF_TOOLS_PATH` in Windows
settings: PlatformIO in `C:\pio` uses its own ESP-IDF 5.5 and has broken before.

For the **first implementation execution**, remote CI dispatch, run-status
checking and waiting belong to **prompt 71 only**. Prompts 1-70 and 72-80 use
focused local verification, including applicable SDK/package builds. This is the
user's exception to the parent's recurring exact-final-commit CI requirement.
Preserve automatic triggers, hooks and required merge/release checks. At 71,
verify the actual completed commit and record jobs/results/SHA; later stages
retain that historical SHA and report their own local evidence. Subsequent
executions follow parent CI policy unless the user changes it.

Inspect workflow definitions initially and when affected; reuse actual commands,
registered tests and pinned dependencies. Repeat checks for changed inputs,
failures or unresolved concerns. Keep assertions active in Release and register
real executing suites. Fix failures while preserving required behavior/tests.

Report host, portable/package consumer, SDK link, remote CI and physical evidence
separately as PASS/FAIL/NOT RUN/BLOCKED. Include actual environment, identity and
scope. A source review, fake backend or linked image proves its own layer.

## Engineering requirements

- Deliver a framework-independent C++17 core and native ESP-IDF adapter, starting
  with exact DP83826I. Qualify later variants individually. Implement the complete
  documented programmable feature set under `docs/prompts/FULL_COVERAGE.md`.
- Keep core public interfaces in `include/DP8382x/`, implementation in `src/`,
  SDK-only code in separate `esp_idf/` subdirectories, examples in `examples/`,
  tests in `test/`, and focused tools in `tools/`/`scripts/`. Create real behavior
  and useful contracts rather than empty scaffolding or successful hardware stubs.
- Use namespace/class `DP8382x`, PascalCase types, camelCase methods/fields,
  `_camelCase` private members and uppercase enum/constants. Follow CONVENTIONS
  for Status, health, lifecycle and source responsibilities, adapted to MDIO.
- Prefer simple explicit values, procedures and state switches. Share chip
  behavior and the example CLI; add abstractions/files for current responsibilities.
  Keep SDK headers, RTOS, allocation, platform clocks and logging outside core.
- Applications own board pins/address/clocks, controller setup, scheduling,
  synchronization, retry/recovery policy, network/EtherCAT stacks and power
  delivery. Reuse the supported ESP-IDF MAC/PHY framework with one executing
  owner per operation and explicit treatment of SDK state/partial effects.
- Serialize every access to a driver instance, including snapshots, under the
  application owner. Serialize complete indirect and RMW transactions across all
  bus participants. Release the controller lock before hardware waits/output.
  Guard raw selector and read-clear access through the same owner/collector.
- Use bounded terminal callbacks and aggregate deadlines. Anchor post-write
  waits to observed completion or a conservative later timestamp. Keep admission,
  controller lock, primary result and finite cleanup separate. Preserve accepted
  or uncertain effects on failure, cancellation and teardown.
- Maintain desired settings, observed values and configuration trust separately.
  Restore persistent owned state in a finite order; temporary diagnostic settings
  remain temporary. Follow exact mixed command/status access semantics.
- Give destructive event/counter evidence one collector, retaining consumed
  partial samples. Cached health/link/configuration views perform no I/O.
  Keep transport health, resolved link and application readiness distinct.
- Follow `docs/prompts/EVIDENCE.md`: compare actual pinned sources with exact-part
  datasheets, original tables and applicable clarification. Preserve numeric
  namespaces, access effects and timing qualifiers. Use independent literal
  tests and identified measurements. Unknowns remain explicit; invent no values,
  device support or results. Update affected contracts when evidence changes.
- Preserve vendor payloads/provenance/hashes. Keep archives outside firmware and
  release packages. Use factual GPL references without copying implementation
  into MIT files; retain applicable notices for actually reused permissive code.

## Documentation, examples and tools

Use simple engineering language in code, comments, diagnostics and handoffs.
Write public Doxygen and meaningful procedure comments with each implementation:
units, legal states, ownership, side effects, output validity, errors and bounds.
Review README/CHANGELOG and affected guides at each stage and update changed
instructions alongside behavior. Use source-document checks from the foundation.

Keep examples application-owned and actually linked against the selected SDK.
The shared CLI provides strict input, bounded output and owner-routed feature
coverage. Python runners use finite sessions, explicit fixtures, actual firmware
identity, correlated outcomes and complete bounded artifacts. Preserve failures,
missing evidence and cleanup results in the verdict. Fixture actions require
identified authorized scope.

Use `library.json` as version truth, generating
`Version.h` and applicable manifests reproducibly. Keep firmware Git identity
separate. Ordinary consumers build without Git, Python, Doxygen or sibling paths.
For a release-version change, run `python scripts/generate_version.py sync`,
commit the changed generated header and run the read-only `check`; do not edit
`Version.h` by hand.
Test clean installed/exported consumers and preserve required licenses. If
PlatformIO is used on Windows, use the existing managed installation via a
repository wrapper.

## Collaboration and completion

Use subagents for useful disjoint work or independent review. Assign outcomes,
writable/forbidden paths, shared contracts and acceptance cases. The lead owns
shared headers, architecture, progress, integration and Git. Reviewers are
read-only; use independent build directories and review every contribution.

Choose available models/effort by task: lightweight for mechanical inventories,
capable coding for implementation/tests/docs, stronger reasoning for silicon,
ownership/deadlines and independent review. Record a short rationale and use the
current model if selection is unavailable.

At each prompt entry briefly state relevant rules, scope, source audit, ownership,
documentation/checks and execution branch/checkpoint. Check the final diff against
them. Deliver tests and documentation with working behavior, update progress and
source/feature coverage, and retain explicit blocked dependencies. Continue
independent eligible work within the requested scope. Mark completion only when
the stage's actual outputs/checks exist; keep physical qualification separate.
Finish with the verified commit/sync result and remaining gates.

First-execution workflow policy: create automatic CI triggers at prompt 71;
none exists yet. Preserve existing hooks and required checks. Prompts 72–80
retain the recorded CI-tested SHA and their own local evidence.
