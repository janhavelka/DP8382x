# 58 - Implement A-generation identity, reset and link differences

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md), [FULL_COVERAGE](FULL_COVERAGE.md)
and [VERIFICATION](VERIFICATION.md). Required inputs are resolved applicable
[57](57-a-source-policy.md) facts and the existing identity/reset/configuration/
observation/clock procedures. Do not wait for an unrelated source or physical
gate. Full AI/AE support requires [59](59-a-feature-closure.md) as well.

## Current evidence and ownership

Reopen current MB85RC variant metadata/identity tests, TMP1x2 variant tests and
TUSB32x declared-versus-observed profile validators. Inspect corresponding public
headers, actual source, CLI/runner selection, tests/check registration and AGENTS.
Record revisions, dirty state, exact symbols and adopted/adapted/rejected rules.
Return to SNLS783A tables and rollover sections for every changed procedure.

The lead owns shared public selection, reset/timing contracts, metadata and
integration. Delegate independent literal-vector tests or read-only sequence
review only after the contract freezes; assign exact writable/forbidden paths
and failure acceptance. Use an available model appropriate to coding or difficult
silicon/ownership review, record the choice and inspect all contributions. No
agent Git mutations or sibling edits.

## Concrete source checks

Recheck [A identities and procedures](../reference/library-family.md#identity-and-a-generation-comparison)
and [reset/timing evidence](../reference/library-family.md#fiber-reset-diagnostic-and-power-procedures).
SNLS783A gives hardware reset pulse 25 us, reset-to-SMI2 ms, POR-to-SMI 50 ms
and RMII leader-clock0.2 ms; do not use an external-reset timing as an unproved
BMCR/PHYRCR completion bound. Contrast Linux bit 15 software-reset versus
bit 14 digital-restart naming with the older combined source. Test exact domains,
bounded readiness and owned-state restoration; a source's 50 ms busy wait is
framework policy, not a universal timeout. Reconcile conflicting evidence before
coding and keep real clock/reset qualification explicit.

## Work and acceptance

Implement source-proved A-generation identity, mode/revision admission,
initialization, reset/default/strap restoration, startup/10BASE-Te, advertised and
forced link settings, MDIX/pause/link decoding, FLD and clock/interface differences.
Keep unchanged paths shared in their existing responsibility modules; use small
explicit switches/tables for real differences, without subtype hierarchies,
duplicate drivers or empty per-subtype files. Preserve declared AI versus AE
when observed IDs are indistinguishable. Do not relax a single model check to
claim generation support.

Verify `VOD_CFG3 0x030E=0x4A40` reset/applicability/order from exact A evidence.
Original recipes run on A only when proved applicable. Preserve ANAR bit 13's
local advertisement prohibition while retaining supported observation. Reassess
MII/RMII and FLD restrictions; board clock/reset pins and application link policy
remain outside core. Separate source-proved programmable FLD effects from the
unresolved Strap11/CR3 EMC equivalence claim.

Register explicit AI and AE rows with independently derived literal vectors and
Release-active checks. Exercise shared IDs, wrong generation/declaration/mode,
unknown revisions, all-zero/all-one identity, invalid enum casts and incompatible
settings. Pure validation failures cause zero callbacks; a successfully observed
identity mismatch allows bounded identity reads but no writes/further feature I/O.
Trace complete reset/configuration/link sequences, changed frame counts and
restoration. Inject every changed read/write/cleanup failure, accepted-write/error,
cancellation before/after first write, exhausted primary/cleanup budgets and
failed restore. Preserve output validity, backend detail, retained uncertainty
and primary versus cleanup errors. Cached capability/health views stay bus-silent;
link down remains separate from management transport failure.

Ship Doxygen on exact-part restrictions, units, reset effects and source sequence
citations; update README partial evidence, CHANGELOG, variants, API/feature,
adapter and fixture guides, or record inspected no-change reasons. Run affected
host/CLI/Python/docs checks and actual SDK links when adapter inputs change.
Record executed counts, source audit and remaining A feature gates in report,
validation and progress. Recheck entry rules against the final diff. Commit/sync
checked units and this prompt on the same recorded execution branch; remote CI
execution/status checking stays exclusively in 71.
