# Fresh audit of prompts 9 and 10

Audited 2026-10-29 on `promptset/dp8382x-implementation-20261028` from clean,
fetched, upstream-equal `0c4436d6a7449498931607840298d20d9a3a6892`.
The original execution base remains `main` / `origin/main` at
`5e148fffde074b85fadc15d8ff66fe2c77111036`. The lead alone changed
this repository and Git. The final corrective commit/sync SHA is in the
handoff to avoid a self-reference.

## Independent review and source check

Re-read [prompt 9](../../prompts/9-portable-source-foundation.md),
[prompt 10](../../prompts/10-version-tooling.md), COMMON, CONVENTIONS,
VERIFICATION and accepted prompt-7/8 contracts. Reviewed the actual
`7311a81962f45d5c3a0baf7a709b89603875acfe..0c4436d6a7449498931607840298d20d9a3a6892`
diff, current headers, generator, Doxygen checker, CMake registration, tests,
consumer and maintained guides. Three read-only subagents worked in parallel:
strong reasoning for requirements/scope and version preservation, capable
coding review for C++ semantics/tests. Checks ran in separate build or
temporary directories; none changed files or Git. The lead reproduced each finding
against current source and checked the corrections.

The referenced peers remained read-only and clean at TMP1x2
`bafc34067530eb340e55c42dae3fad4eb795b6e9`, TCA9548A
`ce425e1984ac269e7934dd811a06ec08b91e307c`, 24Cxx
`936aa85e21b40b61c6f9cf0306d64c3beb6eaf6e` and MB85RC
`1bf7c293549b1038b2d8f3b7a8d655fa74201e6e`. The lead rechecked
TMP1x2 CMake registrations, TCA9548A's separate consumer, 24Cxx's contract
check and TMP1x2/MB85RC import-time generator calls. Earlier prompt reports
retain their stage-specific peer observations and decisions.

## Confirmed findings and fixes

| Finding in completed work | Correction |
| --- | --- |
| `test/README.md` said three public headers and three isolated translation units, although prompt 10 added `Version.h` and `test/header_version.cpp`. | State the current count of four; retain the historical prompt-9 report's correct three-header observation. |
| README and test guide used bare CTest, which returned success for zero tests. | Add `--no-tests=error` to host and consumer commands. A real tests-OFF zero-test build returned exit 0 bare and exit 8 guarded. |
| The guide said CMake 3.16 was enough for commands using `ctest --test-dir`; that option arrived in 3.20 and `--no-tests=error` in 3.26. | Keep the portable target's CMake 3.16 minimum and state that the documented guarded CTest commands require 3.26 or newer. |
| `test/README.md` still pointed at planning validation as if the current work were documentation-only. | Link the prompt-9/10 implementation reports and label planning validation as historical. |

The [CMake 3.16 CTest manual](https://cmake.org/cmake/help/v3.16/manual/ctest.1.html),
[CMake 3.27 CTest manual](https://cmake.org/cmake/help/v3.27/manual/ctest.1.html)
and installed CMake 4.0.1 help establish the command-version distinction.
No confirmed C++ Status/validator/mask, generator, Doxygen or consumer defect
remained. No unused operation engine, driver stub, SDK/package claim or
unnecessary abstraction was found; code behavior and version `0.1.0` remain
unchanged.

## Final local evidence

On Windows with MinGW-W64 GCC 15.1.0, CMake/CTest 4.0.1 with Ninja, Python
3.12.10 and Doxygen 1.13.2:

- Debug and Release CMake builds compile four isolated public headers and pass
  four registered CTest suites each, using `--no-tests=error`.
- The separate tests-OFF Release consumer passes one registered CTest with the
  same guard. A tests-OFF root build with zero tests is rejected (CTest exit 8).
- Release's deliberate failing runtime check exits 1; its ordinary four suites
  run 127,573 active checks.
- `python scripts/generate_version.py check`, an idempotent second `sync`,
  all 98 Python tests, `python tools/check_doxygen.py` (48 pages and two
  rejected negative fixtures), reference/link and ledger checks pass.
- `git diff --check` passes. No remote CI was dispatched; prompt 71 owns it
  during this first execution.

Host foundation, version tooling, source documentation and build-tree consumer
are **PASS** within those limits. SDK link, installed package, remote CI and
physical PHY remain **NOT RUN** at their later stages. README and test guidance
were corrected; CHANGELOG and progress record the audit. Architecture, API,
silicon ledger, examples and native guides need no change because behavior and
support scope did not change.
