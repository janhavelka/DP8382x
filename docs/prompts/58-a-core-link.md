# 58 - Implement A-generation identity, reset and link differences

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md),
[FULL_COVERAGE](FULL_COVERAGE.md) and [VERIFICATION](VERIFICATION.md). Required
inputs are resolved applicable [57](57-a-source-policy.md) facts and the
existing identity/reset/configuration/observation/clock procedures. Do not wait
for an unrelated source or physical gate. Full AI/AE support requires
[59](59-a-feature-closure.md) as well.

## Current evidence and ownership

Reopen current MB85RC variant metadata/identity tests, TMP1x2 variant tests and
TUSB32x declared-versus-observed profile validators. Inspect corresponding
public headers, actual source, CLI/runner selection, tests/check registration
and AGENTS. Record revisions, dirty state, exact symbols and
adopted/adapted/rejected rules. Return to SNLS783A tables and rollover sections
for every changed procedure.

The lead owns shared public selection, reset/timing contracts, metadata and
integration.

## Concrete source checks

Recheck [A identities and
procedures](../reference/library-family.md#identity-and-a-generation-comparison)
and [reset/timing
evidence](../reference/library-family.md#fiber-reset-diagnostic-and-power-procedures).
SNLS783A 7.6 gives a hardware-reset pulse minimum of 25 us, reset-to-SMI maximum
of 2 ms, POR-to-SMI maximum of 50 ms and nominal RMII leader-clock readiness of
0.2 ms. External-reset values do not establish BMCR/PHYRCR software completion
bounds. Table 8-49 defines PHYRCR bit 15 as software hard reset with the
hardware-reset effect, and bit 14 as digital restart preserving registers; BMCR
reset has its separate table-defined domain. Implement/test those domains,
readiness and owned-state restoration using exact tables, regardless of driver
labels. Keep physical clock/reset qualification separate.

## Work and acceptance

Implement source-proved A-generation identity, mode/revision admission,
initialization, reset/default/strap restoration, startup/10BASE-Te, advertised
and forced link settings, MDIX/pause/link decoding, FLD and clock/interface
differences. Keep unchanged paths shared in their existing responsibility
modules; use small explicit switches/tables for real differences, without
subtype hierarchies, duplicate drivers or empty per-subtype files. Preserve
declared AI versus AE when observed IDs are indistinguishable. Do not relax a
single model check to claim generation support.

Verify `VOD_CFG3 0x030E=0x4A40` reset/applicability/order from exact A evidence.
Original recipes run on A only when proved applicable. Preserve ANAR bit 13's
local advertisement prohibition while retaining supported observation. Reassess
MII/RMII and FLD restrictions; board clock/reset pins and application link
policy remain outside core. Separate source-proved programmable FLD effects from
the unresolved Strap11/CR3 EMC equivalence claim.

Register explicit AI and AE rows with independently derived literal vectors and
Release-active checks. Exercise shared IDs, wrong generation/declaration/mode,
unknown revisions, all-zero/all-one identity, invalid enum casts and
incompatible settings. Pure validation failures cause zero callbacks; a
successfully observed identity mismatch allows bounded identity reads but no
writes/further feature I/O. Trace complete reset/configuration/link sequences,
changed frame counts and restoration. Apply VERIFICATION's
failure/deadline/cleanup and cached-state cases to changed and inherited AI/AE
paths, including cancellation before and after the first write.

Update A-generation Doxygen, variants/API/feature/adapter and fixture guidance
with actual partial support. Run focused host/CLI/Python/docs checks and real
SDK links for changed adapter inputs. Record remaining A feature gates for 59
and complete COMMON's report and checkpoint.
