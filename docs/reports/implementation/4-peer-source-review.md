# Prompt 4 peer and current-driver review

Reviewed 2026-10-28 for [prompt 4](../../prompts/4-silicon-diagnostic-ledger.md).
This delegated review owns only this report. Siblings, vendor payloads and Git
were read-only. The inherited capable model was used for the bounded source and
test comparison; separate stronger reviewers own original-PDF diagnostic and
destructive-access arithmetic. The lead owns ledger integration and verification.
No inspected peer test was executed and no hardware result is claimed here.

## Live peer revisions and paths

Paths below are relative to `C:/Users/Honza/Documents/Projects/`. Root rules in
all three siblings were reopened; the parent/project rules and prompt-71 remote
CI exception govern DP8382x work. All three trees were clean at inspection.

| Repository | Inspected HEAD |
| --- | --- |
| TMP1x2 | `bafc34067530eb340e55c42dae3fad4eb795b6e9` |
| PCA9555 | `498178e4d9e4acb6fab7d8b816ad5021efe8806a` |
| MB85RC | `1bf7c293549b1038b2d8f3b7a8d655fa74201e6e` |

| Actual declaration, implementation and test paths inspected | Adopted/adapted principle and limit |
| --- | --- |
| `TMP1x2/include/TMP1x2/TMP1x2.h`: `OperationResult`, `OperationSnapshot`, `RegisterSnapshot`, `SettingsSnapshot`; `src/TMP1x2Operations.cpp`: `admit`, `finishJob`, `stepJob`, `poll`, `cancel`, `takeResult`; `docs/owner-operations.md`; `test/jobs/test_jobs.cpp`: admission/retained-result, cancellation and deadline cases | Adopt one admitted request and retained terminal result, pure start validation, passive snapshots, identity-qualified consumption and possible-write-effect evidence. Keep desired settings distinct from observations/trust. Adapt post-write wait anchoring to observed completion or a conservative later sample. Reject copying temperature conversion intervals, temperature-format predicates or the rule suppressing all failed-operation samples: a PHY collector must retain successful destructive samples even when a later phase fails. |
| `PCA9555/include/PCA9555/PCA9555.h`: `ObservedState`, `OperationResult`, `SettingsSnapshot`; `src/PCA9555.cpp`: input-read and `POINTER_PARK` phases, `pollOperation`, `takeOperationResult`; `test/test_basic.cpp`: `test_input_timeout_runs_pointer_park_and_preserves_timeout_cause`, `test_cancel_cause_survives_deadline_during_required_pointer_cleanup`, `test_input_pointer_park_failure_keeps_valid_input_evidence` | Adopt per-sample validity, completed/uncertain masks, separate primary and cleanup status, preservation of a valid input despite cleanup failure, and cancellation that retains its cause while owed cleanup runs. Adapt to bounded PHY stop/restore and collector records; do not copy PCA pointer-parking errata commands, transaction counts, input polarity or I2C timing. |
| `MB85RC/include/MB85RC/Config.h`: `TransportResult`, `WriteCommit`; `include/MB85RC/MB85RC.h`: `WriteResult`, `VerifyDetailedResult`, `TransferResult`; `src/MB85RC.cpp`: `pollTransfer`, `getTransferProgress`, `takeTransferResult`, `resumeVerifiedWrite`, cancel/timeout methods; `test/test_basic.cpp`: `test_staged_budget_progress_and_terminal_result_are_retained`, `test_staged_read_fill_verify_failures_preserve_terminal_provenance`, `test_cancel_and_timeout_are_distinct_exactly_once_terminal_results` | Adopt definite completed-prefix evidence, terminal result retention and separation of accepted transport from verified effects. An error need not mean no mutation; reconciliation retains the original error. Adapt completed bytes to independently acquired PHY words/events. Reject replaying an uncertain trigger or destructive read as though it were idempotent memory readback. Memory write-protect, chunk geometry and reconciliation timing do not specify PHY behavior. |

For the PHY, cancellation/timeout must stop new primary work, retain every
successfully acquired event/counter word and any ambiguous history loss, and
perform only the separately bounded authorized cleanup. A completed restoration
does not erase primary failure. A cached snapshot must perform no I/O. Controller
serialization must span complete MMD/RMW transactions; an I2C callback's natural
transaction boundary does not establish the equivalent MDIO boundary.

## Examples, runners and workflow comparison

Reopened `TMP1x2/examples/common/Tmp1x2CliDiagnostics.cpp` start/poll/finish and
diagnostic admission. It copies consumed terminal evidence into CLI-owned state
and rejects overlapping diagnostics. Reopened
`PCA9555/examples/common/CliShell.h`: this is an Arduino line reader with a finite
input length, not a portable PHY operation dispatcher. Reopened
`MB85RC/examples/common/DiagnosticCore.h`: `restoreVerified`, `takeStagedTerminal`
and its bounded staged-transfer polling helper. These examples support explicit
restoration and retained results, not a generic firmware diagnostic pass claim.

The inspected runner/parser paths were:

- `TMP1x2/tools/hil_tmp1x2_runner.py` and `tools/test_hil_tmp1x2_runner.py`:
  firmware identity, incomplete/deadline response handling, token/kind matching,
  conflicting/duplicate-record rejection and truncated-output failure.
- `PCA9555/tools/run_i2c_hil.py` and `tools/test_run_i2c_hil_parser.py`:
  bounded per-command timeouts, explicit destructive-command gating, safe command
  sequence, prompt-aware completion and ACK-versus-device-identity distinction.
- `MB85RC/tools/hil_runner.py` and `tools/test_hil_runner.py`:
  complete payload/diagnostic/restore proof, preserved partial read errors,
  short-write handling and stopping after response timeout.

Adopt finite sessions, exact operation correlation, complete evidence and an
explicit restoration outcome. Adapt the grammar to the future shared PHY CLI;
neither a console prompt nor received traffic proves BIST/TDR success. Parser
tests prove parsing only. Fixture identity, cable termination, speed/interface,
instrumentation and physical observation remain separate requirements.

Reopened each sibling's `.github/workflows/ci.yml`. TMP registers CMake/CTest and
contract checks plus SDK builds; PCA/MB register native PlatformIO and Python
checks, with PlatformIO 6.1.19. Their S2/S3 and sensor/memory builds establish no
P4 Ethernet result. No workflow run was dispatched, queried or awaited. Existing
DP8382x unittest discovery remains the simple research-suite registration.

## Fresh primary-driver comparison

GitHub connector reads reopened current branch commits and the actual source
files below. No source implementation was copied or archived. Per-file notices
were inspected; mixed repository licensing was not inferred from its root.

| Immutable source and files inspected | License, observation and diagnostic limit |
| --- | --- |
| [TI RTOS](https://github.com/TexasInstruments/ethernet-rtos-drivers/tree/94bf4343004732983b0b8e1eec123d88ff70453b), current `main` still `94bf4343004732983b0b8e1eec123d88ff70453b`; `rtos_drivers/src/dp83826.c`, `dp83826_priv.h` | BSD-3-Clause in both files. The config path uses the loopback flag to disable Auto-MDIX, but supplies no complete mode-specific loopback/BIST/TDR acquisition/restoration implementation. PRBS LED macros do not establish a BIST result API. Existing reset/address and ignored-write-error findings remain in the core catalogue. |
| [TI MCU+ ICSS-EMAC](https://github.com/TexasInstruments/mcupsdk-core/tree/4b9a66f4e18e02a834961f5f834155c846cab041), current default branch `next` remains `4b9a66f4e18e02a834961f5f834155c846cab041` (commit date 2026-08-20); `source/board/ethphy/icss_emac/ethphy_dp83826e.c` and `.h` | BSD-3-Clause notices in both files. Repository metadata confirms `next` is default; `main` is a separate older branch at `517a277791c1de252189bf2d650395726b494dd1` (2024-12-19). Both files were fetched at both commits and their contents are identical. These observations do not establish a newer implementation. The inspected files contain no loopback/BIST/TDR/counter procedure; ordinary configuration functions do not settle diagnostic conflicts. |
| [TI combined Linux](https://github.com/TexasInstruments/ti-ethernet-software/blob/450af1144b8caeb58724bcbae081b409f6bf5c3a/linux_drivers/dp83822.c), current combined-tree `main` still `450af1144b8caeb58724bcbae081b409f6bf5c3a`; `dp83822_config_intr`, `dp83822_handle_interrupt` and driver registration | SPDX GPL-2.0, factual comparison only. Status high byte is intersected with enabled low-byte sources shifted up. The handler consumes MISR1 then reads MISR2; if MISR2 fails it returns without publishing the first sample. This cannot satisfy the required retained partial collector evidence. Mask configuration also reads destructive status; it needs the same collector ownership. No complete BIST or TDR operation appears in this file. |
| [TI combined Zephyr](https://github.com/TexasInstruments/ti-ethernet-software/blob/450af1144b8caeb58724bcbae081b409f6bf5c3a/zephyr_drivers/dp83826a_driver/driver_files/drivers/ethernet/phy/phy_ti_dp83826x.c), same current commit | SPDX Apache-2.0. Source inspected for diagnostic methods and related terms; no loopback/BIST/TDR/counter procedure was found. Its A-generation directory label and ordinary link configuration cannot resolve original DP83826I diagnostic ambiguities. |

These are bounded file-level findings, not claims that no other public source
exists. Exact TI document conflicts and targeted support searches belong to the
parallel silicon reviews. The [existing catalogue](../../reference/library-ti-features.md)
remains comparison input, with original PDF evidence taking precedence.

## Coverage-check recommendations for integration

The entry checker and `test/test_ledger.py` already reject malformed records,
field overlap, invalid owned masks, dropped core procedures and unresolved MMD
aliases. Prompt 4 should extend these existing checks with a small explicit set
of researched rows and required procedure-to-feature owners. Require completed
software features to have a procedure beyond stage 3, and prevent a completed
diagnostic row silently reverting to PENDING. Keep all-reserved banks valid and
non-writable; unknown reset bits must remain unknown. Tests should delete each
required procedure with its backlinks and demote required rows to prove rejection.

At entry, the ledger assigns 37 register rows and eight feature families to
prompt 4. Shared records include PHYSCR, RCSR, 10BTSCR, RXFS and FLD controls;
researching their complete bit partitions must not silently claim completion
of the separate prompt-5 power/interface/WoL procedures. Numeric/access test
oracles must come from the independent PDF reviews, not the integrated ledger.
Record per-sample validity, history-loss uncertainty, cleanup bounds and fixture
requirements in the accepted procedure contracts and future driver tests.

Delegated verification: source/path/revision inspection PASS. Sibling host tests,
portable/package consumers, SDK builds, remote CI and physical evidence NOT RUN.
The lead records integrated ledger/reference/unit-test/diff results and sync.

## Integrated checker review

Reopened the actual working `tools/verify_ledger.py`,
`test/test_diagnostic_ledger.py` and integrated JSON rather than relying on the
source-review summaries. The part-specific action table keeps exact exceptional
writes separate from ordinary field access; it does not add a generic execution
framework. Exact action tuples, required procedure/action links and recipe
address/name mappings now reject the preliminary bypasses identified above.

The delegated run passed `python tools/verify_ledger.py` (96 registers,
28 procedures, 35 features) and all 54 registered tests. Independently exercised
1,279 malformed scalar/container substitutions across action, diagnostic
procedure and recipe records: 1,245 were rejected, 34 valid-shape empty
ownership/gate arrays remained accepted, and none raised an uncaught exception.
Targeted recipe rename/demotion, recipe address swaps, missing digital action
links, action retargeting to identity, widened action masks and null CDCR command
neutral were rejected. Changing CDCR's access label to W1S remains structurally
representable but is caught by the independent access oracle, as intended.

Recommended extending literal ownership assertions to all diagnostic procedures
and preserving mandatory gate references on the four exception actions: empty
ownership/gate arrays are valid structural forms elsewhere, so their semantic
completeness belongs in the independently reviewed procedure oracle. These
checks establish research structure and sourced expectations, not executing
driver or physical behavior.
