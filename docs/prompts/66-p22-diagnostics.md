# 66 - Implement DP83822 diagnostics and documented test procedures

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md), [FULL_COVERAGE](FULL_COVERAGE.md)
and [VERIFICATION](VERIFICATION.md). Inputs are [62](62-p22-source-subtypes.md)'s
finite diagnostic/procedure inventory, core/reset paths in [63](63-p22-copper-core.md),
interface/medium applicability from [64](64-p22-interface-clock.md) and
[65](65-p22-fiber.md), and existing bounded diagnostic/cleanup contracts.

## Current evidence and ownership

Reopen current TMP1x2 operation/failure tests and variant tests, MB85RC selection
metadata, and relevant peer runner restoration/artifact tests. Inspect actual
headers/source, shared CLI maintenance admission, diagnostic suite eligibility,
check registration and AGENTS. Record revisions, dirty state, exact paths and
adopted/adapted/rejected conventions. Derive silicon sequences from SNLS505H,
SNLA253 and applicable exact-part TI compliance/debug guidance.

The lead owns public diagnostic contracts, shared operations and integration.
Delegate disjoint independent vector/failure tests or read-only TDR procedure
review after signatures freeze; assign writable/forbidden paths and concrete
acceptance. Choose available models for coding versus difficult side-effect/
ownership reasoning and review all contributions. Agents do not mutate Git,
shared contracts or siblings, and do not operate fixtures incidentally.

## Work and acceptance

Implement every assigned documented loopback, BIST/generator/checker/PRBS,
cable/TDR and test/compliance/analog/repeater procedure applicable to exact
I/H/IF/HF modes. Validate inherited procedures and add only source-proved deltas
in existing responsibility modules. Account for register-only controls and
recipes outside summary tables with exact allowlisted addresses/values; raw
writes alone do not implement a multistep test. Do not infer supported TDR
conditions or cable-length calibration from another variant or medium.

Apply SNLA253 prerequisites, launch/observation/termination and restoration
rules only under documented exact conditions. Reject copper-only diagnostics
on fiber before I/O. Link-disrupting/test modes need explicit application
maintenance admission, finite budgets and owned pre-state; board wiring or
missing measurement equipment changes qualification, not required software.
Keep one MDIO transaction owner and destructive-status collector. Compose mixed
control/status writes with exact W0C/W1C/W1S/RC/command-neutral semantics.

Register independently derived literal vectors and explicit part/medium/mode
rows with Release-active checks. Cover every legal diagnostic setting, invalid
enum/mask, unsupported combinations with zero callbacks, no-link/inconclusive
results and retained partial read-clear evidence. Verify complete frame order,
termination and reset/restore traces. Inject every changed transfer/cleanup
failure, accepted-write/error, cancellation at side-effect boundaries, reset
mid-operation, no primary budget and exhausted finite cleanup budget. Preserve
terminal tokens/results, backend detail, primary versus cleanup errors,
configuration uncertainty and unresolved hardware effects. A successful primary
test with failed restoration cannot become a passing restored result.

Expose capabilities through existing typed API, one CLI grammar and explicit
runner/fixture suites. Ship Doxygen parameters/units/effects/lifetime/bounds and
source sequence citations; update README/CHANGELOG/variants/API/diagnostic/
fixture guides or record inspected no-change reasons. Run focused host/CLI/
Python/docs checks, earlier-family regressions and actual SDK links for changed
inputs. Record counts, audit and physical limitations in report, validation and
progress. Check final diff against entry rules; commit/sync verified units and
completion on the execution branch. Only 71 executes/checks remote CI.
