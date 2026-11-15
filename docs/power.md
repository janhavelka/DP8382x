# Power policy and observations

`startConfigurePower` copies one complete `PowerSettings` into the existing
operation slot without bus or clock access. Polling applies active-sleep policy,
the owned software IEEE power-down request and optional PLL disable. It requires
initialized identity, maintenance admission and an application guarantee that
management/reference clocks remain available. IEEE power-down and PLL disable
also require explicit approval of generated-clock loss. The core changes no pins,
rails, MAC state, IRQ route or scheduling policy.

The four stage-28 ledger procedures are `power_active_sleep`, `power_ieee`,
`power_pll_control` and `power_deep`. The first three have executing control
procedures and host tests. **Deep-power-down remains source-blocked**: SNLS647I
8.3.6.3 specifies entry but no complete exit, opaque-register composition or
clock-readiness contract. `DEEP_POWER_DOWN` returns `UNSUPPORTED/SOURCE_GATE`
before callbacks or desired-state changes. The `deep_power_exit` gate requires
exact-part TI clarification or admitted evidence establishing the complete
procedure. No entry-only implementation is exposed.

## Policy, indication and physical state

| Evidence | Meaning |
| --- | --- |
| `desired.activeSleep` and `trust` | Configured automatic policy. Successful enable can occur while linked and awake. |
| `requestedPowerDown` | Copied software intent. Never inferred from the hardware readback. |
| `observedBmcrPowerDown` | BMCR software/pin OR. There is no independent actual IEEE power-state register. |
| `NORMAL_INDICATED` | Power-down readback clear and BISCR normal indication; no measured clock/link readiness. |
| `ACTIVE_SLEEP_INDICATED` | BISCR sleep indication with active-sleep policy observed enabled and power-down clear. |
| `WAKE_BLOCKED_OR_UNCONFIRMED` | A NORMAL request still reads power-down high. One clear write was issued; no repeated writes follow. |
| `observedStale`, raw validity and time | Partial/historical evidence remains explicit. Zero is a valid timestamp when `timeValid` is true. |

`readPowerStatus` performs exactly three reads in one terminal batch: BMCR,
PHYSCR, BISCR. BISCR uses the existing collector. This is ordered acquisition,
not simultaneous silicon state. Ordinary output changes only after the complete
successful pass. Each valid word is retained even if a later read fails;
contradictory owned controls immediately dirty trust. A reset indication invalidates
identity/readiness even if a later callback/release fails. Failed aggregate
observations remain stale and never become evidence of sleep or wake. A live read
does not upgrade dirty configuration trust. `powerSnapshot` is passive.

## Finite control procedure

Source authority is SNLS647I pp28-29 section8.3.6, p55 table8-19, p65 table8-35
and p69 table8-40. The [ledger](registers/dp83826i.json) owns field values;
[independent vectors](register-vectors.md) give literal arithmetic.

1. Read BMCR and PHYSCR. Reject active reset or unowned interrupt stimulus.
2. If PLL disable is being cleared, require observed IEEE power-down and clock
   approval, clear PLL disable and verify it before clearing the IEEE request.
3. Apply and verify active-sleep policy, preserving PLL and unrelated controls.
4. Write the software power request once using an atomic RMW. Neutralize reset
   and AN restart, preserve isolation and other stable fields. Verify the OR
   readback. A blocked clear returns `CONFIG_MISMATCH/POWER_STATE` immediately.
5. If requested, set PLL disable only after IEEE power-down readback is established,
   and verify its control. Read BISCR once for the distinct active-sleep indication.

The normal path uses nine frames in seven callbacks; a PLL enable or disable adds
three frames in two callbacks. No wait for active sleep, link, a partner or PLL
lock is invented. The 1.4-second NLP interval is not a software completion deadline.
Callbacks share the operation deadline/frame allowance; short poll slices yield
before a complete atomic batch. Controller ownership never spans polls or waits.

Failure or cancellation retains intent, accepted/possible effects and the
independent primary/release statuses. OP-9 debt blocks further mutation. A later
`startInitialize` has its own token, budget and result; no cancellation cleanup,
automatic retry or rollback occurs. `end` stays bus-silent and refuses live debt.
Application disposition through `abandonCleanup` makes no hardware wake claim.

## Configuration and restoration

Every semantic BMCR writer composes bit11 from owned software intent, including
profile application and diagnostic restoration. Without explicit power ownership,
the earlier active-profile preflight still requires observed power-down zero.
Pin assertion between that preflight and the atomic read cannot become software
power-down. Owned IEEE power-down is retained across profile application; AN
restart/wait/page exchange is not requested while software power-down is owned.
Loopback requires awake intent. Reset commands retain their exact literal words.

Initialization restores persistent power last, after VOD, link, diagnostic,
autorun and IRQ state. A non-HARD initialization first reads the live power
controls when power is owned; if PLL disable is active it clears and verifies it
while IEEE power-down remains asserted. This avoids PCS clearing BMCR while the
vendor PLL-disable bit remains set. The preparation uses live evidence even after
an accepted PLL write timed out. HARD reloads PLL controls itself. No-reset
restoration uses the same preparation and final replay. VERIFY and ordinary
CONFIGURE tails only read owned power controls. An invalid live power prerequisite
fails explicitly and preserves any earlier operation effects/debt.

| Transition | Retained intent and evidence |
| --- | --- |
| PCS | Desired policy retained; BMCR reset effects and retained vendor PLL state handled by preparation and final replay. |
| HARD | Identity/straps/configuration invalidated; same full replay, existing shared-bus silence unchanged. |
| RESTART | Register-retention semantics remain distinct; owned controls are re-established and verified. |
| External reset/POR notification | No I/O; desired policy retained, observations/trust stale. Owner supplies physical readiness before initialization. |
| Ordinary power change | Identity/profile remain separately owned; link observation invalidated. No implicit optional-feature activation. |

## Examples and qualification

[Power.h](../examples/portable/Power.h) compiles and executes through the tests-OFF
portable consumer: enable policy while normal, request IEEE power-down with PLL
disable, request NORMAL, acquire once and read cached copies. Failure results
are retained separately from a later initialization/restoration result.

Host tests establish literal composition, ownership, finite work and evidence
handling. Q6 remains NOT RUN: measure generated-clock loss/recovery, management
access and ENHANCED pin assertion/release with identified board/straps, external
reference, PHY/MAC clocks and supply instrumentation. Exercise active-sleep
partner appearance/disappearance independently from IEEE request control. Physical
clock lock, actual power, link timing and DPD exit are not proved by fake readback.
