# 37 - Implement the native ESP-IDF factory and lifecycle

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md), [ENGINEERING](ENGINEERING.md)
and [VERIFICATION](VERIFICATION.md). Prerequisites:
[6](6-framework-ownership.md), [8](8-operation-contracts.md) and the implemented
DP83826I procedures reconciled in [36](36-primary-feature-closure.md), including
transport, initialization, reset, configuration and link. Record concrete inputs.

## Current evidence and ownership

Reopen current INA228/PCA9555/TCA9548A native component lifecycle, cleanup and
error mapping; compare `TMP1x2/examples/esp_idf/basic/main/main.cpp` (`mapError`,
`finish`, `initializeConsole`) and PCA9555's `mapI2c`/`ensureDevice`. Inspect the
pinned Espressif `esp_eth_phy_t`, IEEE 802.3 base, mediator, driver and MAC source,
including actual controller waits. Record revisions, paths, dirty state and
adopted/adapted/rejected conventions. I2C categories and S2/S3 examples do not
establish Ethernet behavior.

The lead owns platform factory signatures, shared contracts and integration.
Delegate disjoint fault cases or read-only lifetime review with exact paths;
choose available models by complexity, using stronger reasoning for callbacks,
locks and ownership. Reviewers do not edit shared headers or Git.

## Concrete public-source inputs

Apply [the SDK source facts](../reference/library-sdk.md) and
[EVIDENCE](EVIDENCE.md). Generic defaults use GPIO 5, address -1, reset 100 ms,
AN 4000 ms and assertion 100 us; the base maps default post-reset delay to -1
(no wait). Supply actual board choices and enforce the chip's 2 ms reset-release
and 50 ms POR SMI readiness. Auto-discovery can try 96 reads and ignores read
errors; no stale ID may select a device. Test and correct these paths before
calling lifecycle integration complete. They are observed SDK choices, not TI
silicon defaults or measured limits.

## Closed work unit

Implement a real native factory in `include/DP8382x/esp_idf/Phy.h` and
`src/esp_idf/Phy.cpp`, unless the frozen API selects equivalent platform paths.
Use one adapter and existing core chip procedures. Enumerate every pinned PHY
and selected base method: owner, backend calls, transaction boundary, side
effects, notification, error and finite bound. Resolve new SDK methods explicitly;
do not ship null callbacks, successful no-ops or unfinished-software claims of
unsupported silicon.
Audit base address selection/discovery and reset semantics before reuse. Bind
only the caller-selected legal PHY address; any explicit discovery is bounded,
owner-routed and cannot silently select or configure a device.

Deliver correct binding, initialization, reset, link observation, start/stop,
deinit and delete behavior now. Reuse audited base methods only where semantics
agree. Establish complete-transaction serialization and lock order for actual
base-to-mediator calls without recursive locking. Release controller ownership
before waits. Borrowed contexts outlive every callback; SDK allocation remains
outside the allocation-free core. Separate bus-silent release from explicit
fallible power/reset actions, preserving primary and cleanup failures.

Basic notifications and controller bounds must already be correct. Do not defer
working callback behavior to task 39. Avoid duplicate link events and invented
speed/duplex on down or unresolved links. Audit actual finite controller/lock
limits; a core deadline cannot preempt a blocking SDK call. Task 38 expands
SDK-aware configuration/raw entry, while task 39 deepens conformance evidence.
Until task 38 completes those routes, reject entries that cannot preserve owner
and SDK state before I/O at the actual mediator/ioctl boundary. Caller restraint
alone is insufficient; unfinished software is not unsupported silicon.

Add exact component metadata/source selection and a small real P4 firmware
fixture, such as `test/esp_idf/adapter_link/main/main.cpp`, instantiating actual
MAC/PHY framework dependencies. The portable source list excludes SDK files.

## Acceptance and checkpoint

Inject factory/resource, mediator, initialization, callback and notification
failures; verify unwinding, unchanged outputs, retained backend detail and no
released-context access. Build/link the real pinned SDK fixture and rebuild the
plain host consumer. A shim cannot replace missing SDK evidence.

Ship factory/lifetime/bounds Doxygen, `docs/esp-idf.md`, ownership-table updates
and exact build commands. Review README, CHANGELOG and affected guides; document
no-change reasons where appropriate. Run source-documentation and focused local
checks. Record evidence layers and gates separately. Commit/sync checked work
and coherent verified units on the recorded execution branch under COMMON;
remote CI remains reserved for task 71.
