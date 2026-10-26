# 33 - Implement clock, interface and repeater controls

Follow [COMMON](COMMON.md) and its shared contracts, including
[CONVENTIONS](CONVENTIONS.md), [EVIDENCE](EVIDENCE.md) and
[FULL_COVERAGE](FULL_COVERAGE.md). Begin with the stage-specific rule reminder:
owned paths, fresh relevant sibling audit, selected names, one MDIO owner,
documentation/local checks and the recorded execution branch. Use available
subagents for disjoint work or independent review as COMMON specifies.

Prerequisites: [32](32-led-gpio.md), [18](18-forced-crossover-settings.md),
[5](5-silicon-power-interface-ledger.md). Verify the concrete inputs below; retain
unrelated open gates under COMMON.

## Scope, current evidence and work

Deliver every documented DP83826I clock/interface and RMII repeater option,
including BMCR[10] MII isolation and its RMII-leader 50-MHz-clock effect under
table 8-19. Isolation is separately owned from BMCR power-down. Test isolate/
release, preserved power request and application clock admission on both paths.
Apply exact mode/reset constraints, shared pin validation and restoration. Extend
the existing configuration surface and procedures; the portable library never
owns oscillator wiring, MAC initialization or board pin assignments.

Verify the [interface anchors](../reference/library-ti-features.md#led-interface-and-link-quality-controls)
against SNLS647I tables 8-41/73/84 and the pinned TI Zephyr implementation.
RCSR `0017` uses RMII enable `0x0020`, follower reference `0x0080`,
follower-only TX shift `0x0100` and revision `0x0010`.
Elasticity codes 0/1/2/3 map to 14/2/6/10 bits, not ascending sizes.
IO_CFG1 `0302[8]` selects RX_DV for repeater and bit 6 selects LED1 versus
CLKOUT25M in ENHANCED mode. GENCFG `0456[3]` selects 120 ns versus 200 ns
IPG; keep it distinct from CR2 bit 2 RX_ER-during-IDLE detection.
Test mode/clock wiring assumptions, collector-safe RCSR reads and exact
per-pin capability; configuration alone cannot verify a two-PHY repeater.

Reopen TMP1x2 configuration/dependency validators, PCA9555 complete-profile/
shadow tests and relevant native sibling example board-adapter boundaries.
Inspect actual source/registration and FieldCore owner adapters as read-only
examples of application responsibility. Recheck SNLS647I clock, MII/RMII,
repeater and compatibility tables and reset/strap descriptions.

Build a legal matrix covering available interfaces, reference-clock direction/
selection/output, interface timing/compatibility and repeater settings with
their exact dependencies. Distinguish hardware straps, POR-only properties,
software-programmable fields and observed mode. A software API cannot change
a hardware-only property merely because a related part offers such a register.
Additional family interfaces are qualified later, not inherited now.

Validate the whole requested profile and pin users before any transfer. Clock/
interface changes may break the active MAC or MDIO access: explicitly identify
permitted runtime changes, required stopped-driver/maintenance state and
application clock/pin sequencing. Reject unsupported live transitions before
writes without declaring the silicon feature absent. No automatic clock source
switch, reset GPIO control or hidden board policy.

Implement source-defined ordered application/readback, stable-owned masks and
command-neutral composition with aggregate deadlines. Keep desired settings,
observed mode and verified configuration distinct. Extend every applicable reset/
power recovery path using the same restoration ledger and currently implemented
pin/IRQ/WoL ownership rules. Preserve hardware effects if an accepted transition
times out or readback becomes unavailable; no guessed rollback/retry.

Use bounded direct operations for simple fields and the existing cooperative
engine only for real waits. Complete transactions remain atomic and the bus lock
does not span application scheduling. Cancellation or local end cannot certify
restored clocks or stopped hardware; failed/denied cleanup retains uncertainty
and separate primary/cleanup status.

Test independent exact encodings and legal combinations for all documented
options, invalid/reserved values, mode mismatches, pin conflicts and unsafe
live-transition admission with zero I/O. Inject every write/readback/restore
failure, including loss of access after an accepted clock-related write.
Check reset-loaded versus runtime retention and exact operation bounds.

Compile owner examples showing the required application sequencing and a
rejected live transition. Document electrical timing/clock-source assumptions
and physical measurement gates. A host register test cannot prove a working MAC
clock, repeater link or compliant board timing.

## Documentation, verification and checkpoint

Update public Doxygen, procedure citations, affected API/user/test/tool guides,
coverage, stage report and progress with this behavior. Review README/CHANGELOG
and record relevant no-change reasons. Run the focused registered host tests
above with active Release checks, affected portable-consumer and Doxygen checks,
`python tools/verify_references.py` and `git diff --check`.

Apply COMMON's final maintainability review and evidence reporting. Keep source
gaps, SDK/package results and physical qualification separate. The lead commits
and syncs this complete task and coherent verified units on the same execution
branch, verifies upstream equality, and reports any sync failure. First-set
remote CI runs and status checks belong only to prompt 71.
