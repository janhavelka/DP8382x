# 14 - Implement exact identity and capability selection

Read [COMMON](COMMON.md), [ENGINEERING](ENGINEERING.md),
[CONVENTIONS](CONVENTIONS.md), [FULL_COVERAGE](FULL_COVERAGE.md) and
[VERIFICATION](VERIFICATION.md). Use [SOURCE_MAP](SOURCE_MAP.md) for current
source locations; historical reports do not replace a fresh audit.

At entry record owned paths, applicable rules, current source revisions/dirty
state, selected names, one MDIO owner, local checks and the recorded execution
branch. Resume that branch. Use disjoint implementation/test work or independent
read-only review when useful; choose available models/effort by task complexity
and record the rationale. The lead owns shared contracts, integration and Git.
Siblings remain read-only.

Prerequisites: [13](13-side-effect-register-access.md), [3](3-silicon-core-ledger.md); only the concrete inputs used below are required.

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
`0x03F0 >> 4`, revision with `0x000F` and OUI from both ID words. Test a second
ID read failure: TI's generic helper has already changed caller output after
the first read, which this project must reject. Reconcile current source,
datasheet revision policy and actual part/strap readbacks; neither the library's
revision-1 filter nor its erroneous RGMII comment is a universal capability rule.

## Documentation, verification and checkpoint

Ship accurate public Doxygen and procedure citations with this behavior: units,
legal states, ownership, output validity, I/O effects, deadlines and cleanup.
Update API contracts, affected guides, restoration/feature evidence and the stage
report/progress. Review README, CHANGELOG and relevant example/test/tool guides;
update affected claims and record why others need no change. Future APIs and
physical qualification remain explicitly unimplemented/unverified.

Run registered focused host tests with active Release checks, relevant portable
consumer/source-documentation/link checks, the reference checker and
`git diff --check`. Record actual commands and separate PASS/FAIL/NOT RUN/BLOCKED
for host, SDK, packages and hardware where relevant. Missing source evidence
blocks the affected requirement; it cannot become unsupported silicon or close
a required gate. Continue independent eligible work.

Review the final diff for simple control flow, stale comments, duplicated
procedures and undocumented errors. Commit and sync checked tests/docs/code on
the same execution branch after this task and each coherent verified unit.
The lead fetches, safely integrates, pushes and verifies upstream equality;
report exact sync failures. First-execution remote CI dispatch, polling and
waiting belong only to checkpoint **71**, never this task.
