# Prompt 15: fresh sibling operation review

Reviewed 2026-11-08 on `promptset/dp8382x-implementation-20261028`, entering
from `f7165130b8f584aaf43fee997e4887aa4c3bc064`. The lead owns shared contracts,
implementation, integration and Git. This disjoint review uses the current
capable model for actual source, tests and owner-facing examples. Siblings
were clean at inspection and remained read-only. The review owns only this
report. DP8382x OP-1 through OP-17 and prompt 15 control the adaptation;
sibling device timing and lifecycle details are not PHY specifications.

| Repository | Pinned revision | Actual reviewed paths and anchors |
| --- | --- | --- |
| TMP1x2 | `db7687a5b2dfeee29df75a5b559ae6dd02baa8bb` | `include/TMP1x2/TMP1x2.h` OperationResult/OperationSnapshot (50/60); all of `src/TMP1x2Operations.cpp`, especially admit (7), finishJob (97), poll (190), cancel (239), takeResult (245), getOperationSnapshot (253); `test/jobs/test_jobs.cpp` cancellationAndTeardown (335), readFailureAndLatePublication, frozen-clock anchoring (574 onward), test table (656 onward); `examples/common/Tmp1x2CliDiagnostics.cpp` pollOperation/finishOperation (70/80). |
| PCA9555 | `414e0f66b13082e974ec27a4a6e86985a65d83fd` | `src/PCA9555.cpp` detach (104), cancelOperation/_requestTerminal (650-668), per-transfer operation phases and remaining callback allowance (677 onward, 877-919); `test/test_basic.cpp` shared deadline (2495), near-deadline callback minimum (2515), passive detach (2543), pending-result rebind (2564), required-cleanup detach refusal (2580); registrations (2742-2746); `examples/01_basic_bringup_cli/main.cpp` cached settings rendering (486-509). |
| 24Cxx | `e4e1eeb41575df462e585c54989950323c43ab7a` | `src/EEPROM24Cxx.cpp` retained write barrier/tick/afterCallback (145-171), finish/cancel/takeResult (460-502), poll (510 onward); `test/test_core/test_core.cpp` writeEvidenceAndNoReplay (207), cancellationDeadlineAndRollover (244), verificationEvidenceAndLifecycle (308), updateCancellationAndSkipEvidence (638), registration (656 onward); `examples/common/Eeprom24CxxCli.cpp` separately retained primary/restoration rendering (494-525); `test/test_cli/test_cli.cpp` retained backup and cancellation/restore coverage (374-446). |

Read each sibling's AGENTS.md, inspected the relevant workflow definitions and
actual test registration. TMP1x2's `CMakeLists.txt` registers the `jobs` suite
(36-38); its `test/test_custom_runner.py` parses the same executable's explicit
PASS/FAIL records. PCA9555 uses Unity under `platformio.ini` and the RUN_TEST
list in `test/test_basic.cpp`; its root CMake file is an ESP-IDF component, not
a host-suite registration. 24Cxx registers the `core`, `cli` and runner suites
in `CMakeLists.txt`. No sibling tests or remote jobs were executed for this
read-only audit.

The actual runner paths are TMP1x2 `tools/hil_tmp1x2_runner.py`, PCA9555
`tools/run_i2c_hil.py` and 24Cxx `tools/hil_runner.py`. Their bounded sessions,
identity/provenance checks and explicit mutation admission are useful owner
patterns. A passing transcript remains separate from physical qualification.
Their workflow triggers and SDK jobs do not override DP8382x's prompt-71 CI
exception. No fixture, SDK build or physical session was run here.

## Adopted and adapted principles

- Adopt explicit operation state and a retained terminal result. TMP1x2 keeps
  `hardwareEffectPossible` even after cancellation or a failed configuration,
  gates sample publication on timely success, and requires the matching token
  for one-shot consumption. Its tests cancel after each transfer boundary and
  recover separately. DP8382x retains semantic mutation, protocol completion,
  persistent restoration and cleanup obligation separately.
- Adopt remaining aggregate allowance and exact-expiry rejection. PCA9555's
  9 ms test sums callback grants, and its 3 ms test admits only three 1 ms
  callbacks before a bus-silent timeout. For DP8382x one poll admits one whole
  batch, including all four indirect frames, and grants cannot split an MMD
  transaction. OP-6 adds the literal reset look-ahead with four clock lags.
- Adapt cleanup refusal without copying the cleanup operation. PCA9555 defers
  its requested terminal cause while a pointer park remains owed and refuses
  detach. DP8382x cancellation is immediately terminal and bus/clock silent;
  `cleanupOwed` prevents teardown and mutation until a separately admitted
  initialization restores state, or the owner explicitly abandons cleanup.
  The later operation has its own token, budget and result.
- Adopt 24Cxx's separation of accepted effects, completion and verification.
  Its accepted-write/error tests retain the write-cycle barrier after result
  consumption; cancellation or end cannot make an accepted write disappear.
  Its CLI stores primary and restoration evidence independently. DP8382x uses
  this principle for accepted/uncertain PCS and VOD effects, without treating
  selector writes as target mutation or a later read as proof of an earlier
  ambiguous write's outcome.
- Adapt a completion-time anchor, not a timestamp sampled before mutation.
  TMP1x2 explicitly defers a conservative anchor when the clock was unavailable;
  24Cxx samples after callback. DP8382x uses its monotonic uint64 microsecond
  contract, bounded quantization and checked arithmetic, including frozen-clock
  transfer bounds and stalled-poll exhaustion. Return to the application for
  waits; release controller ownership before returning.
- Adopt owner-rendered retained output. TMP1x2 `finishOperation` consumes once,
  stores the result, then prints; PCA9555 settings rendering uses a cached
  snapshot. 24Cxx prints the primary and restoration statuses separately. No
  cached DP8382x operation/identity/result query may issue bus or clock calls.

## Deliberate non-transfers

TMP1x2 uses uint32 tokens and an exhaustion branch, and its admission and
snapshot helpers can invoke its clock. DP8382x instead uses a nonzero uint64
lifetime token without an exhaustion branch/test, and starts and cached access
are clock silent. TMP1x2's pending-result cancel returns BUSY; DP8382x follows
its own OP contract. Peer multi-transfer poll loops, wrapping millisecond
arithmetic and health transitions are not copied: DP8382x uses one whole next
batch, absolute deadlines and managed health only from prompt 21.

PCA9555 pointer parking, I2C command-byte completion and post-deadline cleanup
do not specify MDIO reset/restoration. EEPROM STOP-triggered programming, ACK
polling, known-busy NACK treatment and maximum tWR do not establish PHY reset
completion or readiness. Sensor conversion delays and format refresh likewise
do not establish PHY waits. Only the exact SNLS647I PCS/VOD procedures and
DP8382x's explicit operation contracts determine those values and traces.

This review supplies source/convention evidence only. Production tests,
portable consumer, Doxygen/reference checks and the final implementation
review belong to the lead's stage report; this report makes no SDK/controller
timing, silicon, link-up or electrical qualification claim.
