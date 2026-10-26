# 7 - Freeze value, Status and direct transport contracts

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md),
[ENGINEERING](ENGINEERING.md) and [FULL_COVERAGE](FULL_COVERAGE.md).
Prerequisite: [6](6-framework-ownership.md)'s selected owner/transport design
and researched feature records. Record the rule reminder, owned contracts,
current source checks, documentation and same-branch checkpoint.

## Complete outcome

Write the exact foundational values and direct-call declarations in
`docs/api-contracts.md`, including Doxygen-style behavioral contracts and
plain-owner/SDK design examples. Prompt 8 owns cooperative lifetimes. Do not
ship declarations for future methods or force unknown feature signatures into
headers merely to finish this design task.

## Concrete public-source inputs

Use [the core source audit](../reference/library-ti-core.md) and
[EVIDENCE](EVIDENCE.md) to specify output validity from actual failure cases.
Require a live link query to assign a valid result on every success, including
link-down, and to leave ordinary output unchanged on failure. Include a low
first BMSR sample and a failing second sample in the contract examples. Select
revision acceptance from exact-part evidence, separately from default ID values. Keep declared
variant, observed identity, source acceptance and physical evidence distinct;
never fill an unknown status or reset field with a plausible default.

## Work and live convention audit

Reopen TMP1x2/PCA9555 `Status.h`, Config and actual validation/state code;
INA228 `HealthPolicy`/`HardwareState`, MB85RC model/capability definitions and
SHT3x snapshots/tests. Record exact revisions/paths and deliberate differences.
Keep namespace/class `DP8382x`, PascalCase types, camelCase methods/fields,
`_camelCase` private members and uppercase enums/constants.

- Freeze `Status { Err code; int32_t detail; const char* msg; }`, static
  message lifetime, helpers `ok`, `is`, `inProgress`, explicit bool, `Ok`,
  `Error` and `errorName` according to CONVENTIONS. Define each error's detail
  meaning and terminal callback normalization, including invalid status/enums.
  No SDK errors or I2C NACK meanings leak into the portable interface.
  Define transaction-release guarantees. Prefer a backend contract that releases
  ownership on every terminal return. If release can fail, specify retained/
  unknown ownership, primary-error precedence and application resolution before
  further affected I/O; this is distinct from passive managed health.
- Specify `Variant`, `DeviceIdentity`, `Capabilities`, Config, advertisement,
  forced mode and legal combinations. Separate declared part, observed IDs,
  revision and strap/interface evidence; no suffix inference from shared IDs.
- Freeze passive `DriverState` UNINIT/READY/DEGRADED/OFFLINE and
  `driverStateName`, `HealthSnapshot` fields/validity and the UNINIT/initializing
  boundary. Define counting units, threshold, saturating counters, last
  error/success timestamps and recovery. Specify exactly what `resetStatistics`
  clears and preserves, including trust, retained results and last-error policy.
  Prompt 11 starts transport accounting; prompt 21 implements managed health.
  Keep unimplemented health methods out of the public facade until then, rather
  than returning placeholder READY/UNINIT values from a working initialized PHY.
  OFFLINE never suppresses I/O. Managed transactions, physical frames and chip
  error counters differ.
- Define LinkSnapshot and desired/observed/trusted settings with explicit
  validity/age; default speed or zero timestamp is not fresh evidence. Cached
  `driverState`, `healthSnapshot`, `linkSnapshot`, `getSettingsSnapshot` do no
  I/O. `readLinkStatus` is a live observation. Cable-down is not MDIO failure.
- Specify borrowed callbacks/context, non-reentrancy, output-on-error, complete
  transaction lifetime and aggregate budgets for `readClause22`/`writeClause22`
  and `readMmd`/`writeMmd`. Short bounded calls need no token/job wrapper.
  `bind`, `end`, `isBound`, `isInitialized` and `invalidateDeviceState` get
  exact local effects; unresolved cooperative teardown is settled in prompt 8.
  Freeze non-copyable driver/operation-owner semantics and decide whether moving
  a bound object is supported. Borrowed context and active tokens must not gain
  a second independent owner through an accidental copy or move.

Write a state/error table covering invalid input before callbacks, unsupported
exact silicon versus unimplemented software, no accepted identity after valid
reads versus observed wrong part/revision, MDIO error/timeout versus operation
deadline, stable owned-bit CONFIG_MISMATCH, stale output and accepted/late
writes with uncertain effects. Logical aggregate failure may leave public
output unchanged while the collector retains successful destructive samples.

Map every documented feature to a planned typed operation/configuration and
observable result, including all legal values. Raw access alone cannot satisfy
multistep feature coverage. Keep unresolved feature-specific signatures and
source gates explicit until the owning implementation prompt. Avoid duplicate
`init`/`begin`/`state` aliases or a generic registry.

## Review and completion

Delegate independent state/error and vocabulary review with suitable available
models; shared declarations have one lead writer. Review design examples against
the pinned SDK owner design and real sibling behavior, not just spelling.
Resolve contradictory status/validity interpretations before coding.

Document every proposed value/direct call's units, ranges, valid states,
ownership, side effects, failure outputs and bound. Reconcile architecture,
feature map, README/CHANGELOG and example plans; no-change decisions are recorded.
Run reference/link/diff checks and update report/progress. Commit and sync the
complete contract and coherent checked units on the same execution branch.
Remote CI belongs only to prompt 71.
