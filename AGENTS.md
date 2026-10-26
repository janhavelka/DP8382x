# DP8382x engineering and agent guidance

## Current scope

This workspace contains reference material and an implementation plan. Do not
describe planned APIs, example firmware, chip support or tests as implemented.
Read README.md, docs/architecture.md, docs/variants.md and the reference index
before implementation. Follow applicable parent Projects/AGENTS.md rules, with
the user-requested first-promptset CI exception below.
Read docs/prompts/README.md and docs/prompts/COMMON.md when executing a numbered
stage, plus docs/prompts/CONVENTIONS.md for selected concrete names and layout.
Editing or auditing the promptset does not itself start implementation.
Before every implementation stage, re-audit the relevant current sibling I2C
headers, source, examples/CLI, tests, Python runners, workflows and agent rules.
Record source revisions, paths and adopted/adapted/rejected conventions; historic
audit reports do not replace live source inspection.

## Git workflow: one branch for each promptset execution

The user authorizes routine commits and synchronization for this repository's
promptset work, including complete planning and maintenance changes. Do not ask
again for permission to perform those routine checkpoints.

- Before editing, inspect status, staged changes, current branch and upstream.
  Fetch the configured remote and compare histories. Fast-forward only when safe;
  preserve all local and concurrent work. A dirty worktree alone is not a blocker.
- At the start of every new promptset execution, create a new descriptive branch
  from the synchronized intended base before running its first stage, such as
  `promptset/dp8382x-implementation-YYYYMMDD`. Use the user's requested name/base
  when supplied. Do not execute a new set on main or reuse a previous set's
  branch. If the name already belongs to another execution, choose a new unique
  name; never reset or overwrite it. Record the branch and original base commit
  in `docs/implementation-progress.md`. Establish its same-name upstream on first push.
- Keep that same branch throughout every prompt, subagent and resumed session
  of this execution. Resume the recorded branch and incomplete work. Do not make
  a branch/worktree per prompt or agent, or return to main when a prompt ends.
  Every separate later promptset starts on its own new execution branch. Editing or
  reviewing prompts alone is maintenance and does not start implementation or
  require a new execution branch.
- Commit and sync after every completed prompt, before reporting completion or
  starting the next one. Also commit and sync during a prompt whenever a coherent,
  verified unit is ready: an API with its tests/docs, a complete register procedure,
  an adapter integration or a runner improvement. Do not wait for the whole set.
- Each checkpoint must stand on its own, including relevant tests, source/API
  documentation, guides and changelog updates. Run appropriate local checks
  first. Do not commit broken intermediate code, empty scaffolding or unrelated
  work merely to create a checkpoint. No empty commits for a no-change review.
- Stage only this task's files/hunks, including reviewed subagent work. Inspect
  the staged diff and preserve unrelated changes. Never stash, discard, overwrite
  or commit someone else's work. Use plain commit messages describing the change.
- Sync means fetch, safely integrate upstream changes if needed, push the working
  branch to its configured upstream, then verify the remote result. A local
  commit or fetch alone is not sync. For divergence, preserve both histories and
  use a normal merge when safe, resolve relevant conflicts and rerun affected
  checks. Never rewrite shared history or force-push.
- Verify local/upstream commit equality and zero ahead/behind after the final
  fetch, plus any remaining task changes in the working tree. Set missing
  tracking only when the intended existing remote and same-name branch are clear;
  do not invent a remote, repository URL or maintainer.
- If access, network, branch protection or an unsafe conflict blocks sync, retain
  checked local work and report the exact failure, local commit and next step.
  Keep unsynced state visible and retry when possible; continue independent
  authorized work. Never claim a failed push is synchronized or bypass protection.
- The lead alone switches branches, stages, commits, integrates and pushes shared
  work. Subagents do not mutate Git. Record checkpoint/sync evidence in handoffs;
  put completed commit IDs in the next progress update rather than creating a
  self-referencing commit-hash update loop.
- Routine commits and pushes are authorized. Release tags, package/release
  publication, hardware flashing/fixture operation and sibling changes remain
  separate task scopes. Do not infer them from the checkpoint rule.

## CI and local verification

- Current runnable checks are `python tools/verify_references.py` and
  `git diff --check`. The repository has no driver build, firmware, test suite or
  CI workflow yet. Add commands and evidence as implementation actually exists.
- For the first implementation promptset, remote CI execution and run-status
  checking belong to **prompt 71 only**. Prompts 1-70 and 72-80 do not dispatch,
  poll, wait for or require remote CI results, including after substantive fixes,
  commits or pushes. This explicit user-requested exception overrides the
  parent's recurring exact-final-commit CI requirement for that first execution.
- Inspect checked-in workflow definitions/check registration during the initial
  audit, when affected by changes and at prompt 71. Reuse their commands, pinned
  dependencies and source selection. Auditing sibling CI means reviewing source,
  not checking their remote runs. Do not repeatedly inspect unchanged workflows.
- Keep focused local verification throughout: host behavior, CLI/Python, source
  documentation, metadata/packages and actual SDK builds as relevant. Small prose
  changes need appropriate local text/link checks. Repeat checks for new changes,
  confirmed failures or unresolved concerns, not the full matrix after each edit.
- At prompt 71, commit/sync the completed CI work and verify the supported remote
  matrix for that actual commit. Repeat affected jobs there for confirmed failures
  or changed inputs. Later prompts keep that CI SHA as historical evidence and
  validate locally; they do not need a new remote run to finish this first set.
- Push-triggered automatic runs may still occur. Outside prompt 71 they are not
  an execution/completion gate for the first set. Preserve workflow triggers,
  hooks and required merge/release checks; do not use skip markers, disable checks
  or bypass protection to implement this exception. A later explicit user request
  can change the CI scope. After the first promptset, the parent CI policy applies.
- Fix confirmed failures without weakening assertions, dropping suites, suppressing
  documentation warnings or fabricating success. Register tests so they execute;
  zero tests and inactive Release assertions are not acceptable evidence.
- Report host tests, portable/package consumers, linked SDK firmware, remote CI
  and physical hardware separately with PASS/FAIL/NOT RUN/BLOCKED. Source grep,
  mocks, header shims and workflow files cannot prove a real link or physical PHY.

## Layout, naming and maintainability

These are intended boundaries; create files only when their behavior or durable
contract is implemented. Do not populate the future tree with successful stubs.

| Location | Responsibility |
| --- | --- |
| `include/DP8382x/` | Portable public types, Config, Status, register constants, driver and generated version |
| `src/` | Shared chip behavior, transactions, operations and passive diagnostics |
| Separate `esp_idf/` subdirectories | SDK-only public adapter and implementation, excluded from portable compilation |
| `examples/common/` | Framework-free example CLI and bounded console support |
| `examples/esp_idf/`, portable examples | Application-owned board/transport/console and real consumers |
| `test/`, `tools/`, `scripts/` | Executing tests, Python runners, guards and metadata tooling |
| `docs/`, `docs/reference/` | Maintained contracts/guides, evidence and preserved reference material |

- Use namespace/class `DP8382x`, PascalCase types, camelCase functions/fields,
  `_camelCase` private members and uppercase enum values/constants. Follow the
  shared engineering contract and current peer conventions without I2C names.
- Prefer small explicit values, clear ownership and direct procedures. Reuse,
  simplify or remove existing code before adding abstractions. Every new class,
  file or layer needs a current responsibility and consumer/test. Avoid generic
  managers, registries, speculative extension points and duplicate API aliases.
- Group implementation by real responsibilities; neither one huge source file
  nor a class/file per register is a goal. Keep one implementation of each chip
  procedure and one shared example CLI grammar.
- Use simple engineering English in comments, diagnostics, commit messages and
  reports. Document units, valid states, ownership, side effects, error/output
  behavior and time bounds. Cite non-obvious silicon sequences at their source.
- Maintain Doxygen in public headers and meaningful procedure comments in source
  with every behavior change. Document parameters, output validity, returned
  statuses, callback lifetimes, I/O side effects and deadlines where relevant.
  Start source documentation with the first implemented files; prompt 76 is an
  audit, not permission to defer it. Run the relevant documentation checks.
- Ship tests and source/user documentation with behavior, not as debt for later
  prompts. Do not turn planned contracts or compile-only fixtures into support
  claims. Keep public naming stable and resolve conflicts in all consumers.
- Review README, CHANGELOG and affected architecture/API/example/test/tool guides
  at every prompt. Update affected files with the code; record why an inspected
  guide needs no change instead of adding empty edits. Review the diff for simple
  control flow, useful names, duplicated rules and stale comments before commit.

## Engineering boundaries

- Start with the exact DP83826I part. Explicitly qualify each additional variant.
- Implement the complete publicly documented software-accessible feature set of
  each supported part, not only the first board's needs. Read
  docs/prompts/FULL_COVERAGE.md. Runtime opt-in features remain required scope;
  raw register access alone is not implementation of a multistep feature.
- Research manufacturer GitHub/SDK sources, datasheets, app notes, EVM/tool
  guides and relevant primary implementations. Maintain exhaustive register,
  field and documented-procedure coverage with pinned source evidence. Missing
  board wiring or an incomplete vendor driver cannot justify omitted features.
- Keep generic 10/100 capabilities separate from the board's 100/full policy.
- Reuse the supported ESP-IDF Ethernet MAC and PHY framework. Keep board pin
  assignments, PHY address, clocks, recovery policy and EtherCAT outside the core.
- Public interfaces belong in include/DP8382x/, implementation in src/, and
  platform examples in examples/. Do not create successful hardware stubs.
- Deliver a real framework-independent C++17 core and native ESP-IDF adapter.
  Establish operation ownership before implementation so portable behavior and
  the IDF base do not independently configure, reset or poll the same PHY.
  Keep SDK headers, RTOS, allocation, logging and platform timing out of the core.
- Use bounded waits, explicit errors and application-controlled serialization.
  Serialize complete indirect register transactions, not just individual reads.
  Release the controller lock before waits/later polls; operation admission is
  separate. Short reads/writes need no cooperative token/job wrapper.
- Borrow synchronous terminal callbacks/context with documented lifetime, complete
  transaction ownership and aggregate deadlines including controller/lock waits.
  No hidden retries, background tasks, platform delays, re-entry or retained
  caller buffers. A timeout argument alone does not prove a bounded adapter.
- Keep transport health, link/negotiation, configuration trust and application
  readiness separate. Cached snapshots/health/CLI rendering do no I/O. Cable
  disconnect is not an MDIO failure. One collector owns read-clear/latch evidence.
  Retain consumed event samples even when later reads fail. Mixed control/status
  writes require exact W0C/W1C/W1S/RC semantics and command-neutral composition.
- Preserve uncertain write effects, partial configuration and primary/cleanup
  errors. Cancellation/end cannot undo accepted writes. Deferred cleanup must
  have an explicit finite budget and leave unresolved hardware effects visible.
- Use the sibling structured Status with static-lifetime messages and preserved
  backend detail; do not leak SDK error types into core or invent I2C NACK status.
  Unknown/unsupported silicon, incomplete software and unavailable fixtures are
  different facts. Unsupported claims require exact-part evidence.
- Datasheets for the exact variant/revision are authoritative for register
  behavior. Manufacturer reference drivers still require review.
- Follow docs/prompts/EVIDENCE.md and the extracted public-library catalogues.
  Reopen actual pinned code and exact datasheet tables before using any address,
  mask, identity, timing or sequence. Never invent missing data or treat vendor
  function names, repeated code, a prompt or a passing fake as silicon proof.
  Separate code observations, corroborated facts, conflicts and measurements.
  Correct confirmed errors in prompts/contracts/code/tests/docs together; keep
  unresolved facts gated. Physical checks need real identified authorized fixtures.
- Preserve unmodified vendor payloads and their provenance/hashes. Do not copy
  GPL reference implementation into MIT-licensed project files.
- Keep reference archives out of future firmware compilation and release
  packages. Preserve required source-license notices for any code actually reused.

## Examples, tools and evidence

- Native IDF examples use the supported MAC/PHY framework and actual SDK APIs,
  not an Arduino facade. Core and shared CLI compile without framework headers.
  Do not copy sibling S2/S3 I2C build matrices into an unsupported EMAC example.
- Match sibling help/Status/CLI conventions while exposing every documented PHY
  capability through the full-coverage ledger. Reject malformed inputs without
  transfers. Keep raw writes, resets and link-disrupting diagnostics explicit,
  bounded and subject to application ownership/maintenance admission.
- Python runners have finite sessions/output, explicit fixtures, actual firmware
  identity, correlated terminal results, preserved artifacts and tested failure
  verdicts. Missing/truncated/stale output, skipped tests or failed restoration
  cannot become PASS. Dry runs never claim HIL evidence or auto-flash hardware.
- Use independent datasheet vectors and failure injection at every transaction
  boundary. Test complete MMD atomicity, reset/restore, 10BASE-Te, health/link
  distinction, side-effect reads, partial writes and safe cleanup. Guards supplement
  actual portable and SDK compilation, not replace them.
- When metadata is added, use `library.json` as version truth and regenerate
  `Version.h`/applicable manifests; do not hand-edit generated versions. Keep
  firmware Git identity separate and unknown when unavailable. Ordinary core
  consumers must not require Git, Python, Doxygen or sibling include paths.
- If PlatformIO is introduced on Windows, use the existing managed installation
  through a repository wrapper rather than installing another Core. Advertise
  only real implemented builds and test clean extracted-package consumers.

## Collaboration and completion

- Sibling projects are read-only references unless the user requests edits there.
- Use subagents when there is useful disjoint work or independent review. Assign
  writable paths, frozen shared contracts, forbidden scope and acceptance cases.
  Shared headers, architecture, integration, progress and Git have one lead owner.
  Reviewers are read-only; agents do not race a shared build directory.
- Choose from the models actually available for each task. Use a lightweight
  model for bounded inventories and mechanical checks, a capable coding model
  for scoped implementation/tests/docs, and stronger reasoning for ownership,
  register semantics, deadlines and independent correctness review. Set model
  and reasoning effort when supported; record the choice and short rationale.
  Use the current model if selection is unavailable. Do not delay useful work
  for a preferred model, or delegate shared semantic decisions to an unchecked
  mechanical pass. Task scope and evidence requirements do not change by model.
- The lead reviews/integrates every contribution and fixes confirmed findings.
  Prefer available agents suited to the task; do not hard-code model versions
  copied from another repository. Preserve all unrelated/concurrent work.
- At each prompt entry, briefly restate the applicable rules in the work plan:
  scope/owned paths, fresh sibling audit, chosen names, single MDIO owner,
  documentation/local checks, branch and commit/sync checkpoint. Check them again
  against the final diff. This is a short working reminder, not copied boilerplate.
- Follow COMMON and the selected prompt's acceptance. Update progress, source/
  feature coverage and reports with actual commands, results, constraints and
  next steps. Never report implementation or hardware gates closed by prose.
- Keep blocked required evidence visible while continuing independent eligible
  work. Missing hardware or an optional runtime mode does not excuse omitted
  documented software functionality. Use the first-set CI exception accurately.
- Finish checked work with the authorized commit/sync checkpoint. Report changes,
  evidence, actual branch/commit/sync state and remaining gates. Do not start a
  later prompt when only one was requested, or begin implementation from a request
  to maintain this guidance.
