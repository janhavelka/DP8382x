# Prompt 51 · Diagnostic and opt-in control fixtures

Implementation phase (b) started on `main` at
`a13485aef2a6bcc34f172052a881e3876367d0a7` with a clean index and worktree.
The wrapper recorded equality with `origin/main`. Prompt 50's synchronized
completion maps to `d31ed99e8b2f151925c01e03c621f1f4a633ae12` after the
owner-authorized attribution update. The wrapper owns the phase (b) commit
and remote equality check. Phase (c) independently reviews that checkpoint.

## Evidence and implementation

Prompts 46 and 50 supply the typed CLI and the bounded campaign session.
The lead checked `Dp8382xCli.cpp`, the diagnostic/control/pin command
tables and their feature handlers and formatters. Scheduled command modes
determine deadline and response form. The handler determines whether an
actual token is admitted. Feature admissions carry no feature payload.
The host assertions use the actual direct or cooperative receipt for each
selected command and retain any consumed partial fields in the transcript.
[Typed CLI report](46-cli-diagnostics-controls.md) · [Campaign report](50-hil-observation-modes.md)

| Read-only sibling revision | Source precedent used |
| --- | --- |
| MB85RC `a9a83cafd6cd84cd41e7233332e854d9c32cde47` | `tools/test_hil_runner.py::test_diagnostic_prompt_requires_all_counters_and_restore_proof`, classifier in `tools/hil_runner.py`, restore verdict in `examples/common/DiagnosticCore.h` and native diagnostic example. Require the complete counter and restore receipt. FRAM scratch state supplies no PHY semantics. |
| PCA9555 `b9297a45a6f18fa8251e95857aedea0588a32e27` | Destructive gates and `test_aggregate_destructive_failure_runs_recovery_before_stopping` in `tools/test_run_i2c_hil_parser.py`, production sequence in `tools/run_i2c_hil.py` and direction/latch ordering in the fixture examples. Keep primary and independently eligible restoration results. Use DP8382x session continuity and a verified typed exit profile. |
| TCA9548A `63b356afd6cb90642628e407548cacea3851aa96` | `tools/test_tca9548a_hil.py` runs lost-prompt, readback and failed-mask recovery tests. Compare `run_mask_checks` in `tools/tca9548a_hil.py` and state/restore paths in `examples/common/Tca9548aCli.cpp`. Preserve uncertainty when framing fails. The external mask fixture supplies an evidence pattern. |

Exact-part source remains SNLS647I Rev I, SHA-256
`4e440fb8bd1cd44ee9ab0773944bef4f0605884f25c4ab96ea55d0ed859fd9c3`.
Rechecked the diagnostic and control sections and tables, including BISCR and
COMPT, PHYSCR and MISR status, and the WoL control/indication separation.
SNLA428 tables 2-1 and 2-5 are used with the existing D3 decision and raw
capture qualifiers. SNLA266B tables 5-1, 5-2, 5-4 and 5-5 apply through the
existing exact-part electrical procedure. No silicon encoding, register
permission or timing fact changes in this phase. The pinned ESP-IDF
reference remains v6.1 at `fff9895c82d744c7237be8847347bdd1b07c6643`.
[Reference archive](../../reference/README.md) · [Independent values](../../register-vectors.md)

The two implementation agents used the current coding model and owned
separate diagnostic and control modules. The lead coordinated the common
session/fixture interface, integrated the case selector, tests and docs, and
owns the commit request. Both agents finished their offline checks before
handoff. G7-5 uses the actual compiled CLI and core. G7-10 removes, corrupts
and duplicates every field of each bound positive transcript. Per-family
implementation runs rejected all 132,651 such mutations. Diagnostic cases
also passed 50 sessions with replay across BASIC and ENHANCED operation.

- Twenty-five diagnostic cases cover eight loopback selections, seven BIST
  selections, six-row/manual-raw/autorun TDR, four compliance stimuli,
  both MII collision rates and conditional calibration recovery. Owned
  stop results retain uncertainty and require readback. Digital/TDR stops
  verify integrated HARD and replay. Compliance and calibration authorize
  separate reinitialization only after a proved quiescent HARD exit.
- Twenty control cases cover IRQ/event service, BASIC and ENHANCED pins,
  MII/RMII and two distinct repeater roles, IEEE/active-sleep/PLL/deep power,
  EEE and forced LPI, three WoL choices, quality, raw MSE and analog control.
  The source-gated deep-power attempt verifies zero-frame rejection.
  A target that may retain persistent analog, quality or pin effects can
  use the separately authorized no-reset route only after checking its
  retained ownership and saved desired link profile. Typed baseline
  restore and independent readback still follow that replay.
- The compiled baseline prelude is fixed per case and runs before the
  session nonce. The bound session rechecks owned target and baseline
  fields. Only typed, fixed child actions supply simulated disturbance
  evidence. Neither fixture arguments nor operator declarations can
  authorize a native grant or substitute for instrument observations.
  The status/report schema and replay of prior cases remain compatible.

The longest compiled diagnostic path used 42 correlated commands, one
prelude command and two interventions. The longest control path used 48
correlated commands, one prelude command and at most one intervention in a
control case. Cleanup failure exits that branch and preserves both receipts.
The separate persistent debt replay adds bounded preflight, initialization
and re-observation in cases whose ordinary path uses 33 commands.
The combined prelude/campaign ceiling remains 64 commands with four
interventions. Successful exit requires restored feature readback where
applicable, profile verification, live link, idle/no-debt slot and owner
transport and health evidence.
[Hardware fixture requirements](../../hardware-validation.md#feature-fixtures) · [Coverage](../../feature-coverage.md)

## Local verification

Windows x86-64, GNU 15.1.0, Ninja 1.12.1, CMake 4.0.1 and Python 3.12.10.

| Layer | Result and scope |
| --- | --- |
| Release host fixture | PASS · 9/9 focused CTests. The new case suite replays 44 passing compiled cases and one source BLOCKED. Compiled negatives exercise primary/cleanup failure, guarded counters, output pressure and command admission. No physical inference from the simulated words. |
| Python and source checks | PASS · 186 Python tests ran successfully and 24 compiled-fixture tests skipped in discovery. Doxygen generated 293 pages and rejected two negative fixtures. Version, reference, ledger and diff checks passed. |
| Portable/package consumer | NOT RUN in phase (b). The implementation changes runner code and the host fixture. |
| ESP-IDF link | NOT RUN in phase (b). No SDK or firmware source changed. |
| Remote CI | NOT RUN. Prompt 71 owns first-execution remote CI. |
| Physical | NOT RUN. Stock P4 feature grants and first-board INT remain BLOCKED. |

```powershell
cmake -S . -B build/p51-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON
cmake --build build/p51-release --target dp8382x_cli_fixture
ctest --test-dir build/p51-release -R '^cli_(hil_(features|campaign|runner|serial)|.*transcript)$' --output-on-failure --no-tests=error -j 4
python -S -m unittest discover -s test -p 'test_*.py' -q
python -S tools/check_doxygen.py
python -S scripts/generate_version.py check
python -S tools/verify_references.py
python -S tools/verify_ledger.py
git diff --check
```

Physical execution needs a fixture-specific application grant, justified
management and restore route, independently observed stimulus and the
equipment in the hardware guide. The stock P4 examples supply no grant.
Repeater needs two independent owners and real traffic. Raw autorun needs
capture provenance, and TDR distance needs characterized cable/calibration.
Live Secure-ON values stay outside the generic runner. Preserve the named
source gates for deep-power exit, BIST interpretation, pin/clock routes,
analog receiver gain and COUNT100. No local serial, peer, power or bench
action was performed in this phase.

## Phase (c) independent review

The wrapper committed and verified remote equality for
`cad68249178468525315dca3254739ab62e6a145`. Phase (c) started at that
clean checkpoint on `main`. Two read-only reviewers used the current Sol
model at high effort for diagnostic and control ownership, effects and
source semantics. The lead reviewed shared correlation and evidence,
integrated the fixes and reviewed their final diff. Both reviewers finished.

The review reopened the sibling revisions above, including their public
drivers, CLI examples, runner classifiers and actual failure tests. Their
qualified recovery and readback precedents apply to the same-boot owner
and independent result receipts. SNLS647I tables 8-19, 8-35 through 8-40
and 8-90/91 anchor collision timing, IRQ/BIST and wake evidence. SNLA428
and SNLA266B remain the TDR and electrical fixture references. The local
ESP-IDF checkout still resolves to `fff9895c82d744c7237be8847347bdd1b07c6643`.

- A malformed feature payload in a correlated reply fails the assertion
  and leaves the independently guarded typed stop eligible. Broken framing,
  boot continuity or operation correlation still ends command admission.
  A failed restoration keeps the successful primary receipt intact. Exit
  compares the ordinary link state and valid partner evidence to entry.
- An admitted autorun enable retains desired intent even after a prewrite
  failure. A clean slot permits the typed disable. Retained persistent debt
  requires explicit no-reset replay authorization, verified saved link
  intent and known autorun target. Restoration then disables the policy
  and verifies both the retained state and CR1. Reversible stimulus paths
  for IRQ and LPI require evidence of a possible temporary effect before
  sending a stop.
- Interface gates run before a fixture prelude. WoL fixtures exercise
  modeled RXFS and MISR2 event history, plus separate level acknowledgment
  in level mode. The hardware guide specifies COL timing and the selected
  nominal wake pulse width. Packet matching, endpoint wake and waveforms
  remain physical qualification work.

Windows x86-64 Release GNU 15.1.0 passed the same focused CTest selection
above, 9/9. The feature suite covers all 44 positive nonce-correlated
transcripts and the source-blocked deep-power path. It exercises every bound
field by removal, corruption and duplication, and includes regressions for
the review findings. A separate supported-mode sweep passed 50 diagnostic
and 35 control sessions, with two deep-power BLOCKED cases. Including
prelude, the respective command maxima were 44 and 50, with two and one
interventions. Python discovery ran 215 tests with 29 compiled-only skips.
Doxygen, version, reference, ledger and whitespace checks passed.

Portable/package consumer, ESP-IDF link, remote CI and physical evidence
are NOT RUN in phase (c). Native feature grants and first-board INT remain
BLOCKED. Phase (d) owns final verification and the completion decision.
