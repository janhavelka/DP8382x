# Independent audit of prompts 21-22

Date: 2026-11-15. Clean, fetched and synchronized entry on `main`:
`04985e91f892160d1767405647a211667dec7438`. The final commit and upstream
equality are recorded in the handoff. Completion remains prompts 1-22; this
audit does not start loopback/BIST or any later feature.

## Scope and independent review

Re-read both original prompts in full, root/parent AGENTS and COMMON. Checked
the shipped headers, API OP-1 through OP-17, health/reconciliation guides,
source, examples, CMake registration and the actual implementation diffs:
`0a2d2c4..fe53923` and `fe53923..04985e9`. Reviewed README, architecture,
variants, source index and applicable engineering/evidence/verification rules.
One application owner continues to serialize calls and complete transactions;
no lock spans a wait. No remote workflow exists; CI remains prompt 71.

Three inherited capable agents reviewed disjoint scopes in parallel:
`audit21_health` checked accounting and transitions; `audit22_reconcile` checked
trust, reset effects, ownership and failure handling; `audit2122_tests` checked
requirements, executing tests, examples and original-source literals. This
selection kept independent reasoning on state/error boundaries without changing
model mid-review. Reviewers were read-only, used separate build directories and
did not edit siblings or Git. The lead verified findings against current code,
implemented the correction and regression, and owns documentation and Git.

## Findings and dispositions

| Finding | Independent evidence | Correction |
| --- | --- | --- |
| Active-reset evidence lost on later initialization read error | From an initialized/configured object, start NONE, read identity/straps, receive valid BMCR `8000`, then fail release. The old code returned MDIO_ERROR before withdrawing initialization/VERIFIED trust. The same issue affected PHYRCR checks/waits, including HARD observed during a RESTART wait. | Retain valid direct reset-read evidence before the terminal error guard. PCS/HARD invalidate their state; RESTART retains settings/readiness and invalidates link/history. Preserve the primary error and earlier effects. Removed duplicate later invalidation calls. |
| Initialization guide described mandatory VOD as the only owned state and unqualified its frame totals | Actual stages 17/18 replay the desired link profile through the shared configuration path. | Qualified the 20/32-frame totals as no-profile cases and documented additional profile replay. |

The new `verify_reset_observation_errors` suite failed on the original code with
64 assertion failures. It now passes all 3,928 checks across 20 combinations:
BMCR check, PHYRCR check, PHYRCR wait, HARD during RESTART completion, and
register-retaining RESTART, each followed by release error, exact deadline,
clock regression or uint64 overflow. It asserts one attempted/completed read,
no extra writes, retained intent, error/status details, reset-specific validity,
unchanged mutation/barrier ownership and independent managed-health accounting.
The reviewer also reproduced the baseline defect using an ignored temporary
harness, then verified the corrected source and full suite.

No prompt-21 defect was found. The following suspected issues were checked and
rejected against the frozen contracts: per-batch rather than public-call health
counts; transport success followed by a local clock/deadline error; explicit
managed restoration clearing the failure streak while preserving lastError;
raw traffic never inheriting an operation's classification; and statistics
reset preserving active failure/trust/effect evidence. Widths, saturation,
zero/missing timestamps, OFFLINE admission and cached no-I/O behavior are covered.

For prompt 22, stable masks, exact bounds, staged output, first-mismatch retention,
changed identity, selector re-addressing, cancellation and cleanup ownership were
confirmed. Successful VERIFY never clears cleanupOwed. Recovery still uses one
initialization/configuration implementation. No extra abstraction or state
machine was needed. The existing private health arithmetic helper remains useful
for testing real-width saturation without a public state-seeding API.

## Fresh source evidence

All sibling worktrees below were clean and read-only at review:

| Repository and revision | Reopened paths / decision |
| --- | --- |
| TMP1x2 `afd50889312f6e218a6a34eca687e3e8547d8b40` | `src/TMP1x2.cpp` track/health/invalidation, configuration verification, public operation values, `test/helpers/test_helpers.cpp`, `test/cli/test_cli.cpp`, `tools/hil_tmp1x2_runner.py`. Adopt retained errors and cached views; adapt batch widths and timestamps. |
| PCA9555 `8b461b2afe9910b5b15fe32cc049d52e3b498970` | `src/PCA9555.cpp` health/cleanup/shadow paths, `test/test_basic.cpp` cleanup-retention test and registration, `examples/common/HealthDiag.h`, `tools/run_i2c_hil.py`. Adopt separate primary/restoration evidence; matching bits cannot clear DP opaque cleanup obligations. |
| INA228 `4de84f2564eb6382835ca9a72d8688377657dcb4` | `src/INA228.cpp` health/init/offline branches and `test/test_basic.cpp` policy cases. Reject offline admission latch and initial-bring-up exclusion. |
| 24Cxx `1bd9f969d1df55ca5819b0d290ff331869e7d5bc` | `src/EEPROM24Cxx.cpp` track and busy-poll accounting. Reject EEPROM busy-NACK semantics for MDIO. |
| FieldCore-node `c40ea8341edf8d665460b902cafefbe857a7f73f` | `src/devices/i2c/Ina228Module.cpp` explicit refresh versus passive copyStatus. Keep application presence/readiness separate. |

Reopened the archived SNLS647I original-PDF extraction and the current
[TI datasheet](https://www.ti.com/lit/ds/symlink/dp83826i.pdf), still revision I,
August 2025. Tables 8-19/48 define the distinct reset domains; tables 8-43/81
support the independent routing and mandatory-setting masks. Review derived
VOD mask `F000|0800|0780|0040=FFC0` and value `4000|0800|0200|0040=4A40`
without using production constants. No silicon value or vendor payload changed.
Q5 reset-strap and Q11 forced-routing physical gates remain open.

## Verification and documentation

Windows, GNU C++ 15.1, Ninja, C++17 Release, strict warnings and active CHECKs
under NDEBUG. No second compiler was found on PATH.

| Layer / command | Result |
| --- | --- |
| `cmake -S . -B build/audit2122 -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON`, build, CTest | PASS: 109/109, including 13 deliberate negative assertion probes |
| Same configuration in `build/audit2122-ubsan` with `-fsanitize=undefined -fsanitize-undefined-trap-on-error` | PASS: 109/109 |
| Independent reviewer `build/audit22-peer` | PASS: 109/109 after observing the new regression fail before the fix |
| `cmake -S test/consumer -B build/audit2122-consumer -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=OFF`, build, CTest | PASS: 1/1 portable consumer |
| `python -m unittest discover -s test -p "test_*.py" -v` | PASS: 103 tests |
| `python tools/check_doxygen.py` | PASS: 159 pages and two rejected fixtures |
| `python tools/verify_ledger.py` | PASS: 97 registers, 52 procedures, 35 features |
| `python tools/verify_references.py`, `python scripts/generate_version.py check`, `git diff --check` | PASS |
| SDK link / installed package consumer | NOT RUN: no SDK/package implementation changed |
| Remote CI | NOT RUN: reserved for prompt 71 |
| Physical qualification | NOT RUN: no admitted fixture or measurement |

Updated public Doxygen, OP-13, initialization/reconciliation and test guides,
CHANGELOG, report index and progress. README/architecture/variants/feature
coverage and tooling already describe the corrected scope; no feature, version,
package or source-ledger change is needed. No confirmed in-scope issue remains.

## Current-code response — 2026-11-17

Rechecked every finding and rejected proposal above on `main`, entry
`77c544f415edb26a0e23de76591a0163b9b73147`, against current prompts 21/22,
ENGINEERING and the shipped implementation. The earlier stage limit is historical.

| Finding or proposal | Current verdict and evidence |
| --- | --- |
| Active reset evidence lost when the read later failed | **Confirmed implemented.** `src/DP8382x.cpp::_retainEvidence` applies valid direct/vendor-alias BMCR/PHYRCR reset evidence before terminal error handling. `test/verify.cpp::resetObservationErrors` still covers all five reset situations and four later failures; `rawResetObservation` also checks both namespaces and frozen results. Primary transport errors remain separate from the observed reset and its domain. |
| Initialization guide omitted profile replay and qualified frame bounds | **Original correction implemented; later stale wording corrected here.** `docs/initialization.md` now limits the 20/32-frame counts to no owned link or optional-feature settings and describes the actual shared restoration tails. Its former blanket “No RCSR mutation” sentence was removed: `src/DP8382xInterface.cpp::_interfaceRowValue` restores owned stable RCSR settings through the collector. Unowned interfaces add no RCSR write; calibration remains a separate route. `test/interface.cpp::replayReadiness` and `replay` prove the current behavior. |
| Per-batch health accounting proposed as a defect | **Incorrect under the current contract.** `_execute` accounts at the actual callback boundary, as prompt 21 requires; `test/health.cpp::accounting` distinguishes logical transactions from frames/public calls. |
| Successful transport followed by clock/deadline error proposed as failed MDIO | **Incorrect.** `_execute` classifies transport health before the later local time verdict. `test/health.cpp::timing` and `reports` preserve this distinction. |
| Managed restoration resetting the failure streak | **Incorrect as a defect.** Prompt 21 expressly permits separately admitted managed restoration success to reset the streak while retaining lastError. `test/health.cpp::restoration` exercises this, including separate results. |
| Raw calls inheriting operation classification or statistics reset erasing active evidence | **Confirmed protected.** `_execute` uses explicit managed-call classification; `resetStatistics` clears cumulative counts only. `test/health.cpp::rawAndLocal`, `cancellation`, `saturation` and `isolation` cover raw traffic, widths and state retention. OFFLINE remains passive. |
| VERIFY clearing cleanup or duplicating recovery | **Confirmed protected.** `startVerify` and `_finishOperation` retain OP-9 cleanup debt; the ordinary initialization/configuration path performs restoration. `test/verify.cpp::retainedBarrier`, `failures`, `recovery` and `interruptedReplay` cover stable masks, partial evidence, first mismatch, new identity and bounded retry admission. |

Reopened original SNLS647I tables 8-19/48 and 8-81 directly from the archived
PDF. PCS resets PCS registers; HARD has hardware-reset effects; RESTART preserves
registers. These definitions support retaining an observed reset before a later
callback failure. They do not prove a hardware timing or strap outcome.

Fresh host verification: Windows/GNU C++ 15.1/Ninja, C++17 Release build
`build/audit-current-response-silicon`, **PASS 236/236 CTests**, including all
health/verify suites. Package consumer, SDK, CI and physical checks were **NOT RUN**
by this response review. Q5/Q11 remain open. No new health/reconciliation
production defect was confirmed; the shared initialization guide now includes
the current-tail qualification described above.
