# 64 - Implement DP83822 interface, clock and fixed RGMII shifts

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md), [FULL_COVERAGE](FULL_COVERAGE.md)
and [VERIFICATION](VERIFICATION.md). Required inputs are exact subtype/interface
facts from [62](62-p22-source-subtypes.md), verified reset/core integration from
[63](63-p22-copper-core.md) and the existing clock/interface configuration contract.
Physical clock/timing qualification remains a separate fixture gate.

## Current evidence and ownership

Reopen TMP1x2 configuration validators/tests and TUSB32x `Config.h::DeviceProfile`
with actual profile admission tests; inspect MB85RC variant metadata/selection
for declared capability discipline. Read current relevant headers/source,
CLI/runner fixtures, SDK MAC/interface admission, registration and AGENTS. Record
revisions, dirty state, paths and adopted/adapted/rejected conventions. Do not
borrow a sibling's I2C platform build matrix for an EMAC target.

The lead owns shared interface types, capabilities, adapter admission and
integration. Delegate disjoint literal register/delay tests or read-only SDK
admission review after signatures freeze. Specify writable/forbidden files,
independent acceptance and available models chosen by reasoning complexity;
review all contributions. No agent Git mutations or sibling edits.

## Work and acceptance

Implement every documented applicable MII/RMII/RGMII interface and clock control,
with exact subtype/mode applicability, direction/source prerequisites, reset
values, units and safe programming order. Board pin assignments, oscillator
selection and physical strap changes stay application-owned. Hardware-only
properties need cited dispositions rather than invented programmable APIs.
Preserve unrelated/reserved fields and incorporate owned controls in reset/
recovery restoration without overwriting strap or external-owner state.

SNLS505H table 8-23 specifies fixed 3.5 ns RGMII shifts. Model explicit fixed
shift choices: RX RCSR bit 12 enables with 1, TX bit 11 enables with 0. Test
both polarities and reset values; do not invent arbitrary delay taps/ranges.
Validate combinations before callbacks. An invalid enum or incompatible
subtype/interface choice produces no I/O or configuration mutation.

Portable PHY capability includes source-proved RGMII even when the current P4
MAC cannot consume it. Adapter admission must explicitly reject unavailable MAC
interfaces while preserving generic device capabilities and useful diagnostics.
Do not require a nonexistent P4 RGMII firmware build or pretend an SDK header
shim proves a supported MAC path. Fiber is a media-side selection independent
of this interface; preserve valid IF/HF interface choices for prompt 65.

Use independent literal vectors for every legal/boundary clock and shift
encoding and subtype/interface row, including opposite TX/RX polarity. Check
complete read/write order, neutral mixed-register composition and reset/restore
traces. Inject each changed transfer/cleanup failure, accepted-write/error,
cancellation, exhausted aggregate budget and failed restoration. Preserve
output-on-error, backend detail, configuration uncertainty and primary/cleanup
errors; snapshots remain bus-silent. Test adapter rejection and CLI/runner
capability agreement through actual code, not grep alone.

Ship Doxygen units, restrictions, reset effects and exact citations, plus affected
README/CHANGELOG/variants/interface/API/adapter/fixture guides; record inspected
no-change reasons. Run focused host/CLI/Python/docs checks and actual supported
SDK links; package checks follow export changes. Record commands/counts and
unqualified timing claims in report, validation and progress. Check the final
diff against entry rules, commit/sync verified units and completion on the
execution branch. Only prompt 71 executes/checks remote CI in this first set.
