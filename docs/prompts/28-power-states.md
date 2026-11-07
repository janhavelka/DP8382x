# 28 - Implement active-sleep policy and explicit power states

Follow [COMMON](COMMON.md) and its shared contracts, including
[CONVENTIONS](CONVENTIONS.md), [EVIDENCE](EVIDENCE.md) and
[FULL_COVERAGE](FULL_COVERAGE.md). Begin with the stage-specific rule reminder:
owned paths, fresh relevant sibling audit, selected names, one MDIO owner,
documentation/local checks and the recorded execution branch. Use available
subagents for disjoint work or independent review as COMMON specifies.

Prerequisites: [22](22-reconcile-recover.md), [5](5-silicon-power-interface-ledger.md).
Verify the concrete inputs below; retain unrelated open gates under COMMON.

## Scope, current evidence and work

Deliver active-sleep policy plus documented IEEE power-down, deep-power-down,
disable and wake transitions. Keep policy configuration distinct from observed
power state and partner-dependent behavior. Extend one shared configuration/
operation/restore implementation; local `end` remains bus-silent.

Verify the [power facts](../reference/library-ti-features.md#interrupts-power-eee-and-wol)
against SNLS647I 8.3.6 / table 8-40. Active-sleep configuration is PHYSCR
mask/value `0x7000/0x6000`, while BISCR bit 8 is 0 sleeping/1 normal;
the documented 1.4-second NLP interval is not a guaranteed time to find a partner.
IEEE power-down uses BMCR `0x0800`; DPD entry first sets `0x0428[2]`,
then enables IEEE power-down. Preserve BMCR isolation independently; setting or
clearing power-down must not silently change the caller's interface isolation.
DPD stops the PLL. Confirm exit timing/order and
clock availability independently; do not assume reversing the two writes is
a complete wake procedure. Recheck conflicts with compliance controls at 0428
and preserve separate policy, observed state and transition validity.

Reopen MB85RC `src/MB85RC.cpp::enterSleep`, `wake`,
`_ensureAwakeForI2c` and public sleep/capability declarations. Inspect
SCD41/RV3032-C7 explicit power command implementations/tests and PCA9555
uncertain-effect/cleanup cases. Preserve transition uncertainty, but do not copy
memory wake timings or no-clock policies into the PHY.

Use SNLS647I section 8.3.6.1 precisely: active sleep is enabled by
`PHYSCR[14:12]=110`; hardware enters/wakes according to partner detection,
and BISCR[8] reports current sleep/normal state. Policy configuration can
complete successfully while linked and awake. Do not wait for sleep entry or
promise partner-dependent wake/link timing as a finite software completion gate.

For IEEE/deep power transitions document prerequisite access/clock state, effects
and bounded source-defined waits. BMCR[11] reads the software/pin OR, so report
`requestedPowerDown` from owner intent and `observedBmcrPowerDown` from hardware;
there is no separate actual-state register. Compose bit 11 from desired intent,
never sampled readback. After one clear write, observed1 means
WAKE_BLOCKED_OR_UNCONFIRMED and never triggers repeated writes. BISCR[8] reports
active sleep only. Coordinate PHY-generated clock loss with the application;
Q6 measures the ENHANCED pin-release hazard.

Preserve desired settings and extend reset/recovery/restoration for each power
kind, using stable source-supported verification. Pin, oscillator, GPIO reset,
power rails and application MAC/link policy remain outside core. Power delivery/
PoE is not a PHY library feature. No default low-power activation or autonomous
power cycling; explicit opt-in settings may be replayed by initialization/
recovery when owned.

Cancellation ends immediately and leaves OP-9's barrier after accepted or
uncertain mutation. A later restoration operation has its own finite budget and
result; no refreshed per-read timeout, lock across waits or local-setter wake
claim is permitted.

Test literal policy/power sequences, active-sleep enable while linked and awake,
partner-dependent state changes, invalid/unsupported zero-I/O requests and all
reset/power retention differences. Inject every write/read/restore failure,
accepted-effect timeout, never-completing explicit transition, cancellation and
cleanup denial. Verify exact bounds and no false asleep/awake publication.

Compile policy and explicit-transition owner examples. Document source-bounded
software operations separately from unbounded environmental link/wake outcomes
and unperformed physical clock/power measurements.

Own `power_pll_control`: PHYSCR[15] mask8000 is separate from DPD0428[2],
admitted only in IEEE power-down with explicit application clock-loss approval.
Active-sleep mask7000 must never alter PLL disable.

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
