# 36 - Audit complete primary-chip feature coverage

Read [COMMON](COMMON.md), [ENGINEERING](ENGINEERING.md),
[CONVENTIONS](CONVENTIONS.md), [FULL_COVERAGE](FULL_COVERAGE.md) and
[VERIFICATION](VERIFICATION.md). Use [SOURCE_MAP](SOURCE_MAP.md) for current
source locations; historical reports do not replace a fresh audit.

At entry record owned paths, applicable rules, current source revisions/dirty
state, selected names, one MDIO owner, local checks and the recorded execution
branch. Resume that branch. Use disjoint implementation/test work or independent
read-only review when useful; choose available models/effort by task complexity
and record the rationale. The lead owns shared contracts, integration and Git.
Siblings remain read-only.

Prerequisites: [3](3-silicon-core-ledger.md), [4](4-silicon-diagnostic-ledger.md), [5](5-silicon-power-interface-ledger.md), [13](13-side-effect-register-access.md), [14](14-identity-capabilities.md), [16](16-hard-reset-restart.md), [18](18-forced-crossover-settings.md), [19](19-link-observation.md), [20](20-event-counter-acquisition.md), [21](21-passive-health.md), [22](22-reconcile-recover.md), [23](23-loopback-modes.md), [24](24-bist-generator-checker.md), [25](25-cable-manual.md), [26](26-cable-autorun.md), [27](27-interrupt-service.md), [28](28-power-states.md), [29](29-eee-negotiation.md), [30](30-legacy-mac-lpi.md), [31](31-wake-on-lan.md), [32](32-led-gpio.md), [33](33-clock-interface-repeater.md), [34](34-quality-link-loss.md), [35](35-electrical-compliance-calibration.md); only the concrete inputs used below are required.

## Scope, current evidence and work

Audit every row of the
[TI feature/source catalogue](../reference/library-ti-features.md) against the
current implementation, exact datasheet and any actual measurements. Require
explicit adopted/adapted/rejected dispositions and independent negative tests for
MCU odd-nibble polarity, LED1/3 swaps, RTOS LED0 truncation, Zephyr MMD selector,
basis-point/raw-code confusion, discarded IRQ samples and missing VOD_CFG3.
Revisit procedure-only EEE/loopback/compliance writes and remaining TDR, MSE,
GPIO, IOCTRL and nominal-timing limits. A table of numbers or two agreeing vendor
drivers cannot close a conflicting silicon requirement or a physical gate.
Return substantial gaps to their feature owner without reducing required scope.

Audit DP83826I software completeness against the entire authoritative register/
field/procedure ledger and independently inspect representative implementations.
This is a bounded integration review with focused corrections, not a replacement
for unfinished feature implementations in tasks 11–35. Own the review report,
coverage reconciliation and narrowly identified fixes with their tests/docs.

Refresh relevant sibling sources for the actual correction/review domains:
TMP1x2 Config/Status/operations/configuration, PCA9555 transaction/cleanup tests,
INA228 destructive diagnostics, and their example/CLI/test registrations.
Record paths, revisions and adopted/adapted/rejected findings. Ask independent
read-only reviewers to inspect disjoint difficult domains with suitable reasoning
effort; the lead resolves disagreements and owns shared contracts/Git.

Reconcile the complete datasheet summary, detailed register headings, feature
text and documented procedure-only addresses, including the original 79-entry
inventory as a cross-check, not a completeness oracle. Every non-reserved field,
legal encoding and multistep feature needs an exact source, public typed behavior,
source documentation, executing independent-vector/failure tests and clear
software disposition. Hardware-only/inapplicable claims require exact-part
evidence; constants or raw writes cannot complete a stateful feature.

Cross-check one-owner transport/MMD/RMW atomicity, aggregate deadlines, finite
cleanup, result/token lifecycle, partial destructive-event retention and raw/
external invalidation across features. Inspect all newly owned persistent fields
in the common reset/power/recovery restoration ledger. Look for conflicting
pin users, accidental optional activation, duplicated register procedures and
configuration trust restored by unrelated successful reads.

Explicitly revisit the known traps: 10BASE-Te setup and replay, robust Auto-MDIX
Force Mode, prohibited ANAR remote-fault advertisement, speed-specific MII
loopback, RXFCFG1 W0C and MAC ordering, active-sleep policy versus state,
EEE printed-address/wire mapping, PHYSTS forced interpretation, RECR arithmetic
and cable calibration. Closed factual conflicts need source evidence; unresolved
ones remain named required gates, never silently unsupported features.

Run focused cross-feature regression and portable consumer/Doxygen checks for
actual fixes. Use independent literal oracles and fail-at-every-boundary evidence
where corrected behavior warrants it. Do not repeat all unchanged checks merely
to inflate counts; ensure registered tests execute and Release checks remain
active. Review planned CLI/SDK/runner mappings without claiming those later
surfaces already exist.

If a large feature or essential source fact is missing, identify its responsible
task and exact remaining acceptance, leave this closure incomplete and continue
independent eligible downstream work. Do not absorb another full feature project
into this audit or mark the whole primary implementation complete by prose.
Report software, SDK and physical evidence separately, including optional-runtime
features that remain mandatory library scope.

## Documentation, verification and checkpoint

Ship accurate public Doxygen and procedure citations with this behavior: units,
legal states, ownership, output validity, I/O effects, deadlines and cleanup.
Update API contracts, affected guides, restoration/feature evidence and the stage
report/progress. Review README, CHANGELOG and relevant example/test/tool guides;
update affected claims and record why others need no change. Future APIs and
physical qualification remain explicitly unimplemented/unverified.

Run registered focused host tests with active Release checks, relevant portable
consumer/source-documentation/link checks, the reference checker and
`git diff --check`. Record actual commands and separate PASS/FAIL/NOT RUN/BLOCKED
for host, SDK, packages and hardware where relevant. Missing source evidence
blocks the affected requirement; it cannot become unsupported silicon or close
a required gate. Continue independent eligible work.

Review the final diff for simple control flow, stale comments, duplicated
procedures and undocumented errors. Commit and sync checked tests/docs/code on
the same execution branch after this task and each coherent verified unit.
The lead fetches, safely integrates, pushes and verifies upstream equality;
report exact sync failures. First-execution remote CI dispatch, polling and
waiting belong only to checkpoint **71**, never this task.
