# 30 - Implement forced TX LPI control

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

Deliver forced TX LPI control as two bounded calls, `startLpi()` and `stopLpi()`,
under OP-9. Each performs04D1 bit 12 RMW and masked readback; after the recipe,
008B->108B->008B. Application owns dwell, MAC drain and post-exit delay. Keep the
operation slot free during dwell while the temporary-state barrier protects it.

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

Validate explicit MAC/owner admission and cached EEE/link/power prerequisites.
The PHY sends LPI and does not buffer MAC frames. Restricting below100M is an
explicit policy inference:8.3.4 is silent and7.6 provides100M timings only.
Do not import DP83825 bits 14:13; DP83826 section 8.3.4 establishes bit 12 only. Q2 tests
108B on the line and retains the neighboring-part difference if it fails.

Use cached `forcedLpi {OFF, REQUESTED, UNCERTAIN}`. There is no documented PHY
LPI readiness event or dwell limit; 36 us wake is nominal. Stop compares bit 12
only. Admitted reads/link polling continue under OP-14 but link publishes its
bit only while the temporary session is active.

An accepted or uncertain LPI write remains visible until explicit stop/restoration.
Failure leaves UNCERTAIN and reports "forced LPI may remain active; frames sent
now can be lost". Cancellation ends the call; a later stop has a separate budget
and result. No automatic reset/retry or false completion follows an uncertain exit.

Test independent literal entry/exit traces and legal setting combinations,
missing MAC admission and invalid prerequisites with zero I/O. Inject every
mutation/acquisition/exit/restore failure, including error after LPI took
effect, cancellation at each effect boundary and never-valid status. Assert
maximum calls, no lock across waits, no stale result reuse, primary/cleanup
precedence and retained uncertain-LPI evidence after teardown attempts.

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
