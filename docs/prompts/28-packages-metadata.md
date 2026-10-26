# 28 - Validate packages, metadata and ownership

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-27.

At entry apply CONVENTIONS' names, Status/health, layout and simplicity rules;
reopen current relevant sibling sources. Keep source Doxygen and affected
README/CHANGELOG/guides current, verify locally and checkpoint/sync on this
execution's recorded branch.

## Fresh audit and owned work

Inspect TCA9548A/SCD41 clean package consumers and archive safety tests, TMP1x2/MB85RC
version tools, current repo license/remote and actual maintainers.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Work

Finalize package metadata for the portable library and selected ESP-IDF adapter.
Preserve one release version source and deterministic generation. Express only
tested/justified framework/SDK constraints and actual supported part scope. No invented
URL, maintainer, CODEOWNERS account, registry version or publication.

Create a bounded package checker with explicit allowed contents, path/symlink/duplicate
protections and required license notices. Exclude docs/reference vendor payloads and
extracted text, firmware/build artifacts, test fixtures/HIL logs and private local paths
from firmware/library distribution as appropriate. Preserve notices for source actually
reused.

Build packages locally, extract into a clean temporary directory and compile/link an
ordinary consumer and real SDK example from exported contents. Do not fall back to
source-tree files, Git/sibling includes, generated files from a previous build or
globally installed headers. Include necessary adapter sources and supported public
paths.

Test source archives without Git and tests-OFF consumption without Python/Doxygen. Keep
example build identity unknown when its source provenance is unavailable rather than
adopting the parent checkout's SHA. Document dependency resolution and minimum
toolchains. Update CONTRIBUTING and ownership guidance with facts.

## Concrete export contract and negative cases

Reopen `../TMP1x2/scripts/generate_version.py` (`_expected_outputs`,
`_sync_outputs`, `main`) and `tools/test_version_sync.py::VersionSyncTests`;
`../TCA9548A/tools/check_package.py` (`expected_files`, `check_archive`,
`build_consumer`) and its finite archive/build bounds; and
`../SCD41/tools/check_target_package_consumer.py::platformio_configuration/main`
with `tools/check_clean_consumer_compile.py::safe_extract`. Compare
`../MB85RC/tools/check_metadata_consistency.py`, package metadata, current
AGENTS/CONTRIBUTING and build registration. Record revisions/dirty state and
adopted/adapted/rejected behavior. Do not copy sibling maintainer names,
S2/S3 firmware matrices, compatibility aliases or their release version.

The lead owns metadata/source membership and version truth. Delegate disjoint
archive-negative fixtures or read-only license/content review; assign exact
files, available models suited to complexity and forbidden shared files. Tests
can own `tools/test_check_package.py` while the lead integrates
`tools/check_package.py`. Prefer extending an existing checker/test over a
second packaging layer. Apply CONVENTIONS' small tools and plain diagnostics.

Write the durable export rules in `docs/packaging.md`: artifact formats, source
root layout, allowed files/notices, portable versus adapter sources, dependency
constraints, build commands and evidence limits. Keep explicit source lists in
CMake/component metadata synchronized with actual required code. Test that the
package contains every implementation linked by advertised consumers and no
vendor archive/source, build output or fixture log. Source evidence is not a
redistributable driver dependency.

Extend the existing `scripts/generate_version.py` and its isolated fixture suite
to cover `library.json`, generated `Version.h`, CMake/component and Doxygen
version fields actually present. `sync` is idempotent, `check` detects stale
outputs without mutation, invalid input fails clearly, and an archive consumer
does not invoke the generator. Release/version changes are intentional decisions,
not a bump per prompt. Keep example firmware SHA/dirty/SDK identity separate;
an extracted archive must not inherit the parent checkout's Git commit.

Select one archive format actually emitted by the package command and one exact
expected path/content allowlist. Validate that package; do not build a generic
multi-format archive framework. Reuse standard-library parsing with explicit
containment and the checks below.

In `tools/check_package.py`, validate before extraction. Set compressed bytes,
member count, expanded bytes and subprocess time bounds appropriate to measured
payload size. Reject absolute/drive/UNC paths, backslashes/colons, `.`/`..`/empty
segments, control characters, trailing-dot/space aliases, duplicate/case-colliding
members, links/sparse/special files, unexpected directories and missing/changed
required source. Restrict writes/cleanup to a resolved temporary work directory.
Do not use unvalidated `extractall` or convert archive errors to a warning/pass.

Exercise positive and negative fixtures for each rule, truncation, malformed
metadata, absent required implementation, stale generated version and child
configure/build/run failure or timeout. Compile/link/run a renamed clean C++17
consumer using only extracted contents with tests OFF and no Git/Python/Doxygen
or sibling include requirement. Link the actual native IDF P4 example from the
extraction with pinned SDK dependencies. Missing SDK access is BLOCKED, never
replaced by a shim/header-only success. PlatformIO is conditional; on Windows
use the managed wrapper if needed, never install another Core.

Update checker Python docstrings and public/generated-header Doxygen where
metadata contracts change. Update README install/support scope, CHANGELOG
Unreleased, `docs/packaging.md`, integration/testing docs and CONTRIBUTING with
executed commands and package identity/hash. Record artifact payload, license
review, host versus SDK results and gaps in
`docs/reports/implementation/28-packages-metadata.md`, validation and progress.
Run affected local tests/guards and extraction consumers; remote CI stays at 29.

## Acceptance and handoff

Run positive package consumption and negative archive/member/content tests, version-sync
check and generated-file drift checks. Inspect package payload and licenses. Native/SDK
packages must link from extraction. Publication and tags are outside this stage.

Expected output: Reproducible consumable packages and accurate project metadata. Include
source documentation, relevant guides, feature/evidence matrix and progress updates.
Apply COMMON's completion rules; report actual commands, results and remaining gates.
Continue only when the concrete required inputs exist; COMMON explains how unrelated
gates stay open.

Before reporting completion or starting another prompt, commit and sync this
prompt's checked work on the recorded execution branch. Also checkpoint coherent
verified units during the prompt. Follow root AGENTS' first-set CI exception;
do not create empty commits or include unrelated work.
