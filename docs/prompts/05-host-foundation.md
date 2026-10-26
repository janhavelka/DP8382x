# 05 - Build the portable foundation and version tooling

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-04.

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
