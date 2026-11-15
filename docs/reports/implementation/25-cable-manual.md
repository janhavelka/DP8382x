# Prompt 25 - Complete manual cable capture

Date: 2026-11-15. Execution branch: `main`. Entry was clean, fetched and equal
to upstream at `87b08d2ccb407d4fe33b1dc03920600855570c7a`. The final commit and
verified upstream equality are recorded in the handoff without a self-reference.

## Rules, scope and ownership

Reopened parent/root AGENTS, COMMON, CONVENTIONS, EVIDENCE, FULL_COVERAGE,
architecture, variants, reference index and OP-1 through OP-17. Prerequisites
are the shared reconciliation/reset owner, single collector and diagnostic ledger.
Owned procedures are `cable_configure`, `cable_manual`, `cable_capture`; autorun
remains prompt 26 and calibration recovery prompt 35. One application owner
serializes all calls and full controller transactions across PHY addresses.
Starts/cancellation/cached access are bus-silent; no lock spans waits.

Lead owns shared header/lifecycle integration, the cohesive cable source,
CMake, ledger/guides/progress and Git. Fixed storage contains six result rows;
there is no allocation, SDK dependency, worker, second collector or job engine.
Manual operations are RAW traffic, separate from managed health. The pure
interpretation helper has no hardware access. Version remains 0.1.0: this is
an implementation stage, not a release-version change.

## Fresh source and sibling audit

Reviewed original archived SNLS647I (August 2025), section 8.3.16.1 and tables
8-47/54/56 through 71; SNLA428 (June 2023), tables 2-1/4/5 and section 2.2.
Current official [datasheet](https://www.ti.com/lit/ds/symlink/dp83826i.pdf) and
[exact-part application note](https://www.ti.com/lit/pdf/snla428) retain these
revisions. Independent source review found no stronger exact-part clarification
of missed freshness transitions or engine-abort acknowledgement. Vendor bytes,
archive hashes and provenance remain unchanged; no copyrighted sample code was
copied. The note's MATLAB indexing/undefined-variable inconsistencies do not
become production behavior. DP83822/DP83825 recipes are not imported.

| Read-only sibling / pinned revision | Reopened material and selected principle |
| --- | --- |
| TMP1x2 `afd50889312f6e218a6a34eca687e3e8547d8b40` | AGENTS; `include/TMP1x2/TMP1x2.h` results/snapshots; `src/TMP1x2Operations.cpp` poll/finishJob/takeResult; `examples/common/Tmp1x2CliDiagnostics.cpp` retained result/restore; `tools/hil_tmp1x2_runner.py` fixture verdicts; `tools/test_hil_tmp1x2_runner.py` partial/cleanup cases and CMake registration. Adopt correlated partial evidence and passive display; reject sensor timing and automatic cleanup for OP-9. |
| PCA9555 `8b461b2afe9910b5b15fe32cc049d52e3b498970` | AGENTS, public lifecycle/cleanup declarations and implementation, `test/test_basic.cpp` primary/cleanup failures, remaining-deadline and detach cases, executing registrations. Adopt separate primary/restoration results and refusal barriers; reject I2C pointer/ACK rules as PHY evidence. |
| 24Cxx `1bd9f969d1df55ca5819b0d290ff331869e7d5bc` | AGENTS; `tools/test_hil_identity.py` parser and CMake registration. Host configuration and fixture labels do not replace observed identity. EEPROM ACK polling is not a TDR completion rule. |

Selected SNLA428 runs are exactly 1A, 1B, 2, 3, 4, 5 or an explicit subset.
GENCFG owns only the exceptional mask; all other bits survive its RMW. Opaque
companions are not readable/restorable raw registers: exit uses shared HARD and
current implemented persistent replay. RAW covers all documented channels,
averaging, DC, segment, pulse/threshold/shadow controls with independent
SNLS647I vectors. RAW does not own exceptional GENCFG bits.

## Behavior and independent review

Live preflight rejects autorun, calibration, active diagnostic/reset or link-up
before mutation. Saved documented fields precede every mutation. Completion
requires accepted trigger, observed start/done progress, then start-clear/done.
Each nondestructive result word survives a later failure; the aggregate remains
stale until complete. Explicit reread is finite, never retriggers and does not
resume unperformed later rows. Stop is a new operation/result and performs
manual-control stop, shared HARD/profile replay and verified saved-field tail.
Failure/cancellation never append cleanup frames or overwrite a prior result.

Three inherited capable-model agents were selected for independent silicon
reasoning, lifecycle/peer review and executable literal tests. `cable25_sources`
reviewed original PDFs/current official sources and wrote only `docs/cable.md`.
`cable25_peers` reviewed lifecycle/ownership and wrote only the portable example
and separate consumer. `cable25_tests` owned only `test/cable.cpp`, using
`build/cable25-tests`. Lead checked their findings against actual source and diff.
No subagent edited siblings or Git.

Confirmed corrections made before handoff:

- Preserve OP-2 admission precedence before zero-budget outcomes.
- Reject prohibited saved averaging/segment encodings before mutation; replay
  must not turn an invalid prior image into an authorized write.
- Retain observed calibration as an initialization gate; only owning clear
  observation or external reset/POR removes the known contingency.
- Invalidate readiness for observed BMCR reset even if release/deadline fails,
  including the sampled first frame of restoration RMW. A neutral write cannot
  erase reset evidence or produce a falsely verified result.
- Observed PHYRCR restart invalidates link/history while retaining settings;
  HARD invalidates readiness too, even with a later release/deadline error.
- Allow live raw/collector/link observation during cable waits while retaining
  HARD's persistent silence and reset-wait rejection.
- Reapply the full saved tail after any interrupted restoration and later HARD.
- Correct RAW tests to check its six owned controls; it does not own recipe-only
  GENCFG bits. This preserves the documented ownership boundary after HARD.

The pure helper checks every selected row before publication, uses matching
index/amplitude/whole peak, and searches 2,3,4,5,1B,1A. The complete table 2-5
sample gives run5/open/100m at 4.6 ns/m. NO_PEAK has no distance/health claim.
Maximum amplitude is flagged without inventing saturation semantics. Raw bits
11/5 remain visible; arbitrary raw/B-pair data have no derived interpretation.

## Verification and evidence

Local environment: Windows PowerShell, GNU C++ 15.1, C++17, Ninja, Release.
Assertions use explicit executing checks and a registered failure-path probe.

| Layer | Result and scope |
| --- | --- |
| Host | PASS: 136 registered Release CTests, including eight cable suites and failure probe. Independent traces, every frame/release/cancel boundary, clocks/deadlines, raw matrices, reread, calibration, restoration and bus ownership. |
| Portable consumer | PASS: tests-OFF separate build; ordinary consumer and new raw cable consumer execute. Two cached views do no I/O; stop retains the earlier capture result. |
| Python / source tools | PASS: 103 Python tests, ledger/reference integrity, Doxygen including negative fixtures, deterministic version check and diff check. |
| SDK/package | NOT RUN: no native controller or release package changed. Portable source linking is not SDK evidence. |
| Remote CI | NOT RUN: first-execution checkpoint remains prompt 71. |
| Physical | NOT RUN: Q7, actual timing, engine abort, routing, GENCFG inference and distance accuracy require an identified admitted fixture campaign. |

Commands: `cmake -S . -B build/host -G Ninja -DCMAKE_BUILD_TYPE=Release
-DDP8382X_BUILD_TESTS=ON`, `cmake --build build/host`,
`ctest --test-dir build/host --output-on-failure --no-tests=error`.
Separate consumer: configure `test/consumer` into `build/cable25-consumer` with
Release and `DP8382X_BUILD_TESTS=OFF`, build and run its CTest.
Also run `python -m unittest discover -s test -p "test_*.py" -v`,
`python tools/verify_ledger.py`, `python tools/verify_references.py`,
`python tools/check_doxygen.py`, `python scripts/generate_version.py check`,
and `git diff --check`.

README/CHANGELOG, public Doxygen, API/architecture/initialization/device guides,
vectors, ledger, coverage, audit dispositions, test guide and progress are updated.
The [cable guide](../../cable.md) specifies bounded behavior and an actionable
known-good/open/short/length fixture plan. `tdr_recipe`, `tdr_calibration` and
`tdr_freshness_abort` remain visible; this software completion does not close
them or invent EEE restoration before that feature is implemented.
