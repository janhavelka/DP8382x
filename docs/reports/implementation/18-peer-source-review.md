# Prompt 18 - fresh sibling configuration review

Reviewed 2026-11-08 on `promptset/dp8382x-implementation-20261028`, entering
from synchronized prompt-17 checkpoint `57a06cb12140b8f2666e1b1a46c64ffe80f8035c`.
The current capable model was selected for concrete implementation/test review.
Lead owns shared declarations, source integration and Git. This assignment owns
this report, `docs/forced-settings.md` and `examples/portable/Forced.h` only.
Siblings and their histories remain read-only; no sibling builds/fetches ran.

Read parent and sibling agent rules, COMMON, prompt 18 and the existing value,
configuration, operation and restore contracts. Fresh local status checks found
both referenced sibling worktrees clean. Unchanged pins were verified directly,
not inferred from the prior-stage report.

| Repository/revision | Reopened actual source and evidence |
| --- | --- |
| TMP1x2 `db7687a5b2dfeee29df75a5b559ae6dd02baa8bb` | `src/TMP1x2Configuration.cpp`: `validateSettings`, `beginApply`, `stepApply`, `verify`, `update`; `src/TMP1x2.cpp`: `markConfigurationDirty`; `include/TMP1x2/TMP1x2.h`: pure validation, bounded operation and readback Doxygen; `test/jobs/test_jobs.cpp::configurationAdmission`; `test/test_core.cpp`: `dirtyRecovery`, `verificationPreservesMismatchEvidence`, `thresholdPartialWriteFailureMatrix`; `CMakeLists.txt` actual core/jobs test registration. |
| PCA9555 `414e0f66b13082e974ec27a4a6e86985a65d83fd` | `src/PCA9555.cpp`: `_invalidateShadowPair`, `_establishShadowPair`, `_recordPairObservation`, tracked/raw pair writes, `_compareObserved`; public `include/PCA9555/PCA9555.h`; `test/test_basic.cpp`: ambiguous output/apply writes at every phase, read-after-POR observation, matching verify after ambiguous commit, partial raw pair invalidation; actual `RUN_TEST` dispatch and native environment in `platformio.ini`. |

## Principles selected for the coupled PHY profile

TMP1x2 validates enum storage, threshold ranges and cross-field ordering before
encoding. Its explicit apply phases retain a complete target rather than updating
unrelated Booleans piecemeal. DP8382x adopts complete-profile validation: speed,
duplex, routing, robust policy, mirror and extension are checked together before
I/O or desired-state mutation. Sensor conversion/format wait phases are not PHY
timing evidence and are not copied.

TMP1x2 adopts target intent at FIRST_WRITE. DP8382x deliberately retains its own
admission-time desired-state rule: cancel before any transfer retains accepted
intent but does not create an effect or cleanup obligation from intent alone.
The original operation result remains independent of a later restoration result.

TMP1x2 `verify` records a low-threshold mismatch before reading the high threshold;
its test makes the later read fail and proves earlier dirty evidence survives.
DP8382x extends that pattern across all coupled PHY fields. A matching CR1 or
PHYCR read cannot erase an earlier CR2/CR3/BMCR mismatch or uncertain write.
Full signed backend detail remains separate from semantic comparison evidence.

PCA9555 invalidates the entire affected write shadow after MAY_HAVE_COMMITTED,
and every-phase tests assert terminal failure with no later apply phase or retry.
It distinguishes successful ordinary observation from establishing write intent.
DP8382x adopts those distinctions using complete MDIO frame receipts and explicit
configuration trust. I2C applied-byte counts, ACK semantics and output-before-
direction ordering do not describe Force Mode or an MDIO transaction.

PCA9555 permits an explicit matching verify to re-establish an ambiguous shadow.
That does not authorize erasing DP8382x's historical result or clearing OP-9
with one ordinary successful read. Its complete restoration remains a separately
admitted initialization/configuration operation with its own budget and result.

## Public contracts, CLI and tooling

Reopened actual TMP1x2 CLI status/operation rendering in
`examples/common/Tmp1x2Cli.cpp` and `Tmp1x2CliDiagnostics.cpp`, and PCA9555
`examples/01_basic_bringup_cli/main.cpp::printStatus`/`printSettings`.
Error names, signed details, retained terminal results and cached dirty/uncertain
state are displayed explicitly. A cached display is not a fresh hardware read.
The same distinction applies to DP8382x desired/observed/verified settings and
to forced local duplex versus unproved partner compatibility.

Inspected `TMP1x2/tools/hil_tmp1x2_runner.py` and
`PCA9555/tools/run_i2c_hil.py` for finite session/command timeouts and explicit
mutation policy, plus their existing workflow/test registration. No runner,
sibling workflow or hardware fixture was executed. Sensor delays, expander
restore order, SDK builds and I2C recovery cannot establish PHY electrical timing
or robust manual routing/mirror behavior. Those remain exact TI and Q11 evidence.

The assigned guide and owner helper reuse the shipped `LinkSettings`,
`startConfigure` and `ConfigureAttempt` contracts. No forced-only operation slot,
retry policy, callback or public reset alias is introduced. The initial board's
100/full AN preference stays application data, separate from a forced example.

## Independent implementation review and regression adaptation

Reviewed the actual changes to `Config.h`, `DP8382x.h`, `DP8382x.cpp` and
`DP8382xConfiguration.cpp`, not a completion summary. Confirmed full-request
validation before intent adoption, coupled mirror admission, exact routing-mask
selection, reserved/unrelated preservation, forced exclusion of ANAR and AN-only
options, stable comparison masks and shared reset replay. No alternate forced
engine, transaction owner or implicit VOD write was introduced.

Found a transition-order defect in the initial implementation: clearing robust
CR1 before activating AN temporarily left an outgoing forced profile without its
required robust control. Reported it to the lead and independent test author.
The lead now defers requested robust disable until after AN activation, without
depending on a previously trusted hardware image. The forced tests gained a
per-write transient-state assertion. Also reviewed the lead's fix that avoids
waiting on the AN-restart bit when forced mode makes that command irrelevant.
No remaining confirmed defect was found in the resulting code review.

Under a later explicit assignment, adapted only `test/negotiation.cpp` to the
expanded complete profile. The independent controller now retains CR1, PHYCR,
CR3 and CR2. Literal standard AN count is 24 frames and the two-page trace is
39, or 41 with duplicate evidence. Existing next-page, all60 advertisement/pause,
partial evidence, failure/cancel and timing assertions remain active. Obsolete
stage-17 rejection cases now test invalid enum, incomplete forced, contradictory
inactive intent or uncoupled mirror requests; legal stage-18 profiles are covered
by the new forced test file owned by another agent.

Strict GNU15.1 C++17 Release build in `build/forced18-peers` passed, followed by
all 12 registered negotiation CTests (11 suites plus executing negative probe).
The example header passed standalone strict C++17 syntax compilation. Reference
and scoped whitespace checks passed. The lead owns final full-suite, linked
consumer, Doxygen and sync evidence; sibling tests, SDK, remote CI and physical
qualification were NOT RUN by this reviewer.

The final observation-decoder review checked the new complete-readback behavior
against the active-policy snapshot contract. Decoding actual stable fields on a
complete mismatch correctly avoids presenting desired intent as observed state.
Reported two additional boundary cases to the lead: an unexpectedly enabled AN
mode after a forced-only acquisition must retain older complete tuple/time as
stale rather than erase its historical validity; and a non-IEEE ANAR selector
cannot establish fresh decoded IEEE technology/pause intent. Both corrections
were verified in the final source: the decoder retains the older tuple and its
validity/timestamp, marks it stale, and leaves current raw mismatch evidence
available. The independent forced-suite owner added regressions for both cases
and reported all seven suites passing with 44,513 checks. Review of those actual
assertions confirmed historical tuple/time retention, selector mismatch detail
and separate admitted desired intent. No remaining confirmed defect was found
within this review scope.
