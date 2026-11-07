# 64 - Implement DP83822 interface, clock and fixed RGMII shifts

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md),
[FULL_COVERAGE](FULL_COVERAGE.md) and [VERIFICATION](VERIFICATION.md). Required
inputs are exact subtype/interface facts from [62](62-p22-source-subtypes.md),
verified reset/core integration from [63](63-p22-copper-core.md) and the
existing clock/interface configuration contract. Physical clock/timing
qualification remains a separate fixture gate.

## Current evidence and ownership

Reopen TMP1x2 configuration validators/tests and TUSB32x
`Config.h::DeviceProfile` with actual profile admission tests; inspect MB85RC
variant metadata/selection for declared capability discipline. Read current
relevant headers/source, CLI/runner fixtures, SDK MAC/interface admission,
registration and AGENTS. Record revisions, dirty state, paths and
adopted/adapted/rejected conventions. Do not borrow a sibling's I2C platform
build matrix for an EMAC target.

The lead owns shared interface types, capabilities, adapter admission and
integration.

## Concrete source checks

Use [the exact clock/shift
table](../reference/library-family.md#interface-clock-analog-and-led-facts).
RCSR `0x17` RX/TX shift-mask `0x1800` independent vectors are `0x0800` off/off,
`0x1800` on/off, `0x0000` off/on, `0x1000` on/on. Test both prior states, so
either shift can be enabled or disabled independently. SNLS505H table 8-23
requires after setting RMII bit 5: write `0x0462=0x4300`, then `0x001F=0x4000`.
Register `0x0462` also owns GPIO/clock controls: reconcile the whole-register
recipe, restart effects and final mux state before implementation. Admit the transition and restore compatible fields under the explicit
source/inference rules below. Do not substitute an inferred RMW for the
documented recipe or silently overwrite another owner. SNLS505H 7.4.2.1 permits
standard 0..31 direct or indirect access through vendor DEVAD 31; REGCR `0x0D`
and ADDAR `0x0E` are direct-only exceptions. Test RCSR `0x17` access with
complete transaction ownership and literal selector frames; this DP83822 fact
does not establish another part's aliases or real traces. GPIO clock selectors
0, 1, 2, 4, 6, 7 are documented; 3/5 reserved. Validate mux conflicts, preserve
RC FIFO evidence and keep actual clock/timing measurement gates separate.

## RMII transition and clock evidence

Run literal `0x0462=0x4300`, then `0x001F=0x4000`, only for software RCSR[5]
0-to-1; strap-selected RMII runs no recipe (7.4.1.2). This word selects RX_D3
50 MHz output and LED_1 tri-state. Read `0x0462` before changing RCSR and reject
owners requiring different [14:12]/[10:8] values. If [6:4] or [2:0] is nonzero,
owned or strap-loaded, reapply it after restart by masked write, explicitly
INFERRED and physical NOT RUN. Keep OPEN that the literal selects Leader
reference even in Follower mode. Leaving RMII (RCSR[5] 1-to-0 or [9] 0-to-1)
returns `0x0462[10:8]` to zero by masked write, then digital restart: inferred
from table 5-1 data-pin requirements because no reverse recipe is documented.

Include GPIO1 clock output `0x0462[6:4]`, active with [2:0]=011; infer mutual
exclusion with FX SD on pin 24. Implement MII TXCPSR `0x0042` phase control with
its [4:0]/[3:0] wording gate. Fixed 3.5 ns choices apply specifically to RGMII
RCSR[12:11]. Cite section 4 and 7.4.1.1/.2 for FX/MII/RMII; FX+RGMII is not
stated or excluded, table 7-10 RX_ER mode 3 encodes it, physical NOT RUN.

## Work and acceptance

Implement every documented applicable MII/RMII/RGMII interface and clock
control, with exact subtype/mode applicability, direction/source prerequisites,
reset values, units and safe programming order. Board pin assignments,
oscillator selection and physical strap changes stay application-owned.
Hardware-only properties need cited dispositions rather than invented
programmable APIs. Preserve unrelated/reserved fields and incorporate owned
controls in reset/recovery restoration without overwriting strap or
external-owner state.

SNLS505H table 8-23 specifies fixed 3.5 ns RGMII shifts. Model explicit fixed
shift choices: RX RCSR bit 12 enables with 1, TX bit 11 enables with 0. Test
both polarities and reset values; do not invent arbitrary delay taps/ranges.
Validate combinations before callbacks. An invalid enum or incompatible
subtype/interface choice produces no I/O or configuration mutation.

Portable PHY capability includes source-proved RGMII even when the current P4
MAC cannot consume it. Adapter admission must explicitly reject unavailable MAC
interfaces while preserving generic device capabilities and useful diagnostics.
Do not require a nonexistent P4 RGMII firmware build or pretend an SDK header
shim proves a supported MAC path. Preserve the exact IF/HF medium/interface evidence and unresolved physical
combinations for prompt 65.

Use independent literal vectors for every legal/boundary clock and shift
encoding and subtype/interface row, including opposite TX/RX polarity. Check
complete read/write order, neutral mixed-register composition and reset/restore
traces, including nondefault owned GPIO/clock settings across the RMII recipe
and zero-transfer rejection of ownership/pin conflicts. Apply VERIFICATION's
failure/deadline/cleanup and cached-state cases to every changed interface/clock
procedure. Test adapter rejection and CLI/runner capability agreement through
actual code, not grep alone.

Update Doxygen and interface/API/adapter/fixture guides with exact units,
restrictions and reset/recipe effects. Run focused host/CLI/Python/docs checks,
real supported SDK links and package checks for changed exports. Record
unmeasured timing claims separately and complete COMMON's report and checkpoint.
