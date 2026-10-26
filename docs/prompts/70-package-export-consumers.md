# 70 - Validate bounded archives and clean extracted consumers

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md) and
[VERIFICATION](VERIFICATION.md). Prerequisites:
[69](69-package-metadata.md), [68](68-portable-owner-consumer.md) and actual
native P4 consumer build inputs. Package/export evidence must use only exported
contents; source-tree or sibling fallbacks invalidate the claim.

## Current evidence and ownership

Reopen TCA9548A `tools/check_package.py` (`expected_files`, `check_archive`,
`build_consumer`) and finite archive/build bounds; compare SCD41
`tools/check_target_package_consumer.py::platformio_configuration/main` and
`tools/check_clean_consumer_compile.py::safe_extract`. Inspect their negative
tests, source membership, license notices and actual workflow registration.
Record revisions, dirty state, exact paths and adopted/adapted/rejected choices;
do not import I2C S2/S3 targets or another PlatformIO installation.

The lead owns package payload and checker integration. Delegate disjoint
archive-negative fixtures, such as `tools/test_check_package.py`, or read-only
license/content review with frozen allowlist and forbidden shared paths. Choose
available models by inventory/security/build complexity. Shared metadata and Git
remain lead-owned.

## Closed work unit

Implement/extend one small bounded `tools/check_package.py` for one archive format
actually emitted and one exact expected path/content allowlist. Avoid a generic
multi-format framework. Validate before extraction: measured compressed/expanded
bytes, member count, required implementation/content/notices and finite child
configure/build/run time. All writes/cleanup stay in a resolved temporary
directory; do not use unvalidated `extractall`.

Reject absolute, drive and UNC paths, backslashes/colons, empty/dot/dot-dot
segments, controls, trailing-dot/space aliases, duplicate/case-colliding members,
links/sparse/special files, unexpected directories and missing/changed required
source. Exercise truncation/malformed archives and every negative rule. An
archive error is failure, never warning/pass.

Exclude vendor/reference payloads and extracted text, build/firmware artifacts,
test fixtures/HIL logs and private paths as appropriate. Preserve actual reused
source notices and all sources/public headers required by advertised portable
and adapter consumers. Review payload/license membership explicitly.

Build an archive, validate/extract into a clean temporary directory, then
compile/link/run a renamed ordinary C++17 consumer with tests OFF using only
extracted public artifacts. No source-tree generated file, global header,
sibling, Git/Python/Doxygen configure dependency or fallback is allowed. Build/
link an actual native P4 example from extraction against the pinned SDK. Missing
SDK access is BLOCKED, never shim success. Test source archives without Git and
truthful unknown firmware identity. PlatformIO is conditional and uses the
existing managed Windows wrapper if needed, never a new Core installation.

## Acceptance and checkpoint

Run positive extraction consumers, all archive/member/content negatives, stale
version checks and child configure/build/run failure/timeout fixtures. Record
archive identity/hash, payload, notices, host execution and actual SDK link
separately; publication/tags remain outside this task.

Ship checker docstrings, `docs/packaging.md`, README installation/support,
CHANGELOG, testing/integration docs and CONTRIBUTING updates. Run relevant source
documentation/local checks, record exact commands/gates and commit/sync coherent
verified units and completion on the recorded branch. Remote CI stays at 71.
