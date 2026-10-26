# 66 - Implement DP83822 diagnostics and documented test procedures

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md),
[FULL_COVERAGE](FULL_COVERAGE.md) and [VERIFICATION](VERIFICATION.md). Inputs
are [62](62-p22-source-subtypes.md)'s finite diagnostic/procedure inventory,
core/reset paths in [63](63-p22-copper-core.md), interface/medium applicability
from [64](64-p22-interface-clock.md) and [65](65-p22-fiber.md), and existing
bounded diagnostic/cleanup contracts.

## Current evidence and ownership

Reopen current TMP1x2 operation/failure tests and variant tests, MB85RC
selection metadata, and relevant peer runner restoration/artifact tests. Inspect
actual headers/source, shared CLI maintenance admission, diagnostic suite
eligibility, check registration and AGENTS. Record revisions, dirty state, exact
paths and adopted/adapted/rejected conventions. Derive silicon sequences from
SNLS505H, SNLA253 and applicable exact-part TI compliance/debug guidance.

The lead owns public diagnostic contracts, shared operations and integration.

## Concrete source checks

Start with [SNLA253 procedure
facts](../reference/library-family.md#fiber-reset-diagnostic-and-power-procedures):
`0x0171[3:0]=0xC`, `0x0173[15:8]=0xFF`, `0x0177[12:8]=0x18`; manual `0x001E[15]`
or auto `0x0009[8]`, status `0x001E[1:0]`, results `0x0180..0x018A`. Test
invalid 0xC samples and raw locations/amplitudes/polarity; the documented
location example `0x33` yields about 36.31 m, not another part's conversion.
Recheck original fields and conditions before adopting the recipe. [Analog
facts](../reference/library-family.md#interface-clock-analog-and-led-facts)
include exact LDCTRL voltage codes and fractional-ohm termination; Linux rounded
property tables are comparison facts. Keep calibration and measured electrical
accuracy explicit; correct source conflicts instead of fitting tests to a guess.

## Work and acceptance

Implement every assigned documented loopback, BIST/generator/checker/PRBS,
cable/TDR and test/compliance/analog/repeater procedure applicable to exact
I/H/IF/HF modes. Validate inherited procedures and add only source-proved deltas
in existing responsibility modules. Account for register-only controls and
recipes outside summary tables with exact allowlisted addresses/values; raw
writes alone do not implement a multistep test. Do not infer supported TDR
conditions or cable-length calibration from another variant or medium.

Apply SNLA253 prerequisites, launch/observation/termination and restoration
rules only under documented exact conditions. Reject copper-only diagnostics on
fiber before I/O. Link-disrupting/test modes need explicit application
maintenance admission, finite budgets and owned pre-state; board wiring or
missing measurement equipment changes qualification, not required software. Keep
one MDIO transaction owner and destructive-status collector. Compose mixed
control/status writes with exact W0C/W1C/W1S/RC/command-neutral semantics.

Register independently derived literal vectors and explicit part/medium/mode
rows with Release-active checks. Cover every legal diagnostic setting, invalid
enum/mask, unsupported combinations with zero callbacks, no-link/inconclusive
results and retained partial read-clear evidence. Verify complete frame order,
termination and reset/restore traces. Apply VERIFICATION's transfer, reset,
deadline/cancel and cleanup cases to every changed diagnostic procedure. A
successful primary test with failed restoration cannot become a passing restored
result.

Expose diagnostics through the existing typed API, shared CLI and explicit
runner/fixture suites. Update procedure Doxygen and diagnostic/fixture guides.
Run focused host/CLI/Python/docs checks, previous-family regressions and real
SDK links for changed inputs. Keep physical/calibration evidence separate and
complete COMMON's report and checkpoint.
