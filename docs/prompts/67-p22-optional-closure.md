# 67 - Close DP83822 power, wake, LED and complete subtype coverage

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md),
[FULL_COVERAGE](FULL_COVERAGE.md) and [VERIFICATION](VERIFICATION.md). Inputs
are the complete assigned feature inventory in [62](62-p22-source-subtypes.md),
implemented [63](63-p22-copper-core.md), [64](64-p22-interface-clock.md),
[65](65-p22-fiber.md) and [66](66-p22-diagnostics.md) paths, plus existing
primary optional-control APIs.

## Current evidence and ownership

Reopen TMP1x2 current configuration/capability and variant tests, MB85RC variant
metadata/actual dispatch, and peer tool tests distinguishing missing fixtures
from implemented controls. Inspect current public/source, SDK admission,
CLI/help, runner suites, registration and AGENTS. Record revisions, dirty state,
paths and adopted/adapted/rejected rules. Recheck SNLS505H and exact applicable
EEE/WoL/power/LED notes, including limitations of vendor reference APIs.

The lead owns shared capabilities, public types, integration and closure claims.

## Concrete source checks

Implement the documented [wake map omitted by
Linux](../reference/library-family.md#interrupt-and-wake-facts-from-ti-linux):
64 pattern bytes `0x04A8..0x04C7`, byte masks `0x04C8..0x04CB` where 1 ignores a
byte, offset `0x04CC[5:0]` default 12; CRC gate and W1S clear in `0x04A0`
require exact neutral writes. Test boundary bytes 0/63, masks and offset, not
only Magic/Secure-ON. [Power
facts](../reference/library-family.md#fiber-reset-diagnostic-and-power-procedures)
require IEEE power-down before `0x0428[2]` deep power-down, with bit 3 selecting
50/100 ms exit; preserve adjacent test-mode bits. Reconcile LED0/GPIO2 MLED,
clock/LED pin conflicts, EEE/LPI and SFD's documented WoL-enable dependency.
Verify exact-part semantics and actual hardware separately; API omissions and
missing wiring cannot close required software rows.

## Work and acceptance

Implement the finite source-assigned differences for EEE/LPI and supported MAC
signalling, WoL/frame filtering, every PHY low-power state and wake transition,
LED/GPIO mux/polarity and remaining documented independent controls. Validate
inherited behavior rather than reimplementing common features. “Optional” means
runtime opt-in, not permission to omit software. Missing IRQ/LED/wake wiring,
optical equipment or P4 interface support limits physical/adapter evidence only.
If the source audit discovers a substantial unassigned family, repair the plan
explicitly instead of hiding it as unbounded residual closure.

For I/H/IF/HF independently, reconcile every register/non-reserved
field/documented procedure, all legal settings and reset/mode effects to source,
production symbol/typed path, CLI, independent test and fixture requirements.
Share source-proved identical records with explicit part applicability and
separate evidence; do not copy a family table unchecked. Hardware-only and
source-proved inapplicable rows need citations. Required unsupported-by-software
or ambiguous rows remain OPEN and prevent full software support claims.

Preserve whole-transaction ownership, one destructive collector, neutral mixed
register writes, bounded restoration and primary/cleanup uncertainty. Extend
independent literal vectors for every changed feature and each inherited
part/medium/interface applicability row. Test invalid masks/enums, incompatible
combinations with zero preflight callbacks, reset-loaded state, no-link versus
transport health, and zero-callback cached views. Apply VERIFICATION's transfer,
partial-acquisition, reset, deadline/cancel and cleanup cases to changed and
inherited subtype paths. Verify exact traces and retained read-clear evidence.
Reconcile capability getters, SDK admission, CLI help/results and runner suite
eligibility.

Run full subtype ledger reconciliation, focused host/CLI/Python/Doxygen checks,
previous-family regressions and actual supported SDK builds. Package checks
follow changed exports. Update exact-part Doxygen and
variants/API/interface/feature/adapter/fixture guides with reconciled support.
Record per-subtype software closure, SDK evidence and separate physical gates;
complete COMMON's report and checkpoint.
