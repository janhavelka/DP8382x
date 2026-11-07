# 38 - Complete SDK-aware configuration and raw MDIO arbitration

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md),
[ENGINEERING](ENGINEERING.md) and [VERIFICATION](VERIFICATION.md).
Prerequisites: [37](37-esp-idf-lifecycle.md), implemented typed
configuration/register procedures and the frozen single-owner matrix. Preserve
task 37's working lifecycle and linked integration fixture.

## Current evidence and ownership

Inspect current INA228/PCA9555/TCA9548A native ownership and lifetime seams,
including their production implementations and failure tests. Reopen the
pinned SDK `esp_eth_driver.h`, driver ioctl implementation, IEEE 802.3 base
and MAC register callbacks. Trace `ETH_CMD_S_AUTONEGO`, `ETH_CMD_G_AUTONEGO`,
`auto_nego_en`, raw-register ioctls and stopped-driver constraints. Record
exact revisions/paths and adopted, adapted or rejected conventions; peer I2C
locking is not evidence of complete PHY transaction ownership.

Assign paths and select subagent models under COMMON. Keep shared contracts
and Git with the lead; use independent review for ownership and failure
semantics.

## Source requirements and regression vectors

Use [SDK register/state traces](../reference/library-sdk.md) under
[EVIDENCE](EVIDENCE.md). Emit whole initialized MMD control/data values and
serialize all four frames. DEVAD 31 address/data selectors are exactly
`0x001F/0x401F`; validate 16-bit data and function enums before mutation.
`ETH_CMD_G_AUTONEGO` observes cached state; AN changes use stopped-driver
`ETH_CMD_S_AUTONEGO`. Define speed/duplex getter meaning from the pinned SDK:
a resolved-state cache is not necessarily immediate desired-setting readback.

## Implement

Implement full SDK-aware owner routing for configuration, indirect access and
raw register entry. Settings mirrored by the SDK, especially AN enable, use
supported stopped-driver controls so SDK cache and PHY remain consistent. Full
advertisement and chip-specific fields use existing shared procedures; do not
invent private ioctls or a second configuration state machine.

Raw BMCR mutations, resets and other actions that can change SDK-mirrored
state invalidate that relationship and hardware trust. Enforce public-control
reconciliation before restart, including an attempted direct framework start.
Reconciliation may itself mutate hardware and must obey maintenance admission,
finite deadlines and uncertain-write rules. A correctly locked write cannot
certify a stale `auto_nego_en` cache. Preserve exact legal forced/advertised
modes and general 10/100 support, keeping application policy separate.

Raw ioctls reach the mediator outside PHY methods. Both hooks reject
unconditionally with ESP_ERR_NOT_SUPPORTED, even on the owner task. The typed
owner raw route calls core validation and MAC frame methods inside one batch
scope. Test real ioctl zero-frame rejection, selector/alias protection,
collector ownership and no recursive locking.

Control ioctls obey application admission. Flow-control can mutate the MAC
before PHY advertisement fails, so retain both effects and reconcile explicitly;
there is no blanket zero-effect promise for arbitrary control ioctls.
set_addr returns OK for the current value only, otherwise NOT_SUPPORTED.
Changing address requires delete/recreate, without a per-PHY registry.

The pinned flow-control ioctl documents bool* but reads uint32_t* for PHY
advertisement. Use the supported owner path with separately correctly typed MAC
and PHY calls, preserving both results/effects. Test width/alignment and never
pass a small Boolean through the defective ioctl.

Apply one lock order across base operations, application, CLI and second-PHY
entrants. Distinguish long-operation admission from controller ownership,
never holding the controller across waits or future polls. Bound acquisition
and aggregate sequences with the SDK limitations established in task 37.
Preserve write uncertainty, partial configuration and primary/cleanup detail.

Run failures against prompt 37's hash-checked real esp_eth.c host target, then
link the P4 fixture. Header fakes alone are not SDK link evidence; host-driver
execution remains explicitly host evidence.

## Acceptance

Execute AN-to-forced configuration followed by stop/start, getter checks and
forced-mode admission as one scenario. Exercise actual raw ioctls, raw BMCR
reconciliation, every indirect boundary, contention, failed acquisition,
reentry and releases. Assert no illegal interleaving, duplicate lock or bypass
and no MDIO mutation on rejected raw admission. Check complete no-effect
rejection for local CLI validation separately from partly applied SDK control
failures. Rebuild the real P4 fixture and host consumer; SDK/header shims
prove only their stated host layer.

Test direct framework start after invalidation with cached AN both enabled and
disabled. Preserve any earlier FSM/event effects before adapter rejection and
verify the supported cleanup path.

Update adapter Doxygen, `docs/esp-idf.md`, configuration/ownership guides and
the feature ledger with actual entry points, state reconciliation and bounds.

Review README/CHANGELOG and affected guides. Run focused local and
source-documentation checks (Doxygen where applicable), then update
evidence/progress under COMMON. Commit/sync checked units and completion on
the execution branch; first-set remote CI belongs only to 71.
