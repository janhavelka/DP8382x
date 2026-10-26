# 38 - Complete SDK-aware configuration and raw MDIO arbitration

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md), [ENGINEERING](ENGINEERING.md)
and [VERIFICATION](VERIFICATION.md). Prerequisites: [37](37-esp-idf-lifecycle.md),
implemented typed configuration/register procedures and the frozen single-owner
matrix. Preserve task 37's working lifecycle and linked integration fixture.

## Current evidence and ownership

Inspect current INA228/PCA9555/TCA9548A native ownership and lifetime seams,
including their production implementations and failure tests. Reopen the pinned
SDK `esp_eth_driver.h`, driver ioctl implementation, IEEE 802.3 base and MAC
register callbacks. Trace `ETH_CMD_S_AUTONEGO`, `ETH_CMD_G_AUTONEGO`,
`auto_nego_en`, raw-register ioctls and stopped-driver constraints. Record exact
revisions/paths and adopted, adapted or rejected conventions; peer I2C locking
is not evidence of complete PHY transaction ownership.

The lead owns adapter/core configuration contracts and shared source changes.
Assign disjoint failure tests or a read-only bypass audit after entry points are
frozen. Select available models and reasoning for concurrency complexity;
reviewers have no shared-header, workflow or Git ownership.

## Concrete public-source inputs

Reopen [SDK register/state traces](../reference/library-sdk.md) under
[EVIDENCE](EVIDENCE.md). MMD helpers emit four separately mediated frames and
leave nine reserved control bits uninitialized. Require exact address/data
selector words, such as `0x001F/0x401F`, through the real adapter. Raw ioctls
call mediator hooks directly; `ETH_CMD_G_AUTONEGO` reads the SDK cache while
`ETH_CMD_S_AUTONEGO` updates it only with the driver stopped. Verify the actual
path and error results; neither a per-frame lock nor correct register readback
alone proves SDK state consistency.

## Closed work unit

Implement full SDK-aware owner routing for configuration, indirect access and
raw register entry. Settings mirrored by the SDK, especially AN enable, use
supported stopped-driver controls so SDK cache and PHY remain consistent.
Full advertisement and chip-specific fields use existing shared procedures;
do not invent private ioctls or a second configuration state machine.

Raw BMCR mutations invalidate the SDK relationship and hardware trust. Require
public-control reconciliation before restart. A correctly locked write cannot
certify a stale `auto_nego_en` cache. Preserve exact legal forced/advertised
modes and general 10/100 support, keeping application policy separate.

Standard raw ioctls reach the mediator outside PHY methods. Enforce arbitration
and invalidation through supported `esp_eth_config_t` custom register callbacks,
or explicitly prohibit bypass calls and supply a documented owner-routed path.
Test the actual selected SDK entry, not only core fakes. The per-frame
`read_phy_reg`/`write_phy_reg` hooks forward to MAC methods without recursive
ioctl/mediator calls. Whole MMD and required RMW ownership surrounds them;
per-frame locking alone is insufficient.

Apply one lock order across base operations, application, CLI and second-PHY
entrants. Distinguish long-operation admission from controller ownership, never
holding the controller across waits or future polls. Bound acquisition and
aggregate sequences with the SDK limitations established in task 37. Preserve
write uncertainty, partial configuration and primary/cleanup detail.

## Acceptance and checkpoint

Execute AN-to-forced configuration followed by stop/start, getter checks and
forced-mode admission as one scenario. Exercise actual raw ioctls, raw BMCR
reconciliation, every indirect boundary, contention, failed acquisition, reentry
and releases. Assert no illegal interleaving, duplicate lock or bypass and no
mutation on rejected admission. Rebuild the real P4 fixture and host consumer;
SDK/header shims prove only their stated host layer.

Update adapter Doxygen, `docs/esp-idf.md`, configuration/ownership guides and
the feature/evidence ledger with entry points, lifetimes and actual bounds.
Review README/CHANGELOG and affected guides, run documentation and targeted
local checks, then record results and open gates. Commit/sync every completed
task and coherent checked unit on the same execution branch. Only task 71
dispatches or checks remote CI during this execution.
