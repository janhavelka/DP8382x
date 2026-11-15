# Independent audit of prompts 25-26

Date: 2026-11-15. Clean, fetched and synchronized entry on `main`:
`e1e14a84f80fd2826c245c231bfda8cbabfc6a8c`. Re-read both original prompts in
full and inspected the actual implementation diff from
`87b08d2ccb407d4fe33b1dc03920600855570c7a`, through manual capture `1aedc22`
and autorun `e1e14a8`. Final audit SHA and upstream equality are recorded in
the handoff. This audit does not start prompt 27.

## Scope and independent review

Reviewed `cable_configure`, `cable_manual`, `cable_capture` and `cable_autorun`
against COMMON, the operation contracts and original sources. The shared
application owner, operation slot, destructive collector and initialization
restore path remain authoritative. No SDK, worker, retry loop, allocated
framework or fixture operation was introduced.

Three inherited capable-model reviewers worked in parallel, with disjoint
scopes and no Git or sibling edits:

- `audit2526_sources`: independent original-PDF numeric/access/recipe review,
  public declarations, factual ledger and cable guide.
- `audit2526_lifecycle`: actual shared-driver diff and current state machine,
  deadlines, atomic RMW, cancellation, retained effects, reset and restore tails.
- `audit2526_tests`: executing registration, literal oracles, failure coverage,
  portable examples, sibling contracts and simplicity. After lead confirmation,
  this reviewer edited only `test/cable.cpp` to close the gap below.

The split gives independent reasoning to silicon semantics and ownership/error
boundaries. The lead reviewed the code and contributed diff directly, owned
shared documentation and Git, and repeated final verification.

## Findings and disposition

No production behavior defect was confirmed. One required test scenario was
missing: exhaustive failure/cancellation tests used a single selected row,
whereas multi-run tests covered successful plans. The existing code correctly
handled partial multi-run acquisition, but this behavior lacked a regression.

The registered `cable_capture` suite now fails or cancels row 1B after its index
read, with row 1A complete and requested row 2 not yet started. It proves:

- Prior samples and their original timestamps survive; frozen results do not
  change after a later acquisition.
- Explicit reacquisition completes only the current accepted trigger, using
  exactly 14 frames in five callbacks, with no writes or new trigger.
- Future selected rows stay `NOT_RUN`; the aggregate remains incomplete/stale,
  the interpretation output remains unchanged, and cleanup remains owed.
- Separately admitted stop/HARD restoration succeeds without starting another run.

The tests run within the existing suite; no new abstraction or redundant
production path was needed. The changelog's outdated statement that autorun
still belonged to a later stage was also corrected.

Potential concerns were checked and rejected: CDSCR averaging code 7 is
reserved, but the separate shadow-average field does not carry that restriction;
reserved result banks cannot be promoted to peaks; whole-word nonzero peak
classification follows the selected SNLA428 interpretation; equal autorun
bookends cannot establish freshness. CableWord uses a validated post-callback
upper timestamp, independently of optional per-frame timestamps. These are not
reasons to invent different controls, interpretations or timing guarantees.

Lifecycle review confirmed sticky calibration gates, no controller ownership
across waits, indivisible five-frame RMW, retained accepted effects, exactly-once
result consumption, rearmed saved-state restoration after a failed HARD tail,
and policy verification before completed initialization. Raw autorun acquisition
remains bounded and makes no generation/coherence claim.

## Exact sources and sibling comparison

Original SNLS647I pages 43, 61, 73 and 76-81, including section 8.3.16.1 and
tables 8-28/47/54-71, independently confirm the controls and result banks.
Original SNLA428 pages 3-7 confirm the six recipe rows/order, search thresholds,
same-row index selection and distance formula under the recorded D3 exception.
The current official [datasheet](https://www.ti.com/lit/ds/symlink/dp83826i.pdf)
still serves SNLS647I, revised August 2025. Live retrieval of SNLA428 failed in
the source reviewer; its archived original June 2023 PDF was examined directly.
Vendor bytes, licenses and hashes are unchanged; no vendor code was copied.

Fresh read-only sibling worktrees were clean at these revisions:

| Revision | Reopened paths and adopted/rejected principle |
| --- | --- |
| TMP1x2 `afd50889312f6e218a6a34eca687e3e8547d8b40` | `include/TMP1x2/TMP1x2.h`, `src/TMP1x2Operations.cpp`, `test/jobs/test_jobs.cpp`, CMake registration, `examples/common/Tmp1x2CliDiagnostics.cpp`, `tools/hil_tmp1x2_runner.py` and `tools/test_hil_tmp1x2_runner.py`. Adopt retained results, explicit validity and correlated fixture verdicts; reject sensor timing as a PHY procedure. |
| PCA9555 `8b461b2afe9910b5b15fe32cc049d52e3b498970` | Public lifecycle/agent guidance, `src/PCA9555.cpp`, `test/test_basic.cpp` cancellation, required-cleanup and deferred-observation cases and registration. Preserve primary versus restoration results; I2C cleanup and ACK rules do not specify MDIO or cable-engine behavior. |
| 24Cxx `1bd9f969d1df55ca5819b0d290ff331869e7d5bc` | `tools/test_hil_identity.py` and CMake registration. Fixture labels must not substitute for observed identity; EEPROM timing does not define TDR bounds. |

## Verification and documentation

Windows PowerShell, GNU C++ 15.1, Ninja, C++17 Release. Registered CHECKs and
intentional failure probes remain active under NDEBUG.

| Layer / command | Result |
| --- | --- |
| Release configure/build `build/host`, `ctest --test-dir build/host --output-on-failure --no-tests=error` | PASS: 144/144 after the added regression |
| Independent `build/audit2526-tests` | PASS: 144/144 baseline; 17/17 affected suites/probes after the change |
| Separate `test/consumer` configure/build in `build/audit2526-consumer`, tests OFF, registered CTest | PASS: 2/2; actual owner loops execute |
| `python -m unittest discover -s test -p "test_*.py" -v` | PASS: 103 tests |
| `python tools/check_doxygen.py` | PASS: 193 HTML pages, two rejected negative fixtures |
| `python tools/verify_ledger.py` | PASS: 97 registers, 52 procedures, 35 features; structure/coverage evidence |
| `python tools/verify_references.py` | PASS: vendor hashes, extracted references and local links |
| `python scripts/generate_version.py check`, `git diff --check` | PASS |
| SDK link / exported package | NOT RUN; this audit changes tests and documentation only |
| Remote CI | NOT RUN; reserved for prompt 71 |
| Physical trigger timing, quiescence, coherence and distance accuracy | NOT RUN; Q7 remains open |

Updated this report, progress, test guide and changelog. README, public Doxygen,
API/cable/initialization guides, vectors and ledger were reviewed; their behavior
and support statements remain correct, so no numeric or API rewrite was made.
No production simplification was justified: one explicit cable phase switch and
the shared restore path already express the responsibility directly. Source
gates `tdr_recipe`, `tdr_calibration` and `tdr_freshness_abort` remain explicit;
prompt 35 still owns calibration recovery. Host tests do not close those gates.
