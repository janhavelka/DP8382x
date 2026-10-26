# 60 - Establish DP83825I sources and implement its core differences

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md),
[FULL_COVERAGE](FULL_COVERAGE.md) and [VERIFICATION](VERIFICATION.md). Inputs
are stable exact-part, MDIO, identity/reset/link contracts and the existing
primary core. Applicable family improvements may be reused without requiring
unrelated A-generation gates. Complete DP83825I feature support also requires
[61](61-p25-feature-closure.md).

## Current evidence and ownership

Reopen `../TMP1x2/include/TMP1x2/Config.h::isValidModel/hasAlertOutput` and
`test/variants/test_variants.cpp`; `../24Cxx/include/EEPROM24Cxx/Config.h` and
actual geometry/variant selection tests; MB85RC `VariantInfo` and ID matching.
Inspect relevant current CLI/help, runners, registration and AGENTS. Record
revisions, dirty state, exact paths and adopted/adapted/rejected conventions.
Adopt declared-device discipline without I2C or memory-geometry semantics.

The lead owns shared API, selection and integration.

## Concrete source checks

Use SNLS638C table 7-6 as the source-backed DP83825I expectation: model `0x14`,
revision 0, ID `0x2000A140`. Record [TI Linux's conflicting
labels](../reference/library-family.md#identity-and-a-generation-comparison)
(I=`0x2000A150`, S=`0x2000A140`) as a discrepancy. They do not override the
exact datasheet or justify admitting both IDs. Implement and independently test
the documented policy; keep observed-silicon qualification separate. Any
alternate identity needs exact-part/revision authority before admission. [RCSR
facts](../reference/library-family.md#interface-clock-analog-and-led-facts)
include bit 7 25/50 MHz polarity and a reset `0x0061` versus reserved-bit 5/6
reset-zero contradiction. Preserve reserved values read from hardware and
compare only source-proved owned fields; do not synthesize a whole-register
reset word from conflicting defaults. Record that ambiguity without blocking
other source-proved operations. SNLS638C specifies a 25 us minimum hardware-reset pulse,
2 ms maximum reset-to-SMI and 50 ms maximum POR readiness; Zephyr's 1 ms/50 ms waits are observed
policy. Add literal conflict, wrong-device, clock-polarity and
no-MII/RGMII/fiber cases before claiming support.

## Work and acceptance

Read SNLS638C directly, exact DP83825 troubleshooting/EVM material and relevant
current primary code. Pin documents/commits/paths/licenses and actual inspection
depth. Inventory every register/field/procedure, including recipe-only addresses
and optional runtime controls; compare exact identity/revision, Clause
22/extended mapping, reset defaults/domains, straps, interface/clock direction,
startup, 10 Mbit, link/status and diagnostic capability. Assign remaining
feature-family differences explicitly to 61. Research completion may record
conflicts; affected implementation cannot be marked complete until essential
facts resolve.

Implement exact DP83825I selection and source-proved identity, initialization,
reset/restore, RMII clock/configuration and link behavior in common
responsibility modules. Keep selection exact to DP83825I; undocumented Linux
S/CM/CS names establish no additional supported part. Report an observed
identity outside the documented policy as a mismatch with raw evidence retained.
RMII-only portable configuration and SDK admission reject MII/RGMII/fiber and
other unavailable combinations before callbacks or configuration mutation. Board
clock/reset pins remain application-owned. Matching Clause 22 addresses do not
authorize DP83826 extended recipes. Enforce local ANAR remote-fault bit 13
prohibition while retaining supported remote-fault observation.

Register independently derived SNLS638C literal vectors for legal RMII/clock
choices, general 10/100 and duplex settings, reset-loaded values,
wrong/shared/unknown/all-zero/all-one identity, revisions and invalid
masks/enums. Check exact traces and untouched read outputs; zero-I/O validation
is measured by callbacks. Identity mismatch permits only bounded observation
reads. Apply VERIFICATION's failure/deadline/cleanup and cached-state cases to
the changed startup/link and inherited DP83825I paths. Trace owned-field
restoration and verify DP83826-only writes never run without exact DP83825I
authority.

Update exact-I Doxygen, variants/API/clock and fixture guidance with actual
partial support. Run focused host/CLI/Python/docs checks and real SDK links for
changed inputs. Record the finite remaining feature list for 61, identity source
discrepancy and separate physical evidence; complete COMMON's report/checkpoint.
