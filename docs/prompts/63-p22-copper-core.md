# 63 - Implement DP83822 copper identity, reset and link behavior

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md),
[FULL_COVERAGE](FULL_COVERAGE.md) and [VERIFICATION](VERIFICATION.md). Required
inputs are resolved applicable [62](62-p22-source-subtypes.md) source
dispositions and the existing atomic MDIO, identity, reset, link and collector
contracts. This task covers copper core differences, not complete DP83822 family
support or physical qualification.

## Current evidence and ownership

Reopen current MB85RC variant metadata/dispatch tests, TMP1x2 variant and
configuration tests, and TUSB32x declared/observed profile validators. Inspect
relevant public source, CLI/runner admission, registration and AGENTS. Record
revisions, dirty state, paths and adopted/adapted/rejected conventions. Derive
changed behavior directly from exact SNLS505H tables/sections.

The lead owns shared selection, reset/link contracts, capabilities and
integration.

## Concrete source checks

Check [TI RTOS register facts and
defects](../reference/library-family.md#ti-rtos-dp83822-concrete-behavior): CR1
`0x09[5]` robust MDIX, PHYCR `0x19[15]` auto/`[14]` forced crossover, and PHYSTS
`0x10[2:0]`. Independent linked literals are `0x0001` 100/half, `0x0005`
100/full, `0x0003` 10/half, `0x0007` 10/full. Preserve MDIX transfer errors and
incomplete configuration effects. SNLS505H specifies a 10 us minimum
hardware-reset pulse and up to 200 ms POR register readiness, distinct from
25/26. Recheck original timing columns and reset domains before encoding
deadlines. Test no-link/read failure separately and never treat shared ID as
subtype/medium proof.

## Work and acceptance

Implement documented copper identity/revision admission, startup/10 Mbit,
initialization, reset/default/strap behavior, owned-state restoration, forced
and advertised 10/100/duplex settings, negotiation/next-page/pause, MDIX and
link resolution. Use exact per-subtype applicability; preserve declared
I/H/IF/HF when registers cannot distinguish them. Do not transfer another
family's vendor writes by address resemblance. Source-proved absent
interface/medium combinations reject before I/O; unknown source facts remain
gates rather than unsupported silicon.

Implement copper link/status/event/counter and interrupt-register differences
through the existing collector and typed paths. One collector owns destructive
reads; retain consumed samples when later reads fail while aggregate validity
becomes incomplete. Use exact neutral values for mixed command/status writes and
preserve reset domains. Keep generic 10/100 separate from board 100/full policy,
transport health separate from no-link and configuration trust. Interface timing
and fiber-specific behavior belong to their following prompts; do not publish
complete family support from this core task.

Register explicit host rows for every applicable exact copper part and legal
mode using independent literal SNLS505H vectors and Release-active checks. Cover
shared/unknown/all-zero/all-one identity, wrong declaration/mode/revision,
invalid enum/mask, reset-loaded values, incompatible selections, link flaps,
negotiation/pause and unavailable registers. Pure preflight failures make zero
callbacks; observed identity mismatch allows bounded identity reads but no
writes or later feature I/O. Verify complete startup/configuration/reset traces
and apply VERIFICATION's failure/deadline/cleanup and cached-state cases to
changed and inherited DP83822 copper paths.

Update subtype Doxygen, variants/API/feature and fixture guidance with the
implemented copper scope. Run focused host/CLI/Python/docs checks,
original-family regressions and real SDK links for changed inputs. Reconcile
remaining interface, fiber and feature gates, then complete COMMON's report and
checkpoint.
