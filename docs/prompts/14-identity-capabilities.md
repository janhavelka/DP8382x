# 14 - Implement exact identity and capability selection

Follow [COMMON](COMMON.md) and its shared contracts, including
[CONVENTIONS](CONVENTIONS.md), [EVIDENCE](EVIDENCE.md) and
[FULL_COVERAGE](FULL_COVERAGE.md). Begin with the stage-specific rule reminder:
owned paths, fresh relevant sibling audit, selected names, one MDIO owner,
documentation/local checks and the recorded execution branch. Use available
subagents for disjoint work or independent review as COMMON specifies.

Prerequisites: [13](13-side-effect-register-access.md), [3](3-silicon-core-ledger.md).
Verify the concrete inputs below; retain unrelated open gates under COMMON.

## Scope, current evidence and work

Deliver exact-address DP83826I identity acquisition and honest capability
selection. Extend existing config/driver values and register code; no discovery
registry, broad family factory or automatic address selection.

Reopen MB85RC `src/MB85RC.cpp::_selectVariant`,
`_validateActiveDeviceId`, `_readDeviceIdBytesRaw` and its
`CommandTable.h` variant lookup. Compare TMP1x2 declared-model validation
and `probe()`; inspect PCA9555
`test_por_default_check_is_explicit_and_non_identity` and
`test_address_scan_is_not_identity_proof` plus registration.
An I2C ACK or plausible configuration is not an MDIO identity oracle.

Implement `DeviceIdentity`, live `readIdentity` and passive capability
queries under the frozen contracts. Preserve raw IDs, decoded model/revision and
observed BASIC/ENHANCED mode with explicit validity. Reopen SNLS647I PHYID/mode/
strap tables. Shared IDs cannot identify ordering suffix or temperature grade;
the application declares the exact part. Accept revisions only through the
documented policy, not an unqualified vendor literal or broad OUI match.

Stage ordinary output until the identity is complete. Preserve partial read
diagnostics without presenting them as usable identity. Successfully read wrong
part/revision returns `DEVICE_MISMATCH` with its reason; no accepted/plausible
identity returns `DEVICE_NOT_FOUND` under the stated rule, not physical absence.
Failed reads retain `MDIO_ERROR`/`MDIO_TIMEOUT` and backend detail. Explain
all-zero/all-one results from available controller evidence without guessing ACK.

Capabilities depend on exact declared/observed part and mode, software support
and source evidence; board/fixture availability is separate. Reject unimplemented
relatives and contradictory declarations before feature writes. Rebind/reset/
external mode changes invalidate appropriate identity and permissions. Probe
success cannot imply link-up, verified settings, managed recovery or hardware
qualification. Any explicitly required scan uses a finite caller-supplied address
set and budget without implicit selection; otherwise keep exact-address probing.

Test independent literal BASIC/ENHANCED IDs, revision boundaries, wrong OUI/model,
aliases, ambiguous declarations, future relatives, partial reads, raw zero/all-one
data, wrong address, two instances and changed mode/rebind. Assert precise error
codes/details, unchanged outputs and bus-silent invalid/capability rejection.
Add a compiling probe example and variant decision table with distinct declared,
observed, implemented and hardware-qualified facts.

## Concrete TI source inputs

Use the [identity source table](../reference/library-ti-core.md#identity-and-capability-facts).
TI tests OUI `0x080028`, model `0x11/0x13`, revision exactly 1; SNLS647I tables
8-21/22 identify those models as BASIC/ENHANCED, giving literal revision-1 IDs
`0x2000A111` and `0x2000A131`. They do not identify I/E suffix. Decode model with
`0x03F0 >> 4`, revision with `0x000F` and OUI from both ID words. Test a second ID read failure with unchanged caller output. Qualify revision
acceptance and MII/RMII capability from the exact datasheet and declared part;
record actual part/strap measurements separately.

## Documentation, verification and checkpoint

Update public Doxygen, procedure citations, affected API/user/test/tool guides,
coverage, stage report and progress with this behavior. Review README/CHANGELOG
and record relevant no-change reasons. Run the focused registered host tests
above with active Release checks, affected portable-consumer and Doxygen checks,
`python tools/verify_references.py` and `git diff --check`.

Apply COMMON's final maintainability review and evidence reporting. Keep source
gaps, SDK/package results and physical qualification separate. The lead commits
and syncs this complete task and coherent verified units on the same execution
branch, verifies upstream equality, and reports any sync failure. First-set
remote CI runs and status checks belong only to prompt 71.
