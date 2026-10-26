# 60 - Establish DP83825I sources and implement its core differences

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md), [FULL_COVERAGE](FULL_COVERAGE.md)
and [VERIFICATION](VERIFICATION.md). Inputs are stable exact-part, MDIO,
identity/reset/link contracts and the existing primary core. Applicable family
improvements may be reused without requiring unrelated A-generation gates.
Complete DP83825I feature support also requires [61](61-p25-feature-closure.md).

## Current evidence and ownership

Reopen `../TMP1x2/include/TMP1x2/Config.h::isValidModel/hasAlertOutput` and
`test/variants/test_variants.cpp`; `../24Cxx/include/EEPROM24Cxx/Config.h` and
actual geometry/variant selection tests; MB85RC `VariantInfo` and ID matching.
Inspect relevant current CLI/help, runners, registration and AGENTS. Record
revisions, dirty state, exact paths and adopted/adapted/rejected conventions.
Adopt declared-device discipline without I2C or memory-geometry semantics.

The lead owns shared API, selection and integration. Delegate a read-only source
comparison or disjoint literal-vector tests after freezing contracts. Specify
writable/forbidden files and independent acceptance; choose available models by
research/coding/semantics complexity and review contributions. Agents do not
mutate Git, shared contracts or sibling projects.

## Concrete source checks

Resolve [the explicit identity conflict](../reference/library-family.md#identity-and-a-generation-comparison):
SNLS638C table 7-6 specifies DP83825I model `0x14`, revision 0, ID `0x2000A140`;
TI Linux calls that S and uses I=`0x2000A150`. Keep affected selection OPEN
until exact silicon/revision evidence resolves it; neither label wins silently.
[RCSR facts](../reference/library-family.md#interface-clock-analog-and-led-facts)
include bit 7 25/50 MHz polarity and a reset `0x0061` versus reserved-bit 5/6
reset-zero contradiction. Recheck original tables and source; do not invent
defaults. SNLS638C distinguishes 25 us hardware pulse, 2 ms reset-to-SMI and 50 ms
POR readiness; Zephyr's 1 ms/50 ms waits are observed policy. Add literal conflict,
wrong-device, clock-polarity and no-MII/RGMII/fiber cases before claiming support.

## Work and acceptance

Read SNLS638C directly, exact DP83825 troubleshooting/EVM material and relevant
current primary code. Pin documents/commits/paths/licenses and actual inspection
depth. Inventory every register/field/procedure, including recipe-only addresses
and optional runtime controls; compare exact identity/revision, Clause 22/extended
mapping, reset defaults/domains, straps, interface/clock direction, startup,
10 Mbit, link/status and diagnostic capability. Assign remaining feature-family
differences explicitly to 61. Research completion may record conflicts; affected
implementation cannot be marked complete until essential facts resolve.

Implement exact DP83825I selection and source-proved identity, initialization,
reset/restore, RMII clock/configuration and link behavior in common responsibility
modules. Do not add a broad DP83825 selector or Linux S/CM/CS aliases without
exact silicon documents. Unknown or contradictory identity remains observable.
RMII-only portable configuration and SDK admission reject MII/RGMII/fiber and
other unavailable combinations before callbacks or configuration mutation.
Board clock/reset pins remain application-owned. Matching Clause 22 addresses
do not authorize DP83826 extended recipes. Enforce local ANAR remote-fault bit
13 prohibition while retaining supported remote-fault observation.

Register independently derived SNLS638C literal vectors for legal RMII/clock
choices, general 10/100 and duplex settings, reset-loaded values, wrong/shared/
unknown/all-zero/all-one identity, revisions and invalid masks/enums. Check exact
traces and untouched read outputs; zero-I/O validation is measured by callbacks.
Identity mismatch permits only bounded observation reads. Test restoration of
changed owned fields and each changed startup/link transfer and cleanup failure,
accepted-write/error, cancellation and exhausted aggregate budget. Retain backend
detail, terminal evidence, primary/cleanup errors and unresolved hardware effects;
DP83826-only writes never run without exact DP83825I permission.

Ship Doxygen restrictions/units/effects/citations and affected README partial
support, CHANGELOG, variants/API/clock/fixture guides, noting inspected no-change
reasons. Run focused host/CLI/Python/docs checks and actual SDK links for changed
paths. Record source audit, counts and open feature/physical gates in report,
validation and progress. Recheck the final diff, commit/sync checked units and
completion on the execution branch. Only prompt 71 executes/checks remote CI.
