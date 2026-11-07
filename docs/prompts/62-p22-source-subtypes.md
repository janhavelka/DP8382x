# 62 - Establish exact DP83822 subtype and feature evidence

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md),
[FULL_COVERAGE](FULL_COVERAGE.md) and [VERIFICATION](VERIFICATION.md). Required
inputs are frozen exact-part, interface/medium and operation contracts plus the
existing family ledger schema. Independent source research need not wait for
unrelated variant or fixture gates. This prompt produces researched
dispositions, not implemented subtype support.

## Current evidence and ownership

Reopen MB85RC `CommandTable.h::VariantInfo/KNOWN_VARIANTS` and actual
identity/capability tests, TMP1x2 `test/variants/test_variants.cpp` for absent
wiring versus internal controls, and TUSB32x `Config.h::Variant` with profile
validators/tests. Inspect current relevant public/source, CLI/runner, check
registration and AGENTS. Record revisions, dirty state, paths and
adopted/adapted/rejected conventions. No sibling support or hardware claim
transfers to this family.

The lead owns shared schema, contracts and source-conflict decisions.

## Concrete source checks

Use [the pinned TI RTOS
comparison](../reference/library-family.md#ti-rtos-dp83822-concrete-behavior)
and the complete family catalogue. OUI `0x080028`, model `0x24`, revision 0 and
Linux ID `0x2000A240` are shared identity evidence, not IF/HF or temperature
detection. Inventory concrete gaps: RTOS only adjusts MDIX despite admitting
MII/RMII/RGMII; Linux exposes Magic/Secure-ON but omits the documented 64-byte
pattern engine. SNLS505H 7.4.2.1 permits direct/indirect standard-register
access except direct-only REGCR/ADDAR; preserve that exact DP83822 namespace
fact. Record shifted FX-enable write and reset-domain conflicts with source
locations and resolution steps. Assign each documented field/recipe to 63..67
before closure; do not let a vendor API's narrower scope replace exact SNLS505H
coverage.

## Exact subtype and addressing boundaries

Use one DP83822 ledger with parts I/H/IF/HF, fiber capability and temperature
attributes per Variant, and separate admission/physical rows. Table 8-4 controls
revision admission; `DeviceIdentity.mode` is UNKNOWN. Record REGCR function 10
conflict: section 7.4.2.1 rejects it, table 8-13 and sections 7.4.2.6/.7 use it.
Admit no-post-increment 01 only until physical evidence resolves 10.
Printed `0xDnnn` maps to DEVAD D and offset `0x0nnn` under section 7.4.2.9;
use part-keyed DEVAD3/7 policy, e.g. literal `0x0007,0x003D,0x4007`, then read.
Do not share DP83826's printed-address convention.

Distinguish five literal reset mechanisms: POR/RESET_N latch straps and reset
all; `0x001F=0x8000` is pin-equivalent and treated as relatching straps despite
BMCR table 8-1's contradiction; `0x001F=0x4000` restarts digital logic while
retaining registers (the prose's ambiguous software/soft reset); BMCR[15]
resets PCS, retains vendor registers and does not latch straps, with the exact
PCS set unspecified so re-read owned direct fields; MMD3 `0x3000[15]` resets
MMD3/MMD7/PCS while retaining vendor registers. Label the section 7.4.4.2
soft-reset interpretation as inferred; never select a recipe by its prose name.

Section 4 establishes FX parts; sections 7.4.1.1/.2 establish MII/RMII. FX with
RGMII is not stated and not excluded; table 7-10 RX_ER mode 3 can encode it,
physical NOT RUN. Assign MII TXCPSR `0x0042` to 64, including the [4:0] versus
[3:0] wording conflict; fixed RGMII RCSR shifts do not remove this phase control.

## Work and acceptance

Read SNLS505H original tables and explanatory sections, SNLA253, relevant
troubleshooting/EVM/tool notes, current TI repositories/SDKs and independent
primary implementations. Pin exact documents/commits/sections, licenses and
actual review depth. Qualify I, H, IF and HF individually: observed
identity/modes/revisions, declared suffix, ordering/package/temperature facts,
copper/fiber, MII/RMII/RGMII, clocks/delays, reset/defaults/straps, legal link
modes and every optional feature. Shared IDs do not identify suffix, wiring or
optical hardware. Share identical records only with explicit applicability and
cited equality, preserving per-part implementation/test/physical evidence.

Inventory every standard/extended register, non-reserved field and documented
procedure, including recipe-only addresses and legal values outside summary
tables. Establish exact destructive read, mixed write, counter and reset
semantics. Record contradictions and next evidence steps; do not guess an
undocumented field, relabel unfinished software unsupported, or treat a bounded
unsuccessful search as proof that no public source exists.

Preserve specific source corrections: SNLS505H table 8-23 has fixed 3.5 ns RGMII
shifts, RX RCSR bit 12 enables with 1 and TX bit 11 with 0; no arbitrary RGMII RCSR tap range. Fiber media and MAC-side interface require the exact applicability record below. Section 7.4.6 excludes
100BASE-FX from autonegotiation; do not inherit copper advertisement, MDIX, TDR
or 10BASE-Te recipes. A P4 adapter limitation does not remove generic PHY RGMII
capability or require a P4 RGMII build.

Assign every feature delta/inherited applicability to
[63](63-p22-copper-core.md), [64](64-p22-interface-clock.md),
[65](65-p22-fiber.md), [66](66-p22-diagnostics.md) or
[67](67-p22-optional-closure.md), with bounded responsibilities and independent
test expectations. Research can complete with explicit conflicts; dependent
implementation/support gates remain open. Update variants/reference/architecture
guidance with assigned subtype coverage. Run current ledger/reference/link
checks and verify any changed public contract documentation. Record source
dispositions, owners in 63-67 and open gates, then complete COMMON's report and
checkpoint.
