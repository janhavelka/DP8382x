# 56 - Qualify the complete DP83826E software path

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md),
[FULL_COVERAGE](FULL_COVERAGE.md) and [VERIFICATION](VERIFICATION.md). Required
inputs are the frozen exact-part contract, primary feature closure in
[36](36-primary-feature-closure.md), and relevant regression outputs from
prompts 53–55. Physical qualification is separate.

## Current evidence and ownership

Reopen current `../TMP1x2/include/TMP1x2/Config.h` (`Model`, `isValidModel`,
`hasAlertOutput`), `src/TMP1x2Configuration.cpp::validateConfig` and
`test/variants/test_variants.cpp`; compare
`../MB85RC/include/MB85RC/CommandTable.h` (`VariantInfo`, `KNOWN_VARIANTS`,
`findVariantByProductId`) and actual identity dispatch/tests. Inspect relevant
current CLI, runners, check registration and AGENTS. Record revisions, dirty
state, paths and adopted/adapted/rejected rules.

The lead owns shared selection, public headers, architecture and integration.

## Concrete source checks

Use [the family
catalogue](../reference/library-family.md#identity-and-a-generation-comparison)
and the pinned TI RTOS/Linux files beside SNLS647I. Linux C/NC labels
`0x2000A130/0x2000A110` describe source model names, not E/I suffix detection.
Test declared E with the exact admitted BASIC/ENHANCED model and revision rows;
shared identity must retain the declaration. Reconfirm `0x030E=0x4A40` and ANAR
bit 13 `0x2000` prohibition for each applicable E path. Do not import A's
revision 4 or Strap11 EMC guidance into original E. Correct any source conflict
in the ledger and plan; unresolved hardware evidence remains a separate gate.

## Part-keyed implementation contract

Add one private `PartTable` with `RegisterMap { DP83826, DP83826A, DP83825,
DP83822 }` and `PartProfile { Variant variant; RegisterMap map; uint8_t
modelCount; uint8_t models[2]; uint16_t revisionMask; uint8_t interfaceMask;
uint32_t featureMask; bool hasOperatingMode; uint8_t strapAddressMax;
bool forcedRequiresRobust; }`. `bind()` resolves `_part`; policy takes the map,
procedures branch only at real differences, and new revisions are table edits.
No per-part files. Original/A maps may share an address array but not changed
field masks. Reject `requiredMode != UNKNOWN` on a part without operating modes.

Rows: DP83826 models {0x11,0x13}, revision mask `0x0002`, MII|RMII, strap max 7,
mode true, forced robust true; A differs by revision mask `0x0010`; DP83825
model {0x14}, mask `0x0001`, RMII, strap max 3, false, false; DP83822 model
{0x24}, mask `0x0001`, MII|RMII|RGMII, strap max 31, false, false. Pure link
validation checks enums/masks/cross-field constraints; robust admission is
part-keyed preflight. VOD is not a validator rule: its initialization write is
keyed only to DP83826/A. Update foundation flags to `(flags & 30U) == 0U` and
make the corresponding public contracts part-specific.

## Work and acceptance

Use one document-level cited SNLS647I record: I/E share a register map and
procedure set, with temperature and TA-keyed leakage differences only and no
suffix-conditional software behavior. Add `Variant::DP83826E` sharing I's
profile; reopen only if new TI evidence adds a suffix condition. Parameterize
identity and bind tests rather than repeating the complete audit.

Keep the declared suffix when register IDs cannot distinguish it. Do not infer
temperature grade or board wiring from a shared ID. Add public supported
selection and capabilities only with complete implemented paths. Put actual
differences beside the common responsible procedure or a small private table;
retain one MDIO transaction owner, destructive-status collector and shared CLI
grammar. Reconcile cached capabilities, help/status, command admission, adapter
selection and runner fixtures without removing generic features because a pin is
absent.

Register independent literal SNLS647I vectors for E and common I/E procedures:
compatible/shared, unknown, all-zero/all-one IDs; revision and declaration/mode
mismatch; invalid enum casts; reset-loaded values; applicable 10 Mbit
writes/order; and ANAR remote-fault advertisement restrictions. Apply
VERIFICATION's failure/deadline/cleanup and cached-state cases to changed and
inherited I/E paths. Invalid settings cause no callbacks; observed identity
mismatch permits only bounded identity reads.

Update exact-E Doxygen, variants/support tables, API/feature and fixture guides.
Run focused host/CLI/Python/docs checks, shared I/E regressions and real SDK
links for changed adapter inputs. Reconcile E software coverage and physical
status separately, then complete COMMON's report and checkpoint.
