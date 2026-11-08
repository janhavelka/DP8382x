# Prompt 16: fresh sibling lifecycle review

Reviewed 2026-11-08 on `promptset/dp8382x-implementation-20261028`, entering
from `5f05f97a24b929ad5ded85eb08112a4514208bdf`. The lead owns shared headers,
implementation, architecture, progress and Git. This disjoint review uses the
current capable model for actual sibling lifecycle source, tests, examples and
runners. Its sole writable path is this report; all siblings remain read-only.
DP8382x prompt 16 and OP-1 through OP-17 govern the adaptation.

All three sibling worktrees were clean at inspection. The following are current
local revisions, not a claim that their remote branches were fetched or tested:

| Repository | Revision | Reopened source and tests |
| --- | --- | --- |
| TMP1x2 | `db7687a5b2dfeee29df75a5b559ae6dd02baa8bb` | `include/TMP1x2/TMP1x2.h` operation/result declarations; `src/TMP1x2Operations.cpp` admit, startRecover, finishJob, stepJob, poll, cancel and takeResult; `src/TMP1x2Configuration.cpp` beginApply, startApplyWait, stepApply and recover; `src/BusOperations.cpp::generalCallReset`; `test/jobs/test_jobs.cpp` cancellationAndTeardown, wait/deadline/clock cases and registration; `test/bus/test_bus.cpp` accepted reset failure and literal general-call checks. |
| PCA9555 | `414e0f66b13082e974ec27a4a6e86985a65d83fd` | `include/PCA9555/PCA9555.h` OperationResult and detach/end contracts; `src/PCA9555.cpp` detach (104), request-terminal/cancel (650 onward), operation transfers (677 onward); `test/test_basic.cpp` failed cooperative input primary/cleanup evidence (2120), successful cleanup preserving failed primary (2194), shared remaining deadline (2495), passive detach (2543), pending-result rebind (2564), required-cleanup detach refusal (2580), RUN_TEST registrations (2729 onward). |
| TCA9548A | `1997d3f9446057348adbc6b2b6872dd18c041de3` | `include/TCA9548A/TCA9548A.h` end/recover/hardReset and invalidateChannelMask contracts (236 onward, 353); `include/TCA9548A/Config.h` reset callback ownership; `src/TCA9548A.cpp` end (82), recover (95), hardReset (99), resetBindingState (260); `test/test_driver/test_driver.cpp` late-write/new-cleanup-budget (465), bus-silent end/rebind (759), lifetime counters (789), hard-reset callback/failure/readback cases and registration (1562 onward). |

Read the current AGENTS.md in each sibling and inspected their workflow/test
registration. TMP1x2 CMake registers core, jobs, helpers, bus and CLI executables;
its custom runner consumes actual executable results. PCA9555 registers Unity
cases in `test/test_basic.cpp` and executes the native PlatformIO environment.
TCA9548A `platformio.ini` selects `test_driver`, with explicit Unity registrations
for end, reset callback failure, exact-zero readback, mismatch and no retry.
Their existing `.github/workflows/ci.yml` checks are source references only;
DP8382x first-execution remote CI remains prompt 71. No sibling build or remote
workflow was executed in this review.

## Selected principles

- TMP1x2's initialize/recover routes share the same apply phases. Validated
  desired settings are retained before an ambiguous write can reach silicon,
  and observed state does not silently replace desired intent. DP8382x extends
  its existing initialization path for HARD and RESTART rather than adding
  reset/recover aliases or another state machine. Only implemented owned state
  belongs in restoration; currently mandatory VOD is that persistent state.
- TMP1x2 marks effects before mutation and retains them through cancellation.
  Its apply wait uses completion-aware timing, including a deferred conservative
  anchor when the current poll cannot observe callback completion. DP8382x uses
  its own nonwrapping uint64 microsecond clock, bounded lag and post-callback
  upper bound. Its HARD quiet floor persists independently of the result slot.
- PCA9555 retains a primary read error even when pointer cleanup also fails or
  later succeeds. Its tests distinguish requested cancellation, cleanup owed,
  cleanup result and detach admission. DP8382x preserves that evidence principle
  but freezes cancellation immediately: restoration is a separately admitted
  initialization with its own token, budget and result. A later success cannot
  rewrite the original failure.
- TCA9548A `end()` only resets local binding state; its public comment requires
  an explicit checked `disableAll()` when safe-off is needed. Its reset callback
  invalidates cached truth before a possible effect, preserves callback errors,
  then performs one explicit verification read. DP8382x similarly separates
  local teardown from physical safety, accepted command from completion, and
  completion from restored settings. Application GPIO and supply control stay
  outside the driver.
- TCA9548A preserves historical observed bytes with unknown provenance after
  ambiguous operations and exposes explicit local invalidation. DP8382x keeps
  raw identity/strap/history evidence with validity/staleness; external reset
  and POR notifications withdraw current permissions without issuing MDIO or
  silently performing another hardware reset.

## Owner-facing examples and deliberate differences

Reopened TMP1x2 `examples/common/Tmp1x2CliDiagnostics.cpp::pollOperation` and
`finishOperation` (70/81): the CLI consumes once, stores a result, then renders.
PCA9555 `examples/01_basic_bringup_cli/main.cpp` renders cached settings and
uncertain/shadow pairs (486–509). TCA9548A
`examples/common/Tca9548aCli.cpp` distinguishes cached observation, explicit
invalidate, recover safe-off write and optional reset; its HIL reset path is
opt-in. DP8382x examples likewise expose historical results and explicit owner
decisions, without using a display request as a destructive read.

Inspected actual runner paths `tools/hil_tmp1x2_runner.py`, `tools/run_i2c_hil.py`
and `tools/tca9548a_hil.py`. They use finite sessions/deadlines, explicit fixture
admission and separately interpreted transcripts; the TCA runner requires
per-session transfer and cleanup evidence. Such host records do not establish
PHY reset timing or strap resampling. No fixture session was run.

The following sibling details do not transfer:

- TMP1x2's uint32 wrapping millisecond clock, exhaustion branch, clockful
  admission/snapshots, sensor format/conversion waits and general-call broadcast
  reset do not specify MDIO protocol, timing or DP83826I identity.
- PCA9555's pointer-park cleanup may continue after the primary deadline and
  delay terminal cancellation. DP8382x follows immediate retained termination
  and a later restoration operation; its controller lock never spans a wait.
- TCA9548A `recover()` is merely a safe-off alias and `hardReset()` invokes an
  application GPIO callback then checks a zero control byte without restoring
  the previous mask. DP8382x has neither alias nor reset GPIO callback; HARD and
  RESTART are distinct literal PHYRCR commands inside `startInitialize`.
- I2C ACK, command-byte/STOP timing, reset-pin nanosecond timings and sibling
  health transitions provide no PHY completion or bus-quiet proof. The shared
  MDIO owner must enforce the PHY's persistent quiet floor across all addresses;
  the exact TI source and independent vectors determine the reset effects.

This report records source/convention review only. Host production checks,
portable consumer, Doxygen/reference verification and final implementation
review are recorded by the lead. SDK and physical evidence remain NOT RUN.
