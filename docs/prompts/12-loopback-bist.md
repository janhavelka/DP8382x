# 12 - Implement supported loopback and BIST

Read [COMMON](COMMON.md) and the linked engineering, CLI and verification contracts
before working. Prerequisites: 01-11.

## Fresh audit and owned work

Inspect explicit mutating selftests, finite diagnostics and restoration in
PCA9555/MB85RC/TMP1x2; keep their pin/memory/sensor effects out of PHY procedures.

The lead owns shared API/architecture/progress changes. Delegate disjoint tests or
read-only review if useful; siblings remain read-only. Record concrete source paths and
adopted/adapted/rejected conventions in this stage's report.

## Work

Enumerate and implement all documented loopback and BIST/PRBS operating options,
including generator/checker selection, single/continuous behavior, packet/IPG
controls, lock/synchronization, counter freeze/clear and applicable stimulus
controls. A single analog-loopback smoke test does not complete this feature
family. Resolve exact register/recipe interactions and fixture limitations.

Implement only exact DP83826I loopback/BIST modes whose register sequences and
prerequisites are documented. Make capability, link/interface restrictions,
duration/count bounds and result meaning explicit. Distinguish internal PHY test, MAC
loopback and actual cable/frame tests; do not imply a PHY BIST validates the application
frame path.

Require owner admission into maintenance before disturbing an active link. Capture owned
configuration, run bounded steps through the operation mechanism, collect counters once,
and restore/verify the applicable state. Stop/cancel/deadline failure records both
primary outcome and cleanup outcome, with configuration left untrusted if restoration
cannot be proved.

Document BIST counter widths, reset/clear/overflow rules, expected test setup and which
outcomes are inconclusive without a peer. Never label lack of errors during an unstarted
test as PASS. No hidden test activation during initialization or passive status.

Keep live/destructive diagnostic reads behind the single collector. Surface results for
CLI/runner use without adding rendering code to core. Cite exact source sections and
retain unresolved field interpretations as blocked implementation rows and open
completion gates. Use unsupported only where exact silicon/mode evidence proves
the feature is absent; missing evidence cannot close a required capability.

## Acceptance and handoff

Test mode gating before I/O, exact start/stop sequences, deadline/cancel at every
side-effect boundary, no-counter-data cases, wrap/saturation per source and restoration
errors. Independently verify literal sequences. Host tests establish state/protocol
behavior only; physical signal/path checks remain stage 30.

Expected output: Bounded loopback/BIST with precise result and cleanup semantics.
Include source documentation, relevant guides, feature/evidence matrix and progress
updates. Apply COMMON's completion rules; report actual commands, results and remaining
gates. Continue only when the concrete required inputs exist; COMMON explains how
unrelated gates stay open.
