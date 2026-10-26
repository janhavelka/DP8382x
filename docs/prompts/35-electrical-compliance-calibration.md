# 35 - Implement electrical controls and finite compliance procedures

Follow [COMMON](COMMON.md) and its shared contracts, including
[CONVENTIONS](CONVENTIONS.md), [EVIDENCE](EVIDENCE.md) and
[FULL_COVERAGE](FULL_COVERAGE.md). Begin with the stage-specific rule reminder:
owned paths, fresh relevant sibling audit, selected names, one MDIO owner,
documentation/local checks and the recorded execution branch. Use available
subagents for disjoint work or independent review as COMMON specifies.

Prerequisites: [23](23-loopback-modes.md), [33](33-clock-interface-repeater.md),
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

Use [TI electrical/compliance facts](../reference/library-ti-features.md#electrical-and-compliance-anchors)
as independently checked vectors, not vendor-code endorsement. VOD negative
codes `0x28..0x38` mean 150% down to 50%; positive `0x08..0x18` mean 50%
up to 150%, both 6.25% steps. Convert user units to raw codes explicitly;
test raw defaults `0x30/0x10`, split negative-MDIX fields, endpoints and invalid
nonrepresentable settings independently. Reconcile 10BASE-Te
`030E=4A40` and its termination fields. For 100 Mbit/s stimulus, reconfigure by
clearing then setting `0428[4]`; pattern extension is bit 5. Preserve DPD bit 2.
COMP's 10 Mbit/s DP83825/26 BISCR `7101` differs from DP83822 `7108`; its
whole PHYCR `0021/4021` includes strap-loaded read-only address bits. Compose
owned controls without treating those bits as a programmable address change. CDCR bit 14 high requires clear then PHYRCR hard reset, not an
invented routine calibration-start API. IOCTRL and RX10BT code-range ambiguities
remain specific source gates.

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

Simple persistent controls use whole-profile validation, bounded application and
stable-owned readback. Extend reset/power/recover restoration and pin/interface
dependencies. An admitted resistor-calibration action has a finite sequence,
documented completion evidence, error/effect result and required restoration.
For CDCR[14] high, table 8-47 requires clearing it then immediately issuing
PHYRCR hard reset. Admit both commands as one bounded owner step without a
scheduled wait between them; a failed second command retains the partial effect.
Use source-defined verification rather than retry-resetting until a measurement
looks good.

Disruptive compliance patterns require maintenance admission, finite duration/
count, explicit start/stop and verified restoration even when silicon offers
continuous output. Capture only necessary owned state and handle untrusted
starting settings explicitly. Use the one operation slot, aggregate deadline
and existing separately bounded cleanup. No bus lock across waits. Cancellation
retains accepted output effects; failed stop/restore means the stimulus may
remain active and configuration is untrusted.

Test independent literal values/sequences for all documented options and legal
boundaries; reject reserved, wrong speed/interface and unavailable admission
before I/O. Test that entry/exit preserves read-only PHY address and unrelated
power/isolation settings even when a fixture script uses whole-word constants.
Inject failure/cancel after every mutation, including start accepted
before error, calibration completion, stop and each restore/readback. Assert
exact maximum transfers, token/result isolation, primary/cleanup precedence and
visible remaining stimulus after abandoned cleanup. Test persistent settings
across every applicable reset/power kind.

Compile a bounded maintenance-owner example and write fixture instructions
covering termination, probes, reference conditions and operator-controlled
entry/exit. Source sequences and host tests establish software behavior only.
Electrical compliance, EMC, resistor calibration accuracy and signal integrity
require actual suitable measurements and remain unqualified until then. Missing
fixtures do not excuse missing documented software controls.

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
