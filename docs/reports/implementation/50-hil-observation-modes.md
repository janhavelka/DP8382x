# Prompt 50 · Finite observation and link-mode campaigns

Stage (a) assessed prompt 50 on synchronized `main` at
`cc9382f1f86d656fdd2a6e531dab5ea4a313577d`, the prompt-49 completion
checkpoint. The wrapper recorded remote equality for that commit at
10:05:36 on 2026-11-21. Stage (b) started with a clean worktree and index
and zero ahead/behind the tracking ref. The wrapper owns all Git writes and
synchronization. The operator directed solo GPT-6.2 Sol at high effort for
the owner/admission, recovery and transcript correlation work.

## Source audit

Prompt 49 supplies nonce correlation, one outstanding command, the native
optional serial backend and bounded open, write, receive, tail and close.
Prompt 48 supplies the decoder, child lifecycle and two-file evidence
carrier. The prompt-50 audit dispositions G7-5, G7-10 and G7-14 govern real
producer coverage, mutations of every required field, and the explicit
equipment/INT matrix. There were no active workflows or custom hooks at
entry. No register procedure has implementation owner 50.
[Prompt 49 report](49-runner-serial-session.md) · [Audit dispositions](../promptset-audit-dispositions.md)

| Clean sibling checkout | Paths and decision |
| --- | --- |
| TCA9548A `4a244adabf38b432cef9eb2686fc5f3e3547c0fb` | Root AGENTS.md, `include/TCA9548A/TCA9548A.h`, `src/TCA9548A.cpp`, `examples/common/Tca9548aCli.cpp`, `tools/tca9548a_hil.py` and `tools/test_tca9548a_hil.py` inform entry/readback proof and retained unexecuted rows. Preserve DP8382x typed evidence and nonce correlation. |
| PCA9555 `0aec2212894910e0d872c3e511fbcd740837c5f7` | Root AGENTS.md, `include/PCA9555/PCA9555.h`, `src/PCA9555.cpp`, `examples/espidf_basic/main/main.cpp`, `tools/run_i2c_hil.py` and CI-registered `tools/test_run_i2c_hil_parser.py`. Use exact rejection, before/after snapshot invariants, first failure and independent eligible recovery. Reopen `test_fault_guard_snapshot_invariant`, `test_expected_rejection_classification_is_fail_closed` and `test_aggregate_destructive_failure_runs_recovery_before_stopping`. |
| MB85RC `3ace168eb01d687ab60e9fe88cc72322e5cae260` | Root AGENTS.md, `include/MB85RC/MB85RC.h`, `src/MB85RC.cpp`, `examples/espidf_basic/main/main.cpp`, `tools/hil_runner.py`, `tools/test_hil_runner.py` and `tools/test_hil_health_snapshot.py`. Require complete diagnostic fields and independent restoration evidence. I2C scans, GPIO outcomes, human prompts, serial reconnect and scratch writes supply no PHY authority. |

Production evidence comes from `examples/common/Dp8382xCli.cpp` command
metadata, `Reply::identityFields` and `Reply::linkFields`, the shared
`Dp8382xCliCore.h` and `examples/esp_idf/common/Dp8382xCliNative.h`, and
the basic/owner `main/main.cpp` application status producers. Native link
and raw-read access use stopped-only owner routes in `src/esp_idf/Phy.cpp`.
The unchanged native SDK comparison is ESP-IDF v6.1 at
`fff9895c82d744c7237be8847347bdd1b07c6643`. Only the example test fixture
C++ changed.

SNLS647I section 8.3.1 and tables 8-19/20/22–27/34 establish distinct
reset/AN and management evidence. Revision-1 literals are
`0x2000:0xA111` BASIC and `0x2000:0xA131` ENHANCED. The application
declares the fitted I suffix. Section 8.3.13 and table 8-81 establish the
10BASE-Te VOD_CFG3 word `0x4A40` at vendor offset `0x030E`. The real
core writes and verifies it via DEVAD 31. `src/DP8382xLink.cpp` qualifies
the live snapshot and the directional pause fields. The CLI records retain
the one-microsecond observation timestamp allowance. No silicon recipe
changed. [Archived primary reference](../../reference/README.md)

## Stage (b) implementation

- `tools/dp8382x_hil_campaign.py` owns the typed cases and bounded procedure.
  The canonical runner drives one dynamically admitted request at a time
  through the existing decoder and serial/fixture carriers. The old suite
  shapes and schema-1 receipts remain readable.
- A positive mutating case establishes a replayable owned entry profile and
  independently verifies it before any change. It captures target evidence,
  preserves an admitted operation's first failure and checks the saved
  profile after cleanup. A local mutation denial cannot authorize a new
  mutating cleanup command. A known cleanup-debt target can be replayed
  under explicit no-reset initialization permission before restoring the
  saved entry profile.
- Typed records distinguish valid advertisement, qualified
  AN partner ability, resolved mode and directional pause. The peer action
  receipts preserve exact actions, bounds, outcomes and their previous
  request. The compiled fixture's peer transitions are private child
  controls. The native path accepts finite operator declarations and
  requires freshly correlated link samples.
- The 23 case IDs are registered against the guide. The compiled fixture
  exercises 22 available cases, with the unwired INT case explicitly
  BLOCKED. Generated tests delete, corrupt and duplicate each field
  occurrence in every positive case transcript. The suite also covers
  semantic contradictions, artifact replay, wrong identity, denied
  maintenance, missing equipment, timeout/decline, incomplete or recovered
  restoration, and zero-case or skip-only runs.
- Updated the hardware table, runner/CLI guides, README, changelog, native
  example instructions, fixture guide and feature matrix. Public C++
  declarations and Doxygen contracts are unchanged.
  [Cases](../../hardware-validation.md) · [Runner](../../hil-runner.md)

## Checks and gates

Windows x86-64, Python 3.12.10, GNU 15.1.0, CMake 4.0.1 and Ninja 1.12.1.

- Host PASS. Configured `build/p50-implementation` using Ninja,
  `-DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON` and built
  `dp8382x_cli_fixture`. The command pattern and results follow.
- `ctest --test-dir build/p50-implementation -R '^cli_(hil_(campaign|runner|serial)|.*transcript)$' --output-on-failure --no-tests=error`
  PASS, 8/8 registrations including compiled campaign, serial and existing
  transcript suites. `cli_hil_campaign` uses the real core and CLI, with
  a registered 90-second ceiling.
- `python -S -m unittest discover -s test -p 'test_*.py' -q` PASS,
  192 tests discovered, 180 passed and 12 skipped in standalone discovery.
  The live fixture-dependent assertions run in the registered CLI tests.
- Compiled 10BASE-Te dry-run, run-fixture and verify-artifacts PASS with
  the documented simulated firmware `abcdef0`, host/sim-mdio, RMII address
  1. A native serial dry-run previewed the observe case without opening
  the declared dummy port or creating its evidence files.
- `python -S tools/verify_references.py` PASS, 29 archives, 27 extracts
  and 2171 local links. `python -S tools/verify_ledger.py` PASS,
  97 registers, 52 procedures and 35 features.
  `python -S tools/check_doxygen.py` PASS, 293 HTML pages and two
  negative fixtures. `python -S scripts/generate_version.py check` and
  `git diff --check` PASS.
- Portable/package consumer and SDK link NOT RUN for this implementation
  checkpoint. The runner and fixture use the already available CLI routes.
  Remote CI NOT RUN under the first-execution prompt 71 policy.
- Physical evidence NOT RUN. Explicit fixture/port authorization, P4 PHY
  supply and reset/clock/strap readiness remain open. INT pin evidence is
  BLOCKED on the first board. A qualified peer and controlled reset hold
  are needed for the corresponding physical fault cases. Bench execution
  remains in prompts 72–73.

Stage (c) independently reopens the landed changes, the source evidence
and the test outcomes. Stage (d) owns confirmation of the final local
checks and completion record.
