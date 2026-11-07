# 68 - Prove independent two-instance application consumption

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md) and
[VERIFICATION](VERIFICATION.md). Prerequisites: implemented primary software,
applicable exact-variant dispositions through task 67, and actual P4
applications from [47](47-cli-native-integration.md). Unavailable variants
remain explicit; unrelated missing hardware does not block this host
application.

## Current evidence and ownership

Reopen TCA9548A standalone consumption, TMP1x2's public-header consumer and
TMP1x2/24Cxx CMake tests-OFF/source selection. Inspect current
`FieldCore-node/src/devices/i2c/Ina228Module.h` and
`FieldCore-node/src/devices/i2c/Ina228Module.cpp`, borrowed
`I2cOwnerTransport`, `DeviceModuleStatus` and result contracts; compare
`mapLibraryStatus`, `mapJobEffect`, `snapshot` and `copyStatus`. Record
revisions, paths and adopted/adapted/rejected conventions. FieldCore remains
read-only and is not claimed to have existing Ethernet integration.

The lead owns public contract corrections and application mapping.

## Closed work unit

Create or extend `examples/portable/basic/` with a small CMake project,
`main.cpp` and README. Link the existing build-tree target `DP8382x` using only
its public usage requirements and actual implementation. Exercise public headers
without private, `src/`, sibling or test include paths. Installed/exported and
extraction-only consumption are completed in prompts 69-70. A scripted backend
is clearly local simulation, not MCU support.

Demonstrate two independent driver instances with distinct borrowed contexts,
caller-owned monotonic time and explicit shared-controller transaction locking
where applicable. Explain lifetime through terminal callbacks and cleanup.
Initialize/configure one instance, progress cooperative work with finite owner
budgets, retain/take results and render passive health/link/config evidence. Do
not hide a blocking convenience loop behind a poll name.

Reuse `DirectRead.h` and `MmdAccess.h` in the two-instance owner application,
exiting nonzero on a failed demonstration. Keep `test/consumer` as the link
check. Registered CTest suites own independence, zero-frame, sentinel and
failure assertions; do not turn the example into another assertion suite.
Demonstrate cancellation, separate recovery admission, accepted-write uncertainty
and result/resource lifetime with the OP rules, without automatic reset/replay.

Write `docs/firmware-integration.md` with a concrete mapping table for
`Status::{code,detail,msg}`, operation progress, transport health, configuration
trust, link validity/resolution and application readiness. Static message text
is not a stable machine code. Explain FieldCore-like ownership without importing
firmware types, task topology, sensor policy or a generic backend registry.

## Acceptance and checkpoint

Configure/link/run the clean tests-OFF consumer without Git/Python/Doxygen/SDK
or siblings. Locally run UBSan with
`-fsanitize=undefined -fsanitize-trap=undefined`; record second compiler and
other sanitizers NOT RUN locally, to run in Linux at 71 including the tests-OFF
consumer. Do not install toolchains for this. Rebuild both real P4 examples and
inspect source selection. Additional MCU adapters need an actual supported
backend; extraction evidence belongs to 70.

Update helper Doxygen, portable README and integration guidance with the actual
consumer contract. Record host execution and real SDK links separately, then
complete COMMON's report and checkpoint.
