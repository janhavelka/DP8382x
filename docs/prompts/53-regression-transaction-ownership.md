# 53 - Close cross-entry transaction and SDK ownership regressions

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md),
[ENGINEERING](ENGINEERING.md) and [VERIFICATION](VERIFICATION.md).
Prerequisites: implemented core register/ configuration/operation behavior,
[38](38-esp-idf-state-arbitration.md) and [47](47-cli-native-integration.md).
Earlier tasks already ship focused tests; this task closes cross-module gaps
and fixes confirmed defects.

## Current evidence and ownership

Inspect current peer MDIO-relevant fault-injection patterns, production
transaction functions, lock release and CLI negative tests; compare TMP1x2 and
24Cxx explicit test/source registration and native fixture boundaries. Reopen
the selected SDK raw ioctl, mediator/base and controller source plus exact TI
register/access sections. Record revisions, dirty state, paths and
adopted/adapted/rejected conventions. Inspect workflow commands as source
only.

Assign paths and select subagent models under COMMON. Keep shared contracts
and Git with the lead; use independent review for ownership and failure
semantics.

## Source requirements and regression vectors

Apply [core](../reference/library-ti-core.md) and
[SDK](../reference/library-sdk.md) evidence under [EVIDENCE](EVIDENCE.md).
Assert literal MMD selectors `0x001F/0x401F`, reserved bits zero, correct
reset register addresses and unchanged outputs on failed discovery/link
acquisition. Cover raw ioctls, SDK cached AN state and partial notifications
through actual adapter entry points. Derive vectors independently from exact
specifications.

## Implement

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

Attempt base, raw ioctl, CLI, application and second-PHY entry at every
selector/ data boundary. No path may interleave or bypass
arbitration/invalidation. Exercise callback reentry without deadlock,
install-time hook context, two adapter instances and unadmitted calls from the
current owner task. Cover raw selector/data-port access, side-effect reads,
PHY-address and flow-control setters, failed restart admission after raw/reset
invalidation, and partial MAC/PHY updates. Distinguish logical operations from
actual all-source MDIO frames. Zero cooperative budget performs no I/O; atomic
sequences cannot split merely to meet a per-frame budget.

Test the actual SDK raw-register route, not only a core fake. AN-to-forced
configuration followed by stop/start must keep `ETH_CMD_G_AUTONEGO`, mirrored
state and forced-mode admission coherent. Raw BMCR changes require documented
reconciliation. Preserve portable directional RX/TX pause evidence separately
from the SDK's narrower Boolean notification; correct locking is insufficient
for either semantic guarantee.

## Acceptance

Execute affected real core, CLI and meaningful adapter regressions using
independent vectors and explicit counts. Fix defects in owning modules with
focused tests rather than broad source regexes or weakened assertions. Rebuild
actual native SDK fixtures/images when changed paths require it and rebuild
the plain host consumer to detect boundary leakage. Report missing tools/SDK
as NOT RUN/BLOCKED, not shim success.

Update public/procedure Doxygen, API/ownership/testing guides and
`test/README.md` for confirmed fixes, including independently derived vectors
and test registration.

Review README/CHANGELOG and affected guides. Run focused local and
source-documentation checks (Doxygen where applicable), then update
evidence/progress under COMMON. Commit/sync checked units and completion on
the execution branch; first-set remote CI belongs only to 71.
