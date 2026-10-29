# Prompt 10: deterministic release version

Executed 2026-10-29 on `promptset/dp8382x-implementation-20261028` from
clean, fetched and equal prompt-9 checkpoint
`8ca1d6032c591596c358266e083eeedc8505a87f`. The original execution
base remains `main` / `origin/main` at
`5e148fffde074b85fadc15d8ff66fe2c77111036`. The lead alone integrated,
committed and synchronized; sibling repositories were read-only.

## Owned output and decisions

Prompt 9's C++17 INTERFACE target, four host suites, tests-OFF consumer and
Doxygen checker satisfy the prerequisite. This stage adds `library.json`,
`scripts/generate_version.py`, generated `include/DP8382x/Version.h`,
`test/test_version_tool.py`, `test/header_version.cpp`, CMake source selection
and a consumer include. The [tool guide](../../../scripts/README.md) defines
the exact syntax and future identity boundary. README, CHANGELOG,
architecture, public-header/test guides and progress record reflect only
these working outputs.

`library.json` starts at **0.1.0**, a release label for the existing portable
foundation, not a per-prompt counter or driver support claim. Its `MIT`
license agrees with `LICENSE`, whose copyright is Thymos Solution s.r.o.; the
repository URL agrees with configured origin
`https://github.com/janhavelka/DP8382x.git`. No maintainer handle, Arduino,
PlatformIO, SDK target or publication metadata was inferred. Current root
`CMakeLists.txt` has no version field, and no SDK/package manifest exists, so
the generated header is the only applicable output. Future manifests must
join this generator when their real build paths arrive.

The generator accepts exactly five nonblank string fields and a version of
`MAJOR.MINOR.PATCH`, with ASCII decimal components 0..65535 and no leading
zeros, prefix or suffix. Duplicate/unknown/missing keys, wrong types,
malformed UTF-8/JSON and nonfinite JSON constants fail. It renders UTF-8/LF
bytes before touching output. `check` reads only and fails for stale/missing
bytes. `sync` skips identical bytes and replaces one changed header through a
closed same-directory temporary file; temp creation, write and replace failures
retain the previous valid header. There is no import-time action or Git call.
The public constants have Doxygen comments and carry no build time/hash/dirty
state. Later examples must refresh their own build-directory firmware identity
incrementally; if Git identity is unavailable, an archive build reports
`unknown`. Ordinary core use never needs Git or Python.

## Live peer evidence

All peer working trees were clean at the exact HEADs below. `rg --files` found
the paths, and the lead checked the named script logic, test registration and
agent guidance against source. The bounded peer inventory used `gpt-6-luna`
for paths; a read-only preservation review used `gpt-6-astra` for I/O and
Windows edge cases. A `gpt-6.1-sol` coding agent wrote only isolated fixture
tests after generated-file ownership was frozen. The lead wrote the generator,
headers/integration/docs and reviewed every contribution; agents did not
mutate Git or build/edit siblings.
The independent preservation review found no actionable defect in the actual
generator, CMake/consumer or fixture diff. Verified code/test/tool checkpoint:
`37c787203c7aa7de86e8871b200846d0f0b4ed5c`.

| Peer at exact clean revision | Observed source and tests | DP8382x decision |
| --- | --- | --- |
| TMP1x2 `bafc34067530eb340e55c42dae3fad4eb795b6e9` | `library.json`, `scripts/generate_version.py::_expected_outputs/_sync_outputs/main`, `include/TMP1x2/Version.h` and `tools/test_version_sync.py` cover a temporary version change; root `CMakeLists.txt` registers `version_sync`. `AGENTS.md` directs sync and forbids hand edits. The script also writes CMake/Doxyfile/IDF versions and its unconditional tail `main([])` can sync on import. The one sibling test does not cover invalid metadata, read-only check, failed writes or byte/mtime preservation. | **Adopt** one metadata truth and isolated drift tests. **Adapt** to one current output with stricter parsing and preservation tests. **Reject** import-time mutation, build/Git timestamps and unimplemented manifests. |
| MB85RC `1bf7c293549b1038b2d8f3b7a8d655fa74201e6e` | `library.json`, `scripts/generate_version.py::_expected_outputs/_sync_outputs`, `include/MB85RC/Version.h` and `AGENTS.md` use generated release values. Its tail also calls `main([])` on ordinary import; no dedicated version-sync CTest was found. | **Adapt** explicit sync/check, without peer build metadata aliases or import side effects. |
| 24Cxx `936aa85e21b40b61c6f9cf0306d64c3beb6eaf6e` | `library.json`, `scripts/generate_version.py::_expected_outputs/_sync_outputs`, `include/EEPROM24Cxx/Version.h`, `AGENTS.md` and `tools/check_contracts.py` use generated values; root CMake has no dedicated version mutation test. | **Adapt** its contract check intent into direct temporary failure/preservation cases; **reject** package/IDF claims from a different library. |

## Local verification

Windows, MinGW-W64 GCC 15.1.0, CMake 4.0.1 with Ninja, Python 3.12.10 and
Doxygen 1.13.2 were available. `python scripts/generate_version.py sync`
created the committed header; `check` passed. A second `sync` reported
`unchanged`, with identical SHA-256
`3127a2bcb52fd005c1b9c5514a4777fc55af60efc118942df99510d512a350cc`
and unchanged modification time. Temporary fixtures never modified the
committed header.

Python discovery passed **98/98** tests, including 13 version tests. They
exercise independent literal output, boundary versions, malformed/duplicate
metadata, stale/missing/CRLF/non-UTF8 output, no-write checks and import,
idempotence, actual CLI failure exits, and injected temp creation/partial
write/replace failures.
Debug and Release CMake configure/build/CTest each passed **4/4**; both compiled
all four public headers alone. A separate Release tests-OFF consumer that
includes `Version.h` passed **1/1** without generator execution. Doxygen
passed 48 public pages, member/link checks and two rejected negative fixtures.
The ledger check passed 96 registers/52 procedures/35 features. Reference/link
and diff checks pass after the report link was added; the initial reference
check correctly caught the not-yet-created report link, which was resolved.

| Evidence layer | Result | Limit |
| --- | --- | --- |
| Host/tool and source documentation | PASS | Version fixtures, C++17 suites and Doxygen; no PHY I/O. |
| Portable build-tree consumer | PASS | Uses committed header with tests OFF; no installed package yet. |
| SDK link, remote CI, physical PHY | NOT RUN | Later prompts own these layers; first-execution remote CI is prompt 71. |

The final diff contains no driver behavior or new variant/silicon fact. Example,
native, feature-coverage, register-ledger and source-map documents need no
change. The version workflow is complete for its present one-output scope;
package/export and flashed-image identity remain separate implementation work.
