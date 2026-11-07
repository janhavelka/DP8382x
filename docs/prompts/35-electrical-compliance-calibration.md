# 35 - Implement electrical controls and finite compliance procedures

Follow [COMMON](COMMON.md) and its shared contracts, including
[CONVENTIONS](CONVENTIONS.md), [EVIDENCE](EVIDENCE.md) and
[FULL_COVERAGE](FULL_COVERAGE.md). Begin with the stage-specific rule reminder:
owned paths, fresh relevant sibling audit, selected names, one MDIO owner,
documentation/local checks and the recorded execution branch. Use available
subagents for disjoint work or independent review as COMMON specifies.

Prerequisites: [24](24-bist-generator-checker.md), [23](23-loopback-modes.md), [33](33-clock-interface-repeater.md),
[5](5-silicon-power-interface-ledger.md), [22](22-reconcile-recover.md). Verify the
concrete inputs below; retain unrelated open gates under COMMON.

## Scope, current evidence and work

Deliver documented analog/MAC electrical and timing controls, compliance
stimuli and the resistor-calibration contingency. Include BMCR[7] collision test
(table 8-19) with its interface/TX_EN prerequisites, finite admission, explicit
stop and restoration; observed register state alone cannot prove COL timing.
Keep persistent legal configuration distinct from finite disruptive test sessions. Reuse existing
configuration, diagnostic engine and restoration; no autonomous calibration/
reset loop or class per register.

Use [electrical/compliance sources](../reference/library-ti-features.md#electrical-and-compliance-anchors)
and original SNLS647I/SNLA266B. Retain VOD negative28..38=150..50%, positive
08..18=50..150%,6.25% steps, split MDIX fields and exact mandatory4A40 setup.
Eligible compliance is SNLA266B5-1/5-2/5-4 through shared crossover/BIST/loopback,
and COMPT10M codes 10..1D, each with HARD stop. Reject10M E/F. Keep codeE/F
legal in their separate 100M count context. The100M0428 branch keeps typed
validation/literal vectors but rejects before I/O under `compt_reserved`.
Other gates are `ioctrl_example`, `analog_rx_gain`, `hard_reset_straps`.
Do not expose generic reserved writes.

Reopen TMP1x2 diagnostic owner/result handling and PCA9555 primary/cleanup,
ambiguous write and detach-refusal tests. Inspect public documentation and actual
registrations. Reconcile SNLS647I electrical/calibration fields and exact
DP83826 recipes in SNLA266B/troubleshooting sources. Applicability must be explicit;
a family compliance guide does not authorize unrelated silicon values.

Enumerate every documented legal analog drive/timing/compatibility setting and
stimulus option with mode/interface/speed requirements, prerequisite hardware
and reset effects. Reserved bits are not tuning controls. Procedure-only
addresses and values remain bound to their exact source-backed recipe; no
arbitrary undocumented write escape. Reuse task 23's correct speed-specific
loopback where a procedure requires it.

Persistent controls use whole-profile validation and stable readback. Calibration
contingency reads CDCR once: if bit 14 high, clear only4000 with bit 15=0 and
immediately issue PHYRCR8000 in the same bounded owner step. The HARD procedure
provides completion evidence; nothing proves calibration completed. A second
command failure preserves the first effect and OP-14's floor applies whenever
HARD is attempted. No reset-until-good loop.

Compliance and collision test are two-call sessions under OP-9 with explicit
maintenance, bounded start/stop and application dwell. Failed/cancelled start
leaves temporary state and cleanupOwed; a later stop/restoration has its own
budget/result. No lock spans waits and no successful test erases restore failure.

Test every eligible option and each gated option's zero-I/O rejection. Session
writes must not set BMCR[11:10] or target PHYCR[4:0]; reject requested IEEE
power-down/isolation before entry. HARD destroys unrelated configuration, so
re-read identity/address and restore the persistent profile through later
`startInitialize` with finite readback. Inject each accepted-write, stop,
reset-completion and replay failure; retain uncertain stimulus and both results.

Compile a bounded maintenance-owner example and write fixture instructions
covering termination, probes, reference conditions and operator-controlled
entry/exit. Source sequences and host tests establish software behavior only.
Electrical compliance, EMC, resistor calibration accuracy and signal integrity
require actual suitable measurements and remain unqualified until then. Missing
fixtures do not excuse missing documented software controls.

Forced MDI/MDIX composes PHYCR[15:14], preserves bit 7 and does not import
undocumented bit 5 or RH[4:0] from script0021/4021. ScriptBISCR7101 becomes
owned mask701F/value7001; bit 8 is observed, not written. Forced profiles go
through `configure_forced`, including CR1[5]=1 though scripts omit it; Q19
qualifies that difference. The ledger keeps14 exact actions
`compliance_10_pattern_0`..`compliance_10_pattern_D` under the sourced COMPT
bit 4 exception, with normal ownedmask000F and reservedmaskFFF0 preserved.

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
