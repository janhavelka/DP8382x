# 59 - Close A-generation diagnostic and optional-control coverage

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md),
[FULL_COVERAGE](FULL_COVERAGE.md) and [VERIFICATION](VERIFICATION.md). Inputs
are [57](57-a-source-policy.md)'s complete AI/AE ledger,
[58](58-a-core-link.md)'s verified core paths and implemented primary feature
procedures. This prompt implements source-proved differences and validates
inherited applicability; it does not reimplement every shared feature.

## Current evidence and ownership

Reopen TMP1x2 variant tests and configuration validation, MB85RC variant
metadata and actual selection tests, and relevant peer diagnostic
operation/failure tests. Inspect current CLI/help, runner fixture admission,
registration and AGENTS. Record revisions, dirty files, exact paths and
adopted/adapted/rejected conventions. Recheck SNLS783A and applicable exact-part
TI guidance for the covered controls.

The lead owns shared capabilities, declarations and integration.

## Concrete source checks

Use [the family IRQ/wake
facts](../reference/library-family.md#interrupt-and-wake-facts-from-ti-linux) to
challenge inherited paths, then establish exact AI/AE equality from SNLS783A.
MISR1/2 addresses `0x12/0x13` have enable/status byte pairs; test a consumed
first sample followed by failed second read. Vendor-MMD 31 wake config `0x04A0`
includes bit 11 clear-indication command `0x0800`, distinct from retained state.
SNLS783A table 8-91 marks bit 11 W0C: zero clears the indication, and ordinary
configuration updates must write the command-neutral one. Test both intentional
acknowledgement and unrelated configuration with a pending indication. Linux
packs array bytes for MAC `02:12:34:56:78:9A` into `0x1202,0x5634,0x9A78`. This
is an observed candidate, not an independent oracle. Define application octet
order versus TI's Byte 5/MSB convention and derive the actual AI/AE expected
words independently before accepting that vector. Linux omits intermediate error
checks; require partial-write uncertainty and finite cleanup. Do not transfer
DP83822's 64-byte custom filter by a family register label.

## Work and acceptance

Implement the ledger-assigned A-generation differences in interrupts and
destructive status/counters; supported loopback/BIST/PRBS/cable diagnostics; PHY
power/EEE/LPI, wake/filtering, LED/GPIO and documented test/compliance/analog
controls. Apply only features supported by exact AI/AE evidence, but do not omit
documented programmable behavior merely because the first board or a vendor
driver lacks it. Preserve source-backed shared procedures and add only required
small differences. The finite task denominator is the feature list assigned in
57; enumerate any newly discovered requirement and explicitly repair the plan
rather than burying an unbounded new feature family in a “closure” label.

For every AI/AE register, field and procedure, reconcile source, legal settings,
typed path, independent test, CLI and runner eligibility. Recipe-only addresses
remain exact procedure allowlists, not arbitrary raw-write access. Keep complete
transactions under one owner, destructive evidence under one collector and reset
restoration bounded. Mixed control/status writes require exact neutral
composition. Preserve primary/cleanup errors and uncertain accepted-write
effects through cancellation, timeout and end; a successful later probe cannot
erase them.

Extend independent literal vectors for both exact variants and all inherited
procedure applicability. Cover wrong mode/revision and inaccessible registers
with no unintended I/O. Apply VERIFICATION's transfer, partial-acquisition,
reset, deadline/cancel, cleanup and cached-state cases to changed and inherited
AI/AE paths. Reconcile adapter selection, capability getters, CLI
help/admission/results and fixture suite selection.

AI/AE software support can close only when every required software row is
implemented/tested or has a cited valid silicon/application-boundary
disposition. Essential unresolved software stays OPEN; Strap11/EMC physical
uncertainty and each variant's physical evidence remain separately visible. Run
focused host/CLI/Python/Doxygen checks, earlier-family regressions and real
supported SDK links; recheck packages for changed exports. Update exact-A
variants/API/feature/adapter and fixture guidance. Record software closure and
separate physical gates, then complete COMMON's report and checkpoint.
