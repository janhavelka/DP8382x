# 30 - Implement bounded legacy-MAC LPI sessions

Follow [COMMON](COMMON.md) and its shared contracts, including
[CONVENTIONS](CONVENTIONS.md), [EVIDENCE](EVIDENCE.md) and
[FULL_COVERAGE](FULL_COVERAGE.md). Begin with the stage-specific rule reminder:
owned paths, fresh relevant sibling audit, selected names, one MDIO owner,
documentation/local checks and the recorded execution branch. Use available
subagents for disjoint work or independent review as COMMON specifies.

Prerequisites: [29](29-eee-negotiation.md), [28](28-power-states.md),
[22](22-reconcile-recover.md). Verify the concrete inputs below; retain unrelated open
gates under COMMON.

## Scope, current evidence and work

Deliver exact documented legacy-MAC EEE/TX LPI coordination as complete finite
owner-admitted sessions, including entry, status acquisition, exit and verified
restoration. Reuse EEE configuration and the existing operation engine; no
independent LPI task or implicit background control.

Re-open SNLS647I 8.3.4/7.6 and the
[LPI timing facts](../reference/library-ti-features.md#interrupts-power-eee-and-wol):
after qualified EEE setup, BMCR bit 9 restarts AN; EEECFG3
`0x04D1[12]` (`0x1000`) starts/stops forced TX LPI. The PHY does
not buffer MAC frames. At 100 Mbit/s, sleep 210 us, quiet 20 ms, wake 36 us and refresh 200 us
are **nominal** datasheet timings, not maximum bounds or proof the partner/MAC
is ready. Test literal entry/exit preserving bypass controls and add actual
timing/traffic measurements to qualification without fabricating their outcome.

Reopen MB85RC timed wake and uncertain transition source/tests, PCA9555
primary-versus-cleanup evidence and TMP1x2 operation/result and diagnostic
restoration code. Inspect source and registered tests, not just wrappers.
Reopen the exact DP83826I legacy-MAC EEE recipe, LPI control/status and wait
definitions alongside the mapping/procedure evidence settled in task 29.

Specify application/MAC prerequisites, explicit transmit coordination, finite
session bounds and what the PHY does not buffer. A PHY LPI control cannot
guarantee the MAC stopped sending or retain frames on its behalf. Require the
selected owner's maintenance/admission contract and validate request values and
available cached EEE/link/power evidence before transfers. Any required live
precondition uses bounded observation reads before mutation; do not demand
zero-I/O proof of current hardware state. External MAC/frame scheduling remains outside
portable core.

Implement ordered entry, source-defined waits, status/evidence, exit and restore
using one operation slot and aggregate deadline. Capture only necessary owned
state, preserve exact command-neutral values and never split indirect sequences.
The bus lock is released before scheduled waits. Distinguish request accepted,
verified LPI state, link/partner evidence and successful exit; a local callback
return cannot prove an environmental condition.

Any accepted TX-inhibit/LPI mutation remains visible until source-supported exit/
restoration proves otherwise. Cancellation is bus-silent; subsequent cleanup has
its separate finite allowance. Primary test/transport/deadline failure and cleanup
failure remain independently reported. If cleanup is denied, times out or cannot
restore, explicitly report whether TX may remain inhibited and leave relevant
configuration untrusted. No automatic reset/retry or apparent success after
an uncertain exit.

Test independent literal entry/exit traces and legal setting combinations,
missing MAC admission and invalid prerequisites with zero I/O. Inject every
mutation/acquisition/exit/restore failure, including error after TX inhibit took
effect, cancellation at each effect boundary and never-valid status. Assert
maximum calls, no lock across waits, no stale result reuse, primary/cleanup
precedence and retained inhibited-state evidence after teardown attempts.

Compile an owner sequence demonstrating application coordination and both normal
and failed-exit handling. Document bounded software work separately from physical
partner wake/link timing and energy measurements. Opt-in LPI stays disabled unless
explicitly requested; EEE advertisement alone must not silently initiate this
disruptive legacy-MAC procedure.

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
