# 68 - Prove independent two-instance application consumption

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md) and
[VERIFICATION](VERIFICATION.md). Prerequisites: implemented primary software,
exact variant dispositions through task 67, and actual P4 applications from
[47](47-cli-native-integration.md). Unavailable variants remain explicit;
unrelated missing hardware does not block this host application.

## Current evidence and ownership

Reopen TCA9548A standalone consumption, TMP1x2's public-header consumer and
TMP1x2/24Cxx CMake tests-OFF/source selection. Inspect current
`FieldCore-node/src/devices/i2c/Ina228Module.h/.cpp`, borrowed
`I2cOwnerTransport`, `DeviceModuleStatus` and result contracts; compare
`mapLibraryStatus`, `mapJobEffect`, `snapshot` and `copyStatus`. Record revisions,
paths and adopted/adapted/rejected conventions. FieldCore remains read-only and
is not claimed to have existing Ethernet integration.

The lead owns public contract corrections and application mapping. Delegate a
disjoint consumer or read-only validity/mapping review after APIs freeze; choose
available models by coding/state reasoning complexity. Shared API, architecture,
progress and Git remain lead-owned.

## Closed work unit

Create or extend `examples/portable/basic/` with a small CMake project,
`main.cpp` and README. Link target `DP8382x` using only public usage requirements,
installed/exported headers and actual implementation. Do not include private
headers, `src/`, sibling/test paths or source-tree fallback. A scripted backend
is clearly local simulation, not MCU support.

Demonstrate two independent driver instances with distinct borrowed contexts,
caller-owned monotonic time and explicit shared-controller transaction locking
where applicable. Explain lifetime through terminal callbacks and cleanup.
Initialize/configure one instance, progress cooperative work with finite owner
budgets, retain/take results and render passive health/link/config evidence.
Do not hide a blocking convenience loop behind a poll name.

Show generic legal 10/100 policy versus a separate 100/full application test,
cancellation/recovery admission, partial write/cleanup uncertainty and no silent
replay/reset. Inject one instance's backend failure and prove independent
status/detail/counters/trust on the other. Cached rendering has zero frame delta;
failed reads preserve caller output. Release results/resources in order and
reject rebinding/destruction while owned work still borrows context.

Write `docs/firmware-integration.md` with a concrete mapping table for
`Status::{code,detail,msg}`, operation progress, transport health, configuration
trust, link validity/resolution and application readiness. Static message text is
not a stable machine code. Explain FieldCore-like ownership without importing
firmware types, task topology, sensor policy or a generic backend registry.

## Acceptance and checkpoint

Configure/link/run a clean tests-OFF consumer without Git/Python/Doxygen/SDK or
sibling dependencies; repeat with a second available compiler. Rebuild both real
P4 examples and inspect portable/platform source selection. Additional Arduino/
MCU adapters require an actual supported MAC/MDIO backend and separate evidence;
document precise unimplemented scope instead of advertising fictitious targets.
Package extraction belongs to task 70, not this evidence claim.

Ship helper Doxygen, portable README, integration guide, root README and
CHANGELOG updates. Review affected docs, run documentation/focused local checks,
record independent host/SDK/physical layers, and commit/sync checked units and
completion on the recorded branch. Remote CI remains exclusive to task 71.
