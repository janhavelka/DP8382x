# 70 - Validate bounded archives and clean extracted consumers

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md) and
[VERIFICATION](VERIFICATION.md). Prerequisites: [69](69-package-metadata.md),
[68](68-portable-owner-consumer.md) and actual native P4 consumer build inputs.
Package/export evidence must use only exported contents; source-tree or sibling
fallbacks invalidate the claim.

## Current evidence and ownership

Reopen TCA9548A `tools/check_package.py::expected_files/check_archive/build_consumer`
and SCD41 `tools/check_clean_consumer_compile.py::safe_extract`, their negative
tests, source membership and registration. Record revisions and selected rules.
PlatformIO is not supported in this execution; do not adopt SCD41's
`platformio_configuration` or create `scripts/pio.cmd` without an actual
PlatformIO stage.

The lead owns package payload and checker integration.

## Closed work unit

Implement/extend one small bounded `tools/check_package.py` for one archive
format actually emitted and one exact expected path/content allowlist. Avoid a
generic multi-format framework. Validate before extraction: measured
compressed/expanded bytes, member count, required implementation/content/notices
and finite child configure/build/run time and output. Read a bounded private
archive snapshot and validate and consume those same bytes, so an input
replacement cannot bypass the check. Bound actual decompressed bytes as well as
declared member sizes. All writes/cleanup stay in a resolved temporary
directory; do not use unvalidated `extractall`.

Reject absolute, drive and UNC paths, backslashes/colons, empty/dot/dot-dot
segments, controls, Windows reserved device names, trailing-dot/space aliases,
duplicate/case-colliding members, links/sparse/special files, unexpected
directories and missing/changed required source. Exercise truncation/malformed
archives and every negative rule. An archive error is failure, never
warning/pass.

Exclude vendor/reference payloads and extracted text, build/firmware artifacts,
test fixtures/HIL logs and private paths as appropriate. Preserve actual reused
source notices and all sources/public headers required by advertised portable
and adapter consumers. Review payload/license membership explicitly.

Build `git archive --format=tar.gz` of HEAD using 69's exact payload, validate
and extract it into a clean temporary directory. Run a renamed tests-OFF C++17
consumer using only exported artifacts; install, relocate and run a distinct
`find_package` consumer. No checkout/global/sibling/generated-file fallback or
Git/Python/Doxygen configure dependency is allowed. Link the native P4 example
from extraction with the pinned SDK; missing SDK is BLOCKED. Test missing-Git
archive identity as unknown. PlatformIO is not a supported consumer here.

## Acceptance and checkpoint

Run positive extraction consumers, all archive/member/content negatives, stale
version checks, relocated-install consumers and child configure/build/run
failure/timeout/output-limit fixtures. Verify timed-out children are terminated
and cleanup cannot escape the owned temporary tree. Record archive
identity/hash, payload, notices, host execution and actual SDK link separately;
publication/tags remain outside this task.

Update checker docstrings and packaging/install/testing guidance with real
commands and evidence limits. Complete COMMON's report and checkpoint;
publication and tags are separate scope.
