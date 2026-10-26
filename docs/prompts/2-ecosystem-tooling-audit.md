# 2 - Audit examples, tools, repository structure and prompt workflows

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md),
[CLI_CONTRACT](CLI_CONTRACT.md) and [SOURCE_MAP](SOURCE_MAP.md).
Prerequisite: [1](1-ecosystem-core-audit.md)'s core comparison and recorded
execution branch. Briefly restate owned work, source audit, documentation,
local verification and same-branch commit/sync rules.

## Complete outcome

Complete `docs/library-comparison.md` with tooling/consumer comparisons, a
broader library inventory, actual tool availability and a concise future file
responsibility map. Identify incompatible conventions instead of claiming all
libraries use one uniform API or runner protocol.

## Work and source evidence

Reopen current TMP1x2, 24Cxx, INA228, PCA9555 and TCA9548A examples, CLIs,
Python runners, CMake/tests, metadata/version tools, Doxygen checks, workflows
and agent guidance. Inspect relevant declarations and executing code/tests,
recording exact paths, revisions, dirty state and adopt/adapt/reject decisions.

- Trace TMP1x2 `Cli::feed`/`processCommand`/`tick`, bounded output, pending
  work and terminal results. Compare SHT3x-main/24Cxx shared CLI and INA228
  framed HIL parsing, including strict identity/correlation and failure verdicts.
- Inspect TMP1x2 `scripts/generate_version.py`, `Doxyfile`,
  `tools/check_doxygen.py`, 24Cxx registered consumer tests, and relevant
  SCD41/MB85RC package checks. Distinguish tested behavior from documentation
  claims. Generated pages with `EXTRACT_ALL` alone do not prove source comments.
- Read native IDF examples for resource ownership/cleanup; I2C or Arduino
  example matrices do not establish an ESP32-P4 Ethernet backend.
- Read FieldCore-node's current prompt organization, agent/contributor rules,
  implementation plan and tooling. Review TUSB32x's promptset while recording
  which behavior is still planned. Extract clear prerequisites, path ownership,
  completion evidence and scoped delegation rather than copying product code.
- Inventory remaining Projects chip drivers, including ADS1115, INA3221,
  LDC1614, LSM6DS3TR, OPT4001, RV3032-C7, MCP45HVX1, SSD1315 and relevant
  non-I2C drivers. Discover actual directory names. Sample a deeper source only
  where it adds a useful convention; record inventory versus deep-audit coverage.

Inspect this repo's reference checker, parent/root AGENTS and current registered
checks. Record installed compiler, CMake, Python, Doxygen and SDK availability
without installing speculative stacks. Inspect relevant workflow definitions,
not remote badges/runs. Record actual Git remote/ownership; never invent handles
or inherit sibling push, merge, fixture or publication permission.

Map responsibilities to `include/DP8382x/`, `src/`, separate `esp_idf/`
subdirectories, `examples/common/Dp8382xCli.*`, native examples, `test/`,
`tools/dp8382x_hil_runner.py`, version tooling and Doxygen. Files are created
when they implement behavior, not to fill the map. Identify plain C++ MCU,
ordinary P4 Ethernet and application-owned EtherCAT consumers; no framework
or board service becomes a portable-core dependency.

## Delegation, documentation and acceptance

Use disjoint read-only tooling inventories or parser/ownership reviews. Select
available models by complexity, record why, and let the lead reconcile shared
contracts. Siblings stay read-only; agents share the execution branch and do
not mutate Git or race shared output directories.

Update comparison, source map if paths changed, architecture/example plans,
report and progress. Review README/CHANGELOG; keep all unimplemented features
explicit. Verify sampled help/parser/test registration and documented commands
against source; run local reference/link and diff checks. Deliver a concrete
adopt/adapt/reject table with tool limitations and owners, not just an inventory.

Commit and sync this checked task and coherent completed units on the same
execution branch. Remote CI is reserved for prompt 71; current workflow source
inspection does not require dispatching or checking runs.
