# DP8382x repository guidance

## Current scope

This workspace contains reference material and an implementation plan. Do not
describe planned APIs, example firmware, chip support or tests as implemented.
Read README.md, docs/architecture.md, docs/variants.md and the reference index
before implementation. Follow applicable parent Projects/AGENTS.md rules.
Read docs/prompts/README.md and docs/prompts/COMMON.md when executing a numbered
stage. Editing or auditing the promptset does not itself start implementation.
Before every implementation stage, re-audit the relevant current sibling I2C
headers, source, examples/CLI, tests, Python runners, workflows and agent rules.
Record source revisions, paths and adopted/adapted/rejected conventions; historic
audit reports do not replace live source inspection.

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
- Datasheets for the exact variant/revision are authoritative for register
  behavior. Manufacturer reference drivers still require review.
- Preserve unmodified vendor payloads and their provenance/hashes. Do not copy
  GPL reference implementation into MIT-licensed project files.
- Keep reference archives out of future firmware compilation and release
  packages. Preserve required source-license notices for any code actually reused.

## Verification and collaboration

- Inspect existing checks/workflows before changing implementation. This initial
  workspace has only the reference checker, with no firmware builds or CI yet.
- Documentation changes need relevant local checks; apply the parent CI policy
  when implementation and integration work begins.
- Report host, SDK-build and physical-hardware evidence separately.
- Sibling projects are read-only references unless the user requests edits there.
- Preserve unrelated/concurrent work. The lead owns shared Git mutations; agents
  must not independently switch branches, stage or commit others' changes.
- Inspect the current Git/remote configuration; the original setup's lack of a
  selected remote is not a permanent assertion. Do not invent repository or
  maintainer metadata, inherit sibling-specific Git/CI permissions, or publish
  a package merely to fill metadata fields.
