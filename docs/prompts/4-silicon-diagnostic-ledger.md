# 4 - Research diagnostic, event and counter procedures

Read [COMMON](COMMON.md), [FULL_COVERAGE](FULL_COVERAGE.md) and
[SOURCE_MAP](SOURCE_MAP.md). Prerequisite:
[3](3-silicon-core-ledger.md)'s schema, source policy and core register facts.
Record a short rule reminder, owned ledger sections, current-source audit,
documentation/checks and same-branch checkpoint.

## Complete outcome

Complete the primary-part ledger and independent vectors for loopback, BIST,
cable diagnostics, event acquisition, counters and signal-quality evidence.
Every documented control, result and procedure in these families has a source,
legal interpretation or explicit conflict, future implementation owner and
host/fixture validation plan. No hardware or software support follows from
source coverage alone.

## Work and source review

Reopen relevant TMP1x2/PCA9555 operation-result and snapshot declarations,
actual failure/cleanup implementations and tests. Inspect MB85RC uncertainty
and retained-result patterns. Record revisions/paths and adopted/adapted/rejected
principles; their sensor or memory timing does not specify PHY procedures.

Audit SNLS647I's feature prose and detailed tables alongside applicable
SNLA423A/EVM material, SNLA344C, SNLA494 and SNLA495. Search exact TI
clarifications and current primary drivers where a contradiction remains.
Record source version, page/table and per-part applicability. EtherCAT ESC
workarounds and MII-only Fast Link Drop do not transfer automatically to the
P4 RMII master configuration.

- Specify every loopback mode's speed/interface prerequisites, settings,
  activation, observation, stop and restoration. Preserve the MII-loopback
  prose conflict: detailed BMCR table 8-19 requires BISCR[4:0] `00100` at
  100BASE-TX and `00001` at 10BASE-Te. Use independently reviewed vectors,
  absent stronger exact-part evidence; do not generalize an all-speed sentence.
- Cover BIST generator/checker/PRBS, single/continuous traffic, packet and
  interpacket controls, lock/freeze/counter semantics and a finite result.
  Running or receiving traffic is not itself a passing result.
- Inventory manual and link-down autorun cable diagnostics, pulse/threshold,
  averaging, DC/segment/capture controls, supported result banks, shadowing,
  stop/restore and link effects. Reserved bank tables cannot become decoded
  peaks just because names suggest them. Derived lengths/classifications need
  exact calibration evidence; raw capture remains a distinct documented result.
- Define MISR1/MISR2/FCSCR/RECR read effects, latch/current/history distinction,
  clear/overflow/saturation and interrupt interactions. A collector must retain
  each successfully consumed sample even if a later read fails; ambiguous read
  failures may lose history. Keep the RECR width/saturation conflict explicit.
- Audit PHYSTS forced-mode interpretations with core-link facts, MSE address
  `0x0218` versus SNLA423A `0x0225`, RMII status polarity and any derived-unit
  claims. Do not infer SNR, counter width or polarity from a convenient fake.

For each accepted procedure record admitted state, owned registers/masks,
actions/waits, finite primary/cleanup budgets, cancellation, partial effects,
restoration verification and reset interaction. Identify physical fixtures and
instrumentation separately from software capability.

## Delegation, documentation and acceptance

Assign disjoint diagnostic/event source reviews; use stronger reasoning models for
destructive access and contradictory tables, with the lead merging the shared
ledger. Review independent literals directly against original PDFs rather than
production constants. No subagent edits siblings or Git.

Update device behavior, register/vectors guides, feature evidence and conflict
list together. Review README/CHANGELOG and the planned procedure/Doxygen
citations. Verify complete topic coverage against detailed tables and feature
text; unresolved source gates have exact evidence needed and affected scope.
Run local reference/link/diff checks and update report/progress. Commit and
sync this complete research task and coherent checked units on the same branch.
Remote CI is reserved for prompt 71.
