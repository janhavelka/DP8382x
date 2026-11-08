# Independent audit of prompts 13-18

Date: 2026-11-08. Entry checkpoint: `507dff3424668f7404760dca905070b4a4ad1e6f`,
clean and synchronized after fetch. Execution branch remains
`promptset/dp8382x-implementation-20261028`; base remains
`5e148fffde074b85fadc15d8ff66fe2c77111036`. Final checked commit and upstream
equality are recorded in the handoff, avoiding a self-referencing commit.

The lead reread all six original prompts and COMMON/shared contracts. Scope was
the existing portable facade, access/collector, identity, reset/lifecycle and
configuration/replay implementation, focused tests and maintained documentation.
One application MDIO owner, complete bounded transactions, retained evidence,
OP-9 restoration and source-specific masks remain mandatory. No sibling or
vendor payload changes, SDK support, physical operation or remote CI were added.

Ledger procedures checked: `identify`, `pcs_reset`, `hard_reset`, `restart`,
`external_reset`, `initialize_10base`, `configure_an`, `configure_forced`,
`next_page_exchange`, `extended_link_configure`. Stage 13's concrete acquisition
and mixed-register consumers extend the existing access procedures; later typed
features keep their recorded owners.

## Findings and corrections

| Finding | Actual behavior at entry | Correction and executing regression |
| --- | --- | --- |
| Interrupted verification masked the terminal failure | Both `_advanceConfiguration` and `_finishOperation` substituted CONFIG_MISMATCH after an earlier mismatch, losing the terminal MDIO status/detail. Baseline reproducer returned CONFIG_MISMATCH/0 while its batch retained MDIO_TIMEOUT/-18018. | Keep the first mismatch independently; return the error that interrupts the finite pass. CONFIG_MISMATCH requires a completed pass. `forced_verification_outcomes` covers CONFIGURE and NONE replay interrupted by transport, cancel, clock regression, deadline, invalidation and frame exhaustion. Existing negotiated/forced failure expectations were corrected. |
| AN replay claimed reset completion | `startInitialize({NONE})` with retained AN intent returned commandRequired=false, commandAccepted=false, protocolComplete=true. | Remove the AN assignment to the reset-scoped flag. Negotiated configuration and every reset replay now assert all three flags; negotiationComplete retains its separate meaning. |
| Missing callback receipts erased uncertainty | A fake consumed FLDS and returned OK with an empty receipt; core rejected the report but left historyLost=false. Missing semantic-write flags similarly escaped invalidation. | Classify reports before retaining effects. An unsound receipt marks unreturned requested destructive samples as possible lost history and semantic writes as uncertain. Valid prefixes survive; sound zero-attempt denials remain effect-free. `access_reports` covers FLDS, a valid PHYSTS prefix, missing FLDS/MISR suffix, full/partial indirect RXFS receipts and LED writes. Page evidence follows the same uncertainty rule. |
| Completed page loads disappeared on late failure | A completed ANNPTR write was counted only after aggregate success, so a later deadline/release failure omitted it. | Retain completed loads before terminal error handling. `negotiation_page_boundaries` checks both deadline expiry and release failure after the second accepted load, with consumed partner evidence and cleanup preserved. |

Coverage gaps were closed in existing registered negotiation suites: both message
codes 5/6 and their four ordered unformatted payloads, 18 truncated/invalid-order
requests, exactly 16 local/partner pages, and initial partner-toggle inversion
when base bit 11 is one. These use independent literal words, not production
masks. Added loops are bounded and indexed expectations are guarded.

The UBSan trap build exposed a signed-promotion warning in the revision-boundary
test; the model is now widened to uint32_t before shifting. No validation or
warning was disabled. A first draft of the new verification test accidentally
selected the default PCS reset; explicit NONE and a bounded loop corrected the
test before final verification. Identity/access guides contained stale future
initialization wording; they now describe the implemented admission guards.

## Independent review and source evidence

Three parallel reviewers inspected source/tests rather than accepting prior
completion summaries. `audit_access_identity` covered prompts 13-14 and exact
destructive tables; `audit_lifecycle` used GPT-6 Astra/high for timing, ownership
and reset reasoning; `audit_configuration` covered prompts 17-18 and original
Clause 28 evidence. The other two used the inherited coding model. Review was
initially read-only. Later the access/configuration reviewers owned only
`test/access.cpp` and `test/negotiation.cpp`; the lead reviewed their diffs and
owned production code, shared headers, documentation, integration and Git.
Lifecycle review rechecked the final production diff without further findings.

Independent directories: `build/audit-access-only`, `build/audit-lifecycle-only`,
`build/audit-config-only`; lead directories are listed below. Baseline reproducers
for error precedence/reset flags used extracted HEAD source, avoiding concurrent
working-tree changes. No additional confirmed defects were found in identity
admission, masks/access policy, wait arithmetic, overflow/regression/frozen clocks,
HARD quiet-floor persistence, token/cleanup/teardown, forced/AN transitions,
reserved preservation or reset replay.

TI's [current DP83826I PDF](https://www.ti.com/lit/ds/symlink/dp83826i.pdf) was
fetched into the ignored review directory and compared to the archived SNLS647I:
both are the August 2025 revision, SHA-256
`4e440fb8bd1cd44ee9ab0773944bef4f0605884f25c4ab96ea55d0ed859fd9c3`.
The original identity/access tables, reset tables 8-19/47/48 and timing page 15
were reopened. Configuration review used the original IEEE 802.3-2012 pages
294-298, 303-304, 320 and 734-736, including the rendered arbitration diagram,
alongside SNLS647I negotiation/forced tables. Source provenance and interpretation
remain in the [stage 17 source review](17-source-review.md)
and [stage 18 source review](18-source-review.md).
No silicon facts, licensing disposition or vendor bytes changed.

Fresh sibling checks were read-only and clean:

| Sibling revision | Reopened responsibilities |
| --- | --- |
| TMP1x2 `db7687a5b2dfeee29df75a5b559ae6dd02baa8bb` | `include/TMP1x2/Config.h`, public operation/result declarations, `src/TMP1x2Configuration.cpp`, `src/TMP1x2Operations.cpp`, probe, configuration/operation tests, common CLI and runner. |
| PCA9555 `414e0f66b13082e974ec27a4a6e86985a65d83fd` | Public configuration/lifecycle, `src/PCA9555.cpp` cleanup/shadow handling, ambiguity/detach and non-identity tests, CLI and runner. |
| INA228 `73f954838d3fac8cce747d54d0ad738940d4c625` | DiagAlert declarations and `src/INA228.cpp::readDiagAlert`, tracked destructive access, tests and cached/live display. |
| MB85RC `1a45e882bb3b1d29f5a18538a7658b0ed0f8a424` | `CommandTable.h`, `src/MB85RC.cpp` variant selection, raw ID read and active identity validation; rejection tests. |
| 24Cxx `e4e1eeb41575df462e585c54989950323c43ab7a` | Retained operation effects across waits, public contracts and tests. |
| TCA9548A `1997d3f9446057348adbc6b2b6872dd18c041de3` | Bus-silent end and lifecycle tests/contracts. |

Retained-result, desired/observed separation and precise error conventions remain
adapted to MDIO. I2C ACK identity, EEPROM polling and sensor delays are not PHY
evidence. Existing sibling workflow/runner conventions remain references only.
No repository CI workflow exists yet; creation and execution remain prompt 71.

## Verification and scope

Environment: Windows/PowerShell, GNU C++ 15.1.0 WinLibs, C++17, Ninja, Release
with active checks and strict conversion warnings. Final results:

| Evidence layer | Result |
| --- | --- |
| Host Release | PASS: 68/68 registered CTests, including nine deliberate failure probes. |
| Host UBSan trap | PASS: the same 68/68 tests with undefined-behavior trap instrumentation. |
| Portable consumer | PASS: tests-OFF separately linked consumer, 1/1 CTest, all existing owner examples. |
| Python/reference/ledger | PASS: 102 tests; 97 registers, 52 procedures, 35 features; archive hashes/local references and diff checks. |
| Doxygen/version | PASS: 153 HTML pages and two rejected fixtures; generated version remains current at 0.1.0. |
| SDK/package | NOT RUN: actual SDK linking and exported/installed packaging retain their later stage owners. |
| Remote CI | NOT RUN: first-execution prompt-71 policy; no dispatch or status queries. |
| Physical | NOT RUN: Q4/Q5/Q10/Q11 and other named fixture/source gates remain open. |

```sh
cmake -S . -B build/audit13-18-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON
cmake --build build/audit13-18-release
ctest --test-dir build/audit13-18-release --output-on-failure --no-tests=error
cmake -S . -B build/audit13-18-ubsan -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON "-DCMAKE_CXX_FLAGS=-fsanitize=undefined -fsanitize-undefined-trap-on-error"
cmake --build build/audit13-18-ubsan
ctest --test-dir build/audit13-18-ubsan --output-on-failure --no-tests=error
cmake -S test/consumer -B build/audit13-18-consumer -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=OFF
cmake --build build/audit13-18-consumer
ctest --test-dir build/audit13-18-consumer --output-on-failure --no-tests=error
python -m unittest discover -s test -p "test_*.py" -v
python tools/verify_ledger.py
python tools/check_doxygen.py
python scripts/generate_version.py check
python tools/verify_references.py
git diff --check
```

README/CHANGELOG, public Doxygen, API, identity, negotiation, side-effect, coverage,
test and progress guides were updated. Architecture, variants, numeric ledger,
literal silicon vectors, examples and tool interfaces were reviewed without
changes: the corrections change evidence handling, not silicon facts, ownership,
supported parts or public call signatures. Existing examples compile and run.
Final maintainability review keeps one collector, one slot and one shared
configuration/restoration implementation; it removes two error substitutions
instead of adding a new result/state mechanism. All confirmed in-scope defects
are corrected. Later features and physical qualification remain separate work.
