# 56 - Qualify the complete DP83826E software path

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md), [FULL_COVERAGE](FULL_COVERAGE.md)
and [VERIFICATION](VERIFICATION.md). Required inputs are the frozen exact-part
contract, primary feature closure in [36](36-primary-feature-closure.md), and
relevant regression outputs from prompts 53–55. Physical qualification is separate.

## Current evidence and ownership

Reopen current `../TMP1x2/include/TMP1x2/Config.h` (`Model`, `isValidModel`,
`hasAlertOutput`), `src/TMP1x2Configuration.cpp::validateConfig` and
`test/variants/test_variants.cpp`; compare `../MB85RC/include/MB85RC/CommandTable.h`
(`VariantInfo`, `KNOWN_VARIANTS`, `findVariantByProductId`) and actual identity
dispatch/tests. Inspect relevant current CLI, runners, check registration and
AGENTS. Record revisions, dirty state, paths and adopted/adapted/rejected rules.

The lead owns shared selection, public headers, architecture and integration.
A useful delegation is read-only I/E source comparison or a disjoint focused E
test file after signatures freeze. Specify writable/forbidden paths, independent
acceptance vectors and an available model suited to the task; review every result.
Agents do not mutate Git or sibling projects.

## Concrete source checks

Use [the family catalogue](../reference/library-family.md#identity-and-a-generation-comparison)
and the pinned TI RTOS/Linux files beside SNLS647I. Linux C/NC labels
`0x2000A130/0x2000A110` describe source model names, not E/I suffix detection.
Test declared E with the exact admitted BASIC/ENHANCED model and revision rows;
shared identity must retain the declaration. Reconfirm `0x030E=0x4A40` and
ANAR bit 13 `0x2000` prohibition for each applicable E path. Do not import A's
revision 4 or Strap11 EMC guidance into original E. Correct any source conflict
in the ledger and plan; unresolved hardware evidence remains a separate gate.

## Work and acceptance

Repeat the complete public-source/register/field/procedure audit for DP83826E,
using SNLS647I and applicable current official errata or clarification. Compare
ordering code, mode, interface, identity/revision, reset domains/defaults, strap
resampling, 10BASE-Te and every runtime opt-in feature. Sharing a datasheet does
not establish equality. Share ledger records only with explicit I/E applicability
and cited identical semantics; a blanket uncited “same as I” cannot close E.

Keep the declared suffix when register IDs cannot distinguish it. Do not infer
temperature grade or board wiring from a shared ID. Add public supported selection
and capabilities only with complete implemented paths. Put actual differences
beside the common responsible procedure or a small private table; retain one
MDIO transaction owner, destructive-status collector and shared CLI grammar.
Reconcile cached capabilities, help/status, command admission, adapter selection
and runner fixtures without removing generic features because a pin is absent.

Register independent literal SNLS647I vectors for E and common I/E procedures:
compatible/shared, unknown, all-zero/all-one IDs; revision and declaration/mode
mismatch; invalid enum casts; reset-loaded values; applicable 10 Mbit writes/order;
and ANAR remote-fault advertisement restrictions. Test each changed transfer and
cleanup boundary, accepted-write/error, cancellation, expired aggregate budget,
failed restoration, output sentinels and preserved backend/primary/cleanup detail.
Pure unsupported-setting preflight makes zero callbacks; observed identity mismatch
permits only required bounded identity reads. Cached state remains bus-silent;
cable loss alone does not degrade transport health.

Ship Doxygen restrictions/effects/citations and affected README support table,
CHANGELOG, variants, API/feature and fixture guides together. Record no-change
reasons for inspected unaffected guides. Run affected host/CLI/Python/docs checks,
common I/E regressions and real SDK links for changed adapter paths. Keep E physical
status separate; essential software gaps remain open. Record commands/counts,
coverage and evidence in validation, progress and this prompt's implementation
report. Check the entry rules against the final diff. Commit/sync checked work
and coherent verified units on the recorded execution branch; only prompt 71
executes/checks remote CI in this first set. Do not make empty review commits.
