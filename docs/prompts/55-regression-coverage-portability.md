# 55 - Audit independent coverage, portability and executing test registration

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md),
[FULL_COVERAGE](FULL_COVERAGE.md), [ENGINEERING](ENGINEERING.md) and
[VERIFICATION](VERIFICATION.md). Prerequisites:
[53](53-regression-transaction-ownership.md),
[54](54-regression-lifecycle-deadlines.md) and primary feature/CLI/campaign
outputs. This is gap closure, not permission to defer tests from
implementation tasks.

## Current evidence and ownership

Inspect `TMP1x2/CMakeLists.txt`, `test/core_no_arduino/compile_main.cpp`,
current allocation/source-documentation guards and 24Cxx host CLI/Python
registration. Read production functions with their tests and verify enabled
Release assertions. Reopen source ledgers and exact primary TI sections for
uncovered behavior. Record revisions/paths, dirty state and
adopted/adapted/rejected conventions. Review workflow commands as local
references without dispatching or inspecting remote runs.

Assign paths and select subagent models under COMMON. Keep shared contracts
and Git with the lead; use independent review for ownership and failure
semantics.

## Source requirements and regression vectors

Reconcile [source catalogues](../reference/library-evidence.md) with
[EVIDENCE](EVIDENCE.md), production code and independent tests. Each accepted
mask, ID filter, MMD address and timing bound needs the exact specification
and a disposition for source conflicts. Repeated inherited blobs count as one
code source. Correct confirmed errors in owning code/tests/guides; unresolved
contradictions retain their exact affected gate.

## Implement

Use `docs/feature-coverage.md` as the one matrix: one row per ledger procedure
and feature ID, exact part/mode, command or cfg path, owner route, record,
host test name and physical case or reasoned N/A. Require every ID and field
owner to resolve to a complete row. Prompts44/46/47 append CLI cells,47 closes
those cells,55 closes primary coverage;53 adds only gaps. `docs/cli.md` owns
grammar/records, not a duplicate matrix.

Check independent oracles for field extraction, reserved/access masks,
exact per-part W0C/W1S/RC/latch semantics, including description-only effects, reset restoration and feature sequencing. Production
metadata agreeing with its own checker proves no silicon interpretation.
Resolve confirmed omissions in the owning module with tests and source/user
docs; keep genuine ambiguities as exact open gates rather than convenient fake
semantics.

Verify explicit portable/SDK/example/test source lists and public standalone
headers. Core consumers require no framework, sibling include path, hidden
platform time, Git, Python or Doxygen. Execute the existing minimal ordinary
consumer with tests OFF; task 68 later supplies the complete two-instance
owner application. Do not invent other MCU support or import I2C S2/S3
firmware matrices.

Run registered Debug/optimized suites with strict warnings and locally supported
`-fsanitize=undefined -fsanitize-trap=undefined`. Record second compiler and
other sanitizers NOT RUN locally; Linux prompt 71 runs them and the tests-OFF
consumer. Do not install toolchains for this check. Keep Release checks active,
registered WILL_FAIL probes executing, and no empty suites. Extend shared focused
fixtures rather than duplicate harnesses or AST frameworks.

## Acceptance

Execute affected complete host/CLI/Python suites, standalone headers and the
plain consumer. Verify core builds with documented exception/RTTI settings,
without platform symbols and without accidentally compiling reference
archives. Rebuild real SDK images if public/adapter paths change; missing SDK
remains a separate BLOCKED layer. Report behavior coverage and unresolved
sources/SDK/fixtures, without an unsupported coverage percentage.

Update Doxygen, API/testing/feature guides and `test/README.md` with a link to the
feature-coverage matrix, actual registered suites and confirmed corrections.

Review README/CHANGELOG and affected guides. Run focused local and
source-documentation checks (Doxygen where applicable), then update
evidence/progress under COMMON. Commit/sync checked units and completion on
the execution branch; first-set remote CI belongs only to 71.
