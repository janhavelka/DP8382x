# 69 - Finalize deterministic version and supported-package metadata

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md),
[ENGINEERING](ENGINEERING.md) and [VERIFICATION](VERIFICATION.md).
Prerequisites: [10](10-version-tooling.md), [68](68-portable-owner-consumer.md),
actual adapter metadata and supported exact-part decisions. Preserve one
existing version source and generator rather than introducing another metadata
layer.

## Current evidence and ownership

Reopen `TMP1x2/scripts/generate_version.py` (`_expected_outputs`,
`_sync_outputs`, `main`) and `tools/test_version_sync.py::VersionSyncTests`;
inspect MB85RC's metadata consistency tool, manifests, source lists, CMake
install/export rules where present and current agent/contributor rules. Review
this repository's actual license, remote and maintainer facts. Record revisions,
paths and adopted/adapted/rejected behavior. Do not copy sibling company names,
CODEOWNERS accounts, release versions or untested SDK ranges.

The lead owns version truth, source membership and support claims.

## Closed work unit

Finalize portable and selected ESP-IDF package metadata with `library.json` as
release version truth. Extend the existing `scripts/generate_version.py` to
regenerate `Version.h` and applicable CMake/component/Doxygen version fields
actually present. Do not hand-edit generated versions, bump a version per task,
or create manifests for hypothetical support.

`sync` is deterministic and idempotent; `check` detects stale output without
mutation. Invalid/missing metadata fails clearly. Ordinary source/archive
consumers receive required generated artifacts and do not invoke
Python/Git/Doxygen merely to configure/link. Keep firmware source
SHA/dirty/SDK/build identity separate from release version; a source archive
without provenance reports unknown instead of inheriting a parent checkout
identity.

Declare accurate tested/minimum justified SDK constraints, actual required
dependencies and supported exact variants/interfaces. No broad guessed semver,
registry publication claim, invented repository URL or maintainer. Explicit
source selection must include necessary implementation and isolate SDK sources
from portable compilation. Complete a real CMake install/export path for the
portable public target, headers and generated version. Its package configuration
must be relocatable, with no checkout, sibling or build-directory paths in
public usage requirements. Use the existing public target contract; select and
document any required package/import name once. Keep IDF component consumption
separate. Review MIT project ownership and retained notices for source actually
reused; GPL reference implementations are not MIT source.

Start/finalize `docs/packaging.md` metadata and dependency sections with
artifact formats/layout expected by task 70, toolchain commands, version
generation and identity limits. Update CONTRIBUTING/ownership guidance using
established facts. Vendor archives and extracted reference text are evidence
rather than build or redistributable driver dependencies. Actual archive
safety/extracted-consumer execution remains the next task, not a completed claim
here.

## Acceptance and checkpoint

Run isolated positive/negative generator fixtures for every actual output,
malformed metadata, stale generated fields, nonmutating check, repeated sync and
missing-Git archive identity. Test the real standalone CLI entry point and
import path so `check` cannot run a later implicit `sync`; import performs no
writes or command execution. Reject unsupported arguments and validate version
type/range instead of coercing malformed values. Run metadata/source membership
and generated-drift checks, plus configure/install/export smoke checks. Rebuild
affected ordinary/SDK consumers if metadata changes their inputs; record missing
tools separately and do not substitute a successful shim.

Update generator docstrings, public/generated Doxygen and
packaging/integration/contributor guidance for actual metadata and
install/export contracts. Complete COMMON's report and checkpoint; tags and
publication are separate scope.
