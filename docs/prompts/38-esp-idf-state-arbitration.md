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

Standard raw ioctls reach the mediator outside PHY methods. Enforce
arbitration and invalidation through supported `esp_eth_config_t` custom
register callbacks, with an executable rejection path for unadmitted access
and a documented owner-routed raw API. A documented caller prohibition alone
is insufficient. Validate direct read/write address and value widths,
including C22 `0x0D/0x0E`; selector/data registers cannot be used to bypass
complete MMD ownership. Route latch/read-clear reads through the collector.
Test actual SDK raw ioctls, including calls from the current owner task
without transaction admission. The per-frame `read_phy_reg`/`write_phy_reg`
hooks forward to MAC methods without recursive ioctl/mediator calls. Whole MMD
and required RMW ownership surrounds them; per-frame locking alone is
insufficient.

Cover public setters beyond raw BMCR: PHY address, loopback and flow-control
controls must obey the same admission and invalidation policy. Flow-control
control can change the MAC before PHY advertisement fails; retain both effects
and require reconciliation. Expose these controls through the application
owner and document the contract for any exposed raw SDK handle. A PHY callback
cannot prevent a preceding MAC-side mutation; do not promise blanket
zero-effect rejection for arbitrary control ioctls. Keep this limitation
separate from the raw-register hook's enforceable transfer admission. Guard
PHY-address changes against pending operations and another instance's address;
do not silently retarget borrowed core state.

Audit the flow-control argument contract: this pinned SDK documents `bool*`
but reads the same data as `uint32_t*` for PHY advertisement. Use a verified SDK
correction or the supported owner path with correctly typed MAC and PHY method
calls; preserve both operations' effects and errors. Test argument width/alignment
and avoid passing a small Boolean object through the defective path.

Apply one lock order across base operations, application, CLI and second-PHY
entrants. Distinguish long-operation admission from controller ownership,
never holding the controller across waits or future polls. Bound acquisition
and aggregate sequences with the SDK limitations established in task 37.
Preserve write uncertainty, partial configuration and primary/cleanup detail.

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
