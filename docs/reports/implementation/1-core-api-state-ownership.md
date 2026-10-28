# Prompt 1: current driver API, state and ownership audit

Completed 2026-10-28 on `promptset/dp8382x-implementation-20261028`, from
synchronized clean `main`/`origin/main` commit
`5e148fffde074b85fadc15d8ff66fe2c77111036`. No prerequisite stage was
required. The completed artifact is the [live source comparison](../../library-comparison.md).
The final handoff records this report's commit ID and verified upstream equality,
avoiding a self-referencing commit.

## Outcome and decisions

The comparison traces current declarations, implementation and representative
registered tests in TMP1x2, INA228, PCA9555, TCA9548A, MB85RC, BME280,
SHT3x-main and FieldCore-node. It records each exact HEAD and clean state,
source path/symbol, observed behavior, conflict and planned DP8382x destination.
No source was copied from a sibling and no silicon or SDK behavior was inferred
from I2C conventions. FieldCore's INA228 v3.0.4 pin is distinguished from the
current sibling HEAD.

The selected core conventions are the shared static-message `Status` shape with
MDIO error meanings; four passive managed-transport health states; separate
link, desired settings, observation and configuration trust; bus-silent local
binding; explicit bounded hardware operations and retained terminal results.
The application owns instance serialization and a controller-wide lock across
each complete indirect MDIO or read/modify/write transaction. Cleanup and
uncertain effects remain separate from the primary result. The
[architecture](../../architecture.md) now states these decisions without
freezing signatures before prompts 6-8.

Owned changes: `docs/library-comparison.md`, `docs/architecture.md`, this
report, `docs/implementation-progress.md`, `README.md`, `CHANGELOG.md` and
`docs/reports/README.md`. No public header, driver source, build metadata,
test, fixture or sibling repository was changed. Prompt 2 still owns broader
tooling, CLI, runner, package and workflow inventory.

## Source and registration review

The lead rechecked the delegated symbol paths and representative test entry
points against live files. TMP1x2 uses explicit CMake source lists and CTest
registration. PCA9555, INA228, MB85RC, BME280 and SHT3x-main use native
PlatformIO environments with Unity `RUN_TEST` entries; TCA9548A also selects
its driver suite through `test_filter`. FieldCore-node's INA228 suite has
`RUN_TEST` adapters and a native suite link entry. Root CMake files that only
register ESP-IDF components were not mislabeled as host CTest registrations.
These are source/registration observations, not executed sibling tests.

Three read-only subagents covered disjoint sources: a stronger reasoning review
of TMP1x2/PCA9555 Status, state and cleanup; a stronger reasoning review of
INA228/FieldCore-node owner boundaries; and a lightweight mechanical/source
inventory of TCA9548A, MB85RC, BME280 and SHT3x-main. Model choice followed
the task's state/ownership complexity. The lead owned the decisions, files,
integration and Git, and independently checked citations and registrations.

## Documentation and evidence

`README.md` and `CHANGELOG.md` were updated because their all-stages-unstarted
description and historical entry needed the prompt 1 result. The reports index
links the current audit. `docs/variants.md`, reference indices, example/test/tool
READMEs and feature ledgers were reviewed for this scope; no silicon claim,
example behavior, build instruction or fixture changed, so they need no edit.
The source-documentation rule is recorded in the comparison and architecture:
add useful public Doxygen with real declarations and check it from prompt 9,
including units, validity, ownership, side effects, errors and bounds. No fake
headers or empty build targets were introduced.

| Evidence layer | Result for this prompt |
| --- | --- |
| Reference/link | PASS: `python tools/verify_references.py` on Windows, Python 3.12.10; 27 vendor artifacts (43,240,385 bytes), 25 text extracts and 1,098 local links in 126 Markdown files. |
| Diff | PASS: `git diff --check`; documentation whitespace validation. |
| Host driver behavior | NOT RUN: prompt 1 adds no driver or test; sibling tests were inspected but not executed. |
| Portable/package consumer | NOT RUN: no public core or package exists. |
| SDK link | NOT RUN: no DP8382x SDK adapter or firmware exists. |
| Remote CI | NOT RUN: first-execution dispatch/status/wait is assigned to prompt 71; this repository has no workflow definition yet. |
| Physical PHY | NOT RUN: no fixture operation was requested or performed. |

The initial reference/link attempt while this report file was still absent
reported two missing report links. After creating the linked file, the final
reference/link run passed. This was a document ordering issue, not an archive
integrity or source failure.

## Remaining gates

Prompt 1's source comparison is complete. Prompt 2 remains independent and
unstarted. Prompts 3-5 must establish exact silicon procedures; prompt 6 must
assign each portable/SDK executing owner; prompts 7-8 must freeze transport,
Status, lifecycle, deadlines and cleanup signatures. Host, SDK, package, CI and
physical evidence remain future layers and are not implied by this audit.
