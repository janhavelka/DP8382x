# Prompt 2: examples, tools, structure and workflow audit

Completed 2026-10-28 on `promptset/dp8382x-implementation-20261028`, the
execution branch created from synchronized clean `main`/`origin/main` at
`5e148fffde074b85fadc15d8ff66fe2c77111036`. Prompt 1's core comparison
and verified synced checkpoint `77e839810fe8338223a2c1fef89f0af5f48a6f3e`
were present before this stage. The [live comparison](../../library-comparison.md)
contains the source paths, exact revisions/dirty state, observed behavior and
adopt/adapt/reject decisions. The final handoff records this stage's commit and
upstream equality to avoid a self-referencing commit. The comparison was
checkpointed and synced at `05d69f585e8d8984b90645a8ec931802b071ecb1`.

## Outcome and ownership

The comparison now covers actual shared CLI parsers and output, framed HIL
sessions, package consumers, version generation, source-documentation checks,
native example cleanup, prompt organization, the wider Projects inventory,
installed local tools and a concise future file map. TMP1x2, 24Cxx and SHT3x
CLI behavior and INA228, PCA9555 and TCA9548A runner protocols disagree in
specific ways; no uniform sibling protocol was assumed. A CLI attach must stay
local, terminal evidence needs correlated retention, and uncertain cleanup must
remain visible. The future runner has a single strict schema and finite serial
and artifact limits. No sibling I2C example establishes a P4 Ethernet backend.

The portable C++17 core, separate native adapter, ordinary P4 Ethernet app,
application-owned EtherCAT boundary, host/consumer tests, CLI, runner, version
tooling and Doxygen have planned owners in the comparison and architecture.
Files will be created with behavior, not to fill the map. The source map's
current path examples were checked against discovered directories; none needed
correction. These decisions do not freeze the later silicon, SDK or public API
contracts.

Three read-only subagents covered disjoint work: `gpt-6-astra` (high reasoning)
for CLI parsing, session correlation and failure verdicts; `gpt-6-luna`
(medium) for mechanical version, CMake, Doxygen and package inventory; and
`gpt-6-sol` (medium) for native resource ownership, FieldCore/TUSB prompt
organization and the wider inventory. The lead
reconciled their findings against current source, test registration and workflow
definitions, and alone edited shared documents and Git. Siblings, FieldCore-node
and TUSB32x remained read-only. No sibling permissions or product framework were
imported.

## Source and registration review

The lead checked live paths with `rg --files`, reopened declarations and executing
code, and checked representative help/parser tests against CMake/CTest, Unity
entry points or workflow invocations. TMP1x2 registers `cli`, `console` and a
compiled-fixture `hil_runner` in root CMake; its `Cli::feed`, `processCommand`
and `tick` show finite parsing/polling but its CLI cache can overwrite terminal
output. 24Cxx registers `cli` and `cli_transcript`, with bounded session/result
handling; `test/core_no_arduino/` was not counted as a registered test.
SHT3x-main's shared `CliString::assign` truncates and the inspected checks do
not execute the dispatcher, so its help/source checks were not treated as a
parser suite. INA228's parser groups are invoked by its workflow, but its serial
read and default UNKNOWN verdict need stronger bounds. TCA9548A's parser and
artifact tests are invoked by workflow and show finite reads, full writes and
manifest content finalized after its other payloads. PCA9555's serial
write/flush path needs a stricter bound.
These tests and workflows were inspected, not executed here.

TMP1x2's `library.json` generator has an intentional import-side execution hook
and its Doxygen checker validates warnings/output; `EXTRACT_ALL` alone says
nothing about declaration comments. SCD41 executes a clean extracted consumer
and separately compiles target package consumers; MB85RC checks package content
and metadata without a linked consumer. Package and SDK claims in the comparison
are limited to these actual checks. This repository has no CMake project,
manifest, Doxyfile or workflow definition yet. `tools/verify_references.py`
checks local provenance, hashes, archives and links, not live source semantics.

## Documentation and verification

Owned changes are the comparison, architecture, native-example plan, this report,
progress, README, CHANGELOG and reports index. README/CHANGELOG now state that
prompts 1 and 2 are evidence audits with no driver. `test/README.md` remains
correct because the runnable check list did not change. `docs/prompts/SOURCE_MAP.md`
remains correct because no mapped path changed. Reference/variant/board guides
remain unchanged because this stage added no silicon, SDK or board measurement.
Future public Doxygen is required with real declarations and must be checked for
comment coverage, warnings and generated pages, as recorded in the architecture.

| Evidence layer | Result for this prompt |
| --- | --- |
| Reference/link | PASS: `python tools/verify_references.py` on Windows PowerShell with Python 3.12.10; 27 vendor artifacts (43,240,385 bytes), 25 text extracts and 1,104 local links in 127 Markdown files. |
| Diff | PASS: `git diff --check`; documentation whitespace check. |
| Host driver behavior | NOT RUN: no DP driver/test exists; sibling tests were inspected only. |
| Portable/package consumer | NOT RUN: no DP public core or package exists. |
| SDK link | NOT RUN: `idf.py` is unavailable in this shell/checked locations; no DP adapter/example exists. |
| Remote CI | NOT RUN: prompt 71 owns first-execution dispatch/status/wait; no DP workflow exists yet. |
| Physical PHY | NOT RUN: no identified fixture action was performed. |

The installed local environment has MinGW-w64 g++ 15.1.0, CMake 4.0.1,
Ninja 1.12.1, Doxygen 1.13.2, managed PlatformIO Core 6.1.19 and Python
3.12.10. Tool availability is not a DP build result. The configured repository
remote is `https://github.com/janhavelka/DP8382x.git`; it is the only remote
used for this execution. Prompt 3 can start after this checked commit is synced.
