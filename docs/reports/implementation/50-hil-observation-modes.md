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

## Stage (c) review

The solo review started from clean `main` at
`e123226e6687a65bf38c225461b497976a5ea059`. The wrapper verified equality
with `origin/main` at 10:39:27 on 2026-11-21. The index was empty.
The three sibling revisions above were unchanged. The review reopened the
TCA9548A and MB85RC classifiers, PCA9555's classifier, snapshot invariant
and aggregate recovery tests, and the shared CLI, native owner and both
native application status producers. The local SDK remained at
`fff9895c82d744c7237be8847347bdd1b07c6643`.

- The coupled crossover flags round-trip under `validateLinkSettings` in
  `Config.h`, which requires identical pair and polarity swap intent.
  Native `Phy::startConfigure` rejects the extended forced half-duplex
  combination, which the campaign screens before its first mutation.
  The compiled fixture still proves independent verification and full
  profile replay after a partial apply, including the separately authorized
  reinitialization path.
- `src/DP8382xLink.cpp` rejects inconsistent BMSR/PHYSTS bookends with
  `BusyDetail::OBSERVATION_CHANGED`. The CLI supplies an unavailable
  snapshot on that terminal. Count only that exact rejection as a retry
  within the same attempt/time bounds. Down samples still withhold
  resolution, qualified partner and pause evidence, independently of
  the raw AN completion flag. The compiled regression drives a simulated
  peer transition inside the actual bookends.
- `Phy::readMmd` uses stopped-only native admission. Exclusive cold-boot
  now proves the application's stop and maintenance state before the
  VOD read. The concurrent version uses cached evidence. Both retain
  the distinct boot, first request and declared power record requirements.
- Serial operator acknowledgments echo a fresh prompt code and must finish
  strictly before the deadline. Replay enforces the same deadline receipt.
  Operator input failure leaves an explicit receipt and allows eligible
  peer recovery. An incomplete late frame prevents sending the session
  nonce. Partial mutation or cleanup leaves unresolved effects in the
  artifact, with incomplete restoration distinct from a reported failure.
  Compiled and injected tests exercise those boundaries without physical I/O.

Focused host PASS on Windows x86-64, GNU 15.1.0, CMake 4.0.1, Ninja 1.12.1
and Python 3.12.10. The separate `build/p50-review` Release build used
`DP8382X_BUILD_TESTS=ON` and linked `dp8382x_cli_fixture`.
`ctest --test-dir build/p50-review -R '^cli_(hil_(campaign|runner|serial)|.*transcript)$' --output-on-failure --no-tests=error -j 3`
passed 8/8 registrations. Standalone Python discovery passed 183 tests
with 15 skipped out of 198. Reference, ledger, Doxygen, generated version
and whitespace checks passed. Portable/package consumer, SDK link, remote
CI and physical evidence were NOT RUN in this review. The first-board
INT case remains BLOCKED. Stage (d) owns the completion checks and record.

## Stage (d) completion

On 2026-11-21, `main` and the local `origin/main` ref both pointed to
`e123226e6687a65bf38c225461b497976a5ea059`, with zero ahead/behind and an
empty index. The stage (c) session finished and left exactly the eight
paths listed in `commit_20261121-104703.json`. The wrapper log contains
no commit or equality receipt for those changes. Stage (d) verified the
pending diff again and included it with the completion record in one
eight-path request. The wrapper records the resulting SHA and upstream
equality for `janhavelka/DP8382x`.

One read-only subagent used the inherited model for the sibling and receipt
audit while the lead ran verification and owned the completion record. The
TCA9548A and MB85RC revisions above were still clean. PCA9555 was clean at
`b9297a45a6f18fa8251e95857aedea0588a32e27`; the delta changes attribution
guidance in `AGENTS.md` and `CLAUDE.md` only. The exact rejection, snapshot
invariant and eligible recovery tests retain the conventions above. The
lead reopened the archived SNLS647I reference, the CLI identity and link
producer, stopped-only `Phy::readMmd`, the registered fixture tests, and
README, CHANGELOG and the runner, example and hardware guides. The code
under verification changes the runner and simulated fixture. The native
component and application sources remain at their prior verified inputs.

Windows NT 10.0.26300 x86-64, GNU 15.1.0, CMake 4.0.1, Ninja 1.12.1 and
Python 3.12.10. The local v6.1 SDK checkout matched
`fff9895c82d744c7237be8847347bdd1b07c6643`.

- Host PASS. `cmake -S . -B build/p50-final -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON`,
  `cmake --build build/p50-final --parallel 4` and
  `ctest --test-dir build/p50-final --output-on-failure --no-tests=error -j 4`.
  All 264 registrations passed with Release assertions enabled. The campaign
  registration ran 18 tests. It exercises 22 positive simulated cases,
  over 30,000 field removal, corruption and duplication checks, and the
  explicit INT block. The runner and serial registrations ran 14 and 17
  tests, respectively.
- Focused UBSan trap PASS, 8/8 CTests. Configured `build/p50-final-ubsan`
  with the same Release/test options and
  `-DCMAKE_CXX_FLAGS="-fsanitize=undefined -fsanitize-undefined-trap-on-error"`.
  Built `dp8382x_cli_fixture` with `--parallel 3` and ran
  `ctest --test-dir build/p50-final-ubsan -R '^cli_(hil_(campaign|runner|serial)|.*transcript)$' --output-on-failure --no-tests=error -j 3`.
- Python PASS. `python -S -m unittest discover -s test -p 'test_*.py' -q`
  discovered 198 tests, with 183 passed and 15 fixture-dependent skips.
  The registered tests supply the compiled fixture.
- Offline artifact smoke PASS. Ran the guide's `10base-te` dry-run,
  `run-fixture` and `verify-artifacts` with `build/p50-final/test/cli/dp8382x_cli_fixture.exe`,
  simulated identity `abcdef0`, host/sim-mdio, DP83826I ENHANCED and
  RMII address 1. `build/p50-final/evidence/campaign-10base-te.txt` is
  14,231 bytes with SHA-256
  `efa57f2bffd752cca2260c9423442280e84c38d37fc2acb31b0e1c738e345f99`.
- Portable consumer PASS, 2/2 CTests. Configured
  `cmake -S test/consumer -B build/p50-final-consumer -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=OFF`,
  built with `cmake --build build/p50-final-consumer --parallel 2` and ran
  `ctest --test-dir build/p50-final-consumer --output-on-failure --no-tests=error`.
  Exported/package consumer NOT RUN. Package checks remain with prompts 69–71.
- Reference, ledger, Doxygen, generated version and whitespace PASS.
  Ran `python -S tools/verify_references.py`, `python -S tools/verify_ledger.py`,
  `python -S tools/check_doxygen.py`, `python -S scripts/generate_version.py check`
  and `git diff --check cc9382f`. The reference check covered 29 archives,
  27 extracts and 2,171 local links. The ledger checked 97 registers,
  52 procedures and 35 features. Doxygen generated 293 pages and rejected
  both negative fixtures.
- SDK link NOT RUN for this change. Prompt 47 records the linked P4 example
  inputs. Remote CI NOT RUN under the first-execution prompt 71 policy.
- Physical evidence NOT RUN. Prompts 72–73 own authorized fixture execution.
  The reset-silent console, P4 PHY supply, reset/clock/strap readiness,
  peer/cable and reset-hold equipment remain to be qualified as applicable.
  First-board INT is BLOCKED by missing wiring. Polled cases are eligible
  on qualified hardware.
