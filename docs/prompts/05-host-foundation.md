# 05 - Build the portable foundation and version tooling

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-04.

Read [CONVENTIONS](CONVENTIONS.md) and record the relevant rules, selected names,
owned paths, current sources, documentation/checks and same-branch checkpoint.
Use coding models for the scoped build/validator work and a lightweight model
for mechanical manifest checks when useful; the lead reviews all results.

## Fresh audit and owned work

Inspect current TCA9548A/TMP1x2/24Cxx CMake/test consumers, public-header tests,
scripts/generate_version.py and package/version checks. Read actual registration and
generated-file rules.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Work

Introduce a real C++17 portable target and the passive values, Status helpers, masks and
pure validators needed by implemented functionality. Public headers live under
include/DP8382x. Do not add a nonfunctional device facade or future method bodies. Keep
core source lists explicit; SDK paths are separately selected.

Create standalone CMake/CTest with tests OFF by default, isolated headers and a small
linked consumer. Set strict supported warnings and meaningful test registration. Add
tests for passive enums, masks, invalid combinations and status semantics; do not count
compile-only fixtures as protocol tests. Keep assertions active in Release.

Add accurate project metadata only for outputs that exist. Use library.json as the
version source and a reproducible scripts/generate_version.py sync/check workflow for
Version.h and applicable component metadata. Preserve actual repository
license/ownership; use observed remote metadata only when appropriate. Do not publish or
imply support for every PlatformIO/Arduino target.

Make core consumption independent of Python/Git/Doxygen and sibling paths. Build
identity is separate example metadata, with unknown identity for source archives and
reliable incremental refresh once examples exist. Add concise contribution/agent and
source documentation for the new commands. Keep future feature and SDK support claims
explicit.

## Concrete foundation and file ownership

Reopen TMP1x2 `Status.h`, `Config.h`, `CommandTable.h`, `CMakeLists.txt`,
`scripts/generate_version.py`, `Doxyfile` and `tools/check_doxygen.py`. Compare
24Cxx's registered tests and separate consumer. Inspect actual commands rather
than copying target names or the sensor library's unfiltered IDF source list.

Start with implemented `Status.h`, required pure config/variant validators and
source-proved constants in `CommandTable.h`. Use `DP8382x::cmd::REG_*`/`MASK_*`
with one authoritative value and documented access rules. `Version.h` is
generated. Put driver/snapshot declarations in `DP8382x.h` only when they have
real behavior; do not add `start*` methods returning placeholder success.
Create private implementation files only when logic requires them; an honest
header-only passive foundation is preferable to an empty `.cpp` used as a prop.
Later chip logic still needs a real portable consumer.

Use option `DP8382X_BUILD_TESTS` default OFF and explicit CMake source lists.
Organize initial cases under `test/` by actual responsibility, such as status,
validation, register vectors and portable consumer. Test helper behavior and
error cases; do not add hundreds of tests that only mirror constant assignments.
Each portable header must compile alone, and the test executable must exercise
values at runtime with assertions/checks active in Release. Demonstrate a failing
case actually causes test failure; zero discovered tests cannot pass the stage.

Implement the version tool's `sync` and `check` modes with deterministic output,
strict metadata parsing and a meaningful nonzero check result for stale files.
Reject invalid versions without partially replacing generated files. Preserve
actual owner/license/remote fields; only generate manifests for supported build
paths. A normal consumer must not run generation or discover Git at configure time.

Add `Doxyfile` and a focused `tools/check_doxygen.py` when the first public source
appears. Reuse the peer principle of checking actual generated pages and links,
with a small project-appropriate check rather than copying its full private-type
checker blindly. Keep supported warnings actionable, required public inputs
explicit and vendor/generated build trees excluded. Test the checker if it makes
substantial pass/fail decisions; do not replace Doxygen with a keyword grep.
TMP1x2 enables `EXTRACT_ALL`; page presence alone therefore does not prove source
comments exist. Verify the selected warning/extraction policy with temporary
negative fixtures for an undocumented public declaration and an invalid
parameter reference. Both must fail the appropriate check. Keep semantic review
of units/ownership/effects alongside automated checks; tags alone cannot prove it.

Document commands with separate build directories, adapting the active compiler
and multi-config flags where necessary. The intended starting pattern is:

```sh
cmake -S . -B build/host -DDP8382X_BUILD_TESTS=ON
cmake --build build/host
ctest --test-dir build/host --output-on-failure
python scripts/generate_version.py check
python tools/check_doxygen.py
```

These are future commands to implement and verify in this stage, not commands
known to work in the current planning workspace. Add a separate tests-OFF
consumer outside the library build tree and prove it cannot accidentally link a
sibling or source-tree-only dependency. Do not require SDK, Python or Doxygen for
that consumer.

## Documentation in this stage

Document every new public type/helper/field in its header with useful Doxygen;
private source comments explain validation order and non-obvious bit rules.
Update root README/CHANGELOG, `test/README.md`, build/testing/API guidance and
tool usage with commands actually run and the support that now exists. Record
absent driver/SDK/hardware layers separately. Run source-documentation and link
checks locally; first-set remote CI still waits until prompt 29.

## Acceptance and handoff

Configure/build/run native tests and a separate tests-OFF consumer. Compile each
portable header alone, verify clean version generation/check and no SDK headers. Test
invalid metadata/version inputs where relevant. Existing reference/link and diff checks
remain active.

Expected output: Working portable build, passive contracts and reproducible metadata.
Include source documentation, relevant guides, feature/evidence matrix and progress
updates. Apply COMMON's completion rules; report actual commands, results and remaining
gates. Continue only when the concrete required inputs exist; COMMON explains how
unrelated gates stay open.

Before reporting completion or starting another prompt, commit and sync this
prompt's checked work on the recorded execution branch. Also checkpoint coherent
verified units during the prompt. Follow root AGENTS' first-set CI exception;
do not create empty commits or include unrelated work.
