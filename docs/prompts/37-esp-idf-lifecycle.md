# 37 - Implement the native ESP-IDF factory and lifecycle

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md),
[ENGINEERING](ENGINEERING.md) and [VERIFICATION](VERIFICATION.md).
Prerequisites: [6](6-framework-ownership.md), [8](8-operation-contracts.md)
and the implemented DP83826I procedures reconciled in
[36](36-primary-feature-closure.md), including transport, initialization,
reset, configuration and link. Record concrete inputs.

Prompt 6's [accepted ownership design](../design-decisions.md) selects one
terminal batch route, guarded native callbacks and a required SDK lifecycle
correction. Implement and pin that correction (or audit an upstream revision
that satisfies it) before claiming support: checked install/partial unwind,
cleanup-only ownership on failed unwind, finite timer quiescence, retryable
teardown, supported mediator detach and MAC/HAL error propagation. A wrapper
counter alone cannot fence the timer's pre-wrapper callback window. Preserve
the original baseline and record the exact corrected-source identity.

## Current evidence and ownership

Reopen current INA228/PCA9555/TCA9548A native component lifecycle, cleanup and
error mapping; compare `TMP1x2/examples/esp_idf/basic/main/main.cpp`
(`mapError`, `finish`, `initializeConsole`) and PCA9555's
`mapI2c`/`ensureDevice`. Inspect the pinned Espressif `esp_eth_phy_t`, IEEE
802.3 base, mediator, driver and MAC source, including actual controller
waits. Record revisions, paths, dirty state and adopted/adapted/rejected
conventions. I2C categories and S2/S3 examples do not establish Ethernet
behavior.

Assign paths and select subagent models under COMMON. Keep shared contracts
and Git with the lead; use independent review for ownership and failure
semantics.

## Source requirements and regression vectors

Apply [SDK lifecycle/readiness evidence](../reference/library-sdk.md) and
[EVIDENCE](EVIDENCE.md). Select board address/reset wiring explicitly: generic
SDK defaults are address -1, GPIO 5, reset timeout 100 ms, AN timeout 4000 ms,
assertion 100 us and no default post-reset wait. Enforce the exact chip's 2 ms
reset-release and 50 ms POR SMI readiness, with clock conditions and real
elapsed time. Treat SDK defaults as policies, and TI readiness as silicon
requirements. A declared address is the default; explicit discovery validates
each read status before using IDs and has a total bound (the reference search
can attempt 96 reads).

## Implement

Implement a real native factory in `include/DP8382x/esp_idf/Phy.h` and
`src/esp_idf/Phy.cpp`, unless the frozen API selects equivalent platform
paths. Use one adapter and existing core chip procedures. Enumerate every
pinned PHY and selected base method: owner, backend calls, transaction
boundary, side effects, notification, error and finite bound. Classify
required methods and optional SDK hooks explicitly. Implement every required
method; leave an optional hook absent only when the pinned API permits it and
its real capability disposition is documented. Audit base address
selection/discovery and reset semantics before reuse. Bind only the
caller-selected legal PHY address; any explicit discovery is bounded,
owner-routed and cannot silently select or configure a device.

Deliver correct binding, initialization, reset, link observation,
deinit/delete and integration with framework start/stop now. Separate PHY
methods from framework lifecycle functions; driver installation may perform
MDIO/reset even though portable bind is bus-silent. Reuse audited base methods
only where semantics agree. Establish complete-transaction serialization and
lock order for actual base-to-mediator calls without recursive locking.
Release controller ownership before waits. Define lock order for framework
timer, owner requests, mediator and notifications. Stop/drain never waits
while holding a lock needed by the callback being drained. Borrowed contexts
outlive every callback; SDK allocation remains outside the allocation-free
core. Separate bus-silent release from explicit fallible power/reset actions,
preserving primary and cleanup failures.

Basic notifications and controller bounds must already be correct. Do not
defer working callback behavior to task 39. Avoid duplicate link events and
invented speed/duplex on down or unresolved links. Audit actual finite
controller/lock limits; a core deadline cannot preempt a blocking SDK call.
Task 38 expands SDK-aware configuration/raw entry, while task 39 deepens
conformance evidence. Expose implemented owner routes and enforce rejection of
unadmitted raw MDIO at the mediator boundary. Keep control ioctls application-
owned and record their partial-effect limits as specified in task 38. Maintain
explicit availability for routes introduced by later stages.

Resolve custom register-hook context during installation, before the output
driver handle is published, and during failed-install cleanup. Hooks receive
an Ethernet handle rather than a user-context argument. Use public
`esp_eth_get_phy_instance` / `esp_eth_get_mac_instance` on that incoming handle
to obtain the owned adapter context and actual MAC; the pinned driver assigns
both before initialization. Verify the selected SDK's order, failed-install
lifetime and two independent instances.

Trace partial framework failures: start/stop state changes, timer creation or
removal, event publication and deinit can precede a reported error. Record
which resources and callbacks remain owned; preserve them until safely
drained. Select a tested cleanup path for each failure rather than blindly
retrying uninstall or freeing MAC/PHY after a failed stop. Keep any SDK
limitation as an open gate.

The pinned install path ignores `set_mediator`/`reset_hw` return values, then
initializes MAC before PHY. A later PHY rejection cannot prevent those MAC
effects or detect MAC mediator errors by itself. Require verified error
propagation or a source-proved owner preflight path making that order safe;
use a verified SDK correction when the selected APIs cannot provide it. The
error path attempts timer deletion without checking its result and frees the
driver without MAC/PHY deinit. Define cleanup for stale mediator pointers and
partially initialized resources, preserving failed cleanup evidence. Test the
actual setup/reset failures and pre-MAC admission as well as unwind.

Add exact component metadata/source selection and a small real P4 firmware
fixture, such as `test/esp_idf/adapter_link/main/main.cpp`, instantiating
actual MAC/PHY framework dependencies. The portable source list excludes SDK
files.

## Acceptance

Inject factory/resource, mediator, initialization, callback and notification
failures; verify unwinding, unchanged outputs, retained backend detail and no
released-context access. Build/link the real pinned SDK fixture and rebuild
the plain host consumer. A shim cannot replace missing SDK evidence.

Document factory callbacks, lifetime, errors and bounds in Doxygen and
`docs/esp-idf.md`; update the operation-ownership table and component build
guide.

Review README/CHANGELOG and affected guides. Run focused local and
source-documentation checks (Doxygen where applicable), then update
evidence/progress under COMMON. Commit/sync checked units and completion on
the execution branch; first-set remote CI belongs only to 71.
