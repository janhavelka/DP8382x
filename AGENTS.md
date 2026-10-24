# DP8382x repository guidance

## Current scope

This workspace contains reference material and an implementation plan. Do not
describe planned APIs, example firmware, chip support or tests as implemented.
Read README.md, docs/architecture.md, docs/variants.md and the reference index
before implementation. Follow applicable parent Projects/AGENTS.md rules.

## Engineering boundaries

- Start with the exact DP83826I part. Explicitly qualify each additional variant.
- Keep generic 10/100 capabilities separate from the board's 100/full policy.
- Reuse the supported ESP-IDF Ethernet MAC and PHY framework. Keep board pin
  assignments, PHY address, clocks, recovery policy and EtherCAT outside the core.
- Public interfaces belong in include/DP8382x/, implementation in src/, and
  platform examples in examples/. Do not create successful hardware stubs.
- Establish the framework boundary before implementation; a framework-neutral
  register layer is a design option, not a requirement to duplicate the IDF base.
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
- No remote is selected by this setup. Do not invent a repository URL or publish
  a package merely to fill metadata fields.
