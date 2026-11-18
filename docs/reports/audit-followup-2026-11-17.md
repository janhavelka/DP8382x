# Audit follow-up maintenance, 2026-11-17

Reverified all 17 items in the user's supplied follow-up at clean synchronized
`main` **955ecf3181e2bd4954beaa097dfdd4243c7b0964**. The attachment describes
older cb5ffe5 observations; those were inputs to check, not assumed current facts.
This maintenance changes no numbered stage status, supported part or SDK baseline.
Root/parent AGENTS, COMMON, architecture, variants, reference index and applicable
contracts were read. Only the BISCR fix and equivalent private helper change code.

## Checkpoints

| Unit | Commit | Sync evidence |
| --- | --- | --- |
| BISCR fix, regression and private helper | `3fe72bee7803e0d2dedde81faea08624d6e29ec4` | Fetch/push/refetch; equal upstream IDs,0/0 |
| Contracts and source/native/variant guides | `681a10ebf9e74aec74c71d097cca238f4070bda5` | Fetch/push/refetch; equal upstream IDs,0/0 |
| Prompt corrections and deferred acceptance | `380cd83edc09a54c3db97d4d680a3a55f23e9635` | Fetch/push/refetch; equal upstream IDs,0/0 |
| These records | Final handoff | Avoid a self-referencing commit; handoff records final clean sync and SDK identity |

## Individual responses

| Item | Verdict | Verified finding and action |
| --- | --- | --- |
| 1 | DONE | Actual Electrical PRBS10 preserve mask was 8EE0, contrary to original SNLS647I table 8-40 p69 bit 8 R/reset 1. 3fe72be uses 8FE0; verify 701F and BIST remain unchanged. `test/electrical.cpp` models RO bit 8 and asserts literal 7101 on the wire plus verified controls. Register vectors and ledger already described preserving 0100 and need no numeric change. Earlier G5-6/current-code response missed this defect. |
| 2 | DONE | `api-contracts.md` copied shipped SettingsSnapshot/StrapEvidence without current fields. 681a10e removes both type blocks and copied signatures, links header Doxygen and replaces 15 feature restatements with guide links while retaining anchors. Unique unexpected-AN/non-IEEE decoding and owned reset-polling details moved to reconciliation/direct-access. The stale stage 17 mismatch-priority claim was removed: actual code and tests retain mismatch evidence but give later transport/release/clock/deadline failure precedence. The dangling native-owner link is repaired. |
| 3 | DONE | Two look-aheads exist: reset command/first read and EEE 75/71 frames, 21/20 callbacks. 681a10e documents exact formulas and calibration's two-frame command. 3fe72be replaces exactly five 12-flag chains with `_featureProcedureActive()`, preserving every surrounding condition. |
| 4 | DONE | Four archived datasheets contain no W1C field;380cd83 removes the unqualified FULL_COVERAGE inventory entry and retains exact-part access authority. |
| 5 | DONE | Ledger mmd_access step 4 and library-ti-core asserted persistence as fact. 681a10e labels it inferred from SNLS647I 8.3.11.1, not a printed combined RMW sequence; physical Q4 remains open. No transaction behavior changes. |
| 6 | DONE | Raw RXFS status exists, but the reserved RXFCFG1[13] reference remains unresolved. 681a10e changes the gate's next owner to TI clarification plus 73 WoL/error fixture and updates all three feature rows. 380cd83 adds fixture acceptance without experimental reserved-bit writes. No TI reply or hardware evidence is fabricated. |
| 7 | PARTLY ALREADY FIXED; DONE | MDC 40/MDIO 39/reset 54/clock 44 and install-with-no-handle versus installed-resource retention were already correct. 681a10e adds six RMII data pins, pinned IO_MUX citation and removes residual deletion wording. 380cd83 fixes prompt 40's typo. Final-netlist/power/clock qualification remains required. |
| 8 | DONE WITH QUALIFICATION | Actual d2ed96d2 patch only changes stop to blocking timer stop. 681a10e corrects the blanket unbounded-wait rejection: bound depends on callback/backend/scheduler qualification. Re-audit fence and inactive-timer behavior on a future pin; this does not close RX lifetime or prove measured wall time. |
| 9 | DONE WITH QUALIFICATION |681a10e fixes FX citation to SNLS505H 7.4.6 and removes invented I/E software deltas. Keep temperature-dependent electrical limits as well as temperature range; P56 already does. 380cd83 adds DP83822 RXFS W1S neutral-zero status writes while retaining table 8-88 selector 12 reset 0. The attachment conflated existing RXFCFG[11] W1S acknowledgement with an already-written RXFS status rule; these are distinct. |
| 10 | DONE |380cd83 changes only the requested historical P10 sentence: preserve six-field library.json; SDK dependencies belong in component metadata. |
| 11 | ASSIGNED 76; MEASURED NOW | GNU 15.1 x64 confirms OperationResult 1864B, OperationSnapshot 96B, OwnerSession 1984B plus local result 1864B = 3848B combined object storage, not measured optimized stack. 681a10e records this; 380cd83 requires compact per-kind evidence, ABI size regression and coordinated consumers/Doxygen, and forbids further embedded result copies in COMMON. A union of full feature results is insufficient: CableResult 520B. No public layout changes now. |
| 12 | ASSIGNED 53 | Per-kind reset/calibration/EEE checks still live in `_execute`. 380cd83 assigns extraction preserving fresh callback-entry resampling, strict bounds, ordering/effects and unchanged regressions. Source-line quotas are not acceptance; checking only the earlier poll sample would break the executing clock-jump case. |
| 13 | ASSIGNED 55; COUNTS CORRECTED | At 955ecf3 there are 23 top-level Controller suites and 257 same-line bind CHECK sites, not 22/251. Shared FakeController is used by identity/mmd/transport; BoundDriver only access. 380cd83 assigns common mechanical harness/binding consolidation, preserving named independent wire oracles and all assertions. Baseline static CHECK invocations: 5985 in test cpp files plus 12 in support headers=5997, excluding macro definition; P55 remeasures static/executing counts before/after. |
| 14 | PARTLY ALREADY TESTED; ASSIGNED 54 | The exact 100us/2ms and 100Hz/5ms repeated driver loops are absent. `test/operations.cpp` already tests short-slot terminal DEADLINE at maxFrames=0 with zero callbacks. 380cd83 preserves it, adds the missing BOUND_ADMISSION counterpart and names both real driver-loop scenarios. Python arithmetic alone is not executing driver evidence. |
| 15 | PARTLY ALREADY FIXED; DONE | Historical/current note and WP0-WP7 application record already existed. This records unit adds all three prerequisite-report links and explicit prohibition on reapplying sections 2,4,5; findings remain unchanged. |
| 16 | DONE | Added a dated [disposition addendum](promptset-audit-dispositions.md#follow-up-addendum-2026-11-17), including corrected earlier claims, code/docs/prompt commits and future owners. Existing table rows remain byte-for-byte unchanged in this follow-up. |
| 17 | DONE | Separate Unreleased bullets in 3fe72be and 681a10e cover code/regression/helper and documentation/record/assignment maintenance. |

## Source and independent review

Original PDFs were reopened, including visual table checks: SNLS647I p69 BISCR,
p35 selectors, pp91-92 RXFCFG1/RXFS conflict, pp10/14 I/E temperature and TA-keyed
leakage; SNLS505H p41 FX and pp92-93 RXFS. The preserved archive/provenance is the
source, not old audit summaries. No vendor payload was changed.

Native source is unmodified v6.1 `fff9895c82d744c7237be8847347bdd1b07c6643`.
The official d2ed96d2 patch and local timer/MAC/driver source were checked; exact
links are in library-sdk and the board guide. Board document source remains
clean ee16c0ed32356cd87e8efec2d6fd5aa5988d2096; that is not a final-netlist audit.
Live read-only sibling seams: TMP1x2 afd50889312f6e218a6a34eca687e3e8547d8b40
(Status/result/jobs),24Cxx1bd9f969d1df55ca5819b0d290ff331869e7d5bc (shared CLI
fixture), PCA9555 0aec221 and INA228 8e439d6 (lifetime/transport). Shared mechanical
fixtures transfer; independent device oracles and Ethernet RX lifetimes do not.

Lead owns headers/contracts/prompt edits/Git. gpt-6-astra/high independently
checked silicon and implemented only Electrical/test changes; gpt-6.1-sol/high
reviewed native/contracts and structures/tests in separate read-only tasks.
Models were chosen under AGENTS for source/lifetime reasoning. Final independent
review confirmed helper equivalence, RO regression quality, OP-6 arithmetic,
retained normative guide coverage and the future acceptance scopes.

## Verification

Windows PowerShell; GNU MinGW-w64 15.1.0, CMake 4.0.1/Ninja, Python 3.12.10.
Release assertions remain active. No production input changed after these builds.

| Layer | Result | Command/evidence |
| --- | --- | --- |
| Host Release | PASS | `cmake -S . -B build/host -G Ninja -DCMAKE_BUILD_TYPE=Release -DDP8382X_BUILD_TESTS=ON`; build; CTest with `--output-on-failure --no-tests=error`: 236/236 |
| Host UBSan trap | PASS | Same configure/build/CTest in build/followup-ubsan with `-DCMAKE_CXX_FLAGS=-fsanitize=undefined -fsanitize-undefined-trap-on-error`: 236/236 |
| Portable build-tree consumer | PASS | Separate test/consumer build/followup-consumer, tests OFF, registered CTest: 2/2 |
| Native host | PASS | test/esp_idf build/response-foundation-native, pinned SDK closure 45 files, both 1000Hz/100Hz: 63/63 |
| Actual SDK link | PASS | `scripts\idf.cmd -C test/esp_idf/adapter_link -B C:\esp\build\dp8382x-p39 build`; P4 image, unmodified pin; clean final-commit identity recorded in handoff |
| Negative control | PASS | Ignored copied source with old 8EE0 mask compiles but electrical traces fails exactly two literal wire checks (21602 checks). Correct source passes all 7 electrical CTests. |
| Python/ledger | PASS | 103 unittest cases; ledger 97 registers/52 procedures/35 features; research structure does not prove hardware |
| Doxygen/version/references/diff | PASS | 289 HTML pages and 2 rejected negative fixtures; generated-version check; archived hashes/local links; diff whitespace |
| Installed/exported package | NOT RUN | Future packaging acceptance; build-tree consumer is separately scoped |
| Remote CI | NOT RUN | First-execution prompt 71 policy; no dispatch/status/wait |
| Physical | NOT RUN | No flash, fixture or TI clarification obtained; Q4/Q19 and other physical gates remain |
| Installed reusable teardown | BLOCKED | native_rx_quiescence unchanged; resources retained until restart |

The size probe is reproducible with g++ C++17 against include/DP8382x/DP8382x.h;
ignored source/binary are build/audit-size-probe/size.cpp and size.exe. Build and
negative-control logs remain under ignored build directories. No test result is
inferred from an old summary. README/architecture already describe the admitted
scope and need no edits; affected API/feature/reference/example guides, changelog
and progress are coordinated. Future items 11-14 remain assigned requirements,
not implementation claimed by this maintenance.
