# 53 - Close cross-entry transaction and SDK ownership regressions

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md), [ENGINEERING](ENGINEERING.md)
and [VERIFICATION](VERIFICATION.md). Prerequisites: implemented core register/
configuration/operation behavior, [38](38-esp-idf-state-arbitration.md) and
[47](47-cli-native-integration.md). Earlier tasks already ship focused tests;
this task closes cross-module gaps and fixes confirmed defects.

## Current evidence and ownership

Inspect current peer MDIO-relevant fault-injection patterns, production
transaction functions, lock release and CLI negative tests; compare TMP1x2 and
24Cxx explicit test/source registration and native fixture boundaries. Reopen
the selected SDK raw ioctl, mediator/base and controller source plus exact TI
register/access sections. Record revisions, dirty state, paths and
adopted/adapted/rejected conventions. Inspect workflow commands as source only.

The lead owns the cross-entry coverage table and module fixes. Delegate
independent read-only protocol/ownership reviews or disjoint test files after
fixture contracts freeze. Choose available models with sufficient reasoning
for atomicity, silicon semantics and concurrency; shared API/integration and
Git remain with the lead. Do not race shared build directories.

## Closed work unit

Build a regression matrix by public operation/family: independently derived
literal source vector, success ordering, every failure boundary, output/state
invariant, owner/frame budget, actual target and remaining silicon/SDK/fixture
gate. Keep production constants, generated coverage metadata and independent
expected values separate; self-consistency is not a silicon oracle.

Fail every constituent indirect frame in turn, including address/data writes
accepted before timeout. Preserve caller read output until valid completion,
uncertain writes, partial configuration and original backend detail. No hidden
retry or later successful read may manufacture trust. Test complete MMD/RMW
ownership, failed acquisition, release on every acquired exit and no release
of locks never acquired.

Attempt base, raw ioctl, CLI, application and second-PHY entry at every selector/
data boundary. No path may interleave or bypass arbitration/invalidation.
Exercise callback reentry without deadlock and distinguish logical operations
from actual all-source MDIO frames. Zero cooperative budget performs no I/O;
atomic sequences cannot split merely to meet a per-frame budget.

Test the actual SDK raw-register route, not only a core fake. AN-to-forced
configuration followed by stop/start must keep `ETH_CMD_G_AUTONEGO`, mirrored
state and forced-mode admission coherent. Raw BMCR changes require documented
reconciliation. Preserve portable directional RX/TX pause evidence separately
from the SDK's narrower Boolean notification; correct locking is insufficient
for either semantic guarantee.

## Acceptance and checkpoint

Execute affected real core, CLI and meaningful adapter regressions using
independent vectors and explicit counts. Fix defects in owning modules with
focused tests rather than broad source regexes or weakened assertions. Rebuild
actual native SDK fixtures/images when changed paths require it and rebuild
the plain host consumer to detect boundary leakage. Report missing tools/SDK
as NOT RUN/BLOCKED, not shim success.

Ship Doxygen clarifications and update API/ownership/testing guides,
`test/README.md`, README and CHANGELOG for actual fixes. Review affected guides,
run documentation/local checks, record remaining gates, and commit/sync coherent
verified units and completion on the recorded execution branch. Remote CI is
reserved for task 71; sibling remote runs are not inspected here.
