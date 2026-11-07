# 73 - Execute admitted feature, medium and timing qualification

Read [COMMON](COMMON.md), [CONVENTIONS](CONVENTIONS.md),
[FULL_COVERAGE](FULL_COVERAGE.md) and [VERIFICATION](VERIFICATION.md). Inputs
are implemented feature paths, explicit runner suites/fixtures, linked firmware
and the applicable fixture admission record from [72](72-hw-link-campaign.md) or
an equivalent freshly verified record. An unrelated missing fixture does not
block an available independently authorized case. Required physical evidence
remains open until actually obtained.

## Current evidence and ownership

Reopen TCA9548A `tools/test_hil_artifacts.py` and actual runner behavior, MB85RC
runner strict-failure/restoration tests, plus current PHY CLI/firmware and
Python suite admission. Inspect AGENTS, relevant registration, exact
part/board/netlist and instrument setup. Record revisions, dirty state, paths
and adopted/adapted/rejected conventions. Read exact TI procedure restrictions
and equipment limits, not only a previous passing campaign report.

The lead owns fixture safety/admission and physical interpretation.

## Concrete public-source inputs

Use [feature timing and field facts](../reference/library-ti-features.md) with
[EVIDENCE](EVIDENCE.md). Measure correct LED pin/field behavior, wake indication
pulse/acknowledgement, actual reset/clock/MDIO intervals and admitted diagnostic
restoration. Explicitly distinguish nominal EEE timing from maxima, and use an
independent known stimulus for TDR/WoL/BIST. A matching register dump or LED
flash cannot qualify derived cable distance, clock compliance or EtherCAT
latency. Record instrument/raw-data limits and correct contradicted software
assumptions without overwriting prior evidence.

## Required unmeasured cases

Extend the finite cases with Q1 EEE exact low/full-offset probe, gated writes and
final hard reset, then peer advertisement/LPI collector evidence; Q2 forced TX
LPI; Q3 asymmetric MAC WoL level-neutral/ack behavior and reset bit 0; Q4 direct
versus MMD LEDCR alias; Q6 power-pin/BMCR state; Q7 both TDR recipes with known
open/short cables; Q8 reverse/analog restart comparison; Q9 interrupt level,
disabled-enable latching and RXFS retention; Q12 PHYCR[5]/LED0; Q16 RECR boundary
injection; Q17 controlled RMII FIFO polarity/clear; Q18 CR3 strap readback; Q19
compliance stimulus with robust bit set. Preserve the exact stimuli, decision
branches and limits in REMAINING_PROMPTS_AUDIT section 8, including Q1's rule
not to consume 3.1/3.22 during its raw probe. Q1 closes only observed offsets;
unproduced wake-fault evidence remains source-implied.

Without admitted equipment mark 73 BLOCKED (readiness done, physical NOT RUN)
and continue 74-80. The execution set remains PARTIAL; SDK links and host-driver
tests still run. Console transport/baud is recorded for every firmware case.

## Work and acceptance

Freeze the finite campaign matrix from implemented features, intended physical
support claims and admitted fixtures; list untestable rows with their required
fixture. Run applicable BIST/generator/checker/loopback, TDR/cable, interrupt,
LED/GPIO, EEE/LPI, WoL/filtering and PHY power/wake campaigns on explicitly
named exact parts/revisions/interfaces/media. Define finite duration,
initial/owned pre-state, partner/equipment configuration, correlated request
identity, required observations and restoration verdict before execution. Test
supported success, no-link and inconclusive diagnostic outcomes without
converting an inconclusive TDR result into a cable length. An absent IRQ,
optical or wake fixture is BLOCKED/NOT RUN, never a pass inherited from another
board.

For reset/strap/clock/RMII/RGMII timing claims, retain instrument configuration,
traces and measured limits. Packets passing or LEDs toggling do not prove signal
compliance, fixed delay accuracy, deterministic latency or EtherCAT cycle/servo
qualification. Fiber qualification needs actual suitable optics and a supported
MAC-side interface; do not imply P4 RGMII evidence. Temperature/package or
another suffix/revision does not inherit a pass. Electrical compliance needs its
own explicit setup and claimed limits. PHY low-power controls remain in scope;
board power delivery and PoE remain outside the library.

Retain firmware/SDK/part/mode/fixture identity and exactly two artifacts per
run, `<run>.txt` and `<run>.json`, under 48-52. Verify transcript hash/length,
schema and worst-case verdict; preserve failed retries/interruptions. Failed
restoration cannot yield PASS. Fix defects with host regressions and rerun only
affected authorized physical cases.

Missing equipment/permission leaves a precise physical gate with next
fixture/case, while readiness/host work and unrelated stages can finish.
Reconcile exact feature-matrix rows, README qualification, CHANGELOG,
hardware/fixture guides, report and a linked progress row; record inspected
no-change reasons. Run focused local checks and check final diff. Commit/sync
verified units and available work on the recorded branch, keeping BLOCKED
evidence visible. Preserve prompt 71's historical CI SHA; no remote
dispatch/poll/wait occurs here.
