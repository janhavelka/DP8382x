# Prompt 17: fresh sibling configuration review

Reviewed 2026-11-08 on `promptset/dp8382x-implementation-20261028`, entering
from `1e0cec9067c498ac5bc7f2c03607395af73e8c2a`. The lead owns shared headers,
implementation, guides, integration and Git. This disjoint review uses the
current capable model to inspect actual source and failure tests; its only
writable path is this report. Siblings remain read-only. Prompt 17, COMMON and
the DP8382x API contracts govern adaptations, including admission-time desired
intent, one MDIO owner and OP-9's separate restoration operation.

Read the repository entry guides, parent and sibling agent rules, selected prompt
and relevant API contracts. Both sibling worktrees were clean at inspection.
These are current local revisions; no sibling remote fetch or build was run.

| Repository | Revision | Reopened paths and responsibilities |
| --- | --- | --- |
| TMP1x2 | `db7687a5b2dfeee29df75a5b559ae6dd02baa8bb` | `src/TMP1x2Configuration.cpp`: `validateSettings`, `beginApply`, `stepApply`, `verify`, `update`; `src/TMP1x2.cpp`: `markDirty`, `markConfigurationDirty`, raw access; `include/TMP1x2/TMP1x2.h`: configuration, dirty-state and operation Doxygen; `test/jobs/test_jobs.cpp`: configurationAdmission, initializationFailureMatrix, cancellationAndTeardown; `test/test_core.cpp`: dirtyRecovery, thresholdPartialWriteFailureMatrix, verificationPreservesMismatchEvidence. |
| PCA9555 | `414e0f66b13082e974ec27a4a6e86985a65d83fd` | `src/PCA9555.cpp`: `_invalidateShadowPair`, `_establishShadowPair`, `_recordPairObservation`, `_compareObserved`, tracked writes and operation completion; `include/PCA9555/PCA9555.h`: observed-state, apply/verify and shadow contracts; `test/test_basic.cpp`: ambiguous writes at every apply phase, exact readback mismatch, read-after-POR observation, raw-write rejection and partial-byte shadow invalidation. |

## Adopted and adapted behavior

- TMP1x2 validates the complete settings value before encoding/admission and
  keeps its operation state explicit. Its tests reject changed binding hooks,
  addresses and invalid thresholds with unchanged token and zero callbacks.
  DP8382x similarly rejects the entire invalid link profile before changing
  desired state or issuing MDIO. An accepted DP8382x start, however, immediately
  adopts desired intent and marks trust dirty under its own API contract.
- TMP1x2 stages its target until `FIRST_WRITE`, then commits it before the
  callback can ambiguously affect silicon. Its `configurationAdmission` test
  explicitly expects the old profile after cancellation before that write.
  That timing is deliberately **not** copied: DP8382x preserves admitted desired
  intent even when cancelled before mutation, without inventing cleanup owed
  from intent alone.
- `verify()` records a low-threshold mismatch before reading the high threshold.
  `verificationPreservesMismatchEvidence` makes that later read time out and
  asserts both the earlier dirty mismatch/detail and the later transport error.
  DP8382x likewise retains observed ANAR/BMCR mismatch evidence independently of
  a subsequent readback transport failure and its signed backend detail.
- PCA9555 invalidates an affected write shadow after ambiguous completion and
  stops without replaying or advancing to a later phase. Its tests inject each
  possible partial byte count and every apply write phase, asserting exact
  transaction counts and retained uncertainty. DP8382x uses frame/effect
  evidence and complete owned batches; it never interprets I2C byte counts as
  partial MDIO data or retries an accepted/uncertain PHY mutation.
- PCA9555 ordinary reads update observation without adopting hardware as write
  intent. Mismatch withdraws cached RMW permission. Its partial raw output-byte
  test invalidates the whole pair and proves subsequent cached RMW is bus-silent
  until deliberate re-establishment. DP8382x keeps desired, observed and verified
  owned fields distinct and invalidates trust on raw/external mutation.
- PCA9555 explicit verification against a complete caller image may establish a
  matching shadow after an earlier ambiguous commit. This does not authorize
  rewriting the earlier DP8382x result or clearing an opaque OP-9 obligation.
  DP8382x restoration is a separately admitted operation with its own finite
  budget/result; initialization replays the same validated persistent intent.

## Public contracts, actual rendering and test registration

TMP1x2 header Doxygen explicitly documents staged configuration, dirty evidence,
bus-silent starts/cancel/take, bounded poll transfers and terminal-result retention.
PCA9555 header Doxygen separates live `readObservedState` from passive
`lastObservedState`, specifies partial validity and makes apply/verify effects
explicit. DP8382x public contracts should likewise state which values are
passive, when desired settings become authoritative, what was verified, and why
successful register application does not establish negotiation or link-up.

Inspected actual TMP1x2 `examples/common/Tmp1x2Cli.cpp::status` and
`Tmp1x2CliDiagnostics.cpp::startOperation`, `pollOperation`, `finishOperation` and
`observedStatus`. Rendering includes the error name and signed detail; a transport
success does not hide dirty configuration. Terminal results are consumed once
into retained CLI state before printing. PCA9555
`examples/01_basic_bringup_cli/main.cpp::printStatus` renders code, signed detail
and message; `printSettings` uses cached `getSettings()` and displays shadow and
uncertain masks. No display request is used as evidence of a fresh read.

TMP1x2 `CMakeLists.txt` registers its core and jobs executables, and the tests
above occur in their actual test dispatch. PCA9555 registers the reviewed tests
with `RUN_TEST` in `test/test_basic.cpp`; `platformio.ini` enables the native Unity
environment with library source included. Reopened the two `.github/workflows/ci.yml`
files as registration/tooling evidence only. DP8382x remote CI remains prompt 71.

Inspected `TMP1x2/tools/hil_tmp1x2_runner.py` and
`PCA9555/tools/run_i2c_hil.py`: bounded serial sessions, explicit mutating suites,
parsed status/identity evidence and command-specific finite timeouts are useful
runner conventions. These scripts were not executed and do not prove PHY link
negotiation, physical next-page handshake or restoration on an actual board.

## Deliberate differences and evidence limits

Sensor conversion/format settling, uint32 millisecond wraparound, expander
output-before-direction ordering, I2C pointer parking and byte ACK behavior do
not specify a PHY procedure. DP8382x uses its existing uint64 clock/budget,
complete MDIO batches and TI-backed ANAR/BMCR order. Controller ownership ends
before waits and rendering. No sibling retry, recover alias, health transition,
pointer cleanup phase or generic configuration engine is imported.

NP=1 is a temporary typed exchange epoch, never persistent desired advertisement.
The local sibling patterns provide no IEEE next-page handshake evidence. Exact
TI/IEEE review and independent literal tests belong to the lead and silicon
reviewer; physical Q10 remains NOT RUN. Configuration restoration replays NP=0,
and cancellation/failure reports possible remaining NP with cleanup owed rather
than performing hidden terminal writes.

This report records source review only. Implementation tests, portable-consumer,
Doxygen/reference checks and final diff review are recorded in the stage report.
Sibling tests, SDK/controller integration, remote CI and physical qualification
were NOT RUN by this reviewer.

## Independent implementation review

After the implementation landed, reviewed actual `src/DP8382xConfiguration.cpp`,
the changes to `src/DP8382x.cpp` and `include/DP8382x/DP8382x.h`, and
`test/negotiation.cpp`. Checked full-request rejection before desired mutation,
admission-time desired authority, separate raw readback validity, preservation of
first mismatch and later backend evidence, complete RMW exclusion, partial page
retention, bounded duplicate handling, NP-free reset replay and OP-9 restoration.
No confirmed implementation defect was found in that review.

Checked the apparently unusual first received page toggle comparison against the
independent silicon review: IEEE Clause 28 deliberately seeds it from bit 11 of
the base page, even though that bit carries asymmetric pause in a base page.
It is not a next-page-only bit mask accidentally applied to the wrong register.
The protocol reviewer confirmed the rule directly in the original IEEE text and
state diagram. This source check avoids manufacturing a fake-specific fix.

Requested dedicated late-completion and BMCR-mismatch regressions after noticing
that the feature test's corresponding injection controls were initially unused.
Shared transport/operation tests cover deadlines generally; feature tests must
also establish negotiated-state and cleanup evidence on those exits.

Authored `docs/negotiation.md` and `examples/portable/Configure.h` under the lead's
separate assignment. The helper header passed standalone C++17 syntax compilation
through an including translation unit with `-Wall -Wextra -Werror`; this is not
linked-consumer evidence. Reference and whitespace checks passed. Linked example
execution and final registered test results belong to the stage report.
